// Vortex 5.0.0 build 800 "vortexldd41" - MdtBody (restored).
// Mac 0x48adcd-0x48cd47 (static initialiser 0x48cd05 / 0x48cd30 at the end), Win
// 0x72ffb0-0x7321e0 (/LTCG; Win also has accessors Mac dead-stripped, not restored here).
#include "Mdt/MdtBody.h"
#include "Mdt/MdtConstraint.h"
#include "Mdt/MdtWorld.h"
#include "Me/MeMath.h"
#include "Me/MeMessage.h"

#include <string.h>
#include <float.h>

// Mac data 0x7794e0 (0x24 bytes), Win 0xabc2f0 (0x28): defaults of every new body.
// (Win-only setters 0x730000 / getter 0x730020 exist; Mac keeps only the getter.)
static MdtAutoDisableParams MdtBodyDefaultAutoDisable = { 0.02, 0.001, 0.002, 0.002, 10 };

// Win 0x72fc00 (file-static, register arguments); inlined on Mac (0x48c7ea in MdtBodyReset).
// Rotation rows of tm from the unit quaternion q = (w, x, y, z).
static void MdtQuaternionToTM(MeMatrix4 tm, const MeReal* q)
{
    const MeReal w = q[0], x = q[1], y = q[2], z = q[3];
    const MeReal ww = w * w;
    const MeReal xx = x * x;
    const MeReal yy = y * y;
    const MeReal zz = z * z;
    const MeReal w2 = w + w;
    const MeReal x2 = x + x;
    const MeReal xw2 = x * w2;
    const MeReal yw2 = y * w2;
    const MeReal zw2 = w2 * z;
    const MeReal yx2 = y * x2;
    const MeReal zx2 = x2 * z;
    const MeReal y2 = y + y;
    const MeReal zy2 = z * y2;
    tm[0][0] = ((xx + ww) - yy) - zz;
    tm[1][0] = yx2 - zw2;
    tm[2][0] = yw2 + zx2;
    tm[0][1] = yx2 + zw2;
    const MeReal wx = ww - xx;
    tm[1][1] = (yy + wx) - zz;
    tm[2][1] = zy2 - xw2;
    tm[0][2] = zx2 - yw2;
    tm[1][2] = xw2 + zy2;
    tm[2][2] = (wx - yy) + zz;
}

// Mac 0x48adcd, Win 0x730020
void MEAPI MdtBodyGetDefaultAutoDisableParams(MdtAutoDisableParams* p)
{
    *p = MdtBodyDefaultAutoDisable;
}

// Mac 0x48ae1b, Win 0x730060: the transform is the first member
MeReal* MEAPI MdtBodyGetTransformPtr(MdtBody* b)
{
    return b->transform[0];
}

// Mac 0x48ae23, Win 0x730090
void* MEAPI MdtBodyGetUserData(const MdtBody* b)
{
    return b->userData;
}

// Mac 0x48ae31, Win 0x730110
void MEAPI MdtBodyGetQuaternion(const MdtBody* b, MeVector4 q)
{
    q[0] = b->keaBody.qrot[0];
    q[1] = b->keaBody.qrot[1];
    q[2] = b->keaBody.qrot[2];
    q[3] = b->keaBody.qrot[3];
}

// Mac 0x48ae6f, Win 0x730190
void MEAPI MdtBodyGetLinearVelocity(const MdtBody* b, MeVector3 v)
{
    v[0] = b->keaBody.vel[0];
    v[1] = b->keaBody.vel[1];
    v[2] = b->keaBody.vel[2];
}

// Mac 0x48aea0, Win 0x7301c0
MeReal* MEAPI MdtBodyGetLinearVelocityPtr(MdtBody* b)
{
    return b->keaBody.vel;
}

// Mac 0x48aead, Win 0x730200
MeReal* MEAPI MdtBodyGetAngularVelocityPtr(MdtBody* b)
{
    return b->keaBody.velrot;
}

// Mac 0x48aeba, Win 0x730450
MeReal MEAPI MdtBodyGetLinearVelocityDamping(const MdtBody* b)
{
    return b->keaBody.linearDamping;
}

// Mac 0x48aec8, Win 0x730460
MeReal MEAPI MdtBodyGetAngularVelocityDamping(const MdtBody* b)
{
    return b->keaBody.angularDamping;
}

// Mac 0x48aed6, Win 0x730560
int MEAPI MdtBodyGetEnabledTime(const MdtBody* b)
{
    return b->enabledTime;
}

// Mac 0x48aee4, Win 0x730570: only for enabled bodies
void MEAPI MdtBodySetEnabledTime(MdtBody* b, int t)
{
    if (b->enabledTime != -1)
        b->enabledTime = t;
}

// Mac 0x48aefe, Win 0x730600
void MEAPI MdtBodySetUserData(MdtBody* b, void* data)
{
    b->userData = data;
}

// Mac 0x48af0f, Win 0x730890
void MEAPI MdtBodySetLinearVelocity(MdtBody* b, MeReal x, MeReal y, MeReal z)
{
    b->keaBody.vel[0] = x;
    b->keaBody.vel[1] = y;
    b->keaBody.vel[2] = z;
}

// Mac 0x48af3e, Win 0x7308c0
void MEAPI MdtBodySetAngularVelocity(MdtBody* b, MeReal x, MeReal y, MeReal z)
{
    b->keaBody.velrot[0] = x;
    b->keaBody.velrot[1] = y;
    b->keaBody.velrot[2] = z;
}

// Mac 0x48af6d, Win 0x7308f0
void MEAPI MdtBodySetLinearVelocityDamping(MdtBody* b, MeReal d)
{
    b->keaBody.linearDamping = d;
}

// Mac 0x48af82, Win 0x730910
void MEAPI MdtBodySetAngularVelocityDamping(MdtBody* b, MeReal d)
{
    b->keaBody.angularDamping = d;
}

// Mac 0x48af97, Win 0x730a50. NOTE: the inertia tensor is not rescaled (the Win-only 0x731e10
// is the rescaling variant).
void MEAPI MdtBodySetMass(MdtBody* b, MeReal mass)
{
    b->mass = mass;
    b->keaBody.invMass = 1.0 / mass;
    b->keaBody.Iworld[0][3] = 1.0 / mass;
    b->keaBody.invIworld[0][3] = mass;
}

// Mac 0x48afd6, Win 0x730dc0
void MEAPI MdtBodySetMaxTimeStep(MdtBody* b, MeReal h)
{
    b->maxTimeStep = h;
}

// Mac 0x48afeb, Win 0x730de0
int MEAPI MdtBodyIsEnabled(const MdtBody* b)
{
    return b->enabledTime != -1;
}

// Mac 0x48b000, Win 0x731030
void MEAPI MdtBodyAddForce(MdtBody* b, MeReal x, MeReal y, MeReal z)
{
    b->keaBody.force[0] += x;
    b->keaBody.force[1] += y;
    b->keaBody.force[2] += z;
}

// Mac 0x48b047, Win 0x7311a0
void MEAPI MdtBodyAddTorque(MdtBody* b, MeReal x, MeReal y, MeReal z)
{
    b->keaBody.torque[0] += x;
    b->keaBody.torque[1] += y;
    b->keaBody.torque[2] += z;
}

// Mac 0x48b08e, Win 0x7311e0 (VxPart::addForce)
void MEAPI MdtBodyAddImpulse(MdtBody* b, MeReal x, MeReal y, MeReal z)
{
    b->impulse[0] += x;
    b->impulse[1] += y;
    b->impulse[2] += z;
    b->impulseAdded = 1;
}

// Mac 0x48b0df, Win 0x731450
int MEAPI MdtBodyIsFrozen(const MdtBody* b)
{
    return b->frozen & 1;
}

// Mac 0x48b0f0, Win 0x731490: neither frozen nor animated
int MEAPI MdtBodyIsDynamic(const MdtBody* b)
{
    return b->frozen == 0;
}

// Mac 0x48b105, Win 0x731bc0
void MEAPI MdtBodyResetForces(MdtBody* b)
{
    for (int i = 0; i != 4; ++i) {
        b->keaBody.force[i] = 0;
        b->keaBody.constraintForce[i] = 0;
        b->keaBody.torque[i] = 0;
        b->keaBody.constraintTorque[i] = 0;
        b->keaBody.m2a4[i] = 0;
        b->keaBody.m2c4[i] = 0;
        b->keaBody.vel[i] = 0;
        b->keaBody.velrot[i] = 0;
        b->keaBody.accel[i] = 0;
        b->keaBody.accelrot[i] = 0;
        b->impulse[i] = 0;
        b->angularImpulse[i] = 0;
    }
    b->lastForce[0] = 0;
    b->lastTorque[0] = 0;
    b->lastForce[1] = 0;
    b->lastTorque[1] = 0;
    b->lastForce[2] = 0;
    b->lastTorque[2] = 0;
    if (!MdtBodyIsDynamic(b))
        b->enabledTime = -1;
    else if (b->enabledTime != -1)
        b->enabledTime = 0;
}

// Mac 0x48b1e6, Win 0x731680: put a disabled dynamic body back on the world's enabled list
// and tell the world's enable callback (MdtWorld +0x2cc / Win +0x2e0)
void MEAPI MdtBodyEnable(MdtBody* b)
{
    if (b->frozen == 0) {
        if (b->enabledTime == -1 && b->world != 0) {
            MdtWorld* w = b->world;
            b->nextEnabled = w->enabledBodies;
            b->prevNextEnabled = &w->enabledBodies;
            if (w->enabledBodies != 0)
                w->enabledBodies->prevNextEnabled = &b->nextEnabled;
            b->world->enabledBodies = b;
            b->world->numEnabledBodies++;
            if (b->world->bodyEnabledCallback != 0)
                b->world->bodyEnabledCallback(b);
        }
        b->enabledTime = 0;
    }
}

// Mac 0x48b26a, Win 0x731700: MdtBodyEnable without the callback
void MEAPI MdtBodyEnableNoCallback(MdtBody* b)
{
    if (b->frozen == 0) {
        if (b->enabledTime == -1 && b->world != 0) {
            MdtWorld* w = b->world;
            b->nextEnabled = w->enabledBodies;
            b->prevNextEnabled = &w->enabledBodies;
            if (w->enabledBodies != 0)
                w->enabledBodies->prevNextEnabled = &b->nextEnabled;
            b->world->enabledBodies = b;
            b->world->numEnabledBodies++;
        }
        b->enabledTime = 0;
    }
}

// Mac 0x48b2d1, Win 0x731770: enable a dynamic body; for a frozen / animated one enable the
// dynamic bodies it is constrained to instead
void MEAPI MdtBodyWakeUp(MdtBody* b)
{
    if (MdtBodyIsDynamic(b)) {
        MdtBodyEnable(b);
        return;
    }
    for (MdtConstraintNode* n = b->constraints; n != 0; n = n->next) {
        MdtConstraint* c = n->constraint;
        for (int i = 0; i < c->bcl.numBodies; ++i) {
            MdtBody* o = c->body[i];
            if (o != 0 && o != b && MdtBodyIsDynamic(o))
                MdtBodyEnable(o);
        }
    }
}

// Mac 0x48b35a, Win 0x731a60 (meaning unknown)
void MEAPI MdtBodySetM548(MdtBody* b, MdtBodyContactFn fn, void* data)
{
    b->m548 = fn;
    b->m54c = data;
}

// Mac 0x48b374, Win 0x731aa0
void MEAPI MdtBodySetGrouping(MdtBody* b, int grouping)
{
    b->keaBody.grouping = grouping;
}

// Mac 0x48b385
int MEAPI MdtBodyGetGrouping(const MdtBody* b)
{
    return b->keaBody.grouping;
}

// Mac 0x48b393, Win 0x731ad0
MeReal MEAPI MdtBodyGetAutoDisableVelocityThreshold(const MdtBody* b)
{
    return b->autoDisable.velocityThreshold;
}

// Mac 0x48b3a1, Win 0x731ae0
MeReal MEAPI MdtBodyGetAutoDisableAngularVelocityThreshold(const MdtBody* b)
{
    return b->autoDisable.angularVelocityThreshold;
}

// Mac 0x48b3af, Win 0x731af0
MeReal MEAPI MdtBodyGetAutoDisableAccelerationThreshold(const MdtBody* b)
{
    return b->autoDisable.accelerationThreshold;
}

// Mac 0x48b3bd, Win 0x731b00
MeReal MEAPI MdtBodyGetAutoDisableAngularAccelerationThreshold(const MdtBody* b)
{
    return b->autoDisable.angularAccelerationThreshold;
}

// Mac 0x48b3cb, Win 0x731b10
int MEAPI MdtBodyGetAutoDisableAliveWindow(const MdtBody* b)
{
    return b->autoDisable.aliveWindow;
}

// Mac 0x48b3d9, Win 0x7315a0: take an enabled body off the world's enabled list (no callback)
void MEAPI MdtBodyDisableNoCallback(MdtBody* b)
{
    if (b->enabledTime != -1 && b->world != 0) {
        if (b->nextEnabled != 0)
            b->nextEnabled->prevNextEnabled = b->prevNextEnabled;
        *b->prevNextEnabled = b->nextEnabled;
        b->nextEnabled = 0;
        b->prevNextEnabled = 0;
        b->world->numEnabledBodies--;
        MdtBodyResetPartitionInfo(&b->partitionInfo);
        b->m514 = 0;
        for (MdtConstraintNode* n = b->constraints; n != 0; n = n->next) {
            MdtConstraint* c = n->constraint;
            MdtConstraintResetPartitionInfo(&c->partitionInfo);
            c->visited = 0;
        }
    }
    b->enabledTime = -1;
    b->partition = -1;
}

// Mac 0x48b49f, Win 0x7314b0: as above plus the world's disable callback (+0x2d0 / Win +0x2e4)
void MEAPI MdtBodyDisable(MdtBody* b)
{
    if (b->enabledTime != -1 && b->world != 0) {
        if (b->nextEnabled != 0)
            b->nextEnabled->prevNextEnabled = b->prevNextEnabled;
        *b->prevNextEnabled = b->nextEnabled;
        b->nextEnabled = 0;
        b->prevNextEnabled = 0;
        b->world->numEnabledBodies--;
        if (b->world->bodyDisabledCallback != 0)
            b->world->bodyDisabledCallback(b);
        MdtBodyResetPartitionInfo(&b->partitionInfo);
        b->m514 = 0;
        for (MdtConstraintNode* n = b->constraints; n != 0; n = n->next) {
            MdtConstraint* c = n->constraint;
            MdtConstraintResetPartitionInfo(&c->partitionInfo);
            c->visited = 0;
        }
    }
    b->partition = -1;
    b->enabledTime = -1;
}

// Mac 0x48b57a, Win 0x7313e0: re-attach the body in every constraint (refreshes the Kea body,
// velocity, transform and quaternion pointers after a state change)
void MEAPI MdtBodyUpdateConstraints(MdtBody* b)
{
    for (MdtConstraintNode* n = b->constraints; n != 0; n = n->next) {
        MdtConstraint* c = n->constraint;
        for (int i = 0; i < c->bcl.numBodies; ++i) {
            if (c->body[i] == b)
                MdtConstraintSetBody(c, b, i);
        }
    }
}

// Mac 0x48b5da, Win 0x731f60: clear every freeze / animation state
void MEAPI MdtBodySetDynamic(MdtBody* b)
{
    if (b->frozen != 0) {
        b->frozen = 0;
        MdtBodyEnable(b);
        b->keaBody.flags &= ~kMdtKeaBodyFlagDisabled;
        MdtBodyUpdateConstraints(b);
    }
}

// Mac 0x48b614, Win 0x731f10: kinematic body (moved by the user, not by the solver)
void MEAPI MdtBodySetAnimated(MdtBody* b)
{
    if (MdtBodyIsDynamic(b))
        MdtBodyEnable(b);
    if ((b->frozen & 2) == 0) {
        MdtBodyDisableNoCallback(b);
        b->frozen |= 2;
        b->keaBody.flags |= kMdtKeaBodyFlagDisabled;
        MdtBodyUpdateConstraints(b);
    }
}

// Mac 0x48b65f, Win 0x731ed0
void MEAPI MdtBodyUnfreeze(MdtBody* b)
{
    if ((b->frozen & 1) != 0) {
        b->frozen &= ~1;
        if (b->frozen == 0) {
            MdtBodyEnableNoCallback(b);
            MdtBodyResetForces(b);
        }
        b->keaBody.flags &= ~kMdtKeaBodyFlagFrozen;
        MdtBodyUpdateConstraints(b);
    }
}

// Mac 0x48b6a5, Win 0x731e90
void MEAPI MdtBodyFreeze(MdtBody* b)
{
    if ((b->frozen & 1) == 0) {
        MdtBodyDisableNoCallback(b);
        b->keaBody.flags |= kMdtKeaBodyFlagFrozen;
        b->frozen |= 1;
        MdtBodyUpdateConstraints(b);
    }
}

// Mac 0x48b6dc, Win 0x730b10: inertia = i * identity
void MEAPI MdtBodySetSphericalInertia(MdtBody* b, MeReal i)
{
    // NOTE: the identity is scaled element-wise, so the off-diagonal terms are i * 0.0
    // (-0 for a negative i, NaN for an infinite one), as in both binaries.
    static const MeReal identity[3][3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
    MeMatrix3 I;
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            I[r][c] = identity[r][c] * i;
    MeMatrix3InvertSymmetric(I, b->keaBody.invI[0], b->keaBody.invI[1], b->keaBody.invI[2]);
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c != 3; ++c) {
            b->keaBody.I[r][c] = I[r][c];
            b->keaBody.Iworld[r][c] = I[r][c];
            b->keaBody.invIworld[r][c] = b->keaBody.invI[r][c];
        }
    }
}

// Mac 0x48b7af, Win 0x730a80
void MEAPI MdtBodySetInertiaTensor(MdtBody* b, const MeMatrix3 I)
{
    MeMatrix3InvertSymmetric(I, b->keaBody.invIworld[0], b->keaBody.invIworld[1],
                             b->keaBody.invIworld[2]);
    for (int r = 0; r != 3; ++r) {
        for (int c = 0; c != 3; ++c) {
            b->keaBody.Iworld[r][c] = I[r][c];
            b->keaBody.invI[r][c] = b->keaBody.invIworld[r][c];
            b->keaBody.I[r][c] = I[r][c];
        }
    }
}

// Mac 0x48b83c, Win 0x730620
void MEAPI MdtBodySetTransform(MdtBody* b, const MeMatrix4 tm)
{
    memcpy(b->transform, tm, sizeof(MeMatrix4));
    MdtBodyUpdateCOMFromTransform(b);
}

// Mac 0x48b86b, Win 0x72ffb0
void MEAPI MdtBodyFreeToPool(MdtBody* b, VinPool* pool)
{
    b->keaBody.index = -1;
    VinPoolPutStruct(pool, b);
}

// Mac 0x48b88f, Win 0x7302d0
void MEAPI MdtBodyGetAngularAcceleration(const MdtBody* b, MeVector3 a)
{
    a[0] = b->keaBody.accelrot[0];
    a[1] = b->keaBody.accelrot[1];
    a[2] = b->keaBody.accelrot[2];
}

// Mac 0x48b8c0, Win 0x7302a0
void MEAPI MdtBodyGetLinearAcceleration(const MdtBody* b, MeVector3 a)
{
    a[0] = b->keaBody.accel[0];
    a[1] = b->keaBody.accel[1];
    a[2] = b->keaBody.accel[2];
}

// Mac 0x48b8f1, Win 0x7301d0
void MEAPI MdtBodyGetAngularVelocity(const MdtBody* b, MeVector3 w)
{
    w[0] = b->keaBody.velrot[0];
    w[1] = b->keaBody.velrot[1];
    w[2] = b->keaBody.velrot[2];
}

// Mac 0x48b922, Win 0x730eb0
void MEAPI MdtBodyResetVelocities(MdtBody* b)
{
    if (b != 0) {
        b->keaBody.vel[0] = 0;
        b->keaBody.vel[1] = 0;
        b->keaBody.vel[2] = 0;
        b->keaBody.velrot[0] = 0;
        b->keaBody.velrot[1] = 0;
        b->keaBody.velrot[2] = 0;
        b->keaBody.accel[0] = 0;
        b->keaBody.accel[1] = 0;
        b->keaBody.accel[2] = 0;
        b->keaBody.accelrot[0] = 0;
        b->keaBody.accelrot[1] = 0;
        b->keaBody.accelrot[2] = 0;
    }
}

// Mac 0x48b992, Win 0x730e00: remember the force / torque of the step and clear them
void MEAPI MdtBodySaveAndClearForces(MdtBody* b)
{
    if (b != 0) {
        b->lastForce[0] = b->keaBody.force[0];
        b->lastForce[1] = b->keaBody.force[1];
        b->lastForce[2] = b->keaBody.force[2];
        b->keaBody.force[0] = 0;
        b->keaBody.force[1] = 0;
        b->keaBody.force[2] = 0;
        b->lastTorque[0] = b->keaBody.torque[0];
        b->lastTorque[1] = b->keaBody.torque[1];
        b->lastTorque[2] = b->keaBody.torque[2];
        b->keaBody.torque[0] = 0;
        b->keaBody.torque[1] = 0;
        b->keaBody.torque[2] = 0;
        b->keaBody.constraintForce[0] = 0;
        b->keaBody.constraintForce[1] = 0;
        b->keaBody.constraintForce[2] = 0;
        b->keaBody.constraintTorque[0] = 0;
        b->keaBody.constraintTorque[1] = 0;
        b->keaBody.constraintTorque[2] = 0;
    }
}

static MeReal MdtVector3MagnitudeSqr(const MeReal* v)
{
    MeReal s = 0.0;
    s = v[0] * v[0] + s;
    s = v[1] * v[1] + s;
    s = v[2] * v[2] + s;
    return s;
}

// Mac 0x48ba66, Win 0x7312e0: still above an auto-disable threshold or inside the alive window.
// NOTE: Mac tests "threshold < |x|^2" (NaN = at rest), Win "!(|x|^2 <= threshold)" (NaN =
// moving); the Mac form is kept on both.
int MEAPI MdtBodyIsMoving(const MdtBody* b)
{
    if (MdtVector3MagnitudeSqr(b->keaBody.vel) > b->autoDisable.velocityThreshold ||
        MdtVector3MagnitudeSqr(b->keaBody.velrot) > b->autoDisable.angularVelocityThreshold ||
        MdtVector3MagnitudeSqr(b->keaBody.accel) > b->autoDisable.accelerationThreshold ||
        MdtVector3MagnitudeSqr(b->keaBody.accelrot) > b->autoDisable.angularAccelerationThreshold)
        return 1;
    return b->enabledTime < b->autoDisable.aliveWindow;
}

// Mac 0x48bb7b, Win 0x731c70: set the transform and give the body the velocities that move it
// there in h
void MEAPI MdtBodyMoveTo(MdtBody* b, const MeMatrix4 tm, MeReal h)
{
    if (MdtBodyIsFrozen(b)) {
        MeWarning(0xc, "MdtBodyMoveTo: Can't move a frozen body. Unfreeze the body first.\n");
        return;
    }
    MeReal invH;
    if (!(h <= 0.0)) {
        invH = 1.0 / h;
    } else {
        MeWarning(0xc, "MdtBodyMoveTo: time step must be larger than 0.\n");
        invH = 0.0;
    }
    const MeReal oldX = b->keaBody.comTM[3][0];
    const MeReal oldY = b->keaBody.comTM[3][1];
    const MeReal oldZ = b->keaBody.comTM[3][2];
    MeVector4 oldQ;
    oldQ[0] = b->keaBody.qrot[0];
    oldQ[1] = b->keaBody.qrot[1];
    oldQ[2] = b->keaBody.qrot[2];
    oldQ[3] = b->keaBody.qrot[3];
    MdtBodySetTransform(b, tm);
    const MeReal dx = b->keaBody.comTM[3][0] - oldX;
    const MeReal dy = b->keaBody.comTM[3][1] - oldY;
    const MeReal dz = b->keaBody.comTM[3][2] - oldZ;
    MeVector3 w;
    MeQuaternionDifferenceToAngularVelocity(w, oldQ, b->keaBody.qrot);
    const MeReal vx = invH * dx;
    const MeReal vy = invH * dy;
    const MeReal vz = invH * dz;
    MdtBodySetLinearVelocity(b, vx, vy, vz);
    w[0] = invH * w[0];
    w[1] = invH * w[1];
    w[2] = invH * w[2];
    MdtBodySetAngularVelocity(b, w[0], w[1], w[2]);
    MeReal v[3] = { vx, vy, vz };
    if (MdtVector3MagnitudeSqr(v) > 0.0 || MdtVector3MagnitudeSqr(w) > 0.0)
        MdtBodyWakeUp(b);
}

// Mac 0x48bdf9, Win 0x730210: v + w x (p - com)
void MEAPI MdtBodyGetVelocityAtPoint(const MdtBody* b, const MeVector3 p, MeVector3 v)
{
    v[2] = 0.0;
    v[1] = 0.0;
    v[0] = 0.0;
    if (b != 0) {
        const MeReal rx = p[0] - b->keaBody.comTM[3][0];
        const MeReal ry = p[1] - b->keaBody.comTM[3][1];
        const MeReal rz = p[2] - b->keaBody.comTM[3][2];
        const MeReal wx = b->keaBody.velrot[0];
        const MeReal wy = b->keaBody.velrot[1];
        const MeReal wz = b->keaBody.velrot[2];
        v[0] = (rz * wy - ry * wz) + b->keaBody.vel[0];
        v[1] = (rx * wz - rz * wx) + b->keaBody.vel[1];
        v[2] = (ry * wx - rx * wy) + b->keaBody.vel[2];
    }
}

// Mac 0x48bebd, Win 0x731230
void MEAPI MdtBodyAddImpulseAtPosition(MdtBody* b, MeReal ix, MeReal iy, MeReal iz,
                                 MeReal px, MeReal py, MeReal pz)
{
    const MeReal rx = px - b->keaBody.comTM[3][0];
    const MeReal ry = py - b->keaBody.comTM[3][1];
    const MeReal rz = pz - b->keaBody.comTM[3][2];
    b->impulse[0] += ix;
    b->impulse[1] += iy;
    b->impulse[2] += iz;
    b->angularImpulse[0] = (iz * ry - iy * rz) + b->angularImpulse[0];
    b->angularImpulse[1] = (ix * rz - iz * rx) + b->angularImpulse[1];
    b->angularImpulse[2] = (iy * rx - ix * ry) + b->angularImpulse[2];
    b->impulseAdded = 1;
}

// Mac 0x48bfa5, Win 0x731070
void MEAPI MdtBodyAddForceAtPosition(MdtBody* b, MeReal fx, MeReal fy, MeReal fz,
                               MeReal px, MeReal py, MeReal pz)
{
    const MeReal rx = px - b->keaBody.comTM[3][0];
    const MeReal ry = py - b->keaBody.comTM[3][1];
    const MeReal rz = pz - b->keaBody.comTM[3][2];
    b->keaBody.force[0] += fx;
    b->keaBody.force[1] += fy;
    b->keaBody.force[2] += fz;
    b->keaBody.torque[0] = (fz * ry - fy * rz) + b->keaBody.torque[0];
    b->keaBody.torque[1] = (fx * rz - fz * rx) + b->keaBody.torque[1];
    b->keaBody.torque[2] = (fy * rx - fx * ry) + b->keaBody.torque[2];
}

// Mac 0x48c083, Win 0x730300: a + w x (w x r) + alpha x r, r = p - com
void MEAPI MdtBodyGetAccelerationAtPoint(const MdtBody* b, const MeVector3 p, MeVector3 a)
{
    a[2] = 0.0;
    a[1] = 0.0;
    a[0] = 0.0;
    if (b != 0) {
        const MeReal rx = p[0] - b->keaBody.comTM[3][0];
        const MeReal ry = p[1] - b->keaBody.comTM[3][1];
        const MeReal rz = p[2] - b->keaBody.comTM[3][2];
        const MeReal wy = b->keaBody.velrot[1];
        const MeReal wz = b->keaBody.velrot[2];
        const MeReal cx = wy * rz - ry * wz;
        const MeReal wx = b->keaBody.velrot[0];
        const MeReal cy = rx * wz - rz * wx;
        const MeReal cz = ry * wx - wy * rx;
        const MeReal ay = b->keaBody.accelrot[1];
        const MeReal az = b->keaBody.accelrot[2];
        const MeReal ax = b->keaBody.accelrot[0];
        a[0] = (wy * cz - cy * wz) + b->keaBody.accel[0];
        const MeReal a1 = (wz * cx - wx * cz) + b->keaBody.accel[1];
        a[1] = a1;
        const MeReal a2 = (wx * cy - wy * cx) + b->keaBody.accel[2];
        a[2] = a2;
        a[0] = (ay * rz - az * ry) + a[0];
        a[1] = (az * rx - rz * ax) + a1;
        a[2] = (ry * ax - rx * ay) + a2;
    }
}

// Mac 0x48c20f, Win 0x731fa0
void MEAPI MdtBodyReset(MdtBody* b)
{
    if (MdtBodyIsEnabled(b))
        MdtBodyDisable(b);
    b->keaBody.invMass = 1.0;
    b->keaBody.flags = 0;
    b->keaBody.linearDamping = 0;
    b->keaBody.angularDamping = 0;
    b->frozen = 0;
    memset(b->keaBody.I, 0, sizeof(b->keaBody.I));
    memset(b->keaBody.invI, 0, sizeof(b->keaBody.invI));
    memset(b->keaBody.Iworld, 0, sizeof(b->keaBody.Iworld) + sizeof(b->keaBody.invIworld));
    MdtBodyResetForces(b);
    b->impulseAdded = 0;
    b->keaBody.qrot[0] = 1.0;
    b->keaBody.qrot[3] = 0;
    b->keaBody.qrot[2] = 0;
    b->keaBody.qrot[1] = 0;
    b->keaBody.I[0][3] = 1.0;
    b->keaBody.invI[0][3] = 1.0;
    b->keaBody.fastSpin[0] = 0;
    b->keaBody.fastSpin[1] = 1.0;
    b->keaBody.fastSpin[2] = 0;
    b->keaBody.fastSpin[3] = 0;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            b->keaBody.comTM[r][c] = (r == c) ? 1.0 : 0.0;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            b->transform[r][c] = (r == c) ? 1.0 : 0.0;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            b->relativeTransform[r][c] = (r == c) ? 1.0 : 0.0;
    MdtBodySetSphericalInertia(b, 0.4);
    b->useRelativeTransform = 0;
    b->partition = -1;
    b->mass = 1.0;
    b->autoDisable = MdtBodyDefaultAutoDisable;
    b->m514 = 0;
    b->userData = 0;
    b->name = 0;
    b->maxTimeStep = DBL_MAX;
    MdtQuaternionToTM(b->keaBody.comTM, b->keaBody.qrot);
    MdtBodyResetPartitionInfo(&b->partitionInfo);
    b->self = b;
}

// Mac 0x48c91f, Win 0x7321e0
MdtBody* MEAPI MdtBodyCreateFromPool(VinPool* pool)
{
    MdtBody* b = (MdtBody*)VinPoolGetStruct(pool);
    if (b != 0) {
        b->world = 0;
        b->keaBody.index = 100;
        b->keaBody.partitionIndex = 0;
        b->keaBody.grouping = -1;
        b->keaBody.size = sizeof(MdtKeaBody);
        b->enabledTime = -1;
        b->frozen = 0;
        MdtBodyReset(b);
        b->nextEnabled = 0;
        b->prevNextEnabled = 0;
        b->constraints = 0;
        b->m548 = 0;
        b->m54c = 0;
        b->next = 0;
        b->prevNext = 0;
    }
    return b;
}

// Mac 0x48c9d7, Win 0x730bc0: change the body frame relative to the centre of mass. The
// constraint attachment positions of the body are moved so the joints stay where they were.
void MEAPI MdtBodySetCenterOfMassRelativeTransform(MdtBody* b, const MeMatrix4 tm)
{
    MeMatrix4 m;
    for (int r = 0; r != 4; ++r) {
        for (int c = 0; c != 4; ++c) {
            m[r][c] = tm[r][0] * b->relativeTransform[0][c] + tm[r][1] * b->relativeTransform[1][c] +
                      tm[r][2] * b->relativeTransform[2][c] + tm[r][3] * b->relativeTransform[3][c];
        }
    }
    if (MeMatrix4IsIdentity(m, 1e-14))
        return;
    if (b->constraints != 0) {
        MeMatrix4TMInvert(m);
        for (MdtConstraintNode* n = b->constraints; n != 0; n = n->next) {
            MdtConstraint* c = n->constraint;
            if (c->visited == 0) {
                c->visited = 1;
                for (int i = 0; i < c->bcl.numBodies; ++i) {
                    if (c->body[i] == b) {
                        MeVector3 p;
                        MdtConstraintGetBodyPositionRel(c, i, p);
                        MdtConstraintSetBodyPositionRel(
                            c, i, p[0] * m[0][0] + p[1] * m[1][0] + p[2] * m[2][0] + m[3][0],
                            p[0] * m[0][1] + p[1] * m[1][1] + p[2] * m[2][1] + m[3][1],
                            p[0] * m[0][2] + p[1] * m[1][2] + p[2] * m[2][2] + m[3][2]);
                    }
                }
            }
        }
        for (MdtConstraintNode* n = b->constraints; n != 0; n = n->next)
            n->constraint->visited = 0;
    }
    memcpy(b->relativeTransform, tm, sizeof(MeMatrix4));
    MeMatrix4TMInvert(b->relativeTransform);
    b->useRelativeTransform = 1;
    MdtBodyUpdateCOMFromTransform(b);
}
