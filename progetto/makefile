all: ./ds ./peer ./manager

./peer: ./peer.o ./util/connection.o ./util/util_peer.o ./util/data.o
	gcc -Wall -g ./peer.o ./util/connection.o ./util/util_peer.o ./util/data.o -o ./peer

./ds: ./ds.o ./util/connection.o ./util/util_ds.o
	gcc -Wall -g ./ds.o ./util/connection.o ./util/util_ds.o -o ./ds

#./manager: ./manager.o ./util/connection.o ./util/util_ds.o ./util/data.o
#	gcc -Wall -g ./manager.o ./util/connection.o ./util/util_ds.o ./util/data.o  -o ./manager

# con manager
./manager: ./manager.o ./util/connection.o ./util/util_ds.o ./util/data.o 
	gcc -Wall -g ./manager.o ./util/connection.o ./util/util_ds.o ./util/data.o -o ./manager

./peer.o: ./peer.c ./util/connection.h ./util/costanti.h ./util/data.h
	gcc -Wall -g -c ./peer.c -o ./peer.o

./ds.o: ./ds.c ./util/connection.h ./util/costanti.h ./util/util_ds.h
	gcc -Wall -g -c ./ds.c -o ./ds.o

./manager.o: ./manager.c ./util/connection.h ./util/costanti.h ./util/util_ds.h ./util/data.h ./util/util_manager.h
	gcc -Wall -g -c ./manager.c -o ./manager.o

#./util/util_manager.o: ./util/util_manager.c ./util/util_manager.h ./util/costanti.h ./util/data.h ./util/connection.h
#	gcc -Wall -g -c ./util/util_manager.c -o ./util/util_manager.o

./util/connection.o: ./util/connection.c ./util/connection.h ./util/costanti.h
	gcc -Wall -g -c ./util/connection.c -o ./util/connection.o

./util/util_ds.o: ./util/util_ds.c ./util/util_ds.h ./util/costanti.h
	gcc -Wall -g -c ./util/util_ds.c -o ./util/util_ds.o

./util/util_peer.o: ./util/util_peer.c ./util/util_peer.h ./util/costanti.h ./util/data.h
	gcc -Wall -g -c ./util/util_peer.c -o ./util/util_peer.o

./util/data.o: ./util/data.c ./util/data.h ./util/costanti.h 
	gcc -Wall -g -c ./util/data.c -o ./util/data.o

clean:
	-rm ./*.o ./ds ./peer ./manager  ./util/*.o