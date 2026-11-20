/* Vortex 5.0.0 build 800 "vortexldd41" - Vinc capsule geometry (restored).
 * Mac 0x4f66f2-0x4f6d70 (557g), Win 0x76a280-0x76a8b0. Axis = z: a segment of half length
 * halfHeight swept by a sphere of radius `radius`. */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincMath.h"
#include "Me/MeMemory.h"

#include <math.h>

/* Mac 0x7797c8 / Win 0xabd7c0 */
static MeI16 sCapsuleTypeId = -1;
static const char* sCapsuleName = "Capsule";

/* Mac 0x4f66f2, Win 0x76a280 */
MeI16 MEAPI VincCapsuleGetTypeId(void)
{
    return sCapsuleTypeId;
}

/* Mac 0x4f6704, Win 0x76a290 */
MeReal MEAPI VincCapsuleGetRadius(VincGeometryID g)
{
    return ((VincCapsule*)g)->radius;
}

/* Mac 0x4f670f, Win 0x76a2b0 */
MeReal MEAPI VincCapsuleGetHalfHeight(VincGeometryID g)
{
    return ((VincCapsule*)g)->halfHeight;
}

/* Win 0x76a2a0 (Mac: dead-stripped) */
MeReal MEAPI VincCapsuleGetHeight(VincGeometryID g)
{
    return ((VincCapsule*)g)->halfHeight + ((VincCapsule*)g)->halfHeight;
}

/* Mac 0x4f671a, Win 0x76a2c0 */
void MEAPI VincCapsuleSetRadius(VincGeometryID g, MeReal radius)
{
    ((VincCapsule*)g)->radius = radius;
}

/* Mac 0x4f672c, Win 0x76a2d0 */
void MEAPI VincCapsuleSetHeight(VincGeometryID g, MeReal height)
{
    ((VincCapsule*)g)->halfHeight = height * 0.5;
}

/* Mac 0x4f674c, Win 0x76a2f0 */
void MEAPI VincCapsuleSetDimensions(VincGeometryID g, MeReal radius, MeReal height)
{
    ((VincCapsule*)g)->radius = radius;
    ((VincCapsule*)g)->halfHeight = height * 0.5;
}

/* Mac 0x4f6776, Win 0x76a310 */
void MEAPI VincCapsuleDestroy(VincGeometryID g)
{
    if (g)
        MeMemoryAPI.destroy(g);
}

/* Mac 0x4f679c, Win 0x76a510 */
void MEAPI VincCapsuleGetBSphere(VincGeometryID g, MeVector3 center, MeReal* radius)
{
    center[2] = 0.0;
    center[1] = 0.0;
    center[0] = 0.0;
    *radius = ((VincCapsule*)g)->halfHeight + ((VincCapsule*)g)->radius;
}

/* Mac 0x4f67ca, Win 0x76a540 */
MeReal MEAPI VincCapsuleGetBoundingRadius(VincGeometryID g)
{
    return ((VincCapsule*)g)->halfHeight + ((VincCapsule*)g)->radius;
}

/* Mac 0x4f67e7, Win 0x76a770. NOTE: the inertia is the cylinder part's only (as in the
 * original); the volume includes both caps. */
int MEAPI VincCapsuleGetMassProperties(VincGeometryID g, MeMatrix4 relTM, MeMatrix3 inertia, MeReal* volume)
{
    VincCapsule* c = (VincCapsule*)g;
    MeReal r = c->radius;
    MeReal h = c->halfHeight + c->halfHeight;
    MeReal ixx;
    int i, j;
    for (i = 0; i < 3; ++i)
        for (j = 0; j < 3; ++j)
            inertia[i][j] = 0.0;
    ixx = r * 0.25 * r + h * 0.08333333333333333 * h;
    inertia[0][0] = ixx;
    inertia[1][1] = ixx;
    inertia[2][2] = r * 0.5 * r;
    for (i = 0; i < 4; ++i)
        for (j = 0; j < 4; ++j)
            relTM[i][j] = (i == j) ? 1.0 : 0.0;
    *volume = r * 3.141592653589793 * r * h + r * 4.1887902047863905 * r * r;
    return 1;
}

/* Mac 0x4f6915, Win 0x76a330 */
void MEAPI VincCapsuleGetAABB(VincGeometryID g, MeMatrix4 tm, MeReal box[6])
{
    VincCapsule* c = (VincCapsule*)g;
    MeReal ax = tm[2][0], ay = tm[2][1], az = tm[2][2];
    MeReal dx = 1.0 - ax * ax, dy = 1.0 - ay * ay, dz = 1.0 - az * az;
    MeReal sx = 0.0, sy = 0.0, sz = 0.0;
    MeReal r, l, ex, ey, ez;
    if (dx > 0.0)
        sx = sqrt(dx);
    if (dy > 0.0)
        sy = sqrt(dy);
    if (dz > 0.0)
        sz = sqrt(dz);
    r = c->radius;
    l = r + c->halfHeight;
    ex = sx * r + fabs(ax) * l;
    ey = sy * r + fabs(ay) * l;
    ez = r * sz + l * fabs(az);
    box[0] = tm[3][0] - ex;
    box[1] = ex + tm[3][0];
    box[2] = tm[3][1] - ey;
    box[3] = ey + tm[3][1];
    box[4] = tm[3][2] - ez;
    box[5] = ez + tm[3][2];
}

/* Mac 0x4f6a54, Win 0x76a430 */
void MEAPI VincCapsuleUpdateAABB(VincGeometryID g, MeMatrix4 tm, MeVector3 min, MeVector3 max, MeReal box[6])
{
    VincCapsuleGetAABB(g, tm, box);
    VincAABBGetMinMax(box, min, max);
}

/* Mac 0x4f6a90, Win 0x76a870 */
VincGeometryID MEAPI VincCapsuleCreate(MeReal radius, MeReal height)
{
    VincGeometryID g = (VincGeometryID)MeMemoryAPI.create(sizeof(VincCapsule));
    if (!g)
        return 0;
    VincGeometryInit(g, VincCapsuleGetTypeId());
    VincCapsuleSetDimensions(g, radius, height);
    return g;
}

/* Mac 0x4f6b03, Win 0x76a830 */
void MEAPI VincCapsuleRegisterType(void)
{
    sCapsuleTypeId = (MeI16)VincGeometryRegisterType(6, sCapsuleName, VincCapsuleDestroy,
                                                     VincCapsuleUpdateAABB, VincCapsuleGetXYAABB,
                                                     VincCapsuleGetBSphere, VincCapsuleMaximumPoint,
                                                     VincCapsuleGetMassProperties);
}

/* Mac 0x4f6b9f, Win 0x76a460 */
void MEAPI VincCapsuleGetXYAABB(VincGeometryID g, MeMatrix4 tm, MeReal box[4])
{
    VincCapsule* c = (VincCapsule*)g;
    MeReal ax = tm[2][0], ay = tm[2][1];
    MeReal dx = 1.0 - ax * ax, dy = 1.0 - ay * ay;
    MeReal r, l, ex, ey;
    if (!(0.0 <= dx))
        dx = 0.0;
    if (!(0.0 <= dy))
        dy = 0.0;
    r = c->radius;
    l = r + c->halfHeight;
    ex = sqrt(dx) * r + l * fabs(ax);
    ey = r * sqrt(dy) + l * fabs(ay);
    box[0] = tm[3][0] - ex;
    box[1] = ex + tm[3][0];
    box[2] = tm[3][1] - ey;
    box[3] = ey + tm[3][1];
}

/* Mac 0x4f6c79, Win 0x76a550: the outermost points of the axis (cap tips) */
void MEAPI VincCapsuleGetEndpoints(VincGeometryID g, MeMatrix4 tm, MeVector3 top, MeVector3 bottom)
{
    VincCapsule* c = (VincCapsule*)g;
    MeReal l;
    top[0] = tm[2][0];
    top[1] = tm[2][1];
    top[2] = tm[2][2];
    l = c->halfHeight + c->radius;
    top[0] = top[0] * l;
    top[1] = top[1] * l;
    top[2] = top[2] * l;
    top[0] = tm[3][0] + top[0];
    top[1] = tm[3][1] + top[1];
    top[2] = tm[3][2] + top[2];
    bottom[0] = tm[2][0];
    bottom[1] = tm[2][1];
    bottom[2] = tm[2][2];
    l = -c->halfHeight - c->radius;
    bottom[0] = bottom[0] * l;
    bottom[1] = bottom[1] * l;
    bottom[2] = bottom[2] * l;
    bottom[0] = tm[3][0] + bottom[0];
    bottom[1] = tm[3][1] + bottom[1];
    bottom[2] = tm[3][2] + bottom[2];
}

/* Mac 0x4f6d70, Win 0x76a600: end-sphere centre on the side of dir, plus radius * dir (dir is
 * expected to be normalised) */
void MEAPI VincCapsuleMaximumPoint(VincGeometryID g, MeMatrix4 tm, const MeVector3 dir, MeVector3 out)
{
    VincCapsule* c = (VincCapsule*)g;
    MeReal r;
    out[0] = tm[3][0];
    out[1] = tm[3][1];
    out[2] = tm[3][2];
    if (tm[2][0] * dir[0] + tm[2][1] * dir[1] + tm[2][2] * dir[2] < 0.0) {
        out[0] = out[0] - c->halfHeight * tm[2][0];
        out[1] = out[1] - c->halfHeight * tm[2][1];
        out[2] = out[2] - c->halfHeight * tm[2][2];
    } else {
        out[0] = c->halfHeight * tm[2][0] + out[0];
        out[1] = out[1] + c->halfHeight * tm[2][1];
        out[2] = out[2] + c->halfHeight * tm[2][2];
    }
    r = c->radius;
    out[0] = r * dir[0] + out[0];
    out[1] = out[1] + r * dir[1];
    out[2] = out[2] + r * dir[2];
}

/* Win 0x76a6c0 (Mac: inlined into its callers): support point in the capsule frame: the end
 * point of the axis on the side of dir + radius * dir / |dir| (just the end point when
 * |dir| <= 1e-4f) */
void MEAPI VincCapsuleMaximumPointLocal(VincGeometryID g, const MeVector3 dir, MeVector3 out)
{
    VincCapsule* c = (VincCapsule*)g;
    MeReal hh = c->halfHeight;
    MeReal len = sqrt(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
    if (len > 9.999999747378752e-05) { /* 1e-4f */
        MeReal s = c->radius / len;
        MeReal h = (dir[2] < 0.0) ? -hh : hh;
        MeReal y = dir[1], z = dir[2];
        out[0] = dir[0] * s;
        out[1] = y * s;
        out[2] = z * s + h;
        return;
    }
    out[0] = 0.0;
    out[1] = 0.0;
    out[2] = (dir[2] < 0.0) ? -hh : hh;
}
