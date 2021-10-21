#include <stdio.h>

#include "costanti.h"

extern int num_peer;
extern int neighbors[NUM_PEER][NUM_NEIGHBORS];
extern int peer[NUM_PEER];

int getPeerIndex(int peer_port){
    return ((peer_port % BASE_PORT) - 1);
}


void findNextNeighbor(int direction, int index){
    int found, temp_index;

    found = 0;

    if(direction){
        temp_index = (index + 1)%NUM_PEER;
    }else{
        if(index == 0) temp_index = NUM_PEER - 1;
        else temp_index = index - 1;
    }

    while(!found && temp_index != index ){

        if(peer[temp_index] != 0){
            neighbors[index][direction] = peer[temp_index];
            break;
        }

        if(direction){
            temp_index = (temp_index + 1)%NUM_PEER;
        }else{
            if(temp_index == 0) temp_index = NUM_PEER - 1;
            else temp_index--;
        }
    }
}


// prende in input una porta e modifica la struttra neighbors con i nuovi vicini di quel peer
void getNeighbors(int *updated){
    int i, temp_first_neig , temp_second_neig , index;

    if(num_peer == 1){
        printf("un solo peer connesso, nessun vicino disponibile\n");
        return;
    }

    if(num_peer == 2){
        printf("Branch con due soli peer connessi\n");
        for(i = 0; i < NUM_PEER ; i++){
            if(peer[i] == 0) continue;
            
            printf("Analizzo il peer %d, i suoi vicini prima della findnext sono:\n%d\n%d\n" , peer[i] , neighbors[i][0] , neighbors[i][1]);
            findNextNeighbor(0 , i);

            // controllo di non avere duplicare, puo' succedere quando si disconnette il peer che era in posizione 0
            if(neighbors[i][0] == neighbors[i][1]) neighbors[i][1] = -1;

            updated[i] = 1;
        }

        return;
    }

    /*
        scorro tutti i peer, per ogni peer:
            salvo i neighbors correnti
            cerco un peer piu vicino verso destra
            cerco un peer piu vicino verso sinistra
            aggiorno la struttura
            se i nuovi vicini sono diversi da quelli precedenti, mando un pacchetto di aggiornamento
    */
    for(i = 0 ; i < NUM_PEER ; i++){
        int found , temp_index;

        if(peer[i] == 0) continue;

        temp_first_neig = neighbors[i][0];
        temp_second_neig = neighbors[i][1];

        findNextNeighbor(0 , i);
        findNextNeighbor(1 , i);

        if(temp_first_neig != neighbors[i][0]) updated[i] = 1;
        if(temp_second_neig != neighbors[i][1]) updated[i] = 1;
        
    }
}

// aggiunge un peer alla lista dei peer
void addPeer(int peer_port , int* peer_list){
    int index = getPeerIndex(peer_port); 
    peer_list[index] = peer_port;
}

void removePeer(int peer , int* peer_list ){
    peer_list[getPeerIndex(peer)] = 0;
}


// "pulisce" la matrice neighbors 
void setupNeighbors(int neighbors[][NUM_NEIGHBORS]){
    int i , j;

    for(i = 0 ; i < NUM_PEER ; i++){
        for(j = 0 ; j < NUM_NEIGHBORS ; j++){
            neighbors[i][j] = -1;
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


// stampa i comandi possibili appena il DS viene avviato
void stampaComandi(){
    printf("*********************** DS AVVIATO ***********************\n");
    printf("Comandi disponibili:\n\n");
    printf("1) help -> Mostra i dettagli dei singoli comandi\n");
    printf("2) showpeers -> Mostra i peer attualmente connessi\n");
    printf("3) showneighbor <peer> -> Mostra i vicini di un peer specificato\n");
    printf("4) esc -> Chiude il DS e tutti i peer\n");
}


// comando help
void stampaDettagli(){
    printf("Il DS contiene informazioni relative ai client che si connettono e condividono informazioni relative all'andamento della pandemia relativa al virus SarsCov2.\n\n");
    printf("Tramite i comandi disponibili è possibile ottenere informazioni relative ai peer connessi e ai vicini di ogni peer.\n");
    printf("Il comando showpeers permette di avere una lista dei peer connessi al DS mostrando il numero di porta che identifica il peer.\n");
    printf("Il comando showneighbor mostra i vicini di un client, ogni client possiede fino ad un massimo di due peer. È possibile specificare il numero di porta di un client per avere informazioni sui vicini di un peer specifico, altrimenti il comando mostrerà i vicini di ogni client.\n");
    printf("Il comando esc chiude il DS, questo comporta anche la terminazione di tutti i client, i quali salveranno sui specifici file le informazioni ottenute fino a quel momento.\n");
}


// comando showpeers
void stampaPeer(int * peer_list){
    int i;
    printf("Liste dei peer connessi:\n");
    for(i = 0 ; i < NUM_PEER ; i++){
        if(peer_list[i] == 0) continue;

        printf("-%d\n" , peer_list[i]);
    }
}

//showneighbor
void stampaNeighbors(int neighbors[][NUM_NEIGHBORS] , int peer , int* peer_list){
    int i , j;

    if(peer == 0){
        for(i = 0 ; i < NUM_PEER ; i++){
            if(peer_list[i] == 0) continue;
            printf("PEER -> %d:\n" , peer_list[i]);
            for(j = 0 ; j < NUM_NEIGHBORS ; j++){
                printf("%d) %d\n" , (j+1) , neighbors[i][j]);
            }
        }
    }else if(peer != 0 && peer >= 5001 && peer <= 5005){
        printf("PEER -> %d:\n" , peer);
        printf("1) %d\n" , neighbors[getPeerIndex(peer)][0]);
        printf("2) %d\n" , neighbors[getPeerIndex(peer)][1]);
    }else{
        printf("Peer inserito non valido, ritentare!\n");
    }




}