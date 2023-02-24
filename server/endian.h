#ifndef __WILL_ENDIAN_H__
#define __WILL_ENDIAN_H__

#define WILL_LITTLE_ENDIAN 1
#define WILL_BIG_ENDIAN 2

#include <byteswap.h>
#include <stdint.h>

namespace will {

// 8字节类型的字节序转化
template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
byteswap(T value) {
    return (T)bswap_64((uint64_t)value);
}


// 4字节类型的字节序转化
template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
byteswap(T value) {
    return (T)bswap_32((uint32_t)value);
}

// 2字节类型的字节序转化
template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
byteswap(T value) {
    return (T)bswap_16((uint16_t)value);
}

#if BYTE_ORDER == BIG_ENDIAN
#define WILL_BYTE_ORDER WILL_BIG_ENDIAN
#else
#define WILL_BYTE_ORDER WILL_LITTLE_ENDIAN
#endif

#if WILL_BYTE_ORDER == WILL_BIG_ENDIAN

// 只在小端机器上执行byteswap, 在大端机器上什么都不做
template <class T>
T byteswapOnLittleEndian(T t) {
    return t;
}

// 只在大端机器上执行byteswap, 在小端机器上什么都不做
template <class T>
T byteswapOnBigEndian(T t) {
    return byteswap(t);
}
#else

// 只在小端机器上执行byteswap, 在大端机器上什么都不做
template <class T>
T byteswapOnLittleEndian(T t) {
    return byteswap(t);
}

// 只在大端机器上执行byteswap, 在小端机器上什么都不做
template <class T>
T byteswapOnBigEndian(T t) {
    return t;
}
#endif

} // namespace will

#endif
