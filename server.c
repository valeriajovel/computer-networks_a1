#include <stdio.h>
#include <stdlib.h>         // for srand
#include <sys/socket.h>     // for socket, connect, send, and recv funcs
#include <arpa/inet.h>      // for sockaddr_in and inet_addr 
#include <string.h>         // for memset
#include <inttypes.h>       // for uint32_t, etc
#include <unistd.h>			// for fork, close 
#include <time.h>           // for timespec
#include <sys/wait.h>		// for wait
#include <sys/select.h>     // for select
#include <assert.h>         // for assert

#include "server_funcs.h"
#include "client_funcs.h"

#define MAX_CLIENTS 20
#define CLIENT_TIMEOUT 20 // 2 mins

typedef struct client_info {
    struct sockaddr_in addr;
    int highest_seqno;
    time_t last_update;
} ClientInfo;

/* SERVER HANDLES MULTIPLE CLIENTS, however when I set my database to handle multiple clients,
    the <ADDR>:<Port> <SEQ> <MAX> won't be printed since each instance of a client (even if they have 
    same IP address) will generate a different port number each time */

int main(int argc, char *argv[]) {

    ClientInfo clients[MAX_CLIENTS];
    memset(&clients, 0, sizeof(ClientInfo));

    int sd;
    ssize_t bytes_sent, bytes_received;
    struct sockaddr_in server_addr; 

	struct server_arguments args;
	memset(&args, 0, sizeof(struct server_arguments));

	server_parseopt(argc, argv, &args); 
	
	printf("Got port %d and percentage %d\n", args.port, args.percent);

    if((sd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        throw_error("ERROR: socket creation failed.");
    }

    /* Construct the server address structure */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(args.port);

    /* Bind to the local address */
    if ((bind(sd, (struct sockaddr *) &server_addr, sizeof(server_addr))) < 0){
        throw_error("ERROR: bind() failed in server.c");
    }

    /* Set timer for 2 mins */
    time_t now = time(NULL);
    srand(time(NULL));

    for(;;){
        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(struct sockaddr_in));
		socklen_t client_addr_len = sizeof(struct sockaddr_in);
        
        /* Receive Time Requests then prints out the info */
        TimeRequest req;
        if((bytes_received = recvfrom(sd, &req, sizeof(TimeRequest), 0, (struct sockaddr *) &client_addr, &client_addr_len)) < 0){
            close(sd);
            throw_error("ERROR: recvfrom() failed");
        }

        req.seqno = ntohl(req.seqno);
        req.ver = ntohl(req.ver);
        req.client_time_s = ntohll(req.client_time_s);
        req.client_time_ns = ntohll(req.client_time_ns);

        /* Randomly keep/ignore the payload */
        if (send_datagram(args.percent)){

            //printf("(TReq) Seqno: %d, ver: %d\n", req.seqno, req.ver);
            //printf("\tSec: %ld, ns: %ld\n", req.client_time_s, req.client_time_ns);

            int client_index = -1;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].addr.sin_port == client_addr.sin_port &&
                    clients[i].addr.sin_addr.s_addr == client_addr.sin_addr.s_addr) {
                    client_index = i;
                    break;
                }
            }

            if (client_index == -1) { /* If received new client, add it to database */
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].addr.sin_port == 0) {
                        memcpy(&clients[i].addr, &client_addr, sizeof(struct sockaddr_in));
                        clients[i].highest_seqno = req.seqno;
                        clients[i].last_update = time(NULL);
                        client_index = i; // Update the client index
                        break;
                    }
                }
            }

            /*  If the highest sequence number for a given client has not changed for two minutes, the server
                clears the highest observed sequence number for that client, and treats the next (if any)
                TimeRequest from that same client as the new highest sequence number for that client. */

            now = time(NULL);
            if ((now - clients[client_index].last_update) >= CLIENT_TIMEOUT) {
                clients[client_index].highest_seqno = 0;
                clients[client_index].last_update = now;
            } 

            //printf("Current timer: %ld\n", now - clients.last_update);

            // Handle the case where the sequence number is lower than the highest observed sequence number
            /* NOTE: this doesn't print out when I handle multiple clients since each client has a different port 
                number */
            if (req.seqno < clients[client_index].highest_seqno) {
                printf("%s:%d %d %d\n", inet_ntoa(clients[client_index].addr.sin_addr), ntohs(clients[client_index].addr.sin_port),
                    req.seqno, clients[client_index].highest_seqno);
            } else {
                // Update the highest observed sequence number (I update each time)
                clients[client_index].highest_seqno = req.seqno;
                clients[client_index].last_update = time(NULL);

                //printf("highest seqno: %d, last_update %ld\n", clients.highest_seqno, clients.last_update);
            }

            /*  Crafts and sends the TimeResponse payload back to the sender of the TimeRequest. The
                TimeResponse is a single datagram. The sequence number and client timestamp are the same as the 
                corresponding TimeRequest. The server time is the timestamp the server just took. (curr_time) */

            struct timespec curr_time;
            clock_gettime(CLOCK_REALTIME, &curr_time);

            TimeResponse res;
            res.seqno = htonl(req.seqno);
            res.ver = htonl(req.ver);
            res.client_time_s = htonll(req.client_time_s);
            res.client_time_ns = htonll(req.client_time_ns);

            res.server_time_s = htonll(curr_time.tv_sec);
            res.server_time_ns = htonll(curr_time.tv_nsec);

            if((bytes_sent = sendto(sd, &res, sizeof(TimeResponse), 0, (struct sockaddr *)&clients[client_index].addr, sizeof(struct sockaddr_in))) < 0){
                close(sd);
                throw_error("ERROR: sendto() failed");
            }
            printf("Datagram with seqno = %d was sent (see client)\n", req.seqno);

        } else {
            printf("Datagram with seqno = %d was dropped (see client)\n", req.seqno);
        }
        
    }

    return 0;
}