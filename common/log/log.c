/*
 * log.c
 *
 *  Created on: March 5, 2017
 *      Author: melharrar
 */
#include "log.h"
#include "stdarg.h"

static FILE * f_log_info = 0;
static FILE * f_log_error = 0;
static int local_level = 0;

int LOG_init(const char *log_path){

	int err;
	int len;
	char* fname;

	fname = 0;

	/* Open info file */
	len = strlen(log_path);
	len += strlen("/cyber_info.log");
	len += 2;

	fname = (char*)malloc(len);
	sprintf(fname,"%s/cyber_info.log",log_path);

	f_log_info = fopen(fname, "a+");
	if(!f_log_info){
		goto error;
	}

	free(fname);
	fname = 0;

	/* Open error file */
	len = strlen(log_path);
	len += strlen("/cyber_error.log");
	len += 2;

	fname = (char*)malloc(len);
	sprintf(fname,"%s/cyber_error.log",log_path);

	f_log_error = fopen(fname, "a+");
	if(!f_log_error){
		goto error;
	}

	free(fname);
	fname = 0;

	return 0;

error:
	if(fname)
		free(fname);
	if(f_log_info)
		fclose(f_log_info);
	if(f_log_error)
		fclose(f_log_error);

	return -1;
}

static int
_vlog(int level, const char *format, va_list ap)
{
	FILE *f;
	int ret;

	if ((level > local_level))
		return 0;

	if(level == LOG_INFO || level == LOG_DEBUG || level == LOG_NOTICE){
		f = f_log_info;
	}
	else{
		f = f_log_error;
	}

	if(!f)
		return -1;

	ret = vfprintf(f, format, ap);
	fflush(f);
	return ret;
}

int
LOG_print(int level, const char *format, ...)
{
	va_list ap;
	int ret;

	va_start(ap, format);
	ret = _vlog(level, format, ap);
	va_end(ap);
	return ret;
}
