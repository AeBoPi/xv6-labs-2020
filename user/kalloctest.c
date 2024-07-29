#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/riscv.h"
#include "kernel/memlayout.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define NCHILD 2
#define N 100000
#define SZ 4096

void test1(void);
void test2(void);
char buf[SZ];

int
main(int argc, char *argv[])
{
  test1();
  test2();
  exit(0);
}

int ntas(int print)   // 获取系统的统计信息，并从中提取出某个特定的统计值
{
  int n;
  char *c;

  if (statistics(buf, SZ) <= 0) {   // statistics读取系统统计信息并存储在 buf 中
    fprintf(2, "ntas: no stats\n");
  }
  c = strchr(buf, '=');
  n = atoi(c+2);
  if(print)
    printf("%s", buf);
  return n;
}

/*test1 函数创建 NCHILD 子进程，每个子进程循环 N 次进行 sbrk 分配和释放内存。通过 ntas 函数前后测量页表条目数量变化，判断是否有显著变化*/

void test1(void)    // 进行内存分配和释放测试，并检查页表条目数量变化
{
  void *a, *a1;
  int n, m;
  printf("start test1\n");  
  m = ntas(0);
  for(int i = 0; i < NCHILD; i++){
    int pid = fork();
    if(pid < 0){
      printf("fork failed");
      exit(-1);
    }
    if(pid == 0){
      for(i = 0; i < N; i++) {
        a = sbrk(4096);
        *(int *)(a+4) = 1;
        a1 = sbrk(-4096);
        if (a1 != a + 4096) {
          printf("wrong sbrk\n");
          exit(-1);
        }
      }
      exit(-1);
    }
  }

  for(int i = 0; i < NCHILD; i++){
    wait(0);
  }
  printf("test1 results:\n");
  n = ntas(1);
  if(n-m < 10) 
    printf("test1 OK\n");
  else
    printf("test1 FAIL\n");
}

//
// countfree() from usertests.c
//
/*countfree 函数连续分配内存页，直到分配失败，并记录成功分配的页数，然后释放所有分配的内存*/

int
countfree()     // 统计系统中可用的物理页数
{
  uint64 sz0 = (uint64)sbrk(0);
  int n = 0;

  while(1){
    uint64 a = (uint64) sbrk(4096);
    if(a == 0xffffffffffffffff){
      break;
    }
    // modify the memory to make sure it's really allocated.
    *(char *)(a + 4096 - 1) = 1;
    n += 1;
  }
  sbrk(-((uint64)sbrk(0) - sz0));
  return n;
}

/*test2 函数首先统计系统的空闲页数 free0，然后重复 50 次调用 countfree 并比较每次结果。如果发现内存页数发生变化，则表示有内存泄漏*/

void test2() {                // 重复调用 countfree，检查内存分配和释放的一致性
  int free0 = countfree();
  int free1;
  int n = (PHYSTOP-KERNBASE)/PGSIZE;
  printf("start test2\n");  
  printf("total free number of pages: %d (out of %d)\n", free0, n);
  if(n - free0 > 1000) {
    printf("test2 FAILED: cannot allocate enough memory");
    exit(-1);
  }
  for (int i = 0; i < 50; i++) {
    free1 = countfree();
    if(i % 10 == 9)
      printf(".");
    if(free1 != free0) {
      printf("test2 FAIL: losing pages\n");
      exit(-1);
    }
  }
  printf("\ntest2 OK\n");  
}


