#include "utils/protocol.h"

char* create_buffer(uint8_t type, uint32_t size, char* payload) {
    char *p = (char *) malloc(sizeof(type) + size);
    if (p == NULL) {
        perror("MALLOC FAIL");
        exit(-1);
    }
    char *buffer = p;
    memset(buffer, 0, sizeof(type) + size);

    memcpy(buffer, &type, 1);
    memcpy(buffer + 1, payload, size);
    return buffer;
}

void initialise_message(myProtocol *message, u_int32_t size) {
    message->size = size;
    message->payload = (char *) malloc(size);
}

void free_message(myProtocol *message) {
    free(message->payload);
}

void send_message(int sockfd, uint8_t type, uint32_t size,
    char* payload, int flags) {
        char *buffer = create_buffer(type, size, payload);

        // sending the size of the buffer firsthand
        uint32_t size_network_order = htonl(size);
        int ret = send(sockfd, &size_network_order, sizeof(size_network_order), flags);
        DIE(ret < 0, "send");

        // sending the actual buffer
        ret = send(sockfd, buffer, size + 1, flags);
        DIE(ret < 0, "send");

        // we don't need the buffer anymore
        free(buffer);
}

int receive_message(int sockfd, myProtocol *message, int flags) {
    uint32_t size_network_order;
    // receiving the length of the buffer firsthand
    int n = recv(sockfd, &size_network_order, sizeof(size_network_order), flags);

    DIE(n < 0, "recv");
    if (n == 0) {
        return 0;
    }
   
    initialise_message(message, ntohl(size_network_order));
    
    char buf[message->size + 1];
    memset(buf, 0, message->size + 1);
    n = recv(sockfd, buf, message->size + 1, flags);
    DIE(n < 0, "recv");

    //parsing the buffer
    memcpy(&message->type, buf, 1);
    memcpy(message->payload, buf + 1, message->size);
    return n;
}


void print_message(myProtocol message) {
    printf("TYPE - %d, SIZE - %u\nBUFFER - [%s]\n", (int) message.type,
     (unsigned int) message.size, message.payload);
}