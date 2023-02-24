#include <atomic>
#include "fiber.h"
#include "log.h"
#include "macro.h"
#include "scheduler.h"

namespace will {

static Logger::ptr g_logger = WILL_LOG_NAME("system");

// 全局静态变量，用于生成协程id
static std::atomic<uint64_t> s_fiber_id{0};
// 全局静态变量，用于统计当前的协程数
static std::atomic<uint64_t> s_fiber_count{0};

// 线程局部变量，当前线程正在运行的协程
static thread_local Fiber *t_fiber = nullptr;
// 线程局部变量，当前线程的主协程，切换到这个协程，就相当于切换到了主协程中运行，智能指针形式
static thread_local Fiber::ptr t_thread_fiber = nullptr;

static uint32_t g_fiber_stack_size = 128 * 1024;

class MallocStackAllocator {
public:
    static void *Alloc(size_t size) { return malloc(size); }
    static void Dealloc(void *vp, size_t size) { return free(vp); }
};

using StackAllocator = MallocStackAllocator;

uint64_t Fiber::GetFiberId() {
    if (t_fiber) {
        return t_fiber->getId();
    }
    return 0;
}

Fiber::Fiber() {
    SetThis(this);
    m_state = RUNNING;

    if (getcontext(&m_ctx)) {
        WILL_ASSERT2(false, "getcontext");
    }

    ++s_fiber_count;
    m_id = s_fiber_id++; // 协程id从0开始，用完加1

    WILL_LOG_DEBUG(g_logger) << "Fiber::Fiber() main id = " << m_id;
}

void Fiber::SetThis(Fiber *f) { 
    t_fiber = f; 
}


// 获取当前协程，同时充当初始化当前线程主协程的作用，这个函数在使用协程之前要调用一下
Fiber::ptr Fiber::GetThis() {
    if (t_fiber) {
        return t_fiber->shared_from_this();
    }

    Fiber::ptr main_fiber(new Fiber);
    WILL_ASSERT(t_fiber == main_fiber.get());
    t_thread_fiber = main_fiber;
    return t_fiber->shared_from_this();
}


// 带参数的构造函数用于创建其他协程，需要分配栈
Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool run_in_scheduler)
    : m_id(s_fiber_id++)
    , m_cb(cb)
    , m_runInScheduler(run_in_scheduler) {
    ++s_fiber_count;
    m_stacksize = stacksize ? stacksize : g_fiber_stack_size;
    m_stack     = StackAllocator::Alloc(m_stacksize);

    if (getcontext(&m_ctx)) {
        WILL_ASSERT2(false, "getcontext");
    }

    m_ctx.uc_link          = nullptr;
    m_ctx.uc_stack.ss_sp   = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, &Fiber::MainFunc, 0);

    WILL_LOG_DEBUG(g_logger) << "Fiber::Fiber() id = " << m_id;
}


// 线程的主协程析构时需要特殊处理，因为主协程没有分配栈和cb
Fiber::~Fiber() {
    WILL_LOG_DEBUG(g_logger) << "Fiber::~Fiber() id = " << m_id;
    --s_fiber_count;
    if (m_stack) {
        // 有栈，说明是子协程，需要确保子协程一定是结束状态
        WILL_ASSERT(m_state == TERM);
        StackAllocator::Dealloc(m_stack, m_stacksize);
        WILL_LOG_DEBUG(g_logger) << "dealloc stack, id = " << m_id;
    } else {
        // 没有栈，说明是线程的主协程
        WILL_ASSERT(!m_cb);              // 主协程没有cb
        WILL_ASSERT(m_state == RUNNING); // 主协程一定是执行状态

        Fiber *cur = t_fiber; // 当前协程就是自己
        if (cur == this) {
            SetThis(nullptr);
        }
    }
}


// 这里为了简化状态管理，强制只有TERM状态的协程才可以重置，但其实刚创建好但没执行过的协程也应该允许重置的
void Fiber::reset(std::function<void()> cb) {
    WILL_ASSERT(m_stack);
    WILL_ASSERT(m_state == TERM);
    m_cb = cb;
    if (getcontext(&m_ctx)) {
        WILL_ASSERT2(false, "getcontext");
    }

    m_ctx.uc_link          = nullptr;
    m_ctx.uc_stack.ss_sp   = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    m_state = READY;
}

void Fiber::resume() {
    WILL_ASSERT(m_state != TERM && m_state != RUNNING);
    SetThis(this);
    m_state = RUNNING;

    // 如果协程参与调度器调度，那么应该和调度器的主协程进行swap，而不是线程主协程
    if (m_runInScheduler) {
        if (swapcontext(&(Scheduler::GetMainFiber()->m_ctx), &m_ctx)) {
            WILL_ASSERT2(false, "swapcontext");
        }
    } else {
        if (swapcontext(&(t_thread_fiber->m_ctx), &m_ctx)) {
            WILL_ASSERT2(false, "swapcontext");
        }
    }
}

void Fiber::yield() {
    // 协程运行完之后会自动yield一次，用于回到主协程，此时状态已为结束状态
    WILL_ASSERT(m_state == RUNNING || m_state == TERM);
    SetThis(t_thread_fiber.get());
    if (m_state != TERM) {
        m_state = READY;
    }

    // 如果协程参与调度器调度，那么应该和调度器的主协程进行swap，而不是线程主协程
    if (m_runInScheduler) {
        if (swapcontext(&m_ctx, &(Scheduler::GetMainFiber()->m_ctx))) {
            WILL_ASSERT2(false, "swapcontext");
        }
    } else {
        if (swapcontext(&m_ctx, &(t_thread_fiber->m_ctx))) {
            WILL_ASSERT2(false, "swapcontext");
        }
    }
}


void Fiber::MainFunc() {
    Fiber::ptr cur = GetThis(); // GetThis()的shared_from_this()方法让引用计数加1
    WILL_ASSERT(cur);

    cur->m_cb();
    cur->m_cb    = nullptr;
    cur->m_state = TERM;
    
    //返回时如果不手动释放shared_ptr cur的话，则在子协程中cur都有一次引用，
    //cur计数永远不会低于1，所以不会自动释放
    //手动先从cur中获得裸指针
    //然后再调用reset函数将shared_ptr的对象直接释放掉
    //然后再通过裸指针返回主协程

    auto raw_ptr = cur.get(); // 手动让t_fiber的引用计数减1
    cur.reset();
    raw_ptr->yield();
}

} // namespace will