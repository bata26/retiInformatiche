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


// setup sockaddr_in
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
    setup_addr(sockaddr , len , port);
    

    ret = bind(sd, (struct sockaddr*)sockaddr, (*len));

    if(ret < 0){
        perror("Errore in fase di bind: \n");
        exit(0);

    }


    return sd;

}



// manda il messaggio msg di dimension buf_len sul socket sd alla porta port_dest , si mette in attesa di expected_ack,
// controllando che l'ack sia arrivato dalla porta destinataria
void send_pkt(int sd , char* msg , int buf_len , int port_dest , char * expected_ack){
    int ret;
    int send;

    struct sockaddr_in dest_addr;
    socklen_t dest_len;

    struct sockaddr_in tmp_addr;
    socklen_t tmp_len;

    fd_set wait;
    char received[RECEIVED_LEN];
    struct timeval interval;

    // setup del dest_addr
    setup_addr(&dest_addr , &dest_len , port_dest);

    send = 0;
    ret = 0;
    tmp_len = sizeof(tmp_addr);


    // finchè non ho inviato e ricevuto l'ack
    while(!send){        
        do{
            
            ret = sendto(sd , msg , buf_len , 0 , (struct sockaddr*)&dest_addr , dest_len);
        }while(ret<0);

        

        interval.tv_sec = 1;
        interval.tv_usec = 5000;

        //setup interval, se richiesta di dati aspetto di piu
        if(strcmp(msg , "REQ_ENTR") == 0 || strcmp(msg , "ENTR_DAT") == 0){
            interval.tv_sec = 2;
            interval.tv_usec = 5000;
        }

        // ho inviato msg, aspetto di ricevere l'ack
        FD_ZERO(&wait);
        FD_SET(sd , &wait);

        //chiamo la select in attesa dell'ack
        ret = select(sd + 1 , &wait , NULL , NULL , &interval);

        // quando esco dalla select è sicuramente la socket che è accessibile in lettura
        if(FD_ISSET(sd , &wait)){

            // ricevo ack
            ret = recvfrom(sd , received ,RECEIVED_LEN ,  0 , (struct sockaddr*)&tmp_addr , &tmp_len);

            // controllo se entro un loop di richieste del tipo 
            // int sd , char* msg , int buf_len , int port_dest , char * expected_ack
            if(strcmp(msg , "REQ_ENTR") == 0 && dest_addr.sin_port == tmp_addr.sin_port && dest_addr.sin_addr.s_addr == tmp_addr.sin_addr.s_addr && strcmp(msg , received) == 0){
                printf("Rischio di entrare in un loop di richieste, provo a uscire e a gestire la richiesta\n");
                send = 1;
            }

            
            // controllo di aver ricevuto l'ack effettivamente dal processo a cui l'avevo inviato, 
            // controllando la coppia indirizzo/porta e se cioò che ho ricevuto è effettivamente l'ack
            if(dest_addr.sin_port == tmp_addr.sin_port && dest_addr.sin_addr.s_addr == tmp_addr.sin_addr.s_addr && strcmp(received , expected_ack) == 0){
                
                send = 1;
            }else{
                printf("Non ho ricevuto l'ack atteso, reinvio..\n");
            }
        }

        //pulisco il set
        FD_CLR(sd , &wait);

    }

}


/*
Invio l'ack ack_to_send al processo dest_port
*/

void send_ACK(int socket , char * ack_to_send , int dest_port){
    int ret;
    struct sockaddr_in dest_addr;
    socklen_t addr_len;


    setup_addr(&dest_addr , &addr_len , dest_port);
    ret = 0;

    // mando l'ack senza dare peso a cosa potrei ricevere indietro, tanto se il peer non ha ricevuto l'ack rimanda il msg
    do{
        ret = sendto(socket , ack_to_send , ACK_LEN , 0 , (struct sockaddr *)&dest_addr , addr_len);
    }while(ret < 0 );

}

/*
Riceve un pacchetto sul socket e ritorna la porta del sender
*/
int recv_send_pkt(int socket , char * buf , int buf_len){
    int ret;
    struct sockaddr_in sender_addr;
    socklen_t sender_addr_len;

    ret = 0;
    sender_addr_len = sizeof(sender_addr);

    ret = recvfrom(socket , buf , buf_len , 0 , (struct sockaddr*)&sender_addr , &sender_addr_len);

    if(ret < 0){
        perror("Errore nella recv from -> ");
    }

    return ntohs(sender_addr.sin_port);
}

/*
Mi metto in attesa del pacchetto expected_header da sender_port, se arriva rispondo con l'ack ack_to_send
*/
int recv_pkt(int socket , char* buffer , int buf_len , int sender_port , char* expected_header , char * ack_to_send){
    struct sockaddr_in sender_addr;
    socklen_t sender_addr_len;
    int received;
    fd_set read_fds;
    char header_buf[HEADER_LEN];

    received = 0;


    while(!received){
        FD_ZERO(&read_fds);
        FD_SET(socket , &read_fds);

        select(socket+1 , &read_fds , NULL , NULL , NULL);

        // arriva qualcosa dal socket
        if(FD_ISSET(socket  , &read_fds)){
            recvfrom(socket , buffer , buf_len , 0 , (struct sockaddr*)&sender_addr , &sender_addr_len);

            sscanf(buffer , "%s" , header_buf);


            /*
            Nel caso di flooding, il flooder non aspetta pacchetti da una porta specifica, se il sender è ALL_PEER il controllo si sposta solo sull'header
            */
            if( ( (sender_port == ALL_PEER) || (ntohs(sender_addr.sin_port) == sender_port) ) && strcmp(expected_header , header_buf) == 0){
                received = 1;
            }else{
                printf("Arrivato un messaggio diverso da quello che aspettavo. Mi rimetto in attesa\n");
            }
        }
    }

    send_ACK(socket , ack_to_send , ntohs(sender_addr.sin_port));

    return ntohs(sender_addr.sin_port);


} 