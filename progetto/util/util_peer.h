void cleanNeighbors(int * neighbors);
void stampaComandi(int port);
int checkDates(char data_iniziale[DATE_LEN] , char data_finale[DATE_LEN] , char tipo_aggr[AGGR_LEN]);
int checkSingleDate(char date[DATE_LEN]);
int isLeapYear(int year);
FILE * findFirstDate(char data_iniziale[DATE_LEN] , int my_port);
void calculateTotal(char data_iniziale[DATE_LEN] , char data_finale[DATE_LEN] , int my_port);
