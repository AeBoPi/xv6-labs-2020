// Sleeping locks

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sleeplock.h"

void
initsleeplock(struct sleeplock *lk, char *name)
{
  initlock(&lk->lk, "sleep lock");
  lk->name = name;
  lk->locked = 0;
  lk->pid = 0;
}

void
acquiresleep(struct sleeplock *lk)    // sleeplock 是一种无忙等待锁，包含了一个自旋锁struct spinlock lk来保护睡眠锁本身的状态，但是它的主要机制是让线程在无法获取锁时进入睡眠状态，而不是忙等待，从而节省CPU资源
{
  acquire(&lk->lk);   // 获取保护睡眠锁的自旋锁
  while (lk->locked) {  // 如果锁被持有，进程进入睡眠状态
    sleep(lk, &lk->lk);
  }
  lk->locked = 1; // 成功获取睡眠锁
  lk->pid = myproc()->pid;
  release(&lk->lk); // 释放保护睡眠锁的自旋锁
}

void
releasesleep(struct sleeplock *lk)
{
  acquire(&lk->lk); // 获取保护睡眠锁的自旋锁
  lk->locked = 0;
  lk->pid = 0;
  wakeup(lk); // 唤醒等待获取睡眠锁的进程
  release(&lk->lk);   // 释放保护睡眠锁的自旋锁
}

int
holdingsleep(struct sleeplock *lk)
{
  int r;
  
  acquire(&lk->lk);
  r = lk->locked && (lk->pid == myproc()->pid);
  release(&lk->lk);
  return r;
}



