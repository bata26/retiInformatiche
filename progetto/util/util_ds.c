#include "costanti.h"



int getPeerIndex(int peer_port){
    return ((peer_port % BASE_PORT) - 1);
}

void addPeer(int peer_port , int* peer_list){
    int index = getPeerIndex(peer_port); 
    peer_list[index] = peer_port;
}

// cerca tra i peer connessi i due piu vicini al peer_port
void getNeighbor(int peer_port , int* neighbors , int* peer_list){
    int next , prev , index , cur_next , cur_prev , i , cur_prev_diff , cur_next_diff , cur_diff; // i cur indicano gli indici dei presenti in neighbors

    index = getPeerIndex(peer_port);

    //prev = (getPeerIndex(peer_port) == 0) ? (NUM_NEIGHBORS - 1): (index - 1) ;
    //next = (index +1)%NUM_NEIGHBORS;

    // ottengo il "limite" per cercare i neghbor da
    //bound = (next + NUM_NEIGHBORS%2) % NUM_NEIGHBORS;

    cur_prev = -1;
    cur_next = -1;
    neighbors[0] = -1;
    neighbors[1] = -1;
    cur_prev_diff = 0;
    cur_next_diff = 0;

    // cerco gli indici piu "vicini" al peer
    for(i = 0 ; i < NUM_NEIGHBORS ; i++){

        if( i == index ) continue;

        if(peer_list[i] != 0){

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
                cur_diff = (index > i) ? (index - i) : (i - index);

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

}
