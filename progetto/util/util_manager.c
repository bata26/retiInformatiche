
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "costanti.h"
#include "data.h"
#include "connection.h"


void closeRegister(struct datiSalvati manager_data[DATA_LEN] , int* peer , int listen_socket){
    int i , tamponi , casi , buf_len;
    char manager_buffer[MAX_STDIN_LEN];
    buf_len = MAX_STDIN_LEN;
    char command[MAX_COMMAND_LEN];

    for(i = 0 ; i <  NUM_PEER ; i++){
        if(peer[i] == 0) continue;


        send_pkt(listen_socket , "TDAY_CLS" , HEADER_LEN , peer[i] , "TDAY_ACK");
        recv_pkt(listen_socket , manager_buffer , buf_len , peer[i] , "TDAY_AGG" , "MDAY_ACK");

        sscanf(manager_buffer , "%s %d %d" , command , &tamponi , &casi);
        
        printf("Buffer ricevuto: %s\n" , manager_buffer);
        manager_data[TAMPONE_IND].value += tamponi;
        manager_data[CASO_IND].value += casi;


    }

    // ho aggregato tutti i dati, li rinvio ai peer

    for(i = 0 ; i <  NUM_PEER ; i++){
        if(peer[i] == 0) continue;

        memset(manager_buffer ,  0 , MAX_STDIN_LEN);

        buf_len = sprintf(manager_buffer , "%s %d %d" , "DAY_DATA" , manager_data[TAMPONE_IND].value , manager_data[CASO_IND].value);

        send_pkt(listen_socket , manager_buffer , buf_len , peer[i] , "DATA_ACK");
    }
}