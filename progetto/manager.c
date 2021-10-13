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

// data utils
#include "./util/data.h"

// manager utils
//#include "./util/util_manager.h"

// porta che identifica il peer
int my_port;

//socket e strutture relative all'ascolto
int listen_socket;
struct sockaddr_in listen_addr;
socklen_t listen_addr_len;

//buffer per i comandi da stdin
char manager_buffer[MAX_STDIN_LEN];
int buf_len = MAX_STDIN_LEN;

// buffer per i comandi
char command[MAX_COMMAND_LEN];

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

// strutture per la gestione dell'orario
time_t t;
struct tm * timeinfo;
struct timeval timeout;
int current_hour;
int current_min;
int current_day;

// flag che mi dice se il DS ha chiuso prima di me
int server_flag;
int day_close; // giorno in cui il server ha chiuso


// struttura oer la gestione dei dati
struct datiSalvati dati_giornalieri[DATA_LEN];

int i , j;


void closeRegister();

int main(int argc , char** argv){

    printf("avvio il manager...\n");

    //inizializzo i set
    FD_ZERO(&master);
    FD_ZERO(&read_fds);


    //ricavo il numero di porta
    my_port = atoi(argv[1]);
    server_port = atoi(argv[2]); 
    

    // setup data
    setupData(dati_giornalieri);

    //creo il socket di ascolto
    listen_socket = create_listener_socket(&listen_addr , &listen_addr_len , my_port);
    //printf("socket di ascolto creato --> %d\n" , listen_socket);

    //aggiungo il socket di ascolto e il stdin
    FD_SET(listen_socket , &master);
    FD_SET(0 , &master);
    fdmax = listen_socket + 1;


    printf("Inoltro al DS la richiesta di connessione\n");
    // il manager come prima cosa cerca di connettersi al DS
    send_pkt(listen_socket, "MNG_CONN" , HEADER_LEN , server_port , "MNG_ACK");
    printf("Connessione con il DS avvenuta con successo\n");

    //gestione flag
    server_flag = 0;

    
    while(1){

        // la select sposta da read_fds, in questo modo il set master
        // non viene modificato
        read_fds = master; 

        // ogni 5 minuti eseguo operazioni di controllo per l'orario, altrimenti rimarrei fermo nella select finche non arrivano messaggi
        timeout.tv_sec = 300;
        timeout.tv_usec = 0;
        
        

        select(fdmax , &read_fds , NULL , NULL , &timeout);

        // richieste dai peer
        if(FD_ISSET(listen_socket , &read_fds)){
            int sender_port;
            char request_received[HEADER_LEN];

            sender_port = recv_send_pkt(listen_socket , manager_buffer , MAX_STDIN_LEN);
            sscanf(manager_buffer , "%s" , request_received);


            // il server manda un nuovo peer da aggiungere alla lista
            if((strcmp(request_received , "UPDT_LST") == 0) && sender_port == server_port){
                int new_port;

                printf("Ricevuto dal DS il numero di porta di un nuovo peer che si e' connesso\n");

                send_ACK(listen_socket , "MUPD_ACK" , server_port);

                sscanf(manager_buffer , "%s %d" , request_received , &new_port);
                addPeer(new_port , peer);

                printf("Ho aggiunto il peer %d alla lista dei peer\n" , new_port);
            }

            else if(strcmp(request_received , "REMV_LST") == 0 && sender_port == server_port){
                int port_to_remove , tamponi , casi;

                printf("Ricevuto dal DS il nuemro di porta di un peer che si e' disconnesso\n");

                send_ACK(listen_socket , "REMV_ACK" , server_port);

                sscanf(manager_buffer , "%s %d" , request_received , &port_to_remove);

                removePeer(port_to_remove , peer);
                num_peer--;

                printf("Rimosso il peer %d alla lista dei peer" , port_to_remove);

                memset(manager_buffer , 0 , MAX_STDIN_LEN );

                printf("Aspetto dal peer %d i dati del giorno..\n" , port_to_remove);

                recv_pkt(listen_socket , manager_buffer , buf_len , port_to_remove , "TDAY_AGG" , "MDAY_ACK");

                printf("Ricevuti i dati giornalieri del peer %d" ,port_to_remove);

                sscanf(manager_buffer , "%s %d %d" , command , &tamponi , &casi);

                dati_giornalieri[TAMPONE_IND].value += tamponi;
                dati_giornalieri[CASO_IND].value += casi;
                printf("Dati aggiunti:\nTamponi:%d\nCasi:%d\n" , dati_giornalieri[TAMPONE_IND].value , dati_giornalieri[CASO_IND].value);


            }  

            // il DS chiude
            else if(strcmp(request_received , "DS_LEAVE") == 0){
                send_ACK(listen_socket , "DS_LEACK" , server_port);
                printf("Il DS si sta disconnettendo, chiudo i register..\n");
                closeRegister();
                printf("Register chiusi\n");
            }

        }

        // stdin
        else if(FD_ISSET( 0 , &read_fds)){
            memset(manager_buffer ,  0 , MAX_STDIN_LEN);

            fgets(manager_buffer , MAX_STDIN_LEN , stdin);
            sscanf(manager_buffer , "%s" , command);

            if(strcmp(command , "close") == 0){
                closeRegister();
            }

            printf("DATI RACCOLI:\nTAMPONI: %d\nCASI:%d\n" , dati_giornalieri[TAMPONE_IND].value , dati_giornalieri[CASO_IND].value);
        }

        //GESTIONE DEL TEMPO

        
        time(&t);
        timeinfo = localtime(&t);

        //printf("Current time --> %s\n" , ctime(&t));
        //printf("Time -> %d\n" , timeinfo->tm_mon);

        current_day = timeinfo->tm_mday;
        current_min = timeinfo->tm_min;
        current_hour = timeinfo->tm_hour;

        //printf("Current_day -> %d \nCurrent_hour->%d\nCurrent_min->%d\n" , current_day , current_hour , current_min);

        if(current_hour == 17 && current_min > 56){
            printf("Avvio chiusura dei register..\n");
            closeRegister();
            printf("Chiusura terminata, dati salvati\n");
        }

        



    }

    return 0;
}




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