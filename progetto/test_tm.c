#include <time.h>
#include <stdio.h>


int main(){

    time_t t;
    struct tm* timeinfo;

    time(&t);
    timeinfo = localtime(&t);

    printf("TM:\nday->%s\nmon->%s\n" , timeinfo->tm_mday , timeinfo->tm_mon + 1);
}