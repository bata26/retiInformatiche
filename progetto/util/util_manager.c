
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


#include "connection.h"
#include "costanti.h"
#include "data.h"


extern int listen_socket;
extern char manager_buffer[STANDARD_LEN];
extern int buf_len;
extern char command[MAX_COMMAND_LEN];
extern struct datiSalvati dati_giornalieri[DATA_LEN];
extern int peer[NUM_PEER];


/*
Mando a ogni peer connesso un pacchetto di TDAY_CLS per chiedere i dati raccolti del giorno,
dopodichè invio un pacchetto di TDAY_AGG con i dati aggregati di tutti i peer.
*/
void closeRegister(){
    int i , tamponi , casi;

    for(i = 0 ; i <  NUM_PEER ; i++){
        if(peer[i] == 0) continue;

        memset(manager_buffer , 0 , MAX_STDIN_LEN);

        printf("Invio al peer %d la richiesta di chiusura del register\n" , peer[i]);

        send_pkt(listen_socket , "TDAY_CLS" , HEADER_LEN , peer[i] , "TDAY_ACK");
        recv_pkt(listen_socket , manager_buffer , buf_len , peer[i] , "TDAY_AGG" , "MDAY_ACK");

        printf("Ricevuti i dati giornalieri del peer %d" ,peer[i]);

        sscanf(manager_buffer , "%s %d %d" , command , &tamponi , &casi);
        
        dati_giornalieri[TAMPONE_IND].value += tamponi;
        dati_giornalieri[CASO_IND].value += casi;
    }

    printf("Aggregato tutti i dati ricevuti\nPreparo invio ai peer\n");

    // ho aggregato tutti i dati, li rinvio ai peer

    for(i = 0 ; i <  NUM_PEER ; i++){
        if(peer[i] == 0) continue;

        memset(manager_buffer ,  0 , MAX_STDIN_LEN);

        buf_len = sprintf(manager_buffer , "%s %d %d" , "DAY_DATA" , dati_giornalieri[TAMPONE_IND].value , dati_giornalieri[CASO_IND].value);
        printf("Invio al peer %d i dati aggregati del giorno\n" , peer[i]);
        send_pkt(listen_socket , manager_buffer , buf_len , peer[i] , "DATA_ACK");
        printf("Dati inviati\n");
    }

    printf("Aggiorno strutture dati per il nuovo giorno\n");

    dati_giornalieri[TAMPONE_IND].value = 0;
    dati_giornalieri[CASO_IND].value = 0;
}