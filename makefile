all:client server

client:
	gcc -o client client.c -Wall -lsqlite3
server:
	gcc -o server server.c -Wall -lsqlite3

.PHONY:clean
clean:
	rm server client
