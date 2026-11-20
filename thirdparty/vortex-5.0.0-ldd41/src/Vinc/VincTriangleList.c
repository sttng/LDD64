/* Vortex 5.0.0 build 800 "vortexldd41" - Vinc triangle list geometry (restored).
 * Mac 0x4f8a18-0x4f8f47 (557g), Win 0x7ad530-0x7adbf0. The geometry is a local box
 * (min/max); its triangles come from the generator callback when a pair test asks for them.
 * Create / dimension setters exist on Win only (Mac: dead-stripped). */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincMath.h"
#include "Me/MeMemory.h"

#include <math.h>

/* Mac 0x7797f0 / Win 0xac7e6c */
static MeI16 sTriangleListTypeId = -1;
static const char* sTriangleListName = "TriangleList";

/* Mac 0x4f8a18, Win 0x7ad530 */
MeI16 MEAPI VincTriangleListGetTypeId(void)
{
    return sTriangleListTypeId;
}

/* Mac 0x4f8a2a, Win 0x7ad690 */
void MEAPI VincTriangleListDestroy(VincGeometryID g)
{
    if (g)
        MeMemoryAPI.destroy(g);
}

/* Mac 0x4f8a50, Win 0x7ad6b0 */
void MEAPI VincTriangleListGetAABB(VincGeometryID g, MeMatrix4 tm, MeReal box[6])
{
    VincTriangleList* t = (VincTriangleList*)g;
    const MeReal* m = &tm[0][0];
    MeReal r0 = t->radii[0], r1 = t->radii[1], r2 = t->radii[2];
    MeReal ex = r0 * fabs(m[0]) + r1 * fabs(m[4]) + r2 * fabs(m[8]);
    MeReal ey = r0 * fabs(m[1]) + r1 * fabs(m[5]) + r2 * fabs(m[9]);
    MeReal ez = r0 * fabs(m[2]) + r1 * fabs(m[6]) + r2 * fabs(m[10]);
    MeVector3 c, wc;
    c[0] = r0 + t->min[0];
    c[1] = r1 + t->min[1];
    c[2] = r2 + t->min[2];
    VincTMTransform(m, c, wc);
    box[0] = wc[0] - ex;
    box[1] = ex + wc[0];
    box[2] = wc[1] - ey;
    box[3] = ey + wc[1];
    box[4] = wc[2] - ez;
    box[5] = ez + wc[2];
}

/* Mac 0x4f8bee, Win 0x7ad7d0. NOTE: original formula kept: min/max corners projected with the
 * absolute rotation (not a true xy bound of the rotated box). */
void MEAPI VincTriangleListGetXYAABB(VincGeometryID g, MeMatrix4 tm, MeReal box[4])
{
    VincTriangleList* t = (VincTriangleList*)g;
    const MeReal* m = &tm[0][0];
    MeReal a0 = t->min[0], a1 = t->min[1], a2 = t->min[2];
    box[0] = a0 * fabs(m[0]) + a1 * fabs(m[4]) + a2 * fabs(m[8]) + m[12];
    box[2] = a0 * fabs(m[1]) + a1 * fabs(m[5]) + a2 * fabs(m[9]) + m[13];
    a0 = t->max[0];
    a1 = t->max[1];
    a2 = t->max[2];
    box[1] = a0 * fabs(m[0]) + a1 * fabs(m[4]) + a2 * fabs(m[8]) + m[12];
    box[3] = a0 * fabs(m[1]) + a1 * fabs(m[5]) + a2 * fabs(m[9]) + m[13];
}

/* Mac 0x4f8d2f, Win 0x7ad870 */
void MEAPI VincTriangleListGetBSphere(VincGeometryID g, MeVector3 center, MeReal* radius)
{
    VincTriangleList* t = (VincTriangleList*)g;
    center[0] = (t->max[0] + t->min[0]) * 0.5;
    center[1] = (t->max[1] + t->min[1]) * 0.5;
    center[2] = (t->max[2] + t->min[2]) * 0.5;
    *radius = t->boundingRadius;
}

/* Mac 0x4f8d8c, Win 0x7ad990. NOTE: volume = r0 r1 r2 (the half extents, not 8x), kept. */
int MEAPI VincTriangleListGetMassProperties(VincGeometryID g, MeMatrix4 relTM, MeMatrix3 inertia, MeReal* volume)
{
    VincTriangleList* t = (VincTriangleList*)g;
    MeReal r0 = t->radii[0], r1 = t->radii[1], r2 = t->radii[2];
    int i, j;
    for (i = 0; i < 3; ++i)
        for (j = 0; j < 3; ++j)
            inertia[i][j] = 0.0;
    inertia[0][0] = (r1 * r1 + r2 * r2) * 0.3333333333333333;
    inertia[1][1] = (r2 * r2 + r0 * r0) * 0.3333333333333333;
    inertia[2][2] = (r1 * r1 + r0 * r0) * 0.3333333333333333;
    for (i = 0; i < 4; ++i)
        for (j = 0; j < 4; ++j)
            relTM[i][j] = (i == j) ? 1.0 : 0.0;
    *volume = r0 * r1 * r2;
    return 1;
}

/* Mac 0x4f8e9f, Win 0x7ad7a0 */
void MEAPI VincTriangleListUpdateAABB(VincGeometryID g, MeMatrix4 tm, MeVector3 min, MeVector3 max, MeReal box[6])
{
    VincTriangleListGetAABB(g, tm, box);
    VincAABBGetMinMax(box, min, max);
}

/* Mac 0x4f8edb, Win 0x7adb20 */
void MEAPI VincTriangleListRegisterType(void)
{
    sTriangleListTypeId = (MeI16)VincGeometryRegisterType(5, sTriangleListName, VincTriangleListDestroy,
                                                          VincTriangleListUpdateAABB,
                                                          VincTriangleListGetXYAABB,
                                                          VincTriangleListGetBSphere,
                                                          VincTriangleListMaximumPoint,
                                                          VincTriangleListGetMassProperties);
}

/* Mac 0x4f8f47, Win 0x7ad8b0. NOTE: sign(dir) * max corner (not the box corner on the dir side
 * of min/max), as in the original. */
void MEAPI VincTriangleListMaximumPoint(VincGeometryID g, MeMatrix4 tm, const MeVector3 dir, MeVector3 out)
{
    VincTriangleList* t = (VincTriangleList*)g;
    const MeReal* m = &tm[0][0];
    MeVector3 l, s;
    VincTMInverseRotate(m, dir, l);
    s[2] = VincSign(&l[2]) * t->max[2];
    s[1] = VincSign(&l[1]) * t->max[1];
    s[0] = VincSign(&l[0]) * t->max[0];
    VincTMTransform(m, s, out);
}

/* ---- Win-only API (Mac: dead-stripped) ---- */

/* Win 0x7ad540: box of size dx, dy, dz centred on the origin */
void MEAPI VincTriangleListSetDimensions(VincGeometryID g, MeReal dx, MeReal dy, MeReal dz)
{
    VincTriangleList* t = (VincTriangleList*)g;
    MeReal rx = dx * 0.5, ry = dy * 0.5, rz = dz * 0.5;
    t->radii[0] = rx;
    t->radii[1] = ry;
    t->radii[2] = rz;
    t->boundingRadius = sqrt(rx * rx + ry * ry + rz * rz);
    t->max[0] = rx;
    t->max[1] = ry;
    t->max[2] = rz;
    t->min[0] = -rx;
    t->min[1] = -ry;
    t->min[2] = -rz;
}

/* Win 0x7ad5c0 */
void MEAPI VincTriangleListSetBounds(VincGeometryID g, const MeVector3 min, const MeVector3 max)
{
    VincTriangleList* t = (VincTriangleList*)g;
    t->max[0] = max[0];
    t->min[0] = min[0];
    t->radii[0] = (t->max[0] - min[0]) * 0.5;
    t->max[1] = max[1];
    t->min[1] = min[1];
    t->radii[1] = (t->max[1] - min[1]) * 0.5;
    t->max[2] = max[2];
    t->min[2] = min[2];
    t->radii[2] = (t->max[2] - min[2]) * 0.5;
    t->boundingRadius = sqrt(t->radii[0] * t->radii[0] + t->radii[1] * t->radii[1] +
                             t->radii[2] * t->radii[2]);
}

/* Win 0x7ad640 */
void MEAPI VincTriangleListGetDimensions(VincGeometryID g, MeReal* dx, MeReal* dy, MeReal* dz)
{
    VincTriangleList* t = (VincTriangleList*)g;
    *dx = t->max[0] - t->min[0];
    *dy = t->max[1] - t->min[1];
    *dz = t->max[2] - t->min[2];
}

/* Win 0x7ad670 */
MeReal MEAPI VincTriangleListGetBoundingRadius(VincGeometryID g)
{
    return ((VincTriangleList*)g)->boundingRadius;
}

/* Win 0x7ad680 */
MeReal* MEAPI VincTriangleListGetRadii(VincGeometryID g)
{
    return ((VincTriangleList*)g)->radii;
}

/* Win 0x7ada40 */
void* MEAPI VincTriangleListGetM6c(VincGeometryID g)
{
    return ((VincTriangleList*)g)->m6c;
}

/* Win 0x7ada50 */
void MEAPI VincTriangleListSetM6c(VincGeometryID g, void* p)
{
    ((VincTriangleList*)g)->m6c = p;
}

/* Win 0x7ada60 */
void MEAPI VincTriangleListSetTriangles(VincGeometryID g, VincTriangleListTri* tri, int count, int maxCount)
{
    VincTriangleList* t = (VincTriangleList*)g;
    t->triangles = tri;
    t->triangleCount = count;
    t->triangleMaxCount = maxCount;
}

/* Win 0x7ada80 */
void MEAPI VincTriangleListGetTriangles(VincGeometryID g, VincTriangleListTri** tri, int* count, int* maxCount)
{
    VincTriangleList* t = (VincTriangleList*)g;
    *tri = t->triangles;
    *count = t->triangleCount;
    *maxCount = t->triangleMaxCount;
}

/* Win 0x7adae0 */
void MEAPI VincTriangleListSetSphereGenerator(VincGeometryID g, VincTriangleListSphereGenerator gen)
{
    ((VincTriangleList*)g)->sphereGenerator = gen;
}

/* Win 0x7adaf0 */
void MEAPI VincTriangleListSetGenerator(VincGeometryID g, VincTriangleListGenerator gen)
{
    ((VincTriangleList*)g)->generator = gen;
}

/* Win 0x7adb00 */
void MEAPI VincTriangleListSetGetTriangleFn(VincGeometryID g, VincTriangleListGetTriangleFn fn)
{
    ((VincTriangleList*)g)->getTriangle = fn;
}

/* Win 0x7adb60: NULL without a sphere generator. NOTE: lineSegmentGenerator (+0x80) stays
 * uninitialised (as in the original). */
VincGeometryID MEAPI VincTriangleListCreate(MeReal dx, MeReal dy, MeReal dz, VincTriangleListSphereGenerator sphereGen)
{
    VincGeometryID g;
    VincTriangleList* t;
    if (sphereGen == 0)
        return 0;
    g = (VincGeometryID)MeMemoryAPI.create(sizeof(VincTriangleList));
    if (!g)
        return 0;
    VincGeometryInit(g, sTriangleListTypeId);
    VincTriangleListSetDimensions(g, dx, dy, dz);
    t = (VincTriangleList*)g;
    t->triangleCount = 0;
    t->stepLength = 0.0;
    t->triangles = 0;
    t->triangleMaxCount = 0;
    t->sphereGenerator = sphereGen;
    t->generator = 0;
    t->getTriangle = 0;
    t->m6c = 0;
    t->m68 = 0;
    t->m70 = -1;
    return g;
}

/* Win 0x7adbf0. NOTE: no NULL check of the create result (as in the original). */
VincGeometryID MEAPI VincTriangleListCreateWithGenerator(MeReal dx, MeReal dy, MeReal dz, VincTriangleListSphereGenerator sphereGen,
                                                   VincTriangleListGenerator gen,
                                                   VincTriangleListGenerator lineSegmentGen)
{
    VincGeometryID g = VincTriangleListCreate(dx, dy, dz, sphereGen);
    ((VincTriangleList*)g)->generator = gen;
    ((VincTriangleList*)g)->lineSegmentGenerator = lineSegmentGen;
    return g;
}
