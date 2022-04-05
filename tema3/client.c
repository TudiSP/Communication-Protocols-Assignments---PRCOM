#include "client.h"

int main(int argc, char *argv[])
{
    char buff[MAXBUFFlEN];
    char session[MAXBUFFlEN];
    char token[500];
    memset(token, 0, MAXBUFFlEN);
    memset(session, 0, MAXBUFFlEN);

    while(1) {
        memset(buff, 0, MAXBUFFlEN);
        fgets(buff, MAXBUFFlEN, stdin);
        buff[strlen(buff) - 1] = 0;
        if (!strcmp(buff, "exit")) {
            return 0;
        }


        // REGISTER COMMAND
        if (!strcmp(buff, "register")) {
            printf("username=");
            memset(buff, 0, MAXBUFFlEN);
            fgets(buff, MAXBUFFlEN, stdin);

            char username[MAXUSER + 1];
            sscanf(buff, "%[^\n]s", username);

            printf("password=");
            memset(buff, 0, MAXBUFFlEN);
            fgets(buff, MAXBUFFlEN, stdin);

            char password[MAXPASS + 1];
            sscanf(buff, "%[^\n]s", password);

            int ret = client_register(username, password);
            if (!ret) {
                printf("[SUCCESS] Registered user \"%s\" with password \"%s\"\n", username, password);
            }
            continue;
        }

        // LOGIN COMMAND
        if (!strcmp(buff, "login")) {
            printf("username=");
            memset(buff, 0, MAXBUFFlEN);
            fgets(buff, MAXBUFFlEN, stdin);

            char username[MAXUSER + 1];
            sscanf(buff, "%[^\n]s", username);

            printf("password=");
            memset(buff, 0, MAXBUFFlEN);
            fgets(buff, MAXBUFFlEN, stdin);

            char password[MAXPASS + 1];
            sscanf(buff, "%[^\n]s", password);

        
            memset(session, 0, MAXBUFFlEN);
            int ret = client_login(username, password, session);
            if (!ret) {
                printf("[SUCCESS] Logged in with username \"%s\" and password \"%s\"\n", username, password);
            }
            continue;
        }

        // LOGOUT COMMAND
        if (!strcmp(buff, "logout")) {
            int ret = client_logout(session);
            if (!ret) {
                printf("[SUCCESS] User logged out succesfully\n");
            }
            continue;
        }

        // ENTER_LIBRARY COMMAND
        if (!strcmp(buff, "enter_library")) {
            int ret = client_enter_library(session, token);
            if (!ret) {
                printf("[SUCCESS] Library access granted\n");
            }
            continue;
        }

        // GET_BOOKS COMMAND
        if (!strcmp(buff, "get_books")) {
            int ret = client_get_books(token);
            if (!ret) {
                printf("[SUCCESS] Library listed\n");
            }
            continue;
        }

        // GET_BOOK COMMAND
        if (!strcmp(buff, "get_book")) {
            printf("id=");
            memset(buff, 0, MAXBUFFlEN);
            fgets(buff, MAXBUFFlEN, stdin);

            int id;
            sscanf(buff, "%d", &id);

            int ret = client_get_book(token, id);
            if (!ret) {
                printf("[SUCCESS] Book listed\n");
            }
            continue;
        }

        // ADD BOOK COMMAND
        if (!strcmp(buff, "add_book")) {
            printf("title=");
            memset(buff, 0, MAXBUFFlEN);
            fgets(buff, MAXBUFFlEN, stdin);
            
            char title[26];
            sscanf(buff, "%[^\n]s", title);

            printf("author=");
            memset(buff, 0, MAXBUFFlEN);
            fgets(buff, MAXBUFFlEN, stdin);
        
            char author[26];
            sscanf(buff, "%[^\n]s", author);

            printf("genre=");
            memset(buff, 0, MAXBUFFlEN);
            fgets(buff, MAXBUFFlEN, stdin);
        
            char genre[26];
            sscanf(buff, "%[^\n]s", genre);

            printf("publisher=");
            memset(buff, 0, MAXBUFFlEN);
            fgets(buff, MAXBUFFlEN, stdin);
        
            char publisher[26];
            sscanf(buff, "%[^\n]s", publisher);

            printf("page_count=");
            memset(buff, 0, MAXBUFFlEN);
            fgets(buff, MAXBUFFlEN, stdin);

            int page_count;
            sscanf(buff, "%d", &page_count);

            int ret = client_add_book(token, title, author, genre, publisher, page_count);
            if (!ret) {
                printf("[SUCCESS] New Book added\n");
            }
            continue;
        }

        // DELETE_BOOK COMMAND
        if (!strcmp(buff, "delete_book")) {
            printf("id=");
            memset(buff, 0, MAXBUFFlEN);
            fgets(buff, MAXBUFFlEN, stdin);

            int id;
            sscanf(buff, "%d", &id);

            int ret = client_delete_book(token, id);
            if (!ret) {
                printf("[SUCCESS] Book deleted\n");
                continue;
            }
        }
        // We only arrive here if we don't recognize any of the above commands
        printf("[UNKNOWN COMMAND] \"%s\"\n", buff);
    }
}

int client_register(char *username, char *password) {
    char *message;
    char *response;
    int sockfd;

    sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);
    
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "username", username);
    json_object_set_string(root_object, "password", password);
    serialized_string = json_serialize_to_string_pretty(root_value);

    message = compute_post_request("34.118.48.238", " /api/v1/tema/auth/register", NULL, "application/json",
              serialized_string, NULL, 0);
    send_to_server(sockfd, message);

    json_free_serialized_string(serialized_string);
    json_value_free(root_value);   
    
    response = receive_from_server(sockfd);

    char *first_line = strtok(response, "\n");
    response = strtok(NULL, "\0");

    if (strstr(first_line,"OK") == NULL && strstr(first_line,"Created") == NULL) {
        char *json_response = basic_extract_json_response(response);
        root_value = json_parse_string(json_response);
        root_object = json_value_get_object(root_value);
        printf("[ERROR] %s\n",json_object_get_string(root_object, "error"));
        json_value_free(root_value);
        return -1;
    }
    close_connection(sockfd);

    return 0;
}

int client_login(char *username, char *password, char *key) {
    char *message;
    char *response;
    int sockfd;

    sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);
    
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "username", username);
    json_object_set_string(root_object, "password", password);
    serialized_string = json_serialize_to_string_pretty(root_value);

    message = compute_post_request("34.118.48.238", " /api/v1/tema/auth/login", NULL, "application/json",
              serialized_string, NULL, 0);

    send_to_server(sockfd, message);

    json_free_serialized_string(serialized_string);
    json_value_free(root_value);   
    
    response = receive_from_server(sockfd);

    char *first_line = strtok(response, "\n");
    response = strtok(NULL, "\0");
    if (strstr(first_line,"OK") == NULL) {
        char *json_response = basic_extract_json_response(response);
        root_value = json_parse_string(json_response);
        root_object = json_value_get_object(root_value);
        printf("[ERROR] %s\n",json_object_get_string(root_object, "error"));
        return -1;
    }
   
    char *key_location = strstr(response, "connect.sid=");
    char *key_point = strtok(key_location, ";");

    strcpy(key, key_point);
    close(sockfd);

    return 0;
}

int client_logout(char *session) {
    char *message;
    char *response;
    int sockfd;

    sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);

    char **cookies = (char **) malloc (sizeof(char *));
    *cookies = calloc(4 + strlen(session) + 1, sizeof(char));
    sprintf(*cookies, "%s", session);

    message = compute_get_request("34.118.48.238", "/api/v1/tema/auth/logout", NULL, NULL, cookies, 1);

    send_to_server(sockfd, message);

    response = receive_from_server(sockfd);
    close(sockfd);

    char *first_line = strtok(response, "\n");
    response = strtok(NULL, "\0");
    if (strstr(first_line,"OK") == NULL) {
        char *json_response = basic_extract_json_response(response);
        JSON_Value *root_value = json_parse_string(json_response);
        JSON_Object *root_object = json_value_get_object(root_value);
        printf("[ERROR] %s\n",json_object_get_string(root_object, "error"));
        return -1;
    }
    return 0;
}

int client_enter_library(char *session, char *token) {
    char *message;
    char *response;
    int sockfd;

    sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);
    char **cookies = (char **) malloc (sizeof(char *));
    *cookies = calloc(strlen(session) + 1, sizeof(char));
    memset(cookies[0], 0, strlen(session) + 1);
    strcpy(cookies[0], session);

    message = compute_get_request("34.118.48.238", " /api/v1/tema/library/access", NULL, NULL, cookies, 1);

    send_to_server(sockfd, message);

    response = receive_from_server(sockfd);
    close(sockfd);

    char *first_line = strtok(response, "\n");
    response = strtok(NULL, "\0");

    char *json_response = basic_extract_json_response(response);
    JSON_Value *root_value = json_parse_string(json_response);
    JSON_Object *root_object = json_value_get_object(root_value);

    if (strstr(first_line,"OK") == NULL) {
        printf("[ERROR] %s\n",json_object_get_string(root_object, "error"));
        return -1;
    }

    strcpy(token, json_object_get_string(root_object, "token"));
    return 0;
}

int client_get_books(char *token) {
    char *message;
    char *response;
    int sockfd;

    sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);

    message = compute_get_request("34.118.48.238", "/api/v1/tema/library/books", token, NULL, NULL, 0);

    send_to_server(sockfd, message);

    response = receive_from_server(sockfd);
    close(sockfd);

    char *first_line = strtok(response, "\n");
    response = strtok(NULL, "\0");

    char *json_response = basic_extract_json_response(response);
    JSON_Value *root_value = json_parse_string(json_response);

    if (strstr(first_line,"OK") == NULL) {
        JSON_Object *root_object = json_value_get_object(root_value);
        printf("[ERROR] %s\n",json_object_get_string(root_object, "error"));
        return -1;
    }

    json_value_free(root_value);
    char *json_response_array = strstr(response, "[{");
    root_value = json_parse_string(json_response_array);

    if (json_value_get_type(root_value) != JSONArray) {
        JSON_Object *item = json_value_get_object(root_value);
        printf("ID [%d], TITLE [\"%s\"]\n", (int) json_object_get_number(item, "id"), json_object_get_string(item, "title"));
    } else {
        JSON_Array *root_array = json_value_get_array(root_value);
        for (int i = 0; i < json_array_get_count(root_array); i++) {
            JSON_Object *item = json_array_get_object(root_array, i);
            printf("ID [%d], TITLE \"%s\"\n", (int) json_object_get_number(item, "id"), json_object_get_string(item, "title"));
        } 
    }
    json_value_free(root_value);
    return 0;
}

int client_get_book(char *token, int bookid) {
    char *message;
    char *response;
    int sockfd;

    sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);

    char *url = malloc(100 * sizeof(char));
    memset(url, 0, 100);
    sprintf(url, "/api/v1/tema/library/books/%d", bookid);

    message = compute_get_request("34.118.48.238", url, token, NULL, NULL, 0);

    send_to_server(sockfd, message);

    response = receive_from_server(sockfd);
    close(sockfd);

    char *first_line = strtok(response, "\n");
    response = strtok(NULL, "\0");

    char *json_response = basic_extract_json_response(response);
    JSON_Value *root_value = json_parse_string(json_response);

    if (strstr(first_line,"OK") == NULL) {
        JSON_Object *root_object = json_value_get_object(root_value);
        printf("[ERROR] %s\n",json_object_get_string(root_object, "error"));
        return -1;
    }

    JSON_Object *item = json_value_get_object(root_value);
    printf("ID [%d]\nTITLE [\"%s\"]\nAUTHOR [\"%s\"]\nPUBLISHER [\"%s\"]\nGENRE [\"%s\"]\nPAGE_COUNT [%d]\n",
      bookid, json_object_get_string(item, "title"), json_object_get_string(item, "author"),
      json_object_get_string(item, "publisher"), json_object_get_string(item, "genre"), (int) json_object_get_number(item, "page_count"));
    
    json_value_free(root_value);
    return 0;
}

int client_add_book(char *token, char *title, char *author, char *genre, char *publisher, int page_count) {
    char *message;
    char *response;
    int sockfd;

    sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);
    
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "title", title);
    json_object_set_string(root_object, "author", author);
    json_object_set_string(root_object, "genre", genre);
    json_object_set_string(root_object, "publisher", publisher);
    json_object_set_number(root_object, "page_count", (double) page_count);
    serialized_string = json_serialize_to_string_pretty(root_value);

    message = compute_post_request("34.118.48.238", " /api/v1/tema/library/books", token, "application/json",
              serialized_string, NULL, 0);
    send_to_server(sockfd, message);

    json_free_serialized_string(serialized_string);
    json_value_free(root_value);   
    
    response = receive_from_server(sockfd);

    char *first_line = strtok(response, "\n");
    response = strtok(NULL, "\0");

    char *json_response = basic_extract_json_response(response);
    root_value = json_parse_string(json_response);

    if (strstr(first_line,"OK") == NULL) {
        root_object = json_value_get_object(root_value);
        printf("[ERROR] %s\n",json_object_get_string(root_object, "error"));
        return -1;
    }
    json_value_free(root_value);
    close_connection(sockfd);

    return 0;
}

int client_delete_book(char *token, int bookid) {
    char *message;
    char *response;
    int sockfd;

    sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);

    char *url = malloc(100 * sizeof(char));
    memset(url, 0, 100);
    sprintf(url, "/api/v1/tema/library/books/%d", bookid);

    message = compute_delete_request("34.118.48.238", url, token, NULL, 0);

    send_to_server(sockfd, message);

    response = receive_from_server(sockfd);
    close(sockfd);

    char *first_line = strtok(response, "\n");
    response = strtok(NULL, "\0");

    char *json_response = basic_extract_json_response(response);
    JSON_Value *root_value = json_parse_string(json_response);

    if (strstr(first_line,"OK") == NULL) {
        JSON_Object *root_object = json_value_get_object(root_value);
        printf("[ERROR] %s\n",json_object_get_string(root_object, "error"));
        return -1;
    }
    
    json_value_free(root_value);
    return 0;
}