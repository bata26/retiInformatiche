gcc -c util/connection.c
gcc -c peer.c
gcc -Wall connection.o peer.o -o peer
./peer 4242