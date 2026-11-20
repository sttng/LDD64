// Vortex 5.0.0 build 800 "vortexldd41" - Euler angles <-> rotation matrix (restored).
// Mac TU 485 0x447dd6-0x44847e (static initialiser 0x448419: <iostream> only); Win
// 0x752570 (+ the order decoder 0x7522d0, inlined on Mac). File name and function names inferred.
//
// Ken Shoemake's "Euler Angle Conversion" (Graphics Gems IV) with Vortex's order word:
// bit 0 frame (rotating: first and last angle swapped), bit 1 repetition, bit 2 parity, bits 3-4
// the first axis, bit 5 handedness (clear: all angles negated). The glue passes 0x20.
// NOTE: sin / cos / atan2 / sqrt are the C library's; the MinGW Win build routes sin / cos / atan2
// through Me/MeCrtMath.h (the VS2008 CRT results.
#include "Vx/VxTransform.h"
#include <math.h>
#include "Me/MeCrtMath.h"
#include <iostream>

namespace Vx {

namespace {

const signed char kEulSafe[4] = { 0, 1, 2, 0 };     // Mac 0x643ef0
const signed char kEulNext[4] = { 1, 2, 0, 1 };     // Mac 0x643ef5
const double kEpsilon = 1.9073486328125e-06;        // 16 * FLT_EPSILON

struct Order {
    explicit Order(int o)
        : n((o >> 2) & 1), i(kEulSafe[(o >> 3) & 3]), j(kEulNext[i + n]), k(kEulNext[i + 1 - n]),
          repetition((o & 2) != 0), rotating((o & 1) != 0), positive((o & 0x20) != 0)
    {
    }
    int n, i, j, k;
    bool repetition, rotating, positive;
};

} // namespace

// Mac 0x447df8, Win 0x752570
VxEulerAngles VxEulerAnglesFromTransform(const VxTransform& tm, int order)
{
    const VxReal(*M)[4] = tm.m;
    Order o(order);
    const int i = o.i, j = o.j, k = o.k;
    VxEulerAngles ea;
    if (!o.repetition) {
        VxReal cy = sqrt(M[i][i] * M[i][i] + M[j][i] * M[j][i]);
        if (cy <= kEpsilon) {
            ea.angle[0] = atan2(-M[j][k], M[j][j]);
            ea.angle[1] = atan2(-M[k][i], cy);
            ea.angle[2] = 0;
        } else {
            ea.angle[0] = atan2(M[k][j], M[k][k]);
            ea.angle[1] = atan2(-M[k][i], cy);
            ea.angle[2] = atan2(M[j][i], M[i][i]);
        }
    } else {
        VxReal sy = sqrt(M[i][j] * M[i][j] + M[i][k] * M[i][k]);
        if (sy <= kEpsilon) {
            ea.angle[0] = atan2(-M[j][k], M[j][j]);
            ea.angle[1] = atan2(sy, M[i][i]);
            ea.angle[2] = 0;
        } else {
            ea.angle[0] = atan2(M[i][j], M[i][k]);
            ea.angle[1] = atan2(sy, M[i][i]);
            ea.angle[2] = atan2(M[j][i], -M[k][i]);
        }
    }
    if (o.n == 1) {
        ea.angle[0] = -ea.angle[0];
        ea.angle[1] = -ea.angle[1];
        ea.angle[2] = -ea.angle[2];
    }
    if (!o.positive) {
        ea.angle[0] = -ea.angle[0];
        ea.angle[1] = -ea.angle[1];
        ea.angle[2] = -ea.angle[2];
    }
    if (o.rotating) {
        VxReal t = ea.angle[0];
        ea.angle[0] = ea.angle[2];
        ea.angle[2] = t;
    }
    ea.order = order;
    return ea;
}

// Mac 0x44810c (Win: inlined into VxTransform::makeRotation's callers)
void VxEulerAnglesToTransform(const VxEulerAngles& ea, VxTransform& out)
{
    Order o(ea.order);
    const int i = o.i, j = o.j, k = o.k;
    VxReal ti = ea.angle[0], tj = ea.angle[1], th = ea.angle[2];
    if (o.rotating) {
        VxReal t = ti;
        ti = th;
        th = t;
    }
    if (!o.positive) {
        ti = -ti;
        tj = -tj;
        th = -th;
    }
    if (o.n == 1) {
        ti = -ti;
        tj = -tj;
        th = -th;
    }
    const VxReal ci = cos(ti), cj = cos(tj), ch = cos(th);
    const VxReal si = sin(ti), sj = sin(tj), sh = sin(th);
    const VxReal cc = ci * ch, cs = ci * sh, sc = si * ch, ss = si * sh;
    VxReal(*M)[4] = out.m;
    if (!o.repetition) {
        M[i][i] = cj * ch;
        M[i][j] = sj * sc - cs;
        M[i][k] = sj * cc + ss;
        M[j][i] = cj * sh;
        M[j][j] = sj * ss + cc;
        M[j][k] = sj * cs - sc;
        M[k][i] = -sj;
        M[k][j] = cj * si;
        M[k][k] = cj * ci;
    } else {
        M[i][i] = cj;
        M[i][j] = sj * si;
        M[i][k] = sj * ci;
        M[j][i] = sj * sh;
        M[j][j] = -cj * ss + cc;
        M[j][k] = cs * -cj - sc;
        M[k][i] = -sj * ch;
        M[k][j] = cj * sc + cs;
        M[k][k] = cj * cc - ss;
    }
}

} // namespace Vx
