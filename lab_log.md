## trace 系统调用跟踪
### 实验要求：
添加一个trace system call调用，可以实现跟踪system call。
此函数入参为一个数字，可以控制跟踪哪些system call。
如：
trace(1<<SYS_fork)，trace(10b)，trace(2)表示跟踪fork调用；
trace(1<<SYS_read)，trace(10 0000b)，trace(32)，表示跟踪read调用；
trace(10 0010b)，trace(34)，表示跟踪fork、read调用；

```
# 跟踪grep 系统调用
trace 32 grep hello README

# 输出
3: syscall read -> 1023
3: syscall read -> 966
3: syscall read -> 70
3: syscall read -> 0

# 解释
进程ID为3的进程(grep)调用了 read 系统调用，并成功读取了1023字节的数据；
...
```