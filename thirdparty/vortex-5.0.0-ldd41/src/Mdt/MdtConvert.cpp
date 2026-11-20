// Vortex 5.0.0 build 800 "vortexldd41" - body frame conversions (restored).
// Mac 0x492a79-0x4930bc (static initialiser 0x49307a / 0x4930a5), Win 0x778920-0x778ca0
// (file name inferred).
#include "Mdt/MdtBody.h"
#include "Mdt/MdtConstraint.h"
#include "Me/MeMath.h"

#include <math.h>
#include <string.h>

// Mac 0x492a79, Win 0x778920: rotate v from `from`'s COM frame into `to`'s (0 = world)
void MEAPI MdtConvertVector(const MdtBody* from, const MeVector3 v, const MdtBody* to, MeVector3 out)
{
    MeReal x, y, z;
    if (from == 0) {
        x = v[0];
        y = v[1];
        z = v[2];
    } else {
        const MeReal (*R)[4] = from->keaBody.comTM;
        x = v[0] * R[0][0] + v[1] * R[1][0] + v[2] * R[2][0];
        y = v[0] * R[0][1] + v[1] * R[1][1] + v[2] * R[2][1];
        z = v[0] * R[0][2] + v[1] * R[1][2] + v[2] * R[2][2];
    }
    if (to == 0) {
        out[0] = x;
        out[1] = y;
        out[2] = z;
    } else {
        const MeReal (*R)[4] = to->keaBody.comTM;
        out[0] = x * R[0][0] + y * R[0][1] + z * R[0][2];
        out[1] = x * R[1][0] + y * R[1][1] + z * R[1][2];
        out[2] = x * R[2][0] + y * R[2][1] + z * R[2][2];
    }
}

// Mac 0x492be2, Win 0x778a10: the same for a point (translation included)
void MEAPI MdtConvertPositionVector(const MdtBody* from, const MeVector3 p, const MdtBody* to,
                              MeVector3 out)
{
    MeReal x, y, z;
    if (from == 0) {
        x = p[0];
        y = p[1];
        z = p[2];
    } else {
        const MeReal (*R)[4] = from->keaBody.comTM;
        x = p[0] * R[0][0] + p[1] * R[1][0] + p[2] * R[2][0] + R[3][0];
        y = p[0] * R[0][1] + p[1] * R[1][1] + p[2] * R[2][1] + R[3][1];
        z = p[0] * R[0][2] + p[1] * R[1][2] + p[2] * R[2][2] + R[3][2];
    }
    if (to == 0) {
        out[0] = x;
        out[1] = y;
        out[2] = z;
    } else {
        const MeReal (*R)[4] = to->keaBody.comTM;
        x = x - R[3][0];
        y = y - R[3][1];
        z = z - R[3][2];
        out[0] = x * R[0][0] + y * R[0][1] + z * R[0][2];
        out[1] = x * R[1][0] + y * R[1][1] + z * R[1][2];
        out[2] = x * R[2][0] + y * R[2][1] + z * R[2][2];
    }
}

// Mac 0x492d77, Win 0x778ba0: user transform = relative transform * COM transform
void MEAPI MdtBodyUpdateTransformFromCOM(MdtBody* b)
{
    if (b->useRelativeTransform == 0) {
        memcpy(b->transform, b->keaBody.comTM, sizeof(MeMatrix4));
        return;
    }
    const MeReal (*rel)[4] = b->relativeTransform;
    const MeReal (*com)[4] = b->keaBody.comTM;
    for (int r = 0; r != 4; ++r)
        for (int c = 0; c != 4; ++c)
            b->transform[r][c] = rel[r][0] * com[0][c] + rel[r][1] * com[1][c] +
                                 rel[r][2] * com[2][c] + rel[r][3] * com[3][c];
}

// Mac 0x492e34, Win 0x778be0: COM transform = inverse(relative) * user transform; quaternion
// from it, renormalised
void MEAPI MdtBodyUpdateCOMFromTransform(MdtBody* b)
{
    if (b->useRelativeTransform == 0) {
        memcpy(b->keaBody.comTM, b->transform, sizeof(MeMatrix4));
    } else {
        MeMatrix4 inv;
        memcpy(inv, b->relativeTransform, sizeof(MeMatrix4));
        MeMatrix4TMInvert(inv);
        for (int r = 0; r != 4; ++r)
            for (int c = 0; c != 4; ++c)
                b->keaBody.comTM[r][c] = inv[r][0] * b->transform[0][c] + inv[r][1] * b->transform[1][c] +
                                         inv[r][2] * b->transform[2][c] + inv[r][3] * b->transform[3][c];
    }
    MeQuaternionFromTM(b->keaBody.qrot, b->keaBody.comTM);
    MeReal* q = b->keaBody.qrot;
    const MeReal w = q[0], x = q[1], y = q[2], z = q[3];
    const MeReal s = 1.0 / sqrt(z * z + (y * y + (x * x + (w * w + 0.0))));
    q[0] = w * s;
    q[1] = x * s;
    q[2] = y * s;
    q[3] = z * s;
}
