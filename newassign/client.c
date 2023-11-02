#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
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

int main(int argc, char *argv[]) {
    char* longString = "abc\n";

    // Use the combined encoded data
    char* combinedData = divideEncodeCombine(longString);
    printf("Combined binary data: %s\n", combinedData);

    char* combinedTextData = divideDecodeCombine(combinedData);
    printf("Combined Text data: %s\n", combinedTextData);



    // Clean up memory allocated for combinedData
    free(combinedData);
    return 0;
}
