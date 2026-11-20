// Vortex 5.0.0 build 800 "vortexldd41" - cylindrical joint (restored).
// Mac 0x48f771-0x48f8ca (static initialiser 0x48f888 / 0x48f8b3), Win 0x783c90-0x783d00.
#include "Mdt/MdtConstraint.h"
#include "Mdt/MdtBcl.h"

// Mac 0x48f771, Win 0x783c90
void MEAPI MdtCylindricalReset(MdtConstraint* c)
{
    if (MdtConstraintIsEnabled(c))
        MdtConstraintDisable(c);
    MdtCylindrical* j = (MdtCylindrical*)c;
    MdtLimitReset(&j->limit[0]);
    MdtLimitReset(&j->limit[1]);
    MdtConstraintReset(c);
    c->bcl.friction[0].mAxis = 0;   // linear friction along the axis
    c->bcl.friction[3].mAxis = 0;   // angular friction about it
}

// Mac 0x48f7d6, Win 0x783d00 (VxCylindrical): a linear and an angular coordinate
MdtConstraint* MEAPI MdtCylindricalCreate(VinPool* pool)
{
    MdtConstraint* c = MdtConstraintCreateFromPool(pool);
    if (c != 0) {
        c->bcl.type = kMdtTypeCylindrical;
        c->rowBuilder = MdtBclAddCylindrical;
        c->bcl.maxRows = 0xe;
        c->setBodies = MdtConstraintSetBodies2;
        c->setAxis = MdtConstraintSetAxisAll;
        c->numLimits = 2;
        MdtCylindricalReset(c);
        MdtCylindrical* j = (MdtCylindrical*)c;
        j->limit[0].kind = kMdtLimitLinear;
        j->limit[0].index = 0;
        j->limit[0].me8 = 0;
        j->limit[1].kind = kMdtLimitAngular;
        j->limit[1].index = 0;
        j->limit[1].me8 = 0;
    }
    return c;
}
