依赖关系：
main.cpp --> log.h(log.cpp) --> block_queue.h --> locker.h

main.cpp        主函数
log.h(log.cpp)  日志
block_queue.h   队列
locker.h        锁

block_queue.h 和 locker.h 已经整合进 log.h 文件中

问题：
1.程序执行时，产生多个日志文件，程序重启后，产生的日志仍然会从头写入旧文件

