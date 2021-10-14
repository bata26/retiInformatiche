#include "costanti.h"
#include "stdio.h"

#include <sys/time.h>
#include <time.h>


// enumerazione per tipo
enum dataType{TAMPONE=0 , CASO=1};

// struct per salvare in memoria i dati
struct datiSalvati{
    enum dataType type; // tipo (tampone o caso)
    int value; // somma di tutti i dati raccolti fino a quel momento
};


void setupData( struct datiSalvati usr_data[DATA_LEN]){
    usr_data[0].type = TAMPONE;
    usr_data[1].type = CASO;
    usr_data[0].value = 0;
    usr_data[1].value = 0;
}

void writeOnFile(struct datiSalvati usr_data[DATA_LEN] , int port){
    FILE * file_data;
    char filename[DATA_LEN];
    time_t t;
    struct tm * timeinfo;

    time(&t);
    timeinfo = localtime(&t);

    //printf("Info:\nanno:%d\nmese:%d\ngiorno:%d\n" ,timeinfo->tm_year+1900 , timeinfo->tm_mon+1 , timeinfo->tm_mday );
    
    sprintf(filename , "%s%d%s" , FILE_PATH , port , ".txt");
    
    file_data = fopen(filename , "a");

    if(file_data == NULL){
        printf("Problemi con il file, riprovare..\n");
        return;
    }
    
    fprintf(file_data , "%d:%d:%d %d %d\n" , timeinfo->tm_year+1900 , timeinfo->tm_mon+1 , timeinfo->tm_mday  ,  usr_data[TAMPONE_IND].value , usr_data[CASO_IND].value);

    fclose(file_data);

    usr_data[TAMPONE_IND].value = 0;
    usr_data[CASO_IND].value = 0;
}