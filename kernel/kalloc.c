// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct kmem {
  struct spinlock lock;
  int count;
  struct run *freelist;
} a;

struct kmem kmems[NCPU];

void
kinit()
{

  for(int i=0;i<NCPU;i++)
  {
    kmems[i].count = 0;
    initlock(&kmems[i].lock, "kmem");
    kmems[i].freelist = 0;

  }
  
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  push_off();
  struct kmem* k = &kmems[cpuid()];
  pop_off();

  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&k->lock);
  r->next = k->freelist;
  k->freelist = r;
  k->count++;
  release(&k->lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  push_off();
  int id = cpuid();
  pop_off();
  struct run *r;

  for(int i=0;i<NCPU;i++)
  {
    struct kmem *k = &kmems[id];
    acquire(&k->lock);
    r = k->freelist;
    if(r)
    {
      k->freelist = r->next;
      k->count--;
    }  
    release(&k->lock);

    if(r)
      memset((char*)r, 5, PGSIZE); // fill with junk
    if(r)
      return (void*)r;

    id = (id+1)%NCPU;
  }

  return 0;
}
