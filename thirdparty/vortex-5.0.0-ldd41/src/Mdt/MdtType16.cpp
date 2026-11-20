// Vortex 5.0.0 build 800 "vortexldd41" - constraint type 0x16 accessor (restored, V2.1).
// Mac 0x48f8ca (static initialiser 0x48f8e4 / 0x48f90f); Win 0x7ac5d0 in the file with the
// Win-only creator 0x7ac700 (vxadvcon type). The other functions of the file were dead-stripped
// on Mac. Names unknown.
#include "Mdt/MdtConstraint.h"

// Mac 0x48f8ca, Win 0x7ac5d0
MeReal MEAPI MdtType16GetValue(const MdtConstraint* c, int i)
{
    if (c == 0)
        return 0;
    return ((const MdtType16*)c)->value[i];
}
