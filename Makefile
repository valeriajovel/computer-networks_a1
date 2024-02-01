# Valeria Jovel
# CMSC417 Assignment 1

CC = gcc
CFLAGS = -Wall -Werror -ggdb
PROGS = client server
SRCS_C = client.c client_funcs.c
OBJS_C = $(SRCS_C:.c=.o)
SRCS_S = server.c client_funcs.c server_funcs.c 
OBJS_S =$(SRCS_S:.c=.o)

all: $(PROGS)

clean: 
	- rm *.o $(PROGS)

client: $(OBJS_C)
	$(CC) $(CFLAGS) -o $@ $(OBJS_C) -lrt

server: $(OBJS_S)
	$(CC) $(CFLAGS) -o $@ $(OBJS_S) -lrt

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
