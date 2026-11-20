// LEGO Digital Designer — restored source
// The reference count operations of the BaseBrickPhysics* IUnknown implementations: Mac
// OSAtomicAdd32 (inlined calls, e.g. Mac 0x2510a0 / 0x2510c0), Win InterlockedIncrement /
// InterlockedDecrement (Win 0x5c7750 / 0x5c4cf0). Both return the new count. Private to the
// Physics .cpp files (they pull in the OS headers). (file name inferred)
#pragma once
#include "LEGO/Types.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <libkern/OSAtomic.h>
#endif

namespace LEGO {

inline Int32 PhysicsAtomicIncrement(volatile Int32& count)
{
#if defined(_WIN32)
    return (Int32)InterlockedIncrement((LONG*)&count);
#else
    return (Int32)OSAtomicAdd32(1, (int32_t*)&count);
#endif
}

inline Int32 PhysicsAtomicDecrement(volatile Int32& count)
{
#if defined(_WIN32)
    return (Int32)InterlockedDecrement((LONG*)&count);
#else
    return (Int32)OSAtomicAdd32(-1, (int32_t*)&count);
#endif
}

// QueryInterface failure: E_FAIL (Win 0x80004005; the Mac COM headers' E_FAIL 0x80000008).
#if defined(_WIN32)
const Int32 kPhysicsNoInterface = (Int32)0x80004005;
#else
const Int32 kPhysicsNoInterface = (Int32)0x80000008;
#endif

} // namespace LEGO
