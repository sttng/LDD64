/* Vortex 5.0.0 build 800 "vortexldd41" - Vinc sphere geometry (restored).
 * Mac 0x4f8664-0x4f89d1 (557g), Win 0x774270-0x774510. See include/Vinc/VincPrimitives.h. */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincMath.h"
#include "Me/MeMemory.h"

/* Mac 0x7797e8 / Win 0xabdd00 */
static MeI16 sSphereTypeId = -1;
static const char* sSphereName = "Sphere";

/* Mac 0x4f8664, Win 0x774270 */
MeI16 MEAPI VincSphereGetTypeId(void)
{
    return sSphereTypeId;
}

/* Mac 0x4f8676 (Win: inlined into the Vx layer) */
void MEAPI VincSphereSetRadius(VincGeometryID g, MeReal r)
{
    ((VincSphere*)g)->radius = r;
}

/* Mac 0x4f8688, Win 0x7742d0 */
MeReal MEAPI VincSphereGetRadius(VincGeometryID g)
{
    return ((VincSphere*)g)->radius;
}

/* Mac 0x4f8693, Win 0x7742e0 */
void MEAPI VincSphereDestroy(VincGeometryID g)
{
    if (g)
        MeMemoryAPI.destroy(g);
}

/* Mac 0x4f86b9, Win 0x7743f0 */
void MEAPI VincSphereGetBSphere(VincGeometryID g, MeVector3 center, MeReal* radius)
{
    center[2] = 0.0;
    center[1] = 0.0;
    center[0] = 0.0;
    *radius = ((VincSphere*)g)->radius;
}

/* Mac 0x4f86e2, Win 0x774450 */
int MEAPI VincSphereGetMassProperties(VincGeometryID g, MeMatrix4 relTM, MeMatrix3 inertia, MeReal* volume)
{
    VincSphere* s = (VincSphere*)g;
    MeReal r;
    int i, j;
    for (i = 0; i < 3; ++i)
        for (j = 0; j < 3; ++j)
            inertia[i][j] = 0.0;
    inertia[0][0] = s->radius * 0.4 * s->radius;
    inertia[1][1] = s->radius * 0.4 * s->radius;
    inertia[2][2] = s->radius * 0.4 * s->radius;
    for (i = 0; i < 4; ++i)
        for (j = 0; j < 4; ++j)
            relTM[i][j] = (i == j) ? 1.0 : 0.0;
    r = s->radius;
    *volume = r * 4.1887902047863905 /* 4/3 pi */ * r * r;
    return 1;
}

/* Mac 0x4f87ee, Win 0x774280 */
VincGeometryID MEAPI VincSphereCreate(MeReal radius)
{
    VincGeometryID g = (VincGeometryID)MeMemoryAPI.create(sizeof(VincSphere));
    if (!g)
        return 0;
    VincGeometryInit(g, VincSphereGetTypeId());
    ((VincSphere*)g)->radius = radius;
    return g;
}

/* Mac 0x4f8843, Win 0x774510 */
void MEAPI VincSphereRegisterType(void)
{
    sSphereTypeId = (MeI16)VincGeometryRegisterType(1, sSphereName, VincSphereDestroy,
                                                    VincSphereUpdateAABB, VincSphereGetXYAABB,
                                                    VincSphereGetBSphere, VincSphereMaximumPoint,
                                                    VincSphereGetMassProperties);
}

/* Mac 0x4f88af, Win 0x774410: centre + r * dir (dir is not normalised here) */
void MEAPI VincSphereMaximumPoint(VincGeometryID g, MeMatrix4 tm, const MeVector3 dir, MeVector3 out)
{
    MeReal r = ((VincSphere*)g)->radius;
    out[0] = r * dir[0] + tm[3][0];
    out[1] = r * dir[1] + tm[3][1];
    out[2] = r * dir[2] + tm[3][2];
}

/* Mac 0x4f88fd, Win 0x7743b0 */
void MEAPI VincSphereGetXYAABB(VincGeometryID g, MeMatrix4 tm, MeReal box[4])
{
    MeReal r = ((VincSphere*)g)->radius;
    MeReal x = tm[3][0], y = tm[3][1];
    box[0] = x - r;
    box[1] = x + r;
    box[2] = y - r;
    box[3] = y + r;
}

/* Win 0x774300 (Mac: inlined into VincSphereUpdateAABB) */
void MEAPI VincSphereGetAABB(VincGeometryID g, MeMatrix4 tm, MeReal box[6])
{
    MeReal r = ((VincSphere*)g)->radius;
    box[0] = tm[3][0] - r;
    box[1] = r + tm[3][0];
    box[2] = tm[3][1] - r;
    box[3] = r + tm[3][1];
    box[4] = tm[3][2] - r;
    box[5] = tm[3][2] + r;
}

/* Mac 0x4f8945, Win 0x774350 */
void MEAPI VincSphereUpdateAABB(VincGeometryID g, MeMatrix4 tm, MeVector3 min, MeVector3 max, MeReal box[6])
{
    VincSphereGetAABB(g, tm, box);
    min[0] = box[0];
    max[0] = box[1];
    min[1] = box[2];
    max[1] = box[3];
    min[2] = box[4];
    max[2] = box[5];
}
