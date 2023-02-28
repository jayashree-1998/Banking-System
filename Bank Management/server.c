#include <arpa/inet.h>
#include <netinet/ip.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "admin.h"
#include "constants.h"
#include "message.h"
#include "user.h"

// TODO: while writing from server to client use strlen instead of K_MAX_SIZE
// FIXME: error when created a new account and then select option 6

int main() {
    int server_sd, bindStatus, listenStatus, client_sd;
    struct sockaddr_in bindAddress;
    ssize_t messageSize;
    char buffer[K_MAX_SIZE];

    // create a server socket
    server_sd = socket(AF_INET, SOCK_STREAM, 0);  // domain, type, protocol
    errorHandler(server_sd, "Error, socket creation");
    LOG("Socket successfully created!");

    // server address details
    bindAddress.sin_family = AF_INET;
    bindAddress.sin_port = htons(K_HOST_PORT);  // convert to big endian
    if (K_HOST_IP == INADDR_ANY) {
        bindAddress.sin_addr.s_addr = INADDR_ANY;
    } else {
        bindAddress.sin_addr.s_addr = inet_addr(K_HOST_IP);
    }

    // bind socket to address
    bindStatus = bind(server_sd, (struct sockaddr *)&bindAddress, sizeof(bindAddress));
    errorHandler(bindStatus, "Error, binding address!");
    LOG("Socket bounded to address!");

    // listen for connection request
    listenStatus = listen(server_sd, K_BACKLOG);
    errorHandler(listenStatus, "Error, in listening for connection request");
    LOG("Listening for connnection request!");

    struct sockaddr_in clientAddress;
    socklen_t clientSize = sizeof(clientAddress);
    while (1) {
        // accept connection request from ACCEPT queue
        client_sd = accept(server_sd, (struct sockaddr *)&clientAddress, &clientSize);
        errorHandler(client_sd, "Error in accepting connection request!");
        if (fork() == 0) {
            // child process
            // sendMessage(client_sd, K_USER_TYPE);

            start(client_sd, false);  // debug mode: true

            close(client_sd);
        } else {
            // parent process
            close(client_sd);
        }
    }
    close(server_sd);
    return 0;
}