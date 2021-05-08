#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

struct counting_semaphore{
    int value;
    int s1;
    int s2;
};

void csem_down(struct counting_semaphore *);
void csem_up(struct counting_semaphore *);
int csem_alloc(struct counting_semaphore *, int);
void csem_free(struct counting_semaphore *);