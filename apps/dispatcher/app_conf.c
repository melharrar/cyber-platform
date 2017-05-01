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
#include <rte_cfgfile.h>

#include "json.h"
#include "app_conf.h"
#include "main.h"

#define IS_JSON_STR(jobj)		(json_type_string == json_object_get_type(jobj))
#define IS_JSON_INT(jobj)		(json_type_int == json_object_get_type(jobj))
#define CMP_JSON_STR(jobj,str) 	(!strcmp(str, json_object_get_string(jobj)))


static
void _init_conf( app_conf_t* p_conf){

	p_conf->app_name = 0;
	p_conf->core_id = 0;
	LL_Init(&p_conf->rx_interfaces_list);
	LL_Init(&p_conf->tx_interfaces_list);
	LL_Init(&p_conf->app_interfaces_list);
}

static
int _parse_json_app(json_object * jobj){

	json_object * json_app_name;
	json_object * json_core_id;
	int ret;

	/* check app name */
	ret = json_object_object_get_ex(jobj, "name", &json_app_name);
	ERROR_LOG(!ret, return -1, "Failed to get app name ...");
	ERROR_LOG(!(IS_JSON_STR(json_app_name) && CMP_JSON_STR(json_app_name,APP_NAME)),
			return -1, "APP NAME didn't match with the app name.");
	CONF.app_name = strdup(APP_NAME);

	/* get core id */
	ret = json_object_object_get_ex(jobj, "core-id", &json_core_id);
	ERROR_LOG(!ret, return -1, "Failed to get core mask...");
	ERROR_LOG(!IS_JSON_INT(json_core_id), return -1, "Core-mask is not an int value.");
	CONF.core_id = json_object_get_int(json_core_id);

	return 0;
}

static
int _json_parse_interface(json_object* json_array, LL_T* list){
#define IF_ITEMS		7
	const char * if_items[IF_ITEMS];
	enum json_type type;
	int ret;
	int arraylen;
	int i,j;

	type = json_object_get_type(json_array);
	ERROR_LOG(type != json_type_array, return -1, "json object should be an array.");
	arraylen = json_object_array_length(json_array);

	for(i=0; i<arraylen; i++)
	{
		json_object * json_item;
		json_item = json_object_array_get_idx(json_array, i);

		//
		// Get and parse dpdk-apps
		//
		if_items[0] = "used";
		if_items[1] = "is-eth";
		if_items[2] = "if-name";
		if_items[3] = "port-id";
		if_items[4] = "queue-id";
		if_items[5] = "ring-name";
		if_items[6] = "bypass-if-name";

		/* Parse here the attributes */
		if_elem_t* pElem = (if_elem_t*)malloc(sizeof(if_elem_t));
		pElem->p_interface = (if_conf_t*)malloc(sizeof(if_conf_t));
		memset(pElem->p_interface,0, sizeof(if_conf_t));
		LL_InitElement(&pElem->elem);

		for(j=0; j<IF_ITEMS; j++){
			json_object * json_obj;
			ret = json_object_object_get_ex(json_item, if_items[j], &json_obj);
			INFO_LOG(!ret, continue, "no %s item in the current block. It may be OK.", if_items[j]);

			if(!strcmp(if_items[j],"if-name") && ret){
				pElem->p_interface->name = strdup((const char *)json_object_get_string(json_obj));
			}
			else if(!strcmp(if_items[j],"is-eth") && ret){
				pElem->p_interface->is_eth = json_object_get_boolean(json_obj);
			}
			else if(!strcmp(if_items[j],"used") && ret){
				pElem->p_interface->used = json_object_get_boolean(json_obj);
			}
			else if(!strcmp(if_items[j],"port-id") && ret){
				pElem->p_interface->port_id = json_object_get_int(json_obj);
			}
			else if(!strcmp(if_items[j],"queue-id") && ret){
				pElem->p_interface->q_id = json_object_get_int(json_obj);
			}
			else if(!strcmp(if_items[j],"ring-name") && ret){
				pElem->p_interface->ring_name = strdup((const char *)json_object_get_string(json_obj));
			}
			else if(!strcmp(if_items[j],"bypass-if-name") && ret){
				pElem->p_interface->bypass_if_name = strdup((const char *)json_object_get_string(json_obj));
			}
			else {
				//INFO_LOG(ret, LOG_NOP, "Failed to parse %s ...", if_items[j]);
			}
		}

		/* and push the element */
		LListAdd(list, &pElem->elem);
	}
	return 0;
}

static
int _json_parse(const char* buffer)
{
	int ret;
	json_object * jobj;
	json_object * json_if;
	json_object * json_interfaces;

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
	// Get interfaces
	//
	ret = json_object_object_get_ex(json_app, "interfaces", &json_interfaces);
	ERROR_LOG(!ret, goto error, "Failed to get interfaces ...");

	//
	// Get rx-interfaces list
	//
	ret = json_object_object_get_ex(json_interfaces, "rx-interfaces", &json_if);
	ERROR_LOG(!ret, goto error, "Failed to get rx interfaces ...");
	ret = _json_parse_interface(json_if, &CONF.rx_interfaces_list);
	ERROR_LOG(ret, goto error, "Failed to parse rx interface list");
	INFO_LOG(LOG_TRUE, LOG_NOP, "%d rx interfaces were found.", CONF.rx_interfaces_list.count);

	//
	// Get tx interfaces list
	//
	ret = json_object_object_get_ex(json_interfaces, "tx-interfaces", &json_if);
	ERROR_LOG(!ret, goto error, "Failed to get tx interfaces ...");
	ret = _json_parse_interface(json_if, &CONF.tx_interfaces_list);
	ERROR_LOG(ret, goto error, "Failed to parse egress if");
	INFO_LOG(LOG_TRUE, LOG_NOP, "%d tx interfaces were found.", CONF.tx_interfaces_list.count);

	//
	// Get app interfaces list.
	//
	ret = json_object_object_get_ex(json_interfaces, "app-interfaces", &json_if);
	ERROR_LOG(!ret, goto error, "Failed to get app interfaces ...");
	ret = _json_parse_interface(json_if, &CONF.app_interfaces_list);
	ERROR_LOG(ret, goto error, "Failed to parse app interfaces list.");
	INFO_LOG(LOG_TRUE, LOG_NOP, "%d app. interfaces were found.", CONF.app_interfaces_list.count);

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

static
int _parse_elements(const char * filename)
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

int
APP_CONF_parse_file( const char * filename ){

	int err;

	/* init the CONF objects */
	_init_conf(&CONF);

	/* ... and parse the elements */
	err = _parse_elements(filename);
	ERROR_LOG(err, return -1, "Failed to parse elements.");

	return 0;
}
