
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <rte_cfgfile.h>

#include <json-c/json.h>
#include "app_conf.h"
#include "main.h"
#include "process_manager.h"

// I used this file to prototype the json.
//http://www.jsoneditoronline.org/?id=296cdd0556767e2b4d809115199e81dd
#define IS_JSON_STR(jobj)		(json_type_string == json_object_get_type(jobj))
#define IS_JSON_INT(jobj)		(json_type_int == json_object_get_type(jobj))
#define CMP_JSON_STR(jobj,str) 	(!strcmp(str, json_object_get_string(jobj)))

static void _init_conf( app_conf_t* p_conf){

	LL_Init(&p_conf->eth_port_list);
	LL_Init(&p_conf->pkt_pool_list);
	LL_Init(&p_conf->ring_list);
}

static int _parse_pktpools(json_object * jobj){

	int i, ret;
	int arraylen;
	enum json_type type;
	json_object * json_pktpool;

	type = json_object_get_type(jobj);
	ERROR_LOG((type != json_type_array), return -1,"json pktpools object is not an array ...");

	arraylen = json_object_array_length(jobj);

	for (i=0; i< arraylen; i++)
	{
		const char* items[6];
		json_pktpool = json_object_array_get_idx(jobj, i); /*Getting the array element at position i*/
		type = json_object_get_type(json_pktpool);
		ERROR_LOG((type != json_type_object), return -1, "Packetpools: got an json value which is not an object.");

		items[0] = "name";
		items[1] = "private_size";
		items[2] = "data_room_size";
		items[3] = "cache_size";
		items[4] = "socket_id";
		items[5] = "size";

		/* Parse here the attributes */
		pkt_pool_elem_t* pElem = (pkt_pool_elem_t*)malloc(sizeof(pkt_pool_elem_t));
		pElem->pkt_pool = (pkt_pool_conf_t*)malloc(sizeof(pkt_pool_conf_t));
		LL_InitElement(&pElem->elem);


		for(int i=0; i<6; i++){
			json_object * json_val;
			ret = json_object_object_get_ex(json_pktpool, items[i], &json_val);
			ERROR_LOG(!ret, return -1, "Failed to get %s ...", items[i]);

			if(!strcmp(items[i],"name")){
				pElem->pkt_pool->name = strdup((const char *)json_object_get_string(json_val));
			}
			else if(!strcmp(items[i],"private_size")){
				pElem->pkt_pool->priv_size = json_object_get_int(json_val);
			}
			else if(!strcmp(items[i],"data_room_size")){
				pElem->pkt_pool->data_room_size = json_object_get_int(json_val);
			}
			else if(!strcmp(items[i],"cache_size")){
				pElem->pkt_pool->cache_size = json_object_get_int(json_val);
			}
			else if(!strcmp(items[i],"socket_id")){
				pElem->pkt_pool->socket_id = json_object_get_int(json_val);
			}
			else if(!strcmp(items[i],"size")){
				pElem->pkt_pool->size = json_object_get_int(json_val);
			}
			else {
			}
		}
		/* and push the element */
		LListAdd(&CONF.pkt_pool_list, &pElem->elem);
	}
	return 0;
}

static int _parse_eth_queues(json_object * json_q_array, eth_q_t* p_eth_q, int nb_q)
{
	int i, j;
	int ret;
	int type;
	const char* q_items[4];

	type = json_object_get_type(json_q_array);
	ERROR_LOG(type != json_type_array, return -1, "Error: expected json array type.");

	q_items[0] = "q-id";
	q_items[1] = "nb-desc";
	q_items[2] = "pktpool-name"; // for rx only.
	q_items[3] = "socket-id"; // for rx only.

	for(i=0; i< nb_q; i++){

		json_object * json_q_obj;

		// For each q. the index i.
		json_q_obj = json_object_array_get_idx(json_q_array, i); /*Getting the array element at position i*/
		type = json_object_get_type(json_q_obj);
		ERROR_LOG(type != json_type_object, return -1, "q parser got an json value which is not an object.");


		for(j=0; j<4; j++){

			json_object * json_q_item;
			ret = json_object_object_get_ex(json_q_obj, q_items[j], &json_q_item);
			if(!ret){
				continue;
			}
			type = json_object_get_type(json_q_item);

			if(!strcmp(q_items[j],"q-id")){
				ERROR_LOG(type != json_type_int, return -1, "eth. q-id is not a json int value.");
				p_eth_q[i].q_id = json_object_get_int(json_q_item);
			}
			else if(!strcmp(q_items[j],"nb-desc")){
				ERROR_LOG(type != json_type_int, return -1, "eth. nb-tx-desc is not a json int value.");
				p_eth_q[i].nb_desc = json_object_get_int(json_q_item);
			}
			else if(!strcmp(q_items[j],"pktpool-name")){
				ERROR_LOG(type != json_type_string, return -1, "eth. nb-tx-desc is not a json int value.");
				p_eth_q[i].pkt_pool_name = strdup(json_object_get_string(json_q_item));
			}
			else if(!strcmp(q_items[j],"socket-id")){
				ERROR_LOG(type != json_type_int, return -1, "eth socket-id is not a json int value.");
				p_eth_q[i].socket_id = json_object_get_int(json_q_item);
			}
			else{
			}
		}
	}

	return 0;
}

static int _parse_ethports(json_object * jobj){

	int i, ret;
	int arraylen;
	enum json_type type;
	json_object * json_ethport;


	type = json_object_get_type(jobj);
	ERROR_LOG((type != json_type_array), return -1, "json ethport object is not an array ...");

	arraylen = json_object_array_length(jobj);

	for (i=0; i< arraylen; i++)
	{
		// for each ports
		json_ethport = json_object_array_get_idx(jobj, i); /*Getting the array element at position i*/
		type = json_object_get_type(json_ethport);
		ERROR_LOG(type != json_type_object, return -1, "Eth port: got an json value which is not an object.");

		const char* items[4];

		items[0] = "promisc";
		items[1] = "port-id";
		items[2] = "tx-queues";
		items[3] = "rx-queues";

		eth_elem_t* pElem = (eth_elem_t*)malloc(sizeof(eth_elem_t));
		pElem->eth_port = (eth_conf_t*)malloc(sizeof(eth_conf_t));
		LL_InitElement(&pElem->elem);

		for(int i=0; i<4; i++){
			json_object * json_val;
			ret = json_object_object_get_ex(json_ethport, items[i], &json_val);
			ERROR_LOG(!ret, return -1, "Failed to get %s ...", items[i]);
			type = json_object_get_type(json_val);

			if(!strcmp(items[i],"port-id")){
				ERROR_LOG(type != json_type_int, return -1, "Eth port-id is not a json int value.");
				pElem->eth_port->port_id = json_object_get_int(json_val);
			}
			else if(!strcmp(items[i],"promisc")){
				ERROR_LOG(type != json_type_boolean, return -1, "Eth promisc is not a json boolean value.");
				pElem->eth_port->promisc = json_object_get_boolean(json_val);
			}
			else if(!strcmp(items[i],"tx-queues")){
				ERROR_LOG(type != json_type_array, return -1, "Eth tx-queues is not a json array value.");
				pElem->eth_port->nb_tx_q = json_object_array_length(json_val);

				if(pElem->eth_port->nb_tx_q){
					pElem->eth_port->tx_queues = (eth_q_t*) malloc(sizeof(eth_q_t) * pElem->eth_port->nb_tx_q);
					memset(pElem->eth_port->tx_queues,0,sizeof(eth_q_t)* pElem->eth_port->nb_tx_q);
					ret = _parse_eth_queues(json_val, pElem->eth_port->tx_queues, pElem->eth_port->nb_tx_q);
					ERROR_LOG(ret, return -1, "failed to parse tx queues.");
				}
			}
			else if(!strcmp(items[i],"rx-queues")){
				ERROR_LOG(type != json_type_array, return -1, "eth tx-queues is not a json array value.");
				pElem->eth_port->nb_rx_q = json_object_array_length(json_val);

				if(pElem->eth_port->nb_rx_q){
					pElem->eth_port->rx_queues = (eth_q_t*) malloc( sizeof(eth_q_t) * pElem->eth_port->nb_rx_q );
					memset(pElem->eth_port->rx_queues,0,sizeof(eth_q_t)*pElem->eth_port->nb_rx_q);
					ret = _parse_eth_queues(json_val, pElem->eth_port->rx_queues, pElem->eth_port->nb_rx_q);
					ERROR_LOG(ret, return -1, "failed to parse tx queues.");
				}
			}
			else {
				ERROR_LOG(LOG_TRUE, LOG_NOP,"Failed to parse %s ...", items[i]);
			}
		}
		/* and push the element */
		LListAdd(&CONF.eth_port_list, &pElem->elem);
	}
	return 0;
}

static int _parse_rings(json_object * jobj){

	int i, ret;
	int arraylen;
	enum json_type type;
	json_object * json_ring;

	type = json_object_get_type(jobj);
	ERROR_LOG((type != json_type_array), return -1, "json rings object is not an array ...");

	arraylen = json_object_array_length(jobj);

	for (i=0; i< arraylen; i++)
	{
		const char* items[5];
		json_ring = json_object_array_get_idx(jobj, i); /*Getting the array element at position i*/
		type = json_object_get_type(json_ring);
		ERROR_LOG((type != json_type_object), return -1, "Rings: got an json value which is not an object.");

		items[0] = "socket_id";
		items[1] = "single_consumer";
		items[2] = "single_producer";
		items[3] = "size";
		items[4] = "name";

		/* Parse here the attributes */
		ring_elem_t* pElem = (ring_elem_t*)malloc(sizeof(ring_elem_t));
		pElem->ring = (ring_conf_t*)malloc(sizeof(ring_conf_t));
		LL_InitElement(&pElem->elem);

		for(int i=0; i<5; i++){
			json_object * json_val;
			ret = json_object_object_get_ex(json_ring, items[i], &json_val);
			ERROR_LOG(!ret, return -1, "Failed to get %s ...", items[i]);

			if(!strcmp(items[i],"name")){
				pElem->ring->name = strdup((const char *)json_object_get_string(json_val));
			}
			else if(!strcmp(items[i],"size")){
				pElem->ring->size = json_object_get_int(json_val);
			}
			else if(!strcmp(items[i],"socket_id")){
				pElem->ring->socket_id = json_object_get_int(json_val);
			}
			else if(!strcmp(items[i],"single_consumer")){
				pElem->ring->single_consumer = json_object_get_boolean(json_val);
			}
			else if(!strcmp(items[i],"single_producer")){
				pElem->ring->single_producer = json_object_get_boolean(json_val);
			}
			else {
				ERROR_LOG(LOG_TRUE, LOG_NOP,"Failed to parse %s ...", items[i]);
			}
		}

		/* and push the element */
		LListAdd(&CONF.ring_list, &pElem->elem);
	}
	return 0;
}

static int _parse_json_app(json_object * jobj){

	json_object * json_app_name;
	json_object * json_memory_socket;
	int ret;

	ret = json_object_object_get_ex(jobj, "name", &json_app_name);
	ERROR_LOG(!ret, return -1, "Failed to get app name ...");

	ERROR_LOG(!(IS_JSON_STR(json_app_name) && CMP_JSON_STR(json_app_name,APP_NAME)),
			return -1, "Didn't found app name.");

	CONF.app_name = strdup(APP_NAME);

#if 0 // The core mask will be computed by parser and not comes from config file.
	json_object * json_core_mask;
	ret = json_object_object_get_ex(jobj, "core-mask", &json_core_mask);
	ERROR_LOG(!ret, return -1, "Failed to get core mask...");

	ERROR_LOG(!IS_JSON_INT(json_core_mask), return -1,
			"Core-mask is not an int value.");

	CONF.core_mask = json_object_get_int(json_core_mask);
#else
	CONF.core_mask = 1; // core 0 for management, set the first bit.
#endif
	ret = json_object_object_get_ex(jobj, "memory-socket", &json_memory_socket);
	ERROR_LOG(!ret, return -1, "Failed to get memory socket...");

	ERROR_LOG(!IS_JSON_INT(json_memory_socket), return -1, "Memory socket sould be int value.");

	CONF.memory_socket = json_object_get_int(json_memory_socket);

	return 0;
}


static int _json_parse_ressources( json_object * json_ressources )
{
#define NB_RESSOURCES	3
	const char * ressources[NB_RESSOURCES];
	int ret;

	//
	// Get and parse packet pools
	//
	ressources[0] = "pktpools";
	ressources[1] = "ethports";
	ressources[2] = "rings";

	for(int i=0; i<NB_RESSOURCES; i++){
		json_object * json_tab;
		ret = json_object_object_get_ex(json_ressources, ressources[i], &json_tab);
		INFO_LOG(!ret, continue, "Didn't found %s ...", ressources[i]);

		if(!strcmp(ressources[i],"pktpools") && ret){
			ret = _parse_pktpools(json_tab);
		}
		else if(!strcmp(ressources[i],"ethports") && ret){
			ret = _parse_ethports(json_tab);
		}
		else if(!strcmp(ressources[i],"rings") && ret){
			ret = _parse_rings(json_tab);
		}
		ERROR_LOG(ret, return -1, "Failed to parse %s ...", ressources[i]);
	}
	return 0;
}

static int _json_parse_dpdk_apps( json_object * json_dpdk_apps){
#define DPDK_APPS_ITEMS		3
	const char * dpdk_apps_items[DPDK_APPS_ITEMS];
	int ret;
	int arraylen;
	int i,j;
	json_object * json_array;
	enum json_type type;

	//
	// Get apps array
	//
	ret = json_object_object_get_ex(json_dpdk_apps, "dpdk-apps-array", &json_array);
	ERROR_LOG(!ret, return -1, "Didn't found dpdk-apps-array item.");

	type = json_object_get_type(json_array);
	ERROR_LOG(type != json_type_array, return -1, "dpdk-apps: got an json value which is not an array.");
	arraylen = json_object_array_length(json_array);

	for(i=0; i<arraylen; i++)
	{
		json_object * json_item;
		json_item = json_object_array_get_idx(json_array, i);
		//
		// Get and parse dpdk-apps
		//
		dpdk_apps_items[0] = "bin-path";
		dpdk_apps_items[1] = "config-file-path";
		dpdk_apps_items[2] = "core-id";

		/* Parse here the attributes */
		dpdk_app_elem_t* pElem = (dpdk_app_elem_t*)malloc(sizeof(dpdk_app_elem_t));
		pElem->dpdk_app = (dpdk_app_conf_t*)malloc(sizeof(dpdk_app_conf_t));
		LL_InitElement(&pElem->elem);

		for(j=0; j<DPDK_APPS_ITEMS; j++){
			json_object * json_obj;
			ret = json_object_object_get_ex(json_item, dpdk_apps_items[j], &json_obj);
			INFO_LOG(!ret, return -1, "Didn't found %s ...", dpdk_apps_items[j]);

			if(!strcmp(dpdk_apps_items[j],"bin-path") && ret){
				pElem->dpdk_app->bin_path = strdup((const char *)json_object_get_string(json_obj));
			}
			else if(!strcmp(dpdk_apps_items[j],"config-file-path") && ret){
				pElem->dpdk_app->config_file_path = strdup((const char *)json_object_get_string(json_obj));
			}
			else if(!strcmp(dpdk_apps_items[j],"core-id") && ret){
				pElem->dpdk_app->core_id = json_object_get_int(json_obj);
				ERROR_LOG(!IS_JSON_INT(json_obj), return -1, "core-id is not an int value.");
				uint64_t mask = 1;
				mask <<= pElem->dpdk_app->core_id-1;
				CONF.core_mask |= mask;
			}
			//ERROR_LOG(ret, return -1, "Failed to parse %s ...", dpdk_apps_items[j]);
		}

		/* and push the element */
		LListAdd(&CONF.dpdk_app_list, &pElem->elem);
	}
	return 0;
}

static int _json_parse(const char* buffer)
{
	int ret;
	json_object * jobj;

	jobj = json_tokener_parse(buffer);
	ERROR_LOG((!jobj), goto error, "Failed to parse tokener:%s",json_util_get_last_err());

	//
	// First, get the app obj.
	//
	json_object * json_app;
	ret = json_object_object_get_ex(jobj, "app", &json_app);
	ERROR_LOG((!ret), goto error, "Failed to get json object ...");

	//
	// Then check app name
	//
	ret = _parse_json_app(json_app);
	ERROR_LOG((ret), return -1, "Failed to parse json app details.");

	//
	// Get ressources
	//
	json_object * json_ressources;
	ret = json_object_object_get_ex(json_app, "ressources", &json_ressources);
	ERROR_LOG(!ret, goto error, "Failed to get ressources ...");
	ret = _json_parse_ressources(json_ressources);
	ERROR_LOG(ret, goto error, "Failed to parse ressources ...");

	//
	// Get dpdk-apps
	//
	json_object * json_dpdk_apps;
	ret = json_object_object_get_ex(json_app, "dpdk-apps", &json_dpdk_apps);
	ERROR_LOG(!ret, goto error, "Failed to get dpdk-apps object ...");
	ret = _json_parse_dpdk_apps(json_dpdk_apps);
	ERROR_LOG(ret, goto error, "Failed to parse dpdk-apps ...");

	//
	// Free the main json object.
	//
	json_object_put(jobj);
	return 0;

error:
	if(jobj)
		json_object_put(jobj);
	return -1;
}


static int _parse_elements(const char * filename)
{
  FILE * fd;
  int filesize;
  int read;
  int ret;
  char* buffer;

  ret = 0;
  read = 0;

  /* read the file */
  fd = fopen(filename, "r+");
  ERROR_LOG(!fd, ret=-1; goto error, "Failed to open file: %s.",filename);

  fseek(fd,0,SEEK_END);
  filesize = ftell(fd);
  fseek(fd, SEEK_SET, 0);

  /* alloc a buffer */
  buffer = (char*)malloc(filesize);
  ERROR_LOG(!buffer, ret=-1; goto error, "Failed to allocate buffer.");

  /* read the file */
  while(read < filesize){
	  read += fread(buffer, 1, filesize-read, fd);
	  ERROR_LOG((read < 0), ret=-1; goto error, "Failed to read from file.");
  }

  ret = _json_parse(buffer);
  ERROR_LOG(ret, ret=-1; goto error, "Failed to parse buffer.");

  ret = 0;

error:
  fclose(fd);
  free(buffer);
  return ret;
}


int APP_CONF_parse_file( const char * filename ){

	int err;

	/* init the CONF objects */
	_init_conf(&CONF);

	/* ... and parse the elements */
	err = _parse_elements(filename);
	ERROR_LOG(err, return -1, "Failed to parse elements.");

	return 0;
}


