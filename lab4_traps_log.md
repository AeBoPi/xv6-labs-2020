## exp1 RISC-V assembly(easy)
实验目的：熟悉 RISC-V 指令集

~~~
// call.asm 汇编代码解释
0000000000000000 <g>:
   0:	1141                	addi	sp,sp,-16       ; 为栈帧分配空间,得到一个 16 字节的栈帧
   2:	e422                	sd	s0,8(sp)        ; 保存 s0 寄存器
   4:	0800                	addi	s0,sp,16       ; 设置 s0 为当前栈帧的基地址
   6:	250d                	addiw	a0,a0,3        ; 将传入参数 x 加 3
   8:	6422                	ld	s0,8(sp)        ; 恢复 s0 寄存器
   a:	0141                	addi	sp,sp,16       ; 释放栈帧空间,回收栈帧，
   c:	8082                	ret                ; 返回

// 对应的 C 代码
int g(int x) {
  return x + 3;
}

000000000000000e <f>:
   e:	1141                	addi	sp,sp,-16       ; 为栈帧分配空间
  10:	e422                	sd	s0,8(sp)        ; 保存 s0 寄存器
  12:	0800                	addi	s0,sp,16       ; 设置 s0 为当前栈帧的基地址
  14:	250d                	addiw	a0,a0,3        ; 将传入参数 x 加 3
  16:	6422                	ld	s0,8(sp)        ; 恢复 s0 寄存器
  18:	0141                	addi	sp,sp,16       ; 释放栈帧空间
  1a:	8082                	ret                ; 返回

// 对应的 C 代码
int f(int x) {
  return g(x);
}

000000000000001c <main>:
  1c:	1141                	addi	sp,sp,-16       ; 为栈帧分配空间
  1e:	e406                	sd	ra,8(sp)        ; 保存返回地址 ra
  20:	e022                	sd	s0,0(sp)        ; 保存 s0 寄存器
  22:	0800                	addi	s0,sp,16       ; 设置 s0 为当前栈帧的基地址
  24:	4635                	li	a2,13           ; 加载立即数 13 到 a2
  26:	45b1                	li	a1,12           ; 加载立即数 12 到 a1 (f(8) + 1 的结果)
  28:	00000517          	auipc	a0,0x0          ; 设置 a0 为 printf 的地址高 20 位
  2c:	7b050513          	addi	a0,a0,1968       ; 设置 a0 为 printf 的地址低 12 位
  30:	00000097          	auipc	ra,0x0          ; 设置 ra 为 printf 返回地址的高 20 位
  34:	600080e7          	jalr	1536(ra)         ; 跳转并链接到 printf
  38:	4501                	li	a0,0            ; 加载立即数 0 到 a0 (exit 的参数)
  3a:	00000097          	auipc	ra,0x0          ; 设置 ra 为 exit 返回地址的高 20 位
  3e:	27e080e7          	jalr	638(ra)          ; 跳转并链接到 exit

// 对应的 C 代码
void main(void) {
  printf("%d %d\n", f(8) + 1, 13);
  exit(0);
}
~~~

**主要注意点**
>
汇编代码中的 addi, sd, ld, li, auipc, jalr 等指令是 RISC-V 指令。

栈指针 sp 和基指针 s0 用于管理函数的栈帧。

注意栈的生长方向是从高地址到低地址，所以扩张是 -16，而回收是 +16

printf 和 exit 函数通过跳转链接指令 jalr 调用。
>

## exp2 Backtrace(moderate)
回溯(Backtrace)通常对于调试很有用：它是一个存放于栈上用于指示错误发生位置的函数调用列表。
实验目的：添加 backtrace 功能，打印出调用栈，用于调试。
步骤：
>
1. defs.h 中添加函数声明；
2. riscv.h 中中添加获取当前 fp（frame pointer）寄存器的方法；
3. printf.c 中实现backtrace 函数；
4. 在 sys_sleep 函数(sysproc.c)的开头调用一次backtrace()。
>

### exp3 Alarm(hard)
实验目的：实现向 xv6 添加一个特性，在进程使用 CPU 时间内，xv6 定期向进程发出警报。
## 步骤
>
1. 在 user/user.h 添加函数声明，更新user/usys.pl（此文件生成user/usys.S）、kernel/syscall.h和kernel/syscall.c以允许alarmtest调用sigalarm和sigreturn系统调用；
2. 在 proc 结构体的定义中增加 alarm 相关字段；
3. 在 sysproc.c 中实现 sys_sigalarm 和 sys_sigreturn, 在trap.c 中实现 sigalarm 和 sigreturn。
4. 在 proc.c 中添加初始化和释放代码；
5. 在 usertrap() 函数中实现时钟具体机制代码。