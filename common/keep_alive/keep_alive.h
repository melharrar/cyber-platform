/*
 * keepAlive.h
 *
 *  Created on: Mar 1, 2017
 *      Author: melharrar
 */

#ifndef _KEEP_ALIVE_H_
#define _KEEP_ALIVE_H_

typedef struct
{
	int seq_number;
	int core_id;
	int core_type;
	int cpu_load;
	int pid;
	double timestamp;
} keep_alive_msg_t;

typedef struct {
	keep_alive_msg_t last_msg;
	int missed_times;
	int checked;
} keep_alive_client_info_t;

typedef struct {
	struct rte_ring*			rx_ring;
	struct rte_mempool*			msg_pool;
	keep_alive_client_info_t*	clients_info;
	int max_missed;
	int nof_cores;
	int ms_interval;
	double last_ms_checked;
} keep_alive_svr_t;

typedef struct
{
	int core_id;
	int core_type;
	struct rte_ring*	tx_ring;
	struct rte_mempool*	msg_pool;
	int seq_number;
	int pid;
	int ms_interval;
	double last_ms_sent;
} keep_alive_client_t;

typedef enum {
	CORE_INIT,
	CORE_RUNNING,
	CORE_DONT_UPDATE,
	CORE_ERROR
} core_status_t;

typedef struct {
	int core_id;
	core_status_t status;
} keep_alive_result_t;

/*
 * Create a keepalive server.
 * Each secondary core will send him a keepalive messsage during the execution.
 *
 * @param core_ids:		The secondary cores id.
 * @param count:		The number of cores in core_ids table.
 * @param server_name:	The name of DPDK ring to send/receive the msg.
 * @param ms_interval:	Interval between keep alive msg in ms.
 * @param max_missed:	Max missed times before error.
 *
 * */
keep_alive_svr_t*
KEEP_ALIVE_create_server(int* core_ids, int count, const char* server_name, int ms_interval, int max_missed);

void KEEP_ALIVE_free_server(keep_alive_svr_t* svr);

/*
 * Create a keepalive client.
 *
 * @param core_ids:		The cores id of the client.
 * @param ring_name:	The name of DPDK ring to send/receive the msg.
 * @param ms_interval:	Interval between keep alive msg in ms.
 *
 * */
keep_alive_client_t* KEEP_ALIVE_create_client(int core_id, const char* ring_name, int ms_interval);

void KEEP_ALIVE_free_client(keep_alive_client_t* client);

/*
 * Those functions should be called:
 *  - from the main loop.
 * 	- by the primary process which created the server.
 * 	- by the secondary process which created the client.
 *
 * @param svr:			previously created server.
 * @param client:		previously created client.
 *
 * */
int KEEP_ALIVE_handle_server(keep_alive_svr_t* svr);
int KEEP_ALIVE_handle_client(keep_alive_client_t* client);

#endif /* INIT_H_ */
