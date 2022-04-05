#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../helpers.h"

#define TYPE_OFFSET 1
#define PORT_OFFSET 2
#define IP_OFFSET 4
#define TOPIC_OFFSET 50
#define MAX_UDP_PAYLOAD 1500
#define UDP_NOTIF 1
#define SUBSCRIBE_NOTIF 2
#define UNSUBSCRIBE_NOTIF 3

typedef struct myProtocol {
    uint8_t type;
    uint32_t size;
    char *payload;
}myProtocol;

/**
 * @brief Construct a buffer using given parameters in that order
 * 
 * @param type 
 * @param size 
 * @param payload 
 * @return char* 
 */
char* create_buffer(uint8_t type, uint32_t size, char* payload);

/**
 * @brief deallocate message instance
 * 
 * @param message 
 */
void free_message(myProtocol *message);

/**
 * @brief Sends 2 messages via TCP to "sockfd" socket (it is a implementation
 * of my own protocol). It first sends the size of the payload and then the
 * actual type + payload for efficiency. The type is used to communicate to the client
 * which kind of message is this (0 for no type).
 * 
 * @param sockfd the socket file descriptor whom which to send data to
 * @param type the type of message
 * @param size the size of the payload
 * @param payload the actual data
 * @param flags 
 */
void send_message(int sockfd, uint8_t type, uint32_t size,
    char* payload, int flags);

/**
 * @brief Receives 2 messages via TCP and parses the data into a myProtocol-type
 * variable. The first message received is the size of the payload and then the
 * actual type + payload is received.
 * 
 * @param sockfd the socket file descriptor from which to receive data from
 * @param message the myProtocol variable which will encapsulate the data sent
 * @param flags 
 * @return int - returns the number of bytes read by the second recv() or if
 * the socket is closed it returns 0 directly.
 */
int receive_message(int sockfd, myProtocol *message, int flags);

/**
 * @brief debugging tool. prints a message
 * 
 * @param message 
 */
void print_message(myProtocol message);