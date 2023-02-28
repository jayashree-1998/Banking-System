#include "client.h"

#include <arpa/inet.h>
#include <netinet/ip.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "constants.h"
#include "message.h"

void interruptHandler() {
}

int main() {
    signal(SIGINT, (void *)interruptHandler);
    int connectStatus, client_sd;
    struct sockaddr_in serverAddress;
    ssize_t messageSize;

    char buffer[K_MAX_SIZE];

    // create a server socket
    client_sd = socket(AF_INET, SOCK_STREAM, 0);  // domain, type, protocol
    errorHandler(client_sd, "Error, socket creation");
    LOG("Socket successfully created!");

    // server address details
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(K_HOST_PORT);  // convert to big endian
    if (K_HOST_IP == INADDR_ANY) {
        serverAddress.sin_addr.s_addr = K_HOST_IP;
    } else {
        serverAddress.sin_addr.s_addr = inet_addr(K_HOST_IP);
    }

    // connect to server
    connectStatus = connect(client_sd, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    errorHandler(connectStatus, "Error, connecting to server!");
    LOG("Connected to server!");

    char readBuffer[K_MAX_SIZE], writeBuffer[K_MAX_SIZE], tempBuffer[K_MAX_SIZE];

    do {
        // write server message
        bzero(readBuffer, K_MAX_SIZE);
        messageSize = read(client_sd, readBuffer, K_MAX_SIZE);
        bzero(tempBuffer, K_MAX_SIZE);
        bzero(writeBuffer, K_MAX_SIZE);

        clientHandler(messageSize, "Error, reading message");

        if (strchr(readBuffer, SKIP_MESSAGE[0]) != NULL) {
            LOG("SKIP MESSAGE");
            // dummy write
            strncpy(tempBuffer, readBuffer, strlen(readBuffer) - strlen(SKIP_MESSAGE));
            printf("%s", tempBuffer);
            bzero(tempBuffer, K_MAX_SIZE);

            messageSize = write(client_sd, writeBuffer, K_MAX_SIZE);
            clientHandler(messageSize, "Error, sending dummy message");
            bzero(writeBuffer, K_MAX_SIZE);
        } else if (strchr(readBuffer, ERROR_MESSAGE[0]) != NULL) {
            LOG("ERROR MESSAGE");
            strncpy(tempBuffer, readBuffer, strlen(readBuffer) - strlen(ERROR_MESSAGE));
            printf("%s", tempBuffer);
            bzero(tempBuffer, K_MAX_SIZE);

            messageSize = write(client_sd, K_DUMMY, strlen(K_DUMMY));
            clientHandler(messageSize, "Error, sending dummy message");
            bzero(writeBuffer, K_MAX_SIZE);
            break;
        } else {
            bzero(writeBuffer, K_MAX_SIZE);
            if (strchr(readBuffer, HIDE_MESSAGE[0]) != NULL) {
                LOG("HIDE MESSAGE");
                strncpy(tempBuffer, readBuffer, strlen(readBuffer) - strlen(HIDE_MESSAGE));
                strcpy(writeBuffer, getpass(tempBuffer));
            } else {
                LOG("NORMAL MESSAGE");
                // read input from client
                printf("%s", readBuffer);
                scanf(" %[^\n]%*c", &writeBuffer);
            }
            messageSize = write(client_sd, writeBuffer, K_MAX_SIZE);
            clientHandler(messageSize, "Error, sending message");
            bzero(writeBuffer, K_MAX_SIZE);
        }
    } while (messageSize > 0);
    close(client_sd);
    return 0;
}