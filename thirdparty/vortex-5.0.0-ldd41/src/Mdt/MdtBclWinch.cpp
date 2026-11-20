// Vortex 5.0.0 build 800 "vortexldd41" - MdtBcl winch (VxWinch / VxCable) row builder (restored).
// Mac TU 555 head: 0x4a2e04-0x4a391c (static initialiser 0x4a38f1 / 0x4a391c =
// the MdtConstraint.h one; the Kea code after it belongs to the next file), Win 0x75d480-0x75e110
// (both functions start with the vxcable license gate on Win; not restored here,
// the Mac build has none). File and function names inferred.
// Bodies: 0 = drum, 3 = its counter body (frame `frame`), 1 / 2 = cable ends; the cable is a
// distance row between 1 and 2 whose length follows the drum.
#include "Mdt/MdtConstraint.h"
#include "Mdt/MdtBcl.h"
#include "Mdt/MdtBclRows.h"
#include "Me/MeMath.h"

#include <float.h>
#include <math.h>
#include <string.h>

namespace {
typedef MeReal Frame[4][4];

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif
inline MdtWinch* WinchOf(MdtBclConstraint* c)
{
    return (MdtWinch*)((char*)c - offsetof(MdtConstraint, bcl));
}
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

// (inlined) world attachment frames, as in MdtBcl.cpp
void GetWorldFrames(const MdtBclConstraint* c, Frame* frames, MeReal (*arms)[3])
{
    for (int i = 0; i < c->numBodies; ++i) {
        const MeMatrix4* tm = c->comTM[i];
        if (tm == 0) {
            memcpy(frames[i], c->frame[i], sizeof(Frame));
            if (arms != 0) {
                arms[i][0] = frames[i][3][0];
                arms[i][1] = frames[i][3][1];
                arms[i][2] = frames[i][3][2];
            }
        } else {
            MeMatrix4TMCompound(frames[i], c->frame[i], *tm);
            if (arms != 0) {
                arms[i][0] = frames[i][3][0] - (*tm)[3][0];
                arms[i][1] = frames[i][3][1] - (*tm)[3][1];
                arms[i][2] = frames[i][3][2] - (*tm)[3][2];
            }
        }
    }
}

// Mac 0x4a2e04 (EAX = frames, ECX = 0, EDX = constraint, stack: 3): the drum coordinate (minus
// the separation along / the angle about the drum's axis 0 against the counter body)
void WinchDrumCoordinate(MdtWinch* w, Frame* frames, int i, int other)
{
    if (w->part == 0) {
        w->offset = -((frames[i][3][0] - frames[other][3][0]) * frames[i][0][0]
                      + (frames[i][3][1] - frames[other][3][1]) * frames[i][0][1]
                      + (frames[i][3][2] - frames[other][3][2]) * frames[i][0][2]);
    } else {
        w->offset = -MdtBclUnwrapAngle(-w->offset, frames[other][2], frames[i][2], frames[i][1]);
    }
}
} // namespace

// Mac 0x4a2edf, Win 0x75d4c0: the cable row (length clamped to [minLength, maxLength]); at a
// clamp a second row stops the drum
int MEAPI MdtBclAddWinch(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p)
{
    Frame frames[kMdtBclMaxBodies];
    MeReal arms[kMdtBclMaxBodies][3];
    GetWorldFrames(c, frames, arms);
    MdtKeaConstraintRows* k = (MdtKeaConstraintRows*)rows;
    const int first = k->numRows;
    MdtBclRows r;
    r.xi = k->xi + first;
    r.c = k->c + first;
    r.lo = k->lo + first;
    r.hi = k->hi + first;
    r.epsilon = k->epsilon + first;
    r.gamma = k->gamma + first;
    r.flags = k->rowFlags + first;
    MdtBclJacobianRow jacBuf[kMdtBclMaxBodies][kMdtBclMaxRows];
    MdtBclJacobianRow* jac[kMdtBclMaxBodies];
    memset(jacBuf, 0, sizeof(jacBuf));
    for (int i = 0; i < c->numBodies; ++i)
        jac[i] = jacBuf[i];
    r.jacobian = jac;
    MdtWinch* w = WinchOf(c);
    MeReal d[3];
    d[0] = frames[1][3][0] - frames[2][3][0];
    d[1] = frames[1][3][1] - frames[2][3][1];
    d[2] = frames[1][3][2] - frames[2][3][2];
    MeReal dist = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
    if (0.0 < dist)
        dist = sqrt(dist);
    const MeReal fLo = w->m_af0;   // force bounds of the cable row (-m_af0, m_af8)
    MeReal hi0 = w->m_af8;
    MeReal hi1;
    bool atMin = false, atMax = false;
    const MeReal ratio = w->ratio;
    if (ratio != 0.0 && c->keaBody[0] != 0) {
        if (w->m_b08 != 0) {
            WinchDrumCoordinate(w, frames, 0, 3);
            const MeReal x = w->m_b14 - w->m_b00 * p->stepSize;
            w->m_b14 = x;
            w->length = (x + w->offset) * ratio;
        } else {
            const MdtKeaBody* b = c->keaBody[0];
            MeReal v = frames[0][0][0] * b->velrot[0] + frames[0][0][1] * b->velrot[1]
                       + frames[0][0][2] * b->velrot[2];
            const MdtKeaBody* b3 = c->keaBody[3];
            if (b3 != 0) {
                const MeReal* a = frames[w->frame][0];
                v = v - (a[0] * b3->velrot[0] + a[1] * b3->velrot[1] + a[2] * b3->velrot[2]);
            }
            w->length = -(v + w->m_b00) * ratio * p->stepSize * w->m_b3c + w->length;
        }
        if (w->minLength > w->length) {
            w->length = w->minLength;
            hi1 = hi0;
            atMin = true;
        } else if (w->length > w->maxLength) {
            w->length = w->maxLength;
            hi1 = fLo > hi0 ? fLo : hi0;
            if (dist >= w->maxLength && fLo > hi0)
                hi0 = fLo;
            atMax = true;
        } else {
            hi1 = hi0;
        }
    } else {
        hi1 = hi0;
    }
    MdtBclAddDistanceRow(&r, jac + 1, 0, d, arms[1], w->length, 1);
    r.lo[0] = -fLo;
    r.hi[0] = hi0;
    MeReal* j = jac[0][0] + w->part;
    j[0] = ratio * frames[0][0][0];
    j[1] = ratio * frames[0][0][1];
    j[2] = ratio * frames[0][0][2];
    const MeReal nr = -w->ratio;
    j = jac[3][0] + w->part;
    j[0] = nr * frames[3][0][0];
    j[1] = nr * frames[3][0][1];
    j[2] = nr * frames[3][0][2];
    int n = 1;
    if (atMin || atMax) {
        // the drum's stop row
        j = jac[0][1] + w->part;
        j[0] = frames[0][0][0];
        j[1] = frames[0][0][1];
        j[2] = frames[0][0][2];
        j = jac[3][1] + w->part;
        j[0] = frames[3][0][0] * -1.0;
        j[1] = frames[3][0][1] * -1.0;
        j[2] = frames[3][0][2] * -1.0;
        r.c[1] = -0.0 / w->m_b1c;
        if (atMin)
            r.lo[1] = -fLo;
        else
            r.hi[1] = hi1;
        n = 2;
    }
    return MdtBclEndConstraint(rows, c, &r, n, p);
}

// Mac 0x4a3770, Win 0x75d480: refreshes the drum coordinate (before MdtWinchSetLength)
void MEAPI MdtBclWinchUpdate(MdtBclConstraint* c)
{
    Frame frames[kMdtBclMaxBodies];
    GetWorldFrames(c, frames, 0);
    if (c->keaBody[0] != 0)
        WinchDrumCoordinate(WinchOf(c), frames, 0, 3);
}
