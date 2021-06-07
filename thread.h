#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "list.h"


#define MSG_MAX_LEN 1024 



struct shutdown {
    int sock_fd;
    pthread_t * keyboard;
    pthread_t * sender;
    pthread_t * receiver;
    pthread_t * printer;
};


struct UDP_params {
    int sock_fd;
    struct sockaddr_in *peer;
    struct List_s *pList;
    void * shutdown_args;  
};
