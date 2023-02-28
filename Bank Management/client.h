#include "util.h"

void clientHandler(int descriptor, char *msg) {
    if (descriptor == -1) {
        perror(msg);
    }
}