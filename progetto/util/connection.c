#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>


//include costanti
#include "costanti.h"

//#include "connection.h"



// crea un socket di ascolto
int create_listener_socket(struct sockaddr_in * sockaddr , socklen_t * len , int port){
    int ret , sd;

    sd = socket(AF_INET , SOCK_DGRAM , 0);

    memset(sockaddr , 0 , sizeof((*sockaddr)));
    sockaddr->sin_family = AF_INET;
    sockaddr->sin_port = htons(port);
   
    inet_pton(AF_INET , LOCALHOST  , &sockaddr->sin_addr);

    (*len) = sizeof((*sockaddr));
    ret = bind(sd, (struct sockaddr*)sockaddr, (*len));

    if(ret < 0){
        perror("Errore in fase di bind: \n");
        exit(0);

    }

    return sd;


}