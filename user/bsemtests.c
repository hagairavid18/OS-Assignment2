#include "kernel/types.h"
#include "kernel/fs.h"

#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"


void bsem_test(char *s){
    int pid;
    int bid = bsem_alloc();
    bsem_down(bid);
    printf("1. Parent downing semaphore\n");
    if((pid = fork()) == 0){
        printf("2. Child downing semaphore\n");
        bsem_down(bid);
        printf("4. Child woke up\n");
        exit(0);
    }
    sleep(5);
    printf("3. Let the child wait on the semaphore...\n");
    sleep(10);
    bsem_up(bid);

    bsem_free(bid);
    wait(&pid);

    printf("Finished bsem test, make sure that the order of the prints is alright. Meaning (1...2...3...4)\n");
}


int main(int argc, char **argv)
{
    return 0;
}