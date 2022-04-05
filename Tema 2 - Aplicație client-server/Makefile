# Tema 2
# Makefile

CFLAGS = -Wall -g
PORT = 2356
IP_SERVER = 127.0.0.1
ID = tudor

all:  server subscriber

server: server.c client_utils.c protocol.c

subscriber: subscriber.c protocol.c


.PHONY: clean run_server run_subscriber

# Ruleaza serverul
run_server:
	./server ${PORT}

# Ruleaza clientul
run_subscriber:
	./subscriber $(ID) ${IP_SERVER} ${PORT}

clean:
	rm -f server subscriber client_utils protocol
