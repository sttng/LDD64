// Vortex 5.0.0 build 800 "vortexldd41" - the VxCable pieces the Mac build kept (restored;
// Mac TU 463 0x43701a-0x437123; Win 0x77ac80). The rest of the cable module
// is Win only; until then raw offsets stand for the VxCableBase layout: the
// cable's own VxCableBase (segment 0) at Mac +0x408 / Win +0x518, the further segments
// (VxCableBase*) in an array whose begin is at Mac +0x558 / Win +0x688, and each VxCableBase's
// parts in a VxArray<VxPart*> at Mac +0x58 / Win +0x60.
#include "Vx/VxCable.h"

namespace Vx {

namespace {
#if defined(_WIN32)
enum { kCableBase = 0x518, kSegments = 0x688, kBaseParts = 0x60 };
#else
enum { kCableBase = 0x408, kSegments = 0x558, kBaseParts = 0x58 };
#endif

// Mac 0x437024 (inlined on Win): VxCableBase: part `index`, 0 when out of range (name inferred)
VxPart* cableBaseGetPart(const char* base, int index)
{
    const VxArray<VxPart*>& parts = *(const VxArray<VxPart*>*)(base + kBaseParts);
    if (index < 0 || index >= parts.size())
        return 0;
    return parts.begin()[index];
}
} // namespace

// Mac 0x43701a
int VxCable::getStaticClassType()
{
    return 0x17e;
}

// Mac 0x437072 (-> 0x43704e segment, 0x437024 part), Win 0x77ac80
VxPart* VxCable::getPart(int segment, int index) const
{
    // PORT64: the offsets above are the 32-bit layouts and no VxCable can be made in this build
    // (its constructor is part of the unrestored Win module), so 64-bit builds report no part
    // (compiled out on 32-bit)
    if (sizeof(void*) > 4)
        return 0;
    const char* base;
    if (segment == 0)
        base = (const char*)this + kCableBase; // NOTE: Win also maps a null cable to 0 here
    else
        base = (*(char* const* const*)((const char*)this + kSegments))[segment - 1];
    return cableBaseGetPart(base, index);
}

} // namespace Vx
