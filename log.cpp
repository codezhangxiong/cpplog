#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include "log.h"
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>
using namespace std;

Log::Log()
{
    m_count = 0;
    m_is_async = false;
}

Log::~Log()
{
    if (m_fp != NULL)
    {
        fclose(m_fp);
    }
}

bool Log::init()
{
	if (access("log", F_OK))
	{
		if (mkdir("log", S_IRUSR | S_IWUSR | S_IXUSR))
		{
			printf("mkdir fail!\n");
			return false;
		}
	}

    return init("log/RunLog", 0);
}

//异步需要设置阻塞队列的长度，同步不需要设置
bool Log::init(const char* file_name, int close_log, int log_buf_size, int split_lines, int max_queue_size)
{
	if (max_queue_size >= 1) //异步
	{
		m_is_async = true;
		m_log_queue = new block_queue<string>(max_queue_size);
		pthread_t tid;
		//flush_log_thread为回调函数,这里表示创建线程异步写日志
		pthread_create(&tid, NULL, flush_log_thread, NULL);
	}

	m_close_log = close_log;
	m_log_buf_size = log_buf_size;
	m_buf = new char[m_log_buf_size];
	memset(m_buf, '\0', m_log_buf_size);
	m_split_lines = split_lines;

	time_t t = time(NULL);
	struct tm* sys_tm = localtime(&t);
	struct tm my_tm = *sys_tm;
	m_today = my_tm.tm_mday;

	//设置带日期的日志名
	const char* p = strrchr(file_name, '/');
	if (p == NULL)//没有指定目录的情况
	{
		strcpy(log_name, file_name);
		memset(dir_name, '\0', sizeof(dir_name));
	}
	else
	{
		strcpy(log_name, p + 1);
		strncpy(dir_name, file_name, p - file_name + 1);
	}

	char log_name_temp[256] = { 0 };
	char log_full_name[256] = { 0 };
	int num = 0;  //有多个日志文件时，文件后缀序号
	char log_name_suffix[8] = { 0 };

	snprintf(log_name_temp, 255, "%s%d_%02d_%02d_%s", dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, log_name);
	strcpy(log_full_name, log_name_temp);

	while (!access(log_name_temp, F_OK)) //循环判断应该打开的日志文件
	{
		strcpy(log_full_name, log_name_temp);
		sprintf(log_name_suffix, ".%d", ++num);
		snprintf(log_name_temp, 255, "%s%d_%02d_%02d_%s%s", dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, log_name, log_name_suffix);
	}

	m_fp = fopen(log_full_name, "a+");
	if (m_fp == NULL)
	{
		return false;
	}

	//统计原日志中的行数
    if (num > 0)
    {
        m_count = m_split_lines * (num - 1);

        while (true)
        {
            int c = fgetc(m_fp);
            if ('\n' == c) m_count++;
            else if (EOF == c) break;
        }
    }

	return true;
}

void Log::write_log(int level, const char* fname, int line, const char* format, ...)
{
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    time_t t = now.tv_sec;
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    char s[16] = {0};
    switch (level)
    {
    case 0:
        strcpy(s, "[debug]");
        break;
    case 1:
        strcpy(s, "[info]");
        break;
    case 2:
        strcpy(s, "[warn]");
        break;
    case 3:
        strcpy(s, "[erro]");
        break;
    default:
        strcpy(s, "[info]");
        break;
    }
    //写入一个log，对m_count++, m_split_lines最大行数
    m_mutex.lock();
    m_count++;

    //需要新建日志的情况（1.非当日的日志、2.超过最大行数）
    if (m_today != my_tm.tm_mday || m_count % m_split_lines == 0)
    {
        char new_log[256] = {0};
        fflush(m_fp);
        fclose(m_fp);
        char tail[16] = {0};
       
        snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);
       
        if (m_today != my_tm.tm_mday)
        {
            snprintf(new_log, 255, "%s%s%s", dir_name, tail, log_name);
            m_today = my_tm.tm_mday;
            m_count = 0;
        }
        else
        {
            snprintf(new_log, 255, "%s%s%s.%lld", dir_name, tail, log_name, m_count / m_split_lines);
        }
        m_fp = fopen(new_log, "a");
    }
 
    m_mutex.unlock();

    va_list valst;
    va_start(valst, format);

    string log_str;
    m_mutex.lock();

    //写入的具体时间内容格式
    int a = snprintf(m_buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                     my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                     my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);
    int b = sprintf(m_buf + a, "%s(%d): ", strrchr(fname, '/') ? strrchr(fname, '/') + 1 : fname, line);
    int c = vsnprintf(m_buf + a + b, m_log_buf_size - 1, format, valst);

    m_buf[a + b + c] = '\n';
    m_buf[a + b + c + 1] = '\0';
    log_str = m_buf;

    m_mutex.unlock();

    if (m_is_async && !m_log_queue->full())
    {
        m_log_queue->push(log_str);
    }
    else
    {
        m_mutex.lock();
        fputs(log_str.c_str(), m_fp);
        m_mutex.unlock();
    }

    va_end(valst);
}

void Log::flush(void)
{
    m_mutex.lock();
    //强制刷新写入流缓冲区
    fflush(m_fp);
    m_mutex.unlock();
}
