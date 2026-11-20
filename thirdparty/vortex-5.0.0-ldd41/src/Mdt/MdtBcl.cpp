// Vortex 5.0.0 build 800 "vortexldd41" - MdtBcl, the basic constraint library: solver parameters,
// limit coordinates and the row builders of every constraint type (restored).
// Mac TU 553 = 0x494648-0x4a0e56 (static initialiser 0x4a0e13 / 0x4a0e3e = the MdtConstraint.h
// one; the solver-parameter functions 0x494648 / 0x4946fa start the file), Win 0x7360f0-0x73e300
// + 0x73dbe0 / 0x73dc50. All names inferred ("MdtBclAddContact" is the only string).
// A row builder (MdtConstraint::rowBuilder) gets the Kea rows and the constraint's bcl view (+0x1f4
// / Win +0x1f8), builds world attachment frames and lever arms, fills its rows (Jacobian on a zeroed
// stack buffer) and ends with MdtBclEndConstraint; it returns the row count (0 = dropped this step).
#include "Mdt/MdtConstraint.h"
#include "Mdt/MdtWorld.h"
#include "Mdt/MdtBcl.h"
#include "Mdt/MdtBclRows.h"
#include "Me/MeMath.h"
#include "Me/MeMessage.h"

#include <float.h>
#include <math.h>
#include "Me/MeCrtMath.h"
#include <string.h>

// =============================================================================================
// Solver parameters
// =============================================================================================

// Mac 0x494648, Win 0x73dbe0: derived values for step h.
// NOTE: only h is used; the other arguments are never read (Win pushes six 0 dwords between p
// and h, Mac seven).
void MEAPI MdtBclSolverParametersUpdate(MdtBclSolverParameters* p, int grouping, int unused1,
                                        MeReal unused2, MeReal unused3, MeReal h)
{
    (void)grouping;
    (void)unused1;
    (void)unused2;
    (void)unused3;
    p->stepSize = h;
    const MeReal invH = 1.0 / h;
    p->invH = invH;
    const MeReal invH2 = invH * invH;
    p->invH2 = invH2;
    const MeReal d = 1.0 / (p->epsilon * p->gamma * p->invH + 1.0);
    p->m48 = invH2 * (p->epsilon * d);
    p->m68 = d;
    const MeReal d2 = 1.0 / (p->epsilon2 * p->gamma2 * p->invH + 1.0);
    p->m50 = p->epsilon2 * d2 * p->invH2;
    p->m70 = d2;
    p->m58 = p->m28 * p->invH;
    p->m60 = p->m30 * p->invH;
}

// Mac 0x4946fa, Win 0x73dc50 (h = 0 means 1/60)
void MEAPI MdtBclSolverParametersSet(MdtBclSolverParameters* p, MeReal epsilon, MeReal epsilon2,
                                     MeReal h, MeReal k)
{
    if (h == 0.0)
        h = 0.016666666666666666;
    p->epsilon = epsilon;
    MeReal g = 0.0;
    if (epsilon != 0.0)
        g = (h * k) / epsilon;
    p->gamma = g;
    p->epsilon2 = epsilon2;
    g = 0.0;
    if (epsilon2 != 0.0)
        g = (h * k) / epsilon2;
    p->gamma2 = g;
    p->m28 = epsilon / h;
    p->m30 = epsilon2 / h;
}

// =============================================================================================
// Shared pieces
// =============================================================================================
#if defined(_WIN32) && !defined(_WIN64)   // PORT64: the original 32-bit Win layout only
#define MDT_PICK_BCL(mac, win) (win)
#elif defined(__i386__)
#define MDT_PICK_BCL(mac, win) (mac)
#else
#define MDT_PICK_BCL(mac, win) sizeof(MdtBclContactMaterialData)
#endif

namespace {

typedef MeReal Frame[4][4];

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif
const size_t kBclOffset = offsetof(MdtConstraint, bcl);
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

// The constraint type's data after MdtConstraint (MdtHinge::limit ... ; Mac bcl + 0x8f4).
inline MdtConstraint* ConstraintOf(MdtBclConstraint* c)
{
    return (MdtConstraint*)((char*)c - kBclOffset);
}
template <class T> inline T* TypeOf(MdtBclConstraint* c)
{
    return (T*)ConstraintOf(c);
}
// the type's data following the MdtConstraint head
template <class T> inline T* DataOf(MdtBclConstraint* c)
{
    return (T*)((char*)ConstraintOf(c) + sizeof(MdtConstraint));
}

// The contact material data (VxContactMaterial Mac +0x18 / Win +0x30).
#if defined(_WIN32)
const size_t kMaterialDataOffset = 0x30;
#else
const size_t kMaterialDataOffset = 0x18;
#endif
VX_STATIC_ASSERT(sizeof(MdtBclContactMaterialData) == MDT_PICK_BCL(0x110, 0x118));
inline const MdtBclContactMaterialData* MaterialOf(MdtContact* k)
{
    // the data block starts at VxMaterialBase::m18 (Mac +0x18 / Win +0x30 in the binaries; here
    // wherever the build's std::string puts it)
    (void)kMaterialDataOffset;
    return (const MdtBclContactMaterialData*)(const void*)&k->material.m18;
}

inline unsigned int SetKind(unsigned int f, unsigned int kind)
{
    return (f & ~0x6u) | kind;
}
inline unsigned int SetClass(unsigned int f, unsigned int cls)
{
    return (f & ~0x18u) | cls;
}

// Win 0x736020 (inlined on Mac): world attachment frames = frame[i] * comTM[i]
void GetWorldFrames(const MdtBclConstraint* c, Frame* frames)
{
    for (int i = 0; i < c->numBodies; ++i) {
        const MeMatrix4* tm = c->comTM[i];
        if (tm == 0)
            memcpy(frames[i], c->frame[i], sizeof(Frame));
        else
            MeMatrix4TMCompound(frames[i], c->frame[i], *tm);
    }
}

// Win 0x7360f0 (inlined on Mac): world frames and lever arms (attachment point - centre of mass)
void GetWorldFramesAndArms(const MdtBclConstraint* c, Frame* frames, MeReal (*arms)[3])
{
    for (int i = 0; i < c->numBodies; ++i) {
        const MeMatrix4* tm = c->comTM[i];
        if (tm == 0) {
            memcpy(frames[i], c->frame[i], sizeof(Frame));
            arms[i][0] = frames[i][3][0];
            arms[i][1] = frames[i][3][1];
            arms[i][2] = frames[i][3][2];
        } else {
            MeMatrix4TMCompound(frames[i], c->frame[i], *tm);
            arms[i][0] = frames[i][3][0] - (*tm)[3][0];
            arms[i][1] = frames[i][3][1] - (*tm)[3][1];
            arms[i][2] = frames[i][3][2] - (*tm)[3][2];
        }
    }
}

// The builders' common stack frame (Mac: 0x1200 bytes of Jacobian, the frames, the arms, the
// eight row pointers and the six Jacobian block pointers).
struct Scratch {
    Frame frames[kMdtBclMaxBodies];
    MeReal arms[kMdtBclMaxBodies][3];
    MdtBclJacobianRow jacBuf[kMdtBclMaxBodies][kMdtBclMaxRows];
    MdtBclJacobianRow* jac[kMdtBclMaxBodies];
    MdtBclRows r;
    MeReal d[3];    // attachment point 0 - attachment point 1 (most builders)
};

void Begin(MdtKeaConstraints* rows, MdtBclConstraint* c, Scratch* s)
{
    GetWorldFramesAndArms(c, s->frames, s->arms);
    MdtKeaConstraintRows* k = (MdtKeaConstraintRows*)rows;
    const int n = k->numRows;
    s->r.xi = k->xi + n;
    s->r.c = k->c + n;
    s->r.lo = k->lo + n;
    s->r.hi = k->hi + n;
    s->r.epsilon = k->epsilon + n;
    s->r.gamma = k->gamma + n;
    s->r.flags = k->rowFlags + n;
    memset(s->jacBuf, 0, sizeof(s->jacBuf));
    for (int i = 0; i < c->numBodies; ++i)
        s->jac[i] = s->jacBuf[i];
    s->r.jacobian = s->jac;
    for (int i = 0; i != 3; ++i)
        s->d[i] = s->frames[0][3][i] - s->frames[1][3][i];
}

// Drops row `row` of a three-row block (the builders that keep two of the three orientation
// rows): rows row + 1, row + 2 move up, row + 2 is cleared. Jacobian of bodies 0 and 1 only.
void RemoveRow(MdtBclRows* r, int row)
{
    r->xi[row] = r->xi[row + 1];
    r->c[row] = r->c[row + 1];
    r->lo[row] = r->lo[row + 1];
    r->hi[row] = r->hi[row + 1];
    r->epsilon[row] = r->epsilon[row + 1];
    r->gamma[row] = r->gamma[row + 1];
    r->flags[row] = r->flags[row + 1];
    r->xi[row + 1] = r->xi[row + 2];
    r->c[row + 1] = r->c[row + 2];
    r->lo[row + 1] = r->lo[row + 2];
    r->hi[row + 1] = r->hi[row + 2];
    r->epsilon[row + 1] = r->epsilon[row + 2];
    r->gamma[row + 1] = r->gamma[row + 2];
    r->flags[row + 1] = r->flags[row + 2];
    r->xi[row + 2] = 0.0;
    r->c[row + 2] = 0.0;
    r->lo[row + 2] = 0.0;
    r->hi[row + 2] = 0.0;
    r->epsilon[row + 2] = 0.0;
    r->gamma[row + 2] = 0.0;
    r->flags[row + 2] = 0;
    for (int b = 0; b != 2; ++b)
        for (int k = 0; k != 6; ++k)
            r->jacobian[b][row][k] = r->jacobian[b][row + 1][k];
    for (int b = 0; b != 2; ++b)
        for (int k = 0; k != 6; ++k)
            r->jacobian[b][row + 1][k] = r->jacobian[b][row + 2][k];
    for (int b = 0; b != 2; ++b)
        for (int k = 0; k != 6; ++k)
            r->jacobian[b][row + 2][k] = 0.0;
}

bool NeedsLinearLimit(const MdtLimit* l)
{
    return l->limitsActive != 0 || l->control == kMdtLimitLocked || l->control == kMdtLimitMotorized;
}
bool NeedsAngularLimit(const MdtLimit* l)
{
    return l->mf0 != 0 || l->limitsActive != 0 || l->control == kMdtLimitLocked
           || l->control == kMdtLimitMotorized;
}

// The linear limit block of the joint builders: coordinate update, the limit row along axis
// `axis` of body l->me8's frame and, for a motorised coordinate past its stop, a second row
// (stop + motor). maxRow: the second row only below this row (A1P3: 15).
int LinearLimitRows(Scratch* s, MdtBclConstraint* c, const MdtBclSolverParameters* p, MdtLimit* l,
                    int row, int axis, int maxRow = 0x7fffffff)
{
    MdtBclLinearLimitUpdate(c, p, l, s->frames, &s->arms[0][0], s->d);
    if (!MdtBclLimitIsActive(l))
        return row;
    const MeReal* a = s->frames[l->me8][axis];
    MdtBclAddLinearRow(&s->r, s->jac, row, s->d, a, &s->arms[0][0], l->me8, 1);
    MdtBclAddLimitRow(&s->r, p, row, l);
    s->r.flags[row] |= kMdtKeaRowRelaxed;
    ++row;
    if (l->limitsActive == 2 && l->control == kMdtLimitMotorized && row < maxRow) {
        l->limitsActive = 1;
        MdtBclAddLinearRow(&s->r, s->jac, row, s->d, a, &s->arms[0][0], l->me8, 1);
        MdtBclAddLimitRow(&s->r, p, row, l);
        s->r.flags[row] |= kMdtKeaRowRelaxed;
        l->limitsActive = 2;
        ++row;
    }
    return row;
}

// The angular limit block (angle from the frames, row about axis `axis` of body l->me8).
// allowSecond = false: no second row (the first angular limit of types 3 and 4).
int AngularLimitRows(Scratch* s, MdtBclConstraint* c, const MdtBclSolverParameters* p, MdtLimit* l,
                     int row, int axis, bool allowSecond = true)
{
    MdtBclAngleFromFrames(c, l, s->frames);
    MdtBclAngularLimitUpdate(c, p, l, s->frames);
    if (!MdtBclLimitIsActive(l))
        return row;
    const MeReal* a = s->frames[l->me8][axis];
    MdtBclAddAngularAxisRow(&s->r, s->jac, row, a, 0);
    s->r.flags[row] |= kMdtKeaRowRelaxed;
    MdtBclAddLimitRow(&s->r, p, row, l);
    ++row;
    if (allowSecond && l->limitsActive == 2 && l->control == kMdtLimitMotorized) {
        l->limitsActive = 1;
        MdtBclAddAngularAxisRow(&s->r, s->jac, row, a, 0);
        s->r.flags[row] |= kMdtKeaRowRelaxed;
        MdtBclAddLimitRow(&s->r, p, row, l);
        l->limitsActive = 2;
        ++row;
    }
    return row;
}

// Orientation of the two attachment frames relative to their bodies (MeQuaternionFromTM of
// frame[0] / frame[1]); the builders pass them to MdtBclAddOrientationRows with a unit
// reference quaternion and zero reference velocity.
struct OrientationInput {
    int use[2];
    MeReal frameQ[8];
    MeReal qRef[4];
    MeReal wRef[3];
};
void InitOrientation(OrientationInput* o, MdtBclConstraint* c)
{
    o->use[0] = 1;
    o->use[1] = 1;
    o->qRef[0] = 1.0;
    o->qRef[1] = 0.0;
    o->qRef[2] = 0.0;
    o->qRef[3] = 0.0;
    o->wRef[0] = 0.0;
    o->wRef[1] = 0.0;
    o->wRef[2] = 0.0;
    MeQuaternionFromTM(o->frameQ, c->frame[0]);
    MeQuaternionFromTM(o->frameQ + 4, c->frame[1]);
}
int OrientationRows(Scratch* s, MdtBclConstraint* c, int row, OrientationInput* o)
{
    return MdtBclAddOrientationRows(&s->r, s->jac, row, c->vel, o->use, o->frameQ, c->qrot,
                                    o->qRef, o->wRef, 0);
}

// world orientation of an attachment frame: qrot (body) * frameQ (attachment), Mac 0x49617c
void WorldQuaternion(MeReal* q, const MeReal* a, int use, const MeReal* p)
{
    if (a == 0) {
        if (use == 0) {
            q[0] = 1.0;
            q[1] = 0.0;
            q[2] = 0.0;
            q[3] = 0.0;
        } else {
            q[0] = p[0];
            q[1] = p[1];
            q[2] = p[2];
            q[3] = p[3];
        }
    } else if (use == 0) {
        q[0] = a[0];
        q[1] = a[1];
        q[2] = a[2];
        q[3] = a[3];
    } else {
        q[0] = ((p[0] * a[0] - p[1] * a[1]) - p[2] * a[2]) - p[3] * a[3];
        q[1] = ((p[0] * a[1] + a[0] * p[1]) + a[2] * p[3]) - p[2] * a[3];
        q[2] = ((a[2] * p[0] + p[2] * a[0]) + p[1] * a[3]) - a[1] * p[3];
        q[3] = ((p[0] * a[3] + a[0] * p[3]) + a[1] * p[2]) - p[1] * a[2];
    }
}

// Mac 0x644700 / 0x64470c: the plane of an angle about axis i (y, x axes)
const int kAxisNext[3] = { 2, 0, 1 };
const int kAxisPrev[3] = { 1, 2, 0 };

} // namespace

// =============================================================================================
// Limit coordinates (MdtConstraintCalculatePosition)
// =============================================================================================

// Mac 0x4948aa, Win 0x736f00: distance of the attachment points
MeReal MEAPI MdtBclGetDistanceCoordinate(MdtBclConstraint* c, MdtLimit* l)
{
    Frame frames[kMdtBclMaxBodies];
    GetWorldFrames(c, frames);
    const MeReal dx = frames[0][3][0] - frames[1][3][0];
    const MeReal dy = frames[0][3][1] - frames[1][3][1];
    const MeReal dz = frames[0][3][2] - frames[1][3][2];
    const MeReal d = sqrt(0.0 + dx * dx + dy * dy + dz * dz);
    l->position = d;
    return d;
}

// Mac 0x495068, Win 0x736e30: angle about axis l->index of body l->me8 (type 0x15: the angle of
// the separation in that plane)
MeReal MEAPI MdtBclGetAngularCoordinate(MdtBclConstraint* c, MdtLimit* l)
{
    Frame frames[kMdtBclMaxBodies];
    GetWorldFrames(c, frames);
    if (c->type == kMdtTypeAdvanced15) {
        MeReal n[3];
        n[0] = frames[0][3][0] - frames[1][3][0];
        n[1] = frames[0][3][1] - frames[1][3][1];
        const MeReal dz = frames[0][3][2] - frames[1][3][2];
        const MeReal l2 = n[0] * n[0] + n[1] * n[1] + dz * dz;
        if (l2 <= 0.0) {
            n[0] = 1.0;
            n[1] = 0.0;
            n[2] = 0.0;
        } else {
            const MeReal inv = 1.0 / sqrt(l2);
            n[0] = n[0] * inv;
            n[1] = n[1] * inv;
            n[2] = inv * dz;
        }
        MdtBclAngleFromAxes(c, l, n, frames[l->me8][kAxisPrev[l->index]],
                            frames[l->me8][kAxisNext[l->index]]);
    } else {
        MdtBclAngleFromFrames(c, l, frames);
    }
    return l->position;
}

// Mac 0x495321, Win 0x736c10: separation along axis l->index of body 0's frame
MeReal MEAPI MdtBclGetLinearCoordinate(MdtBclConstraint* c, MdtLimit* l)
{
    Frame frames[kMdtBclMaxBodies];
    GetWorldFrames(c, frames);
    const MeReal* a = frames[0][l->index];
    const MeReal p = (frames[0][3][0] - frames[1][3][0]) * a[0]
                     + (frames[0][3][1] - frames[1][3][1]) * a[1]
                     + (frames[0][3][2] - frames[1][3][2]) * a[2];
    l->position = p;
    return p;
}

// =============================================================================================
// Ending a constraint
// =============================================================================================

// Mac 0x494a7c, Win 0x736970 (Win: the two relaxation branches are the helpers 0x7363b0 /
// 0x736420)
int MEAPI MdtBclEndConstraint(MdtKeaConstraints* rows, MdtBclConstraint* c, MdtBclRows* r,
                              int numRows, const MdtBclSolverParameters* p)
{
    if (numRows == 0)
        return 0;
    for (int i = 0; i != kMdtBclMaxRows; ++i)
        c->rowStatus[i] = 0;
    for (int i = 0; i != numRows; ++i) {
        unsigned int f = r->flags[i];
        if ((f & kMdtKeaRowRelaxed) == 0) {
            if (c->relaxActive[i] != 0) {
                // the constraint's own relaxation and force limits for this row
                const MeReal h = p->invH;
                if ((f & kMdtKeaRowBilateral) == 0) {
                    r->epsilon[i] = c->relaxM5e8[i] * h;
                    r->gamma[i] = 0.0;
                } else {
                    MeReal g = 0.0;
                    if (c->relaxDamping[i] != DBL_MAX)
                        g = 1.0 / (h * c->relaxInvStiffness[i] * c->relaxDamping[i] + 1.0);
                    r->epsilon[i] = h * h * c->relaxInvStiffness[i] * g;
                    r->gamma[i] = g;
                }
                if (r->lo[i] < c->forceLo[i])
                    r->lo[i] = c->forceLo[i];
                if (c->forceHi[i] < r->hi[i])
                    r->hi[i] = c->forceHi[i];
            } else if (f == (kMdtKeaRowAngular | kMdtKeaRowBilateral)) {
                r->epsilon[i] = p->m50;
                r->gamma[i] = p->m70;
            } else if (f == (kMdtKeaRowLinear | kMdtKeaRowBilateral)) {
                r->epsilon[i] = p->m48;
                r->gamma[i] = p->m68;
            } else if (f == (kMdtKeaRowLinear | kMdtKeaRowFriction)) {
                r->epsilon[i] = p->m58;
            } else if (f == (kMdtKeaRowAngular | kMdtKeaRowFriction)) {
                r->epsilon[i] = p->m60;
            }
        }
        // the world's minimum relaxation per row kind (rowStatus bit 3 marks a raised row)
        f = r->flags[i];
        if ((f & kMdtKeaRowAngular) == 0) {
            if ((f & kMdtKeaRowBilateral) == 0) {
                if (r->epsilon[i] < p->m58) {
                    r->epsilon[i] = p->m58;
                    r->gamma[i] = 0.0;
                    c->rowStatus[i] |= 8;
                }
            } else if (r->epsilon[i] < p->m48) {
                r->epsilon[i] = p->m48;
                r->gamma[i] = p->m68;
                c->rowStatus[i] |= 8;
            }
        } else if ((f & kMdtKeaRowBilateral) == 0) {
            if (r->epsilon[i] < p->m60) {
                r->epsilon[i] = p->m60;
                r->gamma[i] = 0.0;
                c->rowStatus[i] |= 8;
            }
        } else if (r->epsilon[i] < p->m50) {
            r->epsilon[i] = p->m50;
            r->gamma[i] = p->m70;
            c->rowStatus[i] |= 8;
        }
    }
    // still queued in a later partition of this step: not added now
    if (c->partitionCount != 0)
        return 0;
    // bodies Kea does not move (no Kea body) but with a velocity: their part of J v goes to c
    for (int b = 0; b < c->numBodies; ++b) {
        if (c->keaBody[b] == 0 && c->comTM[b] != 0 && c->vel[b] != 0) {
            const MeReal* v = c->vel[b];
            for (int i = 0; i != numRows; ++i) {
                const MeReal* j = r->jacobian[b][i];
                for (int k = 0; k != 3; ++k)
                    r->c[i] = r->c[i] - (j[k] * v[k] + j[k + 3] * v[k + 4]);
            }
        }
    }
    MdtKeaAddConstraint(rows, r, c->keaBody, c->numBodies, numRows, c->type, 0);
    return numRows;
}

// =============================================================================================
// Contacts
// =============================================================================================

// Mac 0x494e98, Win 0x7372f0: the contact position in the two reference transforms (or the world
// position when a reference is missing)
void MEAPI MdtBclContactUpdateLocalPositions(MdtBclConstraint* c)
{
    MdtContact* k = TypeOf<MdtContact>(c);
    const MeReal* t = k->m_b48;
    if (t == 0) {
        k->m_b00[0] = k->position[0];
        k->m_b00[1] = k->position[1];
        k->m_b00[2] = k->position[2];
    } else {
        const MeReal dx = k->position[0] - t[12];
        const MeReal dy = k->position[1] - t[13];
        const MeReal dz = k->position[2] - t[14];
        k->m_b00[0] = dx * t[0] + dy * t[1] + dz * t[2];
        k->m_b00[1] = dx * t[4] + dy * t[5] + dz * t[6];
        k->m_b00[2] = dx * t[8] + dy * t[9] + dz * t[10];
    }
    t = k->m_b4c;
    if (t == 0) {
        k->m_b18[0] = k->position[0];
        k->m_b18[1] = k->position[1];
        k->m_b18[2] = k->position[2];
    } else {
        const MeReal dx = k->position[0] - t[12];
        const MeReal dy = k->position[1] - t[13];
        const MeReal dz = k->position[2] - t[14];
        k->m_b18[0] = dx * t[0] + dy * t[1] + dz * t[2];
        k->m_b18[1] = dx * t[4] + dy * t[5] + dz * t[6];
        k->m_b18[2] = dx * t[8] + dy * t[9] + dz * t[10];
    }
}

// Mac 0x4954e6, Win 0x73d720: Jacobian of a contact: normal, the friction directions d1 / d2
// (from the contact direction or, without one, any perpendicular), spin about the normal and
// rolling about d1 / d2 (rows only for the axes with a friction model)
void MEAPI MdtBclContactJacobian(MdtBclConstraint* c, MdtBclJacobianRow** jac, MeReal* dirs)
{
    MeReal local[9];
    if (dirs == 0)
        dirs = local;
    MdtContact* k = TypeOf<MdtContact>(c);
    const MdtBclContactMaterialData* m = MaterialOf(k);
    const MeReal* n = k->normal;
    dirs[0] = n[0];
    dirs[1] = n[1];
    dirs[2] = n[2];
    int row;
    if (m->frictionModel[1] == kMdtBclFrictionModelNone && m->frictionModel[4] == kMdtBclFrictionModelNone) {
        row = 1;
        if (m->frictionModel[0] != kMdtBclFrictionModelNone || m->frictionModel[3] != kMdtBclFrictionModelNone) {
            // d1 = the direction projected onto the contact plane
            const MeReal t = -(k->direction[0] * n[0] + k->direction[1] * n[1] + k->direction[2] * n[2]);
            const MeReal x = k->direction[0] + n[0] * t;
            dirs[3] = x;
            const MeReal y = k->direction[1] + t * n[1];
            dirs[4] = y;
            const MeReal z = t * n[2] + k->direction[2];
            dirs[5] = z;
            const MeReal l2 = x * x + y * y + z * z;
            if (l2 <= 0.0) {
                dirs[3] = 1.0;
                dirs[4] = 0.0;
                dirs[5] = 0.0;
            } else {
                const MeReal inv = 1.0 / sqrt(l2);
                dirs[3] = x * inv;
                dirs[4] = y * inv;
                dirs[5] = inv * z;
            }
            if (m->frictionModel[0] != kMdtBclFrictionModelNone)
                row = 2;
        }
    } else {
        if ((m->flags & 1) == 0) {
            // no direction: d1 perpendicular to the normal, d2 = n x d1
            if (fabs(n[0]) + fabs(n[2]) <= fabs(n[1])) {
                const MeReal inv = 1.0 / sqrt(n[2] * n[2] + n[1] * n[1]);
                dirs[3] = 0.0;
                dirs[4] = -n[2] * inv;
                const MeReal z = inv * n[1];
                dirs[5] = z;
                dirs[6] = z * n[1] - n[2] * dirs[4];
                dirs[7] = -n[0] * dirs[5];
                dirs[8] = n[0] * dirs[4];
            } else {
                const MeReal inv = 1.0 / sqrt(n[2] * n[2] + n[0] * n[0]);
                dirs[3] = n[2] * inv;
                dirs[4] = 0.0;
                const MeReal z = -n[0] * inv;
                dirs[5] = z;
                dirs[6] = z * n[1];
                dirs[7] = n[2] * dirs[3] - n[0] * dirs[5];
                dirs[8] = -n[1] * dirs[3];
            }
            k->direction[0] = dirs[3];
            k->direction[1] = dirs[4];
            k->direction[2] = dirs[5];
        } else {
            // d2 = direction x n, d1 = n x d2
            dirs[6] = k->direction[1] * n[2] - k->direction[2] * n[1];
            dirs[7] = k->direction[2] * n[0] - k->direction[0] * n[2];
            dirs[8] = k->direction[0] * n[1] - k->direction[1] * n[0];
            const MeReal x = n[1] * dirs[8] - n[2] * dirs[7];
            dirs[3] = x;
            const MeReal y = n[2] * dirs[6] - n[0] * dirs[8];
            dirs[4] = y;
            const MeReal z = n[0] * dirs[7] - n[1] * dirs[6];
            dirs[5] = z;
            const MeReal l2 = x * x + y * y + z * z;
            if (l2 <= 0.0) {
                dirs[3] = 1.0;
                dirs[4] = 0.0;
                dirs[5] = 0.0;
            } else {
                const MeReal inv = 1.0 / sqrt(l2);
                dirs[3] = x * inv;
                dirs[4] = y * inv;
                dirs[5] = inv * z;
            }
        }
        row = (m->frictionModel[0] != kMdtBclFrictionModelNone) + 1;
        if (m->frictionModel[1] != kMdtBclFrictionModelNone)
            row = row + 1;
    }
    // linear rows (normal, d1, d2) of the moving bodies
    for (int b = 0; b < c->numBodies; ++b) {
        if (c->vel[b] == 0)
            continue;
        const MeReal s = b == 0 ? 1.0 : -1.0;
        const MeReal* tm = &(*c->comTM[b])[0][0];
        const MeReal rx = s * (k->position[0] - tm[12]);
        const MeReal ry = s * (k->position[1] - tm[13]);
        const MeReal rz = s * (k->position[2] - tm[14]);
        MdtBclJacobianRow* j = jac[b];
        j[0][0] = s * dirs[0];
        j[0][1] = s * dirs[1];
        j[0][2] = s * dirs[2];
        j[0][3] = ry * dirs[2] - rz * dirs[1];
        j[0][4] = rz * dirs[0] - rx * dirs[2];
        j[0][5] = rx * dirs[1] - ry * dirs[0];
        int i = 1;
        if (m->frictionModel[0] != kMdtBclFrictionModelNone) {
            j[1][0] = s * dirs[3];
            j[1][1] = s * dirs[4];
            j[1][2] = s * dirs[5];
            j[1][3] = ry * dirs[5] - rz * dirs[4];
            j[1][4] = rz * dirs[3] - rx * dirs[5];
            j[1][5] = rx * dirs[4] - ry * dirs[3];
            i = 2;
        }
        if (m->frictionModel[1] != kMdtBclFrictionModelNone) {
            j[i][0] = s * dirs[6];
            j[i][1] = s * dirs[7];
            j[i][2] = s * dirs[8];
            j[i][3] = ry * dirs[8] - rz * dirs[7];
            j[i][4] = rz * dirs[6] - rx * dirs[8];
            j[i][5] = rx * dirs[7] - ry * dirs[6];
        }
    }
    // spin (about n) and rolling (about d1, d2)
    const MeReal* v = dirs;
    for (int i = 2; i != 5; ++i, v += 3) {
        if (MdtBclContactMaterialGetFrictionModel(m, i) != kMdtBclFrictionModelNone) {
            jac[0][row][3] = v[0];
            jac[0][row][4] = v[1];
            jac[0][row][5] = v[2];
            jac[1][row][3] = -v[0];
            jac[1][row][4] = -v[1];
            jac[1][row][5] = -v[2];
            ++row;
        }
    }
}

// =============================================================================================
// Orientation rows
// =============================================================================================

// Mac 0x49617c, Win 0x7364a0: three angular rows keeping the attachment frames' relative
// orientation at qRef. q0 / q1 = world orientations of the attachment frames (useFrame[i]:
// apply frameQ to the body orientation), e = q0 * conj(q1); xi = E(qRef) e, the Jacobian
// +-0.5 E(q1 qRef) G(q0)^T (bodies with a velocity only), c = 0.5 E(qRef) G(e)^T wRef.
// qOut (may be 0) receives e. Returns 3.
int MEAPI MdtBclAddOrientationRows(MdtBclRows* r, MdtBclJacobianRow** jac, int row,
                                   const MeReal* const* vel, const int* useFrame,
                                   const MeReal* frameQ, const MeReal* const* qrot,
                                   const MeReal* qRef, const MeReal* wRef, MeReal* qOut)
{
    MeReal q0[4], q1[4];
    WorldQuaternion(q0, qrot[0], useFrame[0], frameQ);
    WorldQuaternion(q1, qrot[1], useFrame[1], frameQ + 4);
    MeReal a[3][4];   // G(q0)
    a[0][0] = -q0[1];
    a[1][0] = -q0[2];
    a[2][0] = -q0[3];
    a[2][3] = q0[0];
    a[1][2] = q0[0];
    a[0][1] = q0[0];
    a[2][2] = q0[1];
    a[1][3] = -q0[1];
    a[0][3] = q0[2];
    a[2][1] = -q0[2];
    a[1][1] = q0[3];
    a[0][2] = -q0[3];
    const MeReal n1 = -q1[1], n2 = -q1[2], n3 = -q1[3];
    MeReal e[4];      // q0 * conj(q1)
    e[0] = ((q1[0] * q0[0] - q0[1] * n1) - q0[2] * n2) - q0[3] * n3;
    e[1] = ((q0[0] * n1 + q1[0] * q0[1]) + q0[3] * n2) - q0[2] * n3;
    e[2] = ((q0[0] * n2 + q1[0] * q0[2]) + q0[1] * n3) - q0[3] * n1;
    e[3] = ((n3 * q0[0] + q1[0] * q0[3]) + n1 * q0[2]) - n2 * q0[1];
    MeReal g[4];      // q1 * qRef
    g[0] = ((q1[0] * qRef[0] - q1[1] * qRef[1]) - q1[2] * qRef[2]) - q1[3] * qRef[3];
    g[1] = ((q1[1] * qRef[0] + q1[0] * qRef[1]) + q1[2] * qRef[3]) - q1[3] * qRef[2];
    g[2] = ((q1[2] * qRef[0] + q1[0] * qRef[2]) + q1[3] * qRef[1]) - q1[1] * qRef[3];
    g[3] = ((qRef[0] * q1[3] + q1[0] * qRef[3]) + q1[1] * qRef[2]) - q1[2] * qRef[1];
    MeReal b[3][4];   // E(g)
    b[0][0] = -g[1];
    b[1][0] = -g[2];
    b[2][0] = -g[3];
    b[2][3] = g[0];
    b[1][2] = g[0];
    b[0][1] = g[0];
    b[2][2] = -g[1];
    b[1][3] = g[1];
    b[0][3] = -g[2];
    b[2][1] = g[2];
    b[1][1] = -g[3];
    b[0][2] = g[3];
    MeReal m[3][3];   // E(g) G(q0)^T
    for (int i = 0; i != 3; ++i) {
        for (int j = 0; j != 3; ++j) {
            MeReal sum = 0.0;
            for (int k = 0; k != 4; ++k)
                sum = sum + b[i][k] * a[j][k];
            m[i][j] = sum;
        }
    }
    const MeReal half[2] = { 0.5, -0.5 };
    for (int body = 0; body != 2; ++body) {
        if (vel[body] == 0)
            continue;
        const MeReal s = half[body];
        for (int i = 0; i != 3; ++i)
            for (int k = 3; k != 6; ++k)
                jac[body][row + i][k] = s * m[i][k - 3];
    }
    MeReal cr[3][4];  // E(qRef)
    cr[0][0] = -qRef[1];
    cr[1][0] = -qRef[2];
    cr[2][0] = -qRef[3];
    cr[2][3] = qRef[0];
    cr[1][2] = qRef[0];
    cr[0][1] = qRef[0];
    cr[2][2] = -qRef[1];
    cr[1][3] = qRef[1];
    cr[0][3] = -qRef[2];
    cr[2][1] = qRef[2];
    cr[1][1] = -qRef[3];
    cr[0][2] = qRef[3];
    for (int i = 0; i != 3; ++i) {
        MeReal sum = 0.0;
        for (int k = 0; k != 4; ++k)
            sum = sum + cr[i][k] * e[k];
        r->xi[row + i] = sum;
    }
    MeReal d[3][4];   // G(e)
    d[0][0] = -e[1];
    d[1][0] = -e[2];
    d[2][0] = -e[3];
    d[2][3] = e[0];
    d[1][2] = e[0];
    d[0][1] = e[0];
    d[2][2] = e[1];
    d[1][3] = -e[1];
    d[0][3] = e[2];
    d[2][1] = -e[2];
    d[1][1] = e[3];
    d[0][2] = -e[3];
    MeReal nm[3][3];  // E(qRef) G(e)^T
    for (int i = 0; i != 3; ++i) {
        for (int j = 0; j != 3; ++j) {
            MeReal sum = 0.0;
            for (int k = 0; k != 4; ++k)
                sum = sum + cr[i][k] * d[j][k];
            nm[i][j] = sum;
        }
    }
    for (int i = 0; i != 3; ++i) {
        MeReal sum = 0.0;
        for (int k = 0; k != 3; ++k)
            sum = sum + nm[i][k] * wRef[k];
        r->c[row + i] = sum;
    }
    r->c[row] = r->c[row] * 0.5;
    r->c[row + 1] = r->c[row + 1] * 0.5;
    r->c[row + 2] = r->c[row + 2] * 0.5;
    for (int i = 0; i != 3; ++i) {
        r->flags[row + i] = SetClass(r->flags[row + i], kMdtKeaRowBilateral);
        r->flags[row + i] = SetKind(r->flags[row + i], kMdtKeaRowAngular);
        r->lo[row + i] = -DBL_MAX;
        r->hi[row + i] = DBL_MAX;
    }
    if (qOut != 0) {
        qOut[0] = e[0];
        qOut[1] = e[1];
        qOut[2] = e[2];
        qOut[3] = e[3];
    }
    return 3;
}

// =============================================================================================
// Row builders
// =============================================================================================
namespace {
// Constraint data of the types that have no restored struct in Mdt/MdtTypes.h (layouts from the
// builders; both compilers' natural alignment gives the Mac and Win offsets).
struct Type04Data {             // Mac 0xee4 / Win 0xf10 bytes with the MdtConstraint head
    MdtLimit limit[4];          // +0x000 linear 0 / 1, angular 2 / 3
    int numSamples;             // +0x3e0 lookup table of 6 columns: x, y, z offsets, angles
    const MeReal* samples[6];   // +0x3e4 (column 0 sorted)
};
struct Type06Data {             // Mac 0xb18 / Win 0xb40
    MeReal velocity[3];         // +0x00 attachment velocity in body 0's frame
    MeReal velocity1[3];        // +0x18 in body 1's frame
};
struct Type08Data {             // Mac 0xce4 / Win 0xd10
    MdtLimit limit[2];          // +0x000 angular
    MeReal m1f0;                // +0x1f0
    int body;                   // +0x1f8 body whose frame gives the position rows' axes (-1: world)
};
struct Type10Data {             // Mac 0xb44 / Win 0xb70 (two-pair coupling, e.g. a gear)
    MeReal ratio;               // +0x00
    MeReal hi;                  // +0x08
    MeReal lo;                  // +0x10
    MeReal offset;              // +0x18 integrated
    MeReal invStiffness;        // +0x20 (divides the error)
    MeReal tolerance;           // +0x28 (backlash)
    MeReal velocity;            // +0x30
    int part0;                  // +0x38 Jacobian part of bodies 0 / 2 (0 linear, 3 angular; also
    int part1;                  // +0x3c   the coordinate kind of the pairs) of bodies 1 / 3
    int frame2;                 // +0x40 frame giving body 2's axis
    int frame3;                 // +0x44 frame giving body 3's axis
    int trackPosition;          // +0x48
    MeReal position0;           // +0x4c / Win +0x50 (coordinate of the pair 0 / 2)
    MeReal position1;           // +0x54 / Win +0x58 (pair 1 / 3)
};
struct Type11Data {             // Mac 0xb18 / Win 0xb40 (differential)
    MeReal ratio;               // +0x00
    MeReal hi;                  // +0x08
    MeReal lo;                  // +0x10
    MeReal velocity;            // +0x18
    int frame2;                 // +0x20
    int frame3;                 // +0x24
};
struct Type15Data {             // Mac / Win 0xdd0 / 0xdf8: distance + two swing angles
    MdtLimit limit[3];
};
} // namespace

// Mac 0x496e2a, Win 0x736f60: ball and socket (three position rows; axes of body m_ae8, -1 =
// world axes)
int MEAPI MdtBclAddBSJoint(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p)
{
    Scratch s;
    Begin(rows, c, &s);
    int n = MdtBclAddPositionRows(&s.r, s.jac, 0, s.frames, &s.arms[0][0], TypeOf<MdtBSJoint>(c)->m_ae8, 1);
    n += MdtBclAddFrictionRows(c, &s.r, s.jac, n, s.frames, &s.arms[0][0], p);
    return MdtBclEndConstraint(rows, c, &s.r, n, p);
}

// Mac 0x495d1b, Win 0x738780: type 0x14 (rope: a distance row while longer than m_be0)
int MEAPI MdtBclAddType14(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p)
{
    Scratch s;
    Begin(rows, c, &s);
    const MeReal length = TypeOf<MdtType14>(c)->m_be0;
    MeReal d = s.d[2] * s.d[2] + s.d[0] * s.d[0] + s.d[1] * s.d[1];
    if (0.0 < d)
        d = sqrt(d);
    int n = 0;
    if (0.0 < d - length) {
        n = MdtBclAddDistanceRow(&s.r, s.jac, 0, s.d, &s.arms[0][0], length, 1);
        s.r.hi[n - 1] = 0.0;
    }
    n += MdtBclAddFrictionRows(c, &s.r, s.jac, n, s.frames, &s.arms[0][0], p);
    return MdtBclEndConstraint(rows, c, &s.r, n, p);
}

// Mac 0x497e8e, Win 0x7370b0: lock (MdtConstraintLockConstraint): fixes all six coordinates
int MEAPI MdtBclAddLock(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p)
{
    Scratch s;
    Begin(rows, c, &s);
    static const int a[3] = { 2, 2, 1 };
    static const int b[3] = { 0, 1, 0 };
    int n = 0;
    for (int i = 0; i != 3; ++i) {
        n += MdtBclAddLinearRow(&s.r, s.jac, n, s.d, s.frames[0][i], &s.arms[0][0], 0, 1);
        n += MdtBclAddAngularRow(&s.r, s.jac, n, s.frames[0][a[i]], s.frames[1][b[i]], 1);
    }
    return MdtBclEndConstraint(rows, c, &s.r, n, p);
}

// Mac 0x4982f5, Win 0x73bd20 (+ the vxadvcon license gate on Win; not restored
// here, the Mac build has none): type 0x15 - a soft distance (limit 0 as motor) plus two swing
// angles (limits 1 / 2)
int MEAPI MdtBclAddType15(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p)
{
    Scratch s;
    Begin(rows, c, &s);
    Type15Data* t = DataOf<Type15Data>(c);
    MdtLimit* l = &t->limit[0];
    l->previousPosition = l->position;
    l->position = sqrt(0.0 + s.d[0] * s.d[0] + s.d[1] * s.d[1] + s.d[2] * s.d[2]);
    const MeReal zero = 0.0;
    MdtBclAddDistanceRow(&s.r, s.jac, 0, s.d, &s.arms[0][0], zero, 1);
    MdtBclAddSoftRow(&s.r, p, 0, l->motorInvStiffness, l->motorDamping,
                     (l->position + l->positionOffset) - l->desiredVelocity, l->motorMaxForceLo,
                     l->motorMaxForceHi);
    int n = 1;
    n += MdtBclAddLinearRow(&s.r, s.jac, 1, s.d, s.frames[0][0], &s.arms[0][0], 0, 1);
    n += MdtBclAddAngularRow(&s.r, s.jac, n, s.frames[0][1], s.frames[1][0], 1);
    n += MdtBclAddAngularRow(&s.r, s.jac, n, s.frames[0][2], s.frames[1][0], 1);
    for (int i = 1; i != 3; ++i) {
        MdtLimit* a = &t->limit[i];
        if (!NeedsAngularLimit(a))
            continue;
        const int body = i - 1;
        const MeReal* x = s.frames[body][2];
        const MeReal* y = s.frames[body][1];
        if (body == 0)
            MdtBclAngleFromAxes(c, a, s.d, s.frames[0][1], s.frames[0][2]);
        else
            MdtBclAngleFromAxes(c, a, s.d, s.frames[1][2], s.frames[1][1]);
        MdtBclAngularLimitUpdate(c, p, a, s.frames);
        if (!MdtBclLimitIsActive(a))
            continue;
        MdtBclAddAngleRow(&s.r, s.jac, n, body, x, y, &s.arms[0][0], s.d, 1);
        s.r.flags[n] |= kMdtKeaRowRelaxed;
        MdtBclAddLimitRow(&s.r, p, n, a);
        ++n;
        if (a->limitsActive == 2 && a->control == kMdtLimitMotorized) {
            a->limitsActive = 1;
            MdtBclAddAngleRow(&s.r, s.jac, n, body, x, y, &s.arms[0][0], s.d, 1);
            s.r.flags[n] |= kMdtKeaRowRelaxed;
            MdtBclAddLimitRow(&s.r, p, n, a);
            a->limitsActive = 2;
            ++n;
        }
    }
    return MdtBclEndConstraint(rows, c, &s.r, n, p);
}

// Mac 0x4971c7, Win 0x73dce0 ("MdtBclAddContact"): normal row (penetration, adhesion,
// restitution, soft contact), friction rows per axis of the contact material
int MEAPI MdtBclAddContact(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p)
{
    Scratch s;
    Begin(rows, c, &s);
    MdtContact* k = TypeOf<MdtContact>(c);
    const MdtBclContactMaterialData* m = MaterialOf(k);
    MdtBclRows& r = s.r;
    r.xi[0] = -k->penetration;
    r.flags[0] = SetClass(r.flags[0], kMdtKeaRowBilateral);
    r.flags[0] = SetKind(r.flags[0], kMdtKeaRowLinear);
    r.lo[0] = -m->adhesiveForce;
    r.hi[0] = DBL_MAX;
    if ((m->flags & 4) != 0) {
        MeReal area = k->m_b58;
        if (area <= 0.0) {
            MeWarning(0, "MdtBclAddContact: invalid contact surface, the contact surface is reset to 1.\n");
            area = 1.0;
        }
        const MeReal h = p->invH;
        const MeReal d = area * m->damping;
        const MeReal kk = m->compliance / area;
        MeReal g;
        if (d != DBL_MAX)
            g = 1.0 / (h * kk * d + 1.0);
        else
            g = 0.0;
        r.epsilon[0] = h * h * kk * g;
        r.gamma[0] = g;
        r.flags[0] |= kMdtKeaRowRelaxed;
    }
    int rowOf[5] = { 0, 0, 0, 0, 0 };
    int n = 1;
    for (int i = 0; i != 5; ++i) {
        if (MdtBclContactMaterialGetFrictionModel(m, i) == kMdtBclFrictionModelNone)
            continue;
        r.flags[n] = SetClass(r.flags[n], kMdtKeaRowFriction);
        if (i < 2)
            r.flags[n] = SetKind(r.flags[n], kMdtKeaRowLinear);
        else
            r.flags[n] = SetKind(r.flags[n], kMdtKeaRowAngular);
        if (0.0 < m->slip[i]) {
            r.epsilon[n] = m->slip[i] * p->invH;
            r.gamma[n] = 0.0;
            r.flags[n] |= kMdtKeaRowRelaxed;
        }
        rowOf[i] = n;
        r.c[n] = m->slideVelocity[i];
        ++n;
    }
    MdtBclContactJacobian(c, s.jac, 0);
    // restitution: normal velocity of the bodies that move
    const MeReal restitution = m->restitution;
    if (0.0 < restitution) {
        int moving[3];
        int nm = 0;
        if (c->vel[0] != 0)
            moving[nm++] = 0;
        if (c->vel[1] != 0)
            moving[nm++] = 1;
        moving[nm] = -1;
        MeReal v = 0.0;
        for (int i = 0; moving[i] != -1; ++i) {
            const MeReal* j = s.jac[moving[i]][0];
            const MeReal* w = c->vel[moving[i]];
            for (int a = 0; a != 3; ++a)
                v = v + (j[a] * w[a] + j[a + 3] * w[a + 4]);
        }
        if (-m->restitutionThreshold > v)
            r.c[0] = -restitution * v;
    }
    if (1 < n) {
        MeReal scale[5] = { 1.0, 1.0, 1.0, 1.0, 1.0 };
        if (rowOf[0] < 1 || rowOf[1] < 1) {
            k->m_d00 = 2;
        } else {
            const int state = k->m_d00;
            k->m_d00 = state | 1;
            if ((state & 2) == 0) {
                scale[0] = m->scale[0];
                scale[1] = m->scale[1];
                if ((state & 4) != 0) {
                    for (int i = 2; i != 5; ++i) {
                        if (0 < rowOf[i] && (c->rowStatus[rowOf[i]] & 1) == 0)
                            scale[i] = m->scale[i];
                    }
                }
                if (m->sticky != 0) {
                    // anchored contact: the two friction rows hold the integrated slip
                    if (0.0 < r.xi[0]) {
                        k->m_cf8 = 0.0;
                        k->m_cf0 = 0.0;
                    }
                    if ((k->m_d00 & 4) == 0) {
                        k->m_cf8 = 0.0;
                        k->m_cf0 = 0.0;
                        MdtBclContactUpdateLocalPositions(c);
                    } else {
                        MeReal slip[2] = { 0.0, 0.0 };
                        for (int b = 0; b != 2; ++b) {
                            const MeReal* w = c->vel[b];
                            if (w == 0)
                                continue;
                            for (int i = 1; i != 3; ++i) {
                                const MeReal* j = s.jac[b][i];
                                for (int a = 0; a != 3; ++a) {
                                    slip[i - 1] = slip[i - 1] + j[a] * w[a];
                                    slip[i - 1] = slip[i - 1] + j[a + 3] * w[a + 4];
                                }
                            }
                        }
                        slip[0] = slip[0] - r.c[1];
                        slip[1] = slip[1] - r.c[2];
                        MeReal* acc = &k->m_cf0;
                        for (int i = 0; i != 2; ++i) {
                            const MeReal x = p->stepSize * slip[i] + acc[i];
                            acc[i] = x;
                            r.xi[1 + i] = x;
                            r.flags[1 + i] = SetClass(r.flags[1 + i], kMdtKeaRowBilateral);
                            if ((m->flags & 4) == 0) {
                                r.flags[1 + i] &= ~kMdtKeaRowRelaxed;
                            } else {
                                const MeReal h = p->invH;
                                const MeReal d = k->m_b58 * m->damping;
                                const MeReal kk = m->compliance / k->m_b58;
                                MeReal g = 0.0;
                                if (d != DBL_MAX)
                                    g = 1.0 / (h * kk * d + 1.0);
                                r.epsilon[1 + i] = h * h * kk * g;
                                r.gamma[1 + i] = g;
                                r.flags[1 + i] |= kMdtKeaRowRelaxed;
                            }
                        }
                    }
                }
            } else if (m->sticky != 0) {
                MdtBclContactUpdateLocalPositions(c);
            }
        }
        MdtKeaConstraintRows* kr = (MdtKeaConstraintRows*)rows;
        for (int i = 0; i != 5; ++i) {
            const int row = rowOf[i];
            const int model = MdtBclContactMaterialGetFrictionModel(m, i);
            MeReal limit;
            if (model == kMdtBclFrictionModelBox) {
                const MeReal f = m->boxFrictionForce[i];
                limit = DBL_MAX;
                if (f < DBL_MAX / scale[i])
                    limit = f * scale[i];
            } else if (model == kMdtBclFrictionModelScaledBox) {
                // bounds follow the normal force (Kea): coefficient per coupled row
                kr->numCoupled++;
                kr->coupledRow[row + kr->numRows] = row;
                kr->coupledScale[kr->numRows + row] = m->frictionCoefficient[i] * scale[i];
                limit = 0.0;
            } else {
                continue;
            }
            r.lo[row] = -limit;
            r.hi[row] = limit;
        }
    }
    return MdtBclEndConstraint(rows, c, &r, n, p);
}

// Mac 0x498bb9, Win 0x7381f0: angular1position3 - two orientation rows (the frames' relative
// orientation about axis 0 is free), angular limit 0 and three linear limits
int MEAPI MdtBclAddAngular1Position3(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p)
{
    Scratch s;
    Begin(rows, c, &s);
    MdtAngular1Position3* j = TypeOf<MdtAngular1Position3>(c);
    OrientationInput o;
    InitOrientation(&o, c);
    OrientationRows(&s, c, 0, &o);
    RemoveRow(&s.r, 0);
    int n = 2;
    MdtLimit* l = &j->limit[0];
    if (NeedsAngularLimit(l))
        n = AngularLimitRows(&s, c, p, l, 2, l->index);
    for (int i = 0; i != 3; ++i) {
        l = &j->limit[1 + i];
        if (NeedsLinearLimit(l))
            n = LinearLimitRows(&s, c, p, l, n, i, 0xf);
    }
    n += MdtBclAddFrictionRows(c, &s.r, s.jac, n, s.frames, &s.arms[0][0], p);
    return MdtBclEndConstraint(rows, c, &s.r, n, p);
}

// Mac 0x499664, Win 0x737c20: cylindrical - two linear rows, two orientation rows, a linear
// and an angular limit along / about axis 0
int MEAPI MdtBclAddCylindrical(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p)
{
    Scratch s;
    Begin(rows, c, &s);
    MdtCylindrical* j = TypeOf<MdtCylindrical>(c);
    int n = MdtBclAddLinearRow(&s.r, s.jac, 0, s.d, s.frames[0][1], &s.arms[0][0], 0, 1);
    n += MdtBclAddLinearRow(&s.r, s.jac, n, s.d, s.frames[0][2], &s.arms[0][0], 0, 1);
    OrientationInput o;
    InitOrientation(&o, c);
    OrientationRows(&s, c, n, &o);
    RemoveRow(&s.r, n);
    n += 2;
    if (NeedsLinearLimit(&j->limit[0]))
        n = LinearLimitRows(&s, c, p, &j->limit[0], n, 0);
    if (NeedsAngularLimit(&j->limit[1]))
        n = AngularLimitRows(&s, c, p, &j->limit[1], n, 0);
    n += MdtBclAddFrictionRows(c, &s.r, s.jac, n, s.frames, &s.arms[0][0], p);
    return MdtBclEndConstraint(rows, c, &s.r, n, p);
}

// Mac 0x49a203, Win 0x7378e0: type 2 (prismatic) - two linear rows, three orientation rows, a
// linear limit along axis 0
int MEAPI MdtBclAddType02(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p)
{
    Scratch s;
    Begin(rows, c, &s);
    MdtLimit* l = DataOf<MdtLimit>(c);   // the type's only member
    int n = MdtBclAddLinearRow(&s.r, s.jac, 0, s.d, s.frames[0][1], &s.arms[0][0], 0, 1);
    n += MdtBclAddLinearRow(&s.r, s.jac, n, s.d, s.frames[0][2], &s.arms[0][0], 0, 1);
    OrientationInput o;
    InitOrientation(&o, c);
    n += OrientationRows(&s, c, n, &o);
    if (NeedsLinearLimit(l))
        n = LinearLimitRows(&s, c, p, l, n, 0);
    n += MdtBclAddFrictionRows(c, &s.r, s.jac, n, s.frames, &s.arms[0][0], p);
    return MdtBclEndConstraint(rows, c, &s.r, n, p);
}

// Mac 0x49a903, Win 0x737470: hinge - three position rows (axes of body m_be0, -1 = world),
// two orientation rows (free about axis 0), the angular limit
int MEAPI MdtBclAddHinge(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p)
{
    Scratch s;
    Begin(rows, c, &s);
    MdtHinge* j = TypeOf<MdtHinge>(c);
    int n = MdtBclAddPositionRows(&s.r, s.jac, 0, s.frames, &s.arms[0][0], j->m_be0, 1);
    OrientationInput o;
    InitOrientation(&o, c);
    OrientationRows(&s, c, n, &o);
    RemoveRow(&s.r, n);
    n += 2;
    if (NeedsAngularLimit(&j->limit))
        n = AngularLimitRows(&s, c, p, &j->limit, n, 0);
    n += MdtBclAddFrictionRows(c, &s.r, s.jac, n, s.frames, &s.arms[0][0], p);
    return MdtBclEndConstraint(rows, c, &s.r, n, p);
}

// Mac 0x49b20c, Win 0x73b410: RPRO (relative position, relative orientation): three linear rows
// along body 1's frame axes, the attachment of body 0 optionally following body 1 (m_b80) and
// moving with velocity m_b30, the relative orientation relQ (recomputed from the bodies when
// m_b84) turning with angular velocity m_b68, three orientation rows
int MEAPI MdtBclAddRPROJoint(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p)
{
    Scratch s;
    Begin(rows, c, &s);
    MdtRPROJoint* j = TypeOf<MdtRPROJoint>(c);
    const Frame& w1 = s.frames[1];
    if (j->m_b80 != 0) {
        // attachment of body 0 = body 1's attachment point
        MeReal* pos = c->frame[0][3];
        const MdtKeaBody* kb = c->keaBody[0];
        if (kb == 0) {
            const MeMatrix4* tm = c->comTM[0];
            if (tm == 0) {
                pos[0] = w1[3][0];
                pos[1] = w1[3][1];
                pos[2] = w1[3][2];
            } else {
                const MeReal dx = w1[3][0] - (*tm)[3][0];
                const MeReal dy = w1[3][1] - (*tm)[3][1];
                const MeReal dz = w1[3][2] - (*tm)[3][2];
                pos[0] = dx * (*tm)[0][0] + dy * (*tm)[0][1] + dz * (*tm)[0][2];
                pos[1] = dx * (*tm)[1][0] + dy * (*tm)[1][1] + dz * (*tm)[1][2];
                pos[2] = dx * (*tm)[2][0] + dy * (*tm)[2][1] + dz * (*tm)[2][2];
            }
        } else {
            const MeReal dx = w1[3][0] - kb->comTM[3][0];
            const MeReal dy = w1[3][1] - kb->comTM[3][1];
            const MeReal dz = w1[3][2] - kb->comTM[3][2];
            pos[0] = dx * kb->comTM[0][0] + dy * kb->comTM[0][1] + dz * kb->comTM[0][2];
            pos[1] = dx * kb->comTM[1][0] + dy * kb->comTM[1][1] + dz * kb->comTM[1][2];
            pos[2] = dx * kb->comTM[2][0] + dy * kb->comTM[2][1] + dz * kb->comTM[2][2];
        }
    }
    if (j->m_b30[0] != 0.0 || j->m_b30[1] != 0.0 || j->m_b30[2] != 0.0) {
        // moving attachment: m_b30 is a velocity in body 1's attachment frame
        const MeReal* v = j->m_b30;
        MeReal x = v[0] * w1[0][0] + v[1] * w1[1][0] + v[2] * w1[2][0];
        MeReal y = v[0] * w1[0][1] + v[1] * w1[1][1] + v[2] * w1[2][1];
        MeReal z = v[0] * w1[0][2] + v[1] * w1[1][2] + v[2] * w1[2][2];
        const MeMatrix4* tm = c->comTM[0];
        if (tm != 0) {
            const MeReal x0 = x, y0 = y;
            x = x0 * (*tm)[0][0] + y0 * (*tm)[0][1] + z * (*tm)[0][2];
            y = x0 * (*tm)[1][0] + y0 * (*tm)[1][1] + z * (*tm)[1][2];
            z = x0 * (*tm)[2][0] + y0 * (*tm)[2][1] + z * (*tm)[2][2];
        }
        const MeReal h = 1.0 / p->invH;
        MeReal* pos = c->frame[0][3];
        pos[0] = h * x + pos[0];
        pos[1] = h * y + pos[1];
        pos[2] = z * h + pos[2];
        const MeReal(*f1)[4] = c->frame[1];
        s.r.c[0] = f1[0][0] * v[0] + f1[0][1] * v[1] + f1[0][2] * v[2];
        s.r.c[1] = f1[1][0] * v[0] + f1[1][1] * v[1] + f1[1][2] * v[2];
        s.r.c[2] = f1[2][0] * v[0] + f1[2][1] * v[1] + f1[2][2] * v[2];
        s.r.c[0] = s.r.c[0] * -1.0;
        s.r.c[1] = s.r.c[1] * -1.0;
        s.r.c[2] = s.r.c[2] * -1.0;
    }
    MdtBclAddLinearRow(&s.r, s.jac, 0, s.d, w1[0], &s.arms[0][0], 1, 1);
    MdtBclAddLinearRow(&s.r, s.jac, 1, s.d, w1[1], &s.arms[0][0], 1, 1);
    MdtBclAddLinearRow(&s.r, s.jac, 2, s.d, w1[2], &s.arms[0][0], 1, 1);
    MeReal* rq = j->relQ;
    if (j->m_b84 != 0) {
        // relQ = conj(q2) * conj(qrot1) * qrot0 * q1
        MeReal a[4], cq[4];
        const MeReal* q = c->qrot[0];
        if (q == 0) {
            a[0] = 1.0; a[1] = 0.0; a[2] = 0.0; a[3] = 0.0;
        } else {
            a[0] = q[0]; a[1] = q[1]; a[2] = q[2]; a[3] = q[3];
        }
        q = c->qrot[1];
        if (q == 0) {
            cq[0] = 1.0; cq[1] = 0.0; cq[2] = 0.0; cq[3] = 0.0;
        } else {
            cq[0] = q[0]; cq[1] = -q[1]; cq[2] = -q[2]; cq[3] = -q[3];
        }
        const MeReal s0 = j->q2[0], s1 = -j->q2[1], s2 = -j->q2[2], s3 = -j->q2[3];
        const MeReal* b = j->q1;
        const MeReal p0 = ((a[0] * b[0] - a[1] * b[1]) - a[2] * b[2]) - a[3] * b[3];
        const MeReal p1 = ((a[1] * b[0] + a[0] * b[1]) + a[2] * b[3]) - a[3] * b[2];
        const MeReal p2 = ((a[2] * b[0] + a[0] * b[2]) + a[3] * b[1]) - a[1] * b[3];
        const MeReal p3 = ((b[0] * a[3] + b[3] * a[0]) + b[2] * a[1]) - a[2] * b[1];
        const MeReal t0 = ((cq[0] * p0 - cq[1] * p1) - cq[2] * p2) - cq[3] * p3;
        const MeReal t1 = ((cq[1] * p0 + cq[0] * p1) + cq[2] * p3) - cq[3] * p2;
        const MeReal t2 = ((cq[2] * p0 + cq[0] * p2) + cq[3] * p1) - cq[1] * p3;
        const MeReal t3 = ((p0 * cq[3] + cq[0] * p3) + p2 * cq[1]) - p1 * cq[2];
        rq[0] = ((s0 * t0 - s1 * t1) - s2 * t2) - s3 * t3;
        rq[1] = ((s1 * t0 + s0 * t1) + s2 * t3) - s3 * t2;
        rq[2] = ((s2 * t0 + s0 * t2) + s3 * t1) - s1 * t3;
        rq[3] = ((s3 * t0 + s0 * t3) + s1 * t2) - s2 * t1;
    }
    // relQ turns by -m_b68 over the step
    MeReal q[4] = { 1.0, 0.0, 0.0, 0.0 };
    MeReal w[3];
    w[0] = j->m_b68[0] * -1.0;
    w[1] = j->m_b68[1] * -1.0;
    w[2] = j->m_b68[2] * -1.0;
    MeQuaternionFiniteRotation(q, w, p->stepSize);
    {
        const MeReal r0 = rq[0], r1 = rq[1], r2 = rq[2], r3 = rq[3];
        const MeReal n0 = ((r0 * q[0] - r1 * q[1]) - r2 * q[2]) - r3 * q[3];
        rq[0] = n0;
        const MeReal n1 = ((r0 * q[1] + r1 * q[0]) + q[2] * r3) - r2 * q[3];
        rq[1] = n1;
        const MeReal n2 = ((q[2] * r0 + r2 * q[0]) + r1 * q[3]) - r3 * q[1];
        rq[2] = n2;
        const MeReal n3 = ((r0 * q[3] + q[0] * r3) + q[1] * r2) - r1 * q[2];
        rq[3] = n3;
    }
    // orientation rows: attachment 0 = conj(relQ) * q1, attachment 1 = q2
    MeReal fq[8];
    const MeReal c0 = rq[0], c1 = -rq[1], c2 = -rq[2], c3 = -rq[3];
    const MeReal* b = j->q1;
    fq[0] = ((c0 * b[0] - c1 * b[1]) - c2 * b[2]) - c3 * b[3];
    fq[1] = ((c1 * b[0] + c0 * b[1]) + c2 * b[3]) - c3 * b[2];
    fq[2] = ((c2 * b[0] + c0 * b[2]) + c3 * b[1]) - c1 * b[3];
    fq[3] = ((c3 * b[0] + c0 * b[3]) + c1 * b[2]) - c2 * b[1];
    fq[4] = j->q2[0];
    fq[5] = j->q2[1];
    fq[6] = j->q2[2];
    fq[7] = j->q2[3];
    const MeReal qRef[4] = { 1.0, 0.0, 0.0, 0.0 };
    j->q1Set = 1;
    j->q2Set = 1;
    int n = 3 + MdtBclAddOrientationRows(&s.r, s.jac, 3, c->vel, &j->q1Set, fq, c->qrot, qRef, w, 0);
    n += MdtBclAddFrictionRows(c, &s.r, s.jac, n, s.frames, &s.arms[0][0], p);
    return MdtBclEndConstraint(rows, c, &s.r, n, p);
}

// Mac 0x49c323, Win 0x73b1e0: type 0x11 (differential): one angular row coupling body 0's spin
// with bodies 1 (linear, ratio), 2 (angular) and 3
int MEAPI MdtBclAddType11(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p)
{
    Scratch s;
    Begin(rows, c, &s);
    Type11Data* t = DataOf<Type11Data>(c);
    s.r.lo[0] = t->lo;
    s.r.hi[0] = t->hi;
    s.r.xi[0] = 0.0;
    s.r.c[0] = t->velocity;
    MeReal* j0 = s.jac[0][0];
    j0[3] = s.frames[0][0][0];
    j0[4] = s.frames[0][0][1];
    j0[5] = s.frames[0][0][2];
    const MeReal nr = -t->ratio;
    MeReal* j1 = s.jac[1][0];
    j1[0] = nr * s.frames[1][0][0];
    j1[1] = nr * s.frames[1][0][1];
    j1[2] = nr * s.frames[1][0][2];
    MeReal* j2 = s.jac[2][0];
    j2[3] = s.frames[t->frame2][0][0] * -1.0;
    j2[4] = s.frames[t->frame2][0][1] * -1.0;
    j2[5] = s.frames[t->frame2][0][2] * -1.0;
    const MeReal r = t->ratio;
    if (c->keaBody[2] == c->keaBody[3]) {
        j2[0] = r * s.frames[t->frame3][0][0] + j2[0];
        j2[1] = r * s.frames[t->frame3][0][1] + j2[1];
        j2[2] = j2[2] + r * s.frames[t->frame3][0][2];
    } else {
        MeReal* j3 = s.jac[3][0];
        j3[0] = r * s.frames[t->frame3][0][0];
        j3[1] = r * s.frames[t->frame3][0][1];
        j3[2] = r * s.frames[t->frame3][0][2];
    }
    s.r.flags[0] = SetKind(s.r.flags[0], kMdtKeaRowAngular);
    s.r.flags[0] = SetClass(s.r.flags[0], kMdtKeaRowFriction);
    return MdtBclEndConstraint(rows, c, &s.r, 1, p);
}

// Mac 0x4947a2 (EAX = frames, ECX = i, EDX = out; inlined on Win): coordinate of pair (i, other):
// kind 0 = separation along body i's axis 0, else the unwrapped angle about it
static void Type10Coordinate(Frame* frames, int i, MeReal* out, int other, int kind)
{
    if (kind == 0) {
        *out = (frames[i][3][0] - frames[other][3][0]) * frames[i][0][0]
               + (frames[i][3][1] - frames[other][3][1]) * frames[i][0][1]
               + (frames[i][3][2] - frames[other][3][2]) * frames[i][0][2];
    } else {
        *out = MdtBclUnwrapAngle(*out, frames[other][2], frames[i][2], frames[i][1]);
    }
}

// Mac 0x49483a (EAX = constraint, EDX = frames): both pair coordinates of type 0x10
static void Type10Coordinates(MdtBclConstraint* c, Frame* frames)
{
    Type10Data* t = DataOf<Type10Data>(c);
    if (c->keaBody[0] != 0)
        Type10Coordinate(frames, 0, &t->position0, 2, t->part0);
    if (c->keaBody[1] != 0)
        Type10Coordinate(frames, 1, &t->position1, 3, t->part1);
}

// Mac 0x49c852, Win 0x73ada0: type 0x10 (gear: pairs (0, 2) and (1, 3) coupled with ratio,
// optional position tracking with backlash); returns 0 inside the backlash
int MEAPI MdtBclAddType10(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p)
{
    Scratch s;
    Begin(rows, c, &s);
    Type10Data* t = DataOf<Type10Data>(c);
    MeReal v = t->velocity;
    MeReal lo = t->lo;
    MeReal hi = t->hi;
    if (t->trackPosition != 0) {
        Type10Coordinates(c, s.frames);
        MeReal e = t->offset + v * p->stepSize;
        t->offset = e;
        e = e + (t->ratio * t->position1 + t->position0);
        const MeReal tol = t->tolerance;
        if (1e-07 < tol) {
            if (-tol <= e) {
                if (e <= tol)
                    return 0;
                e = e - tol;
                hi = 0.0;
            } else {
                e = e + tol;
                lo = 0.0;
            }
        }
        v = v - e / t->invStiffness;
    }
    s.r.lo[0] = lo;
    s.r.hi[0] = hi;
    s.r.xi[0] = 0.0;
    s.r.c[0] = v;
    MeReal* j = s.jac[0][0] + t->part0;
    j[0] = s.frames[0][0][0];
    j[1] = s.frames[0][0][1];
    j[2] = s.frames[0][0][2];
    MeReal r = t->ratio;
    j = s.jac[1][0] + t->part1;
    j[0] = r * s.frames[1][0][0];
    j[1] = r * s.frames[1][0][1];
    j[2] = r * s.frames[1][0][2];
    j = s.jac[2][0] + t->part0;
    j[0] = s.frames[t->frame2][0][0] * -1.0;
    j[1] = s.frames[t->frame2][0][1] * -1.0;
    j[2] = s.frames[t->frame2][0][2] * -1.0;
    r = -t->ratio;
    if (c->keaBody[2] == c->keaBody[3]) {
        j = s.jac[2][0] + t->part1;
        j[0] = r * s.frames[t->frame3][0][0] + j[0];
        j[1] = r * s.frames[t->frame3][0][1] + j[1];
        j[2] = j[2] + r * s.frames[t->frame3][0][2];
    } else {
        j = s.jac[3][0] + t->part1;
        j[0] = r * s.frames[t->frame3][0][0];
        j[1] = r * s.frames[t->frame3][0][1];
        j[2] = r * s.frames[t->frame3][0][2];
    }
    if (t->part0 == 3)
        s.r.flags[0] = SetKind(s.r.flags[0], kMdtKeaRowAngular);
    else
        s.r.flags[0] = SetKind(s.r.flags[0], kMdtKeaRowLinear);
    s.r.flags[0] = SetClass(s.r.flags[0], kMdtKeaRowFriction);
    return MdtBclEndConstraint(rows, c, &s.r, 1, p);
}

// Mac 0x49cedf, Win 0x73aba0: cone limit - one angular row while the angle between the two
// axes 0 exceeds the half angle
int MEAPI MdtBclAddConeLimit(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p)
{
    Scratch s;
    Begin(rows, c, &s);
    MdtConeLimit* j = TypeOf<MdtConeLimit>(c);
    const MeReal* a = s.frames[0][0];
    const MeReal* b = s.frames[1][0];
    MeReal cosA = a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    // NOTE: acos = the VS2008 CRT's on Win (Me/MeCrtMath.h), Mac libm on the Mac; fmod is exact
    const MeReal half = fmod(j->halfAngle, 3.141592653589793);
    int sign = 1;
    if (half < 0.0) {
        cosA = -cosA;
        sign = -1;
    }
    int n = 0;
    if (j->cosHalfAngle > cosA) {
        if (cosA >= 1.0 || cosA != cosA)
            cosA = 1.0;
        else
            cosA = -1.0 > cosA ? -1.0 : cosA;
        const MeReal angle = (MeReal)sign * acos(cosA);
        MdtBclAddAngularRow(&s.r, s.jac, 0, a, b, 1);
        s.r.xi[0] = half - angle;
        s.r.c[0] = 0.0;
        n = 1;
    }
    n += MdtBclAddFrictionRows(c, &s.r, s.jac, n, s.frames, &s.arms[0][0], p);
    return MdtBclEndConstraint(rows, c, &s.r, n, p);
}

// Mac 0x49d3fa, Win 0x73a840: spring - a soft distance row (the limit's motor stiffness /
// damping), the distance stops as a bounce row
int MEAPI MdtBclAddSpring(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p)
{
    Scratch s;
    Begin(rows, c, &s);
    MdtLimit* l = &TypeOf<MdtSpring>(c)->limit;
    l->previousPosition = l->position;
    MeReal len = s.d[0] * s.d[0] + s.d[1] * s.d[1] + s.d[2] * s.d[2];
    MeReal nx, ny, nz;
    if (len > 0.0) {
        len = sqrt(len);
        const MeReal inv = 1.0 / len;
        nx = s.d[0] * inv;
        ny = s.d[1] * inv;
        nz = s.d[2] * inv;
    } else {
        nz = 0.0;
        ny = 0.0;
        nx = 1.0;
    }
    l->position = len;
    l->mec = 0;
    const int n = MdtBclAddDistanceRow(&s.r, s.jac, 0, s.d, &s.arms[0][0], 0.0, 1);
    l->control = l->motorInvStiffness != DBL_MAX;
    const MeReal h = p->stepSize;
    if (l->lower.m20 != 0.0 || l->upper.m20 != 0.0) {
        const MeReal lo = l->lower.m20 * h + l->lower.stop;
        const MeReal hi = h * l->upper.m20 + l->upper.stop;
        if (hi > lo) {
            l->lower.stop = lo;
            l->upper.stop = hi;
        }
    }
    if (l->control == kMdtLimitMotorized && l->m18 != 0.0) {
        // NOTE: a target leaving [lower, upper] is put back (motor velocity cleared)
        const MeReal old = l->desiredVelocity;
        const MeReal t = old + l->m18 * h;
        l->desiredVelocity = t;
        if (l->lower.stop > t) {
            l->desiredVelocity = old;
            l->m18 = 0.0;
        } else if (t > l->upper.stop) {
            l->desiredVelocity = old;
            l->m18 = 0.0;
        }
    }
    l->m50 = 0.0;
    bool bounce = false;
    if (l->limitsActive != 0) {
        if (MdtBclLimitUpdateOvershoot(l)) {
            l->limitsActive = 2;
            const MeReal* v = c->vel[0];
            if (v != 0)
                l->m50 = nx * v[0] + ny * v[1] + nz * v[2] + l->m50;
            v = c->vel[1];
            if (v != 0)
                l->m50 = l->m50 - (nx * v[0] + ny * v[1] + nz * v[2]);
            MdtBclAddLimitBounceRow(&s.r, p, 0, l);
            bounce = true;
        } else if (l->limitsActive == 2) {
            l->limitsActive = 1;
        }
    }
    if (!bounce)
        MdtBclAddSoftRow(&s.r, p, 0, l->motorInvStiffness, l->motorDamping,
                         (l->position + l->positionOffset) - l->desiredVelocity, l->motorMaxForceLo,
                         l->motorMaxForceHi);
    const int f = MdtBclAddFrictionRows(c, &s.r, s.jac, n, s.frames, &s.arms[0][0], p);
    return MdtBclEndConstraint(rows, c, &s.r, f + n, p);
}

// Mac 0x49db40, Win 0x73a360: type 0x0c - two linear rows (body 1's axes 1, 2), one angular row
int MEAPI MdtBclAddType0c(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p)
{
    Scratch s;
    Begin(rows, c, &s);
    int n = MdtBclAddLinearRow(&s.r, s.jac, 0, s.d, s.frames[1][1], &s.arms[0][0], 1, 1);
    n += MdtBclAddLinearRow(&s.r, s.jac, n, s.d, s.frames[1][2], &s.arms[0][0], 1, 1);
    n += MdtBclAddAngularRow(&s.r, s.jac, n, s.frames[0][1], s.frames[1][0], 1);
    n += MdtBclAddFrictionRows(c, &s.r, s.jac, n, s.frames, &s.arms[0][0], p);
    return MdtBclEndConstraint(rows, c, &s.r, n, p);
}

// Mac 0x49dfac, Win 0x73a1d0: linear2 - two linear rows along body 1's axes 1 and 2
int MEAPI MdtBclAddLinear2(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p)
{
    Scratch s;
    Begin(rows, c, &s);
    int n = MdtBclAddLinearRow(&s.r, s.jac, 0, s.d, s.frames[1][1], &s.arms[0][0], 1, 1);
    n += MdtBclAddLinearRow(&s.r, s.jac, n, s.d, s.frames[1][2], &s.arms[0][0], 1, 1);
    n += MdtBclAddFrictionRows(c, &s.r, s.jac, n, s.frames, &s.arms[0][0], p);
    return MdtBclEndConstraint(rows, c, &s.r, n, p);
}

// Mac 0x49e3de, Win 0x73a060: type 0x0a - one linear row along body 1's axis 0
int MEAPI MdtBclAddType0a(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p)
{
    Scratch s;
    Begin(rows, c, &s);
    int n = MdtBclAddLinearRow(&s.r, s.jac, 0, s.d, s.frames[1][0], &s.arms[0][0], 1, 1);
    n += MdtBclAddFrictionRows(c, &s.r, s.jac, n, s.frames, &s.arms[0][0], p);
    return MdtBclEndConstraint(rows, c, &s.r, n, p);
}

// Mac 0x49e7c2, Win 0x739e10 (VxMotorConstraint): the row of the limit only (linear or angular
// by MdtLimit::kind), no friction rows
int MEAPI MdtBclAddMotor(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p)
{
    Scratch s;
    Begin(rows, c, &s);
    MdtLimit* l = &TypeOf<MdtMotor>(c)->limit;
    int n = 0;
    if (NeedsAngularLimit(l)) {
        if (l->kind == kMdtLimitLinear) {
            MdtBclLinearLimitUpdate(c, p, l, s.frames, &s.arms[0][0], s.d);
            if (MdtBclLimitIsActive(l)) {
                MdtBclAddLinearRow(&s.r, s.jac, 0, s.d, s.frames[l->me8][l->index], &s.arms[0][0], l->me8, 1);
                MdtBclAddLimitRow(&s.r, p, 0, l);
                s.r.flags[0] |= kMdtKeaRowRelaxed;
                n = 1;
            }
        } else {
            MdtBclAngleFromFrames(c, l, s.frames);
            MdtBclAngularLimitUpdate(c, p, l, s.frames);
            if (MdtBclLimitIsActive(l)) {
                MdtBclAddAngularAxisRow(&s.r, s.jac, 0, s.frames[l->me8][l->index], 0);
                s.r.flags[0] |= kMdtKeaRowRelaxed;
                MdtBclAddLimitRow(&s.r, p, 0, l);
                n = 1;
            }
        }
    }
    return MdtBclEndConstraint(rows, c, &s.r, n, p);
}

// Mac 0x49ecc3, Win 0x739790: type 8 (universal joint): position rows, one angular row, two
// angular limits
int MEAPI MdtBclAddType08(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p)
{
    Scratch s;
    Begin(rows, c, &s);
    Type08Data* t = DataOf<Type08Data>(c);
    int n = MdtBclAddPositionRows(&s.r, s.jac, 0, s.frames, &s.arms[0][0], t->body, 1);
    n += MdtBclAddAngularRow(&s.r, s.jac, n, s.frames[0][0], s.frames[1][1], 1);
    for (int i = 0; i != 2; ++i) {
        MdtLimit* l = &t->limit[i];
        if (NeedsAngularLimit(l))
            n = AngularLimitRows(&s, c, p, l, n, l->index);
    }
    n += MdtBclAddFrictionRows(c, &s.r, s.jac, n, s.frames, &s.arms[0][0], p);
    return MdtBclEndConstraint(rows, c, &s.r, n, p);
}

// Mac 0x49f28a, Win 0x7395a0: type 6 (velocity-driven point): position rows along body 1's
// axes, velocity term = attachment velocity difference
int MEAPI MdtBclAddType06(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p)
{
    Scratch s;
    Begin(rows, c, &s);
    Type06Data* t = DataOf<Type06Data>(c);
    const int n = MdtBclAddPositionRows(&s.r, s.jac, 0, s.frames, &s.arms[0][0], 1, 1);
    const MeReal* v = t->velocity;
    const MeReal* w = t->velocity1;
    const Frame& f0 = s.frames[0];
    const Frame& f1 = s.frames[1];
    MeReal t0, t1, t2;
    if (c->keaBody[1] == 0) {
        t0 = w[0];
        t1 = w[1];
        t2 = w[2];
    } else {
        t0 = w[0] * f1[0][0] + w[1] * f1[1][0] + w[2] * f1[2][0] + f1[3][0] * 0.0;
        t1 = w[0] * f1[0][1] + w[1] * f1[1][1] + w[2] * f1[2][1] + f1[3][1] * 0.0;
        t2 = w[0] * f1[0][2] + w[1] * f1[1][2] + w[2] * f1[2][2] + f1[3][2] * 0.0;
    }
    s.r.c[0] = -((v[0] * f0[0][0] + v[1] * f0[1][0] + v[2] * f0[2][0] + f0[3][0] * 0.0) - t0);
    s.r.c[1] = -((v[0] * f0[0][1] + v[1] * f0[1][1] + v[2] * f0[2][1] + f0[3][1] * 0.0) - t1);
    s.r.c[2] = -((v[0] * f0[0][2] + v[1] * f0[1][2] + v[2] * f0[2][2] + f0[3][2] * 0.0) - t2);
    return MdtBclEndConstraint(rows, c, &s.r, n, p);
}

// Mac 0x4a0587, Win 0x738930: type 3 (planar-like): two linear rows, one angular row, a linear
// limit (limit 0) and two angular limits (1, 2; only limit 2 can take a second row)
int MEAPI MdtBclAddType03(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p)
{
    Scratch s;
    Begin(rows, c, &s);
    MdtLimit* limit = DataOf<MdtLimit>(c);
    int n = MdtBclAddLinearRow(&s.r, s.jac, 0, s.d, s.frames[0][1], &s.arms[0][0], 0, 1);
    n += MdtBclAddLinearRow(&s.r, s.jac, n, s.d, s.frames[0][2], &s.arms[0][0], 0, 1);
    n += MdtBclAddAngularRow(&s.r, s.jac, n, s.frames[0][0], s.frames[1][1], 1);
    if (NeedsLinearLimit(&limit[0]))
        n = LinearLimitRows(&s, c, p, &limit[0], n, limit[0].index);
    for (int i = 1; i != 3; ++i) {
        MdtLimit* l = &limit[i];
        if (NeedsAngularLimit(l))
            n = AngularLimitRows(&s, c, p, l, n, l->index, i == 2);
    }
    n += MdtBclAddFrictionRows(c, &s.r, s.jac, n, s.frames, &s.arms[0][0], p);
    return MdtBclEndConstraint(rows, c, &s.r, n, p);
}

// Mac 0x49f824, Win 0x738e40: type 4 (prismatic with a guide curve): with a lookup table the
// attachment frame of body 1 is replaced by the table's offset / rotation at the current
// position along axis 0; then one linear row (axis 2), one angular row, linear limits 0 / 1 and
// angular limits 2 / 3 (only limit 3 can take a second row)
int MEAPI MdtBclAddType04(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p)
{
    Scratch s;
    Begin(rows, c, &s);
    Type04Data* t = DataOf<Type04Data>(c);
    if (t->numSamples != 0) {
        MeMatrix4 inv0, w1, m;
        memcpy(inv0, s.frames[0], sizeof(Frame));
        memcpy(w1, s.frames[1], sizeof(Frame));
        MeMatrix4TMInvert(inv0);
        MeMatrix4TMCompound(m, w1, inv0);   // body 1's attachment in body 0's attachment frame
        MeReal e[3];
        MeMatrix4TMGetEulerAngles(m, e);
        MeReal v[6] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
        const MeReal x = m[3][0];
        v[0] = x;
        const MeReal* tx = t->samples[0];
        const int count = t->numSamples;
        int i;
        if (tx[0] >= x) {
            i = 1;
        } else if (x >= tx[count - 1]) {
            i = count - 1;
        } else {
            i = 1;
            if (x >= tx[1]) {
                do {
                    ++i;
                } while (x >= tx[i]);
            }
        }
        for (int k = 1; k != 6; ++k) {
            const MeReal* col = t->samples[k];
            const MeReal a = col[i - 1];
            const MeReal x0 = tx[i - 1];
            v[k] = a + ((v[0] - x0) * (col[i] - a)) / (tx[i] - x0);
        }
        m[3][1] = m[3][1] + v[1];
        m[3][2] = m[3][2] + v[2];
        e[0] = e[0] + v[3];
        e[1] = e[1] + v[4];
        e[2] = e[2] + v[5];
        MeMatrix3 r;
        MeMatrix3FromEulerAngles(r, e[0], e[1], e[2]);
        MeMatrix4 n;
        for (int k = 0; k != 3; ++k) {
            n[k][0] = r[k][0];
            n[k][1] = r[k][1];
            n[k][2] = r[k][2];
            n[k][3] = 0.0;
        }
        n[3][0] = m[3][0];
        n[3][1] = m[3][1];
        n[3][2] = m[3][2];
        n[3][3] = 1.0;
        MeMatrix4TMCompound(s.frames[1], n, s.frames[0]);
    }
    for (int k = 0; k != 3; ++k)
        s.d[k] = s.frames[0][3][k] - s.frames[1][3][k];
    int nr = MdtBclAddLinearRow(&s.r, s.jac, 0, s.d, s.frames[0][2], &s.arms[0][0], 0, 1);
    nr += MdtBclAddAngularRow(&s.r, s.jac, nr, s.frames[0][0], s.frames[1][1], 1);
    for (int k = 0; k != 2; ++k) {
        MdtLimit* l = &t->limit[k];
        if (NeedsLinearLimit(l))
            nr = LinearLimitRows(&s, c, p, l, nr, l->index);
    }
    for (int k = 2; k != 4; ++k) {
        MdtLimit* l = &t->limit[k];
        if (NeedsAngularLimit(l))
            nr = AngularLimitRows(&s, c, p, l, nr, l->index, k == 3);
    }
    nr += MdtBclAddFrictionRows(c, &s.r, s.jac, nr, s.frames, &s.arms[0][0], p);
    return MdtBclEndConstraint(rows, c, &s.r, nr, p);
}
