// Vortex 5.0.0 build 800 "vortexldd41" - ball and socket joint (restored).
// Mac 0x48cd47-0x48ce36 (static initialiser 0x48cdf4 / 0x48ce1f), Win 0x78db40.
#include "Mdt/MdtConstraint.h"
#include "Mdt/MdtBcl.h"

// Mac 0x48cd47, Win 0x78db40
void MEAPI MdtBSJointReset(MdtConstraint* c)
{
    if (MdtConstraintIsEnabled(c))
        MdtConstraintDisable(c);
    MdtConstraintReset(c);
}

// Mac 0x48cd73 (VxBallAndSocket ctor Mac 0x42fd4e)
MdtConstraint* MEAPI MdtBSJointCreate(VinPool* pool)
{
    MdtConstraint* c = MdtConstraintCreateFromPool(pool);
    if (c != 0) {
        c->bcl.type = kMdtTypeBallAndSocket;
        c->rowBuilder = MdtBclAddBSJoint;
        c->bcl.maxRows = 9;
        c->setBodies = MdtConstraintSetBodies2;
        c->setAxis = MdtConstraintSetAxisAll;
        ((MdtBSJoint*)c)->m_ae8 = -1;
        c->numLimits = 0;
        MdtBSJointReset(c);
    }
    return c;
}
