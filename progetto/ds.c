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

// strutture per la gestione dei peer
int num_peer;
int peer[NUM_PEER];
int neighbors[NUM_PEER][NUM_NEIGHBORS];

int i , j;

int main(int argc , char** argv){

    printf("avvio il server...\n");


    //inizializzo i set
    FD_ZERO(&master);
    FD_ZERO(&read_fds);


    //ricavo il numero di porta
    my_port = atoi(argv[1]);
    //my_port = 4242;

    //creo il socket di ascolto
    listen_socket = create_listener_socket(&listen_addr , &listen_addr_len , my_port);
    printf("socket di ascolto creato --> %d\n" , listen_socket);

    //aggiungo il socket di ascolto e il stdin
    FD_SET(listen_socket , &master);
    FD_SET(0 , &master);
    fdmax = listen_socket + 1;

    //pulisco i neighbors
    setupNeighbors((int *)neighbors);

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
        }

        // richieste dai peer
        else if(FD_ISSET(listen_socket , &read_fds)){
            int sender_port;
            char request_received[HEADER_LEN];
            int ret;
            struct sockaddr_in sender_addr;
            socklen_t sender_addr_len;
            int i;

            sender_port = recv_pkt(listen_socket , request_received , HEADER_LEN);

            printf("Ho ricevuto dal client %d --> %s\n" , sender_port , request_received);           

            // richiesta di connessione
            if(strcmp(request_received , "CONN_REQ") == 0){
                int neighbors_current_peer[NUM_NEIGHBORS]; 
                char buffer[MAX_LIST_LEN];
                int i , updated , index , buf_len , j;

                num_peer++;

                // aggiungo il peer
                addPeer(sender_port , peer);

                // se tutto va bene
                send_ACK(listen_socket , "CONN_ACK" , sender_port);
                printf("ACK inviato\n");

                // stampo tutti i peer

                printf("LISTA DEI PEER:\n");
                for(i = 0 ; i < NUM_PEER ; i++){
                    printf("%d) peer-> %d\n" , i , peer[i]);
                }
                printf("---------------------------------\n\n");

                // gestione neighbor:
                // 
                // 1) il ds crea un pacchetto formato da: "NGH_LIST" port1 port2
                // 2) il client manda un "ACK_LIST"
                
                // ogni volta che aggiungo un peer controllo se i vicini sono cambiati o no
                for(i = 0 ; i < NUM_PEER ; i++){
                    printf("cerco di aggiornare il peer %d" , peer[i]);

                    if(peer[i] == 0) continue;

                    index = getPeerIndex(sender_port);

                    getNeighbors(peer[i] , neighbors_current_peer , peer);

                    printf("Sto esaminando il peer %d e i vicini che ho trovato sono : %d , %d\n" , peer[i] , neighbors_current_peer[0] , neighbors_current_peer[1]);
                    updated = checkIfUpdated(neighbors[i] , neighbors_current_peer);

                    // se aggiornato, quindi un tipo di messaggio NGB_UPD
                    //if(updated){
                    //}
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

                if(neighbors[index][0] == -1 && neighbors[index][1] == -1)
                    buf_len = sprintf(buffer , "%s" , "NBR_LIST");
                else if(neighbors[index][1] == -1)
                    buf_len = sprintf(buffer , "%s %d" , "NBR_LIST" , neighbors[index][0]);
                else   
                    buf_len = sprintf(buffer , "%s %d %d" , "NBR_LIST" , neighbors[index][0] , neighbors[index][1]);
                
                printf("Sto per inviare al peer il buffer:\n%s\n" , buffer);
                send_pkt(listen_socket , buffer , buf_len , sender_port , "LIST_ACK");

                FD_CLR(listen_socket , &read_fds);
            }

            // CONN_STP
            if(strcmp(request_received , "CONN_STP") == 0){
                printf("Il peer %d ha richiesto una disconnessione.. \n");

                send_ACK(listen_socket , "STOP_ACK" , sender_port);

                peer[getPeerIndex(sender_port)] = 0;

                FD_CLR(listen_socket , &read_fds);

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
