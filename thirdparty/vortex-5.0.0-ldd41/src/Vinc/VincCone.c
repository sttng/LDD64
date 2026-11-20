/* Vortex 5.0.0 build 800 "vortexldd41" - Vinc cone geometry (restored).
 * Mac 0x4f6ea7-0x4f76a3 (557g), Win 0x7b7560-0x7b7aa0. Axis = z; the origin is the centre of
 * mass (apex at +top = 3/4 h, base disc at -bottom = 1/4 h). Create / Init are Win only (dead-
 * stripped on Mac). */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincMath.h"
#include "Me/MeMemory.h"

#include <math.h>

/* Mac 0x7797d0 / Win 0xac7f74 */
static MeI16 sConeTypeId = -1;
static const char* sConeName = "Cone";

/* Mac 0x4f6ea7, Win 0x7b7560 */
MeI16 MEAPI VincConeGetTypeId(void)
{
    return sConeTypeId;
}

/* Mac 0x4f6eb9 */
MeReal MEAPI VincConeGetBottom(VincGeometryID g)
{
    return ((VincCone*)g)->bottom;
}

/* Mac 0x4f6ec4 */
MeReal MEAPI VincConeGetTop(VincGeometryID g)
{
    return ((VincCone*)g)->top;
}

/* Mac 0x4f6ecf */
MeReal MEAPI VincConeGetHeight(VincGeometryID g)
{
    return ((VincCone*)g)->top + ((VincCone*)g)->bottom;
}

/* Mac 0x4f6eec */
MeReal MEAPI VincConeGetRadius(VincGeometryID g)
{
    return ((VincCone*)g)->radius;
}

/* Mac 0x4f6ef7, Win 0x7b7610 */
void MEAPI VincConeDestroy(VincGeometryID g)
{
    if (g)
        MeMemoryAPI.destroy(g);
}

/* Mac 0x4f6f1d, Win 0x7b7900: the sphere through the apex and the base rim, centred on the axis */
void MEAPI VincConeGetBSphere(VincGeometryID g, MeVector3 center, MeReal* radius)
{
    VincCone* c = (VincCone*)g;
    MeReal a = c->top, b = c->bottom;
    MeReal h = a + b;
    MeReal z = ((a * a - b * b) - c->radius * c->radius) / (h + h);
    *radius = a - z;
    center[1] = 0.0;
    center[0] = 0.0;
    center[2] = z;
}

/* Mac 0x4f6f80, Win 0x7b7950 */
int MEAPI VincConeGetMassProperties(VincGeometryID g, MeMatrix4 relTM, MeMatrix3 inertia, MeReal* volume)
{
    VincCone* c = (VincCone*)g;
    MeReal r = c->radius;
    MeReal h = VincConeGetHeight(g);
    MeReal ixx;
    int i, j;
    *volume = (r * 3.141592653589793 * r * h) / 3.0;
    for (i = 0; i < 3; ++i)
        for (j = 0; j < 3; ++j)
            inertia[i][j] = 0.0;
    ixx = (r * 4.0 * r + h * h) * 0.0375;
    inertia[0][0] = ixx;
    inertia[1][1] = ixx;
    inertia[2][2] = r * 0.3 * r;
    for (i = 0; i < 4; ++i)
        for (j = 0; j < 4; ++j)
            relTM[i][j] = (i == j) ? 1.0 : 0.0;
    return 1;
}

/* Mac 0x4f70ac, Win 0x7b7a10 */
void MEAPI VincConeRegisterType(void)
{
    sConeTypeId = (MeI16)VincGeometryRegisterType(4, sConeName, VincConeDestroy, VincConeUpdateAABB,
                                                  VincConeGetXYAABB, VincConeGetBSphere,
                                                  VincConeMaximumPoint, VincConeGetMassProperties);
}

/* Mac 0x4f7118, Win 0x7b7630 (Win calls the out-of-line transform 0x759940) */
void MEAPI VincConeMaximumPoint(VincGeometryID g, MeMatrix4 tm, const MeVector3 dir, MeVector3 out)
{
    VincCone* c = (VincCone*)g;
    const MeReal* m = &tm[0][0];
    MeVector3 local;
    MeReal d0 = dir[0], d1 = dir[1], d2 = dir[2];
    MeReal az = d0 * m[8] + d1 * m[9] + d2 * m[10];
    if (c->sinHalfAngle < az) {
        local[0] = 0.0;
        local[1] = 0.0;
        local[2] = c->top;
    } else {
        MeReal lx = d0 * m[0] + d1 * m[1] + d2 * m[2];
        MeReal ly = d0 * m[4] + d1 * m[5] + d2 * m[6];
        if (fabs(lx) < 1e-14 && fabs(ly) < 1e-14) {
            local[0] = 0.0;
            local[1] = 0.0;
        } else {
            MeReal len = sqrt(lx * lx + ly * ly + 0.0 * 0.0);
            if (len != 0.0) {
                MeReal inv = 1.0 / len;
                lx = lx * inv;
                ly = inv * ly;
            }
            local[0] = c->radius * lx;
            local[1] = c->radius * ly;
        }
        local[2] = -c->bottom;
    }
    VincTMTransform(m, local, out);
}

/* Mac 0x4f743f, Win 0x7b7780: support points along the six world axis directions */
void MEAPI VincConeGetAABB(VincGeometryID g, MeMatrix4 tm, MeReal box[6])
{
    MeVector3 dir, p;
    dir[0] = -1.0; dir[1] = 0.0; dir[2] = 0.0;
    VincConeMaximumPoint(g, tm, dir, p);
    box[0] = p[0];
    dir[0] = 1.0; dir[1] = 0.0; dir[2] = 0.0;
    VincConeMaximumPoint(g, tm, dir, p);
    box[1] = p[0];
    dir[0] = 0.0; dir[1] = -1.0; dir[2] = 0.0;
    VincConeMaximumPoint(g, tm, dir, p);
    box[2] = p[1];
    dir[0] = 0.0; dir[1] = 1.0; dir[2] = 0.0;
    VincConeMaximumPoint(g, tm, dir, p);
    box[3] = p[1];
    dir[0] = 0.0; dir[1] = 0.0; dir[2] = -1.0;
    VincConeMaximumPoint(g, tm, dir, p);
    box[4] = p[2];
    dir[0] = 0.0; dir[1] = 0.0; dir[2] = 1.0;
    VincConeMaximumPoint(g, tm, dir, p);
    box[5] = p[2];
}

/* Mac 0x4f763b, Win 0x7b78b0 */
void MEAPI VincConeGetXYAABB(VincGeometryID g, MeMatrix4 tm, MeReal box[4])
{
    MeReal b[6];
    MeVector3 min, max;
    VincConeGetAABB(g, tm, b);
    VincAABBGetMinMax(b, min, max);
    box[0] = min[0];
    box[1] = max[0];
    box[2] = min[1];
    box[3] = max[1];
}

/* Mac 0x4f76a3, Win 0x7b7880 */
void MEAPI VincConeUpdateAABB(VincGeometryID g, MeMatrix4 tm, MeVector3 min, MeVector3 max, MeReal box[6])
{
    VincConeGetAABB(g, tm, box);
    VincAABBGetMinMax(box, min, max);
}

/* Win 0x7b7570 (Mac: dead-stripped) */
void MEAPI VincConeInit(VincGeometryID g, MeReal height, MeReal radius, MeReal user)
{
    VincCone* c = (VincCone*)g;
    c->radius = radius;
    c->user = user;
    c->bottom = height * 0.25;
    c->top = height * 0.75;
    if (height > 0.0)
        c->sinHalfAngle = radius / sqrt(height * height + radius * radius);
    else
        c->sinHalfAngle = 1.0;
}

/* Win 0x7b7a50 (Mac: dead-stripped) */
VincGeometryID MEAPI VincConeCreate(MeReal height, MeReal radius, MeReal user)
{
    VincGeometryID g = (VincGeometryID)MeMemoryAPI.create(sizeof(VincCone));
    if (!g)
        return 0;
    VincGeometryInit(g, sConeTypeId);
    VincConeInit(g, height, radius, user);
    return g;
}

/* Win 0x7b7aa0 (Mac: dead-stripped) */
VincGeometryID MEAPI VincConeCreateSimple(MeReal height, MeReal radius)
{
    VincGeometryID g = (VincGeometryID)MeMemoryAPI.create(sizeof(VincCone));
    if (!g)
        return 0;
    VincGeometryInit(g, sConeTypeId);
    VincConeInit(g, height, radius, 0.0);
    return g;
}
