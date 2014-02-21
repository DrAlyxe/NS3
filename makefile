all: server

server: server.c
	gcc -g -W -Wall server.c -o server