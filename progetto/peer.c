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

// util data
#include "./util/data.h"

// data
struct datiSalvati peer_data[DATA_LEN];


// porta che identifica il peer
int my_port;

//socket e strutture relative all'ascolto
int listen_socket;
struct sockaddr_in listen_addr;
socklen_t listen_addr_len;

//buffer per i comandi da stdin
char stdin_buffer[MAX_STDIN_LEN];
// buffer per i comandi da server
char server_buffer[STANDARD_LEN];
// buffer per i comandi da manager
char manager_buffer[STANDARD_LEN];
// buffer per altri peer
char peer_buffer[STANDARD_LEN];
// buffer per le richieste ricevute sul socket
char request_buffer[STANDARD_LEN];

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

// gestione del file
FILE * data_file;

// il manager gestisce le richieste di flooding, quando invia il numero di peer connessi al peer,
// gli invia anche un ID per la flooding request, cosi un peer sa se ha gia' partecipato o no
// e si evitano loop di request
int last_flood_request_id;
int served_flag;

// strutture per la gestione dei dati aggregati:
// - numero di risposte ricevute per una richiesta
// - numeri di porta ricevuti
// - dato del giorno corrente
// - dato del giorno precedente
int peer_received[NUM_PEER];
int num_response;
int today_aggr;
int yesterday_aggr;
int today_flag; // settata a 1 quando troviamo il def  
char util_aggr_type[AGGR_LEN]; // TOTALE VARIAZIONE
char util_type[AGGR_LEN]; //TAMPONCE , CASO

//DEBUG
char comando[MAX_COMMAND_LEN];

// flag per accettazione dati
int data_flag;
int mutex_flag;

int main(int argc , char** argv){

    //inizializzo i set
    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    connected = 0;

    //ricavo il numero di porta
    my_port = atol(argv[1]);
    

    //printf("atoi -- > %d" , atoi(argv[1]));

    //my_port = 5001;
    server_port = 4242;
    

    //creo il socket di ascolto
    listen_socket = create_listener_socket(&listen_addr , &listen_addr_len , my_port);

    //aggiungo il socket di ascolto e il stdin
    FD_SET(listen_socket , &master);
    FD_SET(0 , &master);
    fdmax = listen_socket + 1;

    stampaComandi(my_port);
    setupData(peer_data);

    last_flood_request_id = 0;

    data_flag = 1;

    while(1){

        // la select sposta da read_fds, in questo modo il set master
        // non viene modificato
        read_fds = master; 

        //if(strcmp(comando , "start") == 0) goto connetti;
        select(fdmax , &read_fds , NULL , NULL , NULL);

        // se è arrivato qualcosa da stdin
        if(FD_ISSET( 0 , &read_fds)){

            char command[MAX_COMMAND_LEN];
            
            fgets(stdin_buffer , MAX_STDIN_LEN , stdin);
            sscanf(stdin_buffer, "%s", command);
            printf("Ricevuto il comando %s\n" , command);

            if(!data_flag){
                printf("Impossibile eseguire operazioni fino alla chiusura dei register, riprovare piu' tardi\n");
                continue;
            }

            // start
            if(strcmp(command , "start") == 0){

                if(connected){
                    printf("Il peer e' gia' connesso\n");
                    continue;
                }

                printf("Invio al DS la richiesta di connessione\n");
                
                send_pkt(listen_socket , "CONN_REQ" , HEADER_LEN , server_port, "CONN_ACK");
                printf("Ricevuto ACK\n");

                connected = 1;

                printf("Connessione riuscita!\n");            

                FD_CLR(0 , &read_fds);
            }
            // stop
            else if(strcmp(command , "stop") == 0){
                int buf_len;

                if(!connected){
                    printf("Non sono ancora connesso, impossibile disconnettersi\n");
                    continue;
                }

                if(mutex_flag){
                    printf("Non posso disconnettermi in quanto c'e' una get in corso\n");
                    continue;
                }

                send_pkt(listen_socket , "CAN_I_LV" ,HEADER_LEN , server_port , "ACK_U_LV");
                memset(server_buffer ,0 , STANDARD_LEN);
                recv_pkt(listen_socket , server_buffer , STANDARD_LEN , server_port , "U_CAN_LV" , "ACK_I_LV");

                sscanf(server_buffer , "%s %d" , command , &mutex_flag);

                printf("Mutex_Flag-->%d\n" , mutex_flag);

                if(mutex_flag){
                    printf("Non posso disconnettermi al momento..\n");
                    continue;
                }

                printf("Invio al DS la richiesta di disconnessione\n");
                send_pkt(listen_socket , "CONN_STP" , HEADER_LEN , server_port, "STOP_ACK");
                printf("Disconnessione avvenuta con successo!\n");
                connected = 0;

                printf("Invio al manager i dati raccolti fino ad ora\n");
                buf_len = sprintf(manager_buffer , "%s %d %d" , "TDAY_AGG" , peer_data[TAMPONE_IND].value , peer_data[CASO_IND].value);
                send_pkt(listen_socket , manager_buffer , buf_len , manager_port , "MDAY_ACK");
                printf("Dati inviati.\n");

                writeOnFile(peer_data , my_port , 'N');


                FD_CLR(0 , &read_fds);

            }

            // add
            else if(strcmp(command , "add") == 0){                
                enum dataType type;
                int value;
                char tipo[MAX_COMMAND_LEN];

                if(!connected){
                    printf("Impossibile inserire dati finche' il peer non e' connesso\n");
                    continue;
                }


                printf("Inserisco i nuovi dati..\n");

                sscanf(stdin_buffer , "%s %s %d" , command , tipo , &value);

                if(strcmp(tipo , "CASO") == 0){
                    type = CASO;
                }else if(strcmp(tipo , "TAMPONE") == 0){
                    type = TAMPONE;
                }else{
                    printf("Inserire un un tipo di dato TAMPONE o CASO..\n");
                    break;
                }

                peer_data[type].value += value;

                printf("Dati inseriti, Resoconto della giornata:\n");
                printf("Tamponi effettuati: %d\n"  , peer_data[TAMPONE].value);
                printf("Nuovi casi registrati: %d\n"  , peer_data[CASO].value);
                FD_CLR(0 , &read_fds);
            }

            // get
            else if(strcmp(command , "get") == 0){
                char tipo_aggr[AGGR_LEN];
                char tipo[MAX_COMMAND_LEN];
                char data_iniziale[DATE_LEN];
                char data_finale[DATE_LEN];
                int ret;
                int tot;

                if(!connected){
                    printf("Impossibile richiedere dati finche' il peer non e' connesso\n");
                    continue;
                }


                // come prima cosa richiedo al manager il mutex sul canale per le get
                printf("Richiedo la mutex al manager..\n");
                send_pkt(listen_socket , "MUTX_GET" , HEADER_LEN  , manager_port , "MUTX_ACK");
                recv_pkt(listen_socket , manager_buffer , STANDARD_LEN , manager_port , "MUTX_VAL" , "MUTX_ACQ");

                sscanf(manager_buffer , "%s %d" , command , &mutex_flag );

                if(mutex_flag == 1){
                    printf("Impossibile ottenere il mutex\n");
                    continue;
                }

                printf("Ottenuto il mutex per la get\n");
                //if(my_port == 5001) sleep(5);


                ret = sscanf(stdin_buffer , "%s %s %s %s %s" , command , tipo_aggr , tipo , data_iniziale , data_finale);

                //printf("\nAppena inseriti:\nTipo_aggr->%s\nTipo->%s\nDataIniziale->%s\nDataFinale->%s\n" , tipo_aggr , tipo , data_iniziale , data_finale);

                /*
                    CASI:
                    - 2 date distinte
                    - nessuna data
                    - una delle due *

                    CONTROLLI
                    - date distinte-> prima data inserita < seconda data
                    - controlli su numero mese anno giorno etc
                    - La variazione ha bisogno di due date distinte, mentre totale no
                */

                // numero errato di parametri
                if(ret != 3 && ret != 5){
                    printf("I parametri inseriti non sono validi, riprovare..\n");
                    continue;
                }

                // setup data iniziale se non specificata
                if(strcmp(data_iniziale , "*") == 0 || ret == 3){
                    sprintf(data_iniziale , "%d:%d:%d" , START_DAY , START_MONTH , START_YEAR);
                }  

                // setup data finale se non specificata
                if(strcmp(data_finale , "*") == 0 || ret == 3){
                    getFinalDate(data_finale);
                }

                ret = checkDates(data_iniziale , data_finale , tipo_aggr);

                if(!ret){
                    printf("Date non valide\n\n");
                    continue;
                }
                
                //printf("Date valide\n");
                

                //printf("Prima della calculate total:\nDataIniziale:%s\nDataFinale:%s\n" ,  data_iniziale , data_finale);

                strcpy(util_type , tipo);
                strcpy(util_aggr_type , tipo_aggr);

                tot = calculateTotal(data_iniziale , data_finale );

                if(strcmp(tipo_aggr , "TOTALE") == 0){
                    printf("Aggregazione Terminata, %s %s:%d\n" , tipo , tipo_aggr , tot);
                }

                send_pkt(listen_socket , "MUTX_LEA" , HEADER_LEN , manager_port , "MTX_LEAK");

            }

            memset(stdin_buffer , 0 , MAX_STDIN_LEN);
            memset(command , 0 , MAX_COMMAND_LEN);

        }

        // richiesta dal server o da altri peer
        /*
            se viene dal socket in socket buffer metto tutto il pacchetto, poi ottengo il codice del pacchetto nel buffer msg_type

        */
        else if(FD_ISSET(listen_socket , &read_fds)){
            int sender_port;
            char msg_type[MAX_COD_LEN+1];

            memset(request_buffer , 0 , STANDARD_LEN);

            sender_port = recv_send_pkt(listen_socket , request_buffer, STANDARD_LEN);

            // ottengo il codice del pacchetto
            sscanf(request_buffer , "%s" , msg_type);
            msg_type[MAX_COD_LEN] = '\0';

            if(sender_port == server_port){

                strcpy(server_buffer , request_buffer);
                
                // LISTA DEI NEIGHBOR
                if(strcmp(msg_type , "NBR_LIST") == 0){
                    
                    printf("Ricevuta la lista dei neighbors dal DS\n");
                    cleanNeighbors(neighbors);
                    send_ACK(listen_socket , "LIST_ACK" , server_port);

                    sscanf(server_buffer , "%s %d %d" , msg_type , &neighbors[0] , &neighbors[1]);
                    printf("I miei vicini sono: \n1) %d \n2) %d\n" , neighbors[0] , neighbors[1]);
                }

                // NEIGHBOR UPDATE
                if(strcmp(msg_type , "NBR_UPDT") == 0){

                    printf("Ricevuta la lista aggiornata dei neighbors dal DS\n");

                    cleanNeighbors(neighbors);
                    send_ACK(listen_socket , "UPDT_ACK" , server_port);

                    sscanf(server_buffer , "%s %d %d" , msg_type , &neighbors[0] , &neighbors[1]);
                    printf("I miei nuovi vicini sono: \n1) %d \n2) %d\n" , neighbors[0] , neighbors[1]);
                }

                // PORTA DEL MANAGER
                if(strcmp(msg_type , "MNG_PORT") == 0){
                    
                    sscanf(server_buffer , "%s %d" , msg_type , &manager_port);
                    send_ACK(listen_socket , "MNG_ACKP" , server_port);

                    if(manager_port == 0){
                        printf("Il DS ha provato ad inviare la porta del manager ma il manager non e' online\n");
                    }else{
                        printf("Ho ricevuto la porta del server manager\n");

                    }
                }
            }

            // manager
            else if(sender_port == manager_port){

                strcpy(manager_buffer , request_buffer);

                if(strcmp(msg_type , "TDAY_CLS") == 0){
                    int buf_len;

                    printf("Ho ricevuto la richiesta di close dal manager\nInvio al manager i dati odierni\n");

                    send_ACK(listen_socket , "TDAY_ACK" , manager_port);
                    buf_len = sprintf(manager_buffer , "%s %d %d" , "TDAY_AGG" , peer_data[TAMPONE_IND].value , peer_data[CASO_IND].value);
                    send_pkt(listen_socket , manager_buffer , buf_len , manager_port , "MDAY_ACK");

                    printf("Dati inviati\n");

                }

                else if(strcmp(msg_type , "DAY_DATA") == 0){

                    printf("Il manager mi ha inviato i dati aggregati relativi al giorno corrente\n");

                    sscanf(manager_buffer , "%s %d %d" , msg_type , &peer_data[TAMPONE_IND].value , &peer_data[CASO_IND].value);
                    send_ACK(listen_socket ,"DATA_ACK" , sender_port);

                    printf("I dati del giorno sono:\nTAMPONI:%d\nCASI:%d\nSalvo i dati ricevuti..\n" , peer_data[TAMPONE_IND].value , peer_data[CASO_IND].value);

                    writeOnFile(peer_data , my_port , 'F');

                    printf("Salvataggio avvenuto con successo\n");

                    data_flag = 1;
                    
                }

                else if(strcmp(msg_type , "BLCK_ADD") == 0){
                    printf("Blocco l'inserito e la ricerca di dati finche' il manager non chiude i register odierni\n");
                    data_flag = 0;
                    send_ACK(listen_socket , "BLCK_ACK" , manager_port);
                }
            }

            //peer
            else{
                memset(peer_buffer , 0 , MAX_STDIN_LEN);

                strcpy(peer_buffer , request_buffer);

                if(strcmp(msg_type , "REQ_ENTR") == 0){
                    /**
                     * devo:
                     *  -inviare i dati che ho relativi alla data richiesta, se ho la F nella data, evito di fare il flooding
                     *  -se non ho la F inoltro il pacchetto di flooding ai miei vicini, eccetto al sender
                     */

                    int tamponi, casi , request_id , to_flood;
                    char def;
                    char date[DATE_LEN];
                    int flood_requester;
                    //char request_buffer[MAX_STDIN_LEN];

                    send_ACK(listen_socket , "ENTR_ACK" , sender_port);

                    sscanf(peer_buffer , "%s %d %s %d %d" , msg_type ,&flood_requester , date , &request_id , &to_flood);

                    printf("Preparo il pacchetto per il flood requester\n");

                    readFromFile(date , &tamponi , &casi , &def);
                    sprintf(peer_buffer , "%s %s %d %d %c" , "ENTR_DAT" , date , tamponi , casi , def);
                    send_pkt(listen_socket , peer_buffer , STANDARD_LEN , flood_requester , "ENT_DACK");

                    //inoltro il pacchetto al mio vicino
                    if(neighbors[0] == -1 || neighbors[0] == flood_requester) continue;

                    send_pkt(listen_socket ,request_buffer , STANDARD_LEN , neighbors[0] , "ENTR_ACK");
                }

            }

            FD_CLR(listen_socket , &read_fds);
        }

    }

    return 0;
}
