// LEGO Digital Designer — restored source
// TU 017 "SystemTime.cpp" (name inferred)  Mac 0x1cdc0-0x1ce6f (2 functions + static init
// 0x1ce30 = <iostream> Init only), Win 0x436180-0x4361f8.
// Mac: gettimeofday(). Win: timeGetTime() and QueryPerformanceCounter(); GetMicroseconds is
// inlined into all callers, GetMilliseconds into some (e.g. TraceHook). Names inferred.
#include "SystemTime.h"

#if defined(_WIN32)
#include <windows.h>
#include <mmsystem.h>
#else
#include <sys/time.h>
#endif

#include <iostream>

namespace LEGO {

#if defined(_WIN32)
namespace {

// Queried once (function-local static with guard bit, Win 0xb449ac; value 0xb44960).
struct PerformanceFrequency {                                         // (name inferred)
    PerformanceFrequency() { QueryPerformanceFrequency(&m_value); }
    LARGE_INTEGER m_value;
};

// Win 0x436190 (name inferred)
Int64 GetPerformanceFrequency()
{
    static PerformanceFrequency s_frequency;
    return s_frequency.m_value.QuadPart;
}

} // namespace
#endif

// Mac 0x1cdc0, Win 0x436180
// NOTE: Mac computes in 32-bit `long` (wraps every ~49.7 days, then sign-extends); Win
// zero-extends the 32-bit timeGetTime() value.
Int64 GetMilliseconds()
{
#if defined(_WIN32)
    return ::timeGetTime();
#else
    timeval tv;
    gettimeofday(&tv, 0);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
}

// Mac 0x1ce00, Win inlined
// NOTE: Mac computes in 32-bit `long` as well (wraps every ~71 minutes).
Int64 GetMicroseconds()
{
#if defined(_WIN32)
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return counter.QuadPart * 1000000 / GetPerformanceFrequency();
#else
    timeval tv;
    gettimeofday(&tv, 0);
    return tv.tv_sec * 1000000 + tv.tv_usec;
#endif
}

} // namespace LEGO
