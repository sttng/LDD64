// Vortex 5.0.0 build 800 "vortexldd41" - Linear2 joint (restored).
// Mac 0x490165-0x49024a (static initialiser 0x490208 / 0x490233), Win 0x775000.
#include "Mdt/MdtConstraint.h"
#include "Mdt/MdtBcl.h"

// Mac 0x490165, Win 0x775000
void MEAPI MdtLinear2Reset(MdtConstraint* c)
{
    if (MdtConstraintIsEnabled(c))
        MdtConstraintDisable(c);
    MdtConstraintReset(c);
}

// Mac 0x490191 (VxLinear2)
MdtConstraint* MEAPI MdtLinear2Create(VinPool* pool)
{
    MdtConstraint* c = MdtConstraintCreateFromPool(pool);
    if (c != 0) {
        c->bcl.type = kMdtTypeLinear2;
        c->rowBuilder = MdtBclAddLinear2;
        c->bcl.maxRows = 8;
        c->setBodies = MdtConstraintSetBodies2;
        c->setAxis = MdtConstraintSetAxisAll;
        c->numLimits = 0;
        MdtLinear2Reset(c);
    }
    return c;
}
