#include <stdio.h>

#include "costanti.h"


int getPeerIndex(int peer_port){
    return ((peer_port % BASE_PORT) - 1);
}

void addPeer(int peer_port , int* peer_list){
    int index = getPeerIndex(peer_port); 
    peer_list[index] = peer_port;
}

// cerca tra i peer connessi i due piu vicini al peer_port
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

void setupNeighbors(int* neighbors){
    int i , j;

    for(i = 0 ; i < NUM_PEER ; i++){
        for(j = 0 ; j < NUM_NEIGHBORS ; j++){
            printf("i-> %d , j-> %d\n" , i , j);
            neighbors[i][j] = -1;
        }
    }
}