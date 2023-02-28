/*
TODO:
1. account no.: we have already created this!
2. type: (0: single account, 1: joint account)
3. isActive: (1: active, 0: inactive(account deleted!))
4. balance: after updating
5. prev balance: before updating
6. transactionDetail[10]:
7. lastTransationtime: time of last transaction
*/
#include "constants.h"

typedef struct account {
    bool is_Active;
    bool acc_type;  // 0->single, 1->joint_account
    int acc_no;     // used for indexing 0, 1, 2 ...
    int customer_id[2];
    int transactionDetail[K_MAX_TRANSACTIONS];  // list of transaction id's
    long acc_balance;
    long acc_prev_balance;
    time_t last_transaction_time;
} account;
