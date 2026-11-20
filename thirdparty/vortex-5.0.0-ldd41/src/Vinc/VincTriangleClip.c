/* Vortex 5.0.0 build 800 "vortexldd41" - segment clippers (box, half space, z-axis cylinder) and
 * the contact generators of single triangles against a sphere / plane / capsule edge (restored,
 * Mac 558e 0x541fcc-0x545192, 0x5458bd, 0x548776, 0x548dc5; Win 0x7e5320-
 * 0x7e7230, 0x81baa0). cdecl on Win. All names (name inferred). Also the box / box clipper
 * (VincBoxClipBoxPoints); the box / triangle and cylinder / triangle generators are in
 * VincBoxTriangle.c and VincCylinderTriangle.c. */
#include "Vinc/VincIntersect.h"
#include "Vinc/VincPairSupport.h"
#include "Vinc/VincPairTestsB.h"
#include "Vinc/VincTriangleContacts.h"
#include "VincTriangleClip.h"

#include <math.h>
#include "Me/MeCrtMath.h"

/* Mac 0x541fcc, Win 0x7e5320: the point where the segment a-b crosses z = 0 (out z = 0);
 * 2 when the segment is (nearly) parallel to the plane, 0 when it does not cross it */
int VincSegmentPlaneZ(const MeReal* a, const MeReal* b, MeReal* out)
{
    MeReal t, dz = a[2] - b[2];
    if (fabs(dz) < 1e-06)
        return 2;
    t = (1.0 / dz) * a[2];
    if (t < 0.0 || 1.0 < t)
        return 0;
    out[2] = 0.0;
    out[0] = (1.0 - t) * a[0] + t * b[0];
    out[1] = (1.0 - t) * a[1] + t * b[1];
    return 1;
}

/* Mac 0x5421e1, Win 0x7e5870: narrow [*tLo, *tHi] to the part of p + t d inside the box of half
 * extents radii (invD = 1 / d); 0 when nothing is left (or the segment is parallel and outside) */
int VincClipSegmentBoxRange(MeReal* tLo, MeReal* tHi, const MeReal* p, const MeReal* d, const MeReal* invD,
                            const MeReal* radii)
{
    int i;
    MeReal t;
    for (i = 0; i < 3; ++i) {
        if (1e-06 < d[i]) {
            t = (-radii[i] - p[i]) * invD[i];
            if (*tLo < t)
                *tLo = t;
            t = (radii[i] - p[i]) * invD[i];
            if (t < *tHi)
                *tHi = t;
        } else if (d[i] < -1e-06) {
            t = (radii[i] - p[i]) * invD[i];
            if (*tLo < t)
                *tLo = t;
            t = (-radii[i] - p[i]) * invD[i];
            if (t < *tHi)
                *tHi = t;
        } else if (p[i] < -radii[i] || radii[i] < p[i]) {
            return 0;
        }
        if (*tHi < *tLo)
            return 0;
    }
    return 1;
}

static void AddPoint(MeReal** cursor, const MeReal* p, const MeReal* d, MeReal t)
{
    MeReal* q = *cursor;
    *cursor = q + 3;
    q[0] = t * d[0] + p[0];
    q[1] = t * d[1] + p[1];
    q[2] = t * d[2] + p[2];
}

/* Mac 0x542337, Win 0x7e59a0 */
void VincClipSegmentBoxAddInnerPoints(MeReal** cursor, const MeReal* p, const MeReal* d, const MeReal* invD,
                                      const MeReal* radii, MeReal unused)
{
    MeReal lo = -1.0, hi = 1.0;
    (void)unused;
    if (VincClipSegmentBoxRange(&lo, &hi, p, d, invD, radii)) {
        if (-1.0 < lo)
            AddPoint(cursor, p, d, lo);
        if (hi < 1.0)
            AddPoint(cursor, p, d, hi);
    }
}

/* Mac 0x542449, Win 0x7e5a70 */
void VincClipSegmentBoxAddPoints(MeReal** cursor, const MeReal* p, const MeReal* d, const MeReal* invD,
                                 const MeReal* radii, MeReal unused)
{
    MeReal lo = -1.0, hi = 1.0;
    (void)unused;
    if (VincClipSegmentBoxRange(&lo, &hi, p, d, invD, radii)) {
        AddPoint(cursor, p, d, lo);
        AddPoint(cursor, p, d, hi);
    }
}

/* Mac 0x542530, Win 0x7e5bc0: out = the point of the segment p + t d, t in [0, 1], closest to
 * p + q; 1 when it lies inside the segment. out may alias q. */
int VincClosestPointOnSegment(MeReal* out, const MeReal* p, const MeReal* d, const MeReal* q)
{
    MeReal t = d[0] * q[0] + d[1] * q[1] + d[2] * q[2];
    MeReal dd;
    if (t <= 0.0) {
        out[0] = p[0];
        out[1] = p[1];
        out[2] = p[2];
        return 0;
    }
    dd = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
    if (dd <= t) {
        out[0] = d[0] + p[0];
        out[1] = d[1] + p[1];
        out[2] = d[2] + p[2];
        return 0;
    }
    t = t / dd;
    out[0] = t * d[0] + p[0];
    out[1] = d[1] * t + p[1];
    out[2] = d[2] * t + p[2];
    return 1;
}

/* Mac 0x542656, Win 0x7e5f90: narrow [*tLo, *tHi] to the part of p + t dir with z <= 0 */
static int ClipSegmentHalfSpaceRange(MeReal* tLo, MeReal* tHi, const MeReal* p, const MeReal* dir,
                                     const MeReal* invDirZ)
{
    MeReal t = -(p[2] * *invDirZ);
    if (0.0 < dir[2]) {
        if (t < *tHi)
            *tHi = t;
    } else if (dir[2] < 0.0) {
        if (*tLo < t)
            *tLo = t;
    } else if (0.0 < p[2]) {
        return 0;
    }
    return *tLo <= *tHi;
}

/* Mac 0x544db7, Win 0x7e7100 */
void VincClipSegmentAddPoints(MeReal** cursor, const MeReal* p, const MeReal* dir, const MeReal* invDirZ)
{
    MeReal lo = -1.0, hi = 1.0;
    if (ClipSegmentHalfSpaceRange(&lo, &hi, p, dir, invDirZ)) {
        AddPoint(cursor, p, dir, lo);
        AddPoint(cursor, p, dir, hi);
    }
}

/* Mac 0x544e97, Win 0x7e7030 */
void VincClipSegmentAddInnerPoints(MeReal** cursor, const MeReal* p, const MeReal* dir, const MeReal* invDirZ)
{
    MeReal lo = -1.0, hi = 1.0;
    if (ClipSegmentHalfSpaceRange(&lo, &hi, p, dir, invDirZ)) {
        if (-1.0 < lo)
            AddPoint(cursor, p, dir, lo);
        if (hi < 1.0)
            AddPoint(cursor, p, dir, hi);
    }
}

/* Mac 0x543bb5, Win 0x7e5c80: narrow [*tLo, *tHi] to the part of p + t d inside the z-axis
 * cylinder of radius *r and half height *hh; *loKind / *hiKind = 0 when the new end lies on the
 * side, 1 on a cap (-1: unchanged). 0 when nothing is left. */
int VincClipSegmentCylinderRange(MeReal* tLo, MeReal* tHi, const MeReal* p, const MeReal* d, const MeReal* r,
                                 const MeReal* hh, int* loKind, int* hiKind)
{
    MeReal c = (p[0] * p[0] + p[1] * p[1]) - *r * *r;
    MeReal b = p[0] * d[0] + p[1] * d[1];
    MeReal a, disc, t, dz, z, zLo, zHi, h, root, sHi, sLo;
    *hiKind = -1;
    *loKind = -1;
    a = d[0] * d[0] + d[1] * d[1];
    disc = b * b - c * a;
    if (disc < 0.0)
        return 0;
    t = -b - *tHi * a;
    if (0.0 <= t && disc < t * t)
        return 0;
    t = *tLo * a + b;
    if (0.0 <= t && disc < t * t)
        return 0;
    dz = d[2];
    if (dz < 0.0) {
        z = p[2];
        dz = -dz;
    } else {
        z = -p[2];
    }
    h = *hh;
    zLo = z - h;
    if (dz * *tHi < zLo)
        return 0;
    zHi = z + h;
    if (zHi < dz * *tLo)
        return 0;
    root = sqrt(disc);
    sHi = root - b;
    if (sHi * dz < zLo * a)
        return 0;
    sLo = -b - root;
    if (zHi * a < sLo * dz)
        return 0;
    if (1e-16 <= a) {
        if (1e-08 <= fabs(dz)) {
            if (zLo * a <= sLo * dz) {
                zLo = sLo * (1.0 / a);
                *loKind = 0;
            } else {
                zLo = zLo * (1.0 / dz);
                *loKind = 1;
            }
            if (sHi * dz <= zHi * a) {
                zHi = sHi * (1.0 / a);
                *hiKind = 0;
            } else {
                zHi = (1.0 / dz) * zHi;
                *hiKind = 1;
            }
        } else {
            if (h < fabs(p[2]))
                return 0;
            zLo = sLo * (1.0 / a);
            zHi = sHi * (1.0 / a);
            *loKind = 0;
            *hiKind = 0;
        }
    } else {
        if (0.0 < c)
            return 0;
        zLo = zLo * (1.0 / dz);
        zHi = zHi * (1.0 / dz);
        *loKind = 1;
        *hiKind = 1;
    }
    if (*tLo < zLo)
        *tLo = zLo;
    if (zHi < *tHi)
        *tHi = zHi;
    return 1;
}

/* Mac 0x543ed8, Win 0x7e5f50 */
int VincClipSegmentCylinder(MeReal* tLo, MeReal* tHi, const MeReal* p, const MeReal* d, const MeReal* r,
                            const MeReal* hh)
{
    int loKind, hiKind;
    return VincClipSegmentCylinderRange(tLo, tHi, p, d, r, hh, &loKind, &hiKind);
}

/* Mac 0x544fa2, Win 0x7e60f0 */
void VincClipSegmentCylinderAddPoints(MeReal** cursor, const MeReal* p, const MeReal* dir, const MeReal* r, MeReal h,
                                      const MeReal* tLo, const MeReal* tHi)
{
    MeReal lo = *tLo, hi = *tHi;
    if (VincClipSegmentCylinder(&lo, &hi, p, dir, r, &h)) {
        AddPoint(cursor, p, dir, lo);
        AddPoint(cursor, p, dir, hi);
    }
}

/* Mac 0x545088, Win 0x7e6010 */
void VincClipSegmentCylinderAddInnerPoints(MeReal** cursor, const MeReal* p, const MeReal* dir, const MeReal* r,
                                           MeReal h, const MeReal* tLo, const MeReal* tHi)
{
    MeReal lo = *tLo, hi = *tHi;
    if (VincClipSegmentCylinder(&lo, &hi, p, dir, r, &h)) {
        if (*tLo < lo)
            AddPoint(cursor, p, dir, lo);
        if (hi < *tHi)
            AddPoint(cursor, p, dir, hi);
    }
}

/* Win 0x7e5110 (inlined on the Mac, table Mac 0x645740): the order of three values as a sort
 * network table; only the index of the smallest is used here */
static int MinIndex3(const MeReal* v)
{
    static const signed char kOrder[8] = {0x06, 0x09, 0x00, 0x21, 0x12, 0x00, 0x18, 0x24};
    return kOrder[((v[1] < v[0]) * 2 | (v[2] < v[0])) * 2 | (v[2] < v[1])] >> 4;
}

/* Mac 0x5458bd, Win 0x7e53b0: tri (plane frame) against the plane z = 0: *sep = the lowest vertex
 * height; when it is within pad: the vertices exactly at height pad and the crossings of z = 0 of
 * the edges between vertices on different sides of pad go to *points, dims = (count - 1, at most
 * 2) << 8 | 2. NOTE: the original compares the vertex heights with pad but crosses the edges with
 * z = 0, and only vertices at exactly pad are added; kept. */
int VincPlaneTriangleContact(MeReal* sep, MeReal* normal, MeReal** points, MeI16* dims, MeReal pad,
                             const VincTriangle* tri)
{
    MeReal z[3];
    int side[3], i;
    normal[0] = 0.0;
    normal[1] = 0.0;
    normal[2] = -1.0;
    z[0] = tri->vertices[0][2];
    z[1] = tri->vertices[1][2];
    z[2] = tri->vertices[2][2];
    *sep = z[MinIndex3(z)];
    if (pad < *sep)
        return 0;
    *dims = -1;
    for (i = 0; i < 3; ++i) {
        if (pad < z[i]) {
            side[i] = 1;
        } else if (pad <= z[i]) {
            MeReal* q = *points;
            side[i] = 0;
            *points = q + 3;
            q[0] = tri->vertices[i][0];
            q[1] = tri->vertices[i][1];
            q[2] = tri->vertices[i][2];
            ++*dims;
        } else {
            side[i] = -1;
        }
    }
    for (i = 0; i < 3; ++i) {
        int j = (i + 1) % 3;
        if (side[i] != 0 && side[j] != 0 && side[i] != side[j] &&
            VincSegmentPlaneZ(tri->vertices[i], tri->vertices[j], *points) == 1) {
            *points += 3;
            ++*dims;
        }
    }
    if (2 < *dims)
        *dims = 2;
    *dims = (MeI16)(*dims << 8 | 2);
    return 1;
}

/* Mac 0x548776, Win 0x7e7230 (see VincTriangleContacts.h): the face when the sphere centre (the
 * origin) projects into the triangle, else the nearest edge (the one with the largest positive
 * distance outside); edge contacts outside the edge's normal cone (edgeData0 / edgeData1) are
 * bent towards the face normal or rejected. */
int VincSphereTriangleContact(MeReal* sep, MeReal* normal, MeReal* point, MeI16* dims, const MeReal* pad,
                              const MeReal* r, const VincTriangle* tri)
{
    const MeReal* n = tri->normal;
    const MeReal* v0 = tri->vertices[0];
    MeReal d = n[0] * v0[0] + n[1] * v0[1] + n[2] * v0[2];
    MeReal s = fabs(d) - *r;
    MeReal sign = d <= 0.0 ? 1.0 : -1.0;
    MeReal nx, ny, nz;
    int separated;
    *dims = 0x203;
    separated = *pad < s;
    if (!separated) {
        int best = 3, i;
        MeReal bestD2 = 1.7976931348623157e308, bestLen2 = 1.0; /* DBL_MAX */
        for (i = 0; i < 3; ++i) {
            const MeReal* a = tri->vertices[i];
            const MeReal* b = tri->vertices[(i + 1) % 3];
            MeReal e = n[0] * (b[1] * a[2] - b[2] * a[1]) + n[1] * (b[2] * a[0] - a[2] * b[0]) +
                       n[2] * (a[1] * b[0] - b[1] * a[0]);
            if (0.0 < e) {
                const MeReal* ed = tri->edges[i];
                MeReal e2 = e * e;
                MeReal len2 = ed[0] * ed[0] + ed[1] * ed[1] + ed[2] * ed[2];
                if (bestLen2 * e2 < bestD2 * len2) {
                    best = i;
                    bestD2 = e2;
                    bestLen2 = len2;
                }
            }
        }
        if (best != 3) {
            const MeReal* a = tri->vertices[best];
            const MeReal* ed = tri->edges[best];
            MeVector3 c;
            MeReal len, inv, dn, de;
            c[0] = -a[0];
            c[1] = -a[1];
            c[2] = -a[2];
            *dims = (MeI16)(VincClosestPointOnSegment(c, a, ed, c) << 8 | 3);
            len = sqrt(c[0] * c[0] + c[1] * c[1] + c[2] * c[2]);
            s = len - *r;
            separated = *pad < s;
            *sep = s;
            inv = -1.0 / len;
            nz = inv * c[2];
            ny = inv * c[1];
            nx = inv * c[0];
            normal[0] = nx;
            normal[1] = ny;
            normal[2] = nz;
            dn = n[0] * normal[0] + n[1] * normal[1] + n[2] * normal[2];
            de = (nz * n[1] - ny * n[2]) * ed[0] + (n[2] * nx - nz * n[0]) * ed[1] + (ny * n[0] - n[1] * nx) * ed[2];
            if (dn + 0.0 < tri->edgeData0[best]) {
                MeReal x, y, z, l;
                if (de * tri->edgeData1[best] < 0.0)
                    return 0;
                dn = n[0] * normal[0] + n[1] * normal[1] + n[2] * normal[2];
                x = normal[0] - n[0] * dn;
                normal[0] = x;
                y = normal[1] - n[1] * dn;
                normal[1] = y;
                z = normal[2] - dn * n[2];
                normal[2] = z;
                l = sqrt(x * x + y * y + z * z);
                if (l != 0.0) {
                    l = 1.0 / l;
                    normal[0] = x * l;
                    normal[1] = y * l;
                    normal[2] = l * z;
                }
                normal[0] = normal[0] * tri->edgeData1[best];
                normal[1] = normal[1] * tri->edgeData1[best];
                normal[2] = normal[2] * tri->edgeData1[best];
                normal[0] = normal[0] + tri->edgeData0[best] * n[0];
                normal[1] = normal[1] + tri->edgeData0[best] * n[1];
                normal[2] = normal[2] + tri->edgeData0[best] * n[2];
            } else if (0.0 < dn && tri->edgeData1[best] * de < 0.0 &&
                       (0.0 <= tri->edgeData0[best] || 0.0 <= tri->edgeData1[best])) {
                MeReal sg = n[0] * v0[0] + n[1] * v0[1] + n[2] * v0[2] <= 0.0 ? 1.0 : -1.0;
                *dims = 0x203;
                normal[0] = sg * n[0];
                normal[1] = sg * n[1];
                normal[2] = sg * n[2];
            }
            goto point;
        }
    }
    *sep = s;
    nz = sign * n[2];
    ny = sign * n[1];
    nx = sign * n[0];
    normal[0] = nx;
    normal[1] = ny;
    normal[2] = nz;
point:
    if (!separated) {
        MeReal m = -*r;
        point[0] = nx * m;
        point[1] = ny * m;
        point[2] = m * nz;
    }
    return !separated;
}

/* Mac 0x548dc5, Win 0x81baa0 (see VincTriangleContacts.h): the closest points of the edge a-b
 * and the capsule segment (z axis, half height halfHeight); a contact when they are within
 * radius and the direction points against edgeNormal; the contact is written in the frame tm
 * (position and normal), element1 = 0, dims 0x101. A contact whose normal points against
 * triNormal (when given) is dropped. */
int VincCapsuleEdgeContacts(const MeReal* a, const MeReal* b, const MeReal* edgeNormal, MeReal halfHeight,
                            MeReal radius, const MeReal* tm, VincContact** cursor, int maxCount, int* count,
                            const MeReal* triNormal)
{
    MeReal dx, dy, dz, len2, h2, za, e, hh4, f, g, s, t;
    MeReal qx, qy, qz, z, vx, vy, vz, l2;
    VincContact* c;
    if (maxCount <= *count)
        return 0;
    dx = b[0] - a[0];
    dy = b[1] - a[1];
    dz = b[2] - a[2];
    h2 = halfHeight + halfHeight;
    za = a[2] + halfHeight;
    e = h2 * dz;
    len2 = dx * dx + dy * dy + dz * dz;
    hh4 = h2 * h2;
    f = a[0] * dx + a[1] * dy + dz * za;
    g = za * h2;
    if (e != 0.0) {
        MeReal den = e * e - len2 * hh4;
        if (den == 0.0)
            return 0;
        s = (hh4 * f - g * e) / den;
        t = (e * f - len2 * g) / den;
    } else {
        s = -f / len2;
        t = g / hh4;
    }
    if (!(0.0 <= t && t <= 1.0 && 0.0 <= s && s <= 1.0))
        return 0;
    qz = s * dz + a[2];
    qy = dy * s + a[1];
    qx = dx * s + a[0];
    z = h2 * t - halfHeight;
    vz = z - qz;
    vy = 0.0 - qy;
    vx = 0.0 - qx;
    l2 = vx * vx + vy * vy + vz * vz;
    if (!(0.0 < l2 && l2 <= radius * radius && vx * edgeNormal[0] + vy * edgeNormal[1] + vz * edgeNormal[2] < 0.0))
        return 0;
    l2 = sqrt(l2);
    c = *cursor;
    c->position[0] = qx * tm[0] + qy * tm[4] + qz * tm[8] + tm[12];
    c->position[1] = qx * tm[1] + qy * tm[5] + qz * tm[9] + tm[13];
    c->position[2] = qx * tm[2] + qy * tm[6] + qz * tm[10] + tm[14];
    c->dims = 0x101;
    if (radius - l2 <= halfHeight - fabs(z)) {
        MeReal inv;
        c->separation = l2 - radius;
        inv = -1.0 / l2;
        vx = vx * inv;
        vy = vy * inv;
        vz = vz * inv;
        if (0.0 < qx * vx + qy * vy + qz * vz) {
            vx = -vx;
            vy = -vy;
            vz = -vz;
        }
    } else {
        c->separation = -(halfHeight - fabs(z));
        vz = 0.0 <= z ? 1.0 : -1.0;
        vy = 0.0;
        vx = 0.0;
    }
    if (triNormal != 0 && vx * triNormal[0] + vy * triNormal[1] + vz * triNormal[2] < 0.0)
        return 0;
    c->normal[0] = vx * tm[0] + vy * tm[4] + vz * tm[8];
    c->normal[1] = vx * tm[1] + vy * tm[5] + vz * tm[9];
    c->normal[2] = vx * tm[2] + vy * tm[6] + vz * tm[10];
    c->element1 = 0;
    *cursor = (VincContact*)((char*)*cursor + sizeof(VincContact));
    ++*count;
    return 1;
}

/* Mac 0x545192, Win 0x7e8ac0 (see VincPairTestsB.h): the 12 edges of box 2 (rel = its transform
 * in box 1's frame, half extents r2) clipped against box 1 (half extents r1): the x and y edges
 * add only their clipped ends, the z edges both ends of their inside part */
void VincBoxClipBoxPoints(MeReal** cursor, const MeReal* rel, const MeReal* r1, const MeReal* r2, MeReal pad)
{
    MeReal ax[3], ay[3], az[3], ix[3], iy[3], iz[3], p[3];
    const MeReal* t = rel + 12;
    int k;
    for (k = 0; k < 3; ++k) {
        ax[k] = r2[0] * rel[k];
        ix[k] = 1.0 / ax[k];
    }
    for (k = 0; k < 3; ++k) {
        ay[k] = r2[1] * rel[4 + k];
        iy[k] = 1.0 / ay[k];
    }
    for (k = 0; k < 3; ++k) {
        az[k] = r2[2] * rel[8 + k];
        iz[k] = 1.0 / az[k];
    }
#define VINC_EDGE(s1, b, s2, c, dir, inv, fn)                                                                     \
    for (k = 0; k < 3; ++k)                                                                                    \
        p[k] = (t[k] s1 b[k]) s2 c[k];                                                                         \
    fn(cursor, p, dir, inv, r1, pad)
    VINC_EDGE(+, ay, +, az, ax, ix, VincClipSegmentBoxAddInnerPoints);
    VINC_EDGE(-, ay, +, az, ax, ix, VincClipSegmentBoxAddInnerPoints);
    VINC_EDGE(+, ay, -, az, ax, ix, VincClipSegmentBoxAddInnerPoints);
    VINC_EDGE(-, ay, -, az, ax, ix, VincClipSegmentBoxAddInnerPoints);
    VINC_EDGE(+, ax, +, az, ay, iy, VincClipSegmentBoxAddInnerPoints);
    VINC_EDGE(-, ax, +, az, ay, iy, VincClipSegmentBoxAddInnerPoints);
    VINC_EDGE(+, ax, -, az, ay, iy, VincClipSegmentBoxAddInnerPoints);
    VINC_EDGE(-, ax, -, az, ay, iy, VincClipSegmentBoxAddInnerPoints);
    VINC_EDGE(+, ax, +, ay, az, iz, VincClipSegmentBoxAddPoints);
    VINC_EDGE(+, ax, -, ay, az, iz, VincClipSegmentBoxAddPoints);
    VINC_EDGE(-, ax, +, ay, az, iz, VincClipSegmentBoxAddPoints);
    VINC_EDGE(-, ax, -, ay, az, iz, VincClipSegmentBoxAddPoints);
#undef VINC_EDGE
}
