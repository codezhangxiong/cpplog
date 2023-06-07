#include <stdio.h>
#include "log.h"
#include <unistd.h>

int main(int argc, char* argv[])
{
	setbuf(stdout, NULL);

	char cwd[512] = { 0 };
	getcwd(cwd,sizeof(cwd));
	printf("run path: %s\n", cwd);

	//使用log时，需要初始化
	if (Log::get_instance()->init() == false)
	{
		printf("log init fail!\n");
		return -1;
	}

	int count = 0;
	if (argc == 2)
	{
		count = atoi(argv[1]);
	}
	else
	{
		printf("please input data count!\n");
	}

	srand(time(NULL));
	for (int i = 0; i < count; i++)
	{
		LOG_DEBUG("%08d %02X %02X %02X %02X %02X %02X %02X %02X  %02X %02X %02X %02X %02X %02X %02X %02X", i,
			rand() % 16, rand() % 16, rand() % 16, rand() % 16, rand() % 16, rand() % 16, rand() % 16, rand() % 16,
			rand() % 16, rand() % 16, rand() % 16, rand() % 16, rand() % 16, rand() % 16, rand() % 16, rand() % 16
		);
	}

	return 0;
}