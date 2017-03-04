/*
 * process_manager.h
 *
 *  Created on: Feb 28, 2017
 *      Author: melharrar
 */

#ifndef PROCESS_MANAGER_H_
#define PROCESS_MANAGER_H_

#include <stdio.h>

#define MAX_PATH_LEN			256
#define COMMAND_LINE_MAX_LEN	512

typedef enum {

	PROCESS_DOWN = 0,
    DURING_RELOAD_OR_INIT,
    PROCESS_UP
} process_state_t;

typedef enum {

    RELOAD_UNKNOWN = 0,
	RELOAD_SUCCESSED,
	RELOAD_FAILED
} process_reload_status_t ;

typedef struct
{
	const char* 		bin_path;
	const char* 		config_file_path;
	int 				core_id;
	int 				linux_pid;
	char 				process_command_line[COMMAND_LINE_MAX_LEN];
	FILE* 				process_output_file;
	process_state_t		process_state;
	char*				p_log;
/*
	struct timeval		m_tvLastInitTime;
	struct timeval		m_tvLastReloadTime;
*/
	process_reload_status_t	reload_status;
	int					reload_pending;
/*
	char* 	m_snortMessageBuffer;
	size_t 	m_snortMessageBufferLen;
	snort_message_event_t m_snortEvent;
*/
}dpdk_app_conf_t;

typedef struct
{
	LL_ELEMENT_T elem;
	dpdk_app_conf_t* dpdk_app;
}dpdk_app_elem_t;

int PROCESS_MNGR_start( LL_T* p_app_list);
int PROCESS_MNGR_scan(LL_T* p_app_list);
#endif /* PROCESS_MANAGER_H_ */
