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
char server_buffer[MAX_PKT_LEN];
// buffer per i comandi da manager
char manager_buffer[MAX_STDIN_LEN];
// buffer per altri peer
char peer_buffer[MAX_STDIN_LEN];

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

    while(1){

        // la select sposta da read_fds, in questo modo il set master
        // non viene modificato
        read_fds = master; 

        select(fdmax , &read_fds , NULL , NULL , NULL);

        // se è arrivato qualcosa da stdin
        if(FD_ISSET( 0 , &read_fds)){

            char command[MAX_COMMAND_LEN];
            
            fgets(stdin_buffer , MAX_STDIN_LEN , stdin);
            sscanf(stdin_buffer, "%s", command);
            printf("Ricevuto il comando %s\n" , command);

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
                // cosa mi serve:
                // -Tipo (TAMPONE  , NUOVO CASO)
                // quantità
                
                enum dataType type;
                int value;
                char tipo[MAX_COMMAND_LEN];

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

                //printf("\nValore di stdin_buffer->%s\n\n" , stdin_buffer);
                //printf("Data iniziale-> %s\n"  , data_iniziale);

                ret = sscanf(stdin_buffer , "%s %s %s %s %s" , command , tipo_aggr , tipo , data_iniziale , data_finale);

                printf("\nAppena inseriti:\nTipo_aggr->%s\nTipo->%s\nDataIniziale->%s\nDataFinale->%s\n" , tipo_aggr , tipo , data_iniziale , data_finale);

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

                printf("Valore di ret dopo la scanf-> %d\n" , ret);
                // numero errato di parametri
                if(ret != 3 && ret != 5){
                    printf("I parametri inseriti non sono validi, riprovare..\n");
                    continue;
                }

                ret = checkDates(data_iniziale , data_finale , tipo_aggr);
                printf("Dopo la checkdate()\nret--> %d\n" , ret);

                if(ret){
                    printf("Date valide!!\n\n");
                }else{
                    printf("Date non valide\n");
                }

                //printf("Dopo la checkDates:\nDataIniziale:%s\nDataFinale:%s\n" ,  data_iniziale , data_finale);


                if(strcmp(data_iniziale , "*") == 0){
                    sprintf(data_iniziale , "%d:%d:%d" , START_DAY , START_MONTH , START_YEAR);
                }

                if(strcmp(data_finale , "*") == 0){
                    //printf("\n\nPrima della final date:\nDataIniziale:%s\nDataFinale:%s\n\n" ,  data_iniziale , data_finale);
                    getFinalDate(data_finale);
                    //printf("\n\nDOPO della final date:\nDataIniziale:%s\nDataFinale:%s\n\n" ,  data_iniziale , data_finale);
                }

                printf("Prima della calculate total:\nDataIniziale:%s\nDataFinale:%s\n" ,  data_iniziale , data_finale);

                if(strcmp(tipo_aggr , "TOTALE") == 0){
                    calculateTotal(data_iniziale , data_finale );
                }


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

            memset(server_buffer , 0 , MAX_PKT_LEN);

            sender_port = recv_send_pkt(listen_socket , server_buffer, MAX_PKT_LEN);

            // ottengo il codice del pacchetto
            sscanf(server_buffer , "%s" , msg_type);
            msg_type[MAX_COD_LEN] = '\0';

            if(sender_port == server_port){
                
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

                memset(manager_buffer , 0 , MAX_STDIN_LEN);

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

                    sscanf(server_buffer , "%s %d %d" , msg_type , &peer_data[TAMPONE_IND].value , &peer_data[CASO_IND].value);
                    send_ACK(listen_socket ,"DATA_ACK" , sender_port);

                    printf("I dati del giorno sono:\nTAMPONI:%d\nCASI:%d\nSalvo i dati ricevuti..\n" , peer_data[TAMPONE_IND].value , peer_data[CASO_IND].value);

                    writeOnFile(peer_data , my_port , 'F');

                    printf("Salvataggio avvenuto con successo\n");
                    
                }
            }

            //peer
            else{
                memset(peer_buffer , 0 , MAX_STDIN_LEN);

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
                    char request_buffer[MAX_STDIN_LEN];

                    send_ACK(listen_socket , "ENTR_ACK" , sender_port);

                    sscanf(server_buffer , "%s %d %s %d %d" , msg_type ,&flood_requester , date , &request_id , &to_flood);

                    printf("LAST_REQ->%d\nCURRENT_REQ->%d\n" , last_flood_request_id , request_id);

                    // richiesta gia gestita
                    if(request_id == last_flood_request_id && served_flag != 0){
                        printf("Ho gia' partecipato a questo flooding, interrompo\n");
                        continue;
                    }
                    // aggiorno l'id dell'ultima request e segno nel flag che non ho inoltrato il pacchetto a nessuno
                    last_flood_request_id = request_id;
                    served_flag =  0;
                    
                    readFromFile(date , &tamponi , &casi , &def);

                    printf("Ho letto dal file:\nCASI:%d\nTAMPONI:%d\nDEF:%c\n" , tamponi , casi , def);

                    sprintf(request_buffer , "%s %s %d %d %c" , "ENTR_DAT" , date , tamponi , casi , def);
                    printf("Invio al richidente i dati relativi alla data %s\n" , date);
                    send_pkt(listen_socket , request_buffer , MAX_STDIN_LEN , flood_requester , "ENT_DACK");

                    //continue;

                    // dopo aver inviato i miei dati controllo se devo fare flooding o no

                    if(def == 'F' || to_flood == 0){
                        printf("Ho il dato definitivo\n");
                    }else{

                        int dest_port = 0;

                        /* non devo inoltrare la richiesta ne al sender ne al requester*/ 

                        if(neighbors[0] != 0 && neighbors[0] != sender_port && neighbors[0] != flood_requester) dest_port = neighbors[0];
                        if(neighbors[1] != 0 && neighbors[1] != sender_port && neighbors[1] != flood_requester) dest_port = neighbors[1];
                        
                        if(!dest_port)   continue;

                        printf("Non ho il dato definitivo quindi inoltro il pacchetto di richiesta a %d\n" , dest_port);

                        send_pkt(listen_socket , server_buffer , MAX_STDIN_LEN , dest_port , "ENTR_ACK");
                    }         

                    served_flag = 1;           
                }

                else if(strcmp(msg_type , "ENTR_DAT") == 0){

                    // anche qui

                    printf("ACK INVIATO DALLA SELECT\nDOPO IL PKT DI %d\n" , sender_port);
                    send_ACK(listen_socket , "ENT_DACK" , sender_port);

                    checkDataReceived(sender_port , server_buffer);
                }
            }

            FD_CLR(listen_socket , &read_fds);
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
