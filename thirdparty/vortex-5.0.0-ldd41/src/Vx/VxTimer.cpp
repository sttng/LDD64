// Vortex 5.0.0 build 800 "vortexldd41" - VxTimer and the processor count
// (restored). Mac TU 524 0x47f5c6-0x47f732; Win 0x7404c0-0x740660, 0x75d090,
// static initialiser 0x937c50.
#include "Vx/VxTimer.h"
#include "Vx/VxThreadPool.h"
#include "Vx/VxMessage.h"
#include "Me/MeMemory.h"
#include "Me/MeProfile.h"
#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <sys/types.h>
#include <sys/sysctl.h>
#else
#include <unistd.h>
#endif

namespace Vx {

// Mac 0x47f5c6, Win 0x75d090: number of processors (Mac: hw.availcpu, else hw.ncpu, at least
// 1; Win: GetSystemInfo().dwNumberOfProcessors)
int VxGetNumberOfProcessors()
{
#if defined(_WIN32)
    SYSTEM_INFO info = SYSTEM_INFO();
    GetSystemInfo(&info);
    return (int)info.dwNumberOfProcessors;
#elif defined(__APPLE__)
    int mib[2] = { CTL_HW, HW_AVAILCPU };
    int n = 0;
    size_t len = sizeof(n);
    sysctl(mib, 2, &n, &len, 0, 0);
    if (n < 1) {
        mib[1] = HW_NCPU;
        sysctl(mib, 2, &n, &len, 0, 0);
        if (n < 1)
            n = 1;
    }
    return n;
#else
    // HOST-ONLY (Linux-hosted Mac branch): the online processor count, at least 1
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return n < 1 ? 1 : (int)n;
#endif
}

#if defined(_WIN32)

static __int64 sFrequency = 0; // Win 0xaf1f40

namespace {
// Win 0x937c50 (static initialiser)
struct FrequencyInit {
    FrequencyInit()
    {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        sFrequency = f.QuadPart;
    }
} sFrequencyInit;
}

// Win 0x740660
VxTimer::VxTimer()
    : mStart(0)
    , mEnd(0)
    , mTotal(0)
    , mRunning(false)
{
}

// Win 0x7404e0
VxTimer::~VxTimer()
{
}

// Win 0x7404c0
void VxTimer::initialise()
{
}

// Win 0x7404f0
void VxTimer::reset()
{
    mStart = 0;
    mEnd = 0;
    mTotal = 0;
    mRunning = false;
}

// Win 0x740510
void VxTimer::start()
{
    LARGE_INTEGER now;
    if (mRunning)
        VxWarning(0, "VxTimer: Timer already running.\n");
    mRunning = true;
    QueryPerformanceCounter(&now);
    mStart = now.QuadPart;
}

// Win 0x740550
VxReal VxTimer::stop()
{
    LARGE_INTEGER now;
    if (!mRunning)
        VxWarning(0, "VxTimer: Timer is not running.\n");
    QueryPerformanceCounter(&now);
    mEnd = now.QuadPart;
    __int64 d = mEnd - mStart;
    mTotal += d;
    mRunning = false;
    return (VxReal)((long double)d / (long double)sFrequency);
}

// Win 0x7405c0
VxReal VxTimer::getLastTime() const
{
    if (mRunning)
        VxWarning(0, "VxTimer: Timer already running.\n");
    return (VxReal)((long double)(mEnd - mStart) / (long double)sFrequency);
}

// Win 0x740600
VxReal VxTimer::getElapsedTime() const
{
    LARGE_INTEGER now;
    if (!mRunning)
        VxWarning(0, "VxTimer: Timer is not running.\n");
    QueryPerformanceCounter(&now);
    return (VxReal)((long double)(now.QuadPart - mStart) / (long double)sFrequency);
}

#else

// C2 Mac 0x47f6ee, C1 0x47f71e
VxTimer::VxTimer()
{
    mTimer = (MeTimer*)MeMemoryAPI.create(sizeof(MeTimer));
    MeTimerInit(mTimer);
}

// D2 Mac 0x47f674, D1 0x47f698
VxTimer::~VxTimer()
{
    MeMemoryAPI.destroy(mTimer);
}

// Mac 0x47f732
void VxTimer::initialise()
{
    MeTimerInitFrequency();
}

// Mac 0x47f6d8
void VxTimer::reset()
{
    MeTimerInit(mTimer);
}

// Mac 0x47f6c2
void VxTimer::start()
{
    MeTimerStartTimer(mTimer);
}

// Mac 0x47f6ac
VxReal VxTimer::stop()
{
    return MeTimerStopTimer(mTimer);
}

// (Win only in the binaries; the same from the MeTimer fields)
VxReal VxTimer::getLastTime() const
{
    if (mTimer->running)
        VxWarning(0, "VxTimer: Timer already running.\n");
    return (VxReal)(mTimer->end.cpuCycles - mTimer->start.cpuCycles) / (VxReal)MeProfileTicksPerSecond;
}

// (Win only in the binaries)
VxReal VxTimer::getElapsedTime() const
{
    MeProfileTimestamp now;
    if (!mTimer->running)
        VxWarning(0, "VxTimer: Timer is not running.\n");
    MeProfileGetTimestamp(&now);
    return (VxReal)(now.cpuCycles - mTimer->start.cpuCycles) / (VxReal)MeProfileTicksPerSecond;
}

#endif

} // namespace Vx
