# zjhServer
c++实现的服务器框架

实现了如下功能模块：
- 线程模块：提供线程类和用于互斥和同步的锁类
- 协程模块：基于ucontext实现了非对称协程，每个线程有且只有一个主协程，只有在主协程中才能创建
           从协程，主协程不分配栈空间也不执行任务，只用来管理从协程。
- 协程调度模块：实现了一个N:M协程调度器，调度器内置一个线程池。将协程或是直接的回调函数包装成
            struct task，放入task队列，采用生产者消费者模型实现线程池。为每个线程引入idle协程，
            用来陷入epollwait来监听任务队列。
- hook模块：使用epoll实现，将某些同步API给异步化
- socket模块：对linux socket编程相关API的封装，同时经由hook模块实现了非阻塞socket
- http模块：采用状态机解析HTTP报文，封装HTTP和TCP相关类
