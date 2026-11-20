// Vortex 5.0.0 build 800 "vortexldd41" - Angular1Position3 joint (restored).
// Mac 0x48ac0d-0x48adcd (static initialiser 0x48ad8b / 0x48adb6; split off TU 530 by the TU
// tooling), Win 0x78d9b0-0x78da50.
#include "Mdt/MdtConstraint.h"
#include "Mdt/MdtBcl.h"

// Mac 0x48ac0d, Win 0x78d9b0
void MEAPI MdtAngular1Position3Reset(MdtConstraint* c)
{
    if (MdtConstraintIsEnabled(c))
        MdtConstraintDisable(c);
    MdtAngular1Position3* j = (MdtAngular1Position3*)c;
    MdtLimitReset(&j->limit[0]);
    MdtLimitReset(&j->limit[1]);
    MdtLimitReset(&j->limit[2]);
    MdtLimitReset(&j->limit[3]);
    MdtConstraintReset(c);
    c->bcl.friction[0].mAxis = 0;
    c->bcl.friction[1].mAxis = 0;
    c->bcl.friction[2].mAxis = 0;
    c->bcl.friction[3].mAxis = 0;
}

// Mac 0x48ac9d, Win 0x78da50 (VxAngular1Position3): one angular, three linear coordinates
MdtConstraint* MEAPI MdtAngular1Position3Create(VinPool* pool)
{
    MdtConstraint* c = MdtConstraintCreateFromPool(pool);
    if (c != 0) {
        c->bcl.type = kMdtTypeAngular1Position3;
        c->rowBuilder = MdtBclAddAngular1Position3;
        c->bcl.maxRows = 0x10;
        c->setBodies = MdtConstraintSetBodies2;
        c->setAxis = MdtConstraintSetAxisAll;
        c->numLimits = 4;
        MdtAngular1Position3Reset(c);
        MdtAngular1Position3* j = (MdtAngular1Position3*)c;
        j->limit[0].kind = kMdtLimitAngular;
        j->limit[0].index = 0;
        j->limit[0].me8 = 0;
        j->limit[1].kind = kMdtLimitLinear;
        j->limit[1].index = 0;
        j->limit[1].me8 = 0;
        j->limit[2].kind = kMdtLimitLinear;
        j->limit[2].index = 1;
        j->limit[2].me8 = 0;
        j->limit[3].kind = kMdtLimitLinear;
        j->limit[3].index = 2;
        j->limit[3].me8 = 0;
    }
    return c;
}
