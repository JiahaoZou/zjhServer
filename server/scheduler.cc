#include "scheduler.h"
#include "macro.h"
#include "hook.h"

namespace will {

static will::Logger::ptr g_logger = WILL_LOG_NAME("system");

// 当前线程的调度器，同一个调度器下的所有线程共享同一个实例
static thread_local Scheduler *t_scheduler = nullptr;
// 当前线程的调度协程，每个线程都独有一份(线程的主协程)
static thread_local Fiber *t_scheduler_fiber = nullptr;

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string &name) {
    WILL_ASSERT(threads > 0);

    m_useCaller = use_caller;
    m_name      = name;

    if (use_caller) {
        --threads;
        will::Fiber::GetThis();
        WILL_ASSERT(GetThis() == nullptr);
        t_scheduler = this;

        // caller线程的主协程不会被线程的调度协程run进行调度，而且，线程的调度协程停止时，应该返回caller线程的主协程
        // 在user caller情况下，把caller线程的主协程暂时保存起来，等调度协程结束时，再resume caller协程
        //这里的reset是智能指针自带的函数，将该智能指针指向一个新的对象。
        //这里就是创建了一个新的fiber对象，表示调度器所在的协程
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, false));

        will::Thread::SetName(m_name);
        t_scheduler_fiber = m_rootFiber.get();
        m_rootThread      = will::GetThreadId();
        m_threadIds.push_back(m_rootThread);
    } else {
        m_rootThread = -1;
    }
    m_threadCount = threads;
}

Scheduler *Scheduler::GetThis() { 
    return t_scheduler; 
}

Fiber *Scheduler::GetMainFiber() { 
    return t_scheduler_fiber;
}

void Scheduler::setThis() {
    t_scheduler = this;
}

Scheduler::~Scheduler() {
    WILL_LOG_DEBUG(g_logger) << "Scheduler::~Scheduler()";
    WILL_ASSERT(m_stopping);
    if (GetThis() == this) {
        t_scheduler = nullptr;
    }
}

void Scheduler::start() {
    WILL_LOG_DEBUG(g_logger) << "start";
    MutexType::Lock lock(m_mutex);
    if (m_stopping) {
        WILL_LOG_ERROR(g_logger) << "Scheduler is stopped";
        return;
    }
    WILL_ASSERT(m_threads.empty());
    m_threads.resize(m_threadCount);
    for (size_t i = 0; i < m_threadCount; i++) {
        //这里是new的一个Thread类，在构造函数中就会直接启动线程。
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this),
                                      m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }
}

bool Scheduler::stopping() {
    MutexType::Lock lock(m_mutex);
    return m_stopping && m_tasks.empty() && m_activeThreadCount == 0;
}

void Scheduler::tickle() { 
    WILL_LOG_DEBUG(g_logger) << "ticlke"; 
}

void Scheduler::idle() {
    WILL_LOG_DEBUG(g_logger) << "idle";
    while (!stopping()) {
        will::Fiber::GetThis()->yield();
    }
}

void Scheduler::stop() {
    WILL_LOG_DEBUG(g_logger) << "stop";
    if (stopping()) {
        return;
    }
    m_stopping = true;

    // 如果use caller，那只能由caller线程发起stop
    if (m_useCaller) {
        WILL_ASSERT(GetThis() == this);
    } else {
           WILL_ASSERT(GetThis() != this);
    }

    for (size_t i = 0; i < m_threadCount; i++) {
        tickle();
    }

    if (m_rootFiber) {
        tickle();
    }

    // 在use caller情况下，调度器协程结束时，应该返回caller协程
    if (m_rootFiber) {
        m_rootFiber->resume();
        WILL_LOG_DEBUG(g_logger) << "m_rootFiber end";
    }

    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }
    for (auto &i : thrs) {
        i->join();
    }
}

void Scheduler::run() {
    WILL_LOG_DEBUG(g_logger) << "run";
    set_hook_enable(true);
    setThis();
    //创建调度器的线程一定是开启了主协程的，且调度器的rootfiber的run函数不是在主协程上运行
    //这里判断当前线程是否为主线程，如果不是，则在当前线程上不一定创建了主协程
    //调用GetThis创建主协程（即调度携程)。
    if (will::GetThreadId() != m_rootThread) {
        t_scheduler_fiber = will::Fiber::GetThis().get();
    }
    //注意idle协程是每个线程都有一个，线程处于空闲状态时就运行idle协程。
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr cb_fiber;

    ScheduleTask task;
    while (true) {
        task.reset();
        bool tickle_me = false; // 是否tickle其他线程进行任务调度
        {
            MutexType::Lock lock(m_mutex);
            auto it = m_tasks.begin();
            // 遍历所有调度任务
            while (it != m_tasks.end()) {
                if (it->thread != -1 && it->thread != will::GetThreadId()) {
                    // 指定了调度线程，但不是在当前线程上调度，标记一下需要通知其他线程进行调度，然后跳过这个任务，继续下一个
                    ++it;
                    tickle_me = true;
                    continue;
                }

                // 找到一个未指定线程，或是指定了当前线程的任务
                WILL_ASSERT(it->fiber || it->cb);

                // hook IO相关的系统调用时，在检测到IO未就绪的情况下，会先添加对应的读写事件，再yield当前协程，等IO就绪后再resume当前协程
                // 多线程高并发情境下，有可能发生刚添加事件就被触发的情况，如果此时当前协程还未来得及yield，则这里就有可能出现协程状态仍为RUNNING的情况
                // 这里简单地跳过这种情况，以损失一点性能为代价，否则整个协程框架都要大改
                if(it->fiber && it->fiber->getState() == Fiber::RUNNING) {
                    ++it;
                    continue;
                }
                
                // 当前调度线程找到一个任务，准备开始调度，将其从任务队列中剔除，活动线程数加1
                task = *it;
                m_tasks.erase(it++);
                ++m_activeThreadCount;
                break;
            }
            // 当前线程拿完一个任务后，发现任务队列还有剩余，那么tickle一下其他线程
            tickle_me |= (it != m_tasks.end());
        }

        if (tickle_me) {
            tickle();
        }
        //注意，此时要任务指定是在本线程执行才可能取得task，若任务队列里没有任务指定线程为任意或本线程
        //则task为null
        if (task.fiber) {
            //进入这一步表示取得了任务，且任务类型为协程
            //这里resume这个名字没有取好，叫交换比较恰当，就是此时让出当前协程的执行权，执行任务携带的协程
            // resume协程，resume返回时，协程要么执行完了，要么半路yield了，总之这个任务就算完成了，活跃线程数减一
            //执行完后会回到这里
            task.fiber->resume();
            --m_activeThreadCount;
            task.reset();
        } else if (task.cb) {
            //取得了任务，类型为函数
            if (cb_fiber) {
                cb_fiber->reset(task.cb);
            } else {
                cb_fiber.reset(new Fiber(task.cb));
            }
            task.reset();
            cb_fiber->resume();
            --m_activeThreadCount;
            cb_fiber.reset();
        } else {
            // 进到这个分支情况一定是任务队列空了，调度idle协程即可
            if (idle_fiber->getState() == Fiber::TERM) {
                // 如果调度器没有调度任务，那么idle协程会不停地resume/yield，不会结束，如果idle协程结束了，那一定是调度器停止了
                WILL_LOG_DEBUG(g_logger) << "idle fiber term";
                break;
            }
            ++m_idleThreadCount;
            idle_fiber->resume();
            --m_idleThreadCount;
        }
    }
    WILL_LOG_DEBUG(g_logger) << "Scheduler::run() exit";
}

} // end namespace will