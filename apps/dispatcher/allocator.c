/*
 *
 * Mikael Elharrar
 * 16/04/2017
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include <rte_eal.h>
#include <rte_common.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ethdev.h>

#include "main.h"
#include "allocator.h"
#include "app_conf.h"


static
int _store_if_in_ctx(void* elem, void* cookie){

	int count;
	if_conf_t* p_if_conf;
	if_elem_t* p_elem;


	p_elem = (if_elem_t*)elem;
	p_if_conf = p_elem->p_interface;

	interface_t* p_interface = (interface_t*)cookie;

	if(!p_if_conf->used)
		return 0;

	count = p_interface->interface_cnt;
	p_interface->interface[count].name = p_if_conf->name;

	if(p_if_conf->is_eth){
		//Store eth
		p_interface->interface[count].port_id = p_if_conf->port_id;
		p_interface->interface[count].q_id = p_if_conf->q_id;
	}
	else{
		// Loockup ring
		p_interface->interface[count].p_ring = rte_ring_lookup (p_if_conf->ring_name);
		ERROR_LOG(!p_interface->interface[count].p_ring, return -1,"Failed to found ring %s.", p_if_conf->ring_name);
		INFO_LOG(LOG_TRUE, LOG_NOP, "Succedded to attach ring %s.",p_if_conf->ring_name);
	}

	p_interface->interface_cnt++;

	return 0;
}

static
int _create_app_ctx(app_conf_t* p_conf){

	LL_ELEMENT_T * p_elem;

	// init the context
	memset(&dispatcher_ctx,0,sizeof(dispatcher_ctx_t));
	dispatcher_ctx.app_name = p_conf->app_name;
	dispatcher_ctx.core_id = p_conf->core_id;

	p_elem = LL_ForEach(&p_conf->ingress_if_list, _store_if_in_ctx, &dispatcher_ctx.ingress_if);
	ERROR_LOG(p_elem, return -1,"Failed to create ctx for ingress interface.");

	p_elem = LL_ForEach(&p_conf->egress_if_list, _store_if_in_ctx, &dispatcher_ctx.egress_if);
	ERROR_LOG(p_elem, return -1,"Failed to create ctx for egress interface.");

	p_elem = LL_ForEach(&p_conf->upper_app_if_list, _store_if_in_ctx, &dispatcher_ctx.upper_if);
	ERROR_LOG(p_elem, return -1,"Failed to create ctx for upper interface.");

	p_elem = LL_ForEach(&p_conf->lower_app_if_list, _store_if_in_ctx, &dispatcher_ctx.lower_if);
	ERROR_LOG(p_elem, return -1,"Failed to create ctx for lower interface.");

	return 0;
}

/*
 * ./rte-app -c COREMASK [-n NUM] [-b <domain:bus:devid.func>]
 * [--socket-mem=MB,...] [-m MB] [-r NUM] [-v] [--file-prefix]
 * [--proc-type <primary|secondary|auto>] [-- xen-dom0]
 *
 * The EAL options are as follows:
 *
 * -c COREMASK: An hexadecimal bit mask of the cores to run on. Note that core numbering can change between platforms and should be determined beforehand.
 * -n NUM: Number of memory channels per processor socket.
 * -b <domain:bus:devid.func>: Blacklisting of ports; prevent EAL from using specified PCI device (multiple -b options are allowed).
 * --use-device: use the specified Ethernet device(s) only. Use comma-separate [domain:]bus:devid.func values. Cannot be used with -b option.
 * --socket-mem: Memory to allocate from hugepages on specific sockets.
 * -m MB: Memory to allocate from hugepages, regardless of processor socket. It is recommended that --socket-mem be used instead of this option.
 * -r NUM: Number of memory ranks.
 * -v: Display version information on startup.
 * --huge-dir: The directory where hugetlbfs is mounted.
 * --file-prefix: The prefix text used for hugepage filenames.
 * --proc-type: The type of process instance.
 * --xen-dom0: Support application running on Xen Domain0 without hugetlbfs.
 * --vmware-tsc-map: Use VMware TSC map instead of native RDTSC.
 * --base-virtaddr: Specify base virtual address.
 * --vfio-intr: Specify interrupt type to be used by VFIO (has no effect if VFIO is not used).
*/
static
int _init_dpdk(app_conf_t* p_conf)
{
#define NOF_DPDK_PARAM 6
	int ret;
	int argc;
	char* argv[NOF_DPDK_PARAM];
	int i;
	char buffer[256];
	uint64_t core_mask;
	argc= 0;

	core_mask = 1;
	core_mask <<= p_conf->core_id;

	// At the moment we want to add only -c, -n, --proctype (so 3 items * 2) + app_name
	argc = NOF_DPDK_PARAM;

	for(i=0; i<argc; i++){
		switch(i)
		{
		case 0:{
			argv[i]= (char*)strdup(p_conf->app_name);
		}
		break;

		case 1:{
			argv[i]= (char*)strdup("-c");
		}
		break;

		case 2:{
			sprintf(buffer,"0x%"PRIx64"", core_mask);
			argv[i]= (char*)strdup(buffer);
		}
		break;

		case 3:{
			argv[i]= (char*)strdup("-n");
		}
		break;

		case 4:{
			/* TODO: Need to add a configurable parameter */
			sprintf(buffer,"1"/*"%d ", p_conf->memory_socket*/);
			argv[i]= (char*)strdup(buffer);
		}
		break;

		case 5:{
			argv[i]= (char*)strdup("--proc-type=secondary");
		}
		break;
		default:
			break;
		}
	}

	ret = rte_eal_init(argc, argv);
	if (ret < 0){
		return -1;
	}

	INFO_LOG(LOG_TRUE, LOG_NOP, "Succedded to init DPDK. Core mask 0x%"PRIx64" ", core_mask);
	return 0;
}


int	ALLOCATOR_create_ressources	(app_conf_t* p_conf){

	int err;

	ERROR_LOG(!p_conf, return -1,"Configuration struct is null.");

	/* init dpdk */
	err = _init_dpdk(p_conf);
	ERROR_LOG(err, return -1, "Failed to init dpdk.");

	err = _create_app_ctx(p_conf);
	ERROR_LOG(err, return -1, "Failed to create app ctx.");

	return 0;
}
