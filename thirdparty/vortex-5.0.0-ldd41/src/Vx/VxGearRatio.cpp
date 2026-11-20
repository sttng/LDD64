// Vortex 5.0.0 build 800 "vortexldd41" - VxGearRatio (restored in part).
// Mac TU 489 (0x4574d8-0x4575ea; static initialiser 0x45757a: <iostream> + MdtConstraintMaxSize),
// Win 0x750d30-0x7517f0 (only the Mac functions are restored; see Vx/VxGearRatio.h).
#include "Vx/VxGearRatio.h"
#include "Vx/VxMessage.h"
#include "Mdt/MdtConstraint.h"
#include "Mdt/MdtTypes.h"
#include <stddef.h>
#include <iostream>

namespace Vx {

// Mac 0x4574d8, Win 0x750d30
int VxGearRatio::getStaticClassType()
{
    return 0x11f;
}

// The 32-bit offsets of the binaries (PORT64: the setters write through MdtGearRatio; checked in
// 32-bit builds only).
#if defined(__LP64__) || defined(_WIN64)
#elif defined(_WIN32)
typedef char VxGearRatioLayoutCheck[(sizeof(void*) != 4 || (offsetof(MdtGearRatio, ratio) == 0xb28 &&
                                     offsetof(MdtGearRatio, partReferencePosition) == 0xb60)) ? 1 : -1];
#else
typedef char VxGearRatioLayoutCheck[(sizeof(void*) != 4 || (offsetof(MdtGearRatio, ratio) == 0xb00 &&
                                     offsetof(MdtGearRatio, partReferencePosition) == 0xb34)) ? 1 : -1];
#endif

// Mac 0x4574e2, Win 0x7511d0 (Mac +0xb00, Win +0xb28)
void VxGearRatio::setRatio(VxReal r)
{
    ((MdtGearRatio*)mConstraint)->ratio = r;
}

// Mac 0x45751c, Win 0x751190 (Mac +0xb34, Win +0xb60)
void VxGearRatio::setPartReferencePosition(unsigned int part, VxReal position)
{
    if (part < 2) {
        ((MdtGearRatio*)mConstraint)->partReferencePosition[part] = position;
    } else {
        VxWarning(0, "VxGearRatio::setPartReferencePosition: Part index must be 0 or 1.\n");
    }
}

// Mac 0x457564, Win 0x750f40
VxReal VxGearRatio::getRatio() const
{
    return MdtType10GetValue(mConstraint);
}

} // namespace Vx
