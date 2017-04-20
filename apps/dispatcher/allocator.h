#pragma once

#include "app_conf.h"

#define MAX_INTERFACES	8

typedef struct
{
	const char* name;
	int is_eth;
	struct rte_ring* p_ring;
	int port_id;
	int q_id;
} if_t;

typedef struct
{
    if_t interface[MAX_INTERFACES];
    int interface_cnt;
} interface_t;

typedef struct
{
	char* app_name;
    uint64_t core_id;
    interface_t ingress_if;
    interface_t egress_if;
    interface_t upper_if;
    interface_t lower_if;
} dispatcher_ctx_t;

dispatcher_ctx_t dispatcher_ctx;

int	ALLOCATOR_create_ressources	(app_conf_t* conf);
//int	ALLOCATOR_start_ressources	(app_conf_t* conf);
