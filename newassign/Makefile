CC=gcc
LDFLAG1=-lm -L. lib_2layers.a
LDFLAG2=-L. lib_alllayers.a  

app:
	gcc -o client client.c application.c dataLink.c physical.c
	gcc -o server server.c application.c dataLink.c physical.c
	gcc -o testserver testserver.c application.c dataLink.c physical.c
	gcc -o testclient testclient.c application.c dataLink.c physical.c
