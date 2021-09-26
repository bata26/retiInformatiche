#include <stdio.h>

#include "costanti.h"


int getPeerIndex(int peer_port){
    return ((peer_port % BASE_PORT) - 1);
}

// aggiunge un peer alla lista dei peer
void addPeer(int peer_port , int* peer_list){
    int index = getPeerIndex(peer_port); 
    peer_list[index] = peer_port;
}

// cerca tra i peer connessi i due piu vicini al peer_port e li restituisce nella lista neighbors
void getNeighbors(int peer_port , int* neighbors , int* peer_list){
    int index , cur_next , cur_prev , i , cur_prev_diff , cur_next_diff , cur_diff; // i cur indicano gli indici dei presenti in neighbors

    // ottengo l'index del peer corrente
    index = getPeerIndex(peer_port);

    cur_prev = -1; // indice relativo alla posizione 0 dell'array dei vicini 
    cur_next = -1; // indice relativo alla posizione 1 dell'array dei vicini 

    neighbors[0] = -1; // inizializzo i neighbors cosi da sapere se sono presenti o no
    neighbors[1] = -1; // inizializzo i neighbors cosi da sapere se sono presenti o no
    
    cur_prev_diff = NUM_NEIGHBORS; // differenza di indice corrente tra il peer alla posizione 0 e il peer corrente
    cur_next_diff = NUM_NEIGHBORS; // differenza di indice corrente tra il peer alla posizione 1 e il peer corrente

    printf("Sto analizzando il peer %d per trovare i vicini\n" , peer_port);

    // cerco gli indici piu "vicini" al peer
    for(i = 0 ; i < NUM_PEER ; i++){

        // se sono io salto
        if( i == index ) continue;
        
        printf("Analizzo i-->%d\n" , i);
        // se esiste il peer .. 
        if(peer_list[i] != 0){

            printf("Peer %d diverso da 0" , i);

            // primo posto vuoto
            if( cur_prev == -1){
                neighbors[0] = peer_list[i];
                cur_prev = i;

                cur_prev_diff = (index > i) ? (index - i) : (i - index);

            // secondo posto vuoto
            }else if(cur_next == -1){
                neighbors[1] = peer_list[i];
                cur_next = i;

                cur_next_diff = (index > i) ? (index - i) : (i - index);

            //entrambi i posti pieni, controllo se quello che sto esaminando e' piu vicino
            }else{

                // differenza tra quello che sto esaminando e il peer
                cur_diff = (index > i) ? (index - i) : (i - index);

                // se e' piu vicino di neighbors[0]
                if(cur_diff < cur_prev_diff){

                    neighbors[0] = peer_list[i];
                    cur_prev = i;
                    cur_prev_diff = (index > i) ? (index - i) : (i - index);

                }else if(cur_diff < cur_next_diff){

                    neighbors[1] = peer_list[i];
                    cur_next = i;
                    cur_next_diff = (index > i) ? (index - i) : (i - index);

                }
            }
        }
    }

    printf("done");

}

// controlla se rispetto ai vecchi peer la connessione/disconnessione ha portato ad un cambiamento
int checkIfUpdated(int* neighbors , int* new_neighbors){
    int updated = 0;



    if(neighbors[0] != new_neighbors[0]){
        printf("diversi quindi scambio");
        neighbors[0] = new_neighbors[0];
        updated = 1; 
    }

    if(neighbors[1] != new_neighbors[1]){
        neighbors[1] = new_neighbors[1];
        updated = 1; 
    }

    return updated;
}


// "pulisce" la matrice neighbors 
void setupNeighbors(int neighbors[][NUM_NEIGHBORS]){
    int i , j;

    for(i = 0 ; i < NUM_PEER ; i++){
        for(j = 0 ; j < NUM_NEIGHBORS ; j++){
            printf("i-> %d , j-> %d\n" , i , j);
            neighbors[i][j] = -1;
        }
    }
}

// controlla tutta la lista dei peer e aggiorna le struttre dati
void updateNeighbors(int* peer_list , int  neighbors[][NUM_NEIGHBORS] , int * updated){
    int i;
    int neighbors_current_peer[NUM_NEIGHBORS];
    // ogni volta che aggiungo un peer controllo se i vicini sono cambiati o no
    for(i = 0 ; i < NUM_PEER ; i++){
        printf("cerco di aggiornare il peer %d" , peer_list[i]);

        if(peer_list[i] == 0) continue;

        // aggiorno la struttura
        getNeighbors(peer_list[i] , neighbors_current_peer , peer_list);

        // controllo se ho cambiato qualcosa, se ho cambiato qualcosa mando un pacchetto di UPDATE


        printf("Sto esaminando il peer %d e i vicini che ho trovato sono : %d , %d\n" , peer_list[i] , neighbors_current_peer[0] , neighbors_current_peer[1]);
        if(checkIfUpdated(neighbors[i] , neighbors_current_peer)){
            updated[i] = 1;
        }
    }
}


// preparo il buffer per eventuali Neighbors update o List
void setupNeighborsBuffer(char * buffer , int* len , char * msg , int  neighbors[][NUM_NEIGHBORS] , int index){
    // invio la lista dei neighbors a chi si e' appena connesso
    if(neighbors[index][0] == -1 && neighbors[index][1] == -1)
        *len = sprintf(buffer , "%s" , msg);
    else if(neighbors[index][1] == -1)
        *len = sprintf(buffer , "%s %d" , msg  , neighbors[index][0]);
    else   
        *len = sprintf(buffer , "%s %d %d" ,msg  , neighbors[index][0] , neighbors[index][1]);

}

// rimuovo il peer disconnesso dai neighbors

void cleanNeighbors(int sender_port , int  neighbors[][NUM_NEIGHBORS]){
    int i , j;
    
    for(i = 0 ; i < NUM_PEER; i++){
        for(j = 0 ; j < NUM_NEIGHBORS ; j++){
            if(neighbors[i][j] == sender_port){
                neighbors[i][j] = -1;
            }
        }
    }

    neighbors[getPeerIndex(sender_port)][0] = -1;
    neighbors[getPeerIndex(sender_port)][1] = -1;

}