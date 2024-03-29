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
