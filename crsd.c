#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <pthread.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interface.h"
#include "additional.h"

int buffercapacity = MAX_DATA;

static struct Chat_Room chats[MAX_DATA];
static int size = 0;
static int current_port;

int connect_server_side(const int port, char* comm);

char* populate_data (char* name){
    printf("I entered the populate data\n");
    printf("Current size is: %d\n Current chats\n", size);
    for (int i = 0; i < size; ++i){
        printf("%s ", chats[i].name);
    }
    printf("\n");
    for (int i = 0; i < size; ++i){
        if (!strcmp(name, chats[i].name)){
            printf("server: name exists\n");
            return "exists";
        }
    }
    printf("Adding new chat room\n");
    current_port += 1;
    int main_sockfd = connect_server_side(current_port, "NEWCHAT");
    strcpy(chats[size].name, name);
    chats[size].num_mem = 0;
    chats[size].port = current_port;
    chats[size].sockfd = main_sockfd;
    chats[size].client_sockets_size = 0;
    size += 1;
    printf("size: %d\n", size);
    printf("server: name added\n");
    return "success";
}

void process_create(int sockfd, char* name){
    char* status = populate_data(name);
    struct Reply reply;
    if (!strcmp(status, "exists")){
        reply.status = FAILURE_ALREADY_EXISTS;
    }
    else {
        reply.status = SUCCESS;
    }
    send(sockfd, &reply, sizeof(reply), 0);
}

void handle_msg(int sockfd, int chat_index){
    printf("\nHANDLE MESSAGES\n");
    printf("server: handle message\n");
    printf("server: current socket: %d\n List of sockets on current port: ", sockfd);
    for (int i = 0; i < chats[chat_index].client_sockets_size; ++i){
        printf("%d ", chats[chat_index].arr_client_sockets[i]);
    }
    printf("\n");

    char buffer[buffercapacity];

    while (1){
        memset(buffer, 0, sizeof(buffer));

        if (!buffer){
            perror ("Cannot allocate memory for messages\n");
            exit(1);
        }

        int bytes = recv(sockfd, buffer, buffercapacity, 0);
        if (bytes < 0){
            perror("server: Client side message terminated abnormally\n");
            break;
        }
        else if (bytes == 0){
            perror("server: cannot read a message\n");
            break;
        }
        printf("The message buffer: %s\n", buffer);
        if (strcmp(buffer, "Q")) {
            for (int i = 0; i < chats[chat_index].client_sockets_size; ++i){
                if (chats[chat_index].arr_client_sockets[i] != sockfd){
                    printf("Sending through socket %d\n", chats[chat_index].arr_client_sockets[i]);
                    send(chats[chat_index].arr_client_sockets[i], buffer, buffercapacity, 0);
                }
            }
        }
        else {
            int index = 0;
            for (int i = 0; i < chats[chat_index].num_mem; ++i){
                if (sockfd == chats[chat_index].arr_client_sockets[i]){
                    index = i;
                    break;
                }
            }
            for (int i = index; i < chats[chat_index].num_mem; ++i){
                chats[chat_index].arr_client_sockets[i] = chats[chat_index].arr_client_sockets[i+1];
            }
            chats[chat_index].num_mem -= 1;
            chats[chat_index].client_sockets_size -= 1;
            close(sockfd);
            break;
        }
    }
}

void process_messages(int port, int server_sockfd, int chats_index){
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;

    printf("PRCOESS_MESSAGES I am in the chat mode\n");
    sin_size = sizeof(their_addr);
    int client_socket = accept(server_sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if(client_socket == -1){
        perror("server: accept\n");
    }
    else {
        printf("PROCESS_MESSAGES The client_socket is: %d\n", client_socket);
        chats[chats_index].arr_client_sockets[chats[chats_index].client_sockets_size] = client_socket;
        chats[chats_index].client_sockets_size += 1;
        handle_msg(client_socket, chats_index);
    }
}

void process_join(int sockfd, char* name){
    struct Reply reply;
    int exists = 0;
    int main_sockfd;
    int index = 0;
    for (int i = 0; i < size; ++i){
        if (!strcmp(name, chats[i].name)){
            reply.num_member = chats[i].num_mem;
            reply.port = chats[i].port;
            reply.status = SUCCESS;
            reply.num_member = chats[i].num_mem;
            chats[i].num_mem += 1;
            main_sockfd = chats[i].sockfd;
            exists = 1;
            index = i;
            break;
        }
    }
    if (exists == 0) {
        reply.status = FAILURE_NOT_EXISTS;
    }
    send(sockfd, &reply, sizeof(reply), 0);

    if (exists == 1){
        printf("Process JOIN socket: %d\n", main_sockfd);
        printf("Process JOIN port: %d\n", reply.port);
        process_messages(reply.port, main_sockfd, index);
    }
}

void process_delete(int sockfd, char* name){
    int index = -1;
    struct Reply reply;
    char delete_msg[] = "Warnning: the chatting room is going to be closed...";
    for (int i = 0; i < size; ++i){
        if (!strcmp(name, chats[i].name)){
            index = i;
            close(chats[i].sockfd);
            for (int j = 0; j < chats[i].client_sockets_size; ++j){
                send(chats[i].arr_client_sockets[j], delete_msg, sizeof(delete_msg), 0);
                close(chats[i].arr_client_sockets[j]);
            }
            chats[i].client_sockets_size = 0;
            chats[i].num_mem = 0;
            break;
        }
    }
    if (index != -1){
        for (int i = index; i < size-1; ++i){
            chats[i] = chats[i+1];
        }
        size -= 1;
        reply.status = SUCCESS;
    }
    else {
        reply.status = FAILURE_NOT_EXISTS;
    }
    send(sockfd, &reply, sizeof(reply), 0);
}

void process_list(int sockfd){
    printf("I am processing list\n");
    struct Reply reply;
    memset(reply.list_room, 0, sizeof(reply.list_room));
    if (size > 0){
        for (int i = 0; i < size; ++i){
            strcat(reply.list_room, chats[i].name);
            strcat(reply.list_room, ", ");
        }
    }
    else {
        strcat(reply.list_room, "empty");
    }
    reply.status = SUCCESS;
    send(sockfd, &reply, sizeof(reply), 0);
}

void process_unknown(int sockfd){
    struct Reply reply;
    reply.status = FAILURE_INVALID;
    send(sockfd, &reply, sizeof(reply), 0);
}

void process_recv(int sockfd, char* buffer){
    char* command = strtok(buffer, " ");
    char* name = strtok(NULL, " ");
    printf("server: process_recv command: %s, name: %s\n", command, name);
    if (!strcmp(command, "CREATE") && name != NULL){
        process_create(sockfd, name);
    }
    else if (!strcmp(command, "JOIN") && name != NULL){
        process_join(sockfd, name);
    }
    else if (!strcmp(command, "DELETE") && name != NULL){
        process_delete(sockfd, name);
    }
    else if (!strcmp(command, "LIST") && name == NULL){
        process_list(sockfd);
    }
    else {
        process_unknown(sockfd);
    }
}

void handle_loop(int sockfd){
    while (1){
        char buffer[buffercapacity];
        memset(buffer, 0, sizeof(buffer));
        printf("server: I entered the handle loop\n");

        if (!buffer){
            perror ("Cannot allocate memory for server buffer\n");
            exit(1);
        }

        int bytes = recv(sockfd, buffer, buffercapacity, 0);
        if (bytes < 0){
            perror("server: Client side terminated abnormally\n");
            break;
        }
        else if (bytes == 0){
            perror("server: Could not read anything\n");
            break;
        }
        printf("The handle loop buffer: %s\n", buffer);
        process_recv(sockfd, buffer);
    }
}

int connect_server_side(const int port, char* comm){
    struct addrinfo hints, *serv;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    char s[INET6_ADDRSTRLEN];
    int rv;
    int sockfd;
    char port_str[20];
    sprintf(port_str, "%d", port);
    printf("server: the port number entered in getaddrinfo is: %s\n", port_str);

    memset(&hints, 0, sizeof (hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, port_str, &hints, &serv)) != 0) { // since it is a server we dont have a name, lookup
        perror("getaddrinfo");
        exit (-1);
    }
    if ((sockfd = socket(serv->ai_family, serv->ai_socktype, serv->ai_protocol)) == -1) { // make a socket
        perror("server: socket");
        exit (-1);
    }
    if (bind(sockfd, serv->ai_addr, serv->ai_addrlen) == -1) { // server is attaching to a port that we passed in getaddrinfo()
        close(sockfd);
        if (!strcmp(comm, "NEWCHAT")){
            current_port += 1;
            return connect_server_side(current_port, "NEWCHAT");
        }
        else {
            perror("server: bind");
            exit (-1);
        }
    }
    printf("The new port number is: %d\n", current_port);
    printf("I reached here\n");
    freeaddrinfo(serv); // all done with this structure

    if (listen(sockfd, 20) == -1) { // setting up server, incoming connections wait in a queue before accepted, listen() sets the size of the queue
        perror("listen");
        exit(1);
    }

    printf("The server is ready to accept conection in port %d\n", current_port);
    return sockfd;
}

int main(int argc, char** argv){
    if (argc != 2) {
		fprintf(stderr,
				"usage: enter host address\n");
		exit(1);
	}
    printf("The port number is %d\n", atoi(argv[1]));

    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;

    current_port = atoi(argv[1]);
	
	int main_channel = connect_server_side(atoi(argv[1]), NULL);
    printf("MAIN channel socket: %d\n", main_channel);
	printf("server: waiting for connections...\n");

    while(1){
        printf("I am waiting\n");
        sin_size = sizeof(their_addr);
        int client_socket = accept(main_channel, (struct sockaddr *)&their_addr, &sin_size);
        if(client_socket == -1){
            perror("server: accept\n");
            continue;
        }
        printf("MAIN client socket: %d\n", client_socket);
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_loop, client_socket);
        pthread_detach(thread_id);
    }

    printf("Server terminated\n");
}