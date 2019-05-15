#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>
#include <errno.h>
#include "hashsum.h"
#include <fcntl.h>
#include "string.h"
#include "requestQueue.h"
#include "../utils/constants.h"
#include "../utils/types.h"
#include "../utils/sope.h"

pthread_t bankoffice[MAX_BANK_OFFICES];
bank_account_t bankaccounts[MAX_BANK_ACCOUNTS];

void *officeprocessing(void *arg){
    int *i = (int*)arg;
    i++;
    pthread_exit(0);
}

void saltGenerator(char *salt){

    char randomSalt[] = "0123456789abcdef";
    for(int i = 0; i < SALT_LEN; i++){
        salt[i] = randomSalt[rand() % (strlen(randomSalt))];
    }

    salt[SALT_LEN] = '\0';

}

void create_admin_acc(char *password){
    bank_account_t admin_acc;
    admin_acc.account_id = ADMIN_ACCOUNT_ID;
    admin_acc.balance = 0;
    saltGenerator(admin_acc.salt);
    hashGenerator(admin_acc.salt, password, admin_acc.hash);
}

bool verifyAccount(uint32_t id, char * password, bank_account_t account){
    char hash[HASH_LEN + 1];
    hashGenerator(account.salt, password, hash);
    if(id == account.account_id && strcmp(hash, account.hash) == 0) return true;
    return false;
}

int main(int argc, char *argv[]){

    if(argc != 3){
        printf("Wrong number of args!\n");
        return -1;
    }

    if(atoi(argv[1]) > 99){
        printf("The number of bank offices must be at max 99!\n");
        return -1;
    }
    create_admin_acc(argv[2]);

    reqQ_t *requestQueue = createQueue();
    
    if(mkfifo(SERVER_FIFO_PATH, 0666) == -1){
        exit(8);
    }  

    int serverFifo = open(SERVER_FIFO_PATH, O_RDONLY);
    if(serverFifo == -1){
        return -1;
    }
    
    for(int i = 1; i <= atoi(argv[1]); i++) pthread_create(&bankoffice[i], NULL, officeprocessing, NULL);
    
    int i = 0;
    while(i < 3){
        tlv_request_t request;
        req_value_t val;
        read(serverFifo, &request, sizeof(op_type_t) + sizeof(uint32_t));
        read(serverFifo, &val, sizeof(req_value_t));
        request.value = val;
        enQueue(requestQueue, request);
        printf("READ\n TYPE:%d\n LENGHT:%d\n", requestQueue->front->key.type, requestQueue->front->key.length);
        deQueue(requestQueue);
        i++;
    }


    close(serverFifo);
    unlink(SERVER_FIFO_PATH);
}