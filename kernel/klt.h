// Q3
/*
    Task 3 - Kerlnel level threads implementation. 
    klt extends xv6 proc implementation
*/
# define MAX_STACK_SIZE 4000

// Saved registers for kernel context switches.
struct context {
  uint64 ra;
  uint64 sp;

  // callee-saved
  uint64 s0;
  uint64 s1;
  uint64 s2;
  uint64 s3;
  uint64 s4;
  uint64 s5;
  uint64 s6;
  uint64 s7;
  uint64 s8;
  uint64 s9;
  uint64 s10;
  uint64 s11;
};




//TODO: fix / change the statess
enum threadstate { UNUSED_THREAD, SLEEPING, RUNNABLE, RUNNING, ZOMBIE_THREAD };
struct thread {
    struct spinlock lock;
    
    // th->lock must be held when using these:
    enum threadstate state;         // thread state
    void *chan;                     // If non-zero, sleeping on chan
    int killed;                     // If non-zero, have been killed
    int xstate;                     // Exit status to be returned to parent's wait
    int tid;                         // thread id 


    // thread_tree_lock must be held when using this:
    struct proc *procparent;        // Parent process of the thread
    struct trapframe *trapframe;    // data page for trampoline.S
    struct context context;         // swtch() here to run process


    // these are private to the thread, so th->lock need not be held.
    uint64 kstack;                  // Virtual address of kernel stack
    
    // TODO: xstate is needed?
};


