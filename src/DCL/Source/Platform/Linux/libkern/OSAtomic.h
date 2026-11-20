// compat shim: subset of macOS <libkern/OSAtomic.h>
#pragma once
#include <stdint.h>
static inline int32_t OSAtomicAdd32(int32_t amount, volatile int32_t* value)
{
    return __sync_add_and_fetch(value, amount);
}
static inline int32_t OSAtomicIncrement32(volatile int32_t* v) { return __sync_add_and_fetch(v, 1); }
static inline int32_t OSAtomicDecrement32(volatile int32_t* v) { return __sync_sub_and_fetch(v, 1); }
