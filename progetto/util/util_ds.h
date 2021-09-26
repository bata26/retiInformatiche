void addPeer(int peer_port , int *peer_list);
void getNeighbors(int peer_port , int* neighbors , int* peer_list);
int getPeerIndex(int peer_port);
int checkIfUpdated(int * neighbors , int* new_neighbors);
void setupNeighbors(int  neighbors[][NUM_NEIGHBORS]);
void updateNeighbors(int* peer_list , int  neighbors[][NUM_NEIGHBORS] , int * updated);
void setupNeighborsBuffer(char * buffer , int* len , char * msg , int  neighbors[][NUM_NEIGHBORS] , int index);
void cleanNeighbors(int sender_port , int  neighbors[][NUM_NEIGHBORS]);
