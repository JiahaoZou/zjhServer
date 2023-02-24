#ifndef __WILL_IOMANAGER_H__
#define __WILL_IOMANAGER_H__

#include "scheduler.h"
#include "timer.h"

namespace will {

class IOManager : public Scheduler, public TimerManager {
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;

    // IO事件，继承自epoll对事件的定义
    // 这里只关心socket fd的读和写事件，其他epoll事件会归类到这两类事件中
    enum Event {
        /// 无事件
        NONE = 0x0,
        /// 读事件(EPOLLIN)
        READ = 0x1,
        /// 写事件(EPOLLOUT)
        WRITE = 0x4,
    };

private:
    
    // socket fd上下文类
    // 每个socket fd都对应一个FdContext，包括fd的值，fd上的事件，以及fd的读写事件上下文
    struct FdContext {
        typedef Mutex MutexType;
        
        // 事件上下文类
        // fd的每个事件都有一个事件上下文，保存这个事件的回调函数以及执行回调函数的调度器
        // 只预留了读事件和写事件，所有的事件都被归类到这两类事件中
        struct EventContext {
            // 执行事件回调的调度器
            Scheduler *scheduler = nullptr;
            // 事件回调协程
            Fiber::ptr fiber;
            // 事件回调函数
            std::function<void()> cb;
        };


        EventContext &getEventContext(Event event);

        void resetEventContext(EventContext &ctx);

        // 根据事件类型调用对应上下文结构中的调度器去调度回调协程或回调函数
        void triggerEvent(Event event);

        // 读事件上下文
        EventContext read;
        // 写事件上下文
        EventContext write;
        // 事件关联的句柄
        int fd = 0;
        // 该fd添加了哪些事件的回调函数，或者说该fd关心哪些事件
        Event events = NONE;
        // 事件的Mutex
        MutexType mutex;
    };

public:
    
    // 线程数量
    // use_caller 是否将调用线程包含进去
    IOManager(size_t threads = 1, bool use_caller = true, const std::string &name = "IOManager");

    ~IOManager();

    // fd描述符发生了event事件时执行cb函数
    // fd socket句柄
    // event 事件类型
    // cb 事件回调函数，如果为空，则默认把当前协程作为回调执行体
    // 添加成功返回0,失败返回-1
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);

    // 删除事件
    // fd socket句柄
    // event 事件类型
    // 不会触发事件
    // 是否删除成功
    bool delEvent(int fd, Event event);

    // 取消事件
    // fd socket句柄
    // event 事件类型
    // 如果该事件被注册过回调，那就触发一次回调事件
    bool cancelEvent(int fd, Event event);

    // 取消所有事件
    // 所有被注册的回调事件在cancel之前都会被执行一次
    // fd socket句柄
    // 是否删除成功
    bool cancelAll(int fd);

    static IOManager *GetThis();

protected:
    
    // 写pipe让idle协程从epoll_wait退出，待idle协程yield之后Scheduler::run就可以调度其他任务
    void tickle() override;

    // 判断条件是Scheduler::stopping()外加IOManager的m_pendingEventCount为0，表示没有IO事件可调度了
    bool stopping() override;

    // 对于IO协程调度来说，应阻塞在等待IO事件上，idle退出的时机是epoll_wait返回，对应的操作是tickle或注册的IO事件发生
    void idle() override;

    // 判断是否可以停止，同时获取最近一个定时器的超时时间
    // timeout 最近一个定时器的超时时间，用于idle协程的epoll_wait
    bool stopping(uint64_t& timeout);

    // 当有定时器插入到头部时，要重新更新epoll_wait的超时时间，这里是唤醒idle协程以便于使用新的超时时间
    void onTimerInsertedAtFront() override;

    // 重置socket句柄上下文的容器大小
    // size 容量大小
    void contextResize(size_t size);

private:
    // epoll 文件句柄
    int m_epfd = 0;
    // pipe 文件句柄，fd[0]读端，fd[1]写端
    int m_tickleFds[2];
    // 当前等待执行的IO事件数量
    std::atomic<size_t> m_pendingEventCount = {0};
    // IOManager的Mutex
    RWMutexType m_mutex;
    // socket事件上下文的容器
    std::vector<FdContext *> m_fdContexts;
};

} // end namespace will

#endif