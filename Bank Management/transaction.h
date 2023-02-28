/*
TODO:
1. id
2. acc no
3. balance
4. prev balance
5. time
6. type of transaction
7. amount
*/
typedef struct transaction {
    bool type;        // 1->withdraw, 0->Deposit
    int id;           // used for indexing 0, 1, 2 ...
    int customer_id;  // done by whom
    int acc_no;
    long balance;
    long prev_balance;
    time_t transaction_time;
    long amount;
} transaction;