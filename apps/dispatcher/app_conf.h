/*
 *
 * Mikael Elharrar
 * 16/04/2017
 *
 */
#pragma once

#include "llist.h"

typedef struct
{
	const char* name;
	const char* bypass_if_name;
	int used;
	int is_eth;
	int port_id;
	int q_id;
	const char* ring_name;
} if_conf_t;

typedef struct
{
	LL_ELEMENT_T elem;
	if_conf_t* p_interface;
}if_elem_t;

typedef struct
{
	char* app_name;
    uint64_t core_id;
    LL_T rx_interfaces_list;
    LL_T tx_interfaces_list;
    LL_T app_interfaces_list;
} app_conf_t;

app_conf_t CONF;

int APP_CONF_parse_file( const char * filename );
