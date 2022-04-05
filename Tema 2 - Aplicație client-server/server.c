#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "helpers.h"
#include "./utils/client_utils.h"
#define INITIAL_CLIENT_MAX 5
#define SUBSCRIBE_OFFSET 10
#define UNSUBSCRIBE_OFFSET 12


void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}


int main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	int tcpsockfd, udpsockfd, newsockfd, portno;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr;
	int i, ret;
	socklen_t clilen;

	fd_set read_fds; // read file descriptors
	fd_set tmp_fds;	// temporary read file descriptors
	int fdmax_TCP; // maximum value from read file descriptors list

	if (argc < 2) {
		usage(argv[0]);
	}

	// empty both file descriptor lists
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// TCP SOCKET
	tcpsockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcpsockfd < 0, "socket_TCP");

	// deactivate NAGLE algorithm
	int ok = 1;
	ret = setsockopt(tcpsockfd, IPPROTO_TCP,  TCP_NODELAY, &ok, sizeof(ok));
	DIE(ret < 0, "socket options");

	// UDP SOCKET
	udpsockfd = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(udpsockfd < 0, "socket_UDP"); 

	// parse the port number from argument list
	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	// initialise server adress data
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	// TCP
	ret = bind(tcpsockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind_TCP");

	// setup a listen socket
	ret = listen(tcpsockfd, MAX_CLIENTS);
	DIE(ret < 0, "listen_TCP");

	// UDP
	ret = bind(udpsockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind_UDP");

	// initialising new cli_info structure
	int max_client_nr = INITIAL_CLIENT_MAX;
	int client_nr = 0;
	cli_info *clients = malloc(max_client_nr * sizeof(cli_info));
	

	// adding our newly created sockets to read file descriptors list
	FD_SET(tcpsockfd, &read_fds);
	FD_SET(udpsockfd, &read_fds);

	if (tcpsockfd > udpsockfd) {
		fdmax_TCP = tcpsockfd;
	} else {
		fdmax_TCP = udpsockfd;
	}
	FD_SET(STDIN_FILENO, &read_fds);

	while (1) {
		tmp_fds = read_fds;
		
		ret = select(fdmax_TCP + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fdmax_TCP; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == STDIN_FILENO) {
					// received data from standard input
					memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN - 1, stdin);

					// verify if exit command has been given
					if (strncmp(buffer, "exit", 4) == 0) {
						// closing all sockets created by "accept" function
						tmp_fds = read_fds;
						for (int j = 1; j < fdmax_TCP; j++) {
							if (FD_ISSET(j, &tmp_fds)) {
								if (j != tcpsockfd && j != udpsockfd) {
									close(j);
									FD_CLR(j, &read_fds);
								}
							}
						}
						// dynamically deallocate clients list
						free_all_clients(clients, client_nr);

						// close remaining sockets
						close(udpsockfd);
						close(tcpsockfd);
						FD_CLR(udpsockfd, &read_fds);
						FD_CLR(tcpsockfd, &read_fds);

						// closing the program with no errors
						return 0;
					}
				} else if (i == tcpsockfd) {
					// connection request incoming from listen socket
					clilen = sizeof(cli_addr);
					newsockfd = accept(tcpsockfd, (struct sockaddr *) &cli_addr, &clilen);
					DIE(newsockfd < 0, "accept");

					// deactivate NAGLE algorithm
					ok = 1;
					ret = setsockopt(newsockfd, IPPROTO_TCP,  TCP_NODELAY, &ok, sizeof(ok));

					myProtocol message;
					receive_message(newsockfd, &message, 0);
					
					int send_log_ok = 0;
					int index = search_client_by_id(clients, client_nr, message.payload);
					if (index >= 0 && clients[index].is_logged == 0) {
						// client already in client list with sf == 1 on at least a topic
						clients[index].is_logged = 1;
						clients[index].sockfd = newsockfd;

						// check if client has the store-forward option on at least a topic
						send_log_ok = sf_check(clients[index]);

						char continue_msg[] = "OK";
						send_message(newsockfd, 0, strlen(continue_msg) + 1, continue_msg, 0);
					} else {
						int ret = add_client(&clients, &client_nr, &max_client_nr, message.payload, newsockfd);
						// verificam daca exista deja un client cu acelasi id
						if (ret < 0) {
							char close_msg[] = "ER";

							send_message(newsockfd, 0, 3, close_msg, 0);
						    printf("Client %s already connected.\n", message.payload);

							close(newsockfd);
							continue;
						} else {
							char continue_msg[] = "OK";
							send_message(newsockfd, 0, 3, continue_msg, 0);
						}
					}

					// adding the new socket to read file descriptors list
					FD_SET(newsockfd, &read_fds);
					if (newsockfd > fdmax_TCP) { 
						fdmax_TCP = newsockfd;
					}

					// printing to standard output the feedback message
					printf("New client %s connected from %s:%d.\n", message.payload,
							inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

					// sending the log to subscriber after autentification
					if (send_log_ok) {
						send_log(clients[index]);
						remove_log(&clients[index]);
					}

					// dezalocam payload-ul alocat dinamic
					free_message(&message);
				} else if (i == udpsockfd) {
					char big_buffer[TOPIC_OFFSET + TYPE_OFFSET + MAX_UDP_PAYLOAD + IP_OFFSET + PORT_OFFSET];
					memset(big_buffer, 0, sizeof(big_buffer));

					clilen = sizeof(cli_addr);
					ret = recvfrom(udpsockfd, big_buffer, sizeof(big_buffer), 0, (struct sockaddr *) &cli_addr, &clilen);
					DIE(ret < 0, "udp recv");
					
					uint8_t type;
					char topic[TOPIC_OFFSET + 1];
					memset(topic, 0, TOPIC_OFFSET + 1);

					memcpy(topic, big_buffer, TOPIC_OFFSET);
					memcpy(&type, big_buffer + TOPIC_OFFSET, TYPE_OFFSET);

					notify_clients(clients, client_nr, topic, type, big_buffer, cli_addr);
				} else {
					// data incoming from client socket
					myProtocol message;
					int n = receive_message(i, &message, 0);
				
					if (n == 0) {
						// connection closed
						int index = search_client_by_socket(clients, client_nr, i);
						if (index < 0) {
							printf("No clients by that index found");
							exit(-1);
						}
						// we need to store the logs for the client while offline
						printf("Client %s disconnected.\n", clients[index].id);

						clients[index].is_logged = 0;
						close(i);
							
						// removing the closed socket from reading list
						FD_CLR(i, &read_fds);
					} else {
						//message from a TCP client
						int index = search_client_by_socket(clients, client_nr, i);
						if (index < 0) {
							printf("No clients by that index found\n");
						}

						// handle subscriptions and unsubscriptions
						if (strncmp(message.payload, "subscribe", 9) == 0) {
							char topic[TOPIC_OFFSET + 1];
							int sf;
							memset(topic, 0, TOPIC_OFFSET + 1);
							sscanf(message.payload + SUBSCRIBE_OFFSET, "%s %d", topic, &sf);

							ret = subscribe(&clients[index], topic, sf);
							if (ret < 0) {
								printf("client already subscribed to topic - %s with SF - %d\n", topic, sf);
							} else {
								// sending feedback message to client
								send_message(i, SUBSCRIBE_NOTIF, strlen(topic) + 1, topic, 0);
							}
						} else if (strncmp(message.payload, "unsubscribe", 11) == 0) {
							char topic[TOPIC_OFFSET + 1];
							memset(topic, 0, TOPIC_OFFSET + 1);
							sscanf(message.payload + UNSUBSCRIBE_OFFSET, "%[^\n]s", topic);
	
							ret = unsubscribe(&clients[index], topic);
							if (ret < 0) {
								printf("No subscription to topic - %s\n", topic);
							} else {
								// sending feedback message to client
								send_message(i, UNSUBSCRIBE_NOTIF, strlen(topic) + 1, topic, 0);
							}
						}
						free_message(&message);
					}
				}
			}
		}
	}
	return 0;
}
