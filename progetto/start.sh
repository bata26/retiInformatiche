gcc peer.c ./util/connection.c -o peer
gcc ds.c ./util/connection.c -o ds

gnome-terminal -x sh -c "./ds 4242; exec bash"
gnome-terminal -x sh -c "./peer 4444; exec bash"
