// Include the necessary header files
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <unistd.h>

#include "encDec.h"  // Include the encDec.h library

// Define some constants
#define MAX_CLIENTS \
  10  // Maximum number of clients that can be connected to the server
#define MAX_LENGTH \
  50000  // Maximum length of the messages that can be sent or received
#define PORT 8080  // Port number for the server to listen on

// Define the CRC-32 polynomial
#define CRC32_POLY 0x04C11DB7

// Define a structure to store the client information
typedef struct {
  int socket;         // Socket descriptor for the client
  char username[20];  // Username of the client
  int devices;        // Number of devices that the client is logged in from
} client_t;

// Declare a global array of clients
client_t* clients[MAX_CLIENTS];

// Declare a mutex to synchronize access to the clients array
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// Declare a function to compute the CRC checksum of a message
unsigned int crc(char* message, int length) {
  // Declare a variable to store the checksum
  unsigned int checksum = 0xFFFFFFFF;

  // Loop through all the bytes in the message
  for (int i = 0; i < length; i++) {
    // XOR the checksum with the current byte
    checksum ^= message[i];

    // Loop through all the bits in the current byte
    for (int j = 0; j < 8; j++) {
      // Check if the least significant bit is 1
      if (checksum & 1) {
        // Right shift the checksum by one bit and XOR it with the polynomial
        checksum = (checksum >> 1) ^ CRC32_POLY;
      } else {
        // Right shift the checksum by one bit
        checksum = checksum >> 1;
      }
    }
  }

  // Return the complement of the checksum
  return ~checksum;
}

// Declare a function to encode and decode a message using Hamming code
int hamming(char* message, int length, int mode) {
  // Declare a variable to store the status of the decoding
  int status = 0;

  // Declare a variable to store the number of parity bits needed
  int parity_bits = 0;

  // Loop until 2^parity_bits >= length + parity_bits + 1
  while ((1 << parity_bits) < length + parity_bits + 1) {
    // Increment the parity bits by one
    parity_bits++;
  }

  // Check if the mode is encoding or decoding
  if (mode == 0) {
    // Encoding mode

    // Declare a variable to store the encoded message length
    int encoded_length = length + parity_bits;

    // Allocate memory for the encoded message
    char* encoded_message = (char*)malloc(encoded_length);

    // Initialize the encoded message with zeros
    memset(encoded_message, 0, encoded_length);

    // Copy the message bits to the encoded message at positions that are not
    // powers of two
    int j = 0;
    for (int i = 1; i <= encoded_length; i++) {
      if ((i & (i - 1)) != 0) {
        encoded_message[i - 1] = message[j];
        j++;
      }
    }

    // Calculate the parity bits for the encoded message and set them at
    // positions that are powers of two
    for (int i = 0; i < parity_bits; i++) {
      // Declare a variable to store the parity value
      int parity = 0;

      // Loop through all the bits that are checked by the current parity bit
      for (int j = i + 1; j <= encoded_length; j++) {
        if ((j >> i) & 1) {
          // XOR the parity value with the current bit value
          parity ^= encoded_message[j - 1];
        }
      }

      // Set the parity bit at position 2^i in the encoded message
      encoded_message[(1 << i) - 1] = parity;
    }

    // Copy the encoded message back to the original message and free the memory
    // allocated for it
    memcpy(message, encoded_message, encoded_length);
    free(encoded_message);
  } else if (mode == 1) {
    // Decoding mode

    // Declare a variable to store the syndrome value
    int syndrome = 0;

    // Loop through all the parity bits in the message and calculate their
    // values using XOR operation
    for (int i = 0; i < parity_bits; i++) {
      // Declare a variable to store the parity value
      int parity = 0;

      // Loop through all the bits that are checked by the current parity bit
      for (int j = i + 1; j <= length; j++) {
        if ((j >> i) & 1) {
          // XOR the parity value with the current bit value
          parity ^= message[j - 1];
        }
      }

      // Set the corresponding bit in the syndrome value to the parity value
      syndrome |= (parity << i);
    }

    // Check if the syndrome value is zero or not
    if (syndrome == 0) {
      // No error detected
      status = 0;
    } else if ((syndrome & (syndrome - 1)) == 0) {
      // Single bit error detected and corrected
      status = 1;

      // Flip the bit at position syndrome in the message
      message[syndrome - 1] ^= 1;
    } else {
      // Multiple bit errors detected
      status = -1;
    }

    // Declare a variable to store the decoded message length
    int decoded_length = length - parity_bits;

    // Allocate memory for the decoded message
    char* decoded_message = (char*)malloc(decoded_length);

    // Copy the message bits from the original message at positions that are not
    // powers of two to the decoded message
    int j = 0;
    for (int i = 1; i <= length; i++) {
      if ((i & (i - 1)) != 0) {
        decoded_message[j] = message[i - 1];
        j++;
      }
    }

    // Copy the decoded message back to the original message and free the memory
    // allocated for it
    memcpy(message, decoded_message, decoded_length);
    free(decoded_message);
  }

  // Return the status of the decoding
  return status;
}

// Declare a function to handle the communication with a client
void* handle_client(void* arg) {
  // Get the client pointer from the argument
  client_t* client = (client_t*)arg;

  // Declare a buffer to store the messages
  char buffer[MAX_LENGTH];

  // Declare a variable to store the number of bytes received or sent
  int bytes;

  // Loop until the client disconnects or an error occurs
  while (1) {
    // Receive a message from the client
    bytes = recv(client->socket, buffer, MAX_LENGTH, 0);

    // Check if there was an error or the connection was closed
    if (bytes <= 0) {
      break;
    }

    // Decode the message using the encDec.h library
    char* decoded_message = decodeMessage(buffer);

    // Extract the values of the tags using the encDec.h library
    char* tag = getTag(decoded_message);
    char* from = getValue(decoded_message, "FROM");
    char* to = getValue(decoded_message, "TO");
    char* body = getValue(decoded_message, "BODY");

    // Free the memory allocated for the decoded message
    free(decoded_message);

    // Check if the tag is "REQUEST"
    if (strcmp(tag, "REQUEST") == 0) {
      // If the body of the request is "login"
      if (strcmp(body, "login") == 0) {
        // Send a success message to the client
        send(client->socket, "OK", 2, 0);
        // Add the client to the list of clients
        add_client(client->socket, from);
      }
      // If the body of the request is "logout"
      else if (strcmp(body, "logout") == 0) {
        // Remove the client from the list of clients
        remove_client(client->socket);
      } else {
        // Send an error message to the client
        send(client->socket, "Invalid request", 14, 0);
      }
    }
    // Check if the tag is "MSG"
    else if (strcmp(tag, "MSG") == 0) {
      // If the length of the body is even
      if (strlen(body) % 2 == 0) {
        // Apply Hamming code for error detection and correction
        int hamming_status = hamming(body, strlen(body), 1);
        hamming(body, strlen(body), 0);
        // If there's a transmission error
        if (hamming_status == -1) {
          printf("Hamming transmission error detected for message from %s\n",
                 from);
        }
        // If there's a single bit error that was corrected
        else if (hamming_status == 1) {
          printf("Hamming single bit error corrected for message from %s\n",
                 from);
        }
      } else {
        // Calculate checksum using CRC
        unsigned int checksum = crc(body, strlen(body) - sizeof(unsigned int));
        unsigned int received_checksum;
        memcpy(&received_checksum, body + strlen(body) - sizeof(unsigned int),
               sizeof(unsigned int));
        // If there's a transmission error detected by CRC
        if (checksum != received_checksum) {
          printf("CRC transmission error detected for message from %s\n", from);
        }
      }
      // Print the message from the sender
      body[strlen(body) - sizeof(unsigned int)] = '\0';
      printf("%s: %s\n", from, body);
      // Encode and send the message to the recipient
      char* encoded_message =
          encodeMessage(buffer, "MSG", "FROM", from, "TO", to, "BODY", body);
      pthread_mutex_lock(&clients_mutex);
      for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL && strcmp(clients[i]->username, to) == 0) {
          send(clients[i]->socket, encoded_message, strlen(encoded_message), 0);
          break;
        }
      }
      pthread_mutex_unlock(&clients_mutex);
      free(encoded_message);
    }
    // Check if the tag is "LOGIN"
    else if (strcmp(tag, "LOGIN") == 0) {
      // If the username is valid
      if (is_valid_username(from)) {
        send(client->socket, "OK", 2, 0);
        add_client(client->socket, from);
      } else {
        send(client->socket, "Invalid username", 15, 0);
      }
    }
    // Check if the tag is "LOGIN_LIST"
    else if (strcmp(tag, "LOGIN_LIST") == 0) {
      char* client_list = (char*)malloc(MAX_LENGTH);
      strcpy(client_list, "");
      pthread_mutex_lock(&clients_mutex);
      for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL) {
          strcat(client_list, clients[i]->username);
          strcat(client_list, ",");
        }
      }
      pthread_mutex_unlock(&clients_mutex);
      client_list[strlen(client_list) - 1] = '\0';
      char* encoded_client_list = encodeMessage(client_list, "LOGIN_LIST");
      send(client->socket, encoded_client_list, strlen(encoded_client_list), 0);
      free(client_list);
      free(encoded_client_list);
    }
    // Check if the tag is "INFO"
    else if (strcmp(tag, "INFO") == 0) {
      printf("%s\n", body);
    } else {
      send(client->socket, "Invalid tag", 10, 0);
    }
  }

  // Close the socket of the client
  close(client->socket);

  // Lock the clients mutex
  pthread_mutex_lock(&clients_mutex);

  // Loop through all the clients and find the one that disconnected
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i] == client) {
      // Decrement the number of devices for that client
      clients[i]->devices--;

      // Check if there are no more devices for that client
      if (clients[i]->devices == 0) {
        // Free the memory allocated for that client and set it to NULL in the
        // array
        free(clients[i]);
        clients[i] = NULL;
      }

      break;
    }
  }

  // Unlock the clients mutex
  pthread_mutex_unlock(&clients_mutex);

  // Return NULL
  return NULL;
}

// Declare a function to create a new client and add it to the clients array
void add_client(int socket, char* username) {
  // Allocate memory for a new client
  client_t* client = (client_t*)malloc(sizeof(client_t));

  // Initialize the client fields
  client->socket = socket;
  strcpy(client->username, username);
  client->devices = 1;

  // Lock the clients mutex
  pthread_mutex_lock(&clients_mutex);

  // Loop through all the clients and find an empty slot or an existing one with
  // the same username
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i] == NULL) {
      // Set the slot to the new client and break the loop
      clients[i] = client;
      break;
    } else if (strcmp(clients[i]->username, username) == 0) {
      // Increment the number of devices for the existing client and free the
      // new client
      clients[i]->devices++;
      free(client);
      break;
    }
  }

  // Unlock the clients mutex
  pthread_mutex_unlock(&clients_mutex);
}

// Declare a function to remove a client from the clients array and close its
// socket
void remove_client(int socket) {
  // Lock the clients mutex
  pthread_mutex_lock(&clients_mutex);

  // Loop through all the clients and find the one that matches the socket
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i] != NULL && clients[i]->socket == socket) {
      // Decrement the number of devices for that client
      clients[i]->devices--;

      // Check if there are no more devices for that client
      if (clients[i]->devices == 0) {
        // Free the memory allocated for that client and set it to NULL in the
        // array
        free(clients[i]);
        clients[i] = NULL;
      }

      break;
    }
  }

  // Unlock the clients mutex
  pthread_mutex_unlock(&clients_mutex);

  // Close the socket of the client
  close(socket);
}

// Declare a function to check if a username is valid
int is_valid_username(char* username) {
  // Check if the username is empty or too long
  if (username == NULL || strlen(username) != 8) {
    return 0;
  }

  // Check if the username contains only alphanumeric characters and underscores
  for (int i = 0; i < strlen(username); i++) {
    if (!isalnum(username[i]) && username[i] != '_') {
      return 0;
    }
  }

  // Return 1 if the username is valid
  return 1;
}

// Declare a function to create and bind a socket to a port
int create_server_socket(int port) {
  // Create a socket using TCP protocol
  int server_socket = socket(AF_INET, SOCK_STREAM, 0);

  // Check if there was an error in creating the socket
  if (server_socket == -1) {
    perror("socket");
    return -1;
  }

  // Set some socket options to reuse the address and port
  int opt = 1;
  if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt)) == -1) {
    perror("setsockopt");
    return -1;
  }

  // Create a sockaddr_in structure to store the server address and port
  struct sockaddr_in server_address;
  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(port);

  // Bind the socket to the server address and port
  if (bind(server_socket, (struct sockaddr*)&server_address,
           sizeof(server_address)) == -1) {
    perror("bind");
    return -1;
  }

  // Return the socket descriptor
  return server_socket;
}

// Declare a function to accept incoming connections from clients and create
// threads to handle them
void accept_connections(int server_socket) {
  // Listen for incoming connections on the socket with a backlog of 10
  if (listen(server_socket, 10) == -1) {
    perror("listen");
    exit(1);
  }

  // Print a message to indicate that the server is ready
  printf("Server is listening on port %d\n", PORT);

  // Declare a sockaddr_in structure to store the client address and port
  struct sockaddr_in client_address;

  // Declare a variable to store the size of the client address structure
  socklen_t client_address_size = sizeof(client_address);

  // Declare a variable to store the client socket descriptor
  int client_socket;

  // Declare a variable to store the thread identifier
  pthread_t thread_id;

  // Loop forever to accept connections from clients
  while (1) {
    // Accept a connection from a client and get its socket descriptor
    client_socket = accept(server_socket, (struct sockaddr*)&client_address,
                           &client_address_size);

    // Check if there was an error in accepting the connection
    if (client_socket == -1) {
      perror("accept");
      continue;
    }

    // Print a message to indicate that a new connection was accepted
    printf("New connection from %s:%d\n", inet_ntoa(client_address.sin_addr),
           ntohs(client_address.sin_port));

    // Create a thread to handle the communication with the client
    if (pthread_create(&thread_id, NULL, handle_client, (void*)clients) != 0) {
      perror("pthread_create");
      close(client_socket);
    }
  }
}

// The main function of the program
int main() {
  // Create and bind a socket to a port for the server
  int server_socket = create_server_socket(PORT);

  // Check if there was an error in creating or binding the socket
  if (server_socket == -1) {
    exit(1);
  }

  // Accept incoming connections from clients and create threads to handle them
  accept_connections(server_socket);

  // Return 0 to indicate success
  return 0;
}