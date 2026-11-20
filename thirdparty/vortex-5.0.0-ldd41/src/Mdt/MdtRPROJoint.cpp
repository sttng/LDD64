// Vortex 5.0.0 build 800 "vortexldd41" - RPRO joint (relative position, relative orientation)
// (restored). Mac 0x4920f9-0x49288b (static initialiser 0x492849 / 0x492874;
// the one before it, 0x4920b7 / 0x4920e2, is of a fully dead-stripped file), Win 0x774920-0x774f70.
#include "Mdt/MdtConstraint.h"
#include "Mdt/MdtBody.h"
#include "Mdt/MdtBcl.h"

#include <float.h>

// Quaternion product with the term order of both binaries (inlined three times in
// MdtRPROJointUpdateRelativeQuaternion).
static void MdtRPROQuaternionMultiply(MeReal* r, const MeReal* x, const MeReal* y)
{
    const MeReal r0 = ((x[0] * y[0] - x[1] * y[1]) - x[2] * y[2]) - x[3] * y[3];
    const MeReal r1 = ((x[1] * y[0] + x[0] * y[1]) + x[2] * y[3]) - x[3] * y[2];
    const MeReal r2 = ((x[2] * y[0] + x[0] * y[2]) + x[3] * y[1]) - x[1] * y[3];
    const MeReal r3 = ((x[3] * y[0] + x[0] * y[3]) + x[1] * y[2]) - x[2] * y[1];
    r[0] = r0;
    r[1] = r1;
    r[2] = r2;
    r[3] = r3;
}

// Mac 0x4920f9
void MEAPI MdtRPROJointGetQuaternion(const MdtConstraint* c, int which, MeVector4 q)
{
    const MdtRPROJoint* j = (const MdtRPROJoint*)c;
    const MeReal* s = (which == 0) ? j->q1 : j->q2;
    q[0] = s[0];
    q[1] = s[1];
    q[2] = s[2];
    q[3] = s[3];
}

// Mac 0x49213c
void MEAPI MdtRPROJointSetRelativeQuaternion(MdtConstraint* c, const MeVector4 q)
{
    MdtRPROJoint* j = (MdtRPROJoint*)c;
    j->relQ[0] = q[0];
    j->relQ[1] = q[1];
    j->relQ[2] = q[2];
    j->relQ[3] = q[3];
}

// Mac 0x49217a, Win 0x774cf0: force limits of the angular rows 3-5 (+-strength); a finite
// strength turns the row into a relaxed one
void MEAPI MdtRPROJointSetAngularStrength(MdtConstraint* c, MeReal x, MeReal y, MeReal z)
{
    const MeReal s[3] = { x, y, z };
    for (int i = 0; i != 3; ++i) {
        const MeReal v = s[i];
        c->bcl.forceLo[3 + i] = -v;
        c->bcl.forceHi[3 + i] = v;
        c->bcl.relaxActive[3 + i] = v < DBL_MAX;
    }
}

// Mac 0x492213, Win 0x774d90: the same for the linear rows 0-2
void MEAPI MdtRPROJointSetLinearStrength(MdtConstraint* c, MeReal x, MeReal y, MeReal z)
{
    const MeReal s[3] = { x, y, z };
    for (int i = 0; i != 3; ++i) {
        const MeReal v = s[i];
        c->bcl.forceLo[i] = -v;
        c->bcl.forceHi[i] = v;
        c->bcl.relaxActive[i] = v < DBL_MAX;
    }
}

// Mac 0x4922ac, Win 0x774920. NOTE: re-enables (instead of disables) an enabled constraint
// before the reset, unlike every other joint type; kept.
void MEAPI MdtRPROJointReset(MdtConstraint* c)
{
    if (MdtConstraintIsEnabled(c))
        MdtConstraintEnable(c);
    MdtRPROJoint* j = (MdtRPROJoint*)c;
    j->relQ[0] = 1.0;
    j->q2[0] = 1.0;
    j->q1[0] = 1.0;
    j->q2Set = 0;
    j->q1Set = 0;
    j->m_b80 = 0;
    j->m_b84 = 0;
    for (int i = 1; i != 4; ++i) {
        j->relQ[i] = 0;
        j->m_b68[i - 1] = 0;
        j->q2[i] = 0;
        j->q1[i] = 0;
        j->m_b30[i - 1] = 0;
    }
    MdtConstraintReset(c);
}

// Mac 0x492368, Win 0x774f70 (VxRPRO)
MdtConstraint* MEAPI MdtRPROJointCreate(VinPool* pool)
{
    MdtConstraint* c = MdtConstraintCreateFromPool(pool);
    if (c != 0) {
        c->bcl.type = kMdtTypeRPRO;
        c->rowBuilder = MdtBclAddRPROJoint;
        c->bcl.maxRows = 0xc;
        c->setBodies = MdtConstraintSetBodies2;
        c->setAxis = MdtConstraintSetAxisAll;
        c->numLimits = 0;
        MdtRPROJointReset(c);
    }
    return c;
}

// Mac 0x4923df
void MEAPI MdtRPROJointSetQuaternion(MdtConstraint* c, MeReal q0, MeReal q1, MeReal q2, MeReal q3,
                               int which)
{
    MdtRPROJoint* j = (MdtRPROJoint*)c;
    MeReal* q;
    if (which == 0) {
        q = j->q1;
        j->q1Set = 1;
    } else {
        q = j->q2;
        j->q2Set = 1;
    }
    q[0] = q0;
    q[1] = q1;
    q[2] = q2;
    q[3] = q3;
}

// Mac 0x492436: relQ = conj(q2) * conj(qBody1) * qBody0 * q1 (bodies missing = identity)
void MEAPI MdtRPROJointUpdateRelativeQuaternion(MdtConstraint* c)
{
    MeVector4 b0 = { 1.0, 0, 0, 0 };
    MeVector4 b1 = { 1.0, 0, 0, 0 };
    MeVector4 a, d;
    MdtRPROJointGetQuaternion(c, 0, a);
    MdtRPROJointGetQuaternion(c, 1, d);
    MdtBody* body = MdtConstraintGetBody(c, 0);
    if (body != 0)
        MdtBodyGetQuaternion(body, b0);
    body = MdtConstraintGetBody(c, 1);
    if (body != 0)
        MdtBodyGetQuaternion(body, b1);
    const MeVector4 e = { b1[0], -b1[1], -b1[2], -b1[3] };
    const MeVector4 f = { d[0], -d[1], -d[2], -d[3] };
    MeVector4 p, q, r;
    MdtRPROQuaternionMultiply(p, b0, a);
    MdtRPROQuaternionMultiply(q, e, p);
    MdtRPROQuaternionMultiply(r, f, q);
    MdtRPROJointSetRelativeQuaternion(c, r);
}
