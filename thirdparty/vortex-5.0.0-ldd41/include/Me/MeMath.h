/* Vortex 5.0.0 build 800 "vortexldd41" - Me math helpers on plain double arrays
 * (restored; src/Me/MeMath.c). All names inferred (Karma MeMath style).
 * MeMatrix3 = double[3][3] row-major; MeMatrix4 = double[4][4] row-major transform ("TM":
 * rotation rows 0..2, translation in row 3, [3][3] = 1); quaternion = double[4] (w, x, y, z). */
#ifndef ME_MEMATH_H
#define ME_MEMATH_H

#include "Me/MeCall.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef double MeReal;
typedef MeReal MeVector3[3];
typedef MeReal MeVector4[4];
typedef MeReal MeMatrix3[3][3];
typedef MeReal MeMatrix4[4][4];

/* Mac 0x487e9f: G (3x4) with w = 2 G dq (rows: -x w z -y / -y -z w x / -z y -x w) */
void MEAPI MeQuaternionMakeGMatrix(MeReal G[3][4], const MeVector4 q);
/* Mac 0x487f37, Win 0x74e140: in place; returns 0 (unchanged) if the determinant is 0 */
int MEAPI MeMatrix3Invert(MeMatrix3 m);
/* Mac 0x488192, Win 0x74e2b0: inverse of a symmetric 3x3 (e.g. inertia) written as rows */
void MEAPI MeMatrix3InvertSymmetric(const MeMatrix3 m, MeVector3 row0, MeVector3 row1, MeVector3 row2);
/* Mac 0x4882fb: ab = a * b (transforms) */
void MEAPI MeMatrix4TMCompound(MeMatrix4 ab, const MeMatrix4 a, const MeMatrix4 b);
/* Mac 0x488595: rotation part of a * b only (translation row of ab left untouched) */
void MEAPI MeMatrix4TMCompoundRotation(MeMatrix4 ab, const MeMatrix4 a, const MeMatrix4 b);
/* Mac 0x48878b: a expressed in b's frame = a * inverse(b) (translation row too, no [3][3]) */
void MEAPI MeMatrix4TMMakeRelative(MeMatrix4 rel, const MeMatrix4 a, const MeMatrix4 b);
/* Mac 0x4889e0 */
void MEAPI MeQuaternionFromTM(MeVector4 q, const MeMatrix4 tm);
/* Mac 0x488baf: R from Euler angles (uses cos/sin) */
void MEAPI MeMatrix3FromEulerAngles(MeMatrix3 R, MeReal x, MeReal y, MeReal z);
/* Mac 0x488d21: q = rotate q by angular velocity w over time h (uses sin/cos) */
void MEAPI MeQuaternionFiniteRotation(MeVector4 q, const MeVector3 w, MeReal h);
/* Mac 0x488ee8: integrate a body transform; q0 may be NULL (taken from tm0) */
void MEAPI MeMatrix4TMUpdateFromVelocities(MeMatrix4 tm, MeReal epsilon, MeReal h, const MeVector3 vel,
                                     const MeVector3 angvel, const MeMatrix4 tm0, const MeReal* q0);
/* Mac 0x48968f: the same about a centre of mass offset com (body frame) */
void MEAPI MeMatrix4TMUpdateFromVelocitiesCOM(MeMatrix4 tm, MeReal epsilon, MeReal h, const MeVector3 vel,
                                        const MeVector3 angvel, const MeMatrix4 tm0, const MeReal* q0,
                                        const MeVector3 com);
/* Mac 0x489088 / 0x4891ec: |m - I| below tol (relative where |m_ij| >= tol) */
int MEAPI MeMatrix3IsIdentity(const MeMatrix3 m, MeReal tol);
int MEAPI MeMatrix4IsIdentity(const MeMatrix4 m, MeReal tol);
/* Mac 0x48938e: Euler angles of a TM's rotation (asin/cos/atan2) */
void MEAPI MeMatrix4TMGetEulerAngles(const MeMatrix4 tm, MeVector3 angles);
/* Mac 0x4894a4: in place */
void MEAPI MeMatrix4TMInvert(MeMatrix4 tm);
/* Mac 0x48958d: w = 2 G(q0) (q1 - q0) */
void MEAPI MeQuaternionDifferenceToAngularVelocity(MeVector3 w, const MeVector4 q0, const MeVector4 q1);
/* Mac 0x489807, Win 0x74e490: R R^T = I and det R = 1 within tol */
int MEAPI MeMatrix3IsValidOrientation(const MeMatrix3 R, MeReal tol);
/* Mac 0x489b59, Win 0x74e940: [3][3] = 1, [i][3] = 0 and a valid orientation within tol */
int MEAPI MeMatrix4IsTM(const MeMatrix4 tm, MeReal tol);

#ifdef __cplusplus
}
#endif

#endif
