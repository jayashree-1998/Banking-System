// bool userLogin(int, char *, char *);
// void userMainPage(int);

void performTransaction(int, int, int, bool);
bool userLogout(int, int);
int updateTransaction(transaction);
void updateTransactionDetail(int, account *);
void updatePassword(int, int);
void balanceEnquiry(int, int);
void viewDetails(int, int, int);
int userLogin(int descriptor, char *login, char *password) {
    int id = atoi(login);
    char readBuffer[K_MAX_SIZE], writeBuffer[K_MAX_SIZE];
    ssize_t messageSize;

    // first compare login id with the max number(in login file)
    int customer_id_fd = openFile(K_CUST_ID_CNT, O_RDWR);
    errorHandler(customer_id_fd, "Error, K_CUST_ID_CNT file opening!");

    // apply lock
    struct flock customer_id_lck;

    // clear the lock variable
    memset(&customer_id_lck, 0, sizeof(customer_id_lck));

    // set lock type and apply the same on the file
    customer_id_lck.l_type = F_RDLCK;
    customer_id_lck.l_whence = SEEK_SET;

    customer_id_lck.l_start = 0;  // start
    customer_id_lck.l_len = 0;    // end of file
    customer_id_lck.l_pid = getpid();

    // set lock on file
    int lck_val = fcntl(customer_id_fd, F_SETLK, &customer_id_lck);  // returns 0 on successful lock operation
    int acc_no = -1;
    if (lck_val == 0) {
        recordCount record;
        int messageSize = read(customer_id_fd, &record, sizeof(recordCount));
        errorHandler(customer_id_fd, "Error, K_CUST_ID_CNT file reading!");
        int max_customer_id = record.count;
        if (max_customer_id < id) {
            // TODO: dummy write and show the login, password again
            customer_id_lck.l_type = F_UNLCK;
            fcntl(customer_id_fd, F_SETLK, &customer_id_lck);
            // close file
            close(customer_id_fd);
            return -1;
        }
        LOG("id<max");
    } else {
        // TODO: skip message
        LOG("skip message 1");
        return -1;
    }

    customer_id_lck.l_type = F_UNLCK;
    fcntl(customer_id_fd, F_SETLK, &customer_id_lck);
    // close file
    close(customer_id_fd);

    // open customerRecords
    customer tempCustomer;
    int customer_fd = openFile(K_CUST_RCD, O_RDWR);
    errorHandler(customer_fd, "Error, K_CUST_RCD file opening!");

    // apply lock on customerRecords file
    // apply lock
    struct flock customer_lck;

    // clear the lock variable
    memset(&customer_lck, 0, sizeof(customer_lck));

    // set lock type and apply the same on the file
    customer_lck.l_type = F_WRLCK;
    customer_lck.l_whence = SEEK_SET;

    customer_lck.l_start = id * sizeof(customer);  // start
    customer_lck.l_len = sizeof(customer);         // end of file
    customer_lck.l_pid = getpid();

    // set lock on file
    lck_val = fcntl(customer_fd, F_SETLK, &customer_lck);  // returns 0 on successful lock operation
    if (lck_val == 0) {
        errorHandler(lseek(customer_fd, id * sizeof(customer), SEEK_SET), "Error, seek error in K_CUST_RCD file!");

        memset(&tempCustomer, 0, sizeof(customer));
        messageSize = read(customer_fd, &tempCustomer, sizeof(customer));
        errorHandler(messageSize, "Error, reading cust id count!");
        if (tempCustomer.is_logged_in) {
            // TODO: dummy write and show the login, password again
            customer_lck.l_type = F_UNLCK;
            fcntl(customer_fd, F_SETLK, &customer_lck);
            // close file
            close(customer_fd);
            return -1;
        }
        LOG("not logged in");
        acc_no = tempCustomer.acc_no;
    } else {
        // TODO: skip message
        LOG("skip message 2");
        return -1;
    }

    // open accountRecords
    account tempAccount;
    int account_fd = openFile(K_ACC_RCD, O_RDWR);
    errorHandler(account_fd, "Error, K_ACC_RCD file opening!");

    // apply lock on accountRecords file
    // apply lock
    struct flock account_lck;

    // clear the lock variable
    memset(&account_lck, 0, sizeof(account_lck));

    // set lock type and apply the same on the file
    account_lck.l_type = F_RDLCK;
    account_lck.l_whence = SEEK_SET;

    account_lck.l_start = acc_no * sizeof(account);  // start
    account_lck.l_len = sizeof(account);             // end of file
    account_lck.l_pid = getpid();

    // set lock on file
    lck_val = fcntl(account_fd, F_SETLK, &account_lck);  // returns 0 on successful lock operation
    if (lck_val == 0) {
        errorHandler(lseek(account_fd, acc_no * sizeof(account), SEEK_SET), "Error, seek error in K_ACC_RCD file!");

        errorHandler(read(account_fd, &tempAccount, sizeof(account)), "Error, K_ACC_RCD file reading!");

        if (!tempAccount.is_Active) {
            // TODO: dummy write and show the login, password again
            account_lck.l_type = F_UNLCK;
            fcntl(account_fd, F_SETLK, &account_lck);
            // close file
            close(account_fd);
            return -1;
        }
        LOG("active");
        if (strcmp(tempCustomer.password, password)) {
            // TODO: dummy write and show the login, password again
            account_lck.l_type = F_UNLCK;
            fcntl(account_fd, F_SETLK, &account_lck);
            // close file
            close(account_fd);
            return -1;
        }
        LOG("password matched!");
        // update is_logged_in
        tempCustomer.is_logged_in = true;

        errorHandler(lseek(customer_fd, id * sizeof(customer), SEEK_SET), "Error, seek error in K_CUST_RCD file!");

        errorHandler(write(customer_fd, &tempCustomer, sizeof(customer)), "Error, storing account to file!");
        LOG("all done!");
    } else {
        // TODO: skip message
        LOG("skip message 3");
        return -1;
    }

    // unlock customer id count file
    customer_id_lck.l_type = F_UNLCK;
    fcntl(customer_id_fd, F_SETLK, &customer_id_lck);
    // close file
    close(customer_id_fd);

    // unlock customer records file
    customer_lck.l_type = F_UNLCK;
    fcntl(customer_fd, F_SETLK, &customer_lck);
    // close file
    close(customer_fd);

    // unlock account records file
    account_lck.l_type = F_UNLCK;
    fcntl(account_fd, F_SETLK, &account_lck);
    // close file
    close(account_fd);
    return acc_no;
}

void userMainPage(int descriptor, int acc_no, int login_id) {
    char readBuffer[K_MAX_SIZE], writeBuffer[K_MAX_SIZE];
    ssize_t messageSize;
    while (1) {
        messageSize = write(descriptor, K_USER_PAGE, K_MAX_SIZE);
        adminHandler(messageSize, "Error, sending message");

        bzero(readBuffer, sizeof(readBuffer));
        int messageSize = read(descriptor, &readBuffer, K_MAX_SIZE);
        adminHandler(messageSize, "Error, reading login value of admin!");
        switch (atoi(readBuffer)) {
            case 1:
                performTransaction(descriptor, acc_no, login_id, DEPOSIT);
                LOG("deposit");
                break;
            case 2:
                performTransaction(descriptor, acc_no, login_id, WITHDRAW);
                LOG("withdraw");
                break;
            case 3:
                balanceEnquiry(descriptor, acc_no);
                LOG("balance enquiry");
                break;
            case 4:
                updatePassword(descriptor, login_id);
                LOG("password change");
                break;
            case 5:
                viewDetails(descriptor, acc_no, login_id);
                LOG("view details");
                break;
            case 6:
                if (userLogout(descriptor, login_id)) {
                    LOG("logout");
                    return;
                }
                // TODO: skip message: couldn't perform logout shows user main page
                break;
        }
    }
}

bool userLogout(int descriptor, int login_id) {
    // open customerRecords
    customer tempCustomer;
    ssize_t messageSize;

    int customer_fd = openFile(K_CUST_RCD, O_RDWR);
    errorHandler(customer_fd, "Error, K_CUST_RCD file opening!");

    // apply lock on customerRecords file
    // apply lock
    struct flock customer_lck;

    // clear the lock variable
    memset(&customer_lck, 0, sizeof(customer_lck));

    // set lock type and apply the same on the file
    customer_lck.l_type = F_WRLCK;
    customer_lck.l_whence = SEEK_SET;

    customer_lck.l_start = login_id * sizeof(customer);  // start
    customer_lck.l_len = sizeof(customer);               // end of file
    customer_lck.l_pid = getpid();

    // set lock on file
    int lck_val = fcntl(customer_fd, F_SETLKW, &customer_lck);  // returns 0 on successful lock operation
    if (lck_val == 0) {
        errorHandler(lseek(customer_fd, login_id * sizeof(customer), SEEK_SET), "Error, seek error in K_CUST_RCD file!");

        memset(&tempCustomer, 0, sizeof(customer));
        messageSize = read(customer_fd, &tempCustomer, sizeof(customer));
        errorHandler(messageSize, "Error, reading cust record count!");
        if (tempCustomer.is_logged_in) {
            LOG("logging out!");
            tempCustomer.is_logged_in = false;

            errorHandler(lseek(customer_fd, login_id * sizeof(customer), SEEK_SET), "Error, seek error in K_CUST_RCD file!");

            errorHandler(write(customer_fd, &tempCustomer, sizeof(customer)), "Error, storing account to file!");
        }
    } else {
        // TODO: skip message
        LOG("skip message: logging out");
        return false;
    }
    // unlock customer records file
    customer_lck.l_type = F_UNLCK;
    fcntl(customer_fd, F_SETLK, &customer_lck);
    // close file
    close(customer_fd);
    return true;
}

int updateTransaction(transaction t) {
    int messageSize;
    account newAccount;
    char readBuffer[K_MAX_SIZE];

    // open transaction_id_count file
    int transaction_fd = openFile(K_TRANS_ID_CNT, O_RDWR);
    errorHandler(transaction_fd, "Error, K_TRANS_ID_CNT file opening!");

    // apply lock
    struct flock lck;

    // clear the lock variable
    memset(&lck, 0, sizeof(lck));

    // set lock type and apply the same on the file
    lck.l_type = F_WRLCK;
    lck.l_whence = SEEK_SET;

    lck.l_start = 0;  // start
    lck.l_len = 0;    // end of file
    lck.l_pid = getpid();

    // set lock on file
    int lck_val = fcntl(transaction_fd, F_SETLKW, &lck);  // returns 0 on successful lock operation

    if (lck_val == 0) {
        recordCount record;
        messageSize = read(transaction_fd, &record, sizeof(recordCount));
        errorHandler(transaction_fd, "Error, K_TRANS_ID_CNT file reading!");
        if (messageSize == 0) {
            record.count = -1;
            printf("record count 1: %d", record.count);
        }
        t.id = record.count + 1;  // update id for new transaction

        // write updated id in customer id record  file
        errorHandler(lseek(transaction_fd, 0, SEEK_SET), "Error, seek error in K_TRANS_ID_CNT file!");

        record.count = t.id;
        errorHandler(write(transaction_fd, &record, sizeof(record)), "Error, storing recordCount to file!");
    } else {
        LOG("cannot acquire lock on transaction id count file");
        return -1;
    }
    // unlock transction id count file
    lck.l_type = F_UNLCK;
    fcntl(transaction_fd, F_SETLK, &lck);
    // close file
    close(transaction_fd);

    // add new transaction in transactionRecord file
    int fd = openFile(K_TRANS_RCD, O_RDWR);
    errorHandler(fd, "Error, opening K_TRANS_RCD file!");

    // clear the lock variable
    memset(&lck, 0, sizeof(lck));

    // set lock type and apply the same on the file
    lck.l_type = F_WRLCK;
    lck.l_whence = SEEK_SET;

    lck.l_start = 0;  // start
    lck.l_len = 0;    // end of file
    lck.l_pid = getpid();

    // set lock on file
    lck_val = fcntl(fd, F_SETLKW, &lck);  // returns 0 on successful lock operation

    if (lck_val == 0) {
        // seek to last
        errorHandler(lseek(fd, 0, SEEK_END), "Error, seek error!");

        // store the new account
        errorHandler(write(fd, &t, sizeof(transaction)), "Error, storing new account to file!");
    } else {
        LOG("cannot acquire lock on transaction record file");
        return -1;
    }
    // unlock transction record file
    lck.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lck);
    // close file
    close(fd);

    return t.id;
}

void performTransaction(int descriptor, int acc_no, int login_id, bool type) {
    char readBuffer[K_MAX_SIZE];
    ssize_t messageSize;
    int amount = 0;

    // write the amount prompt
    messageSize = write(descriptor, K_ENTER_AMT, sizeof(K_ENTER_AMT));
    errorHandler(messageSize, "Error, K_ENTER_AMT message");

    // read amount
    bzero(readBuffer, K_MAX_SIZE);
    messageSize = read(descriptor, &readBuffer, K_MAX_SIZE);
    errorHandler(messageSize, "Error, reading age type!");
    amount = atoi(readBuffer);

    account tempAccount;
    transaction tempTransaction;
    int fd = openFile(K_ACC_RCD, O_RDWR);
    errorHandler(fd, "Error, K_ACC_RCD file opening!");

    // apply lock on accountRecords file
    struct flock lck;

    // clear the lock variable
    memset(&lck, 0, sizeof(lck));

    // set lock type and apply the same on the file
    lck.l_type = F_WRLCK;
    lck.l_whence = SEEK_SET;

    lck.l_start = acc_no * sizeof(account);  // start
    lck.l_len = sizeof(account);             // end of file
    lck.l_pid = getpid();

    // set lock on file
    int lck_val = fcntl(fd, F_SETLKW, &lck);  // returns 0 on successful lock operation
    if (lck_val == 0) {
        errorHandler(lseek(fd, acc_no * sizeof(account), SEEK_SET), "Error, seek error in K_ACC_RCD file!");

        errorHandler(read(fd, &tempAccount, sizeof(account)), "Error, K_ACC_RCD file reading!");

        if (type == WITHDRAW) {
            if (tempAccount.acc_balance < amount) {
                LOG("acc balance insufficient!");
                // TODO: skip message: insufficent balance

                lck.l_type = F_UNLCK;
                fcntl(fd, F_SETLK, &lck);
                // close file
                close(fd);
                return;
            }
        }

        tempAccount.acc_prev_balance = tempAccount.acc_balance;

        if (type == WITHDRAW) {
            tempAccount.acc_balance = tempAccount.acc_balance - amount;
        } else {
            tempAccount.acc_balance = tempAccount.acc_balance + amount;
        }

        time(&tempAccount.last_transaction_time);
        struct tm *curr_time = localtime(&tempAccount.last_transaction_time);

        // debug
        printf("transaction time: %s", asctime(curr_time));

        tempTransaction.id = -1;  // this will updated after updateTransaction call
        tempTransaction.customer_id = login_id;
        tempTransaction.acc_no = acc_no;
        tempTransaction.type = type;  // set transaction type
        tempTransaction.amount = amount;
        tempTransaction.balance = tempAccount.acc_balance;
        tempTransaction.prev_balance = tempAccount.acc_prev_balance;
        tempTransaction.transaction_time = tempAccount.last_transaction_time;

        int transaction_id = updateTransaction(tempTransaction);

        updateTransactionDetail(transaction_id, &tempAccount);

        errorHandler(lseek(fd, acc_no * sizeof(account), SEEK_SET), "Error, seek error in K_ACC_RCD file!");

        errorHandler(write(fd, &tempAccount, sizeof(account)), "Error, storing account to file!");
    } else {
        LOG("Cannot perform lock on account records in performTransaction");
        return;
    }
    // unlock account records file
    lck.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lck);
    // close file
    close(fd);
}

void updateTransactionDetail(int transaction_id, account *tempAccount) {
    // update the last 5 transaction id's in user account
    for (int i = K_MAX_TRANSACTIONS - 2; i >= 0; i--) {
        tempAccount->transactionDetail[i + 1] = tempAccount->transactionDetail[i];
    }
    tempAccount->transactionDetail[0] = transaction_id;
    for (int i = 0; i < K_MAX_TRANSACTIONS; i++) {
        printf("transaction_id: %d", tempAccount->transactionDetail[i]);
    }
}

void updatePassword(int descriptor, int login_id) {
    char readBuffer[K_MAX_SIZE], writeBuffer[K_MAX_SIZE], tempBuffer[K_MAX_SIZE];
    char oldPassword[K_SIZE];
    char originalPassword[K_SIZE];

    customer tempCustomer;
    // write the amount prompt
    ssize_t messageSize;
    messageSize = write(descriptor, K_ENTER_OLD_PWD, sizeof(K_ENTER_OLD_PWD));
    errorHandler(messageSize, "Error, K_ENTER_OLD_PWD message");

    // read old password
    bzero(readBuffer, K_MAX_SIZE);
    messageSize = read(descriptor, &readBuffer, K_MAX_SIZE);
    errorHandler(messageSize, "Error, reading age type!");

    strcpy(oldPassword, readBuffer);

    // open customerRecords
    int fd = openFile(K_CUST_RCD, O_RDWR);
    errorHandler(fd, "Error, K_CUST_RCD file opening!");

    // apply lock on customerRecords file
    struct flock lck;

    // clear the lock variable
    memset(&lck, 0, sizeof(lck));

    // set lock type and apply the same on the file
    lck.l_type = F_RDLCK;
    lck.l_whence = SEEK_SET;

    lck.l_start = login_id * sizeof(customer);  // start
    lck.l_len = sizeof(customer);               // end of file
    lck.l_pid = getpid();

    // set lock on file
    int lck_val = fcntl(fd, F_SETLKW, &lck);  // returns 0 on successful lock operation
    if (lck_val == 0) {
        errorHandler(lseek(fd, login_id * sizeof(customer), SEEK_SET), "Error, seek error in K_CUST_RCD file!");

        memset(&tempCustomer, 0, sizeof(customer));
        messageSize = read(fd, &tempCustomer, sizeof(customer));
        errorHandler(messageSize, "Error, reading cust record!");
        strcpy(originalPassword, tempCustomer.password);
    } else {
        LOG("Cannot set lock on customer record file: updatePassword() 1");
        return;
    }
    lck.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lck);
    // close file
    close(fd);

    if (strcmp(oldPassword, originalPassword)) {
        bzero(tempBuffer, K_MAX_SIZE);
        strcpy(tempBuffer, K_ERROR_INCORRECT_PWD);
        strcat(tempBuffer, SKIP_MESSAGE);

        messageSize = write(descriptor, tempBuffer, sizeof(tempBuffer));
        adminHandler(messageSize, "Error, sending customer details");

        // skip read
        bzero(readBuffer, K_MAX_SIZE);
        messageSize = read(descriptor, &readBuffer, K_MAX_SIZE);
        adminHandler(messageSize, "Error, dummy read!");
        return;
    }

    char newPassword1[K_SIZE], newPassword2[K_SIZE];

    messageSize = write(descriptor, K_ENTER_NEW_PWD, sizeof(K_ENTER_NEW_PWD));
    errorHandler(messageSize, "Error, K_ENTER_NEW_PWD message");

    bzero(readBuffer, K_MAX_SIZE);
    messageSize = read(descriptor, &readBuffer, K_MAX_SIZE);
    errorHandler(messageSize, "Error, reading age type!");
    strcpy(newPassword1, readBuffer);

    messageSize = write(descriptor, K_CONFIRM_NEW_PWD, sizeof(K_CONFIRM_NEW_PWD));
    errorHandler(messageSize, "Error, K_CONFIRM_NEW_PWD message");

    bzero(readBuffer, K_MAX_SIZE);
    messageSize = read(descriptor, &readBuffer, K_MAX_SIZE);
    errorHandler(messageSize, "Error, reading age type!");
    strcpy(newPassword2, readBuffer);

    if (strcmp(newPassword1, newPassword2)) {
        bzero(tempBuffer, K_MAX_SIZE);
        strcpy(tempBuffer, K_ERROR_PWD_NOT_MATCHING);
        strcat(tempBuffer, SKIP_MESSAGE);

        messageSize = write(descriptor, tempBuffer, K_MAX_SIZE);
        adminHandler(messageSize, "Error, sending customer details");

        // skip read
        messageSize = read(descriptor, &readBuffer, K_MAX_SIZE);
        adminHandler(messageSize, "Error, dummy read!");
        bzero(readBuffer, K_MAX_SIZE);

        return;
    }
    // open customerRecords
    fd = openFile(K_CUST_RCD, O_RDWR);
    errorHandler(fd, "Error, K_CUST_RCD file opening!");

    struct flock lck2;

    // clear the lock variable
    memset(&lck2, 0, sizeof(lck2));

    // set lock type and apply the same on the file
    lck2.l_type = F_WRLCK;
    lck2.l_whence = SEEK_SET;

    lck2.l_start = login_id * sizeof(customer);  // start
    lck2.l_len = sizeof(customer);               // end of file
    lck2.l_pid = getpid();

    // set lock on file
    lck_val = fcntl(fd, F_SETLKW, &lck2);  // returns 0 on successful lock operation
    if (lck_val == 0) {
        errorHandler(lseek(fd, login_id * sizeof(customer), SEEK_SET), "Error, seek error in K_CUST_RCD file!");

        memset(&tempCustomer, 0, sizeof(customer));
        messageSize = read(fd, &tempCustomer, sizeof(customer));
        errorHandler(messageSize, "Error, reading customer record!");

        strcpy(tempCustomer.password, newPassword1);

        errorHandler(lseek(fd, login_id * sizeof(customer), SEEK_SET), "Error, seek error in K_CUST_RCD file!");

        errorHandler(write(fd, &tempCustomer, sizeof(customer)), "Error, storing new password to file!");
    } else {
        LOG("Cannot set lock on customer record file: updatePassword() 2");
        return;
    }
    lck2.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lck2);
    // close file
    close(fd);

    bzero(tempBuffer, K_MAX_SIZE);
    strcpy(tempBuffer, K_PWD_UPDATED);
    strcat(tempBuffer, SKIP_MESSAGE);

    messageSize = write(descriptor, tempBuffer, strlen(tempBuffer));
    adminHandler(messageSize, "Error, sending customer details");

    // skip read
    messageSize = read(descriptor, &readBuffer, K_MAX_SIZE);
    adminHandler(messageSize, "Error: dummy read!");
    bzero(readBuffer, K_MAX_SIZE);
}

void balanceEnquiry(int descriptor, int acc_no) {
    // open account record file
    account tempAccount;
    int balance = 0;

    int fd = openFile(K_ACC_RCD, O_RDWR);
    errorHandler(fd, "Error, K_ACC_RCD file opening!");

    // apply lock on accountRecords file
    struct flock lck;

    // clear the lock variable
    memset(&lck, 0, sizeof(lck));

    // set lock type and apply the same on the file
    lck.l_type = F_RDLCK;
    lck.l_whence = SEEK_SET;

    lck.l_start = acc_no * sizeof(account);  // start
    lck.l_len = sizeof(account);             // end of file
    lck.l_pid = getpid();

    // set lock on file
    int lck_val = fcntl(fd, F_SETLKW, &lck);  // returns 0 on successful lock operation
    if (lck_val == 0) {
        errorHandler(lseek(fd, acc_no * sizeof(account), SEEK_SET), "Error, seek error in K_ACC_RCD file!");

        errorHandler(read(fd, &tempAccount, sizeof(account)), "Error, K_ACC_RCD file reading!");

        balance = tempAccount.acc_balance;
    } else {
        // TODO: error acquiring lock reading balance
        return;
    }
    lck.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lck);
    // close file
    close(fd);

    // sending balance to client for display
    char readBuffer[K_MAX_SIZE], writeBuffer[K_MAX_SIZE], tempBuffer[K_MAX_SIZE];
    strcpy(writeBuffer, "");

    sprintf(tempBuffer, "Balance : %d\n", balance);
    strcat(writeBuffer, tempBuffer);
    // send dummy
    strcat(writeBuffer, SKIP_MESSAGE);

    int messageSize = write(descriptor, writeBuffer, K_MAX_SIZE);
    adminHandler(messageSize, "Error, sending balance");

    // skip read
    bzero(readBuffer, K_MAX_SIZE);
    messageSize = read(descriptor, &readBuffer, K_MAX_SIZE);
    adminHandler(messageSize, "Error, dummy read!");
}

void viewDetails(int descriptor, int login_id, int acc_no) {
    // open customer Records
    customer tempCustomer;
    ssize_t messageSize;
    char readBuffer[K_MAX_SIZE], writeBuffer[K_MAX_SIZE], tempBuffer[K_MAX_SIZE];
    strcpy(writeBuffer, "");

    // open file customerRecords
    int fd = openFile(K_CUST_RCD, O_RDWR);
    errorHandler(fd, "Error, K_CUST_RCD file opening!");

    struct flock lck;

    // clear the lock variable
    memset(&lck, 0, sizeof(lck));

    // set lock type and apply the same on the file
    lck.l_type = F_RDLCK;
    lck.l_whence = SEEK_SET;

    // calculating offset and len for acquing lock on particular record
    lck.l_start = login_id * sizeof(customer);  // start
    lck.l_len = sizeof(customer);               // end
    lck.l_pid = getpid();
    // set lock on file

    int lck_val = fcntl(fd, F_SETLKW, &lck);  // returns 0 on successful lock operation

    if (lck_val == 0) {
        // set offset to account index
        errorHandler(lseek(fd, lck.l_start, SEEK_SET), "Error, seek error in K_CUST_RCD file!");

        // read from file
        memset(&tempCustomer, 0, sizeof(customer));
        messageSize = read(fd, &tempCustomer, sizeof(customer));
        errorHandler(messageSize, "Error, reading customer record!");

    } else {
        LOG("viewDetails: cannot perform lock on customerRecord file");
        return;
    }
    // unlock the file
    lck.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lck);
    close(fd);

    // open file Account Records
    account tempAccount;
    fd = openFile(K_ACC_RCD, O_RDWR);
    errorHandler(fd, "Error, K_ACC_RCD file opening!");

    // clear the lock variable
    memset(&lck, 0, sizeof(lck));

    // set lock type and apply the same on the file
    lck.l_type = F_RDLCK;
    lck.l_whence = SEEK_SET;

    // calculating offset and len for acquing lock on particular record
    lck.l_start = acc_no * sizeof(account);  // start
    lck.l_len = sizeof(account);             // end
    lck.l_pid = getpid();
    // set lock on file

    lck_val = fcntl(fd, F_SETLKW, &lck);  // returns 0 on successful lock operation

    if (lck_val == 0) {
        // set offset to account index
        errorHandler(lseek(fd, lck.l_start, SEEK_SET), "Error, seek error in K_ACC_RCD file!");

        // read from file
        memset(&tempAccount, 0, sizeof(account));
        messageSize = read(fd, &tempAccount, sizeof(account));
        errorHandler(messageSize, "Error, reading cust id count!");
    } else {
        LOG("viewDetails: cannot perform lock on accountRecords file");
        return;
    }
    // unlock the file
    lck.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lck);
    close(fd);

    //

    // open file Account Records
    transaction tempTransaction;
    fd = openFile(K_TRANS_RCD, O_RDWR);
    errorHandler(fd, "Error, K_TRANS_RCD file opening!");

    // clear the lock variable
    memset(&lck, 0, sizeof(lck));

    // set lock type and apply the same on the file
    lck.l_type = F_RDLCK;
    lck.l_whence = SEEK_SET;
    lck.l_len = sizeof(transaction);  // end
    lck.l_pid = getpid();

    int t_id[5];
    transaction transactionRecords[5];

    for (int i = 0; i < K_MAX_TRANSACTIONS; i++) {
        t_id[i] = tempAccount.transactionDetail[i];
    }

    int i = 0;
    for (; i < K_MAX_TRANSACTIONS; i++) {
        if (t_id[i] == -1)
            break;
        else {
            lck_val = fcntl(fd, F_SETLKW, &lck);  // returns 0 on successful lock operation

            if (lck_val == 0) {
                // set offset to account index
                errorHandler(lseek(fd, t_id[i] * sizeof(transaction), SEEK_SET), "Error, seek error in K_ACC_RCD file!");

                // read from file
                memset(&tempTransaction, 0, sizeof(transaction));
                messageSize = read(fd, &tempTransaction, sizeof(transaction));
                errorHandler(messageSize, "Error, reading cust id count!");

                if (messageSize == 0) {
                    bzero(tempBuffer, K_MAX_SIZE);
                    strcpy(tempBuffer, "NO TRANSACTIONS!");
                    strcat(tempBuffer, SKIP_MESSAGE);

                    messageSize = write(descriptor, tempBuffer, strlen(tempBuffer));
                    adminHandler(messageSize, "Error, sending balance");

                    // skip read
                    bzero(readBuffer, K_MAX_SIZE);
                    messageSize = read(descriptor, &readBuffer, K_MAX_SIZE);
                    adminHandler(messageSize, "Error, dummy read!");

                    lck.l_type = F_UNLCK;
                    fcntl(fd, F_SETLK, &lck);
                    close(fd);
                    return;
                }

                transactionRecords[i] = tempTransaction;

            } else {
                LOG("viewDetails: cannot perform lock on transactionRecords file");
                return;
            }
            // unlock the file
            lck.l_type = F_UNLCK;
            fcntl(fd, F_SETLK, &lck);
        }
    }
    lck.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lck);
    close(fd);

    sprintf(tempBuffer, "Name\t\t\t: %s\n", tempCustomer.name);
    strcat(writeBuffer, tempBuffer);

    sprintf(tempBuffer, "Age\t\t\t: %d\n", tempCustomer.age);
    strcat(writeBuffer, tempBuffer);

    sprintf(tempBuffer, "Gender\t\t\t: %c\n", tempCustomer.gender);
    strcat(writeBuffer, tempBuffer);

    sprintf(tempBuffer, "Customer Id\t\t: %d\n", tempCustomer.id);
    strcat(writeBuffer, tempBuffer);

    sprintf(tempBuffer, "\nAccount details\nAcc no.\t\t\t: %d\n", tempAccount.acc_no);
    strcat(writeBuffer, tempBuffer);

    if (tempAccount.acc_type == 0) {
        sprintf(tempBuffer, "Acc type\t\t: Single Account\n");
        strcat(writeBuffer, tempBuffer);
        sprintf(tempBuffer, "Customer Id.\t\t: %d\n", tempAccount.customer_id[0]);
        strcat(writeBuffer, tempBuffer);
    } else {
        sprintf(tempBuffer, "Acc type\t\t\t: Joint Account\n");
        strcat(writeBuffer, tempBuffer);
        sprintf(tempBuffer, "Customer 1 Id.\t\t: %d\n", tempAccount.customer_id[0]);
        strcat(writeBuffer, tempBuffer);
        sprintf(tempBuffer, "Customer 2 Id.\t\t: %d\n", tempAccount.customer_id[1]);
        strcat(writeBuffer, tempBuffer);
    }

    sprintf(tempBuffer, "Account balance\t\t: %ld\n", tempAccount.acc_balance);
    strcat(writeBuffer, tempBuffer);

    sprintf(tempBuffer, "\nTransaction Details\n\n");
    strcat(writeBuffer, tempBuffer);

    for (int j = 0; j <= i - 1; j++) {
        if (transactionRecords[j].type) {
            sprintf(tempBuffer, "Transaction type\t: Withdraw\n");
            strcat(writeBuffer, tempBuffer);
        } else {
            sprintf(tempBuffer, "Transaction type\t: Deposit\n");
            strcat(writeBuffer, tempBuffer);
        }
        sprintf(tempBuffer, "Amount\t\t\t: %ld\n", transactionRecords[j].amount);
        strcat(writeBuffer, tempBuffer);
        sprintf(tempBuffer, "Previous Balance\t: %ld\n", transactionRecords[j].prev_balance);
        strcat(writeBuffer, tempBuffer);
        sprintf(tempBuffer, "Balance\t\t\t: %ld\n", transactionRecords[j].balance);
        strcat(writeBuffer, tempBuffer);
        sprintf(tempBuffer, "Transaction time\t: %s\n", asctime(localtime(&transactionRecords[j].transaction_time)));
        strcat(writeBuffer, tempBuffer);
    }
    strcat(writeBuffer, SKIP_MESSAGE);

    messageSize = write(descriptor, writeBuffer, strlen(writeBuffer));
    adminHandler(messageSize, "Error, sending balance");

    // skip read
    bzero(readBuffer, K_MAX_SIZE);
    messageSize = read(descriptor, &readBuffer, K_MAX_SIZE);
    adminHandler(messageSize, "Error, dummy read!");
}