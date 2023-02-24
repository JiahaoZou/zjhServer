#ifndef __WILL_THREAD_H__
#define __WILL_THREAD_H__

#include "mutex.h"


namespace will {

class Thread : Noncopyable {
public:
    // 线程智能指针类型
    typedef std::shared_ptr<Thread> ptr;

    Thread(std::function<void()> cb, const std::string &name);

    ~Thread();

    pid_t getId() const { return m_id; }

    const std::string &getName() const { return m_name; }

    void join();

    static Thread *GetThis();

    static const std::string &GetName();

    static void SetName(const std::string &name);

private:
    static void *run(void *arg);

private:
    // 线程id
    pid_t m_id = -1;
    // 线程结构
    pthread_t m_thread = 0;
    // 线程执行函数
    std::function<void()> m_cb;
    // 线程名称
    std::string m_name;
    // 信号量
    Semaphore m_semaphore;
};

} // namespace will

#endif
