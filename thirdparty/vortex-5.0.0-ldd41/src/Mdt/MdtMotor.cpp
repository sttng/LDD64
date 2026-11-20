// Vortex 5.0.0 build 800 "vortexldd41" - motor constraint (restored).
// Mac 0x49024a-0x49035b (static initialiser 0x490319 / 0x490344), Win 0x783b80.
#include "Mdt/MdtConstraint.h"
#include "Mdt/MdtBcl.h"

// Mac 0x49024a, Win 0x783b80
void MEAPI MdtMotorReset(MdtConstraint* c)
{
    if (MdtConstraintIsEnabled(c))
        MdtConstraintDisable(c);
    MdtLimitReset(&((MdtMotor*)c)->limit);
    MdtConstraintReset(c);
}

// Mac 0x490284 (VxMotorConstraint): one angular coordinate
MdtConstraint* MEAPI MdtMotorCreate(VinPool* pool)
{
    MdtConstraint* c = MdtConstraintCreateFromPool(pool);
    if (c != 0) {
        c->bcl.type = kMdtTypeMotor;
        c->rowBuilder = MdtBclAddMotor;
        c->bcl.maxRows = 2;
        c->setBodies = MdtConstraintSetBodies2;
        c->setAxis = MdtConstraintSetAxisAll;
        c->numLimits = 1;
        MdtMotorReset(c);
        MdtMotor* m = (MdtMotor*)c;
        m->limit.kind = kMdtLimitAngular;
        m->limit.index = 0;
        m->limit.me8 = 0;
    }
    return c;
}
