#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>


#include "connection.h"
#include "costanti.h"
#include "data.h"



extern int my_port;
extern int neighbors[NUM_NEIGHBORS];
extern int manager_port;
extern int listen_socket;
extern int num_response;
extern int today_aggr;
extern int yesterday_aggr;
extern int peer_received[NUM_PEER];
extern int today_flag; // settata a 1 quando troviamo il def  
extern char util_aggr_type[AGGR_LEN]; // TOTALE, VARIAZIONE
extern char util_type[AGGR_LEN]; // TAMPONE CASO 

char buffer[MAX_STDIN_LEN];
int buf_len;
char header[HEADER_LEN];

int connected_peer;

extern struct  datiSalvati peer_data[DATA_LEN];

// scrivo su file i dati del register
void writeOnFile(char def){
    FILE * file_data;
    char filename[DATA_LEN];
    time_t t;
    struct tm * timeinfo;

    time(&t);
    timeinfo = localtime(&t);

    
    sprintf(filename , "%s%d%s" , FILE_PATH , my_port , ".txt");
    
    file_data = fopen(filename , "a");

    if(file_data == NULL){
        printf("Problemi con il file, riprovare..\n");
        return;
    }
    
    
    fprintf(file_data , "%d:%d:%d %d %d %c\n" ,  timeinfo->tm_mday  , timeinfo->tm_mon+1 , timeinfo->tm_year+1900 ,  peer_data[TAMPONE_IND].value , peer_data[CASO_IND].value , def);

    fclose(file_data);

    peer_data[TAMPONE_IND].value = 0;
    peer_data[CASO_IND].value = 0;
}

/*
	Return Value:
	1) 0 -> Le due date sono uguali
	2) 1 -> first_date e' PRECEDENTE a last_date
	3) 2 -> last_date e' PRECEDENTE a first_date
*/

int compareDates(char first_date[DATE_LEN] , char last_date[DATE_LEN]){
	int num_first_date[3];
	int num_last_date[3];

	sscanf(first_date , "%d:%d:%d" , &num_first_date[0] , &num_first_date[1] , &num_first_date[2]);
	sscanf(last_date , "%d:%d:%d" , &num_last_date[0] , &num_last_date[1] , &num_last_date[2]);


	// uguali
	if( ( num_first_date[0] ==  num_last_date[0]) && (num_first_date[1] ==  num_last_date[1]) && (num_first_date[2] ==  num_last_date[2])) return 0;

	// casi in cui first precedente last:
	// anno di last > anno di first
	// stesso anno ma mese maggiore
	// stesso anno stesso mese giorno maggiore

	if(   (num_last_date[2] > num_first_date[2]) ||
		( (num_last_date[2] == num_first_date[2]) && (num_last_date[1] > num_first_date[1]) ) ||
		( (num_last_date[2] == num_first_date[2]) && (num_last_date[1] == num_first_date[1]) && (num_last_date[0] > num_first_date[0]) ) 
	){
		return 1;
	}

	return 2;
	
}


/*
Se durante il flooding uno dei peer risponde con un dato marcato F, aggiorno il file del flooding peer
con i dati marcati
*/
void updateFile(char * buffer){
    /*
    Struttura del buffer:
    header data tamponi casi def
    */

    FILE * src;
    FILE * dest;
    char filename[FILE_LEN];
    char new_filename[FILE_LEN];
    char line_buffer[50];
    char temp_date[DATE_LEN];
    char buffer_date[DATE_LEN];
    char buffer_header[HEADER_LEN];

    int buffer_tamponi, buffer_casi;
    char buffer_def;

    printf("Aggiorno il mio file\n");

    sscanf(buffer , "%s %s %d %d %c" , buffer_header , buffer_date , &buffer_tamponi , &buffer_casi , &buffer_def);


    sprintf(filename , "%s%d%s" ,  FILE_PATH , my_port , ".txt");
    sprintf(new_filename , "%s%s%d%s" ,  FILE_PATH ,"new_" ,  my_port , ".txt");

    src = fopen(filename , "r");
    dest = fopen(new_filename , "w");

    printf("filename:%s\nnew_filename:%s\n" , filename , new_filename);

    if(src == NULL || dest == NULL){
        printf("Impossibile aggiornare il file\n");
        return;
    }

    while(fgets(line_buffer , 50 , src)){

        sscanf(line_buffer , "%s" , temp_date);

        //stessa data, quindi inserisco i dati del buffer
        if(compareDates(temp_date , buffer_date) == 0){
            fprintf(dest , "%s %d %d %c\n" , temp_date , buffer_tamponi , buffer_casi , buffer_def);
        }else{
            // copio le info dal file precedente
            fprintf(dest , "%s" , line_buffer);
        }
    }

    

    fclose(src);
    fclose(dest);

    if(remove(filename) == 0){
        if(rename(new_filename , filename) == 0){
            printf("Aggiornamento Riuscito con successo!\n");
        }
    }
}

/*
Pulisco la struttura neighbors
*/
void cleanNeighbors(int * neighbors){
    int i;

    for( i = 0 ; i < NUM_NEIGHBORS ; i++){
        neighbors[i] = 0;
    }
}

/*
aggiorno il dato aggregato con i dati appena ricevuti da un peer
*/
void getAggregateValue(char * buffer){
    int casi, tamponi , res;
    char def;
    char date[DATE_LEN];
    char msg[HEADER_LEN];
    
    // s eho già il dato definitivo del giorno
    if(today_flag) return;

    sscanf(buffer , "%s %s %d %d %c" , msg , date , &tamponi , &casi , &def);

    res = (strcmp(util_type , "TAMPONE") == 0) ? tamponi : casi;
    
    // se marcato aggiorno il file
    if(def == 'F'){
        today_aggr = res;
        today_flag = 1;

        updateFile(buffer);
    }else{
        today_aggr += res;
    }

}


void stampaComandi(int port){
    printf("*********************** PEER %d AVVIATO ***********************\n" , port);
    printf("Comandi disponibili:\n\n");
    printf("1) start DS_PORT  -> Connette il peer al DS con indirizzo e porta specificati\n");
    printf("2) add type quantity -> Aggiunge il dato di tipo type \n");
    printf("3) get aggr type period -> Mostra Un resoconto aggregato aggr del dato type nel periodo period specificato \n");
    printf("4) stop -> Chiude la connessione con il DS\n");
}

// ritorna 1 se ho ricevuto tutte le risposte dai peer attualmente connessi
int allPeer(){
    return num_response == connected_peer;
}

// controlla se la porta da cui abbiamo ricevuto entr_dat sia gia stata consultata oppure no
int checkDataReceived(int port){
    int i;

    // devo controllare che il peer port non abbia gia inviato una risposta
    if(num_response == connected_peer){
        printf("Ho gia' esaminato le informazioni del peer %d\n" , port);
        return 0;
    }

    for(i = 0 ; i <  NUM_PEER ; i++){
        if(peer_received[i] == port) return 0;
    }

    // se esco e' perche non c'e' quel peer nell'array, quindi lo devo inserire
    peer_received[num_response] = port;
    num_response++;
    
    return 1;
}

// pulisco variabili per avviare un nuovo flooding
void setupForFlooding(){
    int i;

    num_response = 0;
    today_flag = 0;

    for(i = 0 ; i < NUM_PEER ; i++){
        peer_received[i] = 0;
    }
}

// manda ai vicini una richiesta per una specifica data
void askToPeer(char date[DATE_LEN] , int* tot){
    char receiver_buffer[MAX_STDIN_LEN];

    memset(buffer , 0 , MAX_STDIN_LEN);

    // chiedo al manager il numero di peer attualmente connessi
    send_pkt(listen_socket , "TOT_PEER" , HEADER_LEN , manager_port , "PEER_ACK");
    recv_pkt(listen_socket , buffer , MAX_STDIN_LEN , manager_port , "PEER_LST" , "PLST_ACK");

    connected_peer = 0;

    //printf("buffer ricevuto dal manager: %s\n" , buffer);

    sscanf(buffer , "%s %d" , header , &connected_peer );
    connected_peer--;

    //printf("Ci sono %d peer attualmente connessi oltre me\n" , connected_peer);

    if(connected_peer == 0){
    //    printf("Sono L'unico peer connesso impossibile ottenere il dato richiesto\n");
        return;
    }

    setupForFlooding();

    // preparo il buffer
    buf_len = sprintf(buffer , "%s %d %s" , "REQ_ENTR" , my_port , date);

    memset(receiver_buffer , 0 , MAX_STDIN_LEN);

    // mando la req al primo vicino poi mi metto in attesa di tutte le risposte
    send_pkt(listen_socket , buffer , buf_len , neighbors[0] , "ENTR_ACK");

    // finchè non ho ricevuto la risposta da tutti i peer o ho ottenuto il register definitivo
    while(!allPeer() && !today_flag){
        int port;

        port = recv_pkt(listen_socket , receiver_buffer , STANDARD_LEN ,ALL_PEER , "ENTR_DAT" , "ENT_DACK");
        
        if(checkDataReceived(port)){
            getAggregateValue(receiver_buffer);
        }
    }

    *tot += today_aggr;

}

// bisestile se divisibile per 4 quando non e' secolare o divisibile per 400 quando secolare
int isLeapYear(int year){
    if( ( (year % 100 == 0) && (year % 400 == 0) ) || ( (year % 100 != 0) && (year % 4 == 0) ) ){
        return 1;
    }

    return 0;
}

// controlla se una data è valida
int checkSingleDate(char date[DATE_LEN]){
    int result;
    int num_date[3];

    result = 0;

    /*
        date[0] -> day
        date[1] -> month
        date[2] -> year
    */
    sscanf(date , "%d:%d:%d" , &num_date[0] , &num_date[1] ,&num_date[2]);

    if( (num_date[2] < START_YEAR) || 
        (num_date[1] < START_MONTH && num_date[2] == START_YEAR) ||
        (num_date[2] == START_YEAR && num_date[1] == START_MONTH && num_date[0] < START_DAY) 
    ){
        printf("La data inserita è antecedente alla data di avvio del sistema\n");
        return result;
    }

    if(num_date[2] < 2020 || num_date[2] > 2021){
        printf("Anno inserito non valido\n");
        return result;
    }

    if(num_date[1] < 1 || num_date[1] > 12){
        printf("Mese inserito non valido\n");
        return result;
    }

    if(num_date[0] < 1 || num_date[0] > 31){
        printf("Giorno inserito non valido\n");
        return result;
    }

    // se il giorno e' 31 ma il mese non ha 31 giorni
    if(num_date[0] == 31 && (num_date[1] == 2 || num_date[1] == 4 || num_date[1] == 6 || num_date[1] == 9 || num_date[1] == 11)){
        printf("Il mese inserito non ha 31 giorni\n");
        return result;
    }

    if(num_date[0] == 30 && num_date[1] == 2 ){
        printf("Febbraio non ha 30 giorni\n");
        return result;
    }

    // controllo se siamo in un anno bisestile qualora l'utente abbia inserito 29:02:yyyy
    

    if(num_date[0] == 29 && num_date[1] == 2 && !(isLeapYear(num_date[2]))){
        printf("Si e' inserito il 29 febbraio di un anno non bisestile");
        return result;
    }


    result = 1;
    return result;

}

/*
controlla le due date inserite dall'utente per la get
requisiti:
    - data iniziale < data finale se get chiede CASO
    - data iniziale <= data finale se get chiede TAMPONE
    - che sia data finale che data iniziale siano validi tramite la checkSingleDate
*/ 
int checkDates(char data_iniziale[DATE_LEN] , char data_finale[DATE_LEN] , char tipo_aggr[AGGR_LEN]){
    
    int start_date[3];
    int end_date[3];

    //printf("Nella checkDates()\ndata_iniziale:%s\ndata_finale:%s\n" , data_iniziale , data_finale);

    if(strcmp(data_iniziale , "*") == 0 && strcmp(data_finale , "*") == 0){
        printf("Inserire solo un * o ometterli entrambi\n");
    }

    // controllo se una delle due e' un '*'
    if( (strcmp(data_iniziale , "*") == 0) && (checkSingleDate(data_finale)) ) return 1;

    if( (strcmp(data_finale , "*") == 0) && (checkSingleDate(data_iniziale)) ) return 1;


    sscanf(data_iniziale , "%d:%d:%d" , &start_date[0] , &start_date[1] , &start_date[2]);
    sscanf(data_finale , "%d:%d:%d" , &end_date[0] , &end_date[1] , &end_date[2]);


    if(!checkSingleDate(data_iniziale) || !checkSingleDate(data_finale)) return 0;

    /*
        date[0] -> day
        date[1] -> month
        date[2] -> year
    */
    if( 
        (start_date[2] < end_date[2]) ||  // anno della data iniziale minore dell'anno della data finale, sicuramente precedente
        (start_date[2] == end_date[2] && start_date[1] < end_date[1]) ||  // stesso anno ma la data iniziale ha un mese precedente a quello della data finale
        (start_date[2] == end_date[2] && start_date[1] == end_date[1] && start_date[0] < end_date[0] && strcmp(tipo_aggr , "VARIAZIONE") == 0) || // stesso mese e anno, giorno minore e richiesta la variazione, quando e' richiesto il totale le date possono coincidere
        (start_date[2] == end_date[2] && start_date[1] == end_date[1] && start_date[0] <= end_date[0] && strcmp(tipo_aggr , "TOTALE") == 0) // stesso mese e anno con giorno minore uguale, ma richiesta di totale  
    ){
        return 1;
    }

    return 0;
}


// calcolo il dato aggregato richiesto
int calculateTotal(char data_iniziale[DATE_LEN] , char data_finale[DATE_LEN]){
    FILE * file_data;
    int tamponi , casi;
    char prec_date[DATE_LEN] , current_date[DATE_LEN];
    int tot , found , first , done , all_final;
    char line_buffer[50];
    char filename[DATA_LEN];
	int ret;
    char final;
    int yesterday_flag;
    int alone;


    // buffer che rappresenta una riga del file
    memset(line_buffer , 0 , 50);

    sprintf(filename , "%s%d%s" , FILE_PATH , my_port , ".txt");

    file_data = fopen(filename , "r");
    

    if(file_data == NULL){
        printf("Impossibile aprire il file\n");
        return -1;
    }

    found = 0; // flag che indica se ho trovato o no la prima data
    first = 1;
    final = 0;
    done  = 0;
    tot = 0;
    today_aggr = 0;
    alone = 0;
    
    // flag che indica se ho ottenuto il dato finale o no
    all_final = 1;

    if(neighbors[0] == 0 && neighbors[1] == 0){
        alone = 1;
        all_final = 0;
        printf("Non ho vicini, quindi considererò solo i miei dati\n");
    }

    /*
    scorro il file, per ogni riga confronto con la data iniziale, una volta trovata:
    - se ho il dato definitivo per quel giorno lo salvo e passo alla prossima data
    - altrimenti avvio il flooding
    ripeto fincheè non ho analizzato la data finale
    */
    while(fgets(line_buffer , 50 , file_data) && !(done)){

        // mantengo in prec_date la data prima di quella che stiamo analizzando 
        strcpy(prec_date , current_date);
        sscanf(line_buffer , "%s %d %d %c" , current_date , &tamponi , &casi , &final);

        //printf("Analizzo la data %s\n" , current_date);


        // sto cercando ancora la prima, quindi non calcolo i dati aggregati
        if(!found){

		    ret = compareDates(current_date , data_iniziale);
		   
		    if(ret == 0){
			    printf("inizio l'aggregazione\n");
	            found = 1;
                first = 1;
		    }
		    printf("\n");
        }

        if(found){ // trovato la prima data quindi conto
           
            yesterday_aggr = today_aggr;
            yesterday_flag = today_flag;

            today_aggr = (strcmp(util_type , "TAMPONE") == 0) ? tamponi : casi;

            // se non ho il dato definitivo chiedo ai peer
            if(final != 'F'){
                if(alone){
                    
                    tot += today_aggr;
                }
                askToPeer(current_date , &tot);
            // se ho il dato definitivo non ho bisogno del flooding
            }else{
                tot += today_aggr;
                today_flag = 1;
            }

            if(today_flag == 0){
                all_final = 0;
            }


            if(!first && strcmp(util_aggr_type , "VARIAZIONE") == 0){
                //printf("today_aggr->%d\nyesterday_aggr->%d\n" , today_aggr , yesterday_aggr);
                printf("%s %s: %d" ,  current_date , prec_date , (today_aggr - yesterday_aggr));
                if(!today_flag || !yesterday_flag) printf(" *");
                printf("\n");
            }

            first = 0;
           
        }

        if(compareDates(current_date , data_finale) == 0) done = 1;

        

    }

    if(!all_final) printf("\n\nN.B. Il dato calcolato potrebbe non essere corretto in quanto non riesco ad avere informazioni certe per tutte le date inserite\n\n\n");
    return tot;
}



/*
ottengo la data finale nel caso in cui l'utente abbia inserito '*'
questa è banalmente l'ultima data di cui abbiamo dati nel file
*/
void getFinalDate(char date[DATE_LEN]){
    FILE * file_data;
    char filename[FILE_LEN];
    char line_buffer[50];


    sprintf(filename , "%s%d%s" , FILE_PATH , my_port , ".txt");

    file_data = fopen(filename , "r");

    if(file_data == NULL){
        printf("Impossibile aprire il file\n");
        strcpy("" , date);
        return;
    }

    while(fgets(line_buffer , 50 , file_data)){
        sscanf(line_buffer , "%s" , date);
    }

}

/*
Legge dal proprio file le informazioni sulla data date
*/
void readFromFile(char* date, int *tamponi , int* casi , char* def){
    FILE * file_data;
    char line_buffer[50];
    char filename[DATA_LEN];
    char temp_data[DATE_LEN];


    memset(line_buffer , 0 , 50);

    sprintf(filename , "%s%d%s" , FILE_PATH , my_port , ".txt");

    file_data = fopen(filename , "r");
    

    if(file_data == NULL){
        printf("Impossibile aprire il file\n");
        return;
    }

    while(fgets(line_buffer , 50 , file_data)){
        sscanf(line_buffer , "%s %d %d %c" , temp_data , tamponi , casi , def);

        // data richiesta
        if(compareDates(date , temp_data) == 0){
            //printf("trovata la data richiesta\n");
            return;
        }
    }
}