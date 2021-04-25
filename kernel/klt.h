// Q3
/*
    Task 3 - Kerlnel level threads implementation. 
    klt extends xv6 proc implementation
*/
# define MAX_STACK_SIZE 4000

//TODO: fix / change the statess
enum threadstate { UNUSED_THREAD, SLEEPING, RUNNABLE, RUNNING, ZOMBIE_THREAD };
struct thread {
    struct spinlock *lock;
    
    // th->lock must be held when using these:
    enum threadstate state;         // thread state
    void *chan;                     // If non-zero, sleeping on chan
    int killed;                  // If non-zero, have been killed
    
    // thread_tree_lock must be held when using this:
    struct proc *procparent;        // Parent process of the thread
    struct trapframe *trapframe;    // data page for trampoline.S
    struct context context;         // swtch() here to run process


    // these are private to the thread, so th->lock need not be held.
    uint64 kstack;                  // Virtual address of kernel stack
    
    int id;                         // thread id 

};


