
#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include <string.h>

#define LOG_NOP
#define LOG_TRUE	1

#define __FILENAME__ (strrchr(__FILE__,'/') ? strrchr(__FILE__, '/') +1 : __FILE__)

#define CONSOLE_LOG(output,condition, action,level,fmt, ...)		\
	do{						\
		if ((condition))  	\
		{ 					\
			fprintf(output,"[%s %s:%d]:" fmt "\n",level,__FILENAME__/*__FILE__*/,__LINE__, ##__VA_ARGS__); \
			action;			\
		};					\
	} while(0)

#define ERROR_LOG(condition, action,fmt, ...)	\
	CONSOLE_LOG(stderr,condition, action,"ERROR",fmt, ##__VA_ARGS__)

#define INFO_LOG(condition, action,fmt, ...) \
	CONSOLE_LOG(stdout,condition, action,"INFO",fmt, ##__VA_ARGS__)

#endif
