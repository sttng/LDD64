// Vortex 5.0.0 build 800 "vortexldd41" - Vx message API and vxAlloc / vxFree
// (restored). Mac __text 0x463c0a-0x463d8a (inside TU 504, a small source
// file linked between VxMaterial and VxMotorConstraint); Win 0x70da40-0x70db50, 0x7075c0,
// 0x7c24e0. Thin wrappers of the Me layer: the handler pointers are the Me ones (Mac non-lazy
// pointers 0x702590..0x7025a0 -> 0x7794a0..0x7794bc).
#include "Vx/VxMessage.h"
#include "Vx/VxAllocator.h"
#include "Me/MeMemory.h"
#include "Me/MeMessage.h"



namespace Vx {

// Mac 0x463c0a, Win 0x7075c0
void* vxAlloc(size_t size)
{
    return MeMemoryAPI.create(size);
}

// Mac 0x463c2c, Win 0x7c24e0
void vxFree(void* p)
{
    MeMemoryAPI.destroy(p);
}

// Mac 0x463c4e, Win 0x70da40
int VxSetInfoLevel(int level)
{
    int old = MeInfoLevel;
    MeInfoLevel = level;
    return old;
}

// Mac 0x463c68, Win 0x70da70
int VxSetWarningLevel(int level)
{
    int old = MeWarningLevel;
    MeWarningLevel = level;
    return old;
}

// Win 0x70daa0
int VxSetFatalErrorLevel(int level)
{
    int old = MeFatalErrorLevel;
    MeFatalErrorLevel = level;
    return old;
}

// Mac 0x463c82, Win 0x70db50
void VxAssert(bool condition, const char* format, ...)
{
    if (!condition) {
        va_list ap;
        va_start(ap, format);
        MeFatalErrorHandler(0, format, ap);
        va_end(ap);
    }
}

// Mac 0x463cbb, Win 0x70db30
void VxFatalError(int level, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    MeFatalErrorHandler(level, format, ap);
    va_end(ap);
}

// Mac 0x463ced, Win 0x70db10
void VxWarning(int level, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    MeWarningHandler(level, format, ap);
    va_end(ap);
}

// Mac 0x463d1f, Win 0x70daf0
void VxInfo(int level, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    MeInfoHandler(level, format, ap);
    va_end(ap);
}

// Mac 0x463d51, Win 0x70dae0
VxMessageHandler VxSetFatalErrorHandler(VxMessageHandler handler)
{
    return MeSetFatalErrorHandler(handler);
}

// Mac 0x463d64, Win 0x70dad0
VxMessageHandler VxSetWarningHandler(VxMessageHandler handler)
{
    return MeSetWarningHandler(handler);
}

// Mac 0x463d77, Win 0x70dac0
VxMessageHandler VxSetInfoHandler(VxMessageHandler handler)
{
    return MeSetInfoHandler(handler);
}

} // namespace Vx
