#include <fcntl.h>
#include <stdlib.h>

#include "account.h"
#include "common.h"
#include "customer.h"
#include "recordCount.h"
#include "transaction.h"
#include "util.h"

void adminHandler(int, char *);
void addNewAccount(int);
int createNewCustomer(int, int);
void deleteAccount(int);
void getAllBankAccountDetails(int);
void getCustomerAccountDetails(int);
void getAllTransactionDetails(int);
void modifyAccount(int);

// check admin login status from file "is_logged_in"
// TODO: make a global path w.r.t SS Mini Project using a function which would concatinate path with filename
int openFile(char *filepath, mode_t mode) {
    return open(filepath, mode);
}

int closeLoginFile(int fd) {
    return close(fd);
}

bool checkLogin() {
    int fd = openFile(K_LOGIN_FILE, O_RDWR);
    adminHandler(fd, "Error, opening login file of admin!");

    char login_value;

    int messageSize = read(fd, &login_value, sizeof(login_value));
    errorHandler(messageSize, "Error, reading login value of admin!");
    closeLoginFile(fd);
    printf("admin login_value: %d\n", login_value);
    if (login_value == K_LOGGED_OUT || login_value == '\0') {
        return false;  // if not logged in
    } else {
        return true;  // if logged in
    }
}
void adminLogout() {
    if (checkLogin()) {
        LOG("logging out!");
        int fd = openFile(K_LOGIN_FILE, O_RDWR);
        errorHandler(fd, "Error, adminLogout()!");
        char message = K_LOGGED_OUT;
        int messageSize = write(fd, &message, sizeof(message));
        if (messageSize == -1) {
            LOG("Cannot log out!");
        } else {
            return;
        }
    }
}

bool adminLogin() {
    if (!checkLogin()) {
        int fd = openFile(K_LOGIN_FILE, O_RDWR);
        errorHandler(fd, "adminLogin()");
        char message = K_LOGGED_IN;
        int messageSize = write(fd, &message, sizeof(message));
        adminHandler(messageSize, "Error, writing login value of admin!");
        return true;
    }
    return false;  // already logged in
}

void adminHandler(int descriptor, char *msg) {
    if (descriptor == -1) {
        perror(msg);
        adminLogout();
        _exit(0);
    }
}

void adminMainPage(int descriptor) {
    char readBuffer[K_MAX_SIZE], writeBuffer[K_MAX_SIZE];
    ssize_t messageSize;

    while (1) {
        bzero(writeBuffer, K_MAX_SIZE);
        messageSize = write(descriptor, K_ADMIN_PAGE, strlen(K_ADMIN_PAGE));
        adminHandler(messageSize, "Error, sending message");

        bzero(readBuffer, sizeof(readBuffer));
        messageSize = read(descriptor, readBuffer, K_MAX_SIZE);
        adminHandler(messageSize, "Error, reading login value of admin!");
        printf("data: %s\n", readBuffer);
        printf("admin option: %c\n", readBuffer[0]);
        switch (atoi(readBuffer)) {
            case 1:
                // get all transactions from transaction file (for debuging)
                getAllTransactionDetails(descriptor);
                break;
            case 2:
                // get all bank account details (for debuging)
                getAllBankAccountDetails(descriptor);
                break;
            case 3:
                addNewAccount(descriptor);
                break;
            case 4:
                deleteAccount(descriptor);
                break;
            case 5:
                modifyAccount(descriptor);
                break;
            case 6:
                getCustomerAccountDetails(descriptor);
                break;
            case 7:
                adminLogout();
                return;
        }
    }
}

void addNewAccount(int descriptor) {
    int messageSize;
    account newAccount;
    // create new account
    // send one by one required things to user and get the detail from it and store it in the object of the account
    int fd = openFile(K_ACC_ID_CNT, O_RDWR);
    errorHandler(fd, "Error, K_ACC_ID_CNT file opening!");
    char readBuffer[K_MAX_SIZE];

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
    int lck_val = fcntl(fd, F_SETLKW, &lck);  // returns 0 on successful lock operation

    if (lck_val == 0) {
        recordCount record;
        messageSize = read(fd, &record, sizeof(recordCount));
        errorHandler(fd, "Error, K_ACC_ID_CNT file reading!");

        if (messageSize == 0) {
            record.count = -1;
            printf("record count 1: %d", record.count);
        }

        newAccount.acc_no = record.count + 1;  // set account_no = max_acc_no + 1
        printf("newACCount count 1: %d", newAccount.acc_no);

        int acc_type = 0;
        while (1) {
            messageSize = write(descriptor, K_ACC_TYPE, sizeof(K_ACC_TYPE));
            errorHandler(messageSize, "Error, K_ACC_TYPE message");

            // read acc type from user
            bzero(readBuffer, K_MAX_SIZE);
            int messageSize = read(descriptor, &readBuffer, K_MAX_SIZE);
            adminHandler(messageSize, "Error, reading acc type!");
            acc_type = atoi(readBuffer);
            if (acc_type == 1 || acc_type == 2) {
                break;
            }
        }
        newAccount.acc_type = acc_type - 1;                                            // 0->single, 1->joint
        newAccount.customer_id[0] = createNewCustomer(descriptor, newAccount.acc_no);  // default single acccount

        if (acc_type == 2) {
            newAccount.customer_id[1] = createNewCustomer(descriptor, newAccount.acc_no);
        } else {
            newAccount.customer_id[1] = -1;  // set customer_id = -1 for single account
        }

        newAccount.acc_balance = 0;
        newAccount.acc_prev_balance = 0;
        newAccount.is_Active = true;
        newAccount.last_transaction_time = 0;
        memcpy(newAccount.transactionDetail, (int[]){-1, -1, -1, -1, -1}, sizeof newAccount.transactionDetail);

        // write updated id in customer id record  file
        errorHandler(lseek(fd, 0, SEEK_SET), "Error, seek error in K_CUST_ID_CNT file!");

        record.count = newAccount.acc_no;
        errorHandler(write(fd, &record, sizeof(recordCount)), "Error, storing recordCount to file!");
    } else {
        return;
    }
    // unlock the file
    lck.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lck);
    // close file
    close(fd);

    // store newAccount in accountRecord file
    fd = openFile(K_ACC_RCD, O_RDWR);
    errorHandler(fd, "Error, K_ACC_RCD file opening!");

    // seek to last
    errorHandler(lseek(fd, 0, SEEK_END), "Error, seek error!");

    // store the new account
    errorHandler(write(fd, &newAccount, sizeof(account)), "Error, storing new account to file!");

    close(fd);  // close file

    // debugged for reading lastest added account
    fd = openFile(K_CUST_RCD, O_RDWR);
    errorHandler(fd, "Error, K_CUST_RCD file opening!");

    // seek to last
    errorHandler(lseek(fd, -sizeof(customer), SEEK_END), "Error, seek error in K_CUST_RCD file!");

    customer tempCustomer;
    memset(&tempCustomer, 0, sizeof(customer));
    messageSize = read(fd, &tempCustomer, sizeof(customer));
    errorHandler(messageSize, "Error, reading cust id count!");

    printf("acc no.:  %ld\n", tempCustomer.acc_no);
    printf("age: %d\n", tempCustomer.age);
    printf("gender: %c\n", tempCustomer.gender);
    printf("id: %d\n", tempCustomer.id);
    printf("is logged in: %d\n", tempCustomer.is_logged_in);
    printf("name: %s\n", tempCustomer.name);
    printf("password: %s\n", tempCustomer.password);
    close(fd);

    // debugged for reading lastest added account
    fd = openFile(K_ACC_RCD, O_RDWR);
    errorHandler(fd, "Error, K_ACC_RCD file opening!");

    // seek to last
    errorHandler(lseek(fd, -sizeof(account), SEEK_END), "Error, seek error in K_ACC_RCD file!");

    account tempAccount;
    memset(&tempAccount, 0, sizeof(account));
    messageSize = read(fd, &tempAccount, sizeof(account));
    errorHandler(messageSize, "Error, reading cust id count!");

    printf("acc balance: %ld\n", tempAccount.acc_balance);
    printf("acc no.: %d\n", tempAccount.acc_no);
    printf("acc prev balance: %ld\n", tempAccount.acc_prev_balance);
    printf("customer id 0: %d\n", tempAccount.customer_id[0]);
    printf("customer id 1:%d\n", tempAccount.customer_id[1]);
    printf("is active: %d\n", tempAccount.is_Active);
    printf("transaction time: %s", asctime(localtime(&tempAccount.last_transaction_time)));
    printf("transaction Detail: %d\n", tempAccount.transactionDetail[0]);
    close(fd);
    return;
}

int createNewCustomer(int descriptor, int acc_no) {
    customer newCustomer;
    int fd = openFile(K_CUST_ID_CNT, O_RDWR);
    errorHandler(fd, "Error, K_CUST_ID_CNT file opening!");
    char readBuffer[K_MAX_SIZE];
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
    int lck_val = fcntl(fd, F_SETLKW, &lck);  // returns 0 on successful lock operation

    if (lck_val == 0) {
        recordCount record;
        int messageSize = read(fd, &record, sizeof(recordCount));
        errorHandler(fd, "Error, K_CUST_ID_CNT file reading!");

        if (messageSize == 0) {
            record.count = -1;
        }

        newCustomer.id = record.count + 1;  // set id = max_id + 1

        newCustomer.acc_no = acc_no;

        // name
        messageSize = write(descriptor, K_ENTER_NAME, sizeof(K_ENTER_NAME));
        adminHandler(messageSize, "Error, K_ENTER_NAME message");

        // read name
        bzero(readBuffer, K_MAX_SIZE);
        messageSize = read(descriptor, &readBuffer, K_MAX_SIZE);
        adminHandler(messageSize, "Error, reading name type!");
        strcpy(newCustomer.name, readBuffer);

        // age
        messageSize = write(descriptor, K_ENTER_AGE, sizeof(K_ENTER_AGE));
        adminHandler(messageSize, "Error, K_ENTER_AGE message");

        // read age
        bzero(readBuffer, K_MAX_SIZE);
        messageSize = read(descriptor, &readBuffer, K_MAX_SIZE);
        adminHandler(messageSize, "Error, reading age type!");
        newCustomer.age = atoi(readBuffer);

        // gender
        messageSize = write(descriptor, K_ENTER_GENDER, sizeof(K_ENTER_GENDER));
        adminHandler(messageSize, "Error, K_ENTER_GENDER message");

        // read gender
        bzero(readBuffer, K_MAX_SIZE);
        messageSize = read(descriptor, &readBuffer, K_MAX_SIZE);
        adminHandler(messageSize, "Error, reading gender type!");
        newCustomer.gender = readBuffer[0];

        // password
        messageSize = write(descriptor, K_ENTER_PWD, sizeof(K_ENTER_PWD));
        adminHandler(messageSize, "Error, K_ENTER_PWD message");

        bzero(readBuffer, K_MAX_SIZE);
        messageSize = read(descriptor, &readBuffer, K_MAX_SIZE);
        adminHandler(messageSize, "Error, reading password type!");
        strcpy(newCustomer.password, readBuffer);

        newCustomer.is_logged_in = false;

        // write updated id in customer_id_count file
        errorHandler(lseek(fd, 0, SEEK_SET), "Error, seek error in K_CUST_RCD file!");

        record.count = newCustomer.id;
        errorHandler(write(fd, &record, sizeof(recordCount)), "Error, storing record to file!");
        // unlock file
    } else {
        return -1;
    }
    lck.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lck);
    // close file
    close(fd);

    // open customerRecord file
    fd = openFile(K_CUST_RCD, O_RDWR);
    errorHandler(fd, "Error, K_CUST_RCD file opening!");

    // seek to last
    errorHandler(lseek(fd, 0, SEEK_END), "Error, seek error in K_CUST_RCD file!");

    // store the newCustomer
    errorHandler(write(fd, &newCustomer, sizeof(customer)), "Error, storing new Customer to file!");

    close(fd);  // close file
    return newCustomer.id;
}

void deleteAccount(int descriptor) {
    account tempAccount;
    // ask admin for acc_no
    char readBuffer[K_MAX_SIZE];

    int messageSize = write(descriptor, K_ENTER_ACC_NO, sizeof(K_ENTER_ACC_NO));
    adminHandler(messageSize, "Error, K_ENTER_ACC_NO message");

    // read acc_no
    bzero(readBuffer, K_MAX_SIZE);
    messageSize = read(descriptor, &readBuffer, K_MAX_SIZE);
    adminHandler(messageSize, "Error, reading acc_no!");
    int acc_no = atoi(readBuffer);

    // open file
    int fd = openFile(K_ACC_RCD, O_RDWR);
    errorHandler(fd, "Error, K_ACC_RCD file opening!");

    struct flock lck;

    // clear the lock variable
    memset(&lck, 0, sizeof(lck));

    // set lock type and apply the same on the file
    lck.l_type = F_WRLCK;
    lck.l_whence = SEEK_SET;

    // calculating offset and len for acquing lock on particular record
    lck.l_start = acc_no * sizeof(account);  // start
    lck.l_len = sizeof(account);             // end
    lck.l_pid = getpid();
    // set lock on file

    int lck_val = fcntl(fd, F_SETLKW, &lck);  // returns 0 on successful lock operation

    if (lck_val == 0) {
        // set offset to account index
        errorHandler(lseek(fd, lck.l_start, SEEK_SET), "Error, seek error in K_ACC_RCD file!");

        // read from file
        memset(&tempAccount, 0, sizeof(account));
        messageSize = read(fd, &tempAccount, sizeof(account));
        errorHandler(messageSize, "Error, reading cust id count!");

        // comapre the acc_no and read struct for debuggig
        if (tempAccount.acc_no == acc_no) {
            LOG("success!");
        } else {
            LOG("failure!");
        }
        tempAccount.is_Active = false;

        // offset to record again
        errorHandler(lseek(fd, lck.l_start, SEEK_SET), "Error, seek error in K_ACC_RCD file!");

        // write the updated struct
        errorHandler(write(fd, &tempAccount, sizeof(account)), "Error, storing updated account to file!");
    } else {
        return;
    }
    // unlock the file
    lck.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lck);
    close(fd);
    return;
}

void getAllBankAccountDetails(int descriptor) {
    // open file
    account tempAccount;
    int fd = openFile(K_ACC_RCD, O_RDWR);
    errorHandler(fd, "Error, K_ACC_RCD file opening!");

    // apply read lock
    struct flock lck;

    // clear the lock variable
    memset(&lck, 0, sizeof(lck));

    // set lock type and apply the same on the file
    lck.l_type = F_RDLCK;
    lck.l_whence = SEEK_SET;
    lck.l_start = 0;  // start
    lck.l_len = 0;    // end of file
    lck.l_pid = getpid();

    // set lock on file
    int lck_val = fcntl(fd, F_SETLKW, &lck);  // returns 0 on successful lock operation

    if (lck_val == 0) {
        errorHandler(lseek(fd, -sizeof(account), SEEK_END), "lseek error!");

        memset(&tempAccount, 0, sizeof(account));
        int messageSize = read(fd, &tempAccount, sizeof(account));
        errorHandler(messageSize, "Error, reading cust id count!");
        int max_acc_no = tempAccount.acc_no;
        off_t offset;
        for (int i = 0; i <= max_acc_no; i++) {
            offset = i * sizeof(account);
            errorHandler(lseek(fd, offset, SEEK_SET), "lseek i!");

            memset(&tempAccount, 0, sizeof(account));
            messageSize = read(fd, &tempAccount, sizeof(account));
            errorHandler(messageSize, "Error, reading cust id count!");

            // debugging TODO: send this over to client 1 by 1 using proper write-read sync
            printf("acc balance: %ld\n", tempAccount.acc_balance);
            printf("acc no.: %d\n", tempAccount.acc_no);
            printf("acc prev balance: %ld\n", tempAccount.acc_prev_balance);
            printf("customer id 0: %d\n", tempAccount.customer_id[0]);
            printf("customer id 1:%d\n", tempAccount.customer_id[1]);
            printf("is active: %d\n", tempAccount.is_Active);
            printf("last transaction time: %s\n", asctime(localtime(&tempAccount.last_transaction_time)));
            printf("transaction Detail: %d\n", tempAccount.transactionDetail[0]);
        }
    } else {
        return;
    }
    lck.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lck);
    close(fd);
    return;
}

void getCustomerAccountDetails(int descriptor) {
    account tempAccount;
    customer tempCustomer;
    // ask admin for acc_no
    char readBuffer[K_MAX_SIZE], writeBuffer[K_MAX_SIZE], tempBuffer[K_MAX_SIZE];
    strcpy(writeBuffer, "");

    int messageSize = write(descriptor, K_ENTER_ACC_NO, sizeof(K_ENTER_ACC_NO));
    adminHandler(messageSize, "Error, K_ENTER_ACC_NO message");

    // read acc_no
    bzero(readBuffer, K_MAX_SIZE);
    messageSize = read(descriptor, &readBuffer, K_MAX_SIZE);
    adminHandler(messageSize, "Error, reading acc_no!");
    int acc_no = atoi(readBuffer);

    // open file
    int fd = openFile(K_ACC_RCD, O_RDWR);
    errorHandler(fd, "Error, K_ACC_RCD file opening!");

    struct flock lck;

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

    int lck_val = fcntl(fd, F_SETLKW, &lck);  // returns 0 on successful lock operation

    if (lck_val == 0) {
        // set offset to account index
        errorHandler(lseek(fd, lck.l_start, SEEK_SET), "Error, seek error in K_ACC_RCD file!");

        // read from file
        memset(&tempAccount, 0, sizeof(account));
        messageSize = read(fd, &tempAccount, sizeof(account));
        errorHandler(messageSize, "Error, reading cust id count!");
        if (tempAccount.is_Active == false) {
            lck.l_type = F_UNLCK;
            fcntl(fd, F_SETLK, &lck);
            close(fd);
            // send dummy
            sprintf(tempBuffer, "Acc no. %d does not exists!\n", acc_no);
            strcat(writeBuffer, tempBuffer);
            strcat(writeBuffer, SKIP_MESSAGE);

            messageSize = write(descriptor, writeBuffer, K_MAX_SIZE);
            adminHandler(messageSize, "Error, sending customer details");

            // skip read
            messageSize = read(descriptor, &readBuffer, K_MAX_SIZE);
            adminHandler(messageSize, "Error, dummy read!");
            bzero(readBuffer, K_MAX_SIZE);

            lck.l_type = F_UNLCK;
            fcntl(fd, F_SETLK, &lck);
            close(fd);
            return;
        }
    } else {
        LOG("getcustomerAccountDetails 1: lock error!");
        return;
    }
    // unlock the file
    lck.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lck);
    close(fd);

    // customer 1
    int acc_type = tempAccount.acc_type;
    int customer_id = tempAccount.customer_id[0];
    // open file
    fd = openFile(K_CUST_RCD, O_RDWR);
    errorHandler(fd, "Error, K_CUST_RCD file opening!");

    // clear the lock variable
    memset(&lck, 0, sizeof(lck));

    // set lock type and apply the same on the file
    lck.l_type = F_RDLCK;
    lck.l_whence = SEEK_SET;

    // calculating offset and len for acquing lock on particular record
    lck.l_start = customer_id * sizeof(customer);  // start
    lck.l_len = sizeof(customer);                  // end
    lck.l_pid = getpid();
    // set lock on file

    lck_val = fcntl(fd, F_SETLKW, &lck);  // returns 0 on successful lock operation

    if (lck_val == 0) {
        // set offset to account index
        errorHandler(lseek(fd, lck.l_start, SEEK_SET), "Error, seek error in K_CUST_RCD file!");

        // read from file
        memset(&tempCustomer, 0, sizeof(customer));
        messageSize = read(fd, &tempCustomer, sizeof(customer));
        errorHandler(messageSize, "Error, reading cust id count!");

        sprintf(tempBuffer, "Name : %s\n", tempCustomer.name);
        strcat(writeBuffer, tempBuffer);

        sprintf(tempBuffer, "Age : %d\n", tempCustomer.age);
        strcat(writeBuffer, tempBuffer);

        sprintf(tempBuffer, "Gender : %c\n", tempCustomer.gender);
        strcat(writeBuffer, tempBuffer);
    } else {
        LOG("getcustomerAccountDetails 2: lock error!");
        return;
    }
    // unlock the file
    lck.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lck);

    // customer 2
    if (acc_type == 1) {
        customer_id = tempAccount.customer_id[1];

        // clear the lock variable
        memset(&lck, 0, sizeof(lck));

        // set lock type and apply the same on the file
        lck.l_type = F_RDLCK;
        lck.l_whence = SEEK_SET;

        // calculating offset and len for acquing lock on particular record
        lck.l_start = customer_id * sizeof(customer);  // start
        lck.l_len = sizeof(customer);                  // end
        lck.l_pid = getpid();
        // set lock on file

        int lck_val = fcntl(fd, F_SETLKW, &lck);  // returns 0 on successful lock operation

        if (lck_val == 0) {
            // set offset to account index
            errorHandler(lseek(fd, lck.l_start, SEEK_SET), "Error, seek error in K_CUST_RCD file!");

            // read from file
            memset(&tempCustomer, 0, sizeof(customer));
            messageSize = read(fd, &tempCustomer, sizeof(customer));
            errorHandler(messageSize, "Error, reading cust id count!");

            sprintf(tempBuffer, "Joint Account holder:\nName : %s\n", tempCustomer.name);
            strcat(writeBuffer, tempBuffer);

            sprintf(tempBuffer, "Age : %d\n", tempCustomer.age);
            strcat(writeBuffer, tempBuffer);

            sprintf(tempBuffer, "Gender : %c\n", tempCustomer.gender);
            strcat(writeBuffer, tempBuffer);
        } else {
            LOG("getcustomerAccountDetails 3: lock error!");
            return;
        }
        // unlock the file
        lck.l_type = F_UNLCK;
        fcntl(fd, F_SETLK, &lck);
        close(fd);
    }

    sprintf(tempBuffer, "Account Details:\nAcc no.: %d\n", tempAccount.acc_no);
    strcat(writeBuffer, tempBuffer);

    if (tempAccount.acc_type == 0) {
        sprintf(tempBuffer, "Acc type: Single Account\n");
        strcat(writeBuffer, tempBuffer);
        sprintf(tempBuffer, "Customer Id.: %d\n", tempAccount.customer_id[0]);
        strcat(writeBuffer, tempBuffer);
    } else {
        sprintf(tempBuffer, "Acc type: Joint Account\n");
        strcat(writeBuffer, tempBuffer);
        sprintf(tempBuffer, "Customer 1 Id.: %d\n", tempAccount.customer_id[0]);
        strcat(writeBuffer, tempBuffer);
        sprintf(tempBuffer, "Customer 2 Id.: %d\n", tempAccount.customer_id[1]);
        strcat(writeBuffer, tempBuffer);
    }

    sprintf(tempBuffer, "Account balance: %d\n", tempAccount.acc_balance);
    strcat(writeBuffer, tempBuffer);

    strcat(writeBuffer, SKIP_MESSAGE);  // append the demilimiter
    messageSize = write(descriptor, writeBuffer, K_MAX_SIZE);
    adminHandler(messageSize, "Error, sending customer details");

    // skip read
    messageSize = read(descriptor, &readBuffer, K_MAX_SIZE);
    adminHandler(messageSize, "Error, dummy read!");
    bzero(readBuffer, K_MAX_SIZE);
    return;
}

void modifyAccount(int descriptor) {
    // TODO: check account active or not before modifying the customer details
    char readBuffer[K_MAX_SIZE];
    customer tempCustomer;
    // ask for customer id
    int messageSize = write(descriptor, K_ENTER_CUST_ID, sizeof(K_ENTER_CUST_ID));
    adminHandler(messageSize, "Error, K_ENTER_CUST_ID message");

    bzero(readBuffer, K_MAX_SIZE);
    messageSize = read(descriptor, &readBuffer, K_MAX_SIZE);
    adminHandler(messageSize, "Error, reading cust_id!");
    int cust_id = atoi(readBuffer);

    // fetch the customer record
    int fd = openFile(K_CUST_RCD, O_RDWR);
    errorHandler(fd, "Error, K_CUST_RCD file opening!");

    struct flock lck;

    memset(&lck, 0, sizeof(lck));

    // set lock type and apply the same on the file
    lck.l_type = F_WRLCK;
    lck.l_whence = SEEK_SET;

    // calculating offset and len for acquing lock on particular record
    lck.l_start = cust_id * sizeof(customer);  // start
    lck.l_len = sizeof(customer);              // end
    lck.l_pid = getpid();
    // set lock on file

    int lck_val = fcntl(fd, F_SETLKW, &lck);  // returns 0 on successful lock operation

    if (lck_val == 0) {
        // set offset to account index
        errorHandler(lseek(fd, lck.l_start, SEEK_SET), "Error, seek error in K_CUST_RCD file!");

        memset(&tempCustomer, 0, sizeof(customer));
        messageSize = read(fd, &tempCustomer, sizeof(customer));
        errorHandler(messageSize, "Error, reading cust id count!");

        // ask to change name (Y/N)
        int messageSize = write(descriptor, K_CHANGE_NAME, strlen(K_CHANGE_NAME));
        adminHandler(messageSize, "Error, K_CHANGE_NAME message");

        bzero(readBuffer, K_MAX_SIZE);
        messageSize = read(descriptor, &readBuffer, K_MAX_SIZE);
        adminHandler(messageSize, "Error, reading cust name!");
        char choice = readBuffer[0];
        if (choice == 'Y' || choice == 'y') {
            // prompt for new name
            int messageSize = write(descriptor, K_ENTER_NAME, sizeof(K_ENTER_NAME));
            adminHandler(messageSize, "Error, K_ENTER_NAME message");

            bzero(readBuffer, K_MAX_SIZE);
            messageSize = read(descriptor, &readBuffer, K_MAX_SIZE);
            adminHandler(messageSize, "Error, reading cust_new_name!");

            // store in temp customer
            strcpy(tempCustomer.name, readBuffer);
        }

        // ask to change age (Y/N)
        messageSize = write(descriptor, K_CHANGE_AGE, sizeof(K_CHANGE_AGE));
        adminHandler(messageSize, "Error, K_CHANGE_AGE message");

        bzero(readBuffer, K_MAX_SIZE);
        messageSize = read(descriptor, &readBuffer, K_MAX_SIZE);
        adminHandler(messageSize, "Error, reading cust_age!");
        choice = readBuffer[0];
        if (choice == 'Y' || choice == 'y') {
            // prompt for new age
            messageSize = write(descriptor, K_ENTER_AGE, sizeof(K_ENTER_AGE));
            adminHandler(messageSize, "Error, K_ENTER_AGE message");

            bzero(readBuffer, K_MAX_SIZE);
            messageSize = read(descriptor, &readBuffer, K_MAX_SIZE);
            adminHandler(messageSize, "Error, reading cust_new_age!");
            int new_age = atoi(readBuffer);

            // store in temp customer
            tempCustomer.age = new_age;
        }

        // ask to change gender (Y/N)
        messageSize = write(descriptor, K_CHANGE_GENDER, sizeof(K_CHANGE_GENDER));
        adminHandler(messageSize, "Error, K_CHANGE_GENDER message");

        bzero(readBuffer, K_MAX_SIZE);
        messageSize = read(descriptor, &readBuffer, K_MAX_SIZE);
        adminHandler(messageSize, "Error, reading cust_gender!");
        choice = readBuffer[0];
        if (choice == 'Y' || choice == 'y') {
            // prompt for new gender
            messageSize = write(descriptor, K_ENTER_GENDER, sizeof(K_ENTER_GENDER));
            adminHandler(messageSize, "Error, K_ENTER_GENDER message");

            bzero(readBuffer, K_MAX_SIZE);
            messageSize = read(descriptor, &readBuffer, K_MAX_SIZE);
            adminHandler(messageSize, "Error, reading cust_new_gender!");

            // store in temp customer
            tempCustomer.gender = readBuffer[0];
        }

        // ask to confirm the changes (Y/N)
        messageSize = write(descriptor, K_CONFIRM, sizeof(K_CONFIRM));
        adminHandler(messageSize, "Error, K_CONFIRM message");

        bzero(readBuffer, K_MAX_SIZE);
        messageSize = read(descriptor, &readBuffer, K_MAX_SIZE);
        adminHandler(messageSize, "Error, reading confirmation message!");
        choice = readBuffer[0];
        // if yes, update the record of customer apply write lock
        if (choice == 'Y' || choice == 'y') {
            errorHandler(lseek(fd, lck.l_start, SEEK_SET), "Error, seek error in K_CUST_RCD file!");

            // write the updated struct
            errorHandler(write(fd, &tempCustomer, sizeof(customer)), "Error, storing updated customer to file!");
        }
    } else {
        return;
    }
    // unlock the file
    lck.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lck);
    close(fd);
    return;
}

void getAllTransactionDetails(int descriptor) {
    int maxCount;
    transaction tempTransaction;
    // open transaction_id_count file
    int transaction_fd = openFile(K_TRANS_ID_CNT, O_RDWR);
    errorHandler(transaction_fd, "Error, K_TRANS_ID_CNT file opening!");

    // apply lock
    struct flock lck;

    // clear the lock variable
    memset(&lck, 0, sizeof(lck));

    // set lock type and apply the same on the file
    lck.l_type = F_RDLCK;
    lck.l_whence = SEEK_SET;

    lck.l_start = 0;  // start
    lck.l_len = 0;    // end of file
    lck.l_pid = getpid();

    // set lock on file
    int lck_val = fcntl(transaction_fd, F_SETLKW, &lck);  // returns 0 on successful lock operation

    if (lck_val == 0) {
        recordCount record;
        int messageSize = read(transaction_fd, &record, sizeof(recordCount));
        errorHandler(transaction_fd, "Error, K_TRANS_ID_CNT file reading!");

        maxCount = record.count;
    } else {
        return;
    }
    lck.l_type = F_UNLCK;
    fcntl(transaction_fd, F_SETLK, &lck);
    close(transaction_fd);

    for (int i = 0; i <= maxCount; i++) {
        // add new transaction in transactionRecord file

        int fd = openFile(K_TRANS_RCD, O_RDWR);
        errorHandler(fd, "Error, opening K_TRANS_RCD file!");
        // clear the lock variable
        memset(&lck, 0, sizeof(lck));

        // set lock type and apply the same on the file
        lck.l_type = F_RDLCK;
        lck.l_whence = SEEK_SET;

        lck.l_start = i * sizeof(transaction);  // start
        lck.l_len = sizeof(transaction);        // end of file
        lck.l_pid = getpid();

        // set lock on file
        lck_val = fcntl(fd, F_SETLKW, &lck);  // returns 0 on successful lock operation

        if (lck_val == 0) {
            errorHandler(lseek(fd, i * sizeof(transaction), SEEK_SET), "Error, seek error!");

            errorHandler(read(fd, &tempTransaction, sizeof(transaction)), "Error, K_TRANS_RCD file reading!");
            printf("acc no.: %d\n", tempTransaction.acc_no);
            printf("transaction id: %d\n", tempTransaction.id);
            if (tempTransaction.type == 1) {
                printf("transaction type : WITHDRAW\n");
            } else {
                printf("transaction type : DEPOSIT\n");
            }
            printf("amount: %ld\n", tempTransaction.amount);
            printf("acc prev balance: %ld\n", tempTransaction.prev_balance);
            printf("acc balance: %ld\n", tempTransaction.balance);
            printf("transaction time: %s\n", asctime(localtime(&tempTransaction.transaction_time)));

        } else {
            return;
        }
        lck.l_type = F_UNLCK;
        fcntl(fd, F_SETLK, &lck);
        close(fd);
    }
}