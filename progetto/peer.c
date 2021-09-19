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

// util peer
#include "./util/util_peer.h"

// porta che identifica il peer
int my_port;

//socket e strutture relative all'ascolto
int listen_socket;
struct sockaddr_in listen_addr;
socklen_t listen_addr_len;

//buffer per i comandi da stdin
char stdin_buffer[MAX_STDIN_LEN];
// buffer per i comandi da server
char server_buffer[MAX_PKT_LEN];

//set per gestire socket e stdin
fd_set master;
fd_set read_fds;
int fdmax;

// porte che identificano i server
int server_port;
int manager_port;

// booleano che indica se e' avvenuta la connession al DS o ancora no
int connected;

// struttura per mantenere traccia dei vici
int neighbors[NUM_NEIGHBORS]; 

int main(int argc , char** argv){

    printf("avvio il client...\n");


    //inizializzo i set
    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    connected = 0;

    //ricavo il numero di porta
    my_port = atol(argv[1]);

    printf("atoi -- > %d" , atoi(argv[1]));

    //my_port = 5001;
    server_port = 4242;
    printf("La mia porta e' --> %d\n" , my_port);

    //creo il socket di ascolto
    listen_socket = create_listener_socket(&listen_addr , &listen_addr_len , my_port);
    printf("socket creato\n");

    printf("La porta relativa al mio socket %d e' %d\n" ,listen_socket ,  ntohs(listen_addr.sin_port));


    //aggiungo il socket di ascolto e il stdin
    FD_SET(listen_socket , &master);
    FD_SET(0 , &master);
    fdmax = listen_socket + 1;

    while(1){

        // la select sposta da read_fds, in questo modo il set master
        // non viene modificato
        read_fds = master; 

        select(fdmax , &read_fds , NULL , NULL , NULL);

        // se è arrivato qualcosa da stdin
        if(FD_ISSET( 0 , &read_fds)){

            char command[MAX_COMMAND_LEN];

            printf("comando arrivato\n");
            
            fgets(stdin_buffer , MAX_STDIN_LEN , stdin);
            sscanf(stdin_buffer, "%s", command);
            printf("ho ricevuto il comando %s\n" , command);

            // start
            if(strcmp(command , "start") == 0){

                if(connected){
                    printf("Already Connected!\n");
                    continue;
                }
                
                send_pkt(listen_socket , "CONN_REQ" , HEADER_LEN , server_port, "CONN_ACK");
                printf("Ricevuto ACK\n");

                connected = 1;

                FD_CLR(0 , &read_fds);
            }
            // stop
            else if(strcmp(command , "stop") == 0){

                if(!connected){
                    printf("Not connected yet..\n");
                    continue;
                }

                printf("Tentativo di disconnessione\n");
                send_pkt(listen_socket , "CONN_STP" , HEADER_LEN , server_port, "STOP_ACK");

                connected = 0;

                FD_CLR(0 , &read_fds);

            }
        }

        // richiesta dal server o da altri peer
        /*
            se viene dal socket in socket buffer metto tutto il pacchetto, poi ottengo il codice del pacchetto nel buffer msg_type

        */
        else if(FD_ISSET(listen_socket , &read_fds)){
            int sender_port;
            char msg_type[MAX_COD_LEN+1];

            memset(server_buffer , 0 , MAX_PKT_LEN);

            sender_port = recv_pkt(listen_socket , server_buffer, MAX_PKT_LEN);

            // ottengo il codice del pacchetto
            sscanf(server_buffer , "%s" , msg_type);
            msg_type[MAX_COD_LEN] = '\0';

            if(sender_port == server_port){

                if(strcmp(msg_type , "NBR_LIST") == 0){
                    
                    cleanNeighbors(neighbors);
                    printf("Dopo la cleanNeighbors --> %d , %d\n" , neighbors[0] , neighbors[1] );

                    printf("Ho ricevuto la richiesta di %s" , msg_type);

                    send_ACK(listen_socket , "LIST_ACK" , server_port);

                    sscanf(server_buffer , "%s %d %d" , msg_type , &neighbors[0] , &neighbors[1]);
                    printf("I miei vicini sono: \n1) %d \n2) %d\n" , neighbors[0] , neighbors[1]);
                }

                // NEIGHBOR UPDATE
                if(strcmp(msg_type , "NBR_UPDT") == 0){

                    printf("Server_burrer--> %s\n" , server_buffer);

                    cleanNeighbors(neighbors);
                    printf("Dopo la cleanNeighbors --> %d , %d\n" , neighbors[0] , neighbors[1] );

                    printf("Ho ricevuto la richiesta di %s" , msg_type);

                    send_ACK(listen_socket , "UPDT_ACK" , server_port);

                    sscanf(server_buffer , "%s %d %d" , msg_type , &neighbors[0] , &neighbors[1]);
                    printf("I miei vicini sono: \n1) %d \n2) %d\n" , neighbors[0] , neighbors[1]);
                }

                
            }

        }

    }
}


/*
    Struttura dei comandi

    start DS_addr DS_port
    add type quantity
    get aggr(TOT , DIFF) type(TAMPO , NEW) period
*/
