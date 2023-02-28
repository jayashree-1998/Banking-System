#include <stdbool.h>

#include "constants.h"
#include "message.h"

void compareLoginPassword(int, char, char *, char *);
void errorHandler(int, char *);
void LOG(char *);
bool adminLogin();
int userLogin(int, char *, char *);
void adminMainPage(int);
void userMainPage(int, int, int);
void loginHandler(int, char, bool);
void start(int, bool);

void start(int descriptor, bool debug) {
    ssize_t messageSize;
    char choice[K_MAX_SIZE], tempBuffer[K_MAX_SIZE];
    while (1) {
        // login message to client
        messageSize = write(descriptor, K_USER_TYPE, sizeof(K_USER_TYPE));
        errorHandler(messageSize, "Error, sending message");
        char buffer[K_MAX_SIZE];

        // read input 1 for admin and 2 for user 3 for exit
        bzero(buffer, sizeof(buffer));
        messageSize = read(descriptor, buffer, K_MAX_SIZE);
        errorHandler(messageSize, "Error, reading message");
        if (buffer[0] == K_ADMIN || buffer[0] == K_NUSER) {
            loginHandler(descriptor, buffer[0], debug);
        } else if (buffer[0] == '3') {
            // TODO: send error message
            bzero(tempBuffer, K_MAX_SIZE);
            strcpy(tempBuffer, K_EXIT);
            strcat(tempBuffer, ERROR_MESSAGE);

            messageSize = write(descriptor, tempBuffer, strlen(tempBuffer));
            errorHandler(messageSize, "Error, sending balance");

            // skip read
            bzero(tempBuffer, K_MAX_SIZE);
            messageSize = read(descriptor, &tempBuffer, K_MAX_SIZE);
            errorHandler(messageSize, "Error, dummy read!");
            printf("client: %d exited!\n", descriptor);
            return;
        } else {
            // send skip message

            bzero(tempBuffer, K_MAX_SIZE);
            strcpy(tempBuffer, K_INCORRECT_OPT);
            strcat(tempBuffer, SKIP_MESSAGE);

            messageSize = write(descriptor, tempBuffer, strlen(tempBuffer));
            errorHandler(messageSize, "Error, sending balance");

            // skip read
            bzero(tempBuffer, K_MAX_SIZE);
            messageSize = read(descriptor, &tempBuffer, K_MAX_SIZE);
            errorHandler(messageSize, "Error, dummy read!");
        }
    }
}

void loginHandler(int descriptor, char choice, bool debug) {
    if (debug && choice == K_ADMIN) {
        adminMainPage(descriptor);
        return;
    }

    ssize_t messageSize;
    char login[K_MAX_SIZE];
    char password[K_MAX_SIZE];
    char writeBuffer[K_MAX_SIZE];
    strcpy(writeBuffer, "");

    while (1) {
        // enter login id:
        messageSize = write(descriptor, K_ENTER_LOGIN, sizeof(K_ENTER_LOGIN));
        errorHandler(messageSize, "Error, sending message");

        // read login id
        messageSize = read(descriptor, login, K_MAX_SIZE);
        errorHandler(messageSize, "Error, reading message");

        // enter password:

        bzero(writeBuffer, K_MAX_SIZE);
        strcpy(writeBuffer, K_ENTER_PWD);
        strcat(writeBuffer, HIDE_MESSAGE);
        messageSize = write(descriptor, writeBuffer, sizeof(writeBuffer));
        errorHandler(messageSize, "Error, K_ENTER_PWD message");

        // read password
        messageSize = read(descriptor, password, K_MAX_SIZE);
        errorHandler(messageSize, "Error, reading message");

        // log login and password
        LOG(login);
        LOG(password);

        return compareLoginPassword(descriptor, choice, login, password);
    }
}

void compareLoginPassword(int descriptor, char choice, char *loginid, char *password) {
    if (choice == K_ADMIN) {
        // admin login
        if (!strcmp(loginid, K_ADMIN_ID) && !strcmp(password, K_ADMIN_PASSWORD)) {
            // check admin logged_in
            if (adminLogin()) {
                LOG("logged in!");
                // logged in
                adminMainPage(descriptor);
                return;
            }
            // TODO: skip message: already login
            return;
        }
    } else if (choice == K_NUSER) {
        int acc_no = userLogin(descriptor, loginid, password);
        if (acc_no > -1) {
            LOG("User Logged In!");
            userMainPage(descriptor, acc_no, atoi(loginid));
            return;
        }
        // TODO: skip message: already login
        return;
    }
    return;
}
