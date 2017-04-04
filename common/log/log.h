
#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_NOP
#define LOG_TRUE	1

enum{
	LOG_INFO = 0,
	LOG_DEBUG,
	LOG_NOTICE,
	LOG_WARNING,
	LOG_ERROR,
	LOG_FATAL,
	LOG_MAX_LEVEL
};

#define __FILENAME__ (strrchr(__FILE__,'/') ? strrchr(__FILE__,'/')+1 : __FILE__ )

int LOG_init(const char *log_path);
int LOG_print(int level, const char *format, ...);

#define FATAL_LOG(condition, action,fmt, ...)		\
	do{						\
		if ((condition))  \
		{\
			LOG_print(LOG_FATAL,"[FATAL %s:%d]:" fmt "\n",__FILENAME__,__LINE__, ##__VA_ARGS__); \
			action;			\
		};					\
	} while(0)

#define ERROR_LOG(condition, action,fmt, ...)		\
	do{						\
		if ((condition))  	\
		{ 					\
			LOG_print(LOG_ERROR,"[ERROR %s:%d]:" fmt "\n",__FILENAME__,__LINE__, ##__VA_ARGS__); \
			action;			\
		};					\
	} while(0)

#define WARN_LOG(condition, action,fmt, ...)		\
	do{						\
		if ((condition))  	\
		{ 					\
			LOG_print(LOG_WARN,"[WARNING %s:%d]:" fmt "\n",__FILENAME__,__LINE__, ##__VA_ARGS__); \
			action;			\
		};					\
	} while(0)

#define NOTICE_LOG(condition, action,fmt, ...)		\
	do{						\
		if ((condition))  	\
		{ 					\
			LOG_print(LOG_NOTICE,"[NOTICE %s:%d]:" fmt "\n",__FILENAME__,__LINE__, ##__VA_ARGS__); \
			action;			\
		};					\
	} while(0)

#define DEBUG_LOG(condition, action,fmt, ...)		\
	do{						\
		if ((condition))  	\
		{ 					\
			LOG_print(LOG_DEBUG,"[DEBUG %s:%d]:" fmt "\n",__FILENAME__,__LINE__, ##__VA_ARGS__); \
			action;			\
		};					\
	} while(0)

#define INFO_LOG(condition, action,fmt, ...)		\
	do{						\
		if ((condition))  	\
		{ 					\
			LOG_print(LOG_INFO,"[INFO %s:%d]:" fmt "\n",__FILENAME__,__LINE__, ##__VA_ARGS__); \
			action;			\
		};					\
	} while(0)

#endif
