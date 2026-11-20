/* Vortex 5.0.0 build 800 "vortexldd41" - Vinc cylinder geometry (restored).
 * Mac 0x4f76df-0x4f7f55 (557g), Win 0x76fbd0-0x770210. Axis = z, centred on the origin. */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincMath.h"
#include "Me/MeMemory.h"

#include <math.h>

/* Mac 0x7797d8 / Win 0xabd8ac */
static MeI16 sCylinderTypeId = -1;
static const char* sCylinderName = "Cylinder";

/* Mac 0x4f76df, Win 0x76fbd0 */
MeI16 MEAPI VincCylinderGetTypeId(void)
{
    return sCylinderTypeId;
}

/* Mac 0x4f76f1, Win 0x76fbe0 */
MeReal MEAPI VincCylinderGetRadius(VincGeometryID g)
{
    return ((VincCylinder*)g)->radius;
}

/* Mac 0x4f76fc, Win 0x76fbf0 */
MeReal MEAPI VincCylinderGetHeight(VincGeometryID g)
{
    return ((VincCylinder*)g)->halfHeight + ((VincCylinder*)g)->halfHeight;
}

/* Mac 0x4f7718, Win 0x76fc00 */
MeReal MEAPI VincCylinderGetHalfHeight(VincGeometryID g)
{
    return ((VincCylinder*)g)->halfHeight;
}

/* Win 0x76feb0 */
MeReal MEAPI VincCylinderGetBoundingRadius(VincGeometryID g)
{
    return ((VincCylinder*)g)->boundingRadius;
}

/* Mac 0x4f7723, Win 0x76fca0 */
void MEAPI VincCylinderDestroy(VincGeometryID g)
{
    if (g)
        MeMemoryAPI.destroy(g);
}

/* Mac 0x4f7749, Win 0x76fe90 */
void MEAPI VincCylinderGetBSphere(VincGeometryID g, MeVector3 center, MeReal* radius)
{
    center[2] = 0.0;
    center[1] = 0.0;
    center[0] = 0.0;
    *radius = ((VincCylinder*)g)->boundingRadius;
}

/* Mac 0x4f7772, Win 0x770100 */
int MEAPI VincCylinderGetMassProperties(VincGeometryID g, MeMatrix4 relTM, MeMatrix3 inertia, MeReal* volume)
{
    VincCylinder* c = (VincCylinder*)g;
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
    *volume = r * 3.141592653589793 * r * h;
    return 1;
}

/* Mac 0x4f7888, Win 0x7701b0 */
void MEAPI VincCylinderSetCosTolerance(VincGeometryID g, MeReal c)
{
    ((VincCylinder*)g)->cosTolerance = c;
}

/* Mac 0x4f789a, Win 0x7701c0 */
MeReal MEAPI VincCylinderGetCosTolerance(VincGeometryID g)
{
    return ((VincCylinder*)g)->cosTolerance;
}

/* Mac 0x4f78a5, Win 0x76fcc0. The extent of the rim along world axis i is r * sqrt(1 - a_i^2)
 * (a = the axis direction, row 2 of tm); clamped at 0. */
void MEAPI VincCylinderGetAABB(VincGeometryID g, MeMatrix4 tm, MeReal box[6])
{
    VincCylinder* c = (VincCylinder*)g;
    MeReal ax = tm[2][0], ay = tm[2][1], az = tm[2][2];
    MeReal dx = 1.0 - ax * ax, dy = 1.0 - ay * ay, dz = 1.0 - az * az;
    MeReal sx = 0.0, sy = 0.0, sz = 0.0;
    MeReal r, hh, ex, ey, ez;
    if (dx > 0.0)
        sx = sqrt(dx);
    if (dy > 0.0)
        sy = sqrt(dy);
    if (dz > 0.0)
        sz = sqrt(dz);
    r = c->radius;
    hh = c->halfHeight;
    ex = sx * r + fabs(ax) * hh;
    ey = sy * r + fabs(ay) * hh;
    ez = r * sz + hh * fabs(az);
    box[0] = tm[3][0] - ex;
    box[1] = ex + tm[3][0];
    box[2] = tm[3][1] - ey;
    box[3] = ey + tm[3][1];
    box[4] = tm[3][2] - ez;
    box[5] = ez + tm[3][2];
}

/* Mac 0x4f79e0, Win 0x76fc70 */
void MEAPI VincCylinderSetDimensions(VincGeometryID g, MeReal radius, MeReal height)
{
    VincCylinder* c = (VincCylinder*)g;
    MeReal hh = height * 0.5;
    c->radius = radius;
    c->halfHeight = hh;
    c->boundingRadius = sqrt(radius * radius + hh * hh);
}

/* Mac 0x4f7a1f, Win 0x76fc40 */
void MEAPI VincCylinderSetHeight(VincGeometryID g, MeReal height)
{
    VincCylinder* c = (VincCylinder*)g;
    MeReal hh = height * 0.5;
    c->halfHeight = hh;
    c->boundingRadius = sqrt(c->radius * c->radius + hh * hh);
}

/* Mac 0x4f7a5d, Win 0x76fc10 */
void MEAPI VincCylinderSetRadius(VincGeometryID g, MeReal radius)
{
    VincCylinder* c = (VincCylinder*)g;
    c->radius = radius;
    c->boundingRadius = sqrt(radius * radius + c->halfHeight * c->halfHeight);
}

/* Mac 0x4f7a89, Win 0x76fdb0 */
void MEAPI VincCylinderUpdateAABB(VincGeometryID g, MeMatrix4 tm, MeVector3 min, MeVector3 max, MeReal box[6])
{
    VincCylinderGetAABB(g, tm, box);
    VincAABBGetMinMax(box, min, max);
}

/* Mac 0x4f7ac5, Win 0x770210 */
VincGeometryID MEAPI VincCylinderCreate(MeReal radius, MeReal height)
{
    VincGeometryID g = (VincGeometryID)MeMemoryAPI.create(sizeof(VincCylinder));
    VincCylinder* c;
    if (!g)
        return 0;
    VincGeometryInit(g, VincCylinderGetTypeId());
    VincCylinderSetDimensions(g, radius, height);
    c = (VincCylinder*)g;
    c->cosTolerance = 0.9999999; /* 0x3fefffffca501acb */
    c->flag = 1;
    return g;
}

/* Mac 0x4f7b4d, Win 0x7701d0 */
void MEAPI VincCylinderRegisterType(void)
{
    sCylinderTypeId = (MeI16)VincGeometryRegisterType(3, sCylinderName, VincCylinderDestroy,
                                                      VincCylinderUpdateAABB, VincCylinderGetXYAABB,
                                                      VincCylinderGetBSphere, VincCylinderMaximumPoint,
                                                      VincCylinderGetMassProperties);
}

/* Mac 0x4f7bb9, Win 0x76fde0 (only x and y; the clamp is written as max(0, 1 - a^2)) */
void MEAPI VincCylinderGetXYAABB(VincGeometryID g, MeMatrix4 tm, MeReal box[4])
{
    VincCylinder* c = (VincCylinder*)g;
    MeReal ax = tm[2][0], ay = tm[2][1];
    MeReal dx = 1.0 - ax * ax, dy = 1.0 - ay * ay;
    MeReal ex, ey;
    if (!(0.0 <= dx))
        dx = 0.0;
    if (!(0.0 <= dy))
        dy = 0.0;
    ex = sqrt(dx) * c->radius + c->halfHeight * fabs(ax);
    ey = c->radius * sqrt(dy) + c->halfHeight * fabs(ay);
    box[0] = tm[3][0] - ex;
    box[1] = ex + tm[3][0];
    box[2] = tm[3][1] - ey;
    box[3] = ey + tm[3][1];
}

/* Mac 0x4f7c8f, Win 0x770070: support point in the cylinder frame (rim point towards dir's xy,
 * top or bottom face by the sign of dir z) */
void MEAPI VincCylinderMaximumPointLocal(VincGeometryID g, const MeVector3 dir, MeVector3 out)
{
    VincCylinder* c = (VincCylinder*)g;
    MeReal hh = c->halfHeight;
    MeReal x = dir[0], y = dir[1];
    MeReal len = sqrt(x * x + y * y);
    if (len <= 9.999999747378752e-05) { /* 1e-4f */
        out[0] = 0.0;
        out[1] = 0.0;
        out[2] = (dir[2] < 0.0) ? -hh : hh;
    } else {
        MeReal s = c->radius / len;
        out[0] = x * s;
        out[1] = y * s;
        out[2] = (dir[2] >= 0.0) ? hh : -hh;
    }
}

/* Mac 0x4f7d65, Win 0x76ff60: support point in world coordinates */
void MEAPI VincCylinderMaximumPoint(VincGeometryID g, MeMatrix4 tm, const MeVector3 dir, MeVector3 out)
{
    VincCylinder* c = (VincCylinder*)g;
    MeReal az, s, d1, d0;
    out[0] = tm[3][0];
    out[1] = tm[3][1];
    out[2] = tm[3][2];
    az = tm[2][0] * dir[0] + tm[2][1] * dir[1] + tm[2][2] * dir[2];
    if (az < 0.0) {
        out[0] = out[0] - c->halfHeight * tm[2][0];
        out[1] = out[1] - c->halfHeight * tm[2][1];
        out[2] = out[2] - c->halfHeight * tm[2][2];
    } else {
        out[0] = c->halfHeight * tm[2][0] + out[0];
        out[1] = out[1] + c->halfHeight * tm[2][1];
        out[2] = out[2] + c->halfHeight * tm[2][2];
    }
    s = 0.0;
    if (1.0 - az * az > 0.0)
        s = (1.0 / sqrt(1.0 - az * az)) * c->radius;
    d1 = s * (tm[1][0] * dir[0] + tm[1][1] * dir[1] + tm[1][2] * dir[2]);
    d0 = (tm[0][0] * dir[0] + tm[0][1] * dir[1] + tm[0][2] * dir[2]) * s;
    out[0] = out[0] + (d1 * tm[1][0] + d0 * tm[0][0]);
    out[1] = out[1] + (d1 * tm[1][1] + d0 * tm[0][1]);
    out[2] = out[2] + (d1 * tm[1][2] + d0 * tm[0][2]);
}

/* Mac 0x4f7f55, Win 0x76fec0: centres of the two end faces */
void MEAPI VincCylinderGetEndpoints(VincGeometryID g, MeMatrix4 tm, MeVector3 top, MeVector3 bottom)
{
    VincCylinder* c = (VincCylinder*)g;
    MeReal h;
    top[0] = tm[2][0];
    top[1] = tm[2][1];
    top[2] = tm[2][2];
    h = c->halfHeight;
    top[0] = top[0] * h;
    top[1] = top[1] * h;
    top[2] = top[2] * h;
    top[0] = tm[3][0] + top[0];
    top[1] = tm[3][1] + top[1];
    top[2] = tm[3][2] + top[2];
    bottom[0] = tm[2][0];
    bottom[1] = tm[2][1];
    bottom[2] = tm[2][2];
    h = -c->halfHeight;
    bottom[0] = bottom[0] * h;
    bottom[1] = bottom[1] * h;
    bottom[2] = bottom[2] * h;
    bottom[0] = tm[3][0] + bottom[0];
    bottom[1] = tm[3][1] + bottom[1];
    bottom[2] = tm[3][2] + bottom[2];
}
