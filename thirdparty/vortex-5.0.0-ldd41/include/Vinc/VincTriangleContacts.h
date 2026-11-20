/* Vortex 5.0.0 build 800 "vortexldd41" - one triangle as the triangle-based pair tests see it,
 * and the primitive / triangle contact generators (restored; the 558a functions
 * in src/Vinc/VincTriangleContacts.c). All names inferred.
 *
 * The callers (triangle list / triangle mesh / height field pair tests) build a VincTriangle per
 * triangle in the frame of the other geometry (or centred on it) and call the generator of that
 * geometry kind; edge data defaults: edgeData0 = -2 (no neighbour information), edgeData1 = 0. */
#ifndef VINC_VINCTRIANGLECONTACTS_H
#define VINC_VINCTRIANGLECONTACTS_H

#include "Vinc/VincIntersect.h"
#include "Vinc/VincMath.h"

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

/*                                    Mac    Win */
typedef struct VincTriangle {
    MeReal* vertices[3];           /* +0x00 +0x00 */
    MeVector3 normal;              /* +0x0c +0x10 unit, (v1 - v0) x (v2 - v1) normalised */
    MeVector3 edges[3];            /* +0x24 +0x28 v1 - v0, v2 - v1, v0 - v2 */
    int index;                     /* +0x6c +0x70 triangle index (-1: none) */
    MeReal edgeData0[3];           /* +0x70 +0x78 per edge (from the triangle source's floats; -2) */
    MeReal edgeData1[3];           /* +0x88 +0x90 per edge (0) */
} VincTriangle;                    /* Mac 0xa0, Win 0xa8 */

/* edges[i] = v[i+1] - v[i] (Win 0x75c1a0, a VincTriangle method there; Mac: inlined) */
VINC_INLINE void VincTriangleUpdateEdges(VincTriangle* t)
{
    const MeReal *a = t->vertices[0], *b = t->vertices[1], *c = t->vertices[2];
    t->edges[0][0] = b[0] - a[0];
    t->edges[0][1] = b[1] - a[1];
    t->edges[0][2] = b[2] - a[2];
    t->edges[1][0] = c[0] - b[0];
    t->edges[1][1] = c[1] - b[1];
    t->edges[1][2] = c[2] - b[2];
    t->edges[2][0] = a[0] - c[0];
    t->edges[2][1] = a[1] - c[1];
    t->edges[2][2] = a[2] - c[2];
}

/* normal = normalised edges[0] x edges[1] (Win 0x75c380 / Mac 0x542085; inlined in 558a) */
VINC_INLINE void VincTriangleUpdateNormal(VincTriangle* t)
{
    const MeReal *e0 = t->edges[0], *e1 = t->edges[1];
    MeReal len;
    t->normal[0] = e0[1] * e1[2] - e0[2] * e1[1];
    t->normal[1] = e0[2] * e1[0] - e1[2] * e0[0];
    t->normal[2] = e0[0] * e1[1] - e0[1] * e1[0];
    len = sqrt(t->normal[0] * t->normal[0] + t->normal[1] * t->normal[1] + t->normal[2] * t->normal[2]);
    if (len != 0.0) {
        MeReal inv = 1.0 / len;
        t->normal[0] = t->normal[0] * inv;
        t->normal[1] = t->normal[1] * inv;
        t->normal[2] = inv * t->normal[2];
    }
}

/* ---- (558a): one source triangle (vertices in world space, index, edge data) against a
 * primitive; the contacts are appended to result->contacts (result->contactCount), the
 * primitive's normals summed into result->normal; return 1 when contacts were added. cdecl. ---- */
/* Mac 0x4fbfc6, Win 0x7cea50 */
int VincTriangleSphereContacts(VincIntersectResult* result, const VincTriangle* src, MeReal radius,
                               const MeReal* center, MeReal pad);
/* Mac 0x4fc3e2, Win 0x7cf150 (capsule axis = z of tm) */
int VincTriangleCapsuleContacts(VincIntersectResult* result, const VincTriangle* src, MeReal radius,
                                MeReal halfHeight, const MeReal* tm, MeReal pad);
/* Mac 0x4fd4d0, Win 0x7cf9c0 */
int VincTrianglePlaneContacts(VincIntersectResult* result, const VincTriangle* src, const MeReal* tm, MeReal pad);
/* Mac 0x4fdaa6, Win 0x7cec40 */
int VincTriangleBoxContacts(VincIntersectResult* result, const VincTriangle* src, const MeReal* radii,
                            const MeReal* tm, MeReal pad);
/* Mac 0x4fe12d, Win 0x7ceed0 */
int VincTriangleCylinderContacts(VincIntersectResult* result, const VincTriangle* src, MeReal radius,
                                 MeReal halfHeight, const MeReal* tm, MeReal tol, MeReal cosTol, int flag);

/* Contact generators of other items, called by the triangle-list pair tests (cdecl on Win):
 * Mac 0x548776 / Win 0x7e7230 (558e): sphere (centre = origin of the triangle's frame,
 * radius *r) against tri; separation, normal (from the triangle to the sphere), contact point,
 * dims (high byte 2: face contact). Returns 1 on contact within *pad. */
int VincSphereTriangleContact(MeReal* sep, MeReal* normal, MeReal* point, MeI16* dims, const MeReal* pad,
                              const MeReal* r, const VincTriangle* tri);

/* Mac 0x545a7e / Win 0x7e76c0 (558e): cylinder (radius *r, half height *hh, axis z of the
 * triangle's frame) against tri: appends contact points to *points and separations to *seps,
 * one shared normal (cylinder frame) and dims; tol = padding / 1000; cosTol / flag from the
 * cylinder geometry (+0x1c / +0x24 Mac). out1 / out2: further outputs not read by the caller. */
int VincCylinderTriangleContacts(MeReal* out1, MeReal* normal, MeReal* out2, MeReal** points, MeI16* dims,
                                 const MeReal* tol, const MeReal* r, const MeReal* hh, const VincTriangle* tri,
                                 MeReal cosTol, int flag, MeReal** seps);

/* Mac 0x5458bd / Win 0x7e53b0: tri (plane frame) against z <= 0: contact points appended
 * to *points, one separation, normal (plane frame) */
int VincPlaneTriangleContact(MeReal* sep, MeReal* normal, MeReal** points, MeI16* dims, MeReal pad,
                             const VincTriangle* tri);
/* Mac 0x547a3e / Win 0x7e9780: box (half extents radii) against tri (box frame) */
int VincBoxTriangleContact(MeReal* out1, MeReal* normal, MeReal* out2, MeReal** points, MeReal** seps,
                           MeI16* dims, const MeReal* pad, const MeReal* radii, const VincTriangle* tri);
/* Mac 0x548dc5 / Win 0x81baa0: capsule segment against one triangle edge a-b (capsule
 * frame), contacts written through *cursor while *count < maxCount */
int VincCapsuleEdgeContacts(const MeReal* a, const MeReal* b, const MeReal* edgeNormal, MeReal halfHeight,
                            MeReal radius, const MeReal* tm, VincContact** cursor, int maxCount, int* count,
                            const MeReal* triNormal);

#ifdef __cplusplus
}
#endif

#endif
