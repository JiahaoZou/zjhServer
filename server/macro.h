#ifndef __WILL_MACRO_H__
#define __WILL_MACRO_H__

#include <string.h>
#include <assert.h>
#include "log.h"
#include "util.h"
#include <cstdlib>
#include <cassert>

#define ASSERT_INFO(expr, info) ((expr) ? ((void)0) : ({ std::cerr << info; std::abort(); }) )
#define ASSERT(expr) (assert(expr))


#if defined __GNUC__ || defined __llvm__
// LIKCLY 宏的封装, 告诉编译器优化,条件大概率成立
#define WILL_LIKELY(x) __builtin_expect(!!(x), 1)
// LIKCLY 宏的封装, 告诉编译器优化,条件大概率不成立
#define WILL_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define WILL_LIKELY(x) (x)
#define WILL_UNLIKELY(x) (x)
#endif

// 断言宏封装
#define WILL_ASSERT(x)                                                                \
    if (WILL_UNLIKELY(!(x))) {                                                        \
        assert(x);                                                                     \
    }

// 断言宏封装
#define WILL_ASSERT2(x, w)                                                            \
    if (WILL_UNLIKELY(!(x))) {                                                        \
        WILL_LOG_ERROR(WILL_LOG_ROOT()) << w;                                         \
        assert(x);                                                                    \
    }

#endif
