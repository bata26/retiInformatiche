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


            /*
            if(strcmp(command , CLIENT_START_COMMAND) == 0){
                printf("provo ad inviare la req conn");
                send_pkt(listen_socket , "CON_REQ" , HEADER_LEN , my_port, "CON_ACK");

                
                //    DEVO RICEVERE LE PORTE DEI VICINI
                
            } 
            */
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
            if(strcmp(request_received , "CONN_REQ\n")){
                int neighbors_current_peer[NUM_NEIGHBORS]; 
                char buffer[MAX_LIST_LEN];

                num_peer++;

                // aggiungo il peer
                addPeer(sender_port , peer);

                // se tutto va bene
                send_ACK(listen_socket , "CONN_ACK" , sender_port);
                printf("ACK inviato\n");

                // gestione neighbor:
                // 
                // 1) il ds crea un pacchetto formato da: "NGH_LIST" port1 port2
                // 2) il client manda un "ACK_LIST"

                void getNeighbors(sender_port , neighbors_current_peer , peer);

                printf("\n\nSi e' connesso il peer %d, i due peer sono:\n1) %d \n2) %d\n" , sender_port , neighbors_current_peer[0] , neighbors_current_peer[1]);


                // aggiorno la struct neighbors


                // se ho solo un peer mando un pacchetto di ALONE
                // se ho due peer devo andare a cercare l'unico peer
                // 
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
