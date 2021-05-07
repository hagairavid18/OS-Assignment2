#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"


uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  //TASK 2.3 change kill
  int pid;
  int num;
  if(argint(0, &pid) < 0)
    return -1;
  if(argint(1, &num) < 0)
    return -1;
  return kill(pid,num);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

//TASK 2.1.3
uint64
sys_sigprocmask(void){

  int sigmask;
  if(argint(0, &sigmask) < 0)
    return -1;
  return sigprocmask(sigmask);

}

//TASK 2.1.4
uint64
sys_sigaction(void){

  int signum;
  if(argint(0,&signum)<0){
    return -1;
  }
  struct sigaction* act;
  if(argaddr(1, (void*)&act) < 0)
    return -1;
  
  struct sigaction* oldact;
  if(argaddr(2, (void*)&oldact) < 0)
    return -1;
  
  return sigaction(signum,act, oldact);

}

uint64
sys_sigret(void){
  //printf("insyscall sigret\n");
  //TODO complete
  sigret(); 
  return 0;

}

// <<<<<<<<<<< TASK 3.2

uint64
sys_kthread_create (void) {
  // void ( *start_func ) ( ) , void *stack
  /* 
    TODO: implement function
  */

  uint64 start_func;
  uint64 stack;
  if(argaddr(0, &start_func) < 0)
    return -1;
  if(argaddr(1, &stack) < 0)
    return -1;
  
  return kthread_create((void *) start_func, (void *) stack);
}


uint64
sys_kthread_id(void)
{
  // (void)
  /*  
    TODO: implement function
  */

  return kthread_id();

}


uint64
sys_kthread_exit(void)
{
  int status;
  if(argint(0, &status) < 0)
    return -1;
  
  kthread_exit(status);

  return 0;
}

uint64
sys_kthread_join(void) 
{
   //(int thread_id, int* status)
  /* 
    TODO: implement function
  */

  int thread_id;
  uint64 status;
  if(argint(0, &thread_id) < 0)
    return -1;
  if(argaddr(1, &status) < 0)
    return -1;

  return kthread_join(thread_id, (int *) status);
}

// >>>>>>>>>>>> END

//

uint64 
sys_bsem_alloc(void){
  return bsem_alloc();
}
uint64 sys_bsem_free(void){
  int bsem;
  if(argint(0,&bsem))
    return -1;
  bsem_free(bsem);
  return 0;
}
uint64 sys_bsem_up(void){
  int bsem;
  if(argint(0,&bsem))
    return -1;
  bsem_up(bsem);
  return 0;
}
uint64 sys_bsem_down(void){
  int bsem;
  if(argint(0,&bsem))
    return -1;
  bsem_down(bsem);
  return 0;
}