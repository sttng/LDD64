// Vortex 5.0.0 build 800 "vortexldd41" - spring (restored).
// Mac 0x49288b-0x4929de (static initialiser 0x49299c / 0x4929c7), Win 0x782230-0x7822a0.
#include "Mdt/MdtConstraint.h"
#include "Mdt/MdtBcl.h"
#include <float.h>

// Mac 0x49288b, Win 0x782280: the distance coordinate's desired value (MdtLimit +0x10)
void MEAPI MdtSpringSetNaturalLength(MdtConstraint* c, MeReal l)
{
    if (c->bcl.type == kMdtTypeSpring)
        ((MdtSpring*)c)->limit.desiredVelocity = l;
}

// Mac 0x4928a9, Win 0x7822a0
MeReal MEAPI MdtSpringGetNaturalLength(const MdtConstraint* c)
{
    if (c->bcl.type == kMdtTypeSpring)
        return ((const MdtSpring*)c)->limit.desiredVelocity;
    return 0;
}

// Mac 0x4928c4, Win 0x782230
void MEAPI MdtSpringReset(MdtConstraint* c)
{
    if (MdtConstraintIsEnabled(c))
        MdtConstraintDisable(c);
    MdtSpring* s = (MdtSpring*)c;
    MdtLimitReset(&s->limit);
    s->limit.kind = kMdtLimitDistance;
    s->limit.control = kMdtLimitMotorized;
    MdtConstraintReset(c);
}

// Mac 0x49291b (VxSpring)
MdtConstraint* MEAPI MdtSpringCreate(VinPool* pool)
{
    MdtConstraint* c = MdtConstraintCreateFromPool(pool);
    if (c != 0) {
        c->bcl.type = kMdtTypeSpring;
        c->rowBuilder = MdtBclAddSpring;
        c->bcl.maxRows = 7;
        c->setBodies = MdtConstraintSetBodies2;
        c->setAxis = MdtConstraintSetAxisAll;
        c->numLimits = 1;
        ((MdtSpring*)c)->limit.me8 = 0;
        MdtSpringReset(c);
    }
    return c;
}

// ---- Win-only (dead-stripped on Mac, names inferred) -------------------------

// Win 0x7822c0: the spring's stiffness as the distance coordinate's motor compliance
// NOTE: like Win, 0 gives an infinite compliance (no zero test, unlike the limit setters)
void MEAPI MdtSpringSetStiffness(MdtConstraint* c, MeReal k)
{
    if (c->bcl.type != kMdtTypeSpring)
        return;
    MdtSpring* s = (MdtSpring*)c;
    if (k == DBL_MAX)
        s->limit.motorInvStiffness = 0;
    else
        s->limit.motorInvStiffness = 1.0 / k;
}

// Win 0x782320
void MEAPI MdtSpringSetDamping(MdtConstraint* c, MeReal d)
{
    if (c->bcl.type != kMdtTypeSpring)
        return;
    ((MdtSpring*)c)->limit.motorDamping = d;
}
