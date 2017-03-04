/*
 * keepAlive.c
 *
 *  Created on: Mar 1, 2017
 *      Author: melharrar
 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <rte_lcore.h>
#include <rte_ring.h>
#include <rte_mempool.h>
#include <rte_cycles.h>

#include "keep_alive.h"

static inline double
cycles_to_ms(uint64_t cycles)
{
	double t = cycles;
	double hz = rte_get_timer_hz();

	t *= (double)MS_PER_S;
	t /= hz;

	return t;
}

static void _check_not_checked(keep_alive_svr_t* svr){

	int i;
	for(i=0; i<svr->nof_cores; i++){
		if(!svr->clients_info[i].checked){
			svr->clients_info[i].missed_times++;
			if(svr->clients_info[i].missed_times >= svr->max_missed){
				/* Handle missed keep alive */
				svr->clients_info[i].missed_times = 0;
			}
		}
	}
}

static
void _clear_client_info_checked(keep_alive_svr_t* svr){

	int i;
	for(i=0; i<svr->nof_cores; i++){
		svr->clients_info[i].checked = 0;
	}
}

static
keep_alive_client_info_t* _get_client_info_by_core_id( keep_alive_svr_t* svr,
		int core_id){
	int i;

	for(i=0; i<svr->nof_cores; i++){
		if(svr->clients_info[i].last_msg.core_id == core_id)
			return &svr->clients_info[i];
	}
	return 0;
}

void KEEP_ALIVE_free_server(keep_alive_svr_t* svr){

	if(!svr)
		return;

	if(svr->clients_info)
		free(svr->clients_info);

	if(svr->rx_ring)
		rte_ring_free(svr->rx_ring);

	if(svr->msg_pool)
		rte_mempool_free(svr->msg_pool);

	free(svr);
}

keep_alive_svr_t*
KEEP_ALIVE_create_server(int* core_ids, int count,
		const char* server_name, int ms_interval, int max_missed)
{
	keep_alive_svr_t* svr;
	int ring_size;
	int i;
	char ring_name[256];
	char pool_name[256];
	const unsigned flags = 0;
	const unsigned priv_data_sz = 0;

	if(!core_ids || !count || !server_name || !ms_interval)
		return 0;

	/* create server */
	svr = (keep_alive_svr_t*)malloc(sizeof(keep_alive_svr_t));
	if(!svr)
		return 0;
	memset(svr,0,sizeof(keep_alive_svr_t));

	snprintf(ring_name, 256,"%s_ring", server_name);
	snprintf(pool_name, 256,"%s_pool", server_name);

	svr->clients_info = (keep_alive_client_info_t*)malloc(sizeof(keep_alive_client_info_t)*count);
	if(!svr->clients_info)
		goto error;
	memset(svr->clients_info, 0, sizeof(keep_alive_client_info_t)*count);

	for(i=0; i<count; i++){
		svr->clients_info[i].last_msg.core_id = core_ids[i];
	}

	svr->max_missed = max_missed;
	svr->nof_cores = count;

	/* Create the keep_alive ring */
	ring_size = count * max_missed * 2;
	svr->rx_ring = rte_ring_create(ring_name, ring_size, rte_socket_id(), flags);
	if(!svr->rx_ring)
		goto error;

	/* Create the mempool ring */
	svr->msg_pool = rte_mempool_create(pool_name, ring_size,
					sizeof(keep_alive_msg_t), 0, priv_data_sz,
					NULL, NULL, NULL, NULL,
					rte_socket_id(), flags);
	if(!svr->msg_pool)
		goto error;
	return svr;

error:

	KEEP_ALIVE_free_server(svr);
	return 0;
}

keep_alive_client_t*
KEEP_ALIVE_create_client(int core_id, const char* server_name, int ms_interval){

	keep_alive_client_t* client;
	char ring_name[256];
	char pool_name[256];

	client = (keep_alive_client_t*)malloc(sizeof(keep_alive_client_t));
	if(!client)
		return 0;

	memset(client,0,sizeof(keep_alive_client_t));
	snprintf(ring_name, 256,"%s_ring", server_name);
	snprintf(pool_name, 256,"%s_pool", server_name);

	client->tx_ring = rte_ring_lookup(ring_name);
	if(!client->tx_ring)
		goto error;

	client->msg_pool = rte_mempool_lookup(pool_name);
	if(!client->msg_pool)
		goto error;

	client->core_id = core_id;
	client->ms_interval = ms_interval;

	return client;
error:
	KEEP_ALIVE_free_client(client);
	return 0;
}

void KEEP_ALIVE_free_client(keep_alive_client_t* client){

	if(!client)
		return;

	free(client);
}


int KEEP_ALIVE_handle_server(keep_alive_svr_t* svr){

	uint64_t current_cycles;
	double current_ms;
	void *m;
	keep_alive_msg_t * msg;
	keep_alive_client_info_t* p_client_info;

	/* get the current ms */
	current_cycles = rte_get_timer_cycles();
	current_ms = cycles_to_ms(current_cycles);

	/* check if it's time to check keepalives */
	if(svr->last_ms_checked + svr->ms_interval > current_ms)
		return 0;

	_clear_client_info_checked(svr);

	while (rte_ring_dequeue(svr->rx_ring, &m) >= 0){

		msg = (keep_alive_msg_t *)m;

		p_client_info = _get_client_info_by_core_id(svr, msg->core_id);
		if(!p_client_info)
			return -1;

		if(msg->seq_number != p_client_info->last_msg.seq_number + 1){
			/* Handle loose msg */
		}

		if(msg->timestamp > p_client_info->last_msg.timestamp + (svr->ms_interval * 1.2)){
			/* Handle delay*/
		}

		/* put it as checked */
		p_client_info->checked = 1;
		p_client_info->missed_times = 0;

		/* copy the last KA message */
		memcpy(&p_client_info->last_msg, msg, sizeof(keep_alive_msg_t));

		/* free the element */
		rte_mempool_put(svr->msg_pool, m);
	}

	_check_not_checked(svr);

	return 0;
}

int KEEP_ALIVE_handle_client(keep_alive_client_t* client){

	void * m;
	keep_alive_msg_t * msg;
	uint64_t current_cycles;
	double current_ms;
	int ret;

	if(!client)
		return -1;

	/* get the current ms */
	current_cycles = rte_get_timer_cycles();
	current_ms = cycles_to_ms(current_cycles);

	/* check if it's time to send another keepalive */
	if(client->last_ms_sent + client->ms_interval > current_ms)
		return 0;

	ret = rte_mempool_get(client->msg_pool, &m);
	if(ret < 0)
		return -1;

	msg = (keep_alive_msg_t *)m;
	memset(msg, 0,sizeof(keep_alive_msg_t));

	msg->seq_number = client->seq_number +1;
	msg->core_id = client->core_id;
	msg->pid = client->pid;
	msg->core_type = client->core_type;

	if (rte_ring_enqueue(client->tx_ring, msg) < 0) {
		rte_mempool_put(client->msg_pool, msg);
		return -1;
	}

	/* update last timestamp sent and new sequence number */
	client->last_ms_sent = current_ms;
	client->seq_number++;

	return 0;
}
