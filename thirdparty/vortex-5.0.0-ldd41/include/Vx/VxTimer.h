// Vortex 5.0.0 build 800 "vortexldd41" - VxTimer (restored): VxFrame's step
// stopwatch (VxFrame Mac +0xcc). Mac (TU 524 0x47f674-0x47f732): a pointer to a heap MeTimer, 4 bytes
// (VxFrame: new(4) + C1 0x47f71e).
// Win (0x7404c0-0x740660): its own QueryPerformanceCounter code, 0x20 bytes; frequency read by a
// static initialiser (Win 0x937c50). Class name from the Win "VxTimer: ..." strings; methods inferred.
#ifndef VX_VXTIMER_H
#define VX_VXTIMER_H

#include "Vx/VxPlatform.h"

struct MeTimer;

namespace Vx {

class VxTimer {
public:
    VxTimer();                      // Mac C1 0x47f71e -> C2 0x47f6ee, Win 0x740660
    ~VxTimer();                     // Mac D1 0x47f698 -> D2 0x47f674, Win 0x7404e0 (empty)
    static void initialise();       // Mac 0x47f732 (MeTimerInitFrequency), Win 0x7404c0 (empty)
    void reset();                   // Mac 0x47f6d8, Win 0x7404f0
    void start();                   // Mac 0x47f6c2, Win 0x740510
    VxReal stop();                  // Mac 0x47f6ac, Win 0x740550: seconds of this run
    VxReal getLastTime() const;     // Win 0x7405c0: seconds of the last run (Win only)
    VxReal getElapsedTime() const;  // Win 0x740600: seconds since start (Win only)

#if defined(_WIN32)
    __int64 mStart;   // +0x00
    __int64 mEnd;     // +0x08
    __int64 mTotal;   // +0x10
    bool mRunning;    // +0x18
#else
    MeTimer* mTimer;  // +0x00
#endif

private:
    VxTimer(const VxTimer&);
    VxTimer& operator=(const VxTimer&);
};

} // namespace Vx

#endif
