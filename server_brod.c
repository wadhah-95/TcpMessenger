#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>  // library used to create and manage threads (allow the program to perform multiple tasks simultaneosly)
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 9098  //server listening port
#define BUFFER_SIZE 1024 //size of message buffer

int *client_fds = NULL;  //dynamic array holding all connected client socket descriptors
int client_count = 0;  //number of connecting clients

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;  //mutex to safely manage shared data between multiple threads

// Send message to all clients except the sender
void broadcast(int sender_fd, char *msg, int len) {
    pthread_mutex_lock(&lock);  //to ensure no two threads can modify clientt_fds at the same time
    for (int i = 0; i < client_count; i++) {  //loop through client_fds to send data to each client 
        if (client_fds[i] != sender_fd) {
            write(client_fds[i], msg, len);  //send message to each client
        }
    }
    pthread_mutex_unlock(&lock);  // unlocking mutex to allow another thread to modify client_fds
}

void remove_client(int fd) {  //removes disconnected client from client_fds
    pthread_mutex_lock(&lock);  //locking mutex

    int index = -1;
    for (int i = 0; i < client_count; i++) {
        if (client_fds[i] == fd) {
            index = i;
            break;
        }
    }

    if (index != -1) {
        for (int i = index; i < client_count - 1; i++)
            client_fds[i] = client_fds[i + 1];

        client_count--;
        if (client_count == 0) {
            free(client_fds);
            client_fds = NULL;
        } else {
            client_fds = realloc(client_fds, client_count * sizeof(int));
        }
    }

    pthread_mutex_unlock(&lock);
}

void *client_handler(void *arg) {  //function that will run a new thread for each client
    int client_fd = *(int *)arg;   //void *arg: a generic pointer that can point to any type=>arg is a way to pass data to the thread.
    free(arg);                     //pythread_create requires the thread function to have this signature

    char buffer[BUFFER_SIZE];

    while (1) {
        ssize_t bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);  //read() blocks untill client sends data

        if (bytes_read <= 0) {
            printf("Client disconnected.\n");
            remove_client(client_fd);
            close(client_fd);
            return NULL;
        }

        buffer[bytes_read] = '\0';

        printf("Received: %s\n", buffer);

        if (strcmp(buffer, "quit") == 0) {  // if client enters quit he is removed from client_fds and the socket is closed!
            printf("Client requested exit.\n");
            remove_client(client_fd);
            close(client_fd);
            return NULL;
        }

        broadcast(client_fd, buffer, bytes_read);  //if the client pressed smth other than quit then the message is broadcasted to all other clients!
    }
}

int main() {
    // creating TCP socket, binding it to all interfaces and listening
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);  //creating socket
    if (server_fd < 0) {
        perror("Socket");
        exit(1);
    }

    struct sockaddr_in server, client;
    server.sin_family = AF_INET;  
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {  //binding socket to server address
        perror("Bind");
        exit(2);
    }

    if (listen(server_fd, 3) < 0) {  //marking a bound socket as passive
        perror("Listen");
        exit(3);
    }

    printf("Server started on port %d...\n", PORT);

    while (1) {  //accepting clients to server in a loop and creating a new thread for each on
        socklen_t len = sizeof(client);
        int client_fd = accept(server_fd, (struct sockaddr *)&client, &len);

        if (client_fd < 0) {
            perror("Accept");
            continue;
        }

        printf("New client connected.\n");

        pthread_mutex_lock(&lock);  //locking mutex
        client_count++;
        client_fds = realloc(client_fds, client_count * sizeof(int));
        client_fds[client_count - 1] = client_fd;
        pthread_mutex_unlock(&lock);  //unlocking mutex

        int *new_fd = malloc(sizeof(int));  //allocating memory for an integer because client_handler takes void*arg as a parameter
        *new_fd = client_fd;

        pthread_t tid;  //tid a variable of type pythread_t to hold the thread id of the new thread
        pthread_create(&tid, NULL, client_handler, new_fd);
        pthread_detach(tid); // no need to join
    }

    close(server_fd);
    return 0;
}
