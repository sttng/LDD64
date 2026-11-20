// Vortex 5.0.0 build 800 "vortexldd41" - constraint type 0x10 accessor (restored).
// Mac 0x48f98b (static initialiser 0x48f9a1 / 0x48f9cc); Win 0x7ac240 next to the Win-only
// creator 0x7ac3c0. Name unknown.
#include "Mdt/MdtConstraint.h"

// Mac 0x48f98b, Win 0x7ac240
MeReal MEAPI MdtType10GetValue(const MdtConstraint* c)
{
    if (c == 0)
        return 0;
    return ((const MdtType10*)c)->value;
}
