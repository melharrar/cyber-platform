
#include <stdio.h>
#include <string.h>
#include "allocator.h"

#include <rte_eal.h>
#include <rte_common.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ethdev.h>

#include "process_manager.h"
#include "main.h"

#define NOF_DPDK_PARAM	6

static struct rte_eth_conf port_conf = {
	.rxmode = {
		.mq_mode	= ETH_MQ_RX_RSS,
		.split_hdr_size = 0,
		.header_split   = 0, /**< Header Split disabled */
		.hw_ip_checksum = 1, /**< IP checksum offload enabled */
		.hw_vlan_filter = 0, /**< VLAN filtering disabled */
		.jumbo_frame    = 0, /**< Jumbo Frame Support disabled */
		.hw_strip_crc   = 0, /**< CRC stripped by hardware */
	},
	.rx_adv_conf = {
		.rss_conf = {
			.rss_key = NULL,
			.rss_hf = ETH_RSS_IP,
		},
	},
	.txmode = {
		.mq_mode = ETH_MQ_TX_NONE,
	},
};


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
static int _init_dpdk(app_conf_t* conf)
{
	int ret;
	int argc;
	char* argv[NOF_DPDK_PARAM];
	int i;
	char buffer[256];

	argc= 0;

	// At the moment we want to add only -c, -n, --proctype (so 3 items * 2) + app_name
	argc = NOF_DPDK_PARAM;

	for(i=0; i<argc; i++){
		switch(i)
		{
		case 0:{
			argv[i]= (char*)strdup(conf->app_name);
		}
		break;

		case 1:{
			argv[i]= (char*)strdup("-c");
		}
		break;

		case 2:{
			sprintf(buffer,"0x%"PRIx64"", conf->core_mask);
			argv[i]= (char*)strdup(buffer);
		}
		break;

		case 3:{
			argv[i]= (char*)strdup("-n");
		}
		break;

		case 4:{
			sprintf(buffer,"%d ", conf->memory_socket);
			argv[i]= (char*)strdup(buffer);
		}
		break;

		case 5:{
			argv[i]= (char*)strdup("--proc-type=primary");
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

	INFO_LOG(LOG_TRUE, LOG_NOP, "Succedded to init DPDK. Core mask 0x%"PRIx64" ", conf->core_mask);
	return 0;
}

/* Must return 0 if all is OK, otherwise non-null value */
static int _alloc_pkt_pool(void* elem, void* cookie){

	pkt_pool_elem_t* p_elem;
	pkt_pool_conf_t* pkt_pool_conf;

	p_elem = (pkt_pool_elem_t*)elem;
	pkt_pool_conf = (pkt_pool_conf_t*)p_elem->pkt_pool;

	pkt_pools_t* pkt_pools =  (pkt_pools_t*)cookie;

	INFO_LOG(LOG_TRUE, LOG_NOP, "New pkt pool => name:%s, size:%d, cache_size:%d, priv_size:%d, data_room_size:%d, socket_id:%d.",
			pkt_pool_conf->name,
			pkt_pool_conf->size,
			pkt_pool_conf->cache_size,
			pkt_pool_conf->priv_size,
			pkt_pool_conf->data_room_size,
			pkt_pool_conf->socket_id);

	pkt_pools->pkt_pools_tab[pkt_pools->count]=0;

	pkt_pools->pkt_pools_tab[pkt_pools->count] =
			rte_pktmbuf_pool_create((const char *)pkt_pool_conf->name,
			(unsigned) pkt_pool_conf->size,
			(unsigned) pkt_pool_conf->cache_size,
			(uint16_t) pkt_pool_conf->priv_size,
			(uint16_t) pkt_pool_conf->data_room_size,
			(int) pkt_pool_conf->socket_id);

	ERROR_LOG(!pkt_pools->pkt_pools_tab[pkt_pools->count], return -1,"Failed to allocate pktpools.");

	INFO_LOG(LOG_TRUE, LOG_NOP, "Succedded to allocate pkt pool %s.",pkt_pool_conf->name);
	pkt_pools->count++;

	return 0;
}

static int _alloc_pkt_pools(app_conf_t* conf){

	int size;
	LL_ELEMENT_T* ptr_elem;

	size = conf->pkt_pool_list.count;
	INFO_LOG(LOG_TRUE, LOG_NOP,"Allocating %d pktmbufs pools.", size);

	pkt_pools.pkt_pools_tab = (struct rte_mempool **)malloc(sizeof(struct rte_mempool*)* size);
	pkt_pools.count = 0;

	ptr_elem = LL_ForEach(&conf->pkt_pool_list, _alloc_pkt_pool, (void*)&pkt_pools);

	if(ptr_elem){
		return -1;
	}
	return 0;
}


/* Must return 0 if all is OK, otherwise non-null value */
static int _alloc_ring(void* elem, void* cookie){

	ring_elem_t* p_elem;
	ring_conf_t* ring_conf;
	unsigned int flags;

	p_elem = (ring_elem_t*)elem;
	ring_conf = (ring_conf_t*)p_elem->ring;

	rings_t* rings =  (rings_t*)cookie;

	INFO_LOG(LOG_TRUE, LOG_NOP, "New ring => name:%s, size:%d, single_producer:%d, single_consumer:%d, socket_id:%d.",
			ring_conf->name,
			ring_conf->size,
			ring_conf->single_producer,
			ring_conf->single_consumer,
			ring_conf->socket_id);

	rings->rings_tab[rings->count]=0;

	flags = 0;
	if(ring_conf->single_producer)
		flags |= RING_F_SP_ENQ;
	if(ring_conf->single_consumer)
		flags |= RING_F_SC_DEQ;

	rings->rings_tab[rings->count] = rte_ring_create((const char *)ring_conf->name,
			ring_conf->size,
			ring_conf->socket_id,flags);

	ERROR_LOG(!rings->rings_tab[rings->count], return -1,"Failed to allocate pktpools.");
	INFO_LOG(LOG_TRUE, LOG_NOP, "Succedded to allocate ring %s.",ring_conf->name);

	rings->count++;

	return 0;
}


static int _alloc_rings(app_conf_t* conf){

	int size;
	LL_ELEMENT_T* ptr_elem;

	size = conf->ring_list.count;
	INFO_LOG(LOG_TRUE, LOG_NOP,"Allocating %d rings.", size);

	rings.rings_tab = (struct rte_ring **)malloc(sizeof(struct rte_ring *)* size);
	rings.count = 0;

	ptr_elem = LL_ForEach(&conf->ring_list, _alloc_ring, (void*)&rings);

	if(ptr_elem){
		return -1;
	}
	return 0;
}

static int _init_eth_port(void* elem, __attribute__((unused)) void* cookie){

	int i, ret;
	eth_elem_t* p_elem;
	eth_conf_t* eth_conf;

	p_elem = (eth_elem_t*)elem;
	eth_conf = (eth_conf_t*)p_elem->eth_port;

	/* configure eth. device */
	INFO_LOG(LOG_TRUE, LOG_NOP, " Initializing NIC port port-id:%d, promisc:%d nb_rx_q: %d, nb_tx_q: %d.",
				eth_conf->port_id,
				eth_conf->promisc,
				eth_conf->nb_rx_q,
				eth_conf->nb_tx_q);
	ret = rte_eth_dev_configure(
			eth_conf->port_id,
			(uint8_t) eth_conf->nb_rx_q,
			(uint8_t) eth_conf->nb_tx_q,
			&port_conf);
	ERROR_LOG(ret < 0, return -1,"Cannot init NIC port %u (error:%d)",eth_conf->port_id, ret);

	if(eth_conf->promisc){
		rte_eth_promiscuous_enable(eth_conf->port_id);
	}
	else{
		rte_eth_promiscuous_disable(eth_conf->port_id);
	}

	/* Init RX queues */
	for(i=0; i<eth_conf->nb_rx_q; i++ ){

		struct rte_mempool* pool;

		INFO_LOG(LOG_TRUE, LOG_NOP, "Init rx q-id:%d, nb-desc:%d, pkt_pool_name:%s.",
				eth_conf->rx_queues[i].q_id,
				eth_conf->rx_queues[i].nb_desc,
				eth_conf->rx_queues[i].pkt_pool_name);

		pool = rte_mempool_lookup((const char*)eth_conf->rx_queues[i].pkt_pool_name);
		ERROR_LOG(!pool, return -1, "Didn't found pool named %s",(const char*)eth_conf->rx_queues[i].pkt_pool_name);

		ret = rte_eth_rx_queue_setup(eth_conf->port_id, eth_conf->rx_queues[i].q_id,
						(uint16_t) eth_conf->rx_queues[i].nb_desc,
						eth_conf->rx_queues[i].socket_id,
						NULL /* should be struct rte_eth_rxconf, must be added to the config file */,
						pool);
		ERROR_LOG(ret < 0, return -1, "Cannot init RX queue %u for port %u error(%d)",
				eth_conf->rx_queues[i].q_id,
				eth_conf->port_id,
				ret);
	}

	/* Init TX queues */
	for(i=0; i<eth_conf->nb_tx_q; i++ ){

		INFO_LOG(LOG_TRUE, LOG_NOP, "Init tx q-id %d, socket-id:%d, nb-desc:%d.",
				eth_conf->tx_queues[i].q_id,
				eth_conf->tx_queues[i].socket_id,
				eth_conf->tx_queues[i].nb_desc);

		ret = rte_eth_tx_queue_setup(eth_conf->port_id,
				eth_conf->tx_queues[i].q_id,
				(uint16_t) eth_conf->tx_queues[i].nb_desc,
				eth_conf->tx_queues[i].socket_id,
				NULL);
		ERROR_LOG(ret < 0, return -1, "Cannot init TX queue %d for port %d (%d)\n",
				eth_conf->tx_queues[i].q_id, eth_conf->port_id, ret);
	}
	return 0;
}

static int _alloc_eth_ports(app_conf_t* conf){

	int nb_ports;
	int size;
	LL_ELEMENT_T* ptr_elem;

	size = conf->eth_port_list.count;
	if(!size) return 0;

	INFO_LOG(LOG_TRUE, LOG_NOP,"Initializing %d eth ports.", size);

	nb_ports = rte_eth_dev_count ();
	ERROR_LOG(nb_ports < size, return -1,"eth dev count found %d ports. %d configured.", nb_ports, size);

	INFO_LOG(LOG_TRUE, LOG_NOP,"Found %d DPDK ports in the system.", nb_ports);

	ptr_elem = LL_ForEach(&conf->eth_port_list, _init_eth_port, 0);

	if(ptr_elem){
		return -1;
	}

	return 0;
}


static int _start_eth_ports( void ){

	int nb_ports, i, err;

	nb_ports = rte_eth_dev_count ();
	ERROR_LOG(nb_ports <= 0, return 0,"No eth. ports to start.");

	for(i=0; i<nb_ports; i++){
		err = rte_eth_dev_start(i);
		ERROR_LOG(err < 0, return -1, "Cannot start port %d error(%d)", i, err);
	}

	return 0;
}


int ALLOCATOR_create_ressources(app_conf_t* conf){

	int err;

	ERROR_LOG(!conf, return -1,"Configuration struct is null.");

	/* init dpdk */
	err = _init_dpdk(conf);
	ERROR_LOG(err, return -1, "Failed to init dpdk.");

	/* allocate pkt mbuf */
	err = _alloc_pkt_pools(conf);
	ERROR_LOG(err, return -1, "Failed to alloc pkt pools.");

	/* allocate rings */
	err = _alloc_rings(conf);
	ERROR_LOG(err, return -1, "Failed to alloc rings.");

	/* allocate eth ports */
	err = _alloc_eth_ports(conf);
	ERROR_LOG(err, return -1, "Failed to alloc eth.");

	return err;
}

int ALLOCATOR_start_ressources(app_conf_t* conf){

	int ret;

	/* start eth ports */
	ret = _start_eth_ports();
	ERROR_LOG(ret, return -1, "Failed to start eth.");

	/* start dpdk apps */
	ret = PROCESS_MNGR_start(&conf->dpdk_app_list);
	ERROR_LOG(ret, return -1, "Failed to start DPDK apps.");

	return ret;
}
