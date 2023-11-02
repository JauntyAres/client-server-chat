#include <stdio.h>
#include <string.h>	
#include <stdbool.h>
#include <math.h> 
#include <stdlib.h>

#define FRAME_DATASIZE 32

// Function to calculate parity bit
char calculateParity(char byte) {
    int ct = 0;
    for (int i = 0; i < 7; i++) {
        if (byte & (1 << i)) {
            ct++;
        }
    }
    // If count is even, return '1'; else, return '0' (odd parity)
    return (ct % 2 == 0) ? '1' : '0';
}

// Function to convert a string to binary with odd parity
char* text2bin(char* asciiInput) {
    size_t inputLength = strlen(asciiInput);
    char* bin = (char*)malloc((inputLength * 8) + 1); // 8 bits per character, plus null terminator
    int binIndex = 0;

    for (int i = 0; i < inputLength; i++) {
        char byte = asciiInput[i];
        char parityBit = calculateParity(byte);

        // Replace the 8th bit with the parity bit
        byte &= 0x7F; // Clear the 8th bit (make it '0' temporarily)
        byte |= (parityBit - '0') << 7; // Set the 8th bit to the parity bit

        // Convert the byte to binary and add it to the binary string
        for (int j = 7; j >= 0; j--) {
            bin[binIndex++] = ((byte >> j) & 1) + '0'; // Convert bit to '0' or '1'
        }
    }

    bin[binIndex] = '\0'; // Null-terminate the binary string
    return bin;
}

bool verifyParity(char* bits){
    int ct = 0;
    for (int i = 0; i < 8; i++) {
        if (bits[i] == '1') {
            ct++;
        }
    }
    bool is_odd_parity = (ct % 2 == 1); 
    return is_odd_parity;
}

// also removes odd-parity bit
char* bin2text(char* binaryInput){
    size_t binaryLength = strlen(binaryInput);
    if (binaryLength % 8 != 0) {
        fprintf(stderr, "Invalid binary string length\n");
        exit(1);
    }

    size_t noOfBytes = binaryLength / 8;
    char* text = calloc(noOfBytes + 1, sizeof(char));
    if (text == NULL) {
        perror("Memory allocation failed");
        exit(1);
    }

    bool isCorrupted = false; // Define isCorrupted here
    for (size_t j = 0; j < noOfBytes; j++) {
        char bits[9];
        strncpy(bits, binaryInput + (8 * j), 8);
        bits[8] = '\0';

        char ascii = verifyParity(bits) ? strtol(bits + 1, NULL, 2) : 177;
        strncat(text, &ascii, 1);

        if (!verifyParity(bits)) {
            isCorrupted = true;
        }
    }

    printf(isCorrupted ? "This frame data is Corrupted!\n" : "This frame data is Error-Free\n");
    return text;
}