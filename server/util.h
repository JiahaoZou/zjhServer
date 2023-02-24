#ifndef __WILL_UTIL_H__
#define __WILL_UTIL_H__

#include <sys/types.h>
#include <stdint.h>
#include <sys/time.h>
#include <string>
#include <vector>
#include <iostream>

namespace will {

// 这里不要把pid_t和pthread_t混淆，关于它们之的区别可参考gettid(2)
pid_t GetThreadId();

// 桩函数，暂时返回0，等协程模块完善后再返回实际值
uint64_t GetFiberId();

// 获取当前启动的毫秒数，参考clock_gettime(2)，使用CLOCK_MONOTONIC_RAW
uint64_t GetElapsedMS();

// 获取线程名称，参考pthread_getname_np(3)
std::string GetThreadName();

// 设置线程名称，参考pthread_setname_np(3)
// 线程名称不能超过16字节，包括结尾的'\0'字符
void SetThreadName(const std::string &name);

// 获取当前时间的毫秒
uint64_t GetCurrentMS();

// 获取当前时间的微秒
uint64_t GetCurrentUS();

bool Unlink(const std::string &filename, bool exist = false);

} // namespace will

#endif // __WILL_UTIL_H__
