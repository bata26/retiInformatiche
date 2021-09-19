#include "costanti.h"
// enumerazione per tipo
enum dataType{TAMPONE=0 , CASO=1};

// struct per salvare in memoria i dati
struct datiSalvati{
    enum dataType type; // tipo (tampone o caso)
    int value; // somma di tutti i dati raccolti fino a quel momento
};

void setupData(struct datiSalvati usr_data[DATA_LEN]);