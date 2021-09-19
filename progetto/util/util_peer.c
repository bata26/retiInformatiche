#include "costanti.h"
#include <stdio.h>

void cleanNeighbors(int * neighbors){
    int i;

    for( i = 0 ; i < NUM_NEIGHBORS ; i++){
        neighbors[i] = 0;
    }
}


void stampaComandi(){
     printf("*********************** PEER AVVIATO ***********************\n");
    printf("Comandi disponibili:\n\n");
    printf("1) start DS_ADDR DS_PORT  -> Connette il peer al DS con indirizzo e porta specificati\n");
    printf("2) add type quantity -> Aggiunge il dato di tipo type \n");
    printf("3) get aggr type period -> Mostra Un resoconto aggregato aggr del dato type nel periodo period specificato \n");
    printf("4) stop -> Chiude la connessione con il DS\n");
}