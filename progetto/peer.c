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

// porte che identificano i server
int server_port;
int manager_port;

int connected;


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

            if(strcmp(command , "start") == 0){

                if(connected){
                    printf("Already Connected!\n");
                    continue;
                }
                
                printf("provo ad inviare la req conn\n");
                send_pkt(listen_socket , "CONN_REQ" , HEADER_LEN , server_port, "CONN_ACK");
                printf("Ricevuto ACK\n");

                connected = 1;

                
            }else{
                printf("else");
            }

            printf("fuori dalla cmp");
        }

    }
}


/*
    Struttura dei comandi

    start DS_addr DS_port
    add type quantity
    get aggr(TOT , DIFF) type(TAMPO , NEW) period
*/
