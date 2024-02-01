struct server_arguments {
	int port;
	int percent;
};

void server_parseopt(int argc, char *argv[], struct server_arguments *args);

int send_datagram(int percent);