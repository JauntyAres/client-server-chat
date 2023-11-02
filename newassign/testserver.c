#include <arpa/inet.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
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

void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, "Run client by providing host and port\n");
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    listen(sockfd, 5);

    while (1) {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");

        pid_t pid = fork();

        if (pid < 0) {
            error("ERROR on fork");
        }

        if (pid == 0) { // Child process
            close(sockfd); // Close the listening socket in the child process

            bzero(buffer, 256);
            n = read(newsockfd, buffer, 255);
            
            if (n < 0)
                error("ERROR reading from socket");

            printf("Here is the message: %s,size:%ld\n", buffer,strlen(buffer));

            char* data2send =  divideEncodeCombine("I got your message");
            n = write(newsockfd, data2send, strlen(data2send));
            if (n < 0)
                error("ERROR writing to socket");

            close(newsockfd);
            exit(EXIT_SUCCESS); // Exit the child process
        } else { // Parent process
            close(newsockfd); // Close the new socket in the parent process
        }
    }

    close(sockfd);
    return 0;
}
