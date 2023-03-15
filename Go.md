# goroutine
## 特点
和其他语言的协程对比：

* goroutine是有栈协程，而Python、JS的是无栈协程
* goroutine可以运行在不同线程上，而Python、JS的只能运行在一个线程上。这也导致了goroutine不是线程安全的，访问全局变量需要加锁
* goroutine的接口更接近于线程：
    * 有栈协程不需要声明函数为`async`，没有传染性
    * 对称协程意味着不能直接通过返回值获取结果，而要通过通信来获取结果
    * 有可能隐式切换上下文，而其他语言的协程必须用`await`显式切换上下文
* goroutine的栈大小不是固定的，可以按需增加和减小


## 调度
[参考](https://draveness.me/golang/docs/part3-runtime/ch06-concurrency/golang-goroutine/)

### GPM模型
GPM是Go运行时自己实现的调度系统

* G（Goroutine）：存放goroutine信息、CPU寄存器、栈等信息
* P（Processor）：抽象的执行器，不是真正的CPU。为M的执行提供上下文，比如当前运行的G、本地可运行队列（LRQ）。M只有绑定P才能执行G，当M被阻塞时，P会被传递给其他M
* M（Machine）：对应操作系统内核线程，一个goroutine最终是要放到M上执行的。当M没有工作时，会自旋地找工作，比如检查全局可运行队列（GRQ）、执行GC、尝试从其他P偷工作，实在没有工作就会休眠

为什么需要P，而不是直接把运行队列放到M：为了方便M阻塞时（比如系统调用）可以方便地把运行队列交给其他的M

### 调度时机
* 主动挂起：调用`runtime.gopark`
* 系统调用：Go会在`syscall.Syscall`等系统调用封装函数中，插入`runtime.entersyscall`和`runtime.exitsyscall`
* 协作式调度：Go会在每个函数开头插入调度点，使长时间运行的goroutine有机会被调度。sysmon线程会监控并抢占运行时间超过10ms的goroutine
* 基于信号的抢占式调度：在GC的时候向线程发送SIGURG，在信号处理函数中调度
