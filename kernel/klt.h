// Q3
/*
    Task 3 - Kerlnel level threads implementation. 
    klt extends xv6 proc implementation
*/
# define MAX_STACK_SIZE 4000

enum threadstate { UNUSED_THREAD, USED_THREAD, SLEEPING_THREAD, RUNNABLE_THREAD, RUNNING_THREAD, ZOMBIE_THREAD };
struct thread {
    struct spinlock *lock;
    
    // th->lock must be held when using these:
    enum threadstate state;        // thread state
    
    // thread_tree_lock must be held when using this:
    struct proc *procparent;        // Parent process of the thread
    
    int id;                         // thread id 

};


