/*
 * process_manager.c
 *
 *  Created on: Feb 28, 2017
 *      Author: melharrar
 */
#include <unistd.h>
#include <fcntl.h>

#include "main.h"
#include "llist.h"
#include "process_manager.h"

#define MAX_NUMBER_OF_TRIES_TO_PID	20
#define MAX_LOG_MESSAGE_LEN			1024

typedef enum
{
	START_ALL_APPS,
	SCAN_ALL_APPS
} action_t;


static void _set_process_state(dpdk_app_conf_t * app, process_state_t state)
{
	// nothing to do here.
	app->process_state = state;

	switch(state){

	case PROCESS_DOWN:
		break;

	case DURING_RELOAD_OR_INIT:
		break;

	case PROCESS_UP:
		break;

	}
}

static int _get_pid(dpdk_app_conf_t* dpdk_app_conf){

	// lets try to get the pid using ps command.
	//if not exsist we  deduct that the snort process has faild init.
	int pid;
	char *result;
	size_t BuffLen = 1024;
	char command[COMMAND_LINE_MAX_LEN] = {0};
	char str[COMMAND_LINE_MAX_LEN] = {0};
	int nof_tries;
	int lines;
	FILE *fd;

	fd = 0;
	nof_tries = 0;
	result = 0;
	pid = 0;

	while ((pid == 0) &&  (nof_tries <= MAX_NUMBER_OF_TRIES_TO_PID)){

		nof_tries++;
		sprintf(command,"ps aux --cols 512 | grep %s | grep  \"core-id %d\" | grep -v sh", dpdk_app_conf->bin_path, dpdk_app_conf->core_id);
		fd = popen(command,"r");
		usleep(500);
		lines = 0;
		if (!fd)
			continue;

		while(getline(&result,&BuffLen,fd) >= 0)
		{
			lines++;
			sscanf(result,"%s %u",str,&pid);
			INFO_LOG(LOG_TRUE, LOG_NOP, "Found pid %d", pid);
		}

		if (lines > 1){
			INFO_LOG(LOG_TRUE, LOG_NOP, "ps command result contains more than 1 line in result.");
		}

		pclose(fd);
	}

	free(result);

	if(pid == 0)
		return -1;

	//TODO: Check the pid if valid.
	dpdk_app_conf->linux_pid = pid;

    return 0;
}


static int _start_dpdk_app(void* elem, __attribute__((unused)) void* cookie){

	dpdk_app_elem_t* p_elem;
	dpdk_app_conf_t* dpdk_app_conf;
	int flags, fd, ret;

	p_elem = (dpdk_app_elem_t*)elem;
	dpdk_app_conf = (dpdk_app_conf_t*)p_elem->dpdk_app;

	// Format command line
	sprintf(dpdk_app_conf->process_command_line,
			"%s --config-file %s --core-id %d 2>&1",
			dpdk_app_conf->bin_path,
			dpdk_app_conf->config_file_path,
			dpdk_app_conf->core_id);
	INFO_LOG(LOG_TRUE, LOG_NOP, "Starting dpdk-app %s", dpdk_app_conf->process_command_line);

	_set_process_state(dpdk_app_conf, DURING_RELOAD_OR_INIT);

	/* start the process and get the output file */
	dpdk_app_conf->process_output_file = popen(dpdk_app_conf->process_command_line, "re");
	ERROR_LOG(!dpdk_app_conf->process_output_file, return -1, "popen failed to start %s", dpdk_app_conf->process_command_line);

	/* set the file to be non blocking */
	fd = fileno(dpdk_app_conf->process_output_file);
	flags = fcntl(fd, F_GETFL, 0);
	flags |= O_NONBLOCK;
	fcntl(fd, F_SETFL, flags);

	ret = _get_pid(dpdk_app_conf);
	ERROR_LOG(ret, return -1, "Failed to get pid.");

	/* Alloc buffer */
	dpdk_app_conf->p_log = (char*) malloc(MAX_LOG_MESSAGE_LEN);

	return 0;
}


#if 0
static int _check_process_up(dpdk_app_conf_t* dpdk_app_conf)
{
	(void)dpdk_app_conf;
	// TODO: Need to implement another mechanism
	int ps_ret = 0;
	int ret = 0;
	char ps[MAX_SYSTEM_COMMAND_LEN] = {0};

	ERROR_LOG(dpdk_app_conf->process_state == PROCESS_DOWN, return -1,
			"core id %d is down.", dpdk_app_conf->core_id);

	sprintf(ps,"ps -e --cols 256 | grep %u > /dev/null ", dpdk_app_conf->linux_pid);

	ps_ret = system(ps);
	return 0;
}
#endif

static int _check_process_output(dpdk_app_conf_t* dpdk_app_conf)
{
	char* fgets_ret;

	if(!dpdk_app_conf->process_output_file && dpdk_app_conf->process_state != PROCESS_DOWN){
		_set_process_state(dpdk_app_conf, PROCESS_DOWN);
		ERROR_LOG(LOG_TRUE,return -1,
				"trying to read from output file but the fd is null. pid %d",
				dpdk_app_conf->linux_pid);
	}

	fgets_ret = 0;
	dpdk_app_conf->p_log[0] = 0;

	fgets_ret = fgets( dpdk_app_conf->p_log, MAX_LOG_MESSAGE_LEN, dpdk_app_conf->process_output_file);
	if(!fgets_ret){
		return 0;
	}

	INFO_LOG(LOG_TRUE, LOG_NOP, "core id %d: %s", dpdk_app_conf->core_id, dpdk_app_conf->p_log);

	return 0;
}

static int _scan_dpdk_app(void* elem, __attribute__((unused)) void* cookie){

	dpdk_app_elem_t* p_elem;
	dpdk_app_conf_t* dpdk_app_conf;

	p_elem = (dpdk_app_elem_t*)elem;
	dpdk_app_conf = (dpdk_app_conf_t*)p_elem->dpdk_app;

	return _check_process_output(dpdk_app_conf);
}


static int _loop_on_all_process( LL_T* p_app_list, action_t action)
{
	int nb_apps;

	LL_ELEMENT_T* ptr_elem;

	nb_apps = p_app_list->count;
	INFO_LOG(!nb_apps, return 0,"No applications to loop on.");

	switch(action)
	{

	case START_ALL_APPS:
		ptr_elem = LL_ForEach(p_app_list, _start_dpdk_app,0);
		break;
	case SCAN_ALL_APPS:
		ptr_elem = LL_ForEach(p_app_list, _scan_dpdk_app,0);
		break;
	}

	if(ptr_elem){
		return -1;
	}
	return 0;
}

int PROCESS_MNGR_start( LL_T* p_app_list){

	int ret;

	ret = _loop_on_all_process(p_app_list, START_ALL_APPS);
	ERROR_LOG(!ret, return ret,"Failed to start all processes.");

	return ret;
}


int PROCESS_MNGR_scan(LL_T* p_app_list){
	return _loop_on_all_process(p_app_list, SCAN_ALL_APPS);
}
