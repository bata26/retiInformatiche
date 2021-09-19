all: ./ds ./peer 

./peer: ./peer.o ./util/connection.o ./util/util_peer.o
	gcc -Wall -g ./peer.o ./util/connection.o ./util/util_peer.o -o ./peer

./ds: ./ds.o ./util/connection.o ./util/util_ds.o
	gcc -Wall -g ./ds.o ./util/connection.o ./util/util_ds.o -o ./ds

./peer.o: ./peer.c ./util/connection.h ./util/costanti.h
	gcc -Wall -g -c ./peer.c -o ./peer.o

./ds.o: ./ds.c ./util/connection.h ./util/costanti.h ./util/util_ds.h
	gcc -Wall -g -c ./ds.c -o ./ds.o

./util/connection.o: ./util/connection.c ./util/connection.h ./util/costanti.h
	gcc -Wall -g -c ./util/connection.c -o ./util/connection.o

./util/util_ds.o: ./util/util_ds.c ./util/util_ds.h ./util/costanti.h
	gcc -Wall -g -c ./util/util_ds.c -o ./util/util_ds.o

./util/util_peer.o: ./util/util_peer.c ./util/util_peer.h ./util/costanti.h
	gcc -Wall -g -c ./util/util_peer.c -o ./util/util_peer.o

clean:
	-rm ./*.o ./ds ./peer  ./util/*.o