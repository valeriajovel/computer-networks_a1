#define NANO 1000000000

typedef struct time_request {
    uint32_t seqno;
    uint32_t ver;
    uint64_t client_time_s;    
    uint64_t client_time_ns;
} TimeRequest;

typedef struct time_response{
    uint32_t seqno;
    uint32_t ver;
    uint64_t client_time_s;    
    uint64_t client_time_ns;
    uint64_t server_time_s;    
    uint64_t server_time_ns;
} TimeResponse;

struct client_arguments {
	char ip_address[16]; /* You can store this as a string, but I probably wouldn't */
	int port; 
	int N; /* number of time requests */
	int timeout; /* T time in seconds that the client will wait after sending its last 
                    TimeRequest to receive a TimeResponse */
};

void throw_error(char* err_message);
//error_t client_parser(int key, char *arg, struct argp_state *state);
void client_parseopt(int argc, char *argv[], struct client_arguments *args);
int create_socket(char *address, int port);
uint64_t htonll(uint64_t time);
uint64_t ntohll(uint64_t time);
double set_time_offset(struct timespec t0, struct timespec t1, struct timespec t2);
double set_time_delay(struct timespec t0, struct timespec t2);