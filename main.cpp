#include <stdio.h>
#include "log.h"

int main(int argc, char* argv[])
{
	//printf("this is git test!");

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