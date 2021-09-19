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
char manager_buffer[MAX_STDIN_LEN];

//set per gestire socket e stdin
fd_set master;
fd_set read_fds;
int fdmax;

// porta che identifica il manager
int server_port;

// strutture per la gestione dei peer
int num_peer;
int peer[NUM_PEER];
int neighbors[NUM_PEER][NUM_NEIGHBORS];

int i , j;

int main(int argc , char** argv){

    printf("avvio il manager...\n");

    //inizializzo i set
    FD_ZERO(&master);
    FD_ZERO(&read_fds);


    //ricavo il numero di porta
    my_port = atoi(argv[1]);
    server_port = atoi(argv[2]); 
    //my_port = 4242;

    //creo il socket di ascolto
    listen_socket = create_listener_socket(&listen_addr , &listen_addr_len , my_port);
    //printf("socket di ascolto creato --> %d\n" , listen_socket);

    //aggiungo il socket di ascolto e il stdin
    FD_SET(listen_socket , &master);
    FD_SET(0 , &master);
    fdmax = listen_socket + 1;


    printf("Inoltro al DS il pacchetto MNG_CONN\n");
    // il manager come prima cosa cerca di connettersi al DS
    send_pkt(listen_socket, "MNG_CONN" , HEADER_LEN , server_port , "MNG_ACK");

    
    while(1){

        // la select sposta da read_fds, in questo modo il set master
        // non viene modificato
        read_fds = master; 

        select(fdmax , &read_fds , NULL , NULL , NULL);

        // se è arrivato qualcosa da stdin
        if(FD_ISSET( 0 , &read_fds)){

            
        }

        // richieste dai peer
        else if(FD_ISSET(listen_socket , &read_fds)){
            int sender_port;
            char request_received[HEADER_LEN];

            sender_port = recv_pkt(listen_socket , manager_buffer , MAX_STDIN_LEN);
            sscanf(manager_buffer , "%s" , request_received);
            printf("Ho ricevuto da %d il buffer %s\n" , sender_port , manager_buffer);

            // il server manda un nuovo peer da aggiungere alla lista
            if((strcmp(request_received , "UPDT_LST") == 0) && sender_port == server_port){
                int new_port;

                send_ACK(listen_socket , "MUPD_ACK" , server_port);

                sscanf(manager_buffer , "%s %d" , request_received , &new_port);

                addPeer(new_port , peer);

                printf("Ho aggiunto il peer %d alla lista dei peer\n" , new_port);

                FD_CLR(listen_socket , &read_fds);
            }

            else if(strcmp(request_received , "REMV_LST") == 0 && sender_port == server_port){
                int port_to_remove , i;

                send_ACK(listen_socket , "REMV_ACK" , server_port);

                sscanf(manager_buffer , "%s %d" , request_received , &port_to_remove);

                removePeer(port_to_remove , peer);
                num_peer--;

                printf("Ho rimosso il peer %d alla lista dei peer" , port_to_remove);

                

                printf("STAMPO I PEER:\n");

                for(i = 0 ; i< NUM_PEER ; i++){
                    printf("%d) %d\n" , i , peer[i]);
                }

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
