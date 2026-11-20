// Vortex 5.0.0 build 800 "vortexldd41" - Vinds: the body side of a Kea step (restored).
// Mac 0x4a3d0d-0x4a4eaa and 0x4aaa0b-0x4ab11a (TU 556), Win 0x80ee30-0x810950.
// ("Vinds" from the message "Vinds:: body_update_quaternion"; the other names are inferred.)
// MdtKeaBody fields as used here (Mac offsets):
//   I (+0x64) / invI (+0xc4)              world inertia, rebuilt every step; I[0][3] = mass,
//                                         invI[0][3] = inverse mass
//   Iworld (+0x124) / invIworld (+0x184)  body frame inertia (the user's tensor) / its inverse
//   m2a4 / m2c4                           linear / angular momentum (VindsComputeMomenta)
// NOTE: Kea/MdtKea.h (V2.1) describes I as body frame and Iworld as world frame: the reverse of
// their use here.
// Every expression keeps the Mac SSE2 operation order; Win x87 computes the same trees at 53 bits.
#include "Kea/KeaMath.h"
#include "Kea/MdtKeaConstraints.h"
#include "Kea/LCPMatrix.h"
#include "Kea/MemoryPool.h"
#include "Me/MeMath.h"
#include "Me/MeMessage.h"

#include <math.h>
#include <string.h>

// Mac 0x4a3d0d, Win 0x80ee30
int VindsSolveLcp(LCPMatrix* A, MdtKeaConstraints* c, MdtKeaParameters* p, KeaLcpFunction lcp,
                  MemoryPool* pool)
{
    int steps = lcp(A, c->lambda, c->rhs, c->lo, c->hi, c->lcpState, &p->lcp, pool, 0);
    MeReal* previous = 0;
    if (c->numFrictionRows != 0) {
        MeReal ratio = p->refineTolerance + 1.0;
        previous = (MeReal*)pool->alloc(A->mSize * 8, 0x10, "z");
        KeaCopy(A->mSize, c->lambda, 1, previous, 1);
        int pass = 0;
        if (p->refineIterations >= 1 && !(ratio <= p->refineTolerance)) {
            do {
                p->lcp.warmStart = 1;
                KeaUpdateFrictionBounds(A->mSize, c->lambda, c->lo, c->hi, c->frictionNormal,
                                        c->frictionCoefficient, c->cohesion, c->lcpState);
                steps += lcp(A, c->lambda, c->rhs, c->lo, c->hi, c->lcpState, &p->lcp, pool, 0);
                const MeReal before = KeaNorm2(A->mSize, previous, 1);
                KeaAxpy(A->mSize, -1.0, c->lambda, 1, previous, 1);
                const MeReal change = KeaNorm2(A->mSize, previous, 1);
                ratio = change * (1.0 / before);
                KeaCopy(A->mSize, c->lambda, 1, previous, 1);
                ++pass;
            } while (pass < p->refineIterations && ratio > p->refineTolerance);
        }
        c->refinePasses = pass;
        c->refineRatio = ratio;
    }
    for (int i = 0; i < A->mSize; ++i) {
        const MeReal x = c->lambda[i];
        if (x != x || fabs(x) == HUGE_VAL)
            c->lambda[i] = 0.0;
    }
    if (previous != 0)
        pool->free(previous);
    c->lcpSteps += steps;
    return steps;
}

// Mac 0x4a400b, Win 0x80fae0
void VindsUpdatePositions(MdtKeaBody* const* bodies, int n, const MeReal* h)
{
    const MeReal dt = *h;
    for (int i = 0; i < n; ++i) {
        MdtKeaBody* b = bodies[i];
        b->comTM[3][0] = dt * b->vel[0] + b->comTM[3][0];
        b->comTM[3][1] = dt * b->vel[1] + b->comTM[3][1];
        b->comTM[3][2] = dt * b->vel[2] + b->comTM[3][2];
        b->comTM[3][3] = 1.0;
    }
}

// Win 0x80f9d0 (inlined on Mac): rotation matrix of a unit quaternion (w, x, y, z), rows of tm
static void VindsQuaternionToMatrix(MeReal (*tm)[4], const MeReal* q)
{
    const MeReal w = q[0], x = q[1], y = q[2], z = q[3];
    const MeReal xx = x * x;
    const MeReal yy = y * y;
    const MeReal zz = z * z;
    const MeReal w2 = w + w;
    const MeReal yx2 = y * (x + x);
    const MeReal x2z = (x + x) * z;
    const MeReal zy2 = z * (y + y);
    const MeReal yw2 = y * w2;
    const MeReal zw2 = w2 * z;
    tm[0][0] = ((xx + w * w) - yy) - zz;
    tm[1][0] = yx2 - zw2;
    tm[2][0] = yw2 + x2z;
    tm[0][1] = yx2 + zw2;
    const MeReal wwxx = w * w - xx;
    tm[1][1] = (yy + wwxx) - zz;
    tm[2][1] = zy2 - x * w2;
    tm[0][2] = x2z - yw2;
    tm[1][2] = x * w2 + zy2;
    tm[2][2] = (wwxx - yy) + zz;
}

// Mac 0x4a409c, Win 0x810340 (-> 0x80f9d0 per body)
void VindsUpdateOrientationMatrices(MdtKeaBody* const* bodies, int n, const MeReal*)
{
    for (int i = 0; i < n; ++i)
        VindsQuaternionToMatrix(bodies[i]->comTM, bodies[i]->qrot);
}

// Mac 0x4a41e9, Win 0x810410: torque += scale * (L x w), L = angular momentum
void VindsAddGyroscopicTorques(MdtKeaBody* const* bodies, int n, MeReal scale)
{
    for (int i = 0; i < n; ++i) {
        MdtKeaBody* b = bodies[i];
        const MeReal* L = b->m2c4;
        const MeReal* w = b->velrot;
        b->torque[0] = (L[2] * w[1] - L[1] * w[2]) * scale + b->torque[0];
        b->torque[1] = (L[0] * w[2] - L[2] * w[0]) * scale + b->torque[1];
        b->torque[2] = b->torque[2] + (L[1] * w[0] - L[0] * w[1]) * scale;
    }
}

// Mac 0x4a42a8, Win 0x810670: momenta from the velocities
void VindsComputeMomenta(MdtKeaBody* const* bodies, int n)
{
    for (int i = 0; i < n; ++i) {
        MdtKeaBody* b = bodies[i];
        const MeReal mass = b->I[0][3];
        for (int k = 0; k < 3; ++k) {
            b->m2a4[k] = b->vel[k] * mass;
            b->m2c4[k] = (b->I[k][0] * b->velrot[0] + b->I[k][1] * b->velrot[1]) + b->I[k][2] * b->velrot[2];
        }
    }
}

// Win 0x810750 (inlined on Mac): I = R^T B R and invI = R Binv R^T, R = comTM rotation rows
static void VindsRotateInertia(MdtKeaBody* b)
{
    for (int e = 0; e < 3; ++e) {
        for (int k = 0; k < 3; ++k) {
            b->I[e][k] = 0;
            b->invI[e][k] = 0;
            for (int f = 0; f < 3; ++f) {
                for (int g = 0; g < 3; ++g) {
                    b->I[e][k] = b->comTM[f][e] * b->Iworld[f][g] * b->comTM[g][k] + b->I[e][k];
                    b->invI[e][k] = b->comTM[e][f] * b->invIworld[f][g] * b->comTM[k][g] + b->invI[e][k];
                }
            }
        }
    }
}

// Mac 0x4a4333, Win 0x810870
void VindsComputeWorldInertia(MdtKeaBody* const* bodies, int n, bool spherical)
{
    for (int i = 0; i < n; ++i) {
        MdtKeaBody* b = bodies[i];
        MeReal scratch[2][12];      // NOTE: cleared and never used (as in the original)
        memset(b->I, 0, sizeof(b->I));
        memset(b->invI, 0, sizeof(b->invI));
        memset(scratch[0], 0, sizeof(scratch[0]));
        memset(scratch[1], 0, sizeof(scratch[1]));
        b->I[0][3] = 1.0 / b->invMass;
        b->invI[0][3] = b->invMass;
        if (!spherical && (b->flags & kMdtKeaBodyFlagGyroscopic) == 0) {
            VindsRotateInertia(b);
        } else {
            const MeReal d = ((b->Iworld[0][0] + b->Iworld[1][1]) + b->Iworld[2][2]) / 3.0;
            b->I[2][2] = d;
            b->I[1][1] = d;
            b->I[0][0] = d;
            const MeReal inv = 1.0 / d;
            b->invI[2][2] = inv;
            b->invI[1][1] = inv;
            b->invI[0][0] = inv;
        }
    }
}

// Mac 0x4a4663, Win 0x80fc90
void VindsUpdateVelocities(MdtKeaBody* const* bodies, int n, const MeReal* h, const MeReal* invM)
{
    const MeReal dt = *h;
    for (int i = 0; i < n; ++i, invM += 12) {
        MdtKeaBody* b = bodies[i];
        const MeReal m = invM[3];
        b->vel[0] = m * b->m2a4[0] + dt * b->accel[0];
        b->vel[1] = m * b->m2a4[1] + dt * b->accel[1];
        b->vel[2] = m * b->m2a4[2] + dt * b->accel[2];
        const MeReal* L = b->m2c4;
        const MeReal w0 = (invM[0] * L[0] + invM[1] * L[1]) + invM[2] * L[2];
        b->velrot[0] = w0;
        const MeReal w1 = (invM[4] * L[0] + invM[5] * L[1]) + invM[6] * L[2];
        b->velrot[1] = w1;
        b->velrot[0] = w0 + dt * b->accelrot[0];
        b->velrot[1] = w1 + dt * b->accelrot[1];
        b->velrot[2] = ((invM[8] * L[0] + invM[9] * L[1]) + invM[10] * L[2]) + dt * b->accelrot[2];
    }
}

// q / |q| with |q|^2 summed from 0 in index order
static void VindsNormalizeQuaternion(MeReal* q)
{
    const MeReal s = 1.0 / sqrt((((0.0 + q[0] * q[0]) + q[1] * q[1]) + q[2] * q[2]) + q[3] * q[3]);
    q[0] = q[0] * s;
    q[1] = q[1] * s;
    q[2] = q[2] * s;
    q[3] = q[3] * s;
}

// Mac 0x4a47e9, Win 0x80ff00
void VindsUpdateQuaternions(MdtKeaBody* const* bodies, int n, const MeReal* h, int method)
{
    const MeReal dt = *h;
    for (int i = 0; i < n; ++i) {
        MdtKeaBody* b = bodies[i];
        MeReal w[3] = { b->velrot[0], b->velrot[1], b->velrot[2] };
        if ((b->flags & kMdtKeaBodyFlagUseFastSpin) != 0) {
            // the fast spin axis in world coordinates: integrated exactly, removed from w
            MeReal a[3];
            a[0] = (b->comTM[0][0] * b->fastSpin[0] + b->comTM[1][0] * b->fastSpin[1]) + b->comTM[2][0] * b->fastSpin[2];
            a[1] = (b->comTM[0][1] * b->fastSpin[0] + b->comTM[1][1] * b->fastSpin[1]) + b->comTM[2][1] * b->fastSpin[2];
            a[2] = (b->comTM[0][2] * b->fastSpin[0] + b->comTM[1][2] * b->fastSpin[1]) + b->comTM[2][2] * b->fastSpin[2];
            const MeReal len2 = (a[0] * a[0] + a[1] * a[1]) + a[2] * a[2];
            if (len2 > 0.0) {
                const MeReal s = 1.0 / sqrt(len2);
                a[0] = a[0] * s;
                a[1] = a[1] * s;
                a[2] = a[2] * s;
            } else {
                a[0] = 1.0;
                a[1] = 0.0;
                a[2] = 0.0;
            }
            const MeReal spin = (w[0] * a[0] + w[1] * a[1]) + w[2] * a[2];
            MeQuaternionFiniteRotation(b->qrot, a, spin * dt);
            const MeReal ns = -spin;
            w[0] = ns * a[0] + w[0];
            w[1] = ns * a[1] + w[1];
            w[2] = ns * a[2] + w[2];
        }
        if (method == 2) {
            MeQuaternionFiniteRotation(b->qrot, w, dt);
        } else if (method == 3) {
            // implicit midpoint (Cayley) step
            const MeReal q0 = b->qrot[0], q1 = b->qrot[1], q2 = b->qrot[2], q3 = b->qrot[3];
            const MeReal old[4] = { q0, q1, q2, q3 };
            MeReal dq[4];
            memset(dq, 0, sizeof(dq));
            dq[0] = ((-q1 * w[0]) - q2 * w[1]) - q3 * w[2];
            dq[1] = (q0 * w[0] + w[1] * q3) - q2 * w[2];
            dq[2] = (q0 * w[1] - q3 * w[0]) + q1 * w[2];
            dq[3] = (w[0] * q2 - q1 * w[1]) + q0 * w[2];
            const MeReal half = dt * sqrt(((0.0 + w[0] * w[0]) + w[1] * w[1]) + w[2] * w[2]) * 0.25;
            const MeReal c = 1.0 / (half * half + 1.0);
            const MeReal a = (c + c) - 1.0;
            const MeReal s = dt * (c * 0.5);
            for (int k = 0; k < 4; ++k)
                b->qrot[k] = a * old[k] + s * dq[k];
            VindsNormalizeQuaternion(b->qrot);
        } else if (method == 1) {
            const MeReal q0 = b->qrot[0], q1 = b->qrot[1], q2 = b->qrot[2], q3 = b->qrot[3];
            MeReal dq[4];
            memset(dq, 0, sizeof(dq));
            dq[0] = (((-q1 * w[0]) - q2 * w[1]) - q3 * w[2]) * 0.5;
            dq[1] = ((w[0] * q0 + w[1] * q3) - q2 * w[2]) * 0.5;
            dq[2] = ((w[1] * q0 - q3 * w[0]) + q1 * w[2]) * 0.5;
            dq[3] = ((w[0] * q2 - q1 * w[1]) + w[2] * q0) * 0.5;
            for (int k = 0; k < 4; ++k)
                b->qrot[k] = dt * dq[k] + b->qrot[k];
            VindsNormalizeQuaternion(b->qrot);
        } else {
            MeFatalError(0xc, "\n ++ Vinds:: body_update_quaternion:: No such method : %d \n++", method);
        }
    }
}

// Mac 0x4aaa0b, Win 0x80f0d0
MeReal* VindsComputeInverseMass(const MeReal* h, MdtKeaBody* const* bodies, int n, int spherical,
                                MemoryPool* pool)
{
    MeReal* invM = (MeReal*)pool->allocZeroed(n * 0x60, 0x10, "inv M");
    MeReal* m = invM;
    for (int i = 0; i < n; ++i, m += 12) {
        const MdtKeaBody* b = bodies[i];
        m[3] = 1.0 / (1.0 / b->invMass + *h * b->linearDamping);
        if (spherical == 0 && (b->flags & kMdtKeaBodyFlagGyroscopic) == 0) {
            const MeReal d = *h * b->angularDamping;
            MeReal M[3][3];
            M[0][0] = b->I[0][0] + d;
            M[0][1] = b->I[0][1];
            M[0][2] = b->I[0][2];
            M[1][0] = b->I[1][0];
            M[1][1] = b->I[1][1] + d;
            M[1][2] = b->I[1][2];
            M[2][0] = b->I[2][0];
            M[2][1] = b->I[2][1];
            M[2][2] = b->I[2][2] + d;
            MeMatrix3Invert(M);
            for (int r = 0; r < 3; ++r) {
                m[r * 4 + 0] = M[r][0];
                m[r * 4 + 1] = M[r][1];
                m[r * 4 + 2] = M[r][2];
            }
        } else {
            const MeReal d = 1.0 / (((b->Iworld[0][0] + b->Iworld[1][1]) + b->Iworld[2][2]) / 3.0
                                    + *h * b->angularDamping);
            m[10] = d;
            m[5] = d;
            m[0] = d;
        }
    }
    return invM;
}

// Mac 0x4aabc8, Win 0x80f500
void VindsComputeAccelerations(MdtKeaBody* const* bodies, int n, const MeReal* invM)
{
    for (int i = 0; i < n; ++i, invM += 12) {
        MdtKeaBody* b = bodies[i];
        const MeReal m = invM[3];
        const MeReal a0 = m * b->force[0];
        b->accel[0] = a0;
        const MeReal a1 = m * b->force[1];
        b->accel[1] = a1;
        const MeReal a2 = m * b->force[2];
        b->accel[2] = a2;
        const MeReal m2 = invM[3];
        b->accel[0] = a0 + m2 * b->constraintForce[0];
        b->accel[1] = a1 + m2 * b->constraintForce[1];
        b->accel[2] = a2 + m2 * b->constraintForce[2];
        for (int k = 0; k < 3; ++k) {
            const MeReal* r = invM + k * 4;
            b->accelrot[k] = ((r[0] * b->torque[0] + r[1] * b->torque[1]) + r[2] * b->torque[2])
                             + ((r[0] * b->constraintTorque[0] + r[1] * b->constraintTorque[1])
                                + r[2] * b->constraintTorque[2]);
        }
    }
}

// Mac 0x4aacea, Win 0x80f660 (+ 0x80f3c0): like VindsComputeInverseMass for the stable
// rotational model: M = Iw - scale h^2 S invIw S + h damping, S = [L]x of the angular momentum
MeReal* VindsComputeInverseMassStable(MdtKeaBody* const* bodies, int n, const MeReal* h, MeReal scale,
                                      MemoryPool* pool)
{
    MeReal* invM = (MeReal*)pool->allocZeroed(n * 0x60, 0x10, "inv M");
    const MeReal dt = *h;
    const MeReal c = -(scale * dt * dt);
    MeReal* m = invM;
    for (int i = 0; i < n; ++i, m += 12) {
        const MdtKeaBody* b = bodies[i];
        const MeReal d = *h * b->angularDamping;
        m[3] = 1.0 / (1.0 / b->invMass + *h * b->linearDamping);
        MeReal M[3][3];
        for (int r = 0; r < 3; ++r) {
            M[r][0] = b->I[r][0];
            M[r][1] = b->I[r][1];
            M[r][2] = b->I[r][2];
        }
        const MeReal* L = b->m2c4;
        MeReal S[3][3];
        S[0][0] = 0;
        S[0][1] = -L[2];
        S[0][2] = L[1];
        S[1][0] = L[2];
        S[1][1] = 0;
        S[1][2] = -L[0];
        S[2][0] = -L[1];
        S[2][1] = L[0];
        S[2][2] = 0;
        MeReal F[3][3];   // invIw S
        int r, k, j;
        for (r = 0; r < 3; ++r) {
            for (k = 0; k < 3; ++k) {
                MeReal s = 0.0;
                for (j = 0; j < 3; ++j)
                    s = s + b->invI[r][j] * S[j][k];
                F[r][k] = s;
            }
        }
        for (r = 0; r < 3; ++r) {
            for (k = 0; k < 3; ++k) {
                MeReal s = 0.0;
                for (j = 0; j < 3; ++j)
                    s = s + S[r][j] * F[j][k];
                M[r][k] = c * s + M[r][k];
            }
        }
        M[0][0] = M[0][0] + d;
        M[1][1] = M[1][1] + d;
        M[2][2] = M[2][2] + d;
        MeMatrix3Invert(M);
        for (r = 0; r < 3; ++r) {
            m[r * 4 + 0] = M[r][0];
            m[r * 4 + 1] = M[r][1];
            m[r * 4 + 2] = M[r][2];
        }
    }
    return invM;
}

// ---- Win-only helpers of this file (no callers in either build) -------------------------------
// Win 0x80f040 / 0x80f8d0: y += s x (3-vectors) (name inferred)
void VindsVector3AddScaled(MeReal* y, const MeReal* x, MeReal s)
{
    y[0] = x[0] * s + y[0];
    y[1] = x[1] * s + y[1];
    y[2] = s * x[2] + y[2];
}

// Win 0x80f900: out = a + s x (name inferred)
void VindsVector3ScaleAdd(MeReal* out, const MeReal* a, const MeReal* x, MeReal s)
{
    out[0] = x[0] * s + a[0];
    out[1] = x[1] * s + a[1];
    out[2] = s * x[2] + a[2];
}

// Win 0x80f070 / 0x80f930: out = s x (name inferred)
void VindsVector3Scale(MeReal* out, const MeReal* x, MeReal s)
{
    out[0] = x[0] * s;
    out[1] = x[1] * s;
    out[2] = s * x[2];
}

// Win 0x810370: gathers count doubles at byte offset `field` of every body (name inferred)
void VindsGatherBodyVectors(MeReal* out, MdtKeaBody* const* bodies, int n, int field, int count)
{
    for (int i = 0; i < n; ++i) {
        memcpy(out, (const char*)bodies[i] + field, count * sizeof(MeReal));
        out += count;
    }
}

// Win 0x8103c0: the inverse of VindsGatherBodyVectors (name inferred)
void VindsScatterBodyVectors(MdtKeaBody* const* bodies, int n, const MeReal* in, int field, int count)
{
    for (int i = 0; i < n; ++i) {
        memcpy((char*)bodies[i] + field, in, count * sizeof(MeReal));
        in += count;
    }
}
