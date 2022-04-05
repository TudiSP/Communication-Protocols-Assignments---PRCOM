#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"
#include "utils/protocol.h"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_address server_port\n", file);
	exit(0);
}

void handle_udp_notif(myProtocol message) {
			char topic[51];
			uint8_t type;
			uint8_t sign, pw;
			uint16_t short_uns;
			uint32_t number, l;
			struct sockaddr_in udp_client_addr;

			memset(topic, 0, TOPIC_OFFSET + 1);
			memcpy(topic, message.payload, TOPIC_OFFSET);
			memcpy(&type, message.payload + TOPIC_OFFSET, TYPE_OFFSET);

			switch ((int) type)
			{
			case 0:
				memcpy(&sign, message.payload + TOPIC_OFFSET + TYPE_OFFSET, 1);
				memcpy(&number, message.payload + TOPIC_OFFSET + TYPE_OFFSET + 1, 4);

				number = ntohl(number);

				memcpy(&udp_client_addr.sin_addr, message.payload + TOPIC_OFFSET + TYPE_OFFSET + 5, IP_OFFSET);
				memcpy(&udp_client_addr.sin_port, message.payload + TOPIC_OFFSET + TYPE_OFFSET + 5 + IP_OFFSET, PORT_OFFSET);
				if (sign) {
					printf("%s:%u - %s - INT - -%d\n", inet_ntoa(udp_client_addr.sin_addr), ntohl(udp_client_addr.sin_port),
					 topic, number);
				} else {
					printf("%s:%u - %s - INT - %d\n", inet_ntoa(udp_client_addr.sin_addr), ntohl(udp_client_addr.sin_port),
					 topic, number);
				}
				break;

			case 1:
				memcpy(&short_uns, message.payload + TOPIC_OFFSET + TYPE_OFFSET, 2);

				short_uns = ntohs(short_uns);
				float short_real = (float)((long) short_uns);
				short_real /= 100;

				memcpy(&udp_client_addr.sin_addr, message.payload + TOPIC_OFFSET + TYPE_OFFSET + 2, IP_OFFSET);
				memcpy(&udp_client_addr.sin_port, message.payload + TOPIC_OFFSET + TYPE_OFFSET + 2 + IP_OFFSET, PORT_OFFSET);
				printf("%s:%u - %s - SHORT_REAL - %.2f\n", inet_ntoa(udp_client_addr.sin_addr), ntohl(udp_client_addr.sin_port),
				 topic, short_real);
				break;
			
			case 2:
				number = 0;
				sign = 0;
				memcpy(&sign, message.payload + TOPIC_OFFSET + TYPE_OFFSET, 1);
				memcpy(&number, message.payload + TOPIC_OFFSET + TYPE_OFFSET + 1, 4);
				memcpy(&pw, message.payload + TOPIC_OFFSET + TYPE_OFFSET + 5, 1);

				number = ntohl(number);
				float real = (float)((long) number);
				for (unsigned int i = 0; i < (unsigned int) pw; i++) {
					real /=10;
				}

				memcpy(&udp_client_addr.sin_addr, message.payload + TOPIC_OFFSET + TYPE_OFFSET + 6, IP_OFFSET);
				memcpy(&udp_client_addr.sin_port, message.payload + TOPIC_OFFSET + TYPE_OFFSET + 6 + IP_OFFSET, PORT_OFFSET);
				if (sign) {
					printf("%s:%u - %s - FLOAT - -%.*f\n", inet_ntoa(udp_client_addr.sin_addr), ntohl(udp_client_addr.sin_port),
					 topic,(unsigned int) pw, real);
				} else {
					printf("%s:%u - %s - FLOAT - %.*f\n", inet_ntoa(udp_client_addr.sin_addr), ntohl(udp_client_addr.sin_port),
					 topic,(unsigned int) pw, real);
				}
				break;

			case 3:
                if (message.payload[TOPIC_OFFSET + TYPE_OFFSET + MAX_UDP_PAYLOAD] != '\0') {
                    l = 1500;
                } else {
                    l = strlen(message.payload + TOPIC_OFFSET + TYPE_OFFSET) + 1;
                }
				memcpy(&udp_client_addr.sin_addr, message.payload + TOPIC_OFFSET + TYPE_OFFSET + l, IP_OFFSET);
				memcpy(&udp_client_addr.sin_port, message.payload + TOPIC_OFFSET + TYPE_OFFSET + l + IP_OFFSET, PORT_OFFSET);
				printf("%s:%u - %s - STRING - %s\n", inet_ntoa(udp_client_addr.sin_addr), ntohl(udp_client_addr.sin_port),
				 topic, message.payload + TOPIC_OFFSET + TYPE_OFFSET);
				break;
			default:
				break;
			}
}

void handle_subscribe_notif(myProtocol message) {
	printf("Subscribed to topic.\n");
}

void handle_unsubscribe_notif(myProtocol message) {
	printf("Unsubscribed from topic.\n");
}

int main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	int sockfd, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];

	if (argc < 4) {
		usage(argv[0]);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	// dezactivez algoritmul lui NAGLE
	int ok = 1;
	ret = setsockopt(sockfd, IPPROTO_TCP,  TCP_NODELAY, &ok, 4);
	DIE(ret < 0, "socket options");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

    // verificam lungimea ID-ului 
    if (strlen(argv[1]) > 10) {
        perror("ID size too big");
        close(sockfd);
        exit(-1);
    }

    // trimitem id-ul clientului la server pentru autentificare

    char id[11];
    strcpy(id, argv[1]);
	
	send_message(sockfd, 0, strlen(id) + 1, id, 0);

    // asteptam raspunsul
	myProtocol message;
    receive_message(sockfd, &message, 0);

    // verificam daca serverul ne-a autentificat
    if (strcmp(message.payload, "ER") == 0) {
        close(sockfd);
        exit(-1);
    }

	//dezalocam payload-ul mesajului
	free_message(&message);
    
	while (1) {
		int nfds = sockfd + 1;
		fd_set read_set;
		FD_ZERO(&read_set);
		FD_SET(STDIN_FILENO, &read_set);
		FD_SET(sockfd, &read_set);

		int s = select(nfds, &read_set, NULL, NULL, NULL);

		DIE(s < 0, "select");

		if(FD_ISSET(STDIN_FILENO, &read_set)) {
			// standard input reading
			// send to server
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);

			if (strncmp(buffer, "exit", 4) == 0) {
				break;
			}

		// sending the message to server
		send_message(sockfd, 0, strlen(buffer) + 1, buffer, 0);
		}

		if(FD_ISSET(sockfd, &read_set)) {
			// reading from socket
			// standard output writing
			myProtocol message;
			int r = receive_message(sockfd, &message, 0);

			if (r == 0) {
				break;
			}
			if (r < 0) {
				perror("recv");
				exit(-1);
			}

			switch (message.type) {
			case UDP_NOTIF:
				handle_udp_notif(message);
				break;
			
			case SUBSCRIBE_NOTIF:
				//print_message(message);
				handle_subscribe_notif(message);
				break;

			case UNSUBSCRIBE_NOTIF:
				handle_unsubscribe_notif(message);
				break;
			default:
				break;
			}
			
		}		
	}

	close(sockfd);
	return 0;
}
