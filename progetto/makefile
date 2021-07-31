all: ./ds ./peer 

./peer: ./peer.o ./util/connection.o 
	gcc -Wall -g ./peer.o ./util/connection.o -o ./peer

./ds: ./ds.o ./util/connection.o 
	gcc -Wall -g ./ds.o ./util/connection.o -o ./ds

./peer.o: ./peer.c ./util/connection.h ./util/costanti.h
	gcc -Wall -g -c ./peer.c -o ./peer.o

./ds.o: ./ds.c ./util/connection.h ./util/costanti.h
	gcc -Wall -g -c ./ds.c -o ./ds.o

./util/connection.o: ./util/connection.c ./util/connection.h ./util/costanti.h
	gcc -Wall -g -c ./util/connection.c -o ./util/connection.o

