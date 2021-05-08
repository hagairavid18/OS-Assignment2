#include "Csemaphore.h"

// If the value representing the count of the semaphore variable is not negative,
// decrement it by 1. If the semaphore variable is now negative, the thread executing
// acquire is blocked until the value is greater or equal to 1. Otherwise, the thread
// continues execution
void csem_down(struct counting_semaphore *sem){
    bsem_down(sem->s2);
    bsem_down(sem->s1);
    sem->value--;
    if (sem->value > 0) bsem_up(sem->s2);    
    bsem_up(sem->s1);
    

}

// Increments the value of the semaphore variable by 1. As before, you are free to build
// the struct counting_semaphore as you wish.
void csem_up(struct counting_semaphore *sem){
    bsem_down(sem->s1);
    sem->value++;
    if(sem->value == 1) bsem_up(sem->s2);    
    bsem_up(sem->s1);    

}


// Allocates a new counting semaphore, and sets its initial value. Return value is 0 upon
// success, another number otherwise.
int csem_alloc(struct counting_semaphore *sem, int initial_value){
    sem->value = initial_value;
    sem->s1 = bsem_alloc();
    sem->s2 = bsem_alloc();
    
    // check for validity:
    if (initial_value < 1 || sem->s1 == -1 || sem->s2 == -1) return -1;

    return 0; 

}

// Frees semaphore.
// Note that the behavior of freeing a semaphore while other threads “blocked” because
// of it is undefined and is not supported.
void csem_free(struct counting_semaphore *sem){
    bsem_free(sem->s1);
    bsem_free(sem->s2);
    sem->value = -1;

}
