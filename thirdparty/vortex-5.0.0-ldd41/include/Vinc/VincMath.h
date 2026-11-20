/* Vortex 5.0.0 build 800 "vortexldd41" - Vinc inline vector / transform helpers (restored).
 * All names inferred.
 *
 * The Vinc sources use small inline helpers on MeVector3 / MeMatrix4 (the Mcd "lsVec3" /
 * "lsTransform" heritage). Apple GCC inlines all of them; MSVC /LTCG keeps a few out of line,
 * with register (thiscall-like) conventions:
 *   Win 0x759940  VincTMTransform          (ECX = tm; push p, out)        also Win 0x74f620
 *   Win 0x7599a0  VincTMInverseTransform   (ECX = tm; push p, out)        also Win 0x74f680
 *   Win 0x759a00  VincTMRotate             (ECX = tm; push v, out)
 *   Win 0x7598b0  VincVec3AddTo, 0x7694a0 VincVec3SubFrom, 0x7598e0 VincVec3ScaleBy,
 *   Win 0x759910  VincVec3DivBy            (ECX = the vector modified)
 * The association order of every sum below is the one of those x87 bodies (bit-exact contract:
 * a different order changes the last bit). A TM is MeMatrix4 = double[4][4]: rows 0..2 are the
 * axes, row 3 the position. */
#ifndef VINC_VINCMATH_H
#define VINC_VINCMATH_H

#include "Me/MeMath.h"

#if defined(_MSC_VER) || defined(__GNUC__)
#define VINC_INLINE static __inline
#else
#define VINC_INLINE static
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* out = p * R + T: out_i = (p0 R0i + p1 R1i) + p2 R2i + T_i (Win 0x759940) */
VINC_INLINE void VincTMTransform(const MeReal* tm, const MeReal* p, MeReal* out)
{
    MeReal x = p[0], y = p[1], z = p[2];
    out[0] = x * tm[0] + y * tm[4] + z * tm[8] + tm[12];
    out[1] = x * tm[1] + y * tm[5] + z * tm[9] + tm[13];
    out[2] = x * tm[2] + y * tm[6] + z * tm[10] + tm[14];
}

/* out = (p - T) * R^T: out_i = (Ri0 d0 + Ri1 d1) + Ri2 d2 (Win 0x7599a0) */
VINC_INLINE void VincTMInverseTransform(const MeReal* tm, const MeReal* p, MeReal* out)
{
    MeReal d0 = p[0] - tm[12], d1 = p[1] - tm[13], d2 = p[2] - tm[14];
    out[0] = tm[0] * d0 + tm[1] * d1 + tm[2] * d2;
    out[1] = tm[4] * d0 + tm[5] * d1 + tm[6] * d2;
    out[2] = tm[8] * d0 + tm[9] * d1 + tm[10] * d2;
}

/* out = v * R (no translation) (Win 0x759a00) */
VINC_INLINE void VincTMRotate(const MeReal* tm, const MeReal* v, MeReal* out)
{
    MeReal x = v[0], y = v[1], z = v[2];
    out[0] = x * tm[0] + y * tm[4] + z * tm[8];
    out[1] = x * tm[1] + y * tm[5] + z * tm[9];
    out[2] = x * tm[2] + y * tm[6] + z * tm[10];
}

/* out = v * R^T: out_i = (Ri0 v0 + Ri1 v1) + Ri2 v2 */
VINC_INLINE void VincTMInverseRotate(const MeReal* tm, const MeReal* v, MeReal* out)
{
    MeReal x = v[0], y = v[1], z = v[2];
    out[0] = tm[0] * x + tm[1] * y + tm[2] * z;
    out[1] = tm[4] * x + tm[5] * y + tm[6] * z;
    out[2] = tm[8] * x + tm[9] * y + tm[10] * z;
}

/* rel = a expressed in b's frame: rel_ij = a_row_i . b_row_j, rel_3j = (a_3 - b_3) . b_row_j
 * (Win 0x75c060 / Mac: inlined; rel[i][3] and row 3's [3] are not written) */
VINC_INLINE void VincTMMakeRelative(const MeReal* a, const MeReal* b, MeReal* rel)
{
    int i, j;
    MeReal d0, d1, d2;
    for (i = 0; i < 3; ++i)
        for (j = 0; j < 3; ++j)
            rel[i * 4 + j] = a[i * 4] * b[j * 4] + a[i * 4 + 1] * b[j * 4 + 1] + a[i * 4 + 2] * b[j * 4 + 2];
    d2 = a[14] - b[14];
    d1 = a[13] - b[13];
    d0 = a[12] - b[12];
    rel[12] = d0 * b[0] + d1 * b[1] + d2 * b[2];
    rel[13] = d0 * b[4] + d1 * b[5] + d2 * b[6];
    rel[14] = d0 * b[8] + d1 * b[9] + d2 * b[10];
}

VINC_INLINE MeReal VincVec3Dot(const MeReal* a, const MeReal* b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

VINC_INLINE void VincVec3AddTo(MeReal* a, const MeReal* b)      /* Win 0x7598b0: a += b */
{
    a[0] = b[0] + a[0];
    a[1] = b[1] + a[1];
    a[2] = b[2] + a[2];
}

VINC_INLINE void VincVec3SubFrom(MeReal* a, const MeReal* b)    /* Win 0x7694a0: a -= b */
{
    a[0] = a[0] - b[0];
    a[1] = a[1] - b[1];
    a[2] = a[2] - b[2];
}

VINC_INLINE void VincVec3ScaleBy(MeReal* a, MeReal s)           /* Win 0x7598e0: a *= s */
{
    a[0] = s * a[0];
    a[1] = a[1] * s;
    a[2] = a[2] * s;
}

VINC_INLINE void VincVec3DivBy(MeReal* a, MeReal s)             /* Win 0x759910: a *= 1 / s */
{
    MeReal inv = 1.0 / s;
    a[0] = a[0] * inv;
    a[1] = a[1] * inv;
    a[2] = inv * a[2];
}

#ifdef __cplusplus
}
#endif

#endif
