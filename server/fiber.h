#ifndef __WILL_FIBER_H__
#define __WILL_FIBER_H__

#include <functional>
#include <memory>
#include <ucontext.h>
#include "thread.h"

namespace will {
//fiber类继承std::enable_shared_from_this<Fiber>
//那么在将this作为智能指针返回时返回的是用一个
class Fiber : public std::enable_shared_from_this<Fiber> {
public:
    typedef std::shared_ptr<Fiber> ptr;

    // 定义三态转换关系，也就是协程要么正在运行(RUNNING)，
    // 要么准备运行(READY)，要么运行结束(TERM)。不区分协程的初始状态，初始即READY。不区分协程是异常结束还是正常结束，
    // 只要结束就是TERM状态。也不区别HOLD状态，协程只要未结束也非运行态，那就是READY状态。
    enum State {
        // 就绪态，刚创建或者yield之后的状态
        READY,
        // 运行态，resume之后的状态
        RUNNING,
        // 结束态，协程的回调函数执行完之后为TERM状态
        TERM
    };

private:
    Fiber();
public:
    // run_in_scheduler 本协程是否参与调度器调度，默认为true
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool run_in_scheduler = true);

    ~Fiber();

    // 重置协程状态和入口函数，复用栈空间，不重新创建栈
    void reset(std::function<void()> cb);

    // 将当前协程切到到执行状态
    // 当前协程和正在运行的协程进行交换，前者状态变为RUNNING，后者状态变为READY
    void resume();

    // 当前协程让出执行权
    // 当前协程与上次resume时退到后台的协程进行交换，前者状态变为READY，后者状态变为RUNNING
    void yield();

    uint64_t getId() const { return m_id; }

    State getState() const { return m_state; }
public:
    // 设置当前正在运行的协程，即设置线程局部变量t_fiber的值
    static void SetThis(Fiber *f);

    // 返回当前线程正在执行的协程
    // 如果当前线程还未创建协程，则创建线程的第一个协程，
    // 且该协程为当前线程的主协程，其他协程都通过这个协程来调度，也就是说，其他协程
    // 结束时,都要切回到主协程，由主协程重新选择新的协程进行resume
    // 线程如果要创建协程，那么应该首先执行一下Fiber::GetThis()操作，以初始化主函数协程
    static Fiber::ptr GetThis();

    static uint64_t TotalFibers();

    // 协程入口函数
    static void MainFunc();

    static uint64_t GetFiberId();
private:
    // 协程id
    uint64_t m_id        = 0;
    // 协程栈大小
    uint32_t m_stacksize = 0;
    // 协程状态
    State m_state        = READY;
    // 协程上下文
    ucontext_t m_ctx;
    // 协程栈地址
    void *m_stack = nullptr;
    // 协程入口函数
    std::function<void()> m_cb;
    // 本协程是否参与调度器调度
    bool m_runInScheduler;
};

} // namespace will

#endif