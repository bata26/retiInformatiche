// IP LOCALE
#define LOCALHOST "127.0.0.1"
// massima dimensione di caratteri dei comandi immessi da stdin 
#define MAX_STDIN_LEN 100
// massimo numero di caratteri per il comando stesso (get , start, etc..)
#define MAX_COMMAND_LEN 10
// lunghezza header
#define HEADER_LEN 9
//lunghezza pacchetti ricevuti tramite recvfrom
#define RECEIVED_LEN 9
// lunghezza ACK
#define ACK_LEN 9

// comandi client
#define CLIENT_GET_COMMAND "get"
#define CLIENT_START_COMMAND "start"

// numero client
#define NUM_PEER 5
// numero neighbors
#define NUM_NEIGHBORS 2
// "base" per il numero di porta dei peer
#define BASE_PORT 5000

// massima lunghezza lista neighbors
#define MAX_LIST_LEN 20