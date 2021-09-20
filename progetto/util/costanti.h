// IP LOCALE
#define LOCALHOST "127.0.0.1"
// massima dimensione di caratteri dei comandi immessi da stdin 
#define MAX_STDIN_LEN 100
// massimo numero di caratteri per il comando stesso (get , start, etc..)
#define MAX_COMMAND_LEN 30
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
// dimensione dei codici di scambio
#define MAX_COD_LEN 9
// lunghezza massima di un pacchetto
#define MAX_PKT_LEN 50


// numero di tipi
#define DATA_LEN 2
// indice per il tampoe
#define TAMPONE_IND 0
// indice per il caso
#define CASO_IND 1

// path per il file
#define FILE_PATH "./myfile/"
// lunghezza nome del file
#define FILE_LEN 50

// lunghezza buffer per VARIAZIONE/TOTALE
#define AGGR_LEN 10
// lunghezza stringa contenente la data
#define DATE_LEN 10
// data iniziale di operatività del sistema
#define START_DAY 1
#define START_MONTH 10
#define START_YEAR 2021