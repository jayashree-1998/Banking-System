#include <stdio.h>
#include <string.h>
int main() {
    char temp[10];
    char w[10];
    sprintf(temp, "%dh\n", 10);
    strcat(w, temp);
    printf("%s", temp);
    sprintf(temp, "%dt\n", 11);
    strcat(w, temp);
    printf("%s", temp);
    printf("%s", w);
    return 0;
}