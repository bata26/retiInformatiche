#include <arpa/inet.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>

#define size 20
#define porta 4242

int main(int argc , char * argv[]){
    
    int socket_ascolto , socket_risposta , ret , len;
    struct sockaddr_in server_addr , client_addr;
    char buffer[size];


    //creo il socket di ascolto
    socket_ascolto = socket(AF_INET, SOCK_STREAM , 0);

    //pulisco sockaddr_in
    memset(&server_addr , 0 , sizeof(server_addr));
    server_addr.sin_port = htons(porta);
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET , "127.0.0.1" , &server_addr.sin_addr);

    //aggancio la porta al socket
    ret = bind(socket_ascolto , (struct sockaddr*)&server_addr , sizeof(server_addr));

    if(ret < 0){
        printf("Errore nella bind");
        exit(-1);
    }


    // listen
    ret = listen(socket_ascolto , 10);

    if(ret < 0 ){
        printf("problema nella listen");
        exit(-1);
    }

    // ciclo while accept

    while(1){

        //dimensione sockaddr del client
        len = sizeof(client_addr);

        //creo il socket di risposta nel momento in cui viene eseguita la accept
        socket_risposta = accept(socket_ascolto , (struct sockaddr *)&client_addr , &len);

        // a questo punto il nuovo socket di risposta è stato creato perchè bloccante
        ret = recv(socket_risposta , (void *)buffer , size , 0);

        //ho ricevuto, provo a stampare il buffer
        printf("Ecco cosa ho ricevuto");
        printf("Ho ricevuto %s" , buffer);

        close(socket_ascolto);
    }

}