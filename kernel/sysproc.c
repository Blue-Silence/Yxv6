#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "date.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

#define PAGE_SIZE 4096

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


#ifdef LAB_PGTBL
pte_t *
walk(pagetable_t pagetable, uint64 va, int alloc);
void sys_pgaccess(void)
{

  uint64 bitmask = 0;
  uint64 start_addr,buf;
  int page_num;
  argaddr(0,&start_addr);
  argint(1, &page_num);
  argaddr(2,&buf);
  page_num=page_num%64;

  uint64 *pp,pte;


  pagetable_t pgt=myproc()->pagetable;
  
  for(int i=0;i<page_num;i++)
  {
    pp=walk(pgt,start_addr+i*PAGE_SIZE,0);
    pte=*pp;
    if(pte&PTE_A)
      bitmask=bitmask|(1<<i);
    *pp=pte&(~PTE_A);
    
  }
  copyout(pgt,buf,(char *)&bitmask,sizeof(bitmask));
}
#endif

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
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


