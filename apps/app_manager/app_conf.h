#pragma once

#include "llist.h"

#define MAX_CORE_SLAVE 64

typedef struct 
{
	char* name;
	int size;
	int cache_size;
	int priv_size;
	int data_room_size;
	int socket_id;
} pkt_pool_conf_t;

typedef struct
{
	LL_ELEMENT_T elem;
	pkt_pool_conf_t* pkt_pool;
}pkt_pool_elem_t;


typedef struct 
{
	char* name;
	int size;
	int single_producer;
	int single_consumer;
	int socket_id;
} ring_conf_t;

typedef struct
{
	LL_ELEMENT_T elem;
	ring_conf_t* ring;
}ring_elem_t;

typedef struct
{
	int socket_id;
	int q_id;
	int nb_desc;
	char* pkt_pool_name; // for rx only
}eth_q_t;

typedef struct
{
	int port_id;
	int promisc;
	int nb_rx_q;
	int nb_tx_q;
	eth_q_t* tx_queues;
	eth_q_t* rx_queues;
} eth_conf_t;

typedef struct
{
	LL_ELEMENT_T elem;
	eth_conf_t* eth_port;
}eth_elem_t;

typedef struct
{
    char* app_name;
    uint64_t core_mask;
    int memory_socket;
    int slave_core_ids[MAX_CORE_SLAVE];
    int slave_core_count;
    LL_T eth_port_list;
	LL_T pkt_pool_list;
	LL_T ring_list;
	//LL_T dpdk_app_list;
} app_conf_t;

app_conf_t CONF;

int APP_CONF_parse_file( const char * filename );

