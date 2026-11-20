/* Vortex 5.0.0 build 800 "vortexldd41" - Vinc box geometry (restored).
 * Mac 0x4f5f9d-0x4f6447 (557g), Win 0x770310-0x770830. See include/Vinc/VincPrimitives.h. */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincMath.h"
#include "Me/MeMemory.h"

#include <math.h>

/* Mac 0x7797c0 / Win 0xabd8b8 (-1 until VincBoxRegisterType), name pointer next to it */
static MeI16 sBoxTypeId = -1;
static const char* sBoxName = "Box";

/* Mac 0x4f5f9d, Win 0x7702b0 */
MeI16 MEAPI VincBoxGetTypeId(void)
{
    return sBoxTypeId;
}

/* Mac 0x4f5faf, Win 0x770310 (Mac adds r + r, Win multiplies by 2: identical results) */
void MEAPI VincBoxGetDimensions(VincGeometryID g, MeReal* dx, MeReal* dy, MeReal* dz)
{
    VincBox* b = (VincBox*)g;
    *dx = b->radii[0] + b->radii[0];
    *dy = b->radii[1] + b->radii[1];
    *dz = b->radii[2] + b->radii[2];
}

/* Mac 0x4f5fe7, Win 0x770350 */
MeReal* MEAPI VincBoxGetRadii(VincGeometryID g)
{
    return ((VincBox*)g)->radii;
}

/* Win 0x770340 (Mac: dead-stripped) */
MeReal MEAPI VincBoxGetBoundingRadius(VincGeometryID g)
{
    return ((VincBox*)g)->boundingRadius;
}

/* Mac 0x4f5ff2, Win 0x770360 */
void MEAPI VincBoxDestroy(VincGeometryID g)
{
    if (g)
        MeMemoryAPI.destroy(g);
}

/* Mac 0x4f6018, Win 0x770380: world AABB {min x, max x, min y, max y, min z, max z} */
void MEAPI VincBoxGetAABB(VincGeometryID g, MeMatrix4 tm, MeReal box[6])
{
    VincBox* b = (VincBox*)g;
    const MeReal* m = &tm[0][0];
    MeReal r0 = b->radii[0], r1 = b->radii[1], r2 = b->radii[2];
    MeReal ex = r0 * fabs(m[0]) + r1 * fabs(m[4]) + r2 * fabs(m[8]);
    MeReal ey = r0 * fabs(m[1]) + r1 * fabs(m[5]) + r2 * fabs(m[9]);
    MeReal ez = r0 * fabs(m[2]) + r1 * fabs(m[6]) + r2 * fabs(m[10]);
    box[0] = m[12] - ex;
    box[1] = ex + m[12];
    box[2] = m[13] - ey;
    box[3] = ey + m[13];
    box[4] = m[14] - ez;
    box[5] = ez + m[14];
}

/* Mac 0x4f6154, Win 0x7704d0: sign of *x (NaN -> +1) (name inferred) */
MeReal MEAPI VincSign(const MeReal* x)
{
    if (*x == 0.0)
        return 0.0;
    if (*x < 0.0)
        return -1.0;
    return 1.0;
}

/* Mac 0x4f6199, Win 0x770610: unit density, inertia about the centre, identity relTM */
int MEAPI VincBoxGetMassProperties(VincGeometryID g, MeMatrix4 relTM, MeMatrix3 inertia, MeReal* volume)
{
    VincBox* b = (VincBox*)g;
    MeReal r0 = b->radii[0], r1 = b->radii[1], r2 = b->radii[2];
    int i, j;
    for (i = 0; i < 3; ++i)
        for (j = 0; j < 3; ++j)
            inertia[i][j] = 0.0;
    inertia[0][0] = (r1 * r1 + r2 * r2) * 0.3333333333333333;
    inertia[1][1] = (r0 * r0 + r2 * r2) * 0.3333333333333333;
    inertia[2][2] = (r0 * r0 + r1 * r1) * 0.3333333333333333;
    for (i = 0; i < 4; ++i)
        for (j = 0; j < 4; ++j)
            relTM[i][j] = (i == j) ? 1.0 : 0.0;
    *volume = r0 * 8.0 * r1 * r2;
    return 1;
}

/* Mac 0x4f62b4 (Win: inlined): support point in the box frame */
void MEAPI VincBoxMaximumPointLocal(VincGeometryID g, const MeVector3 dir, MeVector3 out)
{
    VincBox* b = (VincBox*)g;
    out[0] = VincSign(&dir[0]) * b->radii[0];
    out[1] = VincSign(&dir[1]) * b->radii[1];
    out[2] = VincSign(&dir[2]) * b->radii[2];
}

/* Mac 0x4f632a, Win 0x770410 */
void MEAPI VincBoxUpdateAABB(VincGeometryID g, MeMatrix4 tm, MeVector3 min, MeVector3 max, MeReal box[6])
{
    VincBoxGetAABB(g, tm, box);
    VincAABBGetMinMax(box, min, max);
}

/* Mac 0x4f6366, Win 0x7702c0 */
void MEAPI VincBoxSetDimensions(VincGeometryID g, MeReal dx, MeReal dy, MeReal dz)
{
    VincBox* b = (VincBox*)g;
    MeReal rx = dx * 0.5, ry = dy * 0.5, rz = dz * 0.5;
    b->radii[0] = rx;
    b->radii[1] = ry;
    b->radii[2] = rz;
    b->boundingRadius = sqrt(rx * rx + ry * ry + rz * rz);
}

/* Mac 0x4f63bf, Win 0x770830 (Win inlines SetDimensions) */
VincGeometryID MEAPI VincBoxCreate(MeReal dx, MeReal dy, MeReal dz)
{
    VincGeometryID g = (VincGeometryID)MeMemoryAPI.create(sizeof(VincBox));
    if (!g)
        return 0;
    VincGeometryInit(g, VincBoxGetTypeId());
    VincBoxSetDimensions(g, dx, dy, dz);
    return g;
}

/* Mac 0x4f6447, Win 0x7707f0 */
void MEAPI VincBoxRegisterType(void)
{
    sBoxTypeId = (MeI16)VincGeometryRegisterType(2, sBoxName, VincBoxDestroy, VincBoxUpdateAABB,
                                                 VincBoxGetXYAABB, VincBoxGetBSphere,
                                                 VincBoxMaximumPoint, VincBoxGetMassProperties);
}

/* Mac 0x4f64b3, Win 0x770500 (Win inlines VincSign) */
void MEAPI VincBoxMaximumPoint(VincGeometryID g, MeMatrix4 tm, const MeVector3 dir, MeVector3 out)
{
    VincBox* b = (VincBox*)g;
    const MeReal* m = &tm[0][0];
    MeVector3 l, s;
    VincTMInverseRotate(m, dir, l);
    s[2] = VincSign(&l[2]) * b->radii[2];
    s[1] = VincSign(&l[1]) * b->radii[1];
    s[0] = VincSign(&l[0]) * b->radii[0];
    VincTMTransform(m, s, out);
}

/* Mac 0x4f662c, Win 0x770780 */
void MEAPI VincBoxGetXYAABB(VincGeometryID g, MeMatrix4 tm, MeReal box[4])
{
    VincBox* b = (VincBox*)g;
    const MeReal* m = &tm[0][0];
    MeReal ex = b->radii[0] * fabs(m[0]) + b->radii[1] * fabs(m[4]) + b->radii[2] * fabs(m[8]);
    MeReal ey = b->radii[0] * fabs(m[1]) + b->radii[1] * fabs(m[5]) + b->radii[2] * fabs(m[9]);
    box[0] = m[12] - ex;
    box[1] = ex + m[12];
    box[2] = m[13] - ey;
    box[3] = ey + m[13];
}

/* Mac 0x4f612b, Win 0x7704b0 */
void MEAPI VincBoxGetBSphere(VincGeometryID g, MeVector3 center, MeReal* radius)
{
    center[2] = 0.0;
    center[1] = 0.0;
    center[0] = 0.0;
    *radius = ((VincBox*)g)->boundingRadius;
}
