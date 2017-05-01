/*
 *
 * Mikael Elharrar
 * 23/04/2017
 *
 * dispatcher contains the logic of the core.
 * Two options: simple dispatcher
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "rte_ethdev.h"

#include "pkt_router.h"
#include "allocator.h"
#include "main.h"

#define MAX_PACKETS_BURST	16


#include <linux/if_ether.h>

#if 0
		// move over the vlan
		if((ETHTYPE_VLAN == PP_HTONS(ether_type)) ||
		        (AVIAD_ETHTYPE_QNQ == PP_HTONS(ether_type)))
		{
		    struct eth_vlan_hdr* pvlan=(struct eth_vlan_hdr*)(p->payload);
		    while((ETHTYPE_VLAN == PP_HTONS(pvlan->tpid)) ||
		            (AVIAD_ETHTYPE_QNQ == PP_HTONS(pvlan->tpid)))
		    {
		        pvlan +=1;
		        p->len -= SIZEOF_VLAN_HDR;
		        p->tot_len -= SIZEOF_VLAN_HDR;
		    }
            ether_type = pvlan->tpid;

		    pvlan +=1;
            p->len -= SIZEOF_VLAN_HDR;
            p->tot_len -= SIZEOF_VLAN_HDR;
            p->payload = (void *)pvlan;

		}

#endif

#define ETH_PKT_QINQ1	0x9100		/* deprecated QinQ VLAN [ NOT AN OFFICIALLY REGISTERED ID ] */
#define ETH_PKT_QINQ2	0x9200		/* deprecated QinQ VLAN [ NOT AN OFFICIALLY REGISTERED ID ] */
#define ETH_PKT_QINQ3	0x9300		/* deprecated QinQ VLAN [ NOT AN OFFICIALLY REGISTERED ID ] */
#define ETH_PKT_8021Q	0x8100      /* 802.1Q VLAN Extended Header  */
#define ETH_PKT_IPV6	0x86DD		/* IPv6 over bluebook		*/
#define ETH_PKT_IP		0x0800		/* Internet Protocol packet	*/

#define IS_QINQ(ether_type)	(ETH_PKT_QINQ1 == ether_type || \
		ETH_PKT_QINQ2 == ether_type ||\
		ETH_PKT_QINQ3 == ether_type)

struct __attribute__((__packed__)) eth_vlan_hdr {
	uint16_t prio_vid;
	uint16_t tpid;
};
#define SIZEOF_VLAN_HDR 4

static
uint8_t is_ip_frame(uint8_t* p_frame){

	struct eth_vlan_hdr* pvlan;
	uint16_t ether_type;
	uint8_t* ptr;


	ptr = p_frame;

	/* move over ethernet addr */
	ptr += 2*ETH_ALEN;

	ether_type = ntohs(*(uint16_t*)ptr);

	if(ether_type == ETH_PKT_IP || ether_type == ETH_PKT_IPV6)
		return 1;

	if(!IS_QINQ(ether_type) && ether_type != ETH_PKT_8021Q){
		return 0;
	}

	pvlan=(struct eth_vlan_hdr*)(ptr);
    while((ETH_PKT_8021Q == htons(pvlan->tpid)) || IS_QINQ(htons(pvlan->tpid))){
    	pvlan +=1;
    }

    ether_type = pvlan->tpid;

	if(ether_type == ETH_PKT_IP || ether_type == ETH_PKT_IPV6)
		return 1;

	return 0;
}


static
int _init_if(interface_t* p_interface, int tx){

	int i;

	for(i=0; i< p_interface->count; i++){

		struct if_t* p_if = (struct if_t*)&p_interface->ifs[i];
		if(p_if->is_eth){
			//int err;
			/* init and start eth */
			if(!tx){
				//err = rte_eth_dev_rx_queue_start(p_if->port_id, p_if->q_id);
				//ERROR_LOG(err, return -1, "Failed to start rx queue.");
			}
			else{
				//err = rte_eth_dev_tx_queue_start(p_if->port_id, p_if->q_id);
				//ERROR_LOG(err, return -1, "Failed to start tx queue.");
			}
		}
		else{
			/* flush ring */
			int count;
			int i;

			count = rte_ring_count((const struct rte_ring *)p_if->p_ring);
			for(i=0; i<count; i++){
				struct rte_mbuf *m;
				if(0 == rte_ring_dequeue(p_if->p_ring, (void**)&m)){
					if(m){
						rte_pktmbuf_free(m);
					}
				}
			}
		}
	}/* end for */

	return 0;
}

static
int _init_interfaces(dispatcher_ctx_t* p_dispatcher_ctx){

	int err;

	err = _init_if(&p_dispatcher_ctx->rx_interfaces, 0 /*rx*/);
	ERROR_LOG(err, return -1, "Failed to init dispatcher rx interface.");

	err = _init_if(&p_dispatcher_ctx->tx_interfaces, 1 /*tx*/);
	ERROR_LOG(err, return -1, "Failed to init dispatcher tx interface.");

	err = _init_if(&p_dispatcher_ctx->app_interfaces, 0);
	ERROR_LOG(err, return -1, "Failed to init dispatcher app interface.");

	return 0;
}

static
void _process_packets(dispatcher_ctx_t* p_dispatcher_ctx,
		struct rte_mbuf **rx_pkts, uint16_t nb_packets){

	int i;
	uint8_t* p_data;
	(void)p_dispatcher_ctx;

	for(i=0; i<nb_packets; i++){

		struct rte_mbuf *pkt = rx_pkts[i];

		/* get the packet data */
		p_data = rte_pktmbuf_mtod(pkt,uint8_t*);

		/* parse frame */
		if(is_ip_frame(p_data)){
			/*
			 * The frame contains an ip packet,
			 * so send it to upper level
			 *
			 * */
		}
		else{
			/*
			 * The frame is not relevant for our
			 * upper application, so send it out.
			 *
			 */
		}
	}
}

static
int _process_incoming_packets(dispatcher_ctx_t* p_dispatcher_ctx){

	int i, j;
	uint16_t received;
	struct rte_mbuf *rx_pkts[MAX_PACKETS_BURST] = {0};
	struct rte_mbuf *ring_pkt;

	interface_t* p_ingress = &p_dispatcher_ctx->rx_interfaces;

	for(i=0; i<p_ingress->count; i++){

		memset(rx_pkts[0],0,MAX_PACKETS_BURST*sizeof(struct rte_mbuf *));
		ring_pkt = 0;
		received = 0;

		/* get the interface */
		struct if_t* p_if = (struct if_t*)&p_ingress->ifs[i];

		/* retrieve the packets */
		if(p_if->is_eth){
			/* receive packets from eth */
			received = rte_eth_rx_burst(p_if->port_id, p_if->q_id,
					rx_pkts, MAX_PACKETS_BURST);
		}
		else{
			/* receive packets from ring */
			for(j=0; j<MAX_PACKETS_BURST; j++){
				if(0 != rte_ring_dequeue(p_if->p_ring,(void**)&ring_pkt)){
					break;
				}
				else{
					rx_pkts[received++] = ring_pkt;
				}
			}
		}

		/* process the packets */
		_process_packets(p_dispatcher_ctx, rx_pkts, received);

	}/* end for */
	return 0;
}


int PKT_ROUTER_main_loop(dispatcher_ctx_t* p_dispatcher_ctx){

	int err;

	err = _init_interfaces(p_dispatcher_ctx);
	ERROR_LOG(err, return -1, "Failed to init dispatcher interface.");

	while(1){

		/* get packet from ingress port */
		err = _process_incoming_packets(p_dispatcher_ctx);

		sleep(1);
	}

	return 0;
}
