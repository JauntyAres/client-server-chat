#include <stdio.h>
#include <string.h>	
#include <stdlib.h>


// frame data max bytes 32
char* frame(char* data){      
    int dataLength = strlen(data);   
    if(dataLength <= 32){
        // adding extra 3 bytes for frame header
        char* frame = calloc(dataLength + 3,sizeof (char));      

        // frame header
        frame[0] = (char)22;
        frame[1] = (char)22;
        frame[2] = (char)dataLength;  

        // frame data
        memcpy(&frame[3], data, dataLength);

        return(frame);
    }
}

char* deframe(char* frame){
    int frame_len = strlen(frame);
    if(frame_len >= 3) {
        // check SYN characters
        if(frame[0] == 22 && frame[1] == 22) {
            int dataLength = frame[2];
            if(frame_len >= dataLength + 3) {
                // extract data from frame
                char data[dataLength + 1];

                memcpy(data, &frame[3], dataLength);

                data[dataLength] = '\0';

                return strdup(data); 
            }
            else{
                printf("\nFRAME CORRUPTED > Data length is less than expected.\n");
                return NULL;
            }
        }
        else{
            printf("\nFRAME CORRUPTED > SYN characters missing.\n");
            return NULL;
        }
    }
    else {
        printf("Invalid frame! Frame length is less than expected.\n");
        return NULL;
    }
}