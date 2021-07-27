gcc -c util/connection.c
gcc -c peer.c
gcc -c ds.c

gcc -Wall connection.o peer.o -o peer
gcc -Wall connection.o ds.o -o ds

gnome-terminal -x sh -c "./ds 4242; exec bash"
gnome-terminal -x sh -c "./peer 4444; exec bash"
