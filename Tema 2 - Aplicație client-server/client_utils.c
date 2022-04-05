#include "./utils/client_utils.h"

int add_client(cli_info **clients, int *client_nr, int *max_client_nr, char *id, int sockfd) {
    for (int i = 0; i < *client_nr; i++) {
        if (strcmp(id, (*clients)[i].id) == 0) {
            return -1;
        }
    }

    if ((*client_nr) + 1 > (*max_client_nr)) {
        (*max_client_nr) *= 2;
        cli_info *p = (cli_info *) realloc(*clients, (*max_client_nr) * sizeof(cli_info));
        if (p == NULL) {
            perror("REALLOC FAIL");
            exit(-1);
        }
        *clients = p;
    }

    (*client_nr)++;
    strcpy((*clients)[*client_nr - 1].id, id);
    (*clients)[*client_nr - 1].topics = NULL;
    (*clients)[*client_nr - 1].subscription_nr = 0;
    (*clients)[*client_nr - 1].max_subscription = 0;
    (*clients)[*client_nr - 1].sockfd = sockfd;
    (*clients)[*client_nr - 1].sf = NULL;
    (*clients)[*client_nr - 1].is_logged = 1;
    (*clients)[*client_nr - 1].log_messages = NULL;
    (*clients)[*client_nr - 1].log_nr = 0;
    (*clients)[*client_nr - 1].max_log_nr = 0;
    return 0;
}

void free_client_topics (cli_info client) {
    for (int i = 0; i < client.subscription_nr; i++) {
        free(client.topics[i]);
    }
    free(client.sf);
    free(client.topics);
}

void remove_client(cli_info *clients, int *client_nr, char *id) {
    for (int i = 0; i < *client_nr; i++) {
        if (strcmp(id, clients[i].id) == 0) {
            // we first free our allocations done by malloc for the topics
            // and sf array for the client we want to remove.
            free_client_topics(clients[i]);
            for (int j = i ; j < *client_nr - 1; j++) {
                clients[j] = clients[j + 1];
            }
        }
    }
    (*client_nr)--;
}

int search_client_by_id(cli_info *clients, int client_nr, char *id) {
    for (int i = 0; i < client_nr; i++) {
        if (strcmp(id, clients[i].id) == 0) {
            return i;
        }
    }
    return -1;
}

int search_client_by_socket(cli_info *clients, int client_nr, int sockfd) {
    for (int i = 0; i < client_nr; i++) {
        if (clients[i].sockfd == sockfd) {
            return i;
        }
    }
    return -1;
}

int subscribe(cli_info *client, char *topic, int sf) {
     // initial allocations
    if (client->subscription_nr == 0) {     
        client->max_subscription = 5;

        client->topics = (char **) malloc(client->max_subscription * sizeof(char *));
        for (int i = 0; i < client->max_subscription; i++) {
            client->topics[i] = (char *) malloc (51 * sizeof(char));
        }
        
        client->sf = (int *) malloc (client->max_subscription * sizeof(int));
    }
 
    for (int i = 0; i < client->subscription_nr; i++) {
        if (strcmp(client->topics[i], topic) == 0) {
            // client already subscribed to topic
            // but modify the sf value if needed
            if (client->sf[i] != sf) {
                client->sf[i] = sf;
                return 0;
            } else {
                return -1;
            }
        }
    }
    // dynamic reallocation of client topics and sf arrays
    if (client->subscription_nr + 1 > client->max_subscription) {
        // doubling the max size of arrays for spatial efficiency
        client->max_subscription *= 2;

        // dynamic reallocation of client topics array
        char **pc = (char **) realloc(client->topics, client->max_subscription * 51);
        if (pc) {
            client->topics = pc;
        } else {
            perror("REALLOC FAILED - TOPICS");
            exit(-1);
        }    
        
        // dynamic reallocation of client sf array
        int *pi = (int *) realloc(client->sf, client->max_subscription * sizeof(int));
        if (pi) {
            client->sf = pi;
        } else {
            perror("REALLOC FAILED - SF");
            exit(-1);
        }

        for (int i = client->subscription_nr; i < client->max_subscription; i++) {
            client->topics[i] = malloc(51 * sizeof(char));
        }
 
    }

    strcpy(client->topics[client->subscription_nr], topic);
    client->sf[client->subscription_nr] = sf;
    client->subscription_nr++;

    return 0;
}

int unsubscribe(cli_info *client, char *topic) {
    int found = 0;
    for (int i = 0; i < client->subscription_nr; i++) {
        if (strcmp(client->topics[i], topic) == 0) {
            //we can unsubscribe safely
            found = 1;
            for (int j = 0; j < client->subscription_nr - 1; j++) {
                strcpy(client->topics[j], client->topics[j + 1]);
                client->sf[j] = client->sf[j +1];
            }
            break;
        }
    }
    if (found) {
        client->subscription_nr--;
        return 0;
    } else {
        return -1;
    }
}

void notify_clients(cli_info *clients, int client_nr, char *topic, uint8_t type, char *buffer, struct sockaddr_in cli_addr) {
    for (int i = 0; i < client_nr; i++) {
        for (int j = 0; j < clients[i].subscription_nr; j++) {
            if (strcmp(clients[i].topics[j], topic) == 0) {
               uint32_t l;
               switch ((int) type) {
                case 0:
                   memcpy(buffer + TOPIC_OFFSET + TYPE_OFFSET + 5, &cli_addr.sin_addr, IP_OFFSET);
				   memcpy(buffer + TOPIC_OFFSET + TYPE_OFFSET + 5 + IP_OFFSET, &cli_addr.sin_port, PORT_OFFSET);
                   if (clients[i].sf[j] && clients[i].is_logged == 0) {
                       add_message_to_log(&clients[i], UDP_NOTIF,
                        TOPIC_OFFSET + TYPE_OFFSET + 5 + IP_OFFSET + PORT_OFFSET, buffer);
                   } else if (clients[i].is_logged) {
                    send_message(clients[i].sockfd, UDP_NOTIF,
                    TOPIC_OFFSET + TYPE_OFFSET + 5 + IP_OFFSET + PORT_OFFSET, buffer, 0);
                   }
                   break;
                
                case 1:
                   memcpy(buffer + TOPIC_OFFSET + TYPE_OFFSET + 2, &cli_addr.sin_addr, IP_OFFSET);
				   memcpy(buffer + TOPIC_OFFSET + TYPE_OFFSET + 2 + IP_OFFSET, &cli_addr.sin_port, PORT_OFFSET);
                   if (clients[i].sf[j] && clients[i].is_logged == 0) {
                       add_message_to_log(&clients[i], UDP_NOTIF,
                        TOPIC_OFFSET + TYPE_OFFSET + 2 + IP_OFFSET + PORT_OFFSET, buffer);
                   } else if (clients[i].is_logged) {
                   send_message(clients[i].sockfd, UDP_NOTIF,
                    TOPIC_OFFSET + TYPE_OFFSET + 2 + IP_OFFSET + PORT_OFFSET, buffer, 0);
                   }
                   break;
                
                case 2:
                   memcpy(buffer + TOPIC_OFFSET + TYPE_OFFSET + 6, &cli_addr.sin_addr, IP_OFFSET);
				   memcpy(buffer + TOPIC_OFFSET + TYPE_OFFSET + 6 + IP_OFFSET, &cli_addr.sin_port, PORT_OFFSET);
                   if (clients[i].sf[j] && clients[i].is_logged == 0) {
                        add_message_to_log(&clients[i], UDP_NOTIF,
                        TOPIC_OFFSET + TYPE_OFFSET + 6 + IP_OFFSET + PORT_OFFSET, buffer);
                   } else if (clients[i].is_logged) {
                        send_message(clients[i].sockfd, UDP_NOTIF,
                        TOPIC_OFFSET + TYPE_OFFSET + 6 + IP_OFFSET + PORT_OFFSET, buffer, 0);
                   }
                   break;
                
                case 3:
                    if (buffer[TOPIC_OFFSET + TYPE_OFFSET + MAX_UDP_PAYLOAD] != '\0') {
                        l = 1500;
                    } else {
                        l = strlen(buffer + TOPIC_OFFSET + TYPE_OFFSET) + 1;
                    }
                   memcpy(buffer + TOPIC_OFFSET + TYPE_OFFSET + l, &cli_addr.sin_addr, IP_OFFSET);
				   memcpy(buffer + TOPIC_OFFSET + TYPE_OFFSET + l + IP_OFFSET, &cli_addr.sin_port, PORT_OFFSET);
                   if (clients[i].sf[j] && clients[i].is_logged == 0) {
                        add_message_to_log(&clients[i], UDP_NOTIF,
                        TOPIC_OFFSET + TYPE_OFFSET + l + IP_OFFSET + PORT_OFFSET, buffer);
                   } else if (clients[i].is_logged) {
                        send_message(clients[i].sockfd, UDP_NOTIF,
                        TOPIC_OFFSET + TYPE_OFFSET + l + IP_OFFSET + PORT_OFFSET, buffer, 0);
                   }
                   break;
               
               default:
                   break;
               }
               break;
            }
        }
    }
}

void add_message_to_log(cli_info *client, u_int8_t type, uint32_t size, char *payload) {
    if (client->log_messages == NULL) {
        client->max_log_nr = 50;
        client->log_messages = (myProtocol *) malloc(client->max_log_nr * sizeof(myProtocol));
    }

    if (client->log_nr + 1 > client->max_log_nr) {
        client->max_log_nr *= 2;
        myProtocol *p = realloc(client->log_messages, client->max_log_nr * sizeof(myProtocol));
        if (p == NULL) {
            perror("REALLOC FAIL");
            exit(-1);
        }
        client->log_messages = p;
    }
    
    client->log_messages[client->log_nr].type = type;
    client->log_messages[client->log_nr].size = size;
    client->log_messages[client->log_nr].payload = (char *)malloc(size * sizeof(char));
    memcpy(client->log_messages[client->log_nr].payload, payload, size);
    client->log_nr++;
}

void send_log(cli_info client) {
    for (int i = 0; i < client.log_nr; i++) {
        myProtocol message = client.log_messages[i];
        send_message(client.sockfd, message.type, message.size, message.payload, 0);
    }
}

void remove_log(cli_info *client) {
    for (int i = 0; i < client->log_nr; i++) {
        free(client->log_messages[i].payload);
    }
    free(client->log_messages);
    client->log_nr = 0;
}

int sf_check(cli_info client) {
    for (int i = 0; i < client.subscription_nr; i++) {
        if (client.sf[i] == 1) {
            return 1;
        }
    }
    return 0;
}

void free_all_clients(cli_info *clients, int cli_nr) {
    for (int i = 0; i < cli_nr; i++) {
        free_client_topics(clients[i]);
        for (int j = 0; j < clients[i].log_nr; j++) {
            free_message(&clients[i].log_messages[j]);
        }
        free(clients[i].log_messages);
    }
    free(clients);
}

void print_client(cli_info client) {
    printf("--------------\n");
    printf("ID: %s\nTOPICS: ", client.id);
    for (int i = 0; i < client.subscription_nr; i++) {
        printf("[%s, %d] ", client.topics[i], client.sf[i]);
    }
    printf("\nSUBSCRIPTION NUMBER: %d\nMAX_SUBSCRIPTION: %d\nSOCKFD: %d\n",
     client.subscription_nr, client.max_subscription, client.sockfd);
     printf("Messages:\n");

    if (client.log_messages != NULL) {
        for (int i = 0; i < client.log_nr; i++) {
            printf("index - %d ", i);
            print_message(client.log_messages[i]);
        }
    } else {
        printf("client log is NULL.\n");
    }
}