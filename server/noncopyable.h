#ifndef __WILL_NONCOPYABLE_H__
#define __WILL_NONCOPYABLE_H__

namespace will {

class Noncopyable {
public:

    Noncopyable() = default;

    ~Noncopyable() = default;

    Noncopyable(const Noncopyable&) = delete;

    Noncopyable& operator=(const Noncopyable&) = delete;
};

}

#endif
