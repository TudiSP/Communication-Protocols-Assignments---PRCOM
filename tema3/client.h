#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"

#define MAXBUFFlEN 256
#define MAXUSER 25
#define MAXPASS 25

int client_register(char *username, char *password);
int client_login(char *username, char *password, char *key);
int client_logout(char *session);
int client_enter_library(char *session, char *token);
int client_get_books(char *token);
int client_get_book(char *token, int bookid);
int client_add_book(char *token, char *title, char *author, char *genre, char *publisher, int page_count);
int client_delete_book(char *token, int bookid);