#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <argp.h>
#include <arpa/inet.h> 
#include <time.h>           

#include "client_funcs.h"

void throw_error(char* err_message) {
    perror(err_message);
    exit(1);
}

int isValidIP(char *IpAddress){
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, IpAddress, &(sa.sin_addr));
    return result;
}

int isValidPort(int port){
	int result = 0;

	if(port > 1024 && port < 65535){
		result = 1;
	}

	return result;
}

error_t client_parser(int key, char *arg, struct argp_state *state) {
	struct client_arguments *args = state->input;
	error_t ret = 0;
	switch(key) {
	case 'a':
		/* validate that address parameter makes sense */
		strncpy(args->ip_address, arg, 16);
		if (isValidIP(args->ip_address) == 0) {
			argp_error(state, "Invalid address");
		}
		break;
	case 'p':
		/* Validate that port is correct and a number, etc!! */
		args->port = atoi(arg);
		if (isValidPort(args->port) == 0) {
			argp_error(state, "Invalid option for a port, must be a valid number");
		}
		break;
	case 'n':
		args->N = atoi(arg);
		if(args->N <= 0){
			argp_error(state, "Invalid number of time requests");
		}
		break;
	case 't':
		args->timeout = atoi(arg);
		if(args->timeout < 0){
			argp_error(state, "Invalid number of timeout seconds");
		}
		break;
	default:
		ret = ARGP_ERR_UNKNOWN;
		break;
	}
	return ret;
}
void client_parseopt(int argc, char *argv[], struct client_arguments *args) {
	struct argp_option options[] = {
		{ "addr", 'a', "addr", 0, "The IP address the server is listening at", 0},
		{ "port", 'p', "port", 0, "The port that is being used at the server", 0},
		{ "timereqs", 'n', "timereqs", 0, "The number of time requests to send to the server", 0},
		{ "timeout", 't', "timeout", 0, "Secs that client will wait after sending its last timereq to receive a timeresp", 0},
		{0}
	};

	struct argp argp_settings = { options, client_parser, 0, 0, 0, 0, 0 };

	//struct client_arguments args;
	bzero(args, sizeof(struct client_arguments));

	if (argp_parse(&argp_settings, argc, argv, 0, NULL, args) != 0) {
		printf("Got error in parse\n");
		exit(1);
	}
}

int create_socket(char *address, int port){
	struct sockaddr_in server_addr; 
	int sd;

	if((sd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        throw_error("ERROR: socket creation failed.");
    }

    /* Construct the server address structure */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(address);
    server_addr.sin_port = htons(port);

	printf("Connected to %s (%s), sd %d\n", address, inet_ntoa(server_addr.sin_addr), sd);

	return sd;
}

uint64_t htonll(uint64_t time) {
    uint32_t hl = htonl((uint32_t)(time >> 32));
    uint32_t ll = htonl((uint32_t)time);
    return ((uint64_t)ll << 32) | hl;
}

uint64_t ntohll(uint64_t time) {
    uint32_t hl = ntohl((uint32_t)(time >> 32));
    uint32_t ll = ntohl((uint32_t)time);
    return ((uint64_t)ll << 32) | hl;
}

double set_time_offset(struct timespec t0, struct timespec t1, struct timespec t2){
	double ret = -1;

	double time0 = (t0.tv_sec*NANO + (t0.tv_nsec));
	double time1 = (t1.tv_sec*NANO + (t1.tv_nsec));
	double time2 = (t2.tv_sec*NANO + (t2.tv_nsec));

	ret = (time1 - time0 + time1 - time2)/(2*NANO);

	return ret;
}

double set_time_delay(struct timespec t0, struct timespec t2){
	double ret = -1;

	double sec_t2_t0 = (double) t2.tv_sec - (double) t0.tv_sec;
	double nsec_t2_t0 = (double) t2.tv_nsec - (double) t0.tv_nsec;

	if (nsec_t2_t0 < 0) {
		sec_t2_t0 -= 1;
		nsec_t2_t0 += NANO; 
	}

	double diff_t2_t0 = (sec_t2_t0 * NANO) + nsec_t2_t0;

	ret = (double) diff_t2_t0;

	return ret;
}

/*
double result = -1;

	double sec_t1_t0 = (double) (t1.tv_sec - t0.tv_sec);
	double nsec_t1_t0 = (double) (t1.tv_nsec - t0.tv_nsec);

	if (nsec_t1_t0 < 0) {
		sec_t1_t0 -= 1;
		nsec_t1_t0 += NANO; 
	}

	double time_t1_t0 = (sec_t1_t0) + (nsec_t1_t0/NANO);

	double sec_t1_t2 = (double) t1.tv_sec - (double) t2.tv_sec;
	double nsec_t1_t2 = (double) t1.tv_nsec - (double) t2.tv_nsec;

	if (nsec_t1_t2 < 0) {
		sec_t1_t2 -= 1;
		nsec_t1_t2 += NANO; 
	}

	double time_t1_t2 = (sec_t1_t2) + (nsec_t1_t2/NANO);

	result = (time_t1_t0 + time_t1_t2 / 2.0);

	return result;
*/