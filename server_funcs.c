#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <argp.h>
#include <arpa/inet.h> 
#include <time.h> 

#include "server_funcs.h"
#include "client_funcs.h"

int validPort(int port){
	int result = 0;

	if(port > 1024 && port < 65535){
		result = 1;
	}

	return result;
}

error_t server_parser(int key, char *arg, struct argp_state *state) {
	struct server_arguments *args = state->input;
	error_t ret = 0;
	switch(key) {
	case 'p':
		/* Validate that port is correct and a number, etc!! */
		args->port = atoi(arg);
		if (validPort(args->port) == 0) {
			argp_error(state, "Invalid option for a port, must be a valid number");
		}
		break;
	case 'd':
		args->percent = atoi(arg);
        if (args->percent < 0 || args->percent > 100){
            argp_error(state, "Invalid percentage rate, must be between 0 and 100");
        }
		break;
	default:
		ret = ARGP_ERR_UNKNOWN;
		break;
	}
	return ret;
}

void server_parseopt(int argc, char *argv[], struct server_arguments *args) {
	struct argp_option options[] = {
		{ "port", 'p', "port", 0, "The port to be used for the server" ,0},
		{ "percent", 'd', "percent", 0, "The percentage chance the server drops the packet", 0},
		{0}
	};
	struct argp argp_settings = { options, server_parser, 0, 0, 0, 0, 0 };

	/* bzero ensures that "default" parameters are all zeroed out */
	bzero(args, sizeof(struct server_arguments));
	
	if (argp_parse(&argp_settings, argc, argv, 0, NULL, args) != 0) {
		printf("Got an error condition when parsing\n");
	}
}

int send_datagram(int percent){

	int result = 0;

	if(percent == 0){
		result = 1;
	} else if (percent == 100){
		result = 0;
	} else {
		int r = rand() % 100; 
		//printf("R: %d\n", r);
		if (r < percent) {
			result = 1;
		} else {
			result = 0;
		}
	}
	
    return result;
}
