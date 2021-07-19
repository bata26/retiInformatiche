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
    int socketDes , ret , len;
    struct sockaddr_in server_addr;
    char buffer[size];

    //creo il socket
    socketDes = socket(AF_INET , SOCK_STREAM , 0);

    //creo la sockaddr
    memset(&server_addr , 0 , sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sinport = htons(4242);

    //assegno l'IP al socket
    inet_pton(AF_INET , "127.0.0.1" , &server_addr.sin_addr);

    //ho creato il socket, eseguo la connect
    ret = connect(socketDes , (struct sockaddr *) &server_addr , sizeof(server_addr));

    if(ret < 0){
        printf("Errore nella connect");
        exit(-1);
    }

    //inizializzo il buffer
    strcpy(buffer , "CIAONEE\0");
    len = size;

    //invio, funzione bloccante
    ret = send(socketDes , (void *)buffer ,len , 0 );

    if(ret < 0 ){
        printf("Impossibile inviare");
        exit(-1);
    }

    //invio concluso
    printf("Inviato al socket");
    close(socketDes);


}