#pragma once

#include "log.h"
#include <rte_ring.h>


#define APP_NAME				("app_manager")

typedef struct
{
	struct rte_mempool **pkt_pools_tab;
	unsigned int count;
}pkt_pools_t;

pkt_pools_t pkt_pools;

typedef struct
{
	struct rte_ring **rings_tab;
	unsigned int count;
}rings_t;

rings_t rings;

