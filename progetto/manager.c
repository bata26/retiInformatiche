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
#include "./util/util_manager.h"

// porta che identifica il peer
int my_port;

//socket e strutture relative all'ascolto
int listen_socket;
struct sockaddr_in listen_addr;
socklen_t listen_addr_len;

//buffer per tutte le richieste
char manager_buffer[STANDARD_LEN];
// buffer per i comandi dai peer
char peer_buffer[STANDARD_LEN];
// buffer per le richieste del server
char server_buffer[STANDARD_LEN];
int buf_len = STANDARD_LEN;

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



// struttura oer la gestione dei dati
struct datiSalvati dati_giornalieri[DATA_LEN];

// mutex per le get
int mutex_flag; // 1 quando qualcuno sta gestendo una get


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


    
    while(1){

        // la select sposta da read_fds, in questo modo il set master
        // non viene modificato
        read_fds = master; 

        // ogni 5 minuti eseguo operazioni di controllo per l'orario, altrimenti rimarrei fermo nella select finche non arrivano messaggi
        timeout.tv_sec = 300;
        timeout.tv_usec = 0;
        
        

        select(fdmax , &read_fds , NULL , NULL , &timeout);

        // richieste sul socket
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
                num_peer++;
                printf("Ho aggiunto il peer %d alla lista dei peer\n" , new_port);
            }

            // il server avverte sulla disconnessione di uno dei peer
            // ogni volta che un peer si disconnette, comunica al manager i dati che ha raccolto
            // così facendo quando alle 18 si chiudono i register, i peer ancora connessi hanno
            // tutti i dati
            else if(strcmp(request_received , "REMV_LST") == 0 && sender_port == server_port){
                int port_to_remove , tamponi , casi;

                strcpy(server_buffer , manager_buffer);

                printf("Ricevuto dal DS il numero di porta di un peer che si e' disconnesso\n");

                send_ACK(listen_socket , "REMV_ACK" , server_port);

                sscanf(server_buffer , "%s %d" , request_received , &port_to_remove);

                removePeer(port_to_remove , peer);
                num_peer--;

                printf("Rimosso il peer %d alla lista dei peer" , port_to_remove);

                
                memset(manager_buffer , 0 , STANDARD_LEN);
                memset(peer_buffer , 0 , STANDARD_LEN);

                printf("Aspetto dal peer %d i dati del giorno..\n" , port_to_remove);

                recv_pkt(listen_socket , peer_buffer , buf_len , port_to_remove , "TDAY_AGG" , "MDAY_ACK");

                printf("Ricevuti i dati giornalieri del peer %d" ,port_to_remove);

                sscanf(peer_buffer , "%s %d %d" , command , &tamponi , &casi);

                // agigungo i dati appena raccolti
                dati_giornalieri[TAMPONE_IND].value += tamponi;
                dati_giornalieri[CASO_IND].value += casi;
                printf("Dati aggiunti:\nTamponi:%d\nCasi:%d\n" , dati_giornalieri[TAMPONE_IND].value , dati_giornalieri[CASO_IND].value);


            }  

            // il DS esegue una esc, il manager allora chiude tutti i register
            else if(strcmp(request_received , "DS_LEAVE") == 0){
                send_ACK(listen_socket , "DS_LEACK" , server_port);
                printf("Il DS si sta disconnettendo, chiudo i register..\n");
                closeRegister();
                printf("Register chiusi\n");
            }

            // il peer richiede il numero di peer attualmente connessi
            else if(strcmp(request_received , "TOT_PEER") == 0){

                send_ACK(listen_socket , "PEER_ACK" , sender_port);
                printf("Il peer %d richiede il numero di peer connessi\n" , sender_port);
                
               
                // preparo il buffer
                memset(peer_buffer , 0 , STANDARD_LEN);
                buf_len = sprintf(peer_buffer , "%s %d" , "PEER_LST" , num_peer);
                send_pkt(listen_socket , peer_buffer , buf_len , sender_port , "PLST_ACK");
                printf("numero di peer inviato e ricevuto ACK\n");

            }

            // il peer per eseguire una get richiede una mutex sulla get
            else if(strcmp(request_received , "MUTX_GET") == 0){
                int buf_len;

                send_ACK(listen_socket , "MUTX_ACK" , sender_port);
                // mando al peer iol valore di mutex, quindi se invio 1 il peer non puo ottenere il mutex

                buf_len = sprintf(peer_buffer , "%s %d" , "MUTX_VAL" , mutex_flag);
                
                send_pkt(listen_socket , peer_buffer , buf_len , sender_port , "MUTX_ACQ");

                if(!mutex_flag){
                    mutex_flag = 1; 
                    // avverto il server che è in esecuzione una get
                    // così che nessun peer si possa disconnettere durante l'esecuzione
                    // della get
                    send_pkt(listen_socket , "MUTX_LCK" , HEADER_LEN , server_port , "MTX_LACK");
                }

            }

            // il peer lascia la mutua esclusione
            else if(strcmp(request_received , "MUTX_LEA") == 0){
                send_ACK(listen_socket , "MTX_LEAK" , sender_port);
                
                if(mutex_flag){
                    mutex_flag = 0;
                    //avverto il server che la get è terminata
                    send_pkt(listen_socket , "MTX_ULCK" , HEADER_LEN , server_port , "MTX_LUCK");
                } 
            }

        }

        // comandi da stdin utili per debug
        else if(FD_ISSET( 0 , &read_fds)){
            memset(manager_buffer ,  0 , MAX_STDIN_LEN);

            fgets(manager_buffer , MAX_STDIN_LEN , stdin);
            sscanf(manager_buffer , "%s" , command);

            // blocco comandi da stdin per i peer
            if(strcmp(command , "close") == 0){
                int i;
                for(i = 0 ; i < NUM_PEER ; i++){
                    if(peer[i] == 0) continue;

                    send_pkt(listen_socket , "BLCK_ADD" , HEADER_LEN , peer[i] , "BLCK_ACK");
                }
            }
            
            // chiudo i register
            else if(strcmp(command , "closereg") == 0){
                closeRegister();
            }
            //printf("DATI RACCOLI:\nTAMPONI: %d\nCASI:%d\n" , dati_giornalieri[TAMPONE_IND].value , dati_giornalieri[CASO_IND].value);
        }

        //GESTIONE DEL TEMPO

        
        time(&t);
        timeinfo = localtime(&t);


        current_day = timeinfo->tm_mday;
        current_min = timeinfo->tm_min;
        current_hour = timeinfo->tm_hour;

        
        // se sono le 17 e i minuti sono compresi tra le 51 e 55 
        if(current_hour == 17){
            if(current_min >= 56){
                printf("Avvio chiusura dei register..\n");
                closeRegister();
                printf("Chiusura terminata, dati salvati\n");
            }else if(current_min >= 51 && current_min <= 55){
                int i;

                printf("Blocco i peer cosi da non accettare dati finche non chiudo i register\n");
                for(i = 0 ; i < NUM_PEER ; i++){
                    if(peer[i] == 0) continue;

                    send_pkt(listen_socket , "BLCK_ADD" , HEADER_LEN , peer[i] , "BLCK_ACK");
                }
            }
        }

        



    }

    return 0;
}


