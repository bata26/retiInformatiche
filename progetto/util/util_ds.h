void addPeer(int peer_port , int *peer_list);
void getNeighbors(int peer_port , int* neighbors , int* peer_list);
int getPeerIndex(int peer_port);
int checkIfUpdated(int * neighbors , int* new_neighbors);
void setupNeighbors(int neighbors[NUM_PEER][NUM_PEER]);