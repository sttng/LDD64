/* Vortex 5.0.0 build 800 "vortexldd41" - one triangle against a primitive, appending world-space
 * contacts to an intersect result (restored). Mac 0x4fbfc6-0x4fe7e4 (558a),
 * Win 0x7cea50-0x7cf9c0. Callers: the triangle mesh / height field pair tests. All names
 * (name inferred). Each copies the triangle into a local VincTriangle in the primitive's frame,
 * calls the contact generator and appends its points in world space while contactCount <
 * contactMaxCount, adding the world normal to result->normal. The originals return a bool (Win sets
 * only AL: EAX's upper bytes are garbage on the "no contact" paths); these return 0 / 1. cdecl. */
#include "Vinc/VincTriangleContacts.h"
#include "Vinc/VincPairTests.h"
#include "Vinc/VincMath.h"

#include <math.h>
#include <string.h>

/* the local copy: vertices point at caller-provided storage; edge data from src (copyEdgeData) or
 * the defaults (-2 / 0) */
static void TriangleInit(VincTriangle* tri, MeReal (*v)[3], const VincTriangle* src, int copyEdgeData)
{
    int i;
    tri->index = -1;
    for (i = 0; i < 3; ++i) {
        tri->vertices[i] = v[i];
        tri->edgeData0[i] = -2.0;
        tri->edgeData1[i] = 0.0;
    }
    if (copyEdgeData) {
        for (i = 0; i < 3; ++i) {
            tri->edgeData0[i] = src->edgeData0[i];
            tri->edgeData1[i] = src->edgeData1[i];
        }
    }
}

/* Mac 0x4fbfc6, Win 0x7cea50: the triangle centred on the sphere; one contact (element1 = the
 * triangle index, element2 not written) */
int VincTriangleSphereContacts(VincIntersectResult* result, const VincTriangle* src, MeReal radius,
                               const MeReal* center, MeReal pad)
{
    VincTriangle tri;
    MeReal v[3][3];
    MeReal sep;
    MeVector3 n, p;
    MeI16 dims;
    VincContact* c;
    int i;
    TriangleInit(&tri, v, src, 1);
    for (i = 0; i < 3; ++i) {
        v[i][0] = src->vertices[i][0] - center[0];
        v[i][1] = src->vertices[i][1] - center[1];
        v[i][2] = src->vertices[i][2] - center[2];
    }
    VincTriangleUpdateEdges(&tri);
    VincTriangleUpdateNormal(&tri);
    if (!VincSphereTriangleContact(&sep, n, p, &dims, &pad, &radius, &tri))
        return 0;
    if (!(result->contactCount < result->contactMaxCount))
        return 0;
    c = &result->contacts[result->contactCount];
    c->position[0] = center[0] + p[0];
    c->position[1] = center[1] + p[1];
    c->position[2] = center[2] + p[2];
    c->normal[0] = n[0];
    c->normal[1] = n[1];
    c->normal[2] = n[2];
    c->separation = sep;
    c->dims = dims;
    c->element1 = src->index;
    result->normal[0] = result->normal[0] + n[0];
    result->normal[1] = result->normal[1] + n[1];
    result->normal[2] = result->normal[2] + n[2];
    ++result->contactCount;
    return 1;
}

/* Win 0xac96b8 / Mac: the next vertex of each edge */
static const int kNextVertex[3] = {1, 2, 0};

/* Mac 0x4fc3e2, Win 0x7cf150: capsule (radius, half height along the z axis of tm). The two end
 * spheres are tested first (element1 -1 / 1); unless both touch, the vertices against the side,
 * the edges against the segment (VincCapsuleEdgeContacts) and, when neither added anything, the
 * segment through the face. NOTE: the original writes the cap contacts, the scratch separation of
 * the vertex test, the scratch element2 of the edge test and the face contact without checking
 * contactMaxCount (past the end of the buffer when it is full); guarded here (UB). */
int VincTriangleCapsuleContacts(VincIntersectResult* result, const VincTriangle* src, MeReal radius,
                                MeReal halfHeight, const MeReal* tm, MeReal pad)
{
    VincTriangle tri;
    MeReal v[3][3];
    MeVector3 top, bottom, n, p, d;
    MeReal sep, len;
    MeI16 dims;
    VincContact* cur;
    int start, caps, i;
    TriangleInit(&tri, v, src, 1);
    top[0] = tm[12] + tm[8] * halfHeight;
    top[1] = tm[13] + tm[9] * halfHeight;
    top[2] = tm[14] + tm[10] * halfHeight;
    bottom[0] = tm[12] - halfHeight * tm[8];
    bottom[1] = tm[13] - tm[9] * halfHeight;
    bottom[2] = tm[14] - tm[10] * halfHeight;
    for (i = 0; i < 3; ++i) {
        v[i][0] = src->vertices[i][0] - top[0];
        v[i][1] = src->vertices[i][1] - top[1];
        v[i][2] = src->vertices[i][2] - top[2];
    }
    VincTriangleUpdateEdges(&tri);
    VincTriangleUpdateNormal(&tri);
    start = result->contactCount;
    cur = &result->contacts[start];
    caps = 0;
    if (VincSphereTriangleContact(&sep, n, p, &dims, &pad, &radius, &tri)
        && 0.0 < n[2] * tri.normal[2] + n[0] * tri.normal[0] + n[1] * tri.normal[1]
        && result->contactCount < result->contactMaxCount) {
        caps = 1;
        cur->position[0] = p[0] + top[0];
        cur->position[1] = p[1] + top[1];
        cur->position[2] = p[2] + top[2];
        cur->normal[0] = n[0];
        cur->normal[1] = n[1];
        cur->normal[2] = n[2];
        cur->separation = sep;
        cur->dims = dims;
        cur->element1 = -1;
        cur->element2 = src->index;
        ++result->contactCount;
        ++cur;
    }
    /* the same triangle relative to the bottom end (edges and normal unchanged) */
    for (i = 0; i < 3; ++i) {
        v[i][0] = src->vertices[i][0] - bottom[0];
        v[i][1] = src->vertices[i][1] - bottom[1];
        v[i][2] = src->vertices[i][2] - bottom[2];
    }
    if (VincSphereTriangleContact(&sep, n, p, &dims, &pad, &radius, &tri)
        && 0.0 < n[2] * tri.normal[2] + n[0] * tri.normal[0] + n[1] * tri.normal[1]
        && result->contactCount < result->contactMaxCount) {
        cur->position[0] = p[0] + bottom[0];
        cur->position[1] = p[1] + bottom[1];
        cur->position[2] = p[2] + bottom[2];
        cur->normal[0] = n[0];
        cur->normal[1] = n[1];
        cur->normal[2] = n[2];
        cur->separation = sep;
        cur->dims = dims;
        cur->element1 = 1;
        cur->element2 = src->index;
        ++result->contactCount;
        ++cur;
        ++caps;
    }
    if (caps == 2)
        return 1; /* NOTE: result->normal is not updated on this path */

    /* the triangle in the capsule's frame */
    for (i = 0; i < 3; ++i)
        VincTMInverseTransform(tm, src->vertices[i], v[i]);
    VincTriangleUpdateEdges(&tri);
    VincTriangleUpdateNormal(&tri);
    /* vertices within the side's height against the side */
    for (i = 0; i < 3; ++i) {
        const MeReal* q = tri.vertices[i];
        if (!(fabs(q[2]) < halfHeight))
            continue;
        d[0] = -q[0];
        d[1] = -q[1];
        d[2] = 0.0;
        len = sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
        if (0.0 != len) {
            MeReal inv = 1.0 / len;
            d[0] = d[0] * inv;
            d[1] = d[1] * inv;
            d[2] = inv * 0.0;
        }
        if (!(result->contactCount < result->contactMaxCount))
            continue; /* (guard, see above) */
        cur->separation = len - radius; /* written into the free slot even when not used */
        if (cur->separation < 0.0 && 0.0 < len
            && 0.0 < d[2] * tri.normal[2] + d[1] * tri.normal[1] + d[0] * tri.normal[0]) {
            cur->position[0] = src->vertices[i][0];
            cur->position[1] = src->vertices[i][1];
            cur->position[2] = src->vertices[i][2];
            VincTMRotate(tm, d, cur->normal);
            cur->dims = 0;
            cur->element1 = 0;
            cur->element2 = src->index;
            ++result->contactCount;
            ++cur;
        }
    }
    /* edges (convex or without neighbour information) against the segment */
    for (i = 0; i < 3; ++i) {
        const MeReal* e = tri.edges[i];
        if (!(src->edgeData0[i] == -2.0 || 0.0 < src->edgeData1[i]))
            continue;
        d[0] = tri.normal[1] * e[2] - e[1] * tri.normal[2];
        d[1] = tri.normal[2] * e[0] - tri.normal[0] * e[2];
        d[2] = e[1] * tri.normal[0] - tri.normal[1] * e[0];
        len = sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
        if (0.0 != len) {
            MeReal inv = 1.0 / len;
            d[0] = d[0] * inv;
            d[1] = d[1] * inv;
            d[2] = inv * d[2];
        }
        if (0.0 < len) {
            if (result->contactCount < result->contactMaxCount) /* (guard, see above) */
                cur->element2 = tri.index; /* -1 */
            VincCapsuleEdgeContacts(tri.vertices[i], tri.vertices[kNextVertex[i]], d, halfHeight, radius, tm,
                                    &cur, result->contactMaxCount, &result->contactCount, tri.normal);
        }
    }
    /* nothing but end contacts: the segment through the face */
    if (result->contactCount - caps == start) {
        MeVector3 p0, p1, dir, hit;
        MeReal segLen = halfHeight + halfHeight, tBest = 1.0, dist;
        p0[0] = 0.0;
        p0[1] = 0.0;
        p0[2] = -halfHeight;
        p1[0] = 0.0;
        p1[1] = 0.0;
        p1[2] = halfHeight;
        dir[0] = 0.0;
        dir[1] = 0.0;
        dir[2] = segLen;
        if (VincLineSegmentTriangle(tri.vertices[0], tri.vertices[1], tri.vertices[2], tri.normal, p0, p1, dir,
                                    &segLen, &tBest, hit, &dist)
            && result->contactCount < result->contactMaxCount) { /* (guard, see above) */
            VincTMTransform(tm, hit, cur->position);
            VincTMRotate(tm, tri.normal, cur->normal);
            cur->dims = 0;
            cur->element1 = 0;
            cur->element2 = src->index;
            cur->separation = -radius;
            ++result->contactCount;
        }
    }
    if (result->contactCount <= start)
        return 0;
    /* NOTE: the triangle normal in the capsule's frame, not rotated to world space */
    result->normal[0] = result->normal[0] + tri.normal[0];
    result->normal[1] = result->normal[1] + tri.normal[1];
    result->normal[2] = result->normal[2] + tri.normal[2];
    return 1;
}

/* the generator's points (primitive frame) to world-space contacts; returns 1 when there were points */
static int AppendContacts(VincIntersectResult* result, const VincTriangle* src, const MeReal* tm,
                          const MeReal* n, const MeReal* points, const MeReal* end, const MeReal* seps,
                          MeReal sharedSep, MeI16 dims)
{
    MeVector3 nW;
    const MeReal* q;
    VincContact* c = &result->contacts[result->contactCount];
    VincTMRotate(tm, n, nW);
    for (q = points; q != end; q += 3) {
        if (!(result->contactCount < result->contactMaxCount))
            break;
        VincTMTransform(tm, q, c->position);
        c->dims = dims;
        c->separation = seps ? *seps++ : sharedSep;
        c->element1 = src->index;
        c->normal[0] = nW[0];
        c->normal[1] = nW[1];
        c->normal[2] = nW[2];
        ++result->contactCount;
        ++c;
    }
    result->normal[0] = result->normal[0] + nW[0];
    result->normal[1] = result->normal[1] + nW[1];
    result->normal[2] = result->normal[2] + nW[2];
    return end != points;
}

/* Mac 0x4fd4d0, Win 0x7cf9c0: the triangle in the plane's frame (default edge data) against z <= 0;
 * one separation for all points */
int VincTrianglePlaneContacts(VincIntersectResult* result, const VincTriangle* src, const MeReal* tm, MeReal pad)
{
    VincTriangle tri;
    MeReal v[3][3];
    MeReal points[6 * 3];
    MeReal* cursor = points;
    MeVector3 n;
    MeReal sep;
    MeI16 dims;
    int i;
    TriangleInit(&tri, v, src, 0);
    for (i = 0; i < 3; ++i)
        VincTMInverseTransform(tm, src->vertices[i], v[i]);
    VincTriangleUpdateEdges(&tri);
    VincTriangleUpdateNormal(&tri);
    if (!VincPlaneTriangleContact(&sep, n, &cursor, &dims, pad, &tri))
        return 0;
    return AppendContacts(result, src, tm, n, points, cursor, 0, sep, dims);
}

/* Mac 0x4fdaa6, Win 0x7cec40: the triangle in the box's frame (index and edge data from src) */
int VincTriangleBoxContacts(VincIntersectResult* result, const VincTriangle* src, const MeReal* radii,
                            const MeReal* tm, MeReal pad)
{
    VincTriangle tri;
    MeReal v[3][3];
    MeReal points[18 * 3], seps[18];
    MeReal* cursor = points;
    MeReal* sepCursor = seps;
    MeVector3 n;
    MeReal out1, out2;
    MeI16 dims;
    int i;
    TriangleInit(&tri, v, src, 1);
    for (i = 0; i < 3; ++i)
        VincTMInverseTransform(tm, src->vertices[i], v[i]);
    VincTriangleUpdateEdges(&tri);
    VincTriangleUpdateNormal(&tri);
    tri.index = src->index;
    if (!VincBoxTriangleContact(&out1, n, &out2, &cursor, &sepCursor, &dims, &pad, radii, &tri))
        return 0;
    return AppendContacts(result, src, tm, n, points, cursor, seps, 0.0, dims);
}

/* Mac 0x4fe12d, Win 0x7ceed0: the triangle in the cylinder's frame (edge data from src, index -1).
 * NOTE: unlike the box wrapper, the separation cursor is not reset after the generator call: the
 * contacts take their separations from behind the ones written, i.e. uninitialised stack in the
 * original (garbage separations reach the result). Kept, reading zeros here (UB guard; the array
 * is doubled so the read stays inside it). */
int VincTriangleCylinderContacts(VincIntersectResult* result, const VincTriangle* src, MeReal radius,
                                 MeReal halfHeight, const MeReal* tm, MeReal tol, MeReal cosTol, int flag)
{
    VincTriangle tri;
    MeReal v[3][3];
    MeReal points[18 * 3], seps[2 * 18];
    MeReal* cursor = points;
    MeReal* sepCursor = seps;
    MeVector3 n;
    MeReal out1, out2;
    MeI16 dims;
    int i;
    TriangleInit(&tri, v, src, 1);
    for (i = 0; i < 3; ++i)
        VincTMInverseTransform(tm, src->vertices[i], v[i]);
    VincTriangleUpdateEdges(&tri);
    VincTriangleUpdateNormal(&tri);
    memset(seps, 0, sizeof seps);
    if (!VincCylinderTriangleContacts(&out1, n, &out2, &cursor, &dims, &tol, &radius, &halfHeight, &tri, cosTol,
                                      flag, &sepCursor))
        return 0;
    return AppendContacts(result, src, tm, n, points, cursor, sepCursor, 0.0, dims);
}
