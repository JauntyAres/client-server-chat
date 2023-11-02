/*
 *Simple client to work with server.c program.
 *Host name and port used by server are to be
 *passed as arguments.
 *
 *To test: Open a terminal window.
 *At prompt ($ is my prompt symbol) you may
 *type the following as a test:
 *
 * $./client 127.0.0.1 54554
 *Please enter the message: Programming with sockets is fun!
 *I got your message
 * $
 *
 */
#include <stdio.h> 
#include <stdbool.h>
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netdb.h>
#include <sys/wait.h>
#include "encDec.h"


#define FRAME_DATASIZE 32
#define FRAME_PACKET_SIZE (FRAME_DATASIZE + 3) * 8
#define READ  0
#define WRITE 1

char* divideEncodeCombine(const char *input) {
    int length = strlen(input);
    int numChunks = (length + FRAME_DATASIZE - 1) / FRAME_DATASIZE; // Calculate the number of chunks needed

    // Allocate memory for the combined encoded data
    char* combinedData = (char*)malloc(1);
    combinedData[0] = '\0';

    // Divide the input string into chunks and encode each chunk
    for (int i = 0; i < numChunks; ++i) {
        int chunkSize = (i == numChunks - 1) ? length % FRAME_DATASIZE : FRAME_DATASIZE;
        char chunk[FRAME_DATASIZE + 1]; // +1 for null terminator

        // Copy the chunk of the input string into the array
        strncpy(chunk, input + i * FRAME_DATASIZE, chunkSize);
        chunk[chunkSize] = '\0'; // Null-terminate the chunk

        // Encode the chunk
        char* bindata = encodeData(chunk);

        // Combine the encoded chunk with the existing combined data
        combinedData = realloc(combinedData, strlen(combinedData) + strlen(bindata) + 1);
        strcat(combinedData, bindata);

        // Clean up memory allocated for bindata
        free(bindata);
    }

    return combinedData;
}


char* divideDecodeCombine(const char *combinedData) {
    int length = strlen(combinedData);
    int numChunks = (length + FRAME_PACKET_SIZE - 1) / FRAME_PACKET_SIZE; // Calculate the number of chunks needed

    // Allocate memory for the combined decoded data
    char* combinedTextData = (char*)malloc(1);
    combinedTextData[0] = '\0';

    // Divide the input binary data into chunks, decode each chunk, and then combine the decoded chunks
    for (int i = 0; i < numChunks; ++i) {
        int chunkSize = (i == numChunks - 1) ? length % FRAME_PACKET_SIZE : FRAME_PACKET_SIZE;
        char chunk[FRAME_PACKET_SIZE + 1]; // +1 for null terminator

        // Copy the chunk of the input binary data into the array
        strncpy(chunk, combinedData + i * FRAME_PACKET_SIZE, chunkSize);
        chunk[chunkSize] = '\0'; // Null-terminate the chunk

        // Decode the chunk
        char* textData = decodeData(chunk);

        // Combine the decoded chunk with the existing combined data
        combinedTextData = realloc(combinedTextData, strlen(combinedTextData) + strlen(textData) + 1);
        strcat(combinedTextData, textData);

        // Clean up memory allocated for textData
        free(textData);
    }

    return combinedTextData;
}

void error(const char * msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s hostname port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];

    // Get port number from command line arguments
    portno = atoi(argv[2]);

    // Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    // Get server information
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    // Get user input
    printf("Please enter the message: ");
    bzero(buffer, 256);
    fgets(buffer, 255, stdin);

    char* data2send = divideEncodeCombine(buffer);
    // Send data to the server
    printf("data is about to send :%s",data2send);
    n = write(sockfd, data2send, strlen(data2send));
    if (n < 0)
        error("ERROR writing to socket");

    // Receive data from the server
    bzero(buffer, 256);
    n = read(sockfd, buffer, 255);
    if (n < 0)
        error("ERROR reading from socket");

    // Display received message
    printf("receiving data");
    printf("%s\n", divideDecodeCombine(buffer));

    // Close the socket
    close(sockfd);

    return 0;
}