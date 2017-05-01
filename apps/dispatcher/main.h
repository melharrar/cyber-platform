#pragma once

#include "log.h"
#include "rte_ring.h"

#define APP_NAME				("dispatcher")

#define MAX_INTERFACES	8

struct if_t
{
	const char* name;
	const char* bypass_name;
	struct if_t* bypass_if;
	int is_eth;
	struct rte_ring* p_ring;
	int port_id;
	int q_id;
};

typedef struct
{
	struct if_t ifs[MAX_INTERFACES];
    int count;
} interface_t;

typedef struct
{
	char* app_name;
    uint64_t core_id;
    interface_t rx_interfaces;
    interface_t tx_interfaces;
    interface_t app_interfaces;
} dispatcher_ctx_t;

dispatcher_ctx_t dispatcher_ctx;
