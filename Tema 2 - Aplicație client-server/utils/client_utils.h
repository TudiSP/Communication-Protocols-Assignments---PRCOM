#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../helpers.h"
#include "protocol.h"

typedef struct cli_info {
	char id[11];
	char **topics;
    int subscription_nr;
	int max_subscription;
	int *sf;
    int sockfd;
	int is_logged;
	myProtocol *log_messages;
	int log_nr;
	int max_log_nr;
}cli_info;

/**
 * @brief adds a new client to client info array with given id and socket
 * file descriptor
 * 
 * @param clients client array
 * @param client_nr number of clients
 * @param max_client_nr max number of clients
 * @param id client id
 * @param sockfd socket file descriptor
 * @return -1 for error(client already registered) and 0 for success 
 */
int add_client(cli_info **clients, int *client_nr, int *max_client_nr, char *id, int sockfd);

/**
 * @brief removes a client form client info array with given id
 * 
 * @param clients client array
 * @param client_nr number of clients
 * @param id client id
 */
void remove_client(cli_info *clients, int *client_nr, char *id);

/**
 * @brief search for a client in the client info array by it's id
 * 
 * @param clients client array
 * @param client_nr number of clients
 * @param id client id
 * @return index of the client in the original array, -1 for error
 * (no client by that id found)
 */
int search_client_by_id(cli_info *clients, int client_nr, char *id);

/**
 * @brief search for a client in the client info array by it's socket
 * file descriptor open at the time
 * 
 * @param clients client array
 * @param client_nr number of clients
 * @param id client id
 * @return index of the client in the original array, -1 for error
 * (no client by that id found)
 */
int search_client_by_socket(cli_info *clients, int client_nr, int sockfd);

/**
 * @brief subscribe a client to a topic by adding a topic to its topic array.
 * PS: here we do allocations or reallocations for the topic array and sf array
 * if needed.
 * @param client the client.
 * @param topic the topic to subscribe to, max 50 characters char.
 * @param sf store-forward value, 0 or 1.
 */
int subscribe(cli_info *client, char *topic, int sf);

/**
 * @brief unsubscribe a client from a topic by removing a topic from its topic array.
 * 
 * @param client the client.
 * @param topic the topic to unsubscribe from, max 50 characters char.
 */
int unsubscribe(cli_info *client, char *topic);

/**
 * @brief send messages via TCP sockets to notify subscribers of new messages from their
 * subscribed topics.
 * 
 * @param clients the client list
 * @param client_nr the number of clients
 * @param topic the topic of the UDP message
 * @param type the type for UDP message
 * @param buffer the whole buffer including topic, type and payload .
 * @param cli_addr UDP client port and address
 */
void notify_clients(cli_info *clients, int client_nr,
 char *topic, uint8_t type, char *buffer, struct sockaddr_in cli_addr);

/**
 * @brief adds a message to a client's log
 * 
 * @param client 
 * @param type the type of message
 * @param size the size of the payload
 * @param payload the buffer to be stored
 */
void add_message_to_log(cli_info *client, u_int8_t type, uint32_t size, char *payload);
/**
 * @brief sends the stored messages to a client
 * 
 * @param client 
 */
void send_log(cli_info client);
/**
 * @brief deallocates a client's log
 * 
 * @param client 
 */
void remove_log(cli_info *client);

/**
 * @brief checks to see if client has a topic with a SF option enabled
 * 
 * @param client 
 * @return int 
 */
int sf_check(cli_info client);
/**
 * @brief deallocates all clients
 * 
 * @param clients 
 * @param cli_nr 
 */
void free_all_clients(cli_info *clients, int cli_nr);

/**
 * @brief debugging tool. prints a client's most important data
 * 
 * @param client 
 */
void print_client(cli_info client);
