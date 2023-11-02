#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>	
#include <ctype.h>
#include <math.h> 
#include <unistd.h>
#include "encDec.h"
#include <sys/socket.h>



#define FRAME_DATASIZE 32

char* encodeData(char* textdata){
    
    char* bindata = malloc(sizeof (char) * (32 + 3)*8); // 3 for frame data and 8 for bits;
    bindata = text2bin(frame(textdata));   
    
    return bindata;
}
char* decodeData(char* bindata){
    
    char* textdata = calloc(32, sizeof (char)); 
    textdata = deframe(bin2text(bindata));  

    return textdata;
}

void toUpper(char* fileName,char* bindata, int socketSend){
    // This function recieves data in binary, Converts to uppercase
    // and writes to the consumer pipeline, to be read by the producer.
    
    char outf[50] ="";
    sprintf(outf,"%s",fileName);
    strcat(outf,".outf");
    
    FILE* output_file;
    if (access(outf, F_OK) == -1) {
        output_file = fopen(outf, "w");
    }
    else {
        output_file = fopen(outf, "a");
    }
    
    char* textdata = calloc(32, sizeof (char)); 
    textdata = deframe(bin2text(bindata));  
    for (int i = 0; i < strlen(textdata); i++)
    {
        textdata[i] = toupper(textdata[i]); 
    } 
    printf("\nCONSUMER > Uppercase Service Running\n");
    fprintf(output_file, "%s", textdata);
    
    
    char chck[50] ="";
    sprintf(chck,"%s",fileName);
    strcat(chck,".chck");

    FILE* check_file;
    if (access(chck, F_OK) == -1) {
        check_file = fopen(chck, "w");
    }
    else {
        check_file = fopen(chck, "a");
    }

    char* modified_bindata = calloc(FRAME_DATASIZE ,sizeof (char)); 
    modified_bindata = text2bin(frame(textdata));
    fprintf(check_file, "%s", modified_bindata);

    send(socketSend, modified_bindata, strlen(modified_bindata), 0);

    fclose(output_file);
    fclose(check_file);

}
void done(char* fileName, char* result){
    char done[50] ="";
    sprintf(done,"%s",fileName);
    strcat(done,".done");

    FILE* done_file;
    if (access(done, F_OK) == -1) {
        done_file = fopen(done, "w");
    }
    else {
        // File exists, open in "append" mode
        done_file = fopen(done, "a");
    }

    fprintf(done_file, "%s", deframe(bin2text(result)));
    fclose(done_file);
}

// int main(int argc, char *argv[]) {

//     if (atoi(argv[1]) == 0) {
//         encodeData(argv[2], atoi(argv[3]));
//     } else if (atoi(argv[1]) == 1) {
//         decodeData(argv[2], atoi(argv[3]));
//     }
//     return 0;
// }