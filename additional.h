#ifndef ADDITIONAL_H_
#define ADDITIONAL_H_
#include <ctype.h>
#include "interface.h"

struct Chat_Room {
    char name[MAX_DATA];
    int num_mem;
    int port;
    int sockfd;
    int arr_client_sockets[MAX_DATA];
    int client_sockets_size;
};

#endif 