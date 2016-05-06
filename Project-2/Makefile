client.o: client.c server
	  gcc -o client client.c -lrt

server: server.c
	gcc -o server server.c -lrt

clean: 
	rm -f server client
