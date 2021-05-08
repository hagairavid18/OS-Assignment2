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

struct bsem bsem_table[MAX_BSEM]; // Q4.1

struct spinlock bsem_table_lock; // Q4.1

int nextpid = 1;
struct spinlock pid_lock;
// Q3.1
int nexttid = 1;
struct spinlock tid_lock;

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
  struct thread *t;

  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++)
  {
    for (t = p->threads; t < &p->threads[NTHREAD]; t++)
    {
      char *ta = kalloc();
      if (ta == 0)
        panic("kalloc");
      uint64 va = KSTACK(NTHREAD * (int)(p - proc) + (int)(t - p->threads));
      kvmmap(kpgtbl, va, (uint64)ta, PGSIZE, PTE_R | PTE_W);
    }
  }
}

// initialize the proc table at boot time.
void procinit(void)
{
  struct thread *t;

  struct proc *p;

  initlock(&tid_lock, "nexttid");
  initlock(&pid_lock, "nextpid");
  initlock(&wait_lock, "wait_lock");
  for (p = proc; p < &proc[NPROC]; p++)
  {
    initlock(&p->lock, "proc");
    for (t = p->threads; t < &p->threads[NTHREAD]; t++)
    {
      initlock(&t->lock, "thread");
      t->kstack = KSTACK(NTHREAD * (int)(p - proc) + (int)(t - p->threads));
    }
  }

  // Q4.1
  initlock(&bsem_table_lock, "bsem_table_lock");
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

// Q3 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

// alloc thread id (unique id, mostly for debugging)
int alloctid()
{
  int tid;

  //printf("acquire alloctid()\n");
  acquire(&tid_lock);
  tid = nexttid;
  nexttid = nexttid + 1;
  release(&tid_lock);
  //printf("release alloctid()\n");
  return tid;
}

// Return the current struct thread*, or zero if none.
struct thread *
mythread(void)
{
  // TODO: copied from mycpu(), need to be modified
  // push_off();
  struct cpu *c = mycpu();
  struct thread *th = c->thread;
  // pop_off();
  return th;
}

int initthreadstable(struct proc *p)
{
  for (int i = 0; i < NTHREAD; i++)
  {
    struct thread *t = &p->threads[i];
    t->state = UNUSED_THREAD;
    t->chan = 0;
    t->index = i;
    t->procparent = p;
    t->trapframe = (struct trapframe *)p->trapframes + i;
    t->killed = 0;

  }
  return 0;
}

// Unlike allocproc, it initialize a specific thread
// (due to the similarities of allocproc() and kthread_create())
int initthread(struct thread *t)
{
  //printf("initthread() START\n");
  t->tid = alloctid();

  ////printf("initthread() middle\n");

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&t->context, 0, sizeof(t->context));
  t->context.ra = (uint64)forkret;
  t->context.sp = t->kstack + PGSIZE;

  //printf("initthread end\n");
  return 0;
}

int freethread(struct thread *th)
{

  th->tid = -1;
  th->killed = 0;
  th->xstate = 0;
  th->state = UNUSED_THREAD;

  return 0;
}


int mytid(void){
  return mythread()->tid; 
}

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> END

int allocpid()
{
  int pid;

  //printf("acquire allocpid()\n");
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
    //printf("acquire allocproc() 209 proc=%p\n", p);
    acquire(&p->lock);
    if (p->state == UNUSED)
    {
      goto found;
    }
    else
    {
      //printf("release allocproc() 217 proc=%p\n", p);
      release(&p->lock);
    }
  }
  return 0;

found:
  p->pid = allocpid();
  p->state = USED;
  p->killed = 0; //TODO check. i added it

  // Allocate a trapframe page.
  if ((p->trapframes = kalloc()) == 0)
  {
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if (p->pagetable == 0)
  {
    printf("allocproc() p->pagetable failed 240\n");
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // Q3 >>>
  //TODO: finish!

  initthreadstable(p);
  acquire(&p->threads[0].lock);
  initthread(&p->threads[0]);

  // Q2 >>>
  // TODO: what about mask & pending init?

  int i;
  for (i = 0; i < 32; i++)
  {
    p->sigHandlers[i] = (void *)SIG_DFL; // set default flag for all signals
    p->handlersmasks[i] = 0;
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
  // Q3.1
  //there is one allocated trapframe for the whole proc, so kfree just for the proc. 
  if (p->trapframes)
    kfree((void *)p->trapframes);
  struct thread *th;
  // free all threads:
  // TODO: should I acquire each thread?
  for (th = p->threads; th < &p->threads[NTHREAD]; th++)
  {
    freethread(th); // clean thread
  }

  if (p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);
  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
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
  // TODO: is hreads[0] the right choice?
  if (mappages(pagetable, TRAPFRAME, PGSIZE,
               (uint64)(p->trapframes), PTE_R | PTE_W) < 0)
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
  p->threads[0].trapframe->epc = 0;     // user program counter
  p->threads[0].trapframe->sp = PGSIZE; // user stack pointer

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = ACTIVE; //Q3.1
  p->threads[0].state = RUNNABLE;
  //printf("release userinit() 377 proc=%p\n", p);
  release(&p->lock);
  release(&p->threads[0].lock);
}

// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int growproc(int n)
{
  uint sz;
  struct proc *p = myproc();
  //Q3.1
  // Preventing syncronization problems by locking on proc->lock:
  //TODO: acquire (&p->lock) caused panics in usertests, so need somthing else
  //printf("acquire growproc() proc=%p\n", p);
  //acquire(&p->lock);

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

  //printf("release growproc() proc=%p\n", p);
  //release(&p->lock);
  return 0;
}


int fork(void)
{
  //printf("fork() START\n");
  int i, pid;
  struct proc *np;
  struct proc *p = myproc();
  struct thread *th = mythread();

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

  //printf("acquire fork()\n");
  acquire(&wait_lock); //TODO: Check wich lock should be accuired!

  // copy trapframe from the thread who called fork()
  *np->threads[0].trapframe = *th->trapframe;
  // Cause fork to return 0 in the child.
  np->threads[0].trapframe->a0 = 0;

  release(&wait_lock); //TODO: Check wich lock should be accuired!
  //printf("release fork()\n");

  // increment reference counts on open file descriptors.
  for (i = 0; i < NOFILE; i++)
    if (p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = idup(p->cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;

  release(&np->lock);

  //printf("acquire wait_lock fork()\n");
  acquire(&wait_lock);
  np->parent = p;
  release(&wait_lock);

  //printf("acquire fork() proc=%p\n", np);
  acquire(&np->lock);

  np->state = ACTIVE;              // Q3.1
  np->threads[0].state = RUNNABLE; //TODO: I think it is not the right place

  //TASK 2.1.2
  for (i = 0; i < 32; i++)
  {
    np->sigHandlers[i] = p->sigHandlers[i];
    np->handlersmasks[i] = p->handlersmasks[i];
  }
  np->sigMask = p->sigMask;
  //printf("release fork() 544 proc=%p\n", np);
  release(&np->lock);
  release(&np->threads[0].lock);
  //printf("end fork  p is %p \n",np);//TODO delete

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

  // printf("in exit i am %p,my parent is %p\n",myproc(),myproc()->parent); //TODO delete
  struct proc *p = myproc();
  struct thread *ct = mythread();
  struct thread *t;

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

  //tell all children to get killed.:
  acquire(&p->lock);
  for (t = p->threads; t < &p->threads[NTHREAD]; t++)
  {
    if (t != ct)
    {
      ////printf("about to acquire t\n");
      acquire(&t->lock);
      //printf("acquired t\n");
      if (t->state != UNUSED_THREAD)
      {
        t->killed = 1;
        //printf("killed someone\n");
      }
      if (t->state == SLEEPING)
        t->state = RUNNABLE;
      release(&t->lock);
    }
  }
  release(&p->lock);

  //assure all children are killed.:

  int is_alive = 1;
  while (is_alive)
  {

    is_alive = 0;
    for (t = p->threads; t < &p->threads[NTHREAD]; t++)
    {
      if (t != ct)
      {
        if (t->state != ZOMBIE_THREAD && t->state != UNUSED_THREAD)
          is_alive = 1;
      }
    }
  }

  // printf("acquire exit()\n");
  acquire(&wait_lock);

  // Give any children to init.
  reparent(p);

  //printf("before wakeup father i am %p. channel is %p\n",myproc(),p->parent);//TOTO delete
  // Parent might be sleeping in wait().
  wakeup(p->parent);

  //printf("acquire exit()\n");
  acquire(&p->lock);

  p->xstate = status;
  p->state = ZOMBIE;

  // Q3.1
  ct->xstate = status;
  ct->state = ZOMBIE_THREAD;

  release(&p->lock);

  acquire(&ct->lock);

  release(&wait_lock);

  //printf("at end of exit. i am t: %p my father is %p\n",myproc(),myproc()->parent);

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

  //printf("in wait function ' before acqire waitlock\n");
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
        //printf("found child in wait()\n");
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
          //printf("in wait before freeproc\n");
          freeproc(np);
          //printf("in wait after freeproc i am %p my proc is %p\n",mythread(),myproc());

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
    //printf("in wait before sleep. t: %p on channel %p\n",mythread(),myproc());

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


// Q3.1 >>>

void scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();

  struct thread *th;

  c->proc = 0;
  for (;;)
  {
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on();

    for (p = proc; p < &proc[NPROC]; p++)
    {
      //printf("acquire scheduler() 734 proc=%p\n", p);
      // acquire(&p->lock);
      if (p->state == ACTIVE)
      {
        //printf("\nfound ACTIVE proccess %p\n",p);

        for (th = p->threads; th < &p->threads[NTHREAD]; th++)
        {
          //printf("  acquire scheduler() 742 thread=%p\n", th);
          acquire(&th->lock);
          if (th->state == RUNNABLE)
          {
            // printf("    found RUNNABLE thread %p\n", th);
            // Switch to chosen thread.  It is the process's job
            // to release its lock and then reacquire it
            // before jumping back to us.
            th->state = RUNNING;
            c->thread = th;
            c->proc = p;

            swtch(&c->context, &th->context);

            // thread is done running for now.
            // It should have changed its th->state before coming back.
            c->thread = 0;
            c->proc = 0;

            //printf("    done with RUNNABLE thread %p\n",th);
          }
          //printf("  release scheduler() 757 thread=%p\n", th);
          release(&th->lock);
          // printf("release 2\n");
        }
        //printf("done with ACTIVE proccess %p\n\n",p);
      }
      // printf("release scheduler() 763 proc=%p\n", p);
      // release(&p->lock);
    }
  }

  printf("sheduler() END");
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
  // struct proc *p = myproc();
  struct thread *t = mythread();

  //printf("DEBUG: mycpu()->noff=%d\n", mycpu()->noff);

  if (!holding(&t->lock))
    panic("sched p->lock");
  if (mycpu()->noff != 1)
    panic("sched locks");
  if (t->state == RUNNING)
    panic("sched running");
  if (intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  swtch(&t->context, &mycpu()->context);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void yield(void)
{
  struct thread *t = mythread();
  //printf("acquire yield() 802 thread=%p\n", mythread());
  acquire(&t->lock);
  t->state = RUNNABLE;
  sched();
  //printf("release yield() 802 thread=%p\n", mythread());
  release(&t->lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void forkret(void)
{
  //printf("forkret() START\n");
  static int first = 1;

  // Still holding p->lock from scheduler.
  //printf("release forkret() 822 thread=%p\n", mythread());
  release(&mythread()->lock);
  // printf("release forkret() 824 proc=%p\n",myproc());
  // release(&myproc()->lock);

  if (first)
  {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    first = 0;
    fsinit(ROOTDEV);
  }
  // printf("trapframe is at: %p. a0 is %d\n",mythread()->trapframe,*(mythread()->trapframe+40));
  // printf("forkret() END\n");
  // printf("in forkret i am %p, my index is %d epc is %p\n",mythread(),mythread()->index,mythread()->trapframe->epc);
  // printf("dis is %p\n",mythread()->index * sizeof(struct trapframe));
  //   //printf("dis is %p\n",mythread()->trapframe- myproc()->trapframe);
  //       printf("dis is %p\n",(uint64)mythread()->trapframe-(uint64)myproc()->threads[0].trapframe);
  //               printf("dis is %p\n",mythread()->trapframe-mythread()->procparent->threads[0].trapframe);

  //       printf("a0 of 1 - a0 of 2 %p\n",&mythread()->trapframe->a0-&myproc()->threads[0].trapframe->a0);

  usertrapret();
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void sleep(void *chan, struct spinlock *lk)
{

  // Q3.1 - changed from proc to t
  struct thread *t = mythread();

  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.
  //printf("thread %p sleep on %p\n",mythread(),chan);
  //printf("acquire sleep() 847 thread=%p\n on channel %p", mythread(),chan);
  acquire(&t->lock); //DOC: sleeplock1
  //printf("realse sleep() 849 lk = %s\n", lk->name);
  release(lk);

  // Go to sleep.
  t->chan = chan;
  t->state = SLEEPING;

  sched();

  // Tidy up.
  t->chan = 0;

  // Reacquire original lock.
  //printf("relaease sleep() 862 thread=%p\n", mythread());
  release(&t->lock);

  //printf("acquire sleep() lk");
  acquire(lk);
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
void wakeup(void *chan)
{

  //printf("in wake up ny thread : %p on channel %p",mythread(),chan);//TODO delete
  /*
  Q3.1:
  Due to the thread implementation, an inner iteration is added for each proccess,
  waking up each thread sleeping on the channel
  */
  struct thread *t;
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++)
  {
    if (p->state == ACTIVE)
    {
      //printf("acquire wakeup() 885 proc=%p\n", p);
      acquire(&p->lock);
      //printf("acquired wakeup() 885 proc=%p\n", p);

      for (t = p->threads; t < &p->threads[NTHREAD]; t++)
      {
        if (t != mythread())
        {

          //printf("acquire wakeup() 890 thread=%p\n", t);
          acquire(&t->lock);
          if (t->state == SLEEPING && t->chan == chan)
          {
            //printf("woke up thread %p\n", t);//TODO delete
            t->state = RUNNABLE;
          }
          //printf("release wakeup() 896 thread=%p\n", t);
          release(&t->lock);
        }
      }

      //printf("release wakeup() 900 proc=%p\n", p);
      release(&p->lock); //TODO: added 30.4, not sure if needed
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
  //printf("in kill. pid is %d, signum is %d\n",pid,signum);
  struct proc *p;
  uint sig = 1 << signum;

  for (p = proc; p < &proc[NPROC]; p++)
  {

    //printf("acquire kill() 920 proc=%p\n", myproc());
    acquire(&p->lock);
    if (p->pid == pid)
    {
      if (p->state == ZOMBIE || p->state == UNUSED || p->state == USED)
      {
        return -1;
      }
      //printf("changed pending for proc in kill\n");
      p->pendingSigs = (p->pendingSigs | sig);

      // Q3.1: changed implementation
      // Insure at least 1 thread won't be blocked / sleeping, to handle the signal

      // if (p->state == SLEEPING)
      // {
      //   // Wake process from sleep().
      //   p->state = RUNNABLE;
      // }

      // TODO: watch out from deadlocks (what happens if it sends signal to itself?)
      struct thread *t;
      for (t = p->threads; t < &p->threads[NTHREAD]; t++)
      {
        if (t == mythread())
          break; //TODO: CHECK LINE!

        //printf("acquire kill() 946 %d\n", mythread());
        acquire(&t->lock);

        if (t->state == RUNNABLE || t->state == RUNNING)
        {
          release(&t->lock);
          break;
        }

        else if (t->state == SLEEPING)
        {
          // Wake one thread from sleep().
          t->state = RUNNABLE;
          release(&t->lock);
          break;
        }
        //printf("release kill() 957 thread%d\n", mythread());
        release(&t->lock);
      }

      //printf("release kill() 962 thread%d\n", myproc());
      release(&p->lock);
      return 0;
    }
    //printf("release kill() 965 proc=%d\n", myproc());
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
  acquire(&p->lock);
  uint prev = p->sigMask;
  //In sigprocmask, It is not possible to block SIGKILL or SIGSTOP. Attempts to do so are silently ignored.
  //In this case, you should not update the process signal mask, but return the old (and current) mask.

  if ((sigmask & 1 << SIGKILL) == 0 && (sigmask & 1 << SIGSTOP) == 0)
  {
    // printf("in mask. new is %d\n",sigmask); //TODO delete
    // printf("prev is &d\n",prev); //TODO delete
    p->sigMask = sigmask;
  }
  release(&p->lock);
  return prev;
}

//TASK 2.1.4
int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact)
{

  struct proc *p = myproc();
  int new_mask;
  acquire(&p->lock);

  // check whether it is a valid act changing & act is not null
  if (signum == SIGKILL || signum == SIGSTOP || signum > 31 || signum < 0)
  {
    release(&p->lock);

    return -1;
  }
  // check whether the user try to block SIGKILL or SIGSTOP

  //"If act is null, you should check oldact: If oldact is non-NULL, the previous action is saved in oldact."

  if (act != 0)
  {

    copyin(p->pagetable, (char *)&new_mask, (uint64)&act->sigmask, sizeof(act->sigmask));
    if (new_mask & 1 << SIGKILL || new_mask & 1 << SIGSTOP)
    {
      release(&p->lock);

      return -1;
    }
  }
  // copyout previes act
  if (oldact != 0)
  {
    copyout(p->pagetable, (uint64)&oldact->sa_handler, (char *)&p->sigHandlers[signum], sizeof(p->sigHandlers[signum]));
    copyout(p->pagetable, (uint64)&oldact->sigmask, (char *)&p->handlersmasks[signum], sizeof(p->sigMask));
  }

  if (act != 0)
  {
    copyin(p->pagetable, (char *)&p->sigHandlers[signum], (uint64)&act->sa_handler, sizeof(act->sa_handler));
    p->handlersmasks[signum] = new_mask;
  }
  release(&p->lock);

  return 0;
}

//TASK 2.1.5
void sigret(void)
{

  //printf("in sigret\n")sssssssss;
  struct proc *p = myproc();
  struct thread *t = mythread();
  acquire(&p->lock);
  acquire(&t->lock);
  //retreive back our original trapframe and original process's mask
  //printf("curr epc is: %p\n", p->trapframe->epc);
  copyin(p->pagetable, (char *)t->trapframe, (uint64)t->usrTFB, sizeof(struct trapframe));
  p->sigMask = p->maskB;
  //printf("curr epc is: %p\n", p->trapframe->epc);

  //resore satck state as before handeling the signals
  t->trapframe->sp += sizeof(struct trapframe);
  p->handleingsignal = 0;
  release(&p->lock);
  release(&t->lock);
  // printf("finished sigret\n");
}

//TODO: Check if enough
void sigstop_handler()
{
  struct proc *p = myproc();
  printf("in sigstop\n");//TODO delte

  //By default, the process will wait to SIGCONT signal, and if you register for some signal the SIGCONT handler,
  // it should wait for the SIGCONT signal (if its signal handler is still SIGCONT default handler), or to the newly registered signal.
  while (1)
  {
    yield();
    if (((p->pendingSigs & 1 << SIGCONT) == 0) || ((p->pendingSigs & 1 << SIGCONT) && ((p->sigMask & 1 << SIGCONT) == 0) && (p->sigHandlers[SIGCONT] == 0)))
    {
      return;
    }

    for (int i = 0; i < 32; i++)
    {
      if ((p->sigHandlers[i] == (void *)SIGCONT) && (p->pendingSigs & 1 << i) && ((p->sigMask & 1 << i) == 0))
      {
        return;
      }
    }
    if (p->pendingSigs & 1 << SIGKILL)
    {
      return;
    }
  }
}

void sigkill_handler()
{
  printf("in sigkill\n"); //TODO delete
  struct proc *p = myproc();
  p->killed = 1;
}

void handleSignal()
{

  struct proc *p;
  struct thread *t = mythread();
  p = myproc();
  if ((p != 0) && p->handleingsignal == 0)
  {

    for (int i = 0; i < 32; ++i)
    {
      if ((1 << i & p->pendingSigs) && !((1 << i) & p->sigMask))
      {
        //printf(" i am %d found signal to deal with, %d handler is %p\n",p->pid,i,p->sigHandlers[i]);
        if (p->sigHandlers[i] == (void *)SIG_IGN)
        {
          p->pendingSigs ^= (1 << i); // Remove the signal from the pending_signals
          return;
        }
        //printf("found signal to deal with. number %d\n ", i);
        //kernel space
        //printf("found defauld handler for signal: \n ", i);

        if (i == SIGSTOP || p->sigHandlers[i] == (void *)SIGSTOP)
        {
          p->maskB = p->sigMask;
          p->sigMask = p->handlersmasks[i];
          sigstop_handler();
          p->sigMask = p->maskB;
          p->pendingSigs ^= (1 << i);
          return;
        }
        else if (i == SIGKILL || p->sigHandlers[i] == (void *)SIGKILL)
        {
          p->maskB = p->sigMask;
          p->sigMask = p->handlersmasks[i];
          sigkill_handler();
          p->sigMask = p->maskB;

          p->pendingSigs ^= (1 << i);
          return;
        }
        else if ((i == SIGCONT && p->sigHandlers[i] == (void *)SIG_DFL) || p->sigHandlers[i] == (void *)SIGCONT)
        {
          p->pendingSigs ^= (1 << i); // Remove the signal from the pending_signals
          return;
        }

        else if (p->sigHandlers[i] == (void *)SIG_DFL)
        {
          p->maskB = p->sigMask;
          p->sigMask = p->handlersmasks[i];
          sigkill_handler();
          p->sigMask = p->maskB;

          p->pendingSigs ^= (1 << i); // Remove the signal from the pending_signals
          return;
        }
        else if (p->handleingsignal == 0)
        {

          //printf("in user handling case\n");
          //now check for kernelspace handlers setted by user.
          acquire(&p->lock);
          acquire(&t->lock);
          //back up the process' mask and change to its signal handler's costum mask OR the process mask
          p->maskB = p->sigMask;
          p->sigMask = p->handlersmasks[i];
          p->handleingsignal = 1;

          //make a space on user's stack to store the curr trapframe and copy
          t->trapframe->sp -= sizeof(struct trapframe);
          t->usrTFB = (struct trapframe *)(t->trapframe->sp);
          copyout(p->pagetable, (uint64)t->usrTFB, (char *)t->trapframe, sizeof(struct trapframe));

          //calculate the injected function size, make space on top of user's stack and copy the system call sigret call .
          uint64 size = (uint64)&sigret_call_end - (uint64)&sigret_call_start;
          t->trapframe->sp -= size;
          copyout(p->pagetable, (uint64)t->trapframe->sp, (char *)&sigret_call_start, size);

          //store argument for signal handler function, we want to return to the place where we injected the system call
          t->trapframe->a0 = i;
          t->trapframe->ra = t->trapframe->sp;

          //finally change the program counter to point on the signal handler function gave by the user and turn of this ignal bit
          t->trapframe->epc = (uint64)p->sigHandlers[i];
          p->pendingSigs ^= (1 << i); // Remove the signal from the pending_signals
          release(&p->lock);
          release(&t->lock);
          return;
        }
      }
    }
  }
  return;
}

// <<<<<<<<<<<<<< Q3.2

int kthread_create(void (*start_func)(), void *stack)
{
  //printf("in kthread_create p: %p t: %p or %d\n",myproc(),mythread(),mythread()->index);

  struct proc *p = myproc();
  struct thread *t;
  //printf("about to acquire p->lock in kthread_create %p id : d\n",mythread(),mythread()->index);

  acquire(&p->lock); //probably vital, as we dont want the process to be freed during create a new thread.
  
  int i = 0; // will denote the index in the table for the new created thread

  //check for an empty spot in p->threads to allocate a new thread
  for (t = p->threads; t < &p->threads[NTHREAD]; t++)
  {
    //printf("about to acquire in kthread_create %p id : d\n",t,t->index);
    acquire(&t->lock);
    //printf(" acquired in kthread_create %p id : d\n",t,t->index);

    if (t->state == UNUSED_THREAD)
    {
      //printf("found thread_is %p\n", t);
      //important to change to used. we unlock the p, so now the process can again try to create a new thread. so we want to tell him that this slot is already took.
      // maybe used it s not neccessery due to the locked t we hold.
      t->state = USED_THRAEAD;
      release(&p->lock);
      goto found;
    }
    else
    {
      release(&t->lock);
    }
    i++;
  }
  // in case all slots are full, return -1 for a failure.
  release(&p->lock);
  return -1;

found:
  //printf("in kthread_create p: %p, found unused thread,:%d my thread is %d\n",myproc(), i, mythread()->index);

  if (initthread(t) < 0) //TODO initthread can not return somthing else than 0, so maybe change here
  {
    // failure
    t->state = UNUSED;
    freethread(t);
    release(&t->lock);
    return -1;
  }
  //printf("t-p->threads is %d\n", t - p->threads);

  //copy the curr thread trapframe to the new thread and change its registers
  *(t->trapframe) = *(mythread()->trapframe);
  t->trapframe->sp = (uint64)stack + STACK_SIZE - 16;
  t->trapframe->epc = (uint64)start_func;

  // now we can finaly change to runnable, after we finished the creation
  t->state = RUNNABLE; //TODO: I'm not sure it is not the right place
  //printf("in kthread_create, before return\n");

  release(&t->lock);
  return t->tid;
}

int kthread_id(void)
{
  return mythread()->tid;
}

void kthread_exit(int status)
{
  //printf("in kthread exit,p: %p t: %p id is: %d\n", myproc(), mythread(), mythread()->index);
  struct proc *p = myproc();
  struct thread *t = mythread();

  struct thread *nt;
  int found = 0;


  //here we check if we are the last thread that alive. its important to change to "zombie" right before we leave the function,
  //in addition we may be the last thread so we dont want to change to "zombie" until we assure there are still alive thread.
  //very important, to call "wake up" AFTER we change to zombie state, otherwise kthread join may miss this thread.
  //lock here its vital to ensure two threads think they are not the last.
  acquire(&p->lock);
  for (nt = p->threads; nt < &p->threads[NTHREAD]; nt++)
  {
    if (nt != t)
    {
      //printf("in kthread exit, bfore acquire %p id: %d \n", nt,mythread()->index);
      acquire(&nt->lock);
      //printf("in kthread exit, acquired %p id: %d \n", nt,mythread()->index);
      if (nt->state != UNUSED_THREAD && nt->state != ZOMBIE_THREAD)
      {
        //printf("kthread exit, found alive t: %p his status %d\n", nt,nt->state);
        found = 1;
        release(&nt->lock);
        break;
      }
      release(&nt->lock);
    }
  }
  if (found == 0)
  {
    //in case we are the last thread, we will perform exit(), so meanwhile dont change to ZOMBIE
    
    acquire(&t->lock);
    t->xstate = status;
    release(&t->lock);

    //imporant to release just after we change the status. now we can let another thread to check if he is the last

    release(&p->lock);

    //printf("e\n");//TODO delete
    wakeup(t); //TODO: maybe unnecessery, because if we are the last, no one should wait for us.

    exit(status);
  }
  else
  {
    //printf("in kthread exit p: %p t: %p  found alive threads\n", myproc(),t);
    
    // in case some threads still alive, we change to zombie and then exit.
    acquire(&t->lock);
    t->xstate = status;
    t->state = ZOMBIE_THREAD;
    release(&t->lock);

    //imporant to release just after we change the status. now we can let another thread to check if he is the last
    release(&p->lock);

    //wake up should be called without any locks. 
    wakeup(t);
    acquire(&t->lock);
    //printf("e\n");//TODO delte
      

    sched();
    panic("zombie thread exit");
  }
  
  return;
}

int kthread_join(int thread_id, int *status)
{
  //TODO: how we should treat the status?
  //printf("in kthread join wait for id: %d\n", thread_id);
  //"kthread join is between threads in the same process."
  //"in kthread_join should we free thread thread_id that we wait to exit?" "Yes, you should free it."
  struct proc *p = myproc();
  struct thread *t = mythread();

  struct thread *nt;
  int found;


  // go over table and check for the specified thread_id .
  found = 0;
  for (nt = p->threads; nt < &p->threads[NTHREAD]; nt++)
  {
    //printf("nt id is %d\n", nt->tid);//TODO delete

    acquire(&nt->lock);
    if (nt->tid == thread_id && nt != t)
    {
      //printf("in kthread join, found thread ! his id is %p\n", nt);

      found = 1;
      release(&nt->lock);

      break;
    }
    release(&nt->lock);
  }
  if (!found)
  {
    return -1;
  }
  acquire(&wait_lock);

  for (;;)
  {
    //printf("in kthread join, in main loop. my id is %d\n", t->tid);

    acquire(&nt->lock);

    //in case the thread has already been freed or reallocated
    if (t->killed == 1 || nt->tid != thread_id)
    {

      release(&nt->lock);
      release(&wait_lock);

      return -1;
    }

    if (nt->state == ZOMBIE_THREAD)
    {
      freethread(nt);

      release(&nt->lock);
      release(&wait_lock);

      return 0;
    }
    //printf("in kthread join, before sleep. my id is %d\n", t->tid);
    //in case the thread is still alive, wait until it becomes dead. it will wake up us in kthread_exit
    release(&nt->lock);
    sleep(nt, &wait_lock);
  }
}


// <<<<<<<<<<<<<<<<< Task 4.1 <<<<<<<<<<<<<<<<<

// Allocates a new binary semaphore and returns its descriptor(-1 if failure). You are not
// restricted on the binary semaphore internal structure, but the newly allocated binary
// semaphore should be in unlocked state.
int bsem_alloc(void){
  acquire(&bsem_table_lock);
  
  int table_index = 0;
  struct bsem *b; // Semaphore descriptor
  
  for (b = bsem_table; b < &bsem_table[MAX_BSEM]; b++){
    acquire(&b->lock);
    
    if (b->state == FREE){
      b->state = UNLOCKED;
      release(&b->lock);
      release(&bsem_table_lock); 
      return table_index;
    }
    
    release(&b->lock);
    table_index++;
  }
  
  release(&bsem_table_lock);
  return -1; // in case no avalable semaphore found
}

// Frees the binary semaphore with the given descriptor. Note that the behavior of
// freeing a semaphore while other threads “blocked” because of it is undefined and
// should not be supported.
void bsem_free(int ds){
  if ( ds < 0 || ds >= MAX_BSEM || bsem_table[ds].state == FREE) return; // invalid cases
  struct bsem* b = &bsem_table[ds];

  acquire(&bsem_table_lock);
  acquire(&b->lock);
  bsem_table[ds].state = FREE;
  release(&b->lock);
  release(&bsem_table_lock);

  /*
    Note that the behavior of
    freeing a semaphore while other threads “blocked” because of it is undefined and
    should is not supported.
  */
  return;
}

// Attempt to acquire (lock) the semaphore, in case that it is already acquired (locked),
// block the current thread until it is unlocked and then acquire it.
void bsem_down(int ds){
  if ( ds < 0 || ds >= MAX_BSEM || bsem_table[ds].state == FREE) return; // invalid cases
  
  struct bsem* b = &bsem_table[ds];
  acquire(&b->lock);
  while (b->state == LOCKED){
    sleep(b, &b->lock);
  }
  b->state = LOCKED;
  
  release(&b->lock);
  return;
}

// Attempt to acquire (lock) the semaphore, in case that it is already acquired (locked),
// block the current thread until it is unlocked and then acquire it.
void bsem_up(int ds){
  if ( ds < 0 || ds >= MAX_BSEM || bsem_table[ds].state == FREE) return; // invalid cases
  
  struct bsem* b = &bsem_table[ds];
  acquire(&b->lock);
  b->state = UNLOCKED; // Back to unlocked state
  release(&b->lock);
  wakeup(b);
  
  return;
}

// <<<<<<<<<<<<<<< Task 4.1 END <<<<<<<<<<<<<<<