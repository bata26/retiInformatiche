#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>


//costanti
#include "./util/costanti.h" 

// connection utils
#include "./util/connection.h"
// ds utils
#include "./util/util_ds.h"

// porta che identifica il peer
int my_port;

//socket e strutture relative all'ascolto
int listen_socket;
struct sockaddr_in listen_addr;
socklen_t listen_addr_len;

//buffer per i comandi da stdin
char stdin_buffer[MAX_STDIN_LEN];

//set per gestire socket e stdin
fd_set master;
fd_set read_fds;
int fdmax;

// porta che identifica il manager
int manager_port;
int manager_connected;

// strutture per la gestione dei peer
int num_peer;
int peer[NUM_PEER];
int neighbors[NUM_PEER][NUM_NEIGHBORS];

int i , j;


// buffer e buf_len
char buffer[MAX_LIST_LEN];
int buf_len;

int main(int argc , char** argv){

    printf("avvio il server...\n");
    stampaComandi();


    //inizializzo i set
    FD_ZERO(&master);
    FD_ZERO(&read_fds);


    //ricavo il numero di porta
    my_port = atoi(argv[1]);
    //my_port = 4242;

    //creo il socket di ascolto
    listen_socket = create_listener_socket(&listen_addr , &listen_addr_len , my_port);
    //printf("socket di ascolto creato --> %d\n" , listen_socket);

    //aggiungo il socket di ascolto e il stdin
    FD_SET(listen_socket , &master);
    FD_SET(0 , &master);
    fdmax = listen_socket + 1;

    //pulisco i neighbors
    setupNeighbors(neighbors);

    printf("\n\n");
    for ( i = 0; i < NUM_PEER; i++)
    {
        
        printf("STO ESAMINANDO IL PEER %d\n" , peer[i]);

        for(j = 0 ; j < NUM_NEIGHBORS ; j++){
            printf("vicino %d ---> %d\n" , j , neighbors[i][j]);
        }
        printf("\n");
    }

    printf("Mi metto in attesa\n");
    while(1){

        // la select sposta da read_fds, in questo modo il set master
        // non viene modificato
        read_fds = master; 

        select(fdmax , &read_fds , NULL , NULL , NULL);

        // se è arrivato qualcosa da stdin
        if(FD_ISSET( 0 , &read_fds)){

            char command[MAX_COMMAND_LEN];

            fgets(stdin_buffer , MAX_COMMAND_LEN , stdin);
            sscanf(stdin_buffer, "%s", command);
            //command[MAX_COMMAND_LEN] = '\0';

            printf("ricevuto il comando: %s\n" , command);

            if(strcmp(command , "help") == 0){
                stampaDettagli();
                FD_CLR(0 , &read_fds);
            }

            else if(strcmp(command , "showpeers") == 0){
                printf("Prima della stampa\n");
                stampaPeer(peer);
                printf("dopo la stampa\n");

                FD_CLR(0 , &read_fds);
            }

            else if(strcmp(command , "showneighbor") == 0){
                int peer_to_show = 0;
                sscanf(stdin_buffer , "%s %d" , command ,&peer_to_show);

                stampaNeighbors(neighbors , peer_to_show , peer);
                FD_CLR(0 , &read_fds);
            }

            else if(strcmp(command , "esc") == 0){
                FD_CLR(0 , &read_fds);
            }

            else{
                printf("comando non riconosciuto, prego riprovare..\n");
                FD_CLR(0 , &read_fds);
            }

            
            memset(command , 0 , MAX_COMMAND_LEN);
        }

        // richieste dai peer
        else if(FD_ISSET(listen_socket , &read_fds)){
            int sender_port;
            char request_received[HEADER_LEN];

            sender_port = recv_pkt(listen_socket , request_received , HEADER_LEN);

            printf("Ho ricevuto dal client %d --> %s\n" , sender_port , request_received);           

            // richiesta di connessione
            if(strcmp(request_received , "CONN_REQ") == 0){

                //int neighbors_current_peer[NUM_NEIGHBORS]; 
                
                int i ,index , j;
                int updated[NUM_PEER]; // contiene informazioni su quali peer vanno aggiornati

                num_peer++;
                index = getPeerIndex(sender_port);

                // aggiungo il peer
                addPeer(sender_port , peer);

                // se tutto va bene
                send_ACK(listen_socket , "CONN_ACK" , sender_port);
                printf("ACK inviato\n");


                for( i = 0 ; i< NUM_PEER ; i++){
                    updated[i] = 0;
                }

                updated[index] = 1;



                updateNeighbors(peer , neighbors , updated);

                for( i = 0 ; i< NUM_PEER ; i++){
                    printf("DOPO: i->%d , updated[i]->%d\n" , i , updated[i]);
                }


                for(i = 0 ; i <  NUM_PEER ; i++){
                    if(updated[i] == 0) continue;

                    printf("PRIMA: buffer--> %s , buf_len --> %d\n" , buffer , buf_len);

                    if(i == index){ // il peer che si e' appena connesso dovra' ricevere un pacchetto di LIST non di update
                        setupNeighborsBuffer(buffer , &buf_len , "NBR_LIST" , neighbors , i); 
                        send_pkt(listen_socket , buffer , buf_len , peer[i] , "LIST_ACK");  
                    }else{
                        setupNeighborsBuffer(buffer , &buf_len , "NBR_UPDT" , neighbors , i);   
                        send_pkt(listen_socket , buffer , buf_len , peer[i] , "UPDT_ACK");
                    }

                    printf("DOPO: buffer--> %s , buf_len --> %d\n" , buffer , buf_len);

                    


                }

                // STAMPO TUTTI I NEIGHBORS
                printf("\n\n");
                for ( i = 0; i < NUM_PEER; i++)
                {
                   
                    printf("STO ESAMINANDO IL PEER %d\n" , peer[i]);

                    for(j = 0 ; j < NUM_NEIGHBORS ; j++){
                        printf("vicino %d ---> %d\n" , j , neighbors[i][j]);
                    }
                    printf("\n");
                }
                
                printf("Sto per inviare al peer il buffer:\n%s\n" , buffer);

                // invio al peer la porta del manager

                buf_len = sprintf(buffer , "%s %d" , "MNG_PORT" , manager_port);
                send_pkt(listen_socket , buffer , buf_len , sender_port , "MNG_ACKP");

                printf("inviato al peer %d la porta del manager\n" , sender_port);



                // invio il nuovo peer al manager
                buf_len = sprintf(buffer , "%s %d" , "UPDT_LST" , sender_port);
                send_pkt(listen_socket , buffer , buf_len , manager_port , "MUPD_ACK");
                printf("Ho inviato al manager il nuovo peer\n");

                FD_CLR(listen_socket , &read_fds);
            }

            // CONN_STP
            else if(strcmp(request_received , "CONN_STP") == 0){
                int updated[NUM_PEER];
                int i , buf_len , index;
                char buffer[MAX_LIST_LEN];

                printf("Il peer %d ha richiesto una disconnessione.. \n" , sender_port);

                send_ACK(listen_socket , "STOP_ACK" , sender_port);

                // elimino il peer, devo aggiornare neighbors
                index = getPeerIndex(sender_port); 
                peer[index] = 0;


                // rimuovo il peer dalla struct dei neighbors
                cleanNeighbors(sender_port , neighbors);

                for( i = 0 ; i< NUM_PEER ; i++){
                    updated[i] = 0;
                }

                updateNeighbors(peer , neighbors , updated);

                for(i = 0 ; i < NUM_PEER ; i++){
                    if(updated[i] == 0) continue;

                    setupNeighborsBuffer(buffer , &buf_len , "NBR_UPDT" , neighbors , i);   
                    send_pkt(listen_socket , buffer , buf_len , peer[i] , "UPDT_ACK");


                }

                // MANAGER
                // invio il peer da rimuovere al manager
                buf_len = sprintf(buffer , "%s %d" , "REMV_LST" , sender_port);
                send_pkt(listen_socket , buffer , buf_len , manager_port , "REMV_ACK");
                printf("Ho inviato al manager il peer da rimuovere\n");

                //////

                FD_CLR(listen_socket , &read_fds);

            }       

            // richiesta di connessione del manager
            else if(strcmp(request_received , "MNG_CONN") == 0){
                manager_port = sender_port;
                send_ACK(listen_socket , "MNG_ACK" , manager_port);

                printf("Ricevuta la richiesta di connessione del manager!\n");
                manager_connected = 1;
            }    

        }
    }

    return 0;
}


/*
    Struttura dei comandi

    start DS_addr DS_port
    add type quantity
    get aggr(TOT , DIFF) type(TAMPO , NEW) period
*/
