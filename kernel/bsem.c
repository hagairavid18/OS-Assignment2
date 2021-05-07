#include "bsem.h"

static int bsem_table[MAX_BSEM] = {[0 ... MAX_BSEM-1] = -1};
/*
    binary semaphores states:
        -1  := uninitialized
        0   := unlocked
        1   := locked
*/


int bsem_alloc(void){
    int sd = 0; // Semaphore descriptor
    for (sd = 0; sd < MAX_BSEM; sd++){
        if (bsem_table[sd] == 0){
            bsem_table[sd] = 0; // occupy semaphore i
            return sd;
        }
    }
    return -1; // in case no avalable semaphore found
}

void bsem_free(int sd){
    if (sd != -1) bsem_table[sd] = -1; // free a valid binary semaphore

    // Note: Note that the behavior of freeing a semaphore while other threads “blocked” because of it is undefined 
    // and is not be supported.
}

void bsem_down(int sd){

}

void bsem_out(int sd){

    
}

