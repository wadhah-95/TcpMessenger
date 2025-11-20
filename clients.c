////////////// first step: declaring libs we need //////////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 1024

int sock_fd;
char client_name[50];   // <-- Added this

// Thread to handle receiving messages from the server
void *receive_messages(void *arg) {
    char buffer[BUFFER_SIZE];
    while (1) {
        ssize_t bytes_received = read(sock_fd, buffer, BUFFER_SIZE - 1);
        if (bytes_received <= 0) {
            printf("Disconnected from server.\n");
            exit(0);
        }
        buffer[bytes_received] = '\0';

        // Print incoming message
        printf("\n%s\n", buffer);

        // Reprint prompt
        printf("%s> ", client_name);
        fflush(stdout);
    }
    return NULL;
}

int main(int argc, char *argv[]) {

    // ----------- ADD THIS PART -----------
    if (argc > 1) {
        strncpy(client_name, argv[1], sizeof(client_name)-1);
    } else {
        strcpy(client_name, "client");
    }
    // --------------------------------------

    char *server_ip = "127.0.0.1"; // localhost
    int server_port = 9098;        // must match the server's port

    // Create socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Socket creation failed");
        exit(-1);
    }

    // Setup server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    // Connect to server
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connecting failed");
        exit(-2);
    }

    printf("Connected to the server. Type 'quit' to exit.\n");

    // Create a thread for receiving messages
    pthread_t recv_thread;
    if (pthread_create(&recv_thread, NULL, receive_messages, NULL) != 0) {
        perror("Thread creation failed");
        exit(-3);
    }

    // Main thread handles sending messages
    char message[BUFFER_SIZE];
    while (1) {
        printf("%s> ", client_name);   // <-- custom prompt
        if (fgets(message, BUFFER_SIZE, stdin) != NULL) {
            message[strcspn(message, "\n")] = '\0'; // remove newline
            if (strcmp(message, "quit") == 0) break;

            write(sock_fd, message, strlen(message));
        }
    }

    close(sock_fd);
    return 0;
}
