// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"


#define BUCKET_NUM 7
#define HASH(X,Y) (((X%BUCKET_NUM)+(Y%BUCKET_NUM))%BUCKET_NUM)

struct bcache {
  struct spinlock lock;
  struct buf buf[NBUF];
  struct buf head;

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
} bcache_buckets[BUCKET_NUM];
struct spinlock grand_evit_lock;

void
binit(void)
{
  struct buf *b;

  for(int i=0;i<BUCKET_NUM;i++)
  {

    struct bcache *bcache = &bcache_buckets[i];
  
    initlock(&bcache->lock, "bcache_bucket");

    bcache->head.time_stamp = ticks;
    // Create linked list of buffers
    bcache->head.prev = &bcache->head;
    bcache->head.next = &bcache->head;
    for(b = bcache->buf; b < bcache->buf+NBUF; b++){
      b->next = bcache->head.next;
      b->prev = &bcache->head;
      initsleeplock(&b->lock, "bcache_buffer");
      bcache->head.next->prev = b;
      bcache->head.next = b;
    }
  }

  initlock(&grand_evit_lock, "bcache_grand_evit_lock");
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
struct buf* get_buf_in_one_entry(struct bcache *bcache);

static struct buf*
bget(uint dev, uint blockno)
{
  int oi = HASH(dev,blockno);
  struct bcache *bcache = &bcache_buckets[HASH(dev,blockno)];
  struct buf *b;

  acquire(&bcache->lock);

  // Is the block already cached?
  for(b = bcache->head.next; b != &bcache->head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache->lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  release(&bcache->lock);
  acquire(&grand_evit_lock);
  acquire(&bcache->lock);

  b = get_buf_in_one_entry(bcache);
  if(b)
  {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache->lock);
      release(&grand_evit_lock);
      acquiresleep(&b->lock);
      return b;
  }





  for(int i=1;i<BUCKET_NUM;i++)
  {
    if(&bcache_buckets[oi+i] == bcache)
      continue;
    struct bcache *nbcache = &bcache_buckets[i];
    acquire(&nbcache->lock);
    b = get_buf_in_one_entry(nbcache);
    
    if(b)
    {
      if(nbcache->head.next == b)
        nbcache->head.next = b->next;
      if(nbcache->head.prev == b)
        nbcache->head.prev = b->next;
      b->next->prev = b->prev;
      b->prev->next = b->next;

      release(&nbcache->lock);

      b->next = bcache->head.next;
      b->prev = bcache->head.next->prev;
      bcache->head.next->prev = b;
      bcache->head.next = b;
      
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache->lock);
      acquiresleep(&b->lock);
      return b;

    }
    
    release(&nbcache->lock);
  }

  release(&grand_evit_lock);
  

  panic("bget: no buffers");
}

struct buf* get_buf_in_one_entry(struct bcache *bcache){
    for(struct buf* b = bcache->head.prev; b != &bcache->head; b = b->prev)
      if(b->refcnt == 0) 
        return b;

    return 0;
}


// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  struct bcache *bcache = &bcache_buckets[HASH(b->dev,b->blockno)];

  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  acquire(&bcache->lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache->head.next;
    b->prev = &bcache->head;
    bcache->head.next->prev = b;
    bcache->head.next = b;
  }
  
  release(&bcache->lock);
}

void
bpin(struct buf *b) {
  struct bcache *bcache = &bcache_buckets[HASH(b->dev,b->blockno)];

  acquire(&bcache->lock);
  b->refcnt++;
  release(&bcache->lock);
}

void
bunpin(struct buf *b) {
  struct bcache *bcache = &bcache_buckets[HASH(b->dev,b->blockno)];
  
  acquire(&bcache->lock);
  b->refcnt--;
  release(&bcache->lock);
}


