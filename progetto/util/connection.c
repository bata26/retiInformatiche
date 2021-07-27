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

#include "connection.h"


void setup_addr(struct sockaddr_in * sockaddr , socklen_t * len , int port){
    memset(sockaddr , 0 , sizeof((*sockaddr)));
    sockaddr->sin_family = AF_INET;
    sockaddr->sin_port = htons(port);
   
    inet_pton(AF_INET , LOCALHOST  , &sockaddr->sin_addr);

    (*len) = sizeof((*sockaddr));
}


// crea un socket di ascolto
int create_listener_socket(struct sockaddr_in * sockaddr , socklen_t * len , int port){
    int ret , sd;
    // creo il socket
    sd = socket(AF_INET , SOCK_DGRAM , 0);

    // setup del sockaddr_in
    setup_addr(&sockaddr , &len , port);
    

    ret = bind(sd, (struct sockaddr*)sockaddr, (*len));

    if(ret < 0){
        perror("Errore in fase di bind: \n");
        exit(0);

    }

    return sd;

}


/*
 cosa mi serve:
 socket di invio
 buffer contenente il msg
 dest_addr -> sockaddr_in del dest
 addrlen -> lunghezza di dest_addr
*/

void send_pkt(int socket , char * msg , int buf_len , int port_dest , char * expected_ack){
    int ret , done;

    struct sockaddr_in dest_addr;
    socklen_t dest_len;

    struct sockaddr_in tmp_addr;
    socklen_t tmp_len;

    fd_set wait;
    char received[RECEIVED_LEN];



    // setup del dest_addr
    setup_addr(&dest_addr , &dest_len , port_dest);

    // finchè non ho inviato e ricevuto l'ack
    while(!done){
        // invio il buffer
        do{
            sendto(socket , msg , buf_len , 0 , (struct sockaddr*)&dest_addr , dest_len);
        }while( ret < 0);

        // ho inviato msg, aspetto di ricevere l'ack
        FD_ZERO(&wait);
        FD_SET(socket , &wait);

        //chiamo la select in attesa dell'ack
        ret = select(socket + 1 , &wait , NULL , NULL , NULL);

        // quando esco dalla select è sicuramente la socket che è accessibile in lettura
        
        // ricevo ack
        ret = recvfrom(socket , &received ,RECEIVED_LEN ,  0 , (struct sockaddr*)&tmp_addr , &tmp_len);

        // controllo di aver ricevuto l'ack effettivamente dal processo a cui l'avevo inviato, 
        // controllando la coppia indirizzo/porta e se cioò che ho ricevuto è effettivamente l'ack
        if(dest_addr.sin_port == tmp_addr.sin_port && dest_addr.sin_addr.s_addr == tmp_addr.sin_addr.s_addr && strcmp(received , expected_ack)){
            printf("Ho ricevuto l'ack %s da %d" , received , tmp_addr.sin_port);
            done = 1;
        }else{
            ptinf("ho ricevuto qualcosa da qualcun altro RIP");
        }

        //pulisco il set
        FD_CLEAR(socket , &wait);
    }


}


/**/

void send_ACK(int socket , char * ack_to_send , int dest_port){
    int ret , done;
    struct sockaddr_in dest_addr;
    socklen_t addr_len;


    setup_addr(&dest_addr , &addr_len , dest_port);

    // mando l'ack senza dare peso a cosa potrei ricevere indietro, tanto se il peer non ha ricevuto l'ack rimanda il msg
    do{
        ret = sendto(socket , ack_to_send , ACK_LEN + 1 , 0 , (struct sockaddr *)&dest_addr , &addr_len);
    }while(ret < 0 );
}


int recv_pkt(int socket , char * buf , int buf_len){
    int ret;
    struct sockaddr_in * sender_addr;
    socklen_t sender_addr_len;

    sender_addr_len = sizeof(sender_addr);

    recvfrom(socket , buf , buf_len , 0 , (struct sockaddr *)&sender_addr , &sender_addr_len);

    return ntohs(sender_addr.sin_port);
}