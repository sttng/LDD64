// Vortex 5.0.0 build 800 "vortexldd41" - MdtBcl row and limit helpers (restored).
// Mac TU 554 (0x4a0e56-0x4a2e04, static initialiser 0x4a2dc2 / 0x4a2ded = the MdtConstraint.h
// one), Win 0x7921c0-0x7939ff (/LTCG: some helpers inlined, the friction-bound and the
// position-row helpers cdecl). File and function names inferred.
// Row helpers write at relative row `row` and return the rows written. Jacobian rows are [linear
// 0..2, angular 3..5] per body; the buffers are zeroed, so only non-zero entries are written.
#include "Mdt/MdtConstraint.h"
#include "Mdt/MdtBcl.h"
#include "Mdt/MdtBclRows.h"

#include <float.h>
#include <math.h>
#include "Me/MeCrtMath.h"

namespace {
const MeReal kPi = 3.141592653589793;
const MeReal k2Pi = 6.283185307179586;
const MeReal kLimitTolerance = 1e-14;

inline unsigned int SetKind(unsigned int f, unsigned int kind)
{
    return (f & ~0x6u) | kind;
}
inline unsigned int SetClass(unsigned int f, unsigned int cls)
{
    return (f & ~0x18u) | cls;
}

// Relaxation of a soft row: gamma = 1 / (h k d + 1) (0 for infinite damping),
// epsilon = h^2 k gamma (k = compliance). Inlined everywhere (MeReal h = params->invH).
inline void SoftRow(MdtBclRows* r, int row, MeReal h, MeReal k, MeReal d)
{
    MeReal g = 0.0;
    if (d != DBL_MAX)
        g = 1.0 / (h * k * d + 1.0);
    r->epsilon[row] = h * h * k * g;
    r->gamma[row] = g;
}

// Unwraps `angle` to within pi of `previous` (Mac 0x4a15a3 / 0x4a1693 / 0x4a178c).
// NOTE: (int) of a huge or NaN quotient is the integer indefinite value, as in both binaries.
// NOTE: on Win the angle is the CRT atan2's 64-bit-mantissa result still in st(0) (FPATAN
// ignores the precision control); the compare and the sums use it unrounded (Win 0x792d60 /
// 0x792c90 / 0x792b80). MeAtan2Long / MeLong* (Me/MeCrtMath.h) reproduce that; elsewhere they
// are plain double operations.
inline MeReal Unwrap(MeReal previous, MeAtan2Long angle, bool* changed)
{
    int n;
    *changed = true;
    if (MeLongGreater(angle, previous + kPi)) {
        n = (int)(MeRealSubLong(previous + kPi, angle) / k2Pi) - 1;
    } else if (previous > MeLongAdd(angle, kPi)) {
        n = (int)((previous - MeLongAdd(angle, kPi)) / k2Pi) + 1;
    } else {
        *changed = false;
        return MeLongToReal(angle);
    }
    return MeLongAdd(angle, (MeReal)n * k2Pi);
}

// Mac 0x644750 / 0x644758 / 0x644770: the other body and the axis pairs of an angular limit
// measured about axis `index` of body `body`'s frame.
const int kOtherBody[2] = { 1, 0 };
const int kAngleAxisX[2][3] = { { 1, 2, 0 }, { 2, 0, 1 } };
const int kAngleAxisY[2][3] = { { 2, 0, 1 }, { 1, 2, 0 } };
} // namespace

// Mac 0x4a0e56, Win 0x73e300 (thiscall on Win: VxContactMaterial accessor; axis 6 = axis 0)
int MEAPI MdtBclContactMaterialGetFrictionModel(const MdtBclContactMaterialData* m, int axis)
{
    if (axis == 6)
        return m->frictionModel[0];
    return m->frictionModel[axis];
}

// Mac 0x4a0e6f, Win 0x7921c0: row of a limit that is overshot (limitsActive == 2): stop
// stiffness / damping as relaxation, restitution as velocity, one-sided force bound.
void MEAPI MdtBclAddLimitBounceRow(MdtBclRows* r, const MdtBclSolverParameters* p, int row,
                                   MdtLimit* l)
{
    const MeReal d = l->m00;
    const bool below = 0.0 > d;
    const MdtSingleLimit* s = below ? &l->lower : &l->upper;
    if (d != 0.0 && s->invStiffness >= 0.0) {
        r->xi[row] = d;
        SoftRow(r, row, p->invH, s->invStiffness, s->damping);
        r->c[row] = s->m20;
        const MeReal v = (l->m50 - s->m20) * s->restitution;
        if (v * d >= 0.0)
            r->c[row] = r->c[row] - v;
        // NOTE: lockPosition is the lock's velocity (MdtBclAddLockedLimitRow)
        if (l->control == kMdtLimitLocked && (below ? l->lockPosition > 0.0 : 0.0 > l->lockPosition))
            r->c[row] = l->lockPosition;
        if (below) {
            r->lo[row] = 0.0;
            r->hi[row] = s->m28;
        } else {
            r->lo[row] = -s->m28;
            r->hi[row] = 0.0;
        }
    }
    r->flags[row] |= kMdtKeaRowRelaxed;
}

// Mac 0x4a101f, Win 0x7926a0: m00 = how far the coordinate is outside [lower, upper]; returns
// whether the coordinate moves further out (or is outside while not moving).
int MEAPI MdtBclLimitUpdateOvershoot(MdtLimit* l)
{
    l->m00 = 0.0;
    if (l->limitsActive != 0) {
        const MeReal pos = l->position + l->positionOffset;
        l->limitsActive = 1;
        if (l->lower.stop - kLimitTolerance > pos)
            l->m00 = pos - l->lower.stop;
        else if (pos > l->upper.stop + kLimitTolerance)
            l->m00 = pos - l->upper.stop;
    }
    const MeReal v = l->lockPosition;
    if (v == 0.0)
        return l->m00 != 0.0;
    return v * l->m00 > 0.0;
}

// Mac 0x4a10d9: relative velocity of the attachment points along `axis` (arms = the two
// attachment points relative to the bodies' centres of mass)
MeReal MEAPI MdtBclLinearVelocity(const MdtBclConstraint* c, const MeReal* axis, const MeReal* arms)
{
    MeReal v = 0.0;
    for (int i = 0; i != 2; ++i, arms += 3) {
        const MeReal s = i != 0 ? -1.0 : 1.0;
        const MeReal* vel = c->vel[i];
        if (vel != 0) {
            const MeReal* w = vel + 4;
            v = (axis[0] * vel[0] + axis[1] * vel[1] + axis[2] * vel[2]) * s + v
                + s * (axis[0] * (w[1] * arms[2] - w[2] * arms[1])
                       + axis[1] * (w[2] * arms[0] - arms[2] * w[0])
                       + axis[2] * (arms[1] * w[0] - w[1] * arms[0]));
        }
    }
    return v;
}

// Mac 0x4a11f4, Win 0x792990: relative angular velocity about `axis`
MeReal MEAPI MdtBclAngularVelocity(const MdtBclConstraint* c, const MeReal* axis)
{
    MeReal w0, w1, w2;
    const MeReal* a = c->vel[0];
    if (a == 0) {
        const MeReal* b = c->vel[1];
        w0 = -b[4];
        w1 = -b[5];
        w2 = -b[6];
    } else {
        const MeReal* b = c->vel[1];
        if (b == 0) {
            w0 = a[4];
            w1 = a[5];
            w2 = a[6];
        } else {
            w0 = a[4] - b[4];
            w1 = a[5] - b[5];
            w2 = a[6] - b[6];
        }
    }
    return w0 * axis[0] + w1 * axis[1] + w2 * axis[2];
}

// Mac 0x4a12cd, Win 0x792a80: whether the limit needs a row this step. A lock whose target was
// crossed since the last step becomes a motor (mf4); an overshot limit is marked 2.
int MEAPI MdtBclLimitIsActive(MdtLimit* l)
{
    if (l->control == kMdtLimitLocked && l->mf4 != 0) {
        const MeReal d = ((l->desiredVelocity - l->previousPosition) - l->positionOffset)
                         * ((l->desiredVelocity - l->position) - l->positionOffset);
        if (d <= 0.0)
            l->control = kMdtLimitMotorized;
    }
    if (l->limitsActive != 0) {
        if (l->m00 != 0.0) {
            l->limitsActive = 2;
            return 1;
        }
        l->limitsActive = 1;
    }
    return l->control != kMdtLimitFree;
}

// Mac 0x4a1368 (inlined on Win): angular row about `axis` (body 0 +axis, body 1 -axis)
int MEAPI MdtBclAddAngularAxisRow(MdtBclRows* r, MdtBclJacobianRow** jac, int row,
                                  const MeReal* axis, int unused)
{
    (void)unused;
    r->flags[row] = SetKind(r->flags[row], kMdtKeaRowAngular);
    r->flags[row] = SetClass(r->flags[row], kMdtKeaRowBilateral);
    MeReal* j0 = jac[0][row];
    j0[3] = axis[0];
    j0[4] = axis[1];
    j0[5] = axis[2];
    MeReal* j1 = jac[1][row];
    j1[3] = -axis[0];
    j1[4] = -axis[1];
    j1[5] = -axis[2];
    return 1;
}

// Mac 0x4a1423: linear limit coordinate (along axis `index` of body `me8`'s frame), its
// velocity, moving stops, the motor target, then the overshoot.
void MEAPI MdtBclLinearLimitUpdate(const MdtBclConstraint* c, const MdtBclSolverParameters* p,
                                   MdtLimit* l, MeReal (*frames)[4][4], const MeReal* arms,
                                   const MeReal* d)
{
    l->previousPosition = l->position;
    const MeReal* axis = frames[l->me8][l->index];
    l->position = axis[0] * d[0] + axis[1] * d[1] + axis[2] * d[2];
    l->m50 = MdtBclLinearVelocity(c, axis, arms);
    // fallthrough: shared with MdtBclAngularLimitUpdate (written out twice in both binaries)
    const MeReal h = p->stepSize;
    if (l->lower.m20 != 0.0 || l->upper.m20 != 0.0) {
        MeReal lo = l->lower.m20 * h + l->lower.stop;
        MeReal hi = h * l->upper.m20 + l->upper.stop;
        if (l->control == kMdtLimitMotorized) {
            const MeReal t = l->desiredVelocity;
            lo = t < lo ? t : lo;
            hi = t > hi ? t : hi;
        }
        if (hi > lo) {
            l->lower.stop = lo;
            l->upper.stop = hi;
        }
    }
    // NOTE: desiredVelocity is the motor's target position and m18 its velocity
    if (l->control == kMdtLimitMotorized && l->m18 != 0.0) {
        const MeReal t = l->m18 * h + l->desiredVelocity;
        l->desiredVelocity = t;
        if (l->limitsActive != 0) {
            if (l->lower.stop > t) {
                l->desiredVelocity = l->lower.stop + kLimitTolerance;
                l->m18 = 0.0;
            } else if (t > l->upper.stop) {
                l->desiredVelocity = l->upper.stop - kLimitTolerance;
                l->m18 = 0.0;
            }
        }
    }
    MdtBclLimitUpdateOvershoot(l);
}

// Mac 0x4a15a3: angle of `a` in the (x, y) plane, unwrapped near `previous`
MeReal MEAPI MdtBclUnwrapAngle(MeReal previous, const MeReal* a, const MeReal* x, const MeReal* y)
{
    const MeAtan2Long angle = MeAtan2L(a[0] * y[0] + a[1] * y[1] + a[2] * y[2],
                                       a[0] * x[0] + a[1] * x[1] + a[2] * x[2]);
    bool changed;
    return Unwrap(previous, angle, &changed);
}

// Mac 0x4a1693, Win 0x792c90: angular limit coordinate = angle of `a` in the (x, y) plane
void MEAPI MdtBclAngleFromAxes(const MdtBclConstraint* c, MdtLimit* l, const MeReal* a,
                               const MeReal* x, const MeReal* y)
{
    (void)c;
    const MeReal previous = l->position;
    l->previousPosition = previous;
    const MeAtan2Long angle = MeAtan2L(a[0] * y[0] + a[1] * y[1] + a[2] * y[2],
                                       a[0] * x[0] + a[1] * x[1] + a[2] * x[2]);
    l->position = MeLongToReal(angle);
    bool changed;
    const MeReal u = Unwrap(previous, angle, &changed);
    if (changed)
        l->position = u;
}

// Mac 0x4a178c, Win 0x792d60: angular limit coordinate about axis `index` of body `me8`
void MEAPI MdtBclAngleFromFrames(const MdtBclConstraint* c, MdtLimit* l, MeReal (*frames)[4][4])
{
    (void)c;
    const int body = l->me8;
    const int other = kOtherBody[body];
    const int ix = kAngleAxisX[body][l->index];
    const int iy = kAngleAxisY[body][l->index];
    const MeReal previous = l->position;
    l->previousPosition = previous;
    const MeReal* a = frames[other][ix];
    const MeReal* x = frames[body][ix];
    const MeReal* y = frames[body][iy];
    const MeAtan2Long angle = MeLongNeg(MeAtan2L(a[0] * y[0] + a[1] * y[1] + a[2] * y[2],
                                                 a[0] * x[0] + a[1] * x[1] + a[2] * x[2]));
    l->position = MeLongToReal(angle);
    bool changed;
    const MeReal u = Unwrap(previous, angle, &changed);
    if (changed)
        l->position = u;
}

// Mac 0x4a18d9, Win 0x792e10 (cdecl): bounds of a friction row (maxForce, scaled by mScale while
// the row was not at its bound in the last step), loss as relaxation
void MEAPI MdtBclFrictionRowBounds(const MdtBclConstraint* c, Vx::VxConstraintFriction* f,
                                   MdtBclRows* r, const MdtBclSolverParameters* p, int row)
{
    MeReal scale;
    if (f->mStatus < 1 || (c->rowStatus[f->mStatus] & 1) != 0)
        scale = 1.0;
    else
        scale = f->mScale;
    r->lo[row] = -f->mMaxForce * scale;
    r->hi[row] = f->mMaxForce * scale;
    r->xi[row] = 0.0;
    r->c[row] = 0.0;
    f->mStatus = row;
    r->flags[row] |= kMdtKeaRowRelaxed;
    r->flags[row] = SetClass(r->flags[row], kMdtKeaRowFriction);
    r->epsilon[row] = p->invH * f->mLoss;
    r->gamma[row] = 0.0;
}

// Mac 0x4a19a8, Win 0x7925e0: locked coordinate (lockPosition = lock velocity, bounded force)
void MEAPI MdtBclAddLockedLimitRow(MdtBclRows* r, const MdtBclSolverParameters* p, int row,
                                   const MdtLimit* l)
{
    r->lo[row] = l->lockForceLo;
    r->hi[row] = l->lockForceHi;
    r->xi[row] = 0.0;
    r->c[row] = l->lockPosition;
    if (l->limitsActive != 0) {
        const MeReal next = l->position + l->positionOffset + l->lockPosition * p->stepSize;
        if (l->lower.stop + kLimitTolerance > next)
            r->c[row] = 0.0;
        else if (next > l->upper.stop - kLimitTolerance)
            r->c[row] = 0.0;
    }
    r->epsilon[row] = p->invH * l->lockStiffness;
    r->gamma[row] = 0.0;
    r->flags[row] |= kMdtKeaRowRelaxed;
    r->flags[row] = SetClass(r->flags[row], kMdtKeaRowFriction);
}

// Mac 0x4a1abb, Win 0x792550: soft row with explicit error and bounds
void MEAPI MdtBclAddSoftRow(MdtBclRows* r, const MdtBclSolverParameters* p, int row,
                            MeReal invStiffness, MeReal damping, MeReal xi, MeReal lo, MeReal hi)
{
    r->lo[row] = lo;
    r->hi[row] = hi;
    r->xi[row] = xi;
    SoftRow(r, row, p->invH, invStiffness, damping);
    r->flags[row] |= kMdtKeaRowRelaxed;
}

// Mac 0x4a1b7b, Win 0x7924b0: motor row (target position desiredVelocity, velocity m18)
void MEAPI MdtBclAddMotorRow(MdtBclRows* r, const MdtBclSolverParameters* p, int row,
                             const MdtLimit* l)
{
    r->lo[row] = l->motorMaxForceLo;
    r->hi[row] = l->motorMaxForceHi;
    r->xi[row] = (l->position + l->positionOffset) - l->desiredVelocity;
    r->c[row] = l->m18;
    SoftRow(r, row, p->invH, l->motorInvStiffness, l->motorDamping);
    r->flags[row] |= kMdtKeaRowRelaxed;
}

// Mac 0x4a1c5b, Win 0x792b10: the row of an active limit (overshoot, motor or lock)
int MEAPI MdtBclAddLimitRow(MdtBclRows* r, const MdtBclSolverParameters* p, int row, MdtLimit* l)
{
    l->mec = row;
    if (l->limitsActive == 2)
        MdtBclAddLimitBounceRow(r, p, row, l);
    else if (l->control == kMdtLimitMotorized)
        MdtBclAddMotorRow(r, p, row, l);
    else if (l->control == kMdtLimitLocked)
        MdtBclAddLockedLimitRow(r, p, row, l);
    return 0;
}

// Mac 0x4a1ce1, Win 0x792ea0: angular row keeping a perpendicular to b (error a . b)
int MEAPI MdtBclAddAngularRow(MdtBclRows* r, MdtBclJacobianRow** jac, int row, const MeReal* a,
                              const MeReal* b, int setError)
{
    MeReal* j0 = jac[0][row];
    j0[3] = a[1] * b[2] - a[2] * b[1];
    j0[4] = a[2] * b[0] - a[0] * b[2];
    j0[5] = a[0] * b[1] - a[1] * b[0];
    MeReal* j1 = jac[1][row];
    j1[3] = b[1] * a[2] - b[2] * a[1];
    j1[4] = b[2] * a[0] - b[0] * a[2];
    j1[5] = b[0] * a[1] - b[1] * a[0];
    if (setError != 0) {
        r->xi[row] = a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
        r->lo[row] = -DBL_MAX;
        r->hi[row] = DBL_MAX;
    }
    r->flags[row] = SetKind(r->flags[row], kMdtKeaRowAngular);
    r->flags[row] = SetClass(r->flags[row], kMdtKeaRowBilateral);
    return 1;
}

// Mac 0x4a1e41, Win 0x792f90: linear row along `axis` (error d . axis, d = attachment point 0 -
// attachment point 1); the lever arm of body `body` is taken at the other body's point.
int MEAPI MdtBclAddLinearRow(MdtBclRows* r, MdtBclJacobianRow** jac, int row, const MeReal* d,
                             const MeReal* axis, const MeReal* arms, int body, int setError)
{
    MeReal r0x, r0y, r0z, r1x, r1y, r1z;
    if (body == 0) {
        r0x = arms[0] - d[0];
        r0y = arms[1] - d[1];
        r0z = arms[2] - d[2];
        r1x = arms[3];
        r1y = arms[4];
        r1z = arms[5];
    } else {
        r1x = arms[3] + d[0];
        r1y = arms[4] + d[1];
        r1z = arms[5] + d[2];
        r0x = arms[0];
        r0y = arms[1];
        r0z = arms[2];
    }
    MeReal* j0 = jac[0][row];
    j0[0] = axis[0];
    j0[1] = axis[1];
    j0[2] = axis[2];
    MeReal* j1 = jac[1][row];
    j1[0] = -axis[0];
    j1[1] = -axis[1];
    j1[2] = -axis[2];
    j0[3] = r0y * axis[2] - r0z * axis[1];
    j0[4] = r0z * axis[0] - r0x * axis[2];
    j0[5] = r0x * axis[1] - r0y * axis[0];
    j1[3] = r1z * axis[1] - r1y * axis[2];
    j1[4] = r1x * axis[2] - r1z * axis[0];
    j1[5] = r1y * axis[0] - r1x * axis[1];
    if (setError != 0) {
        r->xi[row] = d[0] * axis[0] + d[1] * axis[1] + d[2] * axis[2];
        r->lo[row] = -DBL_MAX;
        r->hi[row] = DBL_MAX;
    }
    r->flags[row] = SetKind(r->flags[row], kMdtKeaRowLinear);
    r->flags[row] = SetClass(r->flags[row], kMdtKeaRowBilateral);
    return 1;
}

// Mac 0x4a2082, Win 0x7936f0: three rows joining the attachment points; body < 0: world axes
// (Jacobian written directly), else the axes of body `body`'s attachment frame
int MEAPI MdtBclAddPositionRows(MdtBclRows* r, MdtBclJacobianRow** jac, int row,
                                MeReal (*frames)[4][4], const MeReal* arms, int body,
                                int setError)
{
    if (body < 0) {
        const MeReal ax = arms[0], ay = arms[1], az = arms[2];
        const MeReal bx = arms[3], by = arms[4], bz = arms[5];
        MdtBclJacobianRow* j0 = jac[0] + row;
        j0[0][3] = 0.0;
        j0[0][4] = az;
        j0[0][5] = -ay;
        j0[1][3] = -az;
        j0[1][4] = 0.0;
        j0[1][5] = ax;
        j0[2][3] = ay;
        j0[2][4] = -ax;
        j0[2][5] = 0.0;
        MdtBclJacobianRow* j1 = jac[1] + row;
        j1[0][3] = 0.0;
        j1[0][4] = -bz;
        j1[0][5] = by;
        j1[1][3] = bz;
        j1[1][4] = 0.0;
        j1[1][5] = -bx;
        j1[2][3] = -by;
        j1[2][4] = bx;
        j1[2][5] = 0.0;
        for (int i = 0; i != 3; ++i) {
            jac[0][row + i][i] = 1.0;
            jac[1][row + i][i] = -1.0;
        }
        if (setError != 0) {
            r->xi[row] = frames[0][3][0] - frames[1][3][0];
            r->xi[row + 1] = frames[0][3][1] - frames[1][3][1];
            r->xi[row + 2] = frames[0][3][2] - frames[1][3][2];
            r->hi[row] = DBL_MAX;
            r->hi[row + 1] = DBL_MAX;
            r->hi[row + 2] = DBL_MAX;
            r->lo[row] = -DBL_MAX;
            r->lo[row + 1] = -DBL_MAX;
            r->lo[row + 2] = -DBL_MAX;
        }
        for (int i = 0; i != 3; ++i) {
            r->flags[row + i] = SetKind(r->flags[row + i], kMdtKeaRowLinear);
            r->flags[row + i] = SetClass(r->flags[row + i], kMdtKeaRowBilateral);
        }
    } else {
        MeReal d[3];
        d[0] = frames[0][3][0] - frames[1][3][0];
        d[1] = frames[0][3][1] - frames[1][3][1];
        d[2] = frames[0][3][2] - frames[1][3][2];
        MdtBclAddLinearRow(r, jac, row, d, frames[body][0], arms, body, setError);
        MdtBclAddLinearRow(r, jac, row + 1, d, frames[body][1], arms, body, setError);
        MdtBclAddLinearRow(r, jac, row + 2, d, frames[body][2], arms, body, setError);
    }
    return 3;
}

// Mac 0x4a23d4, Win 0x793540: friction rows of the enabled coordinate frictions (linear 0-2
// along, angular 3-5 about the axes of the friction's body frame); returns the rows added
int MEAPI MdtBclAddFrictionRows(MdtBclConstraint* c, MdtBclRows* r, MdtBclJacobianRow** jac,
                                int row, MeReal (*frames)[4][4], const MeReal* arms,
                                const MdtBclSolverParameters* p)
{
    MeReal d[3];
    d[0] = frames[0][3][0] - frames[1][3][0];
    d[1] = frames[0][3][1] - frames[1][3][1];
    d[2] = frames[0][3][2] - frames[1][3][2];
    int n = 0;
    for (int i = 0; i != 3; ++i) {
        Vx::VxConstraintFriction* f = &c->friction[i];
        // NOTE: tests the flag VxConstraintFriction::setProportional writes (Mac +5)
        if (!f->mProportional || f->mMaxForce <= 0.0) {
            f->mStatus = 0;
        } else {
            MdtBclAddLinearRow(r, jac, row, d, frames[f->mAxis][i], arms, f->mAxis, 1);
            r->flags[row] = SetKind(r->flags[row], kMdtKeaRowLinear);
            MdtBclFrictionRowBounds(c, f, r, p, row);
            ++n;
            ++row;
        }
    }
    for (int i = 0; i != 3; ++i) {
        Vx::VxConstraintFriction* f = &c->friction[3 + i];
        if (!f->mProportional || f->mMaxForce <= 0.0) {
            f->mStatus = 0;
        } else {
            MdtBclAddAngularAxisRow(r, jac, row, frames[f->mAxis][i], 0);
            r->flags[row] = SetKind(r->flags[row], kMdtKeaRowAngular);
            MdtBclFrictionRowBounds(c, f, r, p, row);
            ++n;
            ++row;
        }
    }
    return n;
}

// Mac 0x4a25ab, Win 0x792740: angular limit velocity, moving stops, motor target, overshoot
// (the position comes from MdtBclAngleFromAxes / MdtBclAngleFromFrames)
void MEAPI MdtBclAngularLimitUpdate(const MdtBclConstraint* c, const MdtBclSolverParameters* p,
                                    MdtLimit* l, MeReal (*frames)[4][4])
{
    l->m50 = MdtBclAngularVelocity(c, frames[l->me8][l->index]);
    const MeReal h = p->stepSize;
    if (l->lower.m20 != 0.0 || l->upper.m20 != 0.0) {
        MeReal lo = l->lower.m20 * h + l->lower.stop;
        MeReal hi = h * l->upper.m20 + l->upper.stop;
        if (l->control == kMdtLimitMotorized) {
            const MeReal t = l->desiredVelocity;
            lo = t < lo ? t : lo;
            hi = t > hi ? t : hi;
        }
        if (hi > lo) {
            l->lower.stop = lo;
            l->upper.stop = hi;
        }
    }
    if (l->control == kMdtLimitMotorized && l->m18 != 0.0) {
        const MeReal t = l->m18 * h + l->desiredVelocity;
        l->desiredVelocity = t;
        if (l->limitsActive != 0) {
            if (l->lower.stop > t) {
                l->desiredVelocity = l->lower.stop + kLimitTolerance;
                l->m18 = 0.0;
            } else if (t > l->upper.stop) {
                l->desiredVelocity = l->upper.stop - kLimitTolerance;
                l->m18 = 0.0;
            }
        }
    }
    MdtBclLimitUpdateOvershoot(l);
}

// Mac 0x4a26f6, Win 0x7932a0: row of the angle atan2(d . y, d . x) of the separation d in the
// (x, y) plane (cone / swing limits); the lever arm of body `body` is taken at the other
// body's point
int MEAPI MdtBclAddAngleRow(MdtBclRows* r, MdtBclJacobianRow** jac, int row, int body,
                            const MeReal* x, const MeReal* y, const MeReal* arms,
                            const MeReal* d, int setError)
{
    (void)setError;
    const MeReal d0 = d[0], d1 = d[1], d2 = d[2];
    MeReal sy = d0 * y[0] + y[1] * d1 + y[2] * d2;
    MeReal sx = d0 * x[0] + d1 * x[1] + d2 * x[2];
    const MeReal inv = 1.0 / (sy * sy + sx * sx);
    sy = sy * inv;
    sx = sx * inv;
    MeReal rx, ry, rz;
    if (body == 0) {
        rx = arms[0] - d0;
        ry = arms[1] - d1;
        rz = arms[2] - d2;
    } else {
        rx = arms[0];
        ry = arms[1];
        rz = arms[2];
    }
    MeReal cx[3], cy[3];
    cx[0] = x[2] * ry - x[1] * rz;
    cx[1] = x[0] * rz - x[2] * rx;
    cx[2] = x[1] * rx - x[0] * ry;
    cy[0] = y[2] * ry - y[1] * rz;
    cy[1] = y[0] * rz - y[2] * rx;
    cy[2] = y[1] * rx - y[0] * ry;
    MeReal* j0 = jac[0][row];
    for (int k = 0; k != 3; ++k) {
        j0[k] = sy * x[k] - sx * y[k];
        j0[3 + k] = sy * cx[k] - sx * cy[k];
    }
    if (body == 1) {
        rx = arms[3] - d0;
        ry = arms[4] - d1;
        rz = arms[5] - d2;
    } else {
        rx = arms[3];
        ry = arms[4];
        rz = arms[5];
    }
    cx[0] = x[2] * ry - x[1] * rz;
    cx[1] = x[0] * rz - x[2] * rx;
    cx[2] = x[1] * rx - x[0] * ry;
    cy[0] = y[2] * ry - y[1] * rz;
    cy[1] = y[0] * rz - y[2] * rx;
    cy[2] = y[1] * rx - y[0] * ry;
    const MeReal nsy = -sy;
    MeReal* j1 = jac[1][row];
    for (int k = 0; k != 3; ++k) {
        j1[k] = nsy * x[k] + sx * y[k];
        j1[3 + k] = nsy * cx[k] + sx * cy[k];
    }
    r->flags[row] = SetKind(r->flags[row], kMdtKeaRowAngular);
    r->flags[row] = SetClass(r->flags[row], kMdtKeaRowBilateral);
    return 1;
}

// Mac 0x4a2b56, Win 0x7930f0: row keeping the attachment points at distance `length`
int MEAPI MdtBclAddDistanceRow(MdtBclRows* r, MdtBclJacobianRow** jac, int row, const MeReal* d,
                               const MeReal* arms, MeReal length, int setError)
{
    MeReal nx = d[0], ny = d[1], nz = d[2];
    const MeReal l2 = nx * nx + ny * ny + nz * nz;
    if (l2 > 0.0) {
        const MeReal inv = 1.0 / sqrt(l2);
        nx = nx * inv;
        ny = ny * inv;
        nz = nz * inv;
    } else {
        nz = 0.0;
        ny = 0.0;
        nx = 1.0;
    }
    const MeReal ax = arms[0], ay = arms[1], az = arms[2];
    const MeReal bx = arms[3], by = arms[4], bz = arms[5];
    MeReal* j0 = jac[0][row];
    j0[0] = nx;
    j0[1] = ny;
    j0[2] = nz;
    MeReal* j1 = jac[1][row];
    j1[0] = -nx;
    j1[1] = -ny;
    j1[2] = -nz;
    j0[3] = nz * ay - ny * az;
    j0[4] = az * nx - nz * ax;
    j0[5] = ax * ny - ay * nx;
    j1[3] = ny * bz - nz * by;
    j1[4] = nz * bx - bz * nx;
    j1[5] = by * nx - bx * ny;
    if (setError != 0) {
        r->xi[row] = sqrt(0.0 + d[0] * d[0] + d[1] * d[1] + d[2] * d[2]) - length;
        r->lo[row] = -DBL_MAX;
        r->hi[row] = DBL_MAX;
    }
    r->flags[row] = SetKind(r->flags[row], kMdtKeaRowLinear);
    r->flags[row] = SetClass(r->flags[row], kMdtKeaRowBilateral);
    return 1;
}
