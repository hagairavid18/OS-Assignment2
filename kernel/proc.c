#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "fs.h"

extern void *sigret_call_start(void);
extern void *sigret_call_end(void);

struct cpu cpus[NCPU];

struct proc proc[NPROC];

struct proc *initproc;

int nextpid = 1;
struct spinlock pid_lock;

extern void forkret(void);
static void freeproc(struct proc *p);

extern char trampoline[]; // trampoline.S

// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;

// Allocate a page for each process's kernel stack.
// Map it high in memory, followed by an invalid
// guard page.
void proc_mapstacks(pagetable_t kpgtbl)
{
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++)
  {
    char *pa = kalloc();
    if (pa == 0)
      panic("kalloc");
    uint64 va = KSTACK((int)(p - proc));
    kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  }
}

// initialize the proc table at boot time.
void procinit(void)
{
  struct proc *p;

  initlock(&pid_lock, "nextpid");
  initlock(&wait_lock, "wait_lock");
  for (p = proc; p < &proc[NPROC]; p++)
  {
    initlock(&p->lock, "proc");
    p->kstack = KSTACK((int)(p - proc));
  }
}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int cpuid()
{
  int id = r_tp();
  return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu *
mycpu(void)
{
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

//TODO: how to fix myproc() for Q3?
// Return the current struct proc *, or zero if none.
struct proc *
myproc(void)
{
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}

// Q3 >>>
// Return the current struct thread*, or zero if none.
struct thread *
mythread(void)
{
  // TODO; copied from mycpu(), need to be modified
  push_off();
  struct cpu *c = mycpu();
  struct thread *th = c->thread;
  pop_off();
  return th;
}

// initizialised first thread in initiated proc
// retrun 0 on success, -1 otherwise
int
initthread (struct proc *p){
  /*
    TODO: implement function
  */
  struct thread* th = &p->threads[0];
  th->id = 0;                         // Set first id to 0;

  return 1;
}

int
initthreadstable (struct proc *p){
  struct thread *th;

  for (th = p->threads; th < &p->threads[NTHREAD]; th++)
  { 
    th->procparent = p;         // set parent
    th->state = UNUSED_THREAD;  // set state
  }
  return 1; 
}

// <<< END

int allocpid()
{
  int pid;

  acquire(&pid_lock);
  pid = nextpid;
  nextpid = nextpid + 1;
  release(&pid_lock);

  return pid;
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.
static struct proc *
allocproc(void)
{
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++)
  {
    acquire(&p->lock);
    if (p->state == UNUSED)
    {
      goto found;
    }
    else
    {
      release(&p->lock);
    }
  }
  return 0;

found:
  p->pid = allocpid();
  p->state = USED;

  // Q3 >>>
  //TODO: finish!

  if ( !initthreadstable(p) || !initthread(p)) return 0;  // Initial threads table with UNUSED state & init the first thread

  // <<< Q3


  // Allocate a trapframe page.
  if ((p->trapframe = (struct trapframe *)kalloc()) == 0)
  {
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if (p->pagetable == 0)
  {
    freeproc(p);
    release(&p->lock);
    return 0;
    memset
  }

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;

  // Q2 >>>
  int i;
  for (i = 0; i < 32; i++)
  {
    p->sigHandlers[i] = (void *)SIG_DFL;
  }
  // <<< END

  return p;
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p)
{
  if (p->trapframe)
    kfree((void *)p->trapframe);
  p->trapframe = 0;
  if (p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);
  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;
}

// Create a user page table for a given process,
// with no user memory, but with trampoline pages.
pagetable_t
proc_pagetable(struct proc *p)
{
  pagetable_t pagetable;

  // An empty page table.
  pagetable = uvmcreate();
  if (pagetable == 0)
    return 0;

  // map the trampoline code (for system call return)
  // at the highest user virtual address.
  // only the supervisor uses it, on the way
  // to/from user space, so not PTE_U.
  if (mappages(pagetable, TRAMPOLINE, PGSIZE,
               (uint64)trampoline, PTE_R | PTE_X) < 0)
  {
    uvmfree(pagetable, 0);
    return 0;
  }

  // map the trapframe just below TRAMPOLINE, for trampoline.S.
  if (mappages(pagetable, TRAPFRAME, PGSIZE,
               (uint64)(p->trapframe), PTE_R | PTE_W) < 0)
  {
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }

  return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
  uvmunmap(pagetable, TRAMPOLINE, 1, 0);
  uvmunmap(pagetable, TRAPFRAME, 1, 0);
  uvmfree(pagetable, sz);
}

// a user program that calls exec("/init")
// od -t xC initcode
uchar initcode[] = {
    0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02,
    0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02,
    0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00,
    0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00,
    0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69,
    0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00};

// Set up first user process.
void userinit(void)
{
  struct proc *p;

  p = allocproc();
  initproc = p;

  // allocate one user page and copy init's instructions
  // and data into it.
  uvminit(p->pagetable, initcode, sizeof(initcode));
  p->sz = PGSIZE;

  // prepare for the very first "return" from kernel to user.
  p->trapframe->epc = 0;     // user program counter
  p->trapframe->sp = PGSIZE; // user stack pointer

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // p->state = RUNNABLE;
  p->state = ACTIVE; //Q3.1
  release(&p->lock);
}

// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int growproc(int n)
{
  uint sz;
  struct proc *p = myproc();

  sz = p->sz;
  if (n > 0)
  {
    if ((sz = uvmalloc(p->pagetable, sz, sz + n)) == 0)
    {
      return -1;
    }
  }
  else if (n < 0)
  {
    sz = uvmdealloc(p->pagetable, sz, sz + n);
  }
  p->sz = sz;
  return 0;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
// int fork(void)
// {
//   int i, pid;
//   struct proc *np;
//   struct proc *p = myproc();

//   // Allocate process.
//   if ((np = allocproc()) == 0)
//   {
//     return -1;
//   }

//   // Copy user memory from parent to child.
//   if (uvmcopy(p->pagetable, np->pagetable, p->sz) < 0)
//   {
//     freeproc(np);
//     release(&np->lock);
//     return -1;
//   }
//   np->sz = p->sz;

//   // copy saved user registers.
//   *(np->trapframe) = *(p->trapframe);

//   // Cause fork to return 0 in the child.
//   np->trapframe->a0 = 0;

//   //TASK 2.1.2
//   for (i = 0; i < 32; i++)
//   {
//     np->sigHandlers[i] = p->sigHandlers[i];
//   }
//   np->sigMask = p->sigMask;
//   //

//   // increment reference counts on open file descriptors.
//   for (i = 0; i < NOFILE; i++)
//     if (p->ofile[i])
//       np->ofile[i] = filedup(p->ofile[i]);
//   np->cwd = idup(p->cwd);

//   safestrcpy(np->name, p->name, sizeof(p->name));

//   pid = np->pid;

//   release(&np->lock);

//   acquire(&wait_lock);
//   np->parent = p;
//   release(&wait_lock);

//   acquire(&np->lock);
//   // np->state = RUNNABLE;
//   np->state = ACTIVE; // Q3.1

//   release(&np->lock);

//   return pid;
// }
int fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *p = myproc();
  struct thread * th = mythread();


  // Allocate process.
  if ((np = allocproc()) == 0)
  {
    return -1;
  }

  // Copy user memory from parent to child.
  if (uvmcopy(p->pagetable, np->pagetable, p->sz) < 0)
  {
    freeproc(np);
    release(&np->lock);
    return -1;
  }
  np->sz = p->sz;

  acquire(&wait_lock); //TODO: Check wich lock should be accuired!

  // copy saved user registers.
  *np->threads[0].trapframe = *th->trapframe;
  // Cause fork to return 0 in the child.
  np->threads[0].trapframe->a0 = 0;
  
  release(&wait_lock); //TODO: Check wich lock should be accuired!

  
  // increment reference counts on open file descriptors.
  for (i = 0; i < NOFILE; i++)
    if (p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = idup(p->cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;

  release(&np->lock);

  acquire(&wait_lock);
  // // copy saved user registers.
  // *np->threads[0].trapframe = *th->trapframe; //TODO: Check wich lock should be accuired!
  // // Cause fork to return 0 in the child.
  // np->threads[0].trapframe->a0 = 0; //TODO: Check wich lock should be accuired!
  np->parent = p;
  release(&wait_lock);

  acquire(&np->lock);

  np->state = ACTIVE; // Q3.1
  np->threads[0].state = RUNNABLE;

  //TASK 2.1.2
  for (i = 0; i < 32; i++)
  {
    // TODO: what about the masks?
    np->sigHandlers[i] = p->sigHandlers[i];
    
  }
  np->sigMask = p->sigMask;
  
  release(&np->lock);


  return pid;
}




// Pass p's abandoned children to init.
// Caller must hold wait_lock.
void reparent(struct proc *p)
{
  struct proc *pp;

  for (pp = proc; pp < &proc[NPROC]; pp++)
  {
    if (pp->parent == p)
    {
      pp->parent = initproc;
      wakeup(initproc);
    }
  }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
void exit(int status)
{
  struct proc *p = myproc();

  if (p == initproc)
    panic("init exiting");

  // Close all open files.
  for (int fd = 0; fd < NOFILE; fd++)
  {
    if (p->ofile[fd])
    {
      struct file *f = p->ofile[fd];
      fileclose(f);
      p->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(p->cwd);
  end_op();
  p->cwd = 0;

  acquire(&wait_lock);

  // Give any children to init.
  reparent(p);

  // Parent might be sleeping in wait().
  wakeup(p->parent);

  acquire(&p->lock);

  p->xstate = status;
  p->state = ZOMBIE;

  release(&wait_lock);

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int wait(uint64 addr)
{
  struct proc *np;
  int havekids, pid;
  struct proc *p = myproc();

  acquire(&wait_lock);

  for (;;)
  {
    // Scan through table looking for exited children.
    havekids = 0;
    for (np = proc; np < &proc[NPROC]; np++)
    {
      if (np->parent == p)
      {
        // make sure the child isn't still in exit() or swtch().
        acquire(&np->lock);

        havekids = 1;
        if (np->state == ZOMBIE)
        {
          // Found one.
          pid = np->pid;
          if (addr != 0 && copyout(p->pagetable, addr, (char *)&np->xstate,
                                   sizeof(np->xstate)) < 0)
          {
            release(&np->lock);
            release(&wait_lock);
            return -1;
          }
          freeproc(np);
          release(&np->lock);
          release(&wait_lock);
          return pid;
        }
        release(&np->lock);
      }
    }

    // No point waiting if we don't have any children.
    if (!havekids || p->killed)
    {
      release(&wait_lock);
      return -1;
    }

    // Wait for a child to exit.
    sleep(p, &wait_lock); //DOC: wait-sleep
  }
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.

// void scheduler(void)
// {
//   struct proc *p;
//   struct cpu *c = mycpu();

//   c->proc = 0;
//   for (;;)
//   {
//     // Avoid deadlock by ensuring that devices can interrupt.
//     intr_on();

//     for (p = proc; p < &proc[NPROC]; p++)
//     {
//       acquire(&p->lock);
//       if (p->state == RUNNABLE)
//       {
//         // Switch to chosen process.  It is the process's job
//         // to release its lock and then reacquire it
//         // before jumping back to us.
//         p->state = RUNNING;
//         c->proc = p;
//         swtch(&c->context, &p->context);

//         // Process is done running for now.
//         // It should have changed its p->state before coming back.
//         c->proc = 0;
//       }
//       release(&p->lock);
//     }
//   }
// }

// Q3.1 >>>

void scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();

  struct thread* th;

  c->proc = 0;
  for (;;)
  {
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on();

    for (p = proc; p < &proc[NPROC]; p++)
    {
      acquire(&p->lock);
      if (p->state == ACTIVE)
      {
        c->proc = p;
        
        for (th = p->threads; th < &p->threads[NTHREAD]; th++)
        {
          acquire(&th->lock);
          if (p->state == RUNNABLE)
          {
            // Switch to chosen thread.  It is the process's job
            // to release its lock and then reacquire it
            // before jumping back to us.
            th->state = RUNNING;
            c->thread = th;
            swtch(&c->context, &th->context);

            // thread is done running for now.
            // It should have changed its th->state before coming back.
            c->thread = 0;
          }
          release(&th->lock);
        }

        c->proc = 0;
      }
      release(&p->lock);
    }
  }
}
// <<< END


// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void sched(void)
{
  int intena;
  struct proc *p = myproc();

  if (!holding(&p->lock))
    panic("sched p->lock");
  if (mycpu()->noff != 1)
    panic("sched locks");
  if (p->state == RUNNING)
    panic("sched running");
  if (intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  swtch(&p->context, &mycpu()->context);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void yield(void)
{
  struct proc *p = myproc();
  acquire(&p->lock);
  p->state = RUNNABLE;
  sched();
  release(&p->lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void forkret(void)
{
  static int first = 1;

  // Still holding p->lock from scheduler.
  release(&myproc()->lock);

  if (first)
  {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    first = 0;
    fsinit(ROOTDEV);
  }

  usertrapret();
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();

  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.

  acquire(&p->lock); //DOC: sleeplock1
  release(lk);

  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  release(&p->lock);
  acquire(lk);
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
void wakeup(void *chan)
{
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++)
  {
    if (p != myproc())
    {
      acquire(&p->lock);
      if (p->state == SLEEPING && p->chan == chan)
      {
        p->state = RUNNABLE;
      }
      release(&p->lock);
    }
  }
}

// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).

//Task 2.2.1
///TODO : think when a failure will be. think if we nned to lock or CAS is also good
int kill(int pid, int signum)
{
  struct proc *p;
  uint sig = 1 << signum;

  for (p = proc; p < &proc[NPROC]; p++)
  {

    acquire(&p->lock);
    if (p->pid == pid)
    {
      if (p->state == ZOMBIE || p->state == UNUSED || p->state == USED)
      {
        return -1;
      }

      p->pendingSigs = (p->pendingSigs | sig);

      if (p->state == SLEEPING)
      {
        // Wake process from sleep().
        p->state = RUNNABLE;
      }
      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int either_copyout(int user_dst, uint64 dst, void *src, uint64 len)
{
  struct proc *p = myproc();
  if (user_dst)
  {
    return copyout(p->pagetable, dst, src, len);
  }
  else
  {
    memmove((char *)dst, src, len);
    return 0;
  }
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
  struct proc *p = myproc();
  if (user_src)
  {
    return copyin(p->pagetable, dst, src, len);
  }
  else
  {
    memmove(dst, (char *)src, len);
    return 0;
  }
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void procdump(void)
{
  static char *states[] = {
      [UNUSED] "unused",
      [SLEEPING] "sleep ",
      [RUNNABLE] "runble",
      [RUNNING] "run   ",
      [ZOMBIE] "zombie"};
  struct proc *p;
  char *state;

  printf("\n");
  for (p = proc; p < &proc[NPROC]; p++)
  {
    if (p->state == UNUSED)
      continue;
    if (p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    printf("%d %s %s", p->pid, state, p->name);
    printf("\n");
  }
}

//TASK 2.1.3
uint sigprocmask(uint sigmask)
{
  struct proc *p = myproc();
  uint prev = p->sigMask;
  p->sigMask = sigmask;
  return prev;
}

//TASK 2.1.4
int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact)
{
  // check whether it is a valid act changing & act is not null
  if (signum == SIGKILL || signum == SIGSTOP || act == 0)
  {
    return -1;
  }
  struct proc *p = myproc();

  if (oldact != 0)
  {
    copyout(p->pagetable, (uint64)&oldact->sa_handler, (char *)&p->sigHandlers[signum], sizeof(p->sigHandlers[signum]));
    copyout(p->pagetable, (uint64)&oldact->sigmask, (char *)&p->handlersmasks[signum], sizeof(p->sigMask));
  }
  copyin(p->pagetable, (char *)&p->sigHandlers[signum], (uint64)&act->sa_handler, sizeof(act->sa_handler));
  copyin(p->pagetable, (char *)&p->handlersmasks[signum], (uint64)&act->sigmask, sizeof(act->sigmask));

  return 0;
}

//TASK 2.1.5
void sigret(void)
{
  printf("in sigret\n");
  struct proc *p = myproc();
  //retreive back our original trapframe and original process's mask
  copyin(p->pagetable,(char *)p->trapframe, (uint64)p->usrTFB,sizeof(struct trapframe));
  p->sigMask = p->maskB;

  //resore satck state as before handeling the signals
  p->trapframe->sp += sizeof(struct trapframe);
  p->handleingsignal = 0;

}

//TODO: Check if enough
void sigstop_handler()
{
  struct proc *p = myproc();
  int sigcontflag = 1 << SIGCONT;
  // if ( p->state == RUNNABLE || p->state == RUNNING || p->state == SLEEPING)
  while ((p->pendingSigs & sigcontflag) == 0)
    yield();
}

void sigkill_handler()
{
  struct proc *p = myproc();
  p->killed = 1;
}

void handleSignal()
{
  

  struct proc *p;
  p = myproc();
  if ((p != 0 )& (p->handleingsignal ==0))
  {
    // if (((tf->cs) & 3) != DPL_USER) {
    //   return;
    // }
    //printf("pending sigs to pid:%d is:%d\n",p->pid,p->pendingSigs);
    for (int i = 0; i < 32; ++i)
    {
      if ((1 << i & p->pendingSigs) && !((1 << i) & p->sigMask) && (p->sigHandlers[i] != (void *)SIG_IGN))
      {
        printf("found signal to deal with. number %d\n ", i);
        if (p->sigHandlers[i] == (void *)SIG_DFL)
        { //kernel space
          printf("found defauld handler for signal: \n ", i);
          if (i == SIGSTOP)
            sigstop_handler();
          else
            sigkill_handler(); // includes the sigkill & default handler
        }
        else
        { 
        //back up the process' mask and change to its signal handler's costum mask
        p->maskB = p->sigMask;
        p->sigMask= p->handlersmasks[i];
        p->handleingsignal = 1;

        //make a space on user's stack to store the curr trapframe and copy
        p->trapframe->sp -= sizeof(struct trapframe);
        p->usrTFB = (struct trapframe* )(p->trapframe->sp);
        copyout(p->pagetable, (uint64)p->usrTFB, (char *)p->trapframe, sizeof(struct trapframe));

        //calculate the injected function size, make space on top of user's stack and copy the system call sigret call .
        uint64 size = (uint64)&sigret_call_end - (uint64)&sigret_call_start;
        p->trapframe->sp -= size;
        copyout(p->pagetable, (uint64)p->trapframe->sp, (char *)&sigret_call_start, size);

        //store argument for signal handler function, we want to return to the place where we injected the system call
        p->trapframe->a0 = i;
        p->trapframe->ra = p->trapframe->sp;
         
        //finally change the program counter to point on the signal handler function gave by the user and turn of this ignal bit
        p->trapframe->epc = (uint64)p->sigHandlers[i];
        p->pendingSigs ^= (1 << i); // Remove the signal from the pending_signals
          
        return;

          
  
        }
      }

    }
  }
  return;
}
