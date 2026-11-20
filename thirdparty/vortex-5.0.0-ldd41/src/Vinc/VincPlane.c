/* Vortex 5.0.0 build 800 "vortexldd41" - Vinc plane geometry (restored).
 * Mac 0x4f8042-0x4f8553 (557g), Win 0x803040-0x8033e0. The plane is z = 0 of its transform,
 * solid below (normal = the transform's z axis, row 2). A NULL transform means the identity. */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincMath.h"
#include "Me/MeMemory.h"

#include <float.h>

/* Mac 0x7797e0 / Win 0xacd0fc */
static MeI16 sPlaneTypeId = -1;
static const char* sPlaneName = "Plane";

/* cos of the largest angle between the normal and a world axis for which the plane counts as
 * axis aligned (bounded on that side only) */
#define VINC_PLANE_AXIS_COS 0.9999995

/* Mac 0x4f8042, Win 0x803040 */
MeI16 MEAPI VincPlaneGetTypeId(void)
{
    return sPlaneTypeId;
}

/* Mac 0x4f8054, Win 0x803080. NOTE: no NULL check (unlike the other types). */
void MEAPI VincPlaneDestroy(VincGeometryID g)
{
    MeMemoryAPI.destroy(g);
}

/* Mac 0x4f8076, Win 0x803090: infinite box except along an axis the normal is parallel to */
void MEAPI VincPlaneGetAABB(VincGeometryID g, MeMatrix4 tm, MeReal box[6])
{
    (void)g;
    box[0] = -DBL_MAX;
    box[1] = DBL_MAX;
    box[2] = -DBL_MAX;
    box[3] = DBL_MAX;
    box[4] = -DBL_MAX;
    if (!tm) {
        box[5] = 0.0; /* NOTE: original quirk (both builds): max z = 0 without a transform */
        return;
    }
    box[5] = DBL_MAX;
    if (tm[2][0] > VINC_PLANE_AXIS_COS)
        box[1] = tm[3][0];
    else if (tm[2][0] < -VINC_PLANE_AXIS_COS)
        box[0] = tm[3][0];
    else if (tm[2][1] > VINC_PLANE_AXIS_COS)
        box[3] = tm[3][1];
    else if (tm[2][1] < -VINC_PLANE_AXIS_COS)
        box[2] = tm[3][1];
    else if (tm[2][2] > VINC_PLANE_AXIS_COS)
        box[5] = tm[3][2];
    else if (tm[2][2] < -VINC_PLANE_AXIS_COS)
        box[4] = tm[3][2];
}

/* Mac 0x4f82a2, Win 0x8032a0 */
void MEAPI VincPlaneGetBSphere(VincGeometryID g, MeVector3 center, MeReal* radius)
{
    (void)g;
    center[2] = 0.0;
    center[1] = 0.0;
    center[0] = 0.0;
    *radius = DBL_MAX;
}

/* Mac 0x4f82cc, Win 0x803330: identity inertia, volume 1 */
int MEAPI VincPlaneGetMassProperties(VincGeometryID g, MeMatrix4 relTM, MeMatrix3 inertia, MeReal* volume)
{
    int i, j;
    (void)g;
    for (i = 0; i < 3; ++i)
        for (j = 0; j < 3; ++j)
            inertia[i][j] = (i == j) ? 1.0 : 0.0;
    for (i = 0; i < 4; ++i)
        for (j = 0; j < 4; ++j)
            relTM[i][j] = (i == j) ? 1.0 : 0.0;
    *volume = 1.0;
    return 1;
}

/* Mac 0x4f8383, Win 0x8033a0 */
void MEAPI VincPlaneGetNormal(VincGeometryID g, MeMatrix4 tm, MeVector3 normal)
{
    (void)g;
    if (!tm) {
        normal[0] = 0.0;
        normal[1] = 0.0;
        normal[2] = 1.0;
    } else {
        normal[0] = tm[2][0];
        normal[1] = tm[2][1];
        normal[2] = tm[2][2];
    }
}

/* Mac 0x4f83d1, Win 0x803250 */
void MEAPI VincPlaneGetXYAABB(VincGeometryID g, MeMatrix4 tm, MeReal box[4])
{
    MeReal b[6];
    MeVector3 min, max;
    VincPlaneGetAABB(g, tm, b);
    VincAABBGetMinMax(b, min, max);
    box[0] = min[0];
    box[1] = max[0];
    box[2] = min[1];
    box[3] = max[1];
}

/* Mac 0x4f8439, Win 0x803220 */
void MEAPI VincPlaneUpdateAABB(VincGeometryID g, MeMatrix4 tm, MeVector3 min, MeVector3 max, MeReal box[6])
{
    VincPlaneGetAABB(g, tm, box);
    VincAABBGetMinMax(box, min, max);
}

/* Mac 0x4f8475, Win 0x803440 */
void MEAPI VincPlaneRegisterType(void)
{
    sPlaneTypeId = (MeI16)VincGeometryRegisterType(0, sPlaneName, VincPlaneDestroy, VincPlaneUpdateAABB,
                                                   VincPlaneGetXYAABB, VincPlaneGetBSphere,
                                                   VincPlaneMaximumPoint, VincPlaneGetMassProperties);
}

/* Mac 0x4f84e1, Win 0x8033e0: signed distance of p above the plane */
MeReal MEAPI VincPlaneGetDistance(VincGeometryID g, MeMatrix4 tm, const MeVector3 p)
{
    MeVector3 n;
    MeReal x, y, z;
    VincPlaneGetNormal(g, tm, n);
    x = p[0];
    y = p[1];
    z = p[2];
    if (tm) {
        x = x - tm[3][0];
        y = y - tm[3][1];
        z = z - tm[3][2];
    }
    return x * n[0] + y * n[1] + z * n[2];
}

/* Mac 0x4f8553, Win 0x8032c0: a point on the plane when dir is (almost) the inward normal, else
 * dir * DBL_MAX. NOTE: a NaN dot product gives the plane origin on Mac and dir * DBL_MAX on Win
 * (comparison written in opposite senses); the Mac form is kept. */
void MEAPI VincPlaneMaximumPoint(VincGeometryID g, MeMatrix4 tm, const MeVector3 dir, MeVector3 out)
{
    (void)g;
    if (tm[2][0] * dir[0] + tm[2][1] * dir[1] + tm[2][2] * dir[2] < VINC_PLANE_AXIS_COS) {
        MeReal y = dir[1], z = dir[2];
        out[0] = dir[0] * DBL_MAX;
        out[1] = y * DBL_MAX;
        out[2] = z * DBL_MAX;
    } else {
        out[0] = tm[3][0];
        out[1] = tm[3][1];
        out[2] = tm[3][2];
    }
}
