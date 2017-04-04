/*
 *
 * Mikael Elharrar
 * 22/10/2016
 *
 * Main function of the app manager.
 * This process is responsible to allocate the relevant shared ressources
 * like pools, and by default have the master rights.
 * The all other processes will have the secondary rights.
 * 
 * The name of the ressource will come from config file.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/queue.h>
#include <unistd.h>
#include <getopt.h>

#include "app_conf.h"
#include "allocator.h"
#include "main.h"

static struct option long_options[] =
{
    {"config-file",	required_argument, NULL, 'f'},
    {NULL, 0, NULL, 0}
};


static void print_usage( const char * argv ){
    printf("Usage: %s [OPTIONS]\n", argv);
    printf("  --config-file configuration file in json format.\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	int err;
	char* filename = NULL;
	int c;

	(void)argc;
	(void)argv;
	int rc;

	rc = LOG_init("/var/log/");
	if (rc){
	    printf("LOG INIT FAILED !!!\n");
	    return -1;
	}


	// Get the file name.
	while( (c=getopt_long(argc, argv, "f:", long_options, NULL)) != -1 ){
		switch(c)
		{

		case 'f':
			filename = optarg;
			INFO_LOG(LOG_TRUE, LOG_NOP, "Got filename: %s", filename);
			break;
		default:
			INFO_LOG(LOG_TRUE, LOG_NOP, "Unknow option %d", c);
			break;
		}
	}

	if(filename == NULL){
		print_usage((const char *)argv[0]);
	}

	err = APP_CONF_parse_file( filename );
	ERROR_LOG(err, exit(-1), "Failed to parse config file.");

	err = ALLOCATOR_create_ressources(&CONF);
	ERROR_LOG(err, exit(-1), "Failed to allocate ressources.");

	err = ALLOCATOR_start_ressources(&CONF);
	ERROR_LOG(err, exit(-1), "Failed to start ressources.");

	while(1){
		sleep(1);
		//PROCESS_MNGR_scan(&CONF.dpdk_app_list);
	}

	return 0;
}

