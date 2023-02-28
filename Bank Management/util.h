#include <stdio.h>
#include <unistd.h>
// to print error or status
void LOG(char *msg) {
    printf("LOG: %s\n", msg);
}

void errorHandler(int descriptor, char *msg) {
    if (descriptor == -1) {
        perror(msg);
        _exit(0);
    }
}