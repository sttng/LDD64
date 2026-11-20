// Vortex 5.0.0 build 800 "vortexldd41" - MdtLimit / MdtSingleLimit (restored).
// Mac 0x48fc8c-0x490123 (static initialiser 0x490123 / 0x49014e), Win 0x783080-0x783930
// (file name inferred).
#include "Mdt/MdtConstraint.h"

#include <float.h>
#include <math.h>

// Mac 0x48fc8c
void MEAPI MdtSingleLimitReset(MdtSingleLimit* l)
{
    l->stop = 0;
    l->restitution = 0;
    l->invStiffness = 0;
    l->damping = 0;
    l->m20 = 0;
    l->m28 = DBL_MAX;
}

// Mac 0x48fcbe
void MEAPI MdtSingleLimitSetStop(MdtSingleLimit* l, MeReal stop)
{
    l->stop = stop;
}

// Mac 0x48fccf: stored as compliance (DBL_MAX -> 0, 0 -> DBL_MAX)
void MEAPI MdtSingleLimitSetStiffness(MdtSingleLimit* l, MeReal k)
{
    k = fabs(k);
    if (k != DBL_MAX) {
        if (k != 0.0)
            l->invStiffness = 1.0 / k;
        else
            l->invStiffness = DBL_MAX;
    } else {
        l->invStiffness = 0;
    }
}

// Mac 0x48fd3a
void MEAPI MdtSingleLimitSetDamping(MdtSingleLimit* l, MeReal d)
{
    if (d < 0.0)
        l->damping = -d;
    else
        l->damping = d;
}

// Mac 0x48fd6e
int MEAPI MdtLimitIsActive(const MdtLimit* l)
{
    return l->limitsActive != 0;
}

// Mac 0x48fd83
MdtSingleLimit* MEAPI MdtLimitGetLowerLimit(MdtLimit* l)
{
    return &l->lower;
}

// Mac 0x48fd8e
MdtSingleLimit* MEAPI MdtLimitGetUpperLimit(MdtLimit* l)
{
    return &l->upper;
}

// Mac 0x48fd9b, Win 0x783870
void MEAPI MdtLimitReset(MdtLimit* l)
{
    l->m00 = 0;
    l->position = 0;
    l->desiredVelocity = 0;
    l->mec = 0;
    l->motorInvStiffness = 0;
    l->motorDamping = 0;
    l->motorMaxForceHi = DBL_MAX;
    l->motorMaxForceLo = -DBL_MAX;
    l->previousPosition = 0;
    l->positionOffset = 0;
    l->m50 = 0;
    l->m18 = 0;
    l->lockPosition = 0;
    l->lockForceHi = DBL_MAX;
    l->lockForceLo = -DBL_MAX;
    l->lockStiffness = 0;
    l->control = kMdtLimitFree;
    l->limitsActive = 0;
    l->mf0 = 0;
    l->mf4 = 0;
    MdtSingleLimitReset(&l->lower);
    MdtSingleLimitReset(&l->upper);
    MdtSingleLimitSetStop(MdtLimitGetUpperLimit(l), DBL_MAX);
    MdtSingleLimitSetStop(MdtLimitGetLowerLimit(l), -DBL_MAX);
}

// Mac 0x48febc, Win 0x783250
MeReal MEAPI MdtLimitGetPosition(const MdtLimit* l)
{
    return l->position + l->positionOffset;
}

// Mac 0x48fed9
int MEAPI MdtLimitIsLocked(const MdtLimit* l)
{
    return l->control == kMdtLimitLocked;
}

// Mac 0x48feee
MeReal MEAPI MdtLimitGetLockPosition(const MdtLimit* l)
{
    return l->lockPosition;
}

// Mac 0x48fefc
MeReal MEAPI MdtLimitGetDesiredVelocity(const MdtLimit* l)
{
    return l->desiredVelocity;
}

// Mac 0x48ff07
MeReal MEAPI MdtLimitGetLockMaximumForce(const MdtLimit* l)
{
    return l->lockForceHi;
}

// Mac 0x48ff15
MeReal MEAPI MdtLimitGetLockMinimumForce(const MdtLimit* l)
{
    return l->lockForceLo;
}

// Mac 0x48ff23
void MEAPI MdtLimitSetLockStiffness(MdtLimit* l, MeReal k)
{
    l->lockStiffness = k;
}

// Mac 0x48ff38, Win 0x783330
MeReal MEAPI MdtLimitGetMotorMaxForce(const MdtLimit* l)
{
    return l->motorMaxForceHi;
}

// Mac 0x48ff43
void MEAPI MdtLimitSetLimitsActive(MdtLimit* l, int active)
{
    l->limitsActive = active != 0;
}

// Mac 0x48ff5b: the larger force goes to lockForceHi
void MEAPI MdtLimitSetLock(MdtLimit* l, MeReal position, MeReal f1, MeReal f2)
{
    l->lockPosition = position;
    if (f2 <= f1) {
        l->lockForceHi = f1;
        l->lockForceLo = f2;
    } else {
        l->lockForceHi = f2;
        l->lockForceLo = f1;
    }
}

// Mac 0x48ffa2
void MEAPI MdtLimitSetM18(MdtLimit* l, MeReal x)
{
    l->m18 = x;
}

// Mac 0x48ffb4, Win 0x7835f0
void MEAPI MdtLimitSetDesiredVelocity(MdtLimit* l, MeReal v)
{
    l->desiredVelocity = v;
}

// Mac 0x48ffc6, Win 0x783600
void MEAPI MdtLimitSetMotorMaxForces(MdtLimit* l, MeReal f1, MeReal f2)
{
    if (f2 <= f1) {
        l->motorMaxForceLo = f2;
        l->motorMaxForceHi = f1;
    } else {
        l->motorMaxForceLo = f1;
        l->motorMaxForceHi = f2;
    }
}

// Mac 0x48fff4
void MEAPI MdtLimitSetMotor(MdtLimit* l, MeReal v, MeReal f1, MeReal f2)
{
    MdtLimitSetMotorMaxForces(l, f1, f2);
    l->desiredVelocity = v;
}

// Mac 0x490036
MeReal MEAPI MdtLimitGetM18(const MdtLimit* l)
{
    return l->m18;
}

// Mac 0x490041, Win 0x783670
void MEAPI MdtLimitSetMotorStiffnessDamping(MdtLimit* l, MeReal k, MeReal d)
{
    if (k != DBL_MAX) {
        if (k != 0.0)
            l->motorInvStiffness = 1.0 / k;
        else
            l->motorInvStiffness = DBL_MAX;
    } else {
        l->motorInvStiffness = 0;
    }
    l->motorDamping = d;
}

// Mac 0x4900a7
MeReal MEAPI MdtLimitGetMotorStiffness(const MdtLimit* l)
{
    const MeReal c = l->motorInvStiffness;
    if (c != DBL_MAX) {
        if (c != 0.0)
            return 1.0 / c;
        return DBL_MAX;
    }
    return 0;
}

// Mac 0x4900f5
MeReal MEAPI MdtLimitGetMotorDamping(const MdtLimit* l)
{
    return l->motorDamping;
}

// Mac 0x490100
int MEAPI MdtLimitIsMotorized(const MdtLimit* l)
{
    return l->control == kMdtLimitMotorized;
}

// Mac 0x490115
int MEAPI MdtLimitGetMf0(const MdtLimit* l)
{
    return l->mf0;
}

// ---- Win-only members (dead-stripped on Mac; used by the Win-only VxConstraint API;
// names inferred) -------------------------------------------------------------------------

// Win 0x7830b0
MeReal MEAPI MdtSingleLimitGetStop(const MdtSingleLimit* l)
{
    return l->stop;
}

// Win 0x7830c0: the stiffness from the stored compliance (0 -> DBL_MAX)
MeReal MEAPI MdtSingleLimitGetStiffness(const MdtSingleLimit* l)
{
    if (l->invStiffness == 0.0)
        return DBL_MAX;
    return 1.0 / l->invStiffness;
}

// Win 0x7830f0
MeReal MEAPI MdtSingleLimitGetDamping(const MdtSingleLimit* l)
{
    return l->damping;
}

// Win 0x783100
MeReal MEAPI MdtSingleLimitGetRestitution(const MdtSingleLimit* l)
{
    return l->restitution;
}

// Win 0x7831b0: clamped to [0, 1]
void MEAPI MdtSingleLimitSetRestitution(MdtSingleLimit* l, MeReal r)
{
    if (1.0 <= r)
        l->restitution = 1.0;
    else if (r < 0.0)
        l->restitution = 0;
    else
        l->restitution = r;
}

// Win 0x783290
MeReal MEAPI MdtLimitGetM50(const MdtLimit* l)
{
    return l->m50;
}

// Win 0x783320
MeReal MEAPI MdtLimitGetLockStiffness(const MdtLimit* l)
{
    return l->lockStiffness;
}

// Win 0x7837d0
int MEAPI MdtLimitIsAngular(const MdtLimit* l)
{
    return l->kind == kMdtLimitAngular;
}

// Win 0x783820
void MEAPI MdtLimitSetMf0(MdtLimit* l, int v)
{
    l->mf0 = v;
}
