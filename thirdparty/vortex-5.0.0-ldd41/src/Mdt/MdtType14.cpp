// Vortex 5.0.0 build 800 "vortexldd41" - constraint type 0x14 accessors (restored, V2.1).
// Mac 0x48f926-0x48f949 (static initialiser 0x48f949 / 0x48f974), Win 0x852a90-0x852ab0 (the
// Win file also has the creator 0x852ac0 / reset 0x852a50). Names unknown.
#include "Mdt/MdtConstraint.h"

// Mac 0x48f926, Win 0x852a90
void MEAPI MdtType14SetValue(MdtConstraint* c, MeReal v)
{
    ((MdtType14*)c)->m_be0 = v;
}

// Mac 0x48f93b, Win 0x852ab0
MeReal MEAPI MdtType14GetValue(const MdtConstraint* c)
{
    return ((const MdtType14*)c)->m_be0;
}
