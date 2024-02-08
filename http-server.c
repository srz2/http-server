#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>

#define PORT 3000
#define BUFFER_SIZE 1024

typedef int bool;
#define false 0
#define true 1

pthread_mutex_t lockIsRunning;
bool _IsRunning = true;
bool getIsRunning();
void setIsRunning(bool newValue);

void* handle_new_client(void * args);
void build_http_response(char * response, size_t * size);

void programTerminatedByUser();

typedef struct client_connection{
    int fd;
    struct sockaddr_in * info;
} client_connection;

int main()
{
    pthread_mutex_init(&lockIsRunning, NULL);
    signal(SIGINT, &programTerminatedByUser);

    int server_fd;
    struct sockaddr_in server_addr;

    // Create socket for server
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Configure the socket
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0 ){
        perror("Failed to set socket option: SO_REUSEADDR\n");
    }

    // Bind socket to port
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for new connections
    if (listen(server_fd, 10) < 0){
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is running, listening on port %d!!\n", PORT);
    
    // Wait for incoming connections
    printf("Waiting for connections...\n");
    while (1)
    {
        // Check if server is running
        // If not, break out of the listening loop
        if (!getIsRunning())
        {
            printf("Server marked not running\n");
            break;
        }

        // Client connection info
        struct sockaddr_in * client_addr = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
        socklen_t len;
        int client_fd = -1;
        
        if ((client_fd = accept(server_fd, (struct sockaddr *)client_addr, &len)) < 0) {
            perror("accepting new connection failed\n");
            continue;
        }

        printf("Got connection with fd: %d\n", client_fd);
        client_connection connectionInfo = {
            .fd = client_fd,
            .info = client_addr
        };

        // Create a thread to handle the new connection
        pthread_t thread;
        pthread_create(&thread, NULL, &handle_new_client, (void *) &connectionInfo);
        pthread_detach(thread);
    }

    printf("Closing down server...");
    close(server_fd);
    pthread_mutex_destroy(&lockIsRunning);
    printf("Server Disposed!\n");

    return 0;
}

void* handle_new_client(void * args)
{
    client_connection info = *(client_connection*)args;
    char ip_addr[INET_ADDRSTRLEN];

    // Get IP info
    inet_ntop(AF_INET, &(info.info->sin_addr), ip_addr, INET_ADDRSTRLEN);

    printf("Handling new client\n");
    printf("\tfd: %d\n", info.fd);
    printf("\tip: %s:%d\n", ip_addr, htons(info.info->sin_port));

    char *buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));
    ssize_t bytes_received = recv(info.fd, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0){
        perror("Received request of size zero");
        close(info.fd);
        free(buffer);
        return (void*)NULL;
    }

    printf("Recieved %ld bytes\n", bytes_received);
    printf("=================\n%s============\n", buffer);

    // Build HTTP response
    char *response = (char*)malloc(BUFFER_SIZE * sizeof(char));
    size_t responseLength;
    build_http_response(response, &responseLength);
    
    printf("Sending %ld bytes:\n%s\n", responseLength, response);

    // Send HTTP response
    ssize_t sent = send(info.fd, response, (size_t)responseLength, 0);
    printf("Sent %ld bytes\n", sent);

    close(info.fd);
    free(buffer);
    free(response);
}

void build_http_response(char * response, size_t * size)
{
    // Html Reponse
    char * html = ""
    "<!DOCTYPE html>\n"
    "<html>\n"
    "<head>\n"
    "<title>\n"
    "This is from Steven's Server"
    "</title>\n"
    "</head>\n"
    "<body>\n"
    "<h1>\n"
    "Steven's First Http Server!</h1>"
    "</body>\n"
    "</html>\n";

    // Build Http Header
    // char *header = (char *)malloc(BUFFER_SIZE * sizeof(char));
    snprintf((void *)response, BUFFER_SIZE,
             "HTTP/1.1 200 OK\n"
             "Content-Type: %s\n"
             "\n"
             "%s\n",
             "text/html", html );
    
    *size = strlen(response);
}

bool getIsRunning()
{
    bool ret = false;
    pthread_mutex_lock(&lockIsRunning);
    ret = _IsRunning;
    pthread_mutex_unlock(&lockIsRunning);
    return ret;
}

void setIsRunning(bool newValue)
{
    pthread_mutex_lock(&lockIsRunning);
    _IsRunning = newValue;
    pthread_mutex_unlock(&lockIsRunning);
}

void programTerminatedByUser()
{
    printf("User requested Termination\n");
    setIsRunning(false);
    exit(1);
}