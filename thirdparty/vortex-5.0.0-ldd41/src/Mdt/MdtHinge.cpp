// Vortex 5.0.0 build 800 "vortexldd41" - hinge (restored).
// Mac 0x48fb5e-0x48fc8c (static initialiser 0x48fc4a / 0x48fc75), Win 0x7750d0-0x775110.
#include "Mdt/MdtConstraint.h"
#include "Mdt/MdtBcl.h"

// Mac 0x48fb5e, Win 0x7750d0
void MEAPI MdtHingeReset(MdtConstraint* c)
{
    if (MdtConstraintIsEnabled(c))
        MdtConstraintDisable(c);
    MdtLimitReset(&((MdtHinge*)c)->limit);
    MdtConstraintReset(c);
    c->bcl.friction[3].mAxis = 1;
}

// Mac 0x48fbab, Win 0x775110 (VxHinge)
MdtConstraint* MEAPI MdtHingeCreate(VinPool* pool)
{
    MdtConstraint* c = MdtConstraintCreateFromPool(pool);
    if (c != 0) {
        MdtHinge* h = (MdtHinge*)c;
        c->bcl.type = kMdtTypeHinge;
        c->rowBuilder = MdtBclAddHinge;
        c->bcl.maxRows = 0xd;
        c->setBodies = MdtConstraintSetBodies2;
        c->setAxis = MdtConstraintSetAxisAll;
        c->numLimits = 1;
        h->limit.kind = kMdtLimitAngular;
        h->limit.index = 0;
        h->limit.me8 = 1;
        h->m_be0 = -1;
        MdtHingeReset(c);
    }
    return c;
}
