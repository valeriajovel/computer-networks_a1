#include <stdio.h>
#include <sys/socket.h>     // for socket, connect, send, and recv funcs
#include <arpa/inet.h>      // for sockaddr_in and inet_addr 
#include <string.h>         // for memset
#include <inttypes.h>       // for uint32_t, etc
#include <unistd.h>			// for fork, close 
#include <time.h>           // for timespec
#include <sys/wait.h>		// for wait
#include <sys/select.h>     // for select
#include <assert.h>         // for assert

#include "client_funcs.h"

/** Array for Time Responses **/
struct time_responses{
    int i;
    char msg[50]; // hold a string with 50 chars (don't forget \0)
};

int main(int argc, char *argv[]) {

    int sd,  seqno;
    ssize_t bytes_sent, bytes_received;
    struct sockaddr_in server_addr; 

	struct client_arguments args;
	memset(&args, 0, sizeof(struct client_arguments));

	client_parseopt(argc, argv, &args); //parse options as the client would
	
	printf("Got %s on port %d with n=%d timeout=%d\n",
           args.ip_address, args.port, args.N, args.timeout);

    //timeout_ms = 1000*args.timeout;

    /* Create UDP socket */

	if((sd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        throw_error("ERROR: socket creation failed.");
    }

    /* Construct the server address structure */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(args.ip_address);
    server_addr.sin_port = htons(args.port);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    seqno = 1;

    while(seqno <= args.N){

        /* Send TimeRequests */
        TimeRequest req;
        req.seqno = htonl(seqno);
        req.ver = htonl(7);

        struct timespec curr_time;
        clock_gettime(CLOCK_REALTIME, &curr_time);

        req.client_time_s = htonll(curr_time.tv_sec);
        req.client_time_ns = htonll(curr_time.tv_nsec);

        if ((bytes_sent = sendto(sd, &req, sizeof(TimeRequest), 0, (struct sockaddr *)&server_addr, sizeof(server_addr))) < 0){
            close(sd);
            throw_error("ERROR: sendto() failed");
        }

        seqno++;
    }

    timeout.tv_sec = args.timeout;

    /* Start array with N indeces */
    struct time_responses res_list[args.N];
    memset(&res_list, 0, sizeof(res_list));

    seqno = 1;
    while(seqno <= args.N) {
        for(;;){
            /* Receive time Responses */
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(sd, &rfds);

            int ret = select(sd + 1, &rfds, NULL, NULL, &timeout); 
            if (ret < 0) {
                close(sd);
                throw_error("ERROR: select() failed ");
            } else if (FD_ISSET(sd, &rfds)) { 

                TimeResponse res;
                bytes_received = recvfrom(sd, &res, sizeof(TimeResponse), 0, NULL, NULL);

                if(bytes_received < 0){
                    close(sd);
                    throw_error("ERROR: recvfrom() failed");
                }

                res.seqno = ntohl(res.seqno);
                res.ver = ntohl(res.ver);

                struct timespec t2;
                clock_gettime(CLOCK_REALTIME, &t2);

                struct timespec t0;
                t0.tv_sec = (time_t) ntohll(res.client_time_s);
                t0.tv_nsec = (long) ntohll(res.client_time_ns);

                struct timespec t1;
                t1.tv_sec = (time_t) ntohll(res.server_time_s);
                t1.tv_nsec = (long) ntohll(res.server_time_ns);

                double theta = set_time_offset(t0, t1, t2);
                double delta = set_time_delay(t0, t2);
                res_list[res.seqno-1].i = res.seqno;
                snprintf(res_list[res.seqno-1].msg, sizeof(res_list[res.seqno-1].msg), "%d: %.4lf %.4lf", res.seqno, theta, delta/NANO);
             
                break;
            } else if (ret == 0){
                /* Timeout occurs */
                break;
            }

        } 
        seqno++;
    }

    for(int i = 0; i < args.N; i++){
        if (res_list[i].i == 0){
            printf("%d: Dropped\n", i+1);
        } else {
            printf("%s\n", res_list[i].msg);
        }
    }

    close(sd);

    return 0;
}