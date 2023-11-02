#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>	
#include <ctype.h>
#include <string.h>	
#include <sys/types.h>	
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/time.h>

#include "encDec.h"

#define READ  0
#define WRITE 1

#define FRAME_DATASIZE 32
#define FRAME_PACKETTSIZE (FRAME_DATASIZE+3)*8    //extra 3 bytes for frame header and convert it into bits


int main(int argc, char *argv[]){
    int TIMEOUT_SECONDS = 1;
    //****** USER ARGUMENT HANDLING *************// 
    bool is_simulation = false;
    char *fileName;
    if (argc < 2) {
        printf("Usage: %s [-s] name\n", argv[0]);
        printf("  file_name: input .inpf filename only\n");
        printf("  sim file_name: for transmission error simulation\n");
        return 0;
    }
    else{
        if (strcmp(argv[1], "sim") == 0) {
            is_simulation = true;
            TIMEOUT_SECONDS = 10;
            printf("\nIn simulation mode, Consumer will gonna wait for 10 seconds for the Producer\n");
            if (argc == 3){
                fileName = argv[2];
            }else{
                printf("Filename is missing");
                return 0;
            }
        } else if (strcmp(argv[1], "-h") == 0){
            printf("Usage: %s [-s] name\n", argv[0]);
            printf("  file_name: input .inpf filename only\n");
            printf("  sim file_name: for transmission error simulation\n");
            return 0;;
        } else{
            fileName = argv[1];  
        }

        // Extracting only filename 
        char *extension = ".inpf";
        // Check if the fileName ends with the extension
        if (strstr(fileName, extension) != NULL) {
            // Replace the extension with an empty string
            *strstr(fileName, extension) = '\0';
        }
    }


    //****** INPUT FILE INITIALIZATION *************//
    char inpf[50] ="";
    sprintf(inpf,"%s",fileName);
    strcat(inpf,".inpf");
    FILE* input_file = fopen(inpf, "r");
    if (input_file == NULL) {
        printf("%s:File isn't available.\n",fileName);
        return 0;
    }
    else{
        printf("File loaded successfully!\n");
    }


    //****** SETTING UP PIPELINES FOR INTER-PROCESS COMMUNICATION *************//
    int Producer_Pipeline[2], Consumer_Pipeline[2]; // Owner is able to write in their pipeline
    pid_t pid;
    ssize_t bytes_read;
   
    char* Producer_Storage = calloc(FRAME_PACKETTSIZE, sizeof(char));  
    char* Consumer_Storage = calloc(FRAME_PACKETTSIZE, sizeof(char)); 
    
    // Use to identify the end of communication
    char* Prodtemp_storage = calloc(FRAME_PACKETTSIZE, sizeof(char));  
    char* Constemp_storage = calloc(FRAME_PACKETTSIZE, sizeof(char));  
    
    fflush(NULL);

    // initialize pipelines
    pipe(Producer_Pipeline);
    pipe(Consumer_Pipeline);

    // conversion from int to string in order to pass these pipelines to execl
    char STR_Producer_Pipeline[2];
    char STR_Consumer_Pipeline[2];
    sprintf(STR_Producer_Pipeline, "%d", Producer_Pipeline[WRITE]);
    sprintf(STR_Consumer_Pipeline, "%d", Consumer_Pipeline[WRITE]);


    // fork child process
    pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        //----- CONSUMER ----------------////
        
        // Consumer will only write to its pipeline and read from producer pipeline
        close(Producer_Pipeline[WRITE]); 
        close(Consumer_Pipeline[READ]);
        
        
        while (true) {
            // --- End communication when there's no data available in a 1 second.
            // in simulation mode it will wait for 10 seconds
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(Producer_Pipeline[READ], &read_fds);

            struct timeval timeout;
            timeout.tv_sec = TIMEOUT_SECONDS;
            timeout.tv_usec = 0;

            // Use select to wait for data with a timeout
            int ready = select(Producer_Pipeline[READ] + 1, &read_fds, NULL, NULL, &timeout);

            if (ready == -1) {
                perror("select");
                exit(EXIT_FAILURE);
            } else if (ready == 0) {
                // when no more data is available it will end the communication.
                break;
            } else {
                // If data is available then read it
                ssize_t bytesRead = read(Producer_Pipeline[READ], Consumer_Storage, FRAME_PACKETTSIZE);

                printf("\nCONSUMER > Data received from Producer : %s\n",Consumer_Storage);

                // ----- TRANSMISSION ERROR SIMULATION CODE -------//
                ssize_t read_user_bindata = -1;
                char* input = NULL;
                if (is_simulation == true) {
                    size_t inputLength = 0;
                    printf("\n\nCONSUMER > Send binary data(Uppercase version) to producer\n");
                    printf("Give binary input (Press Enter to keep default): ");

                    read_user_bindata = getline(&input, &inputLength, stdin);
                    if (read_user_bindata != -1 && input[0] != '\n') {
                        // remove newline character
                        if (input[read_user_bindata - 1] == '\n') {
                            input[read_user_bindata - 1] = '\0';
                        }
                        // allocate memory for data and copy input to it
                        write(Consumer_Pipeline[WRITE], strdup(input), strlen(strdup(input)));
                    }
                }
                //------- END -------//

                if(read_user_bindata == -1 || read_user_bindata == 1) {
                    pid_t exec_proc = fork();
                    if (exec_proc == 0) {
                        // Child process executes execl
                        execl("./application", "./application", "1",  fileName, Consumer_Storage,STR_Consumer_Pipeline,NULL);
                    }
                    wait(NULL);
                }
                
            }
        }
        
        close(Producer_Pipeline[READ]);
        close(Consumer_Pipeline[WRITE]);

    } else {

        //----- PRODUCER ----------------////
        // Producer will only write to its pipeline and read from Consumer pipeline
        close(Producer_Pipeline[READ]); 
        close(Consumer_Pipeline[WRITE]);

        //*** SEND input file data to CONSUMER 
        char* data = calloc(32,sizeof(char)); 
        int c;

        while(c != EOF){
            for (int i = 0; i < FRAME_DATASIZE; i++) {
                c = fgetc(input_file);
                if (c == EOF) {
                    break;
                }
                data[i] = c;
            }
            

             // ----- TRANSMISSION ERROR SIMULATION CODE -------//
            ssize_t read_user_bindata = -1;
            char* input = NULL;
            if (is_simulation == true) {
                size_t inputLength = 0;
                printf("\n\nPRODUCER> send binary data to consumer\n");
                printf("Give binary input (Press Enter to keep default): ");

                read_user_bindata = getline(&input, &inputLength, stdin);

                fflush(NULL);
                if (read_user_bindata != -1 && input[0] != '\n') {
                    // remove newline character

                    if (input[read_user_bindata - 1] == '\n') {
                        input[read_user_bindata - 1] = '\0';
                    }
                    // allocate memory for data and copy input to it
                    write(Producer_Pipeline[WRITE], strdup(input), strlen(strdup(input)));
                }else{
                    free(input);
                }
            }
            //------- END -------//

            if(read_user_bindata == -1 || read_user_bindata == 1) {
                // creating seperate child process to avoid replacing current Producer process by execl
                pid_t exec_proc = fork();
                if (exec_proc == 0) {
                    // Child process executes execl
                    execl("./application", "./application", "0",  fileName, data, STR_Producer_Pipeline,NULL);
                }
                wait(NULL);
            }

            memset(data, 0, FRAME_DATASIZE);
        }
        
        //*** RECIEVE binary data(Uppercase version) from CONSUMER 
        while(true)
        {

            read(Consumer_Pipeline[READ], Producer_Storage, FRAME_PACKETTSIZE);

            // To verify the end of data.
            if(strcmp(Prodtemp_storage, Producer_Storage) == 0){
                break;
            }
            else{
                strcpy(Prodtemp_storage, Producer_Storage);
            }


            printf("\nPRODUCER > Data received from Consumer : %s|\n",Producer_Storage);
            // creating seperate child process to avoid replacing current Producer process by execl
            pid_t exec_proc = fork();
            if (exec_proc == 0) {
                // Child process executes execl
                execl("./application", "./application", "2",  fileName, Producer_Storage,NULL);
            }
            wait(NULL);
            
        }
        wait(NULL); 

        close(Producer_Pipeline[WRITE]); // close write end of pipe
        close(Consumer_Pipeline[READ]);

    }

    return 0;
}
