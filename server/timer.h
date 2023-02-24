#ifndef __WILL_TIMER_H__
#define __WILL_TIMER_H__

#include <memory>
#include <vector>
#include <set>
#include "mutex.h"

namespace will {

class TimerManager;

class Timer : public std::enable_shared_from_this<Timer> {
friend class TimerManager;
public:
    // 定时器的智能指针类型
    typedef std::shared_ptr<Timer> ptr;

    bool cancel();

    bool refresh();

    // ms 定时器执行间隔时间(毫秒)
    // from_now 是否从当前时间开始计算
    bool reset(uint64_t ms, bool from_now);
private:
    // ms 定时器执行间隔时间
    // cb 回调函数
    // recurring 是否循环
    // manager 定时器管理器
    Timer(uint64_t ms, std::function<void()> cb,
          bool recurring, TimerManager* manager);
    // 构造函数
    // next 执行的时间戳(毫秒)
    Timer(uint64_t next);
private:
    // 是否循环定时器
    bool m_recurring = false;
    // 执行周期
    uint64_t m_ms = 0;
    // 精确的执行时间
    uint64_t m_next = 0;
    // 回调函数
    std::function<void()> m_cb;
    // 定时器管理器
    TimerManager* m_manager = nullptr;
private:
    struct Comparator {
        // 比较定时器的智能指针的大小(按执行时间排序)
        // lhs 定时器智能指针
        // rhs 定时器智能指针
        bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const;
    };
};

class TimerManager {
friend class Timer;
public:
    // 读写锁类型
    typedef RWMutex RWMutexType;

    TimerManager();

    virtual ~TimerManager();

    // ms 定时器执行间隔时间
    // cb 定时器回调函数
    // recurring 是否循环定时器
    Timer::ptr addTimer(uint64_t ms, std::function<void()> cb
                        ,bool recurring = false);

    // ms 定时器执行间隔时间
    // cb 定时器回调函数
    // weak_cond 条件
    // recurring 是否循环
    Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb
                        ,std::weak_ptr<void> weak_cond
                        ,bool recurring = false);

    // 到最近一个定时器执行的时间间隔(毫秒)
    uint64_t getNextTimer();

    // 获取需要执行的定时器的回调函数列表
    void listExpiredCb(std::vector<std::function<void()> >& cbs);

    bool hasTimer();
protected:

    // 当有新的定时器插入到定时器的首部,执行该函数
    virtual void onTimerInsertedAtFront() = 0;

    void addTimer(Timer::ptr val, RWMutexType::WriteLock& lock);
private:
    // 检测服务器时间是否被调后了
    bool detectClockRollover(uint64_t now_ms);
private:
    // Mutex
    RWMutexType m_mutex;
    // 定时器集合
    std::set<Timer::ptr, Timer::Comparator> m_timers;
    // 是否触发onTimerInsertedAtFront
    bool m_tickled = false;
    // 上次执行时间
    uint64_t m_previouseTime = 0;
};

}

#endif
