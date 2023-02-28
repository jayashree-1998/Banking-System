#include "constants.h"
/*
TODO:
1. login id: generated as max no. + 1 (read from a file which stores the max login no.)
2. name
3. age
4. gender
5. account no.: genereated as max no. + 1 (read from a file which stores the max acc no.)
6. password (< K_MAX_SIZE)
7. is_logged_in: check this before letting them to login to system
*/

typedef struct customer {
    char gender;
    bool is_logged_in;
    int id;  // used for indexing 0, 1, 2 ...
    int age;
    long acc_no;
    char name[K_SIZE];
    char password[K_SIZE];
} customer;