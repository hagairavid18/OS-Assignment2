#include "spinlock.h"
enum bsemstate { UNUSED, USED, ACTIVE ,ZOMBIE }; 

struct bsem {
    enum bsemstate state;
    struct spinlock lock;
};

