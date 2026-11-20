// Vortex 5.0.0 build 800 "vortexldd41" - winch (constraint type 0x12) length accessors
// (restored). Mac 0x4930bc-0x4931ac (static initialiser 0x49316a / 0x493195), Win
// 0x852790-0x852800 (the Win file holds the whole VxWinch Mdt API).
#include "Mdt/MdtConstraint.h"
#include "Mdt/MdtBcl.h"

// Mac 0x4930bc, Win 0x852800
MeReal MEAPI MdtWinchGetLength(const MdtConstraint* c)
{
    if (c == 0)
        return 0;
    return ((const MdtWinch*)c)->length;
}

// Mac 0x4930d2, Win 0x852790 (Win: through the vxcable license gate 0x75d480): returns the
// previous length
MeReal MEAPI MdtWinchSetLength(MdtConstraint* c, MeReal l)
{
    if (c == 0)
        return 0;
    MdtWinch* w = (MdtWinch*)c;
    MdtBclWinchUpdate(&c->bcl);
    const MeReal old = w->length;
    w->length = l;
    if (w->ratio == 0.0)
        w->m_b14 = -w->offset;
    else
        w->m_b14 = l / w->ratio - w->offset;
    return old;
}
