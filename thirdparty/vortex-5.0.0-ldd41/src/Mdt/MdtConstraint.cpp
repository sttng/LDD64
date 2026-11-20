// Vortex 5.0.0 build 800 "vortexldd41" - MdtConstraint (restored).
// Mac 0x48cfa2-0x48f407 (static initialiser 0x48f3c5 / 0x48f3f0), Win 0x72de10-0x72fa70
// (/LTCG; the Win-only accessors in that range are not restored).
#include "Mdt/MdtConstraint.h"
#include "Mdt/MdtBody.h"
#include "Mdt/MdtBcl.h"
#include "Mdt/MdtWorld.h"
#include "Me/MeMath.h"
#include "Me/MeMessage.h"

#include <math.h>
#include <float.h>
#include <new>

// ---- inline vector helpers (Win 0x72dc10 / 0x72dc60, file-static; inlined on Mac) -----------

// Normalise (x, y, z) unless it is already of unit length within 1e-7; a squared length that is
// not > 0 (zero, negative zero, NaN) gives (1, 0, 0).
static void MdtNormalise(MeReal& x, MeReal& y, MeReal& z)
{
    if (!(fabs(((0.0 + x * x) + y * y) + z * z - 1.0) < 1e-07)) {
        const MeReal s = (x * x + y * y) + z * z;
        if (s > 0.0) {
            const MeReal r = 1.0 / sqrt(s);
            x = x * r;
            y = y * r;
            z = z * r;
        } else {
            x = 1.0;
            y = 0.0;
            z = 0.0;
        }
    }
}

// Two unit vectors b, c completing the unit vector n to a right-handed frame.
static void MdtPlaneSpace(const MeReal* n, MeReal* b, MeReal* c)
{
    if (fabs(n[0]) + fabs(n[2]) > fabs(n[1])) {
        const MeReal r = 1.0 / sqrt(n[2] * n[2] + n[0] * n[0]);
        b[0] = n[2] * r;
        b[1] = 0.0;
        b[2] = -n[0] * r;
        c[0] = n[1] * b[2];
        c[1] = n[2] * b[0] - b[2] * n[0];
        c[2] = b[0] * -n[1];
    } else {
        const MeReal r = 1.0 / sqrt(n[2] * n[2] + n[1] * n[1]);
        b[0] = 0.0;
        b[1] = -n[2] * r;
        b[2] = n[1] * r;
        c[0] = n[1] * b[2] - n[2] * b[1];
        c[1] = b[2] * -n[0];
        c[2] = n[0] * b[1];
    }
}

// ---- basic accessors --------------------------------------------------------------------------

// Mac 0x48cfa2, Win 0x72e150: on the world's enabled list
int MEAPI MdtConstraintIsEnabled(const MdtConstraint* c)
{
    return c->next != 0 || c->prevNext != 0;
}

// Mac 0x48cfc1, Win 0x72e190
MdtBody* MEAPI MdtConstraintGetBody(const MdtConstraint* c, int i)
{
    if (c == 0)
        return 0;
    return c->body[i];
}

// Mac 0x48cfdc, Win 0x72e200
void MEAPI MdtConstraintGetTorque(const MdtConstraint* c, int i, MeVector3 t)
{
    t[0] = c->torque[i][0];
    t[1] = c->torque[i][1];
    t[2] = c->torque[i][2];
}

// Mac 0x48d013, Win 0x72e4a0
MdtConstraint* MEAPI MdtConstraintGetNext(const MdtConstraint* c)
{
    return c->next;
}

// Mac 0x48d01e, Win 0x72e4b0 (a second, identical accessor)
MdtConstraint* MEAPI MdtConstraintGetNextEnabled(const MdtConstraint* c)
{
    return c->next;
}

// Mac 0x48d029, Win 0x72e4c0
void MEAPI MdtConstraintSetBodies(MdtConstraint* c, MdtBody* b0, MdtBody* b1)
{
    c->setBodies(c, b0, b1);
}

// Mac 0x48d04b, Win 0x72e4e0
void MEAPI MdtConstraintSetUserData(MdtConstraint* c, void* data)
{
    c->userData = data;
}

// Mac 0x48d05d, Win 0x72ebc0
int MEAPI MdtConstraintGetNumLimits(const MdtConstraint* c)
{
    return c->numLimits;
}

// Mac 0x48d06b, Win 0x72ebd0: the limits follow the MdtConstraint (MdtHinge::limit, ...)
MdtLimit* MEAPI MdtConstraintGetLimit(MdtConstraint* c, int i)
{
    if (i < c->numLimits && i > -1)
        return (MdtLimit*)(c + 1) + i;
    return 0;
}

// Mac 0x48d097, Win 0x72ecf0
void MEAPI MdtConstraintSetLimitPosition(MdtConstraint* c, int i, MeReal p)
{
    MdtLimit* l = MdtConstraintGetLimit(c, i);
    if (l == 0) // NOTE: guard added; both binaries write through the null pointer
        return;
    l->previousPosition = p;
    l->position = p;
}

// Mac 0x48d0ca
MeReal MEAPI MdtConstraintGetLimitPosition(MdtConstraint* c, int i)
{
    MdtLimit* l = MdtConstraintGetLimit(c, i);
    if (l == 0) // NOTE: guard added; Mac reads through the null pointer
        return 0;
    return l->position;
}

// Mac 0x48d0e8, Win 0x72eeb0
void MEAPI MdtConstraintSetRelaxConstraintDamping(MdtConstraint* c, int row, MeReal d)
{
    c->bcl.relaxDamping[row] = d;
}

// Mac 0x48d102, Win 0x72eef0
void MEAPI MdtConstraintSetForceLimits(MdtConstraint* c, int row, MeReal lo, MeReal hi)
{
    c->bcl.forceLo[row] = lo;
    c->bcl.forceHi[row] = hi;
}

// Mac 0x48d129, Win 0x72ef20
void MEAPI MdtConstraintGetForceLimits(const MdtConstraint* c, int row, MeReal* lo, MeReal* hi)
{
    *lo = c->bcl.forceLo[row];
    *hi = c->bcl.forceHi[row];
}

// Mac 0x48d154, Win 0x72ef50
MeReal MEAPI MdtConstraintGetRelaxConstraintStiffness(const MdtConstraint* c, int row)
{
    MeReal k = c->bcl.relaxInvStiffness[row];
    if (k != 0.0)
        k = 1.0 / k;
    else
        k = DBL_MAX;
    return k;
}

// Mac 0x48d1a2, Win 0x72efe0
void MEAPI MdtConstraintDisableRelaxConstraint(MdtConstraint* c, int row)
{
    c->bcl.relaxActive[row] = 0;
}

// Mac 0x48d1b8, Win 0x72f000
void MEAPI MdtConstraintEnableRelaxConstraint(MdtConstraint* c, int row)
{
    c->bcl.relaxActive[row] = 1;
}

// Mac 0x48d1ce, Win 0x72f020
int MEAPI MdtConstraintIsRelaxConstraintEnabled(const MdtConstraint* c, int row)
{
    return c->bcl.relaxActive[row];
}

// Mac 0x48d1e0, Win 0x72f040
int MEAPI MdtConstraintIsLocked(const MdtConstraint* c)
{
    return c->rowBuilder == MdtBclAddLock;
}

// Mac 0x48d201, Win 0x72f0d0: back to the row builder of the constraint's type
void MEAPI MdtConstraintUnlockConstraint(MdtConstraint* c)
{
    if (!MdtConstraintIsLocked(c))
        return;
    switch (c->bcl.type) {
    case 0x00: c->rowBuilder = MdtBclAddBSJoint; break;
    case 0x01: c->rowBuilder = MdtBclAddHinge; break;
    case 0x02: c->rowBuilder = MdtBclAddType02; break;
    case 0x03: c->rowBuilder = MdtBclAddType03; break;
    case 0x04: c->rowBuilder = MdtBclAddType04; break;
    case 0x06: c->rowBuilder = MdtBclAddType06; break;
    case 0x07: c->rowBuilder = MdtBclAddRPROJoint; break;
    case 0x08: c->rowBuilder = MdtBclAddType08; break;
    case 0x0a: c->rowBuilder = MdtBclAddType0a; break;
    case 0x0b: c->rowBuilder = MdtBclAddLinear2; break;
    case 0x0c: c->rowBuilder = MdtBclAddType0c; break;
    case 0x0e: c->rowBuilder = MdtBclAddSpring; break;
    case 0x10: c->rowBuilder = MdtBclAddType10; break;
    case 0x11: c->rowBuilder = MdtBclAddType11; break;
    case 0x12: c->rowBuilder = MdtBclAddWinch; break;
    case 0x13: c->rowBuilder = MdtBclAddCylindrical; break;
    case 0x14: c->rowBuilder = MdtBclAddType14; break;
    case 0x15: c->rowBuilder = MdtBclAddType15; break;
    }
}

// Mac 0x48d3b6, Win 0x72f350
int MEAPI MdtConstraintGetFlags(const MdtConstraint* c)
{
    return c->flags;
}

// Mac 0x48d3c4, Win 0x72f360
void MEAPI MdtConstraintSetFlags(MdtConstraint* c, int flags)
{
    c->flags = flags;
}

// Mac 0x48d3d6, Win 0x72ee40 (string)
void MEAPI MdtConstraintSetRelaxConstraintStiffness(MdtConstraint* c, int row, MeReal k)
{
    if (k != 0.0) {
        if (k != DBL_MAX)
            k = 1.0 / k;
        else
            k = 0.0;
        c->bcl.relaxInvStiffness[row] = k;
    } else {
        MeWarning(0, "MdtConstraintSetRelaxConstraintStiffness: A constraint can't be relaxed "
                     "with zero stiffess.\n");
    }
}

// Mac 0x48d43f, Win 0x72ed70 (string)
void MEAPI MdtConstraintRelaxConstraint(MdtConstraint* c, int row, MeReal stiffness, MeReal damping,
                                  MeReal m)
{
    if (stiffness != 0.0) {
        c->bcl.relaxDamping[row] = damping;
        if (stiffness != DBL_MAX)
            stiffness = 1.0 / stiffness;
        else
            stiffness = 0.0;
        c->bcl.relaxInvStiffness[row] = stiffness;
        c->bcl.relaxM5e8[row] = m;
        c->bcl.relaxActive[row] = 1;
    } else {
        MeWarning(0, "MdtConstraintRelaxConstraint: A constraint can't be relaxed with zero "
                     "stiffess.\n");
    }
}

// Mac 0x48d4cf, Win 0x72ec00: measure coordinate i (the Bcl function updates the limit's
// position fields, which are restored afterwards)
MeReal MEAPI MdtConstraintCalculatePosition(MdtConstraint* c, int i)
{
    MdtLimit* l = MdtConstraintGetLimit(c, i);
    if (l == 0) // NOTE: guard added; both binaries read and write through the null pointer
        return 0;
    const MeReal previous = l->previousPosition;
    const MeReal position = l->position;
    MeReal p;
    if (l->kind == kMdtLimitAngular)
        p = MdtBclGetAngularCoordinate(&c->bcl, l);
    else if (l->kind == kMdtLimitDistance)
        p = MdtBclGetDistanceCoordinate(&c->bcl, l);
    else if (l->kind == kMdtLimitLinear)
        p = MdtBclGetLinearCoordinate(&c->bcl, l);
    else
        p = 0.0;
    l->previousPosition = previous;
    l->position = position;
    return p + l->positionOffset;
}

// Mac 0x48d59f, Win 0x72f7f0
MeReal MEAPI MdtConstraintGetPosition(MdtConstraint* c, int i)
{
    MdtLimit* l = MdtConstraintGetLimit(c, i);
    if (l == 0)
        return 0;
    if (MdtLimitIsActive(l) == 0 && MdtLimitGetMf0(l) == 0)
        return MdtConstraintCalculatePosition(c, i);
    return MdtLimitGetPosition(l);
}

// ---- attachment frames ------------------------------------------------------------------------

// Mac 0x48d607, Win 0x72e390: primary / secondary axis of body i's frame in world coordinates
void MEAPI MdtConstraintGetBodyAxes(const MdtConstraint* c, int i, MeVector3 primary, MeVector3 secondary)
{
    MdtConvertVector(c->body[i], c->bcl.frame[i][0], 0, primary);
    MdtConvertVector(c->body[i], c->bcl.frame[i][1], 0, secondary);
}

// Mac 0x48d679, Win 0x72e500: attachment point of body i, given in world coordinates
void MEAPI MdtConstraintSetBodyPosition(MdtConstraint* c, int i, MeReal x, MeReal y, MeReal z)
{
    MeVector3 p;
    p[0] = x;
    p[1] = y;
    p[2] = z;
    MdtConvertPositionVector(0, p, c->body[i], c->bcl.frame[i][3]);
}

// Mac 0x48d6e5, Win 0x72e5f0
void MEAPI MdtConstraintSetPosition(MdtConstraint* c, MeReal x, MeReal y, MeReal z)
{
    if (c->bcl.type == kMdtTypeContact) {
        MdtContactSetPosition(c, x, y, z);
    } else {
        MdtConstraintSetBodyPosition(c, 0, x, y, z);
        MdtConstraintSetBodyPosition(c, 1, x, y, z);
    }
}

// Mac 0x48d7a4, Win 0x72e270
void MEAPI MdtConstraintGetBodyPosition(const MdtConstraint* c, int i, MeVector3 p)
{
    MdtConvertPositionVector(c->body[i], c->bcl.frame[i][3], 0, p);
}

// Mac 0x48d7dd, Win 0x72f520: attach body b (may be 0 = the world) as body i. The frame is
// carried over from the previous body so it stays fixed in space.
void MEAPI MdtConstraintSetBody(MdtConstraint* c, MdtBody* b, int i)
{
    MdtConstraintResetPartitionInfo(&c->partitionInfo);
    MdtBody* old = c->body[i];
    if (old != b) {
        MdtConvertVector(old, c->bcl.frame[i][0], b, c->bcl.frame[i][0]);
        MdtConvertVector(c->body[i], c->bcl.frame[i][1], b, c->bcl.frame[i][1]);
        MdtConvertVector(c->body[i], c->bcl.frame[i][2], b, c->bcl.frame[i][2]);
        MdtConvertPositionVector(c->body[i], c->bcl.frame[i][3], b, c->bcl.frame[i][3]);
    }
    c->body[i] = b;
    if (b != 0) {
        if (b->frozen == 0 && MdtConstraintIsEnabled(c))
            c->bcl.keaBody[i] = &b->keaBody;
        else
            c->bcl.keaBody[i] = 0;
        if ((b->frozen & 1) == 0)
            c->bcl.vel[i] = b->keaBody.vel;
        else
            c->bcl.vel[i] = 0;
        c->bcl.comTM[i] = &b->keaBody.comTM;
        c->bcl.qrot[i] = b->keaBody.qrot;
        return;
    }
    c->bcl.keaBody[i] = 0;
    c->bcl.comTM[i] = 0;
    c->bcl.vel[i] = 0;
    c->bcl.qrot[i] = 0;
}

// Mac 0x48d955, Win 0x72f650: the default MdtConstraint::setBodies
void MEAPI MdtConstraintSetBodies2(MdtConstraint* c, MdtBody* b0, MdtBody* b1)
{
    MdtConstraintSetBody(c, b0, 0);
    MdtConstraintSetBody(c, b1, 1);
}

// Mac 0x48d993, Win 0x72f630
void MEAPI MdtConstraintSetBodyAt(MdtConstraint* c, int i, MdtBody* b)
{
    MdtConstraintSetBody(c, b, i);
}

// ---- enable / disable -------------------------------------------------------------------------

// Mac 0x48d9b4, Win 0x72e0b0: off the world's enabled list; the body nodes go back to the pool
void MEAPI MdtConstraintDisable(MdtConstraint* c)
{
    if (c->next == 0) {
        if (c->prevNext == 0)
            return;
    } else {
        c->next->prevNext = c->prevNext;
    }
    *c->prevNext = c->next;
    c->next = 0;
    c->prevNext = 0;
    c->world->numEnabledConstraints--;
    MdtConstraintResetPartitionInfo(&c->partitionInfo);
    c->visited = 0;
    for (int i = 0; i < c->bcl.numBodies; ++i) {
        MdtConstraintNode* n = c->node[i];
        if (n != 0) {
            if (n->next != 0)
                n->next->prevNext = n->prevNext;
            *n->prevNext = n->next;
            n->next = 0;
            n->prevNext = 0;
            VinPoolPutStruct(&c->world->constraintNodePool, n);
            c->node[i] = 0;
        }
    }
}

// Mac 0x48da72, Win 0x72f680
void MEAPI MdtConstraintFreeToPool(MdtConstraint* c, VinPool* pool)
{
    if (c->bcl.type != -1) {
        if (MdtConstraintIsEnabled(c))
            MdtConstraintDisable(c);
        c->bcl.type = -1;
        VinPoolPutStruct(pool, c);
    }
}

// Mac 0x48dab8, Win 0x72fa00
void MEAPI MdtConstraintDestroy(MdtConstraint* c)
{
    c->world->numConstraints--;
    MdtConstraintFreeToPool(c, &c->world->constraintPool);
}

// Mac 0x48dad9, Win 0x72e030: if one body is awake (or animated), wake all dynamic bodies
void MEAPI MdtConstraintWakeBodies(MdtConstraint* c)
{
    for (int i = 0; i < c->bcl.numBodies; ++i) {
        MdtBody* b = c->body[i];
        // NOTE: both binaries read b->frozen even for b == 0 (the world); guarded here.
        const bool wake = (b != 0 && b->frozen == 0) ? b->enabledTime > -1
                                                      : (b != 0 && b->frozen == 2);
        if (wake) {
            for (int j = 0; j < c->bcl.numBodies; ++j) {
                MdtBody* o = c->body[j];
                if (o != 0 && o->frozen == 0)
                    MdtBodyEnable(o);
            }
            return;
        }
    }
}

// Mac 0x48db4d, Win 0x72dff0
void MEAPI MdtConstraintEnableBodies(MdtConstraint* c)
{
    for (int i = 0; i < c->bcl.numBodies; ++i) {
        MdtBody* b = c->body[i];
        if (b != 0 && b->frozen == 0)
            MdtBodyEnable(b);
    }
}

// Mac 0x48db91, Win 0x72f6c0: onto the world's enabled list (needs at least one body); every
// body gets a node in its constraint list
void MEAPI MdtConstraintEnable(MdtConstraint* c)
{
    MdtWorld* w = c->world;
    if (c->prevNext == 0 && (c->body[0] != 0 || c->body[1] != 0)) {
        c->next = w->enabledConstraints;
        c->prevNext = &w->enabledConstraints;
        if (w->enabledConstraints != 0)
            w->enabledConstraints->prevNext = &c->next;
        w->enabledConstraints = c;
        w->numEnabledConstraints++;
        for (int i = 0; i < c->bcl.numBodies; ++i) {
            MdtBody* b = c->body[i];
            if (b != 0) {
                MdtConstraintSetBody(c, b, i);
                MdtConstraintNode* n = (MdtConstraintNode*)VinPoolGetStruct(&w->constraintNodePool);
                n->constraint = c;
                n->next = c->body[i]->constraints;
                n->prevNext = &c->body[i]->constraints;
                if (c->body[i]->constraints != 0)
                    c->body[i]->constraints->prevNext = &n->next;
                c->body[i]->constraints = n;
                c->node[i] = n;
            }
        }
    }
}

// Common part of every constraint's friction set-up (inlined twice on Mac).
static void MdtConstraintResetFriction(Vx::VxConstraintFriction* f)
{
    f->setProportional(false);
    f->setAxis(0);
    f->setCoefficient(0);
    f->setLoss(0);
    f->setMaxForce(0);
    f->setEnabled(false);
    f->setScale(1.0);
}

// Mac 0x48dc64, Win 0x72de10
MdtConstraint* MEAPI MdtConstraintCreateFromPool(VinPool* pool)
{
    MdtConstraint* c = (MdtConstraint*)VinPoolGetStruct(pool);
    if (c != 0) {
        c->bcl.numBodies = 2;
        c->body[1] = 0;
        c->body[0] = 0;
        c->node[1] = 0;
        c->node[0] = 0;
        c->bcl.maxRows = -1;
        c->bcl.keaBody[1] = 0;
        c->bcl.keaBody[0] = 0;
        for (int i = 0; i != 6; ++i) {
            Vx::VxConstraintFriction* f = new (&c->bcl.friction[i]) Vx::VxConstraintFriction();
            MdtConstraintResetFriction(f);
            f->mStatus = 0;
            f->mDirty = &c->bcl.frictionDirty;
        }
        c->bcl.frictionDirty = 0;
        c->bcl.comTM[1] = 0;
        c->bcl.comTM[0] = 0;
        c->bcl.qrot[1] = 0;
        c->bcl.qrot[0] = 0;
        c->bcl.vel[1] = 0;
        c->bcl.vel[0] = 0;
        c->next = 0;
        c->prevNext = 0;
        c->rowBuilder = 0;
        c->world = 0;
        c->partitionInfo.state = 0;
        c->flags = 0;
        c->ignored = 0;
    }
    return c;
}

// Mac 0x48dee4, Win 0x72f380
MdtConstraint* MEAPI MdtConstraintCreate(MdtWorld* w)
{
    MdtConstraint* c = MdtConstraintCreateFromPool(&w->constraintPool);
    if (c != 0) {
        w->numConstraints++;
        w->sizesChanged++;
        c->world = w;
    }
    return c;
}

// Mac 0x48df0e, Win 0x72f3b0: clear the solver output of the last step
void MEAPI MdtConstraintResetForces(MdtConstraint* c)
{
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < c->bcl.numBodies; ++i) {
            c->torque[i][j] = 0;
            c->force[i][j] = 0;
        }
    }
    for (int k = 0; k != 16; ++k) {
        c->bcl.lambda2[k] = 0;
        c->bcl.lambda[k] = 0;
        c->bcl.rowStatus[k] = 0;
    }
    c->bcl.lambda[16] = 4294967295.0;   // the original's m86c (MdtTypes.h)
    for (int i = 0; i < MdtConstraintGetNumLimits(c); ++i) {
        MdtLimit* l = MdtConstraintGetLimit(c, i);
        l->previousPosition = 0;
        l->position = 0;
        if (MdtLimitIsLocked(l) || MdtLimitIsMotorized(l) || MdtLimitGetMf0(l))
            MdtConstraintCalculatePosition(c, i);
        l->previousPosition = l->position;
    }
    for (int i = 0; i != 6; ++i) {
        Vx::VxConstraintFriction& f = c->bcl.friction[i];
        f.mStatus = 0;
        if (f.mEnabled)
            f.mMaxForce = 0;
    }
}

// Mac 0x48e050, Win 0x72e1d0
void MEAPI MdtConstraintGetForce(const MdtConstraint* c, int i, MeVector3 f)
{
    f[0] = c->force[i][0];
    f[1] = c->force[i][1];
    f[2] = c->force[i][2];
}

// Mac 0x48e07e, Win 0x72e5c0
void MEAPI MdtConstraintSetBodyPositionRel(MdtConstraint* c, int i, MeReal x, MeReal y, MeReal z)
{
    c->bcl.frame[i][3][0] = x;
    c->bcl.frame[i][3][1] = y;
    c->bcl.frame[i][3][2] = z;
}

// Mac 0x48e0b3, Win 0x72e410
void MEAPI MdtConstraintGetBodyAxesRel(const MdtConstraint* c, int i, MeVector3 primary, MeVector3 secondary)
{
    primary[0] = c->bcl.frame[i][0][0];
    primary[1] = c->bcl.frame[i][0][1];
    primary[2] = c->bcl.frame[i][0][2];
    secondary[0] = c->bcl.frame[i][1][0];
    secondary[1] = c->bcl.frame[i][1][1];
    secondary[2] = c->bcl.frame[i][1][2];
}

// Mac 0x48e113, Win 0x72e320
void MEAPI MdtConstraintGetBodyPositionRel(const MdtConstraint* c, int i, MeVector3 p)
{
    p[0] = c->bcl.frame[i][3][0];
    p[1] = c->bcl.frame[i][3][1];
    p[2] = c->bcl.frame[i][3][2];
}

// Mac 0x48e14a, Win 0x72e2a0: attachment point in the body's user frame (the stored one is
// relative to the centre of mass)
void MEAPI MdtConstraintGetBodyPositionCOM(const MdtConstraint* c, int i, MeVector3 p)
{
    const MdtBody* b = c->body[i];
    const MeReal* q = c->bcl.frame[i][3];
    if (b == 0 || b->useRelativeTransform == 0) {
        p[0] = q[0];
        p[1] = q[1];
        p[2] = q[2];
        return;
    }
    MeMatrix4 m;
    for (int r = 0; r < 4; ++r)
        for (int k = 0; k < 4; ++k)
            m[r][k] = b->relativeTransform[r][k];
    MeMatrix4TMInvert(m);
    const MeReal x = m[0][0] * q[0] + m[1][0] * q[1] + m[2][0] * q[2];
    p[0] = x;
    const MeReal y = m[0][1] * q[0] + m[1][1] * q[1] + m[2][1] * q[2];
    p[1] = y;
    const MeReal qx = q[0], qy = q[1], qz = q[2];
    p[0] = x + m[3][0];
    p[1] = y + m[3][1];
    p[2] = m[0][2] * qx + m[1][2] * qy + m[2][2] * qz + m[3][2];
}

// Mac 0x48e358, Win 0x72e550
void MEAPI MdtConstraintSetBodyPositionCOM(MdtConstraint* c, int i, MeReal x, MeReal y, MeReal z)
{
    MeReal* q = c->bcl.frame[i][3];
    q[0] = x;
    q[1] = y;
    q[2] = z;
    const MdtBody* b = c->body[i];
    if (b != 0 && b->useRelativeTransform != 0) {
        const MeReal (*R)[4] = b->relativeTransform;
        const MeReal px = q[0], py = q[1];
        q[0] = px * R[0][0] + py * R[1][0] + z * R[2][0] + R[3][0];
        q[1] = px * R[0][1] + py * R[1][1] + z * R[2][1] + R[3][1];
        q[2] = px * R[0][2] + py * R[1][2] + z * R[2][2] + R[3][2];
    }
}

// Mac 0x48e463, Win 0x72f850
void MEAPI MdtConstraintReset(MdtConstraint* c)
{
    c->bcl.partitionCount = 0;
    c->visited = 0;
    c->userData = 0;
    c->name = 0;
    c->flags = 0;
    c->ignored = 0;
    for (int k = 0; k != 16; ++k) {
        c->bcl.relaxM5e8[k] = 0;
        c->bcl.relaxDamping[k] = 0;
        c->bcl.relaxInvStiffness[k] = 0;
        c->bcl.relaxActive[k] = 0;
        c->bcl.m768[k] = 0;
        c->bcl.rowStatus[k] = 0;
        c->bcl.forceLo[k] = -DBL_MAX;
        c->bcl.forceHi[k] = DBL_MAX;
    }
    for (int i = 0; i != 6; ++i) {
        MdtConstraintResetFriction(&c->bcl.friction[i]);
        c->bcl.friction[i].mStatus = 0;
    }
    if (MdtConstraintIsLocked(c))
        MdtConstraintUnlockConstraint(c);
    for (int i = 0; i < c->bcl.numBodies; ++i) {
        c->bcl.keaBody[i] = 0;
        c->bcl.comTM[i] = 0;
        c->bcl.vel[i] = 0;
        c->bcl.qrot[i] = 0;
        c->body[i] = 0;
        c->node[i] = 0;
        for (int r = 0; r < 4; ++r)
            for (int k = 0; k < 4; ++k)
                c->bcl.frame[i][r][k] = (r == k) ? 1.0 : 0.0;
    }
    MdtConstraintResetForces(c);
}

// Mac 0x48e739, Win 0x72df10: the default MdtConstraint::setAxis - primary axis (world) for
// every body, the other two axes chosen perpendicular
void MEAPI MdtConstraintSetAxisAll(MdtConstraint* c, MeReal x, MeReal y, MeReal z)
{
    MeReal a[3] = { x, y, z };
    MdtNormalise(a[0], a[1], a[2]);
    MeReal b[3], d[3];
    MdtPlaneSpace(a, b, d);
    for (int i = 0; i < c->bcl.numBodies; ++i) {
        MdtConvertVector(0, a, c->body[i], c->bcl.frame[i][0]);
        MdtConvertVector(0, b, c->body[i], c->bcl.frame[i][1]);
        MdtConvertVector(0, d, c->body[i], c->bcl.frame[i][2]);
    }
}

// Mac 0x48ea29, Win 0x72e980: body i's frame axes given relative to the body
void MEAPI MdtConstraintSetBodyAxesRel(MdtConstraint* c, int i, MeReal px, MeReal py, MeReal pz,
                                 MeReal sx, MeReal sy, MeReal sz)
{
    MdtNormalise(px, py, pz);
    MdtNormalise(sx, sy, sz);
    MeReal (*f)[4] = c->bcl.frame[i];
    f[0][0] = px;
    f[0][1] = py;
    f[0][2] = pz;
    f[1][0] = sx;
    f[1][1] = sy;
    f[1][2] = sz;
    f[2][0] = py * sz - sy * pz;
    f[2][1] = sx * pz - sz * px;
    f[2][2] = sy * px - py * sx;
}

// Mac 0x48ec40, Win 0x72e8b0
void MEAPI MdtConstraintSetBodyAxisRel(MdtConstraint* c, int i, MeReal x, MeReal y, MeReal z)
{
    MeReal a[3] = { x, y, z };
    MdtNormalise(a[0], a[1], a[2]);
    MeReal b[3], d[3];
    MdtPlaneSpace(a, b, d);
    MeReal (*f)[4] = c->bcl.frame[i];
    f[0][0] = a[0];
    f[0][1] = a[1];
    f[0][2] = a[2];
    f[1][0] = b[0];
    f[1][1] = b[1];
    f[1][2] = b[2];
    f[2][0] = d[0];
    f[2][1] = d[1];
    f[2][2] = d[2];
}

// Mac 0x48ee63, Win 0x72e770: body i's frame axes given in world coordinates
void MEAPI MdtConstraintSetBodyAxes(MdtConstraint* c, int i, MeReal px, MeReal py, MeReal pz,
                              MeReal sx, MeReal sy, MeReal sz)
{
    MeReal p[3] = { px, py, pz };
    MeReal s[3] = { sx, sy, sz };
    // NOTE: here a squared length <= 0 gives (1, 0, 0) (a NaN one is normalised); the other
    // setters use "> 0" (MdtNormalise). Same for every finite input.
    if (!(fabs(((0.0 + p[0] * p[0]) + p[1] * p[1]) + p[2] * p[2] - 1.0) < 1e-07)) {
        const MeReal l = (p[0] * p[0] + p[1] * p[1]) + p[2] * p[2];
        if (l <= 0.0) {
            p[0] = 1.0; p[1] = 0.0; p[2] = 0.0;
        } else {
            const MeReal r = 1.0 / sqrt(l);
            p[0] = px * r; p[1] = py * r; p[2] = pz * r;
        }
    }
    if (!(fabs(((0.0 + s[0] * s[0]) + s[1] * s[1]) + s[2] * s[2] - 1.0) < 1e-07)) {
        const MeReal l = (s[0] * s[0] + s[1] * s[1]) + s[2] * s[2];
        if (l <= 0.0) {
            s[0] = 1.0; s[1] = 0.0; s[2] = 0.0;
        } else {
            const MeReal r = 1.0 / sqrt(l);
            s[0] = sx * r; s[1] = sy * r; s[2] = sz * r;
        }
    }
    MeReal t[3];
    t[0] = p[1] * s[2] - p[2] * s[1];
    t[1] = p[2] * s[0] - s[2] * p[0];
    t[2] = s[1] * p[0] - p[1] * s[0];
    MdtConvertVector(0, p, c->body[i], c->bcl.frame[i][0]);
    MdtConvertVector(0, s, c->body[i], c->bcl.frame[i][1]);
    MdtConvertVector(0, t, c->body[i], c->bcl.frame[i][2]);
}

// Mac 0x48f0f8, Win 0x72e6b0
void MEAPI MdtConstraintSetBodyAxis(MdtConstraint* c, int i, MeReal x, MeReal y, MeReal z)
{
    MeReal a[3] = { x, y, z };
    MdtNormalise(a[0], a[1], a[2]);
    MeReal b[3], d[3];
    MdtPlaneSpace(a, b, d);
    MdtConvertVector(0, a, c->body[i], c->bcl.frame[i][0]);
    MdtConvertVector(0, b, c->body[i], c->bcl.frame[i][1]);
    MdtConvertVector(0, d, c->body[i], c->bcl.frame[i][2]);
}

// ---- Win-only members (dead-stripped on Mac; used by the Win-only VxConstraint API;
// names inferred) -------------------------------------------------------------------------

// Win 0x72f080 (name from the string): switch to the lock row builder (6 rows); false and a
// warning for constraints with fewer rows
int MEAPI MdtConstraintLockConstraint(MdtConstraint* c)
{
    if (c->bcl.maxRows >= 6) {
        c->rowBuilder = MdtBclAddLock;
        return 1;
    }
    MeWarning(0, "MdtConstraintLockConstraint: %s not supported on this constraint\n", c->name);
    return 0;
}

// Win 0x72eca0: the position offset is chosen so that the coordinate reads p
// NOTE: like Win, no null check of the limit
void MEAPI MdtConstraintSetLimitCurrentPosition(MdtConstraint* c, int i, MeReal p)
{
    MdtLimit* l = MdtConstraintGetLimit(c, i);
    l->positionOffset = 0;
    const MeReal q = MdtConstraintCalculatePosition(c, i);
    l->position = q;
    l->positionOffset = p - q;
}

// Win 0x72ef80
MeReal MEAPI MdtConstraintGetRelaxConstraintDamping(const MdtConstraint* c, int row)
{
    return c->bcl.relaxDamping[row];
}

// Win 0x72eed0
void MEAPI MdtConstraintSetRelaxConstraintLoss(MdtConstraint* c, int row, MeReal loss)
{
    c->bcl.relaxM5e8[row] = loss;
}

// Win 0x72efa0
MeReal MEAPI MdtConstraintGetRelaxConstraintLoss(const MdtConstraint* c, int row)
{
    return c->bcl.relaxM5e8[row];
}

// Win 0x72edf0: stiffness (from the stored compliance), damping, loss
void MEAPI MdtConstraintGetRelaxConstraint(const MdtConstraint* c, int row, MeReal* stiffness,
                                           MeReal* damping, MeReal* loss)
{
    *damping = c->bcl.relaxDamping[row];
    if (c->bcl.relaxInvStiffness[row] == 0.0)
        *stiffness = DBL_MAX;
    else
        *stiffness = 1.0 / c->bcl.relaxInvStiffness[row];
    *loss = c->bcl.relaxM5e8[row];
}

// Win 0x72f270: the Kea force (lambda) of a row in the last step
MeReal MEAPI MdtConstraintGetRowForce(const MdtConstraint* c, int row)
{
    return c->bcl.lambda[row];
}

// Win 0x72f290: the force of coordinate i's limit / motor / lock row, 0 when none is active
// NOTE: like Win, no null check of the limit
MeReal MEAPI MdtConstraintGetLimitForce(MdtConstraint* c, int i)
{
    const MdtLimit* l = MdtConstraintGetLimit(c, i);
    if (l->limitsActive == 2 || l->control == kMdtLimitLocked || l->control == kMdtLimitMotorized)
        return c->bcl.lambda[l->mec];
    return 0;
}

// Win 0x72f2f0: Kea row status bits 1 / 2 (name inferred)
int MEAPI MdtConstraintIsRowAtLowerLimit(const MdtConstraint* c, int row)
{
    const int s = c->bcl.rowStatus[row];
    if (s & 2)
        return 1;
    return (s & 4) ? 1 : 0;
}

// Win 0x72f320: Kea row status bits 3-5 (name inferred)
int MEAPI MdtConstraintIsRowAtUpperLimit(const MdtConstraint* c, int row)
{
    const int s = c->bcl.rowStatus[row];
    if (s & 0x18)
        return 1;
    return (s & 0x28) ? 1 : 0;
}

// Win 0x72e680 (Win only): the constraint type's axis function (setAxis), e.g. the hinge axis
// for both bodies in world coordinates (name inferred)
void MEAPI MdtConstraintSetAxis(MdtConstraint* c, MeReal x, MeReal y, MeReal z)
{
    c->setAxis(c, x, y, z);
}
