// Mutual exclusion spin locks.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "proc.h"
#include "defs.h"

#ifdef LAB_LOCK
#define NLOCK 500

static struct spinlock *locks[NLOCK];
struct spinlock lock_locks;

void
freelock(struct spinlock *lk)
{
  acquire(&lock_locks);
  int i;
  for (i = 0; i < NLOCK; i++) {
    if(locks[i] == lk) {
      locks[i] = 0;
      break;
    }
  }
  release(&lock_locks);
}

static void
findslot(struct spinlock *lk) {
  acquire(&lock_locks);
  int i;
  for (i = 0; i < NLOCK; i++) {
    if(locks[i] == 0) {
      locks[i] = lk;
      release(&lock_locks);
      return;
    }
  }
  panic("findslot");
}
#endif

void
initlock(struct spinlock *lk, char *name)   
{
  lk->name = name;
  lk->locked = 0;
  lk->cpu = 0;
#ifdef LAB_LOCK
  lk->nts = 0;
  lk->n = 0;
  findslot(lk);
#endif  
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
void
acquire(struct spinlock *lk)
{
  push_off(); // disable interrupts to avoid deadlock.
  if(holding(lk))
    panic("acquire");

#ifdef LAB_LOCK
    __sync_fetch_and_add(&(lk->n), 1);
#endif      

  // On RISC-V, sync_lock_test_and_set turns into an atomic swap:
  //   a5 = 1   这行代码将寄存器a5设置为1。a5寄存器是用来存储要交换的新值
  //   s1 = &lk->locked  这行代码将寄存器s1设置为lk->locked的地址。s1寄存器用来存储锁变量的地址
  //   amoswap.w.aq a5, a5, (s1)    // 这条指令将s1寄存器指向的内存地址（即lk->locked）的值与a5寄存器的值进行交换，并将原来的值存储在a5中
  // 其中.w表示操作是32位的，.aq表示需要获取（acquire）语义，确保后续的内存操作不会在此指令之前执行
  while(__sync_lock_test_and_set(&lk->locked, 1) != 0) {  // spinlock 是一种忙等待锁，特征：线程在获取锁的过程中一直占用CPU进行检查，而不是阻塞等待
#ifdef LAB_LOCK
    __sync_fetch_and_add(&(lk->nts), 1);
#else
   ;
#endif
  }

  // Tell the C compiler and the processor to not move loads or stores
  // past this point, to ensure that the critical section's memory
  // references happen strictly after the lock is acquired.
  // On RISC-V, this emits a fence instruction.
  __sync_synchronize();

  // Record info about lock acquisition for holding() and debugging.
  lk->cpu = mycpu();
}

// Release the lock.
void
release(struct spinlock *lk)
{
  if(!holding(lk))
    panic("release");

  lk->cpu = 0;

  // Tell the C compiler and the CPU to not move loads or stores
  // past this point, to ensure that all the stores in the critical
  // section are visible to other CPUs before the lock is released,
  // and that loads in the critical section occur strictly before
  // the lock is released.
  // On RISC-V, this emits a fence instruction.
  __sync_synchronize();   // __sync_synchronize() 是一个 GCC 提供的内建函数，用于实现全局内存屏障（memory barrier）。内存屏障确保在它之前的所有读写操作在它之后的读写操作之前完成

  // Release the lock, equivalent to lk->locked = 0.
  // This code doesn't use a C assignment, since the C standard
  // implies that an assignment might be implemented with
  // multiple store instructions.
  // On RISC-V, sync_lock_release turns into an atomic swap:
  //   s1 = &lk->locked
  //   amoswap.w zero, zero, (s1)   这条指令将s1寄存器指向的内存地址（即lk->locked）的值与zero寄存器的值进行交换，并将原来的值存储在zero中。zero寄存器总是返回0
  __sync_lock_release(&lk->locked);

  pop_off();
}

// Check whether this cpu is holding the lock.
// Interrupts must be off.
int
holding(struct spinlock *lk)
{
  int r;
  r = (lk->locked && lk->cpu == mycpu());
  return r;
}

// push_off/pop_off are like intr_off()/intr_on() except that they are matched:
// it takes two pop_off()s to undo two push_off()s.  Also, if interrupts
// are initially off, then push_off, pop_off leaves them off.

void
push_off(void)
{
  int old = intr_get();

  intr_off();
  if(mycpu()->noff == 0)
    mycpu()->intena = old;
  mycpu()->noff += 1;
}

void
pop_off(void)
{
  struct cpu *c = mycpu();
  if(intr_get())
    panic("pop_off - interruptible");
  if(c->noff < 1)
    panic("pop_off");
  c->noff -= 1;
  if(c->noff == 0 && c->intena)
    intr_on();
}

#ifdef LAB_LOCK
int
snprint_lock(char *buf, int sz, struct spinlock *lk)
{
  int n = 0;
  if(lk->n > 0) {
    n = snprintf(buf, sz, "lock: %s: #fetch-and-add %d #acquire() %d\n",
                 lk->name, lk->nts, lk->n);
  }
  return n;
}

int
statslock(char *buf, int sz) {
  int n;
  int tot = 0;

  acquire(&lock_locks);
  n = snprintf(buf, sz, "--- lock kmem/bcache stats\n");
  for(int i = 0; i < NLOCK; i++) {
    if(locks[i] == 0)
      break;
    if(strncmp(locks[i]->name, "bcache", strlen("bcache")) == 0 ||
       strncmp(locks[i]->name, "kmem", strlen("kmem")) == 0) {
      tot += locks[i]->nts;
      n += snprint_lock(buf +n, sz-n, locks[i]);
    }
  }
  
  n += snprintf(buf+n, sz-n, "--- top 5 contended locks:\n");
  int last = 100000000;
  // stupid way to compute top 5 contended locks
  for(int t = 0; t < 5; t++) {
    int top = 0;
    for(int i = 0; i < NLOCK; i++) {
      if(locks[i] == 0)
        break;
      if(locks[i]->nts > locks[top]->nts && locks[i]->nts < last) {
        top = i;
      }
    }
    n += snprint_lock(buf+n, sz-n, locks[top]);
    last = locks[top]->nts;
  }
  n += snprintf(buf+n, sz-n, "tot= %d\n", tot);
  release(&lock_locks);  
  return n;
}
#endif
