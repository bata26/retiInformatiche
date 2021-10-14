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



extern int my_port;
extern int neighbors[NUM_NEIGHBORS];
extern int manager_port;
extern int listen_socket;
void cleanNeighbors(int * neighbors){
    int i;

    for( i = 0 ; i < NUM_NEIGHBORS ; i++){
        neighbors[i] = 0;
    }
}


void stampaComandi(int port){
    printf("*********************** PEER %d AVVIATO ***********************\n" , port);
    printf("Comandi disponibili:\n\n");
    printf("1) start DS_ADDR DS_PORT  -> Connette il peer al DS con indirizzo e porta specificati\n");
    printf("2) add type quantity -> Aggiunge il dato di tipo type \n");
    printf("3) get aggr type period -> Mostra Un resoconto aggregato aggr del dato type nel periodo period specificato \n");
    printf("4) stop -> Chiude la connessione con il DS\n");

    printf("Stampo roba per debvug extern:\n");
    printf("my_port:%d\n" , my_port);
    printf("neighbors[0]:%d\n" , neighbors[0]);
    printf("neighbors[1]:%d\n" , neighbors[1]);
    printf("manager_port:%d\n" , manager_port);
    printf("listen_socket:%d\n", listen_socket);
}

// bisestile se divisibile per 4 quando non e' secolare o divisibile per 400 quando secolare
int isLeapYear(int year){
    if( ( (year % 100 == 0) && (year % 400 == 0) ) || ( (year % 100 != 0) && (year % 4 == 0) ) ){
        return 1;
    }

    return 0;
}

int checkSingleDate(char date[DATE_LEN]){
    int result;
    int num_date[3];
    time_t t;
    struct tm* timeinfo;

    result = 0;

    /*
        date[0] -> day
        date[1] -> month
        date[2] -> year
    */
    sscanf(date , "%d:%d:%d" , &num_date[0] , &num_date[1] ,&num_date[2]);

    if( (num_date[2] < START_YEAR) || 
        (num_date[1] < START_MONTH && num_date[2] == START_YEAR) ||
        (num_date[2] == START_YEAR && num_date[1] && num_date[0] < START_DAY) 
    ){
        printf("La data inserita è antecedente alla data di avvio del sistema\n");
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

    // controllo che non ha inserito un giorno di un register non ancora chiuso

    time(&t);
    timeinfo = localtime(&t);

    result = 1;
    return result;

}

// controlla se le due date distinte sono accettabili
int checkDates(char data_iniziale[DATE_LEN] , char data_finale[DATE_LEN] , char tipo_aggr[AGGR_LEN]){
    //int result;
    int start_date[3];
    int end_date[3];

    printf("Nella checkDates()\n");

    if(strcmp(data_iniziale , "*") == 0 && strcmp(data_finale , "*") == 0){
        printf("Inserire solo un * o ometterli entrambi\n");
    }

    // controllo se una delle due e' un '*'
    if( (strcmp(data_iniziale , "*") == 0) && (checkSingleDate(data_finale)) ) return 1;

    if( (strcmp(data_finale , "*") == 0) && (checkSingleDate(data_iniziale)) ) return 1;

    printf("Dopo il controllo con *\n");

    sscanf(data_iniziale , "%d:%d:%d" , &start_date[0] , &start_date[1] , &start_date[2]);
    sscanf(data_iniziale , "%d:%d:%d" , &end_date[0] , &end_date[1] , &end_date[2]);

    printf("Dopo la sscanf\n");

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


FILE * findFirstDate(char data_iniziale[DATE_LEN] , int my_port){
    FILE* file_data;
    char filename[DATA_LEN];
    char temp_data[DATE_LEN];
    
    

    sprintf(filename , "%s%d%s" , FILE_PATH , my_port , ".txt");

    file_data = fopen(filename , "r");
    

    if(file_data == NULL){
        printf("Impossibile aprire il file\n");
        return NULL;
    }


    if(strcmp(data_iniziale , "*") == 0){
        return file_data;
    }

    while(!file_data == NULL){
        FILE * temp_data_file;
        temp_data_file = file_data;
        //printf("nel while");
        fscanf(file_data , "%s" , temp_data);
        //printf("Analizzo la data %s\n" , temp_data);

        if(strcmp(temp_data , data_iniziale) == 0) return temp_data_file;
        
    }

    printf("Impossibile trovare la data desiderata");
    return file_data;


}

void calculateTotal(char data_iniziale[DATE_LEN] , char data_finale[DATE_LEN] ,  char * tipo){
    FILE * file_data;
    int tamponi , casi;
    char temp_data[DATE_LEN];
    int tot , found , first;
    char line_buffer[50];

    memset(line_buffer , 0 , 50);

    file_data = findFirstDate(data_iniziale , my_port);
    
    if(file_data == NULL){
        printf("Errore nella caculate Total\n");
        return;
    }

    found = 0;
    tot = 0;

    first = 1;

    while(fgets(line_buffer , 50 , file_data) && !(found)){
        printf("%s\n" , line_buffer);

        // se e' il primo non ho la data nel puntatore del file
        if(first){
            sscanf(line_buffer , "%d %d" , &tamponi , &casi);
            first = 0;
        }else{
            sscanf(line_buffer , "%s %d %d" , temp_data , &tamponi , &casi);
            printf("temp_data--> %s\n" , temp_data);
        }

        if(strcmp(tipo , "TAMPONE") == 0){
            tot += tamponi;
            printf("TAMPONI == %d\n" , tamponi);
        }else{
            tot += casi;
            printf("CASI == %d\n" , casi);
        }

        printf("tot == %d\n" , tot);

    }

    printf("TOTALE %s --> %d\n" , tipo , tot);
}


/**
 * La data di partenza e' specificata in costanti.h
 * L'ultima data e' salvata dal manager
 * Struttura dell'elaborazione aggregato:
 * 
 * 1) Scorro il file dall'inizio, se e' * la data iniziale o finche' non la trovo
 * 2) confronto le date per capire se sono successive, devo avere dati di tutti i giorni
 * 3) se non trovo la data, mando una request ai peer
 * 
 * 
 * 
 * 
 * 
 */
// ritorna la data finale, questa coincide con il giorno corrente se sono passate le 18,
// con il giorno precedente se non sono passate
void getFinalDate(char*date){
    time_t t;
    struct tm* timeinfo;
    int day , year , month;

    time(&t);
    timeinfo = localtime(&t);

    // giorno precedente
    if(timeinfo->tm_hour < 18){

        // primo dell'anno
        if(timeinfo->tm_mday == 1){
            if(timeinfo->tm_mon == 0){
                day = 31;
                month = 12;
                year = timeinfo->tm_year - 1 + 1900;
            }
            else if(timeinfo->tm_mon == 1 || timeinfo->tm_mon == 3 || timeinfo->tm_mon == 5 || timeinfo->tm_mon == 8 || timeinfo->tm_mon == 10){
                day = 31;
            }
            else if(timeinfo->tm_mon == 2){
                day = 28 + isLeapYear(timeinfo->tm_year+1900);
            }
            else{
                day = 30;
            }
            
        }
        //ptimo giorno di un mese
        sprintf(date , "%d:%d:%d" , day , month , year);
    }else{
        sprintf(date , "%d:%d:%d" , timeinfo->tm_mday , timeinfo->tm_mon + 1 , timeinfo->tm_year + 1900);
    }


}