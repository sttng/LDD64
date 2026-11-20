// Vortex 5.0.0 build 800 "vortexldd41" - triangle-triangle separation along a direction (restored,
// Mac 557c 0x4c312a-0x4c4126). Used by the triangle mesh pair tests of 557d
// (Mac 0x4ce134, 0x4cedbc, 0x4d15ce). Both triangles are extruded along dir into prisms; the
// separation is the smallest depth of (a) vertices inside the other prism, projected onto its face
// plane, and (b) edge / prism-side intersections. Coplanar triangles: a 2D sweep in (dir, dir x n).
#include "Vinc/VincTriangleMesh.h"

#include <float.h>
#include <math.h>

namespace {

// Mac 0x4c31e8 (name inferred): equal planes (same d within 1e-14 and equal or opposite normals).
// NOTE: for opposite normals the original still compares d with d (not -d).
bool SamePlane(VincTriTriSeparation* /*s*/, const MeReal* p, const MeReal* q)
{
    if (fabs(p[3] - q[3]) <= 1e-14) {
        if (fabs(p[0] - q[0]) <= 1e-14 && fabs(p[1] - q[1]) <= 1e-14 && fabs(p[2] - q[2]) <= 1e-14)
            return true;
        if (fabs(p[0] + q[0]) <= 1e-14 && fabs(p[1] + q[1]) <= 1e-14)
            return fabs(p[2] + q[2]) <= 1e-14;
    }
    return false;
}

// Mac 0x4c32d8 (name inferred): coplanar case
bool CoplanarSeparation(VincTriTriSeparation* s)
{
    MeReal d0 = s->dir[0], d1 = s->dir[1], d2 = s->dir[2];
    MeReal nx = s->plane[0][0], ny = s->plane[0][1], nz = s->plane[0][2];
    MeReal proj[2][3][2];
    for (int t = 0; t < 2; ++t) {
        for (int k = 0; k < 3; ++k) {
            MeReal* p = s->v[t * 3 + k];
            proj[t][k][0] = d0 * p[0] + d1 * p[1] + d2 * p[2];
            proj[t][k][1] = (d1 * nz - d2 * ny) * p[0] + (d2 * nx - nz * d0) * p[1]
                            + (ny * d0 - d1 * nx) * p[2];
        }
    }
    int bestK = 0, bestTa = 0, bestJ = 0, bestTb = 0;
    MeReal bestF = d0;
    for (int ta = 0; ta < 2; ++ta) {
        int tb = ta == 0;
        for (int k = 0; k < 3; ++k) {
            MeReal y = proj[ta][k][1];
            for (int j = 0; j < 3; ++j) {
                int j1 = (j + 1) % 3;
                MeReal yA = proj[tb][j][1];
                MeReal yB = proj[tb][j1][1];
                if ((yA <= y && y <= yB) || (y <= yA && yB <= y)) {
                    MeReal dy = yB - yA;
                    if (dy != 0.0) {
                        MeReal f = (yB - y) / dy;
                        MeReal x = proj[tb][j1][0] - (proj[tb][j1][0] - proj[tb][j][0]) * f;
                        MeReal dist;
                        if (ta == 0)
                            dist = proj[0][k][0] - x;
                        else
                            dist = x - proj[ta][k][0];
                        if (dist < s->depth) {
                            s->depth = dist;
                            bestK = k;
                            bestTa = ta;
                            bestF = f;
                            bestJ = j;
                            bestTb = tb;
                        }
                    }
                }
            }
        }
    }
    bool found = s->depth != DBL_MAX;
    if (found) {
        MeReal* p = s->v[bestTa * 3 + bestK];
        s->point[bestTa][0] = p[0];
        s->point[bestTa][1] = p[1];
        s->point[bestTa][2] = p[2];
        MeReal* a = s->v[bestTb * 3 + bestJ];
        MeReal* b = s->v[bestTb * 3 + (bestJ + 1) % 3];
        s->point[bestTb][0] = b[0] - (b[0] - a[0]) * bestF;
        s->point[bestTb][1] = b[1] - (b[1] - a[1]) * bestF;
        s->point[bestTb][2] = b[2] - (b[2] - a[2]) * bestF;
    }
    return found;
}

// Mac 0x4c3702 (name inferred): face and prism-side planes; 0 for coplanar triangles
int SetupPlanes(VincTriTriSeparation* s)
{
    MeReal* na = s->normal[0];
    s->plane[0][0] = na[0];
    s->plane[0][1] = na[1];
    s->plane[0][2] = na[2];
    MeReal* nb = s->normal[1];
    s->plane[1][0] = nb[0];
    s->plane[1][1] = nb[1];
    s->plane[1][2] = nb[2];
    MeReal* p = s->v[0];
    s->plane[0][3] = na[0] * p[0] + na[1] * p[1] + na[2] * p[2];
    p = s->v[3];
    s->plane[1][3] = nb[0] * p[0] + nb[1] * p[1] + nb[2] * p[2];
    if (SamePlane(s, s->plane[0], s->plane[1]))
        return 0;
    for (int i = 0; i < 3; ++i) {
        for (int t = 0; t < 2; ++t) {
            MeReal* a = s->v[t * 3 + i];
            MeReal* b = s->v[t * 3 + (i + 1) % 3];
            MeReal ex = b[0] - a[0], ey = b[1] - a[1], ez = b[2] - a[2];
            MeReal d0 = s->dir[0], d1 = s->dir[1], d2 = s->dir[2];
            s->edgePlane[t][i][0] = ez * d1 - ey * d2;
            s->edgePlane[t][i][1] = ex * d2 - ez * d0;
            s->edgePlane[t][i][2] = ey * d0 - ex * d1;
        }
    }
    for (int t = 0; t < 2; ++t) {
        MeReal nd = s->plane[t][0] * s->dir[0] + s->plane[t][1] * s->dir[1] + s->plane[t][2] * s->dir[2];
        if (nd < 0.0) {
            for (int i = 0; i < 3; ++i) {
                s->edgePlane[t][i][0] = -s->edgePlane[t][i][0];
                s->edgePlane[t][i][1] = -s->edgePlane[t][i][1];
                s->edgePlane[t][i][2] = -s->edgePlane[t][i][2];
            }
        }
    }
    for (int i = 0; i < 3; ++i) {
        for (int t = 0; t < 2; ++t) {
            MeReal* e = s->edgePlane[t][i];
            MeReal* q = s->v[t * 3 + i];
            e[3] = e[0] * q[0] + e[1] * q[1] + e[2] * q[2];
        }
    }
    return 1;
}

// Mac 0x4c3a8c (name inferred): the two other edges of edge i
void OtherEdges(VincTriTriSeparation* /*s*/, const int* i, int* a, int* b)
{
    if (*i == 1) {
        *a = 0;
        *b = 2;
    } else if (*i == 2) {
        *a = 0;
        *b = 1;
    } else if (*i == 0) {
        *a = 1;
        *b = 2;
    }
}

// Mac 0x4c3ade (name inferred): p + t dir on the plane
bool ProjectToPlane(VincTriTriSeparation* s, const MeReal* p, const MeReal* plane, MeReal* out, MeReal* t)
{
    MeReal den = plane[0] * s->dir[0] + plane[1] * s->dir[1] + plane[2] * s->dir[2];
    bool ok = 1e-14 <= fabs(den);
    if (ok) {
        MeReal u = (plane[3] - (plane[0] * p[0] + plane[1] * p[1] + plane[2] * p[2])) / den;
        *t = u;
        out[0] = u * s->dir[0] + p[0];
        out[1] = u * s->dir[1] + p[1];
        out[2] = u * s->dir[2] + p[2];
    }
    return ok;
}

// Mac 0x4c3bca (name inferred): segment p-q against a plane, within the segment
bool SegmentPlane(VincTriTriSeparation* /*s*/, const MeReal* p, const MeReal* q, const MeReal* plane,
                  MeReal* out)
{
    MeReal ez = q[2] - p[2], ey = q[1] - p[1], ex = q[0] - p[0];
    MeReal den = ex * plane[0] + ey * plane[1] + ez * plane[2];
    if (1e-14 <= fabs(den)) {
        MeReal t = (plane[3] - (p[0] * plane[0] + p[1] * plane[1] + p[2] * plane[2])) / den;
        if (0.0 <= t && t <= 1.0) {
            out[0] = t * ex + p[0];
            out[1] = ey * t + p[1];
            out[2] = ez * t + p[2];
            return true;
        }
    }
    return false;
}

inline MeReal PlaneDist(const MeReal* e, const MeReal* p)
{
    return (e[0] * p[0] + e[1] * p[1] + e[2] * p[2]) - e[3];
}

// Mac 0x4c3cf8 (name inferred): triangle ta against the prism of tb; 1 when all three vertices of
// ta are inside (no edge pass needed)
int VertexEdgeSeparation(VincTriTriSeparation* s, const int* ta, const int* tb)
{
    int inside = 0;
    for (int k = 0; k < 3; ++k) {
        MeReal* p = s->v[*ta * 3 + k];
        int j;
        for (j = 0; j < 3; ++j)
            if (PlaneDist(s->edgePlane[*tb][j], p) < 0.0)
                break;
        if (j != 3)
            continue;
        ++inside;
        MeReal q[3], t;
        if (ProjectToPlane(s, p, s->plane[*tb], q, &t)) {
            if (*ta == 0)
                t = -t;
            if (t < s->depth) {
                s->point[*ta][0] = p[0];
                s->point[*ta][1] = p[1];
                s->point[*ta][2] = p[2];
                s->point[*tb][0] = q[0];
                s->point[*tb][1] = q[1];
                s->point[*tb][2] = q[2];
                s->depth = t;
            }
        }
    }
    if (inside == 3)
        return 1;
    for (int k = 0; k < 3; ++k) {
        for (int j = 0; j < 3; ++j) {
            MeReal r[3];
            if (SegmentPlane(s, s->v[*ta * 3 + k], s->v[*ta * 3 + (k + 1) % 3], s->edgePlane[*tb][j], r)) {
                int j1, j2;
                OtherEdges(s, &j, &j1, &j2);
                MeReal q[3], t;
                if (0.0 <= PlaneDist(s->edgePlane[*tb][j1], r) && 0.0 <= PlaneDist(s->edgePlane[*tb][j2], r)
                    && ProjectToPlane(s, r, s->plane[*tb], q, &t)) {
                    if (*ta == 0)
                        t = -t;
                    if (t < s->depth) {
                        s->point[*ta][0] = r[0];
                        s->point[*ta][1] = r[1];
                        s->point[*ta][2] = r[2];
                        s->point[*tb][0] = q[0];
                        s->point[*tb][1] = q[1];
                        s->point[*tb][2] = q[2];
                        s->depth = t;
                    }
                }
            }
        }
    }
    return 0;
}

} // namespace

extern "C" {

// Mac 0x4c312a / 0x4c3196 (the second is a plain wrapper of the first) (name inferred)
void MEAPI VincTriTriSeparationInit(VincTriTriSeparation* s, MeReal* a0, MeReal* a1, MeReal* a2, MeReal* b0,
                              MeReal* b1, MeReal* b2, MeReal* na, MeReal* nb, const MeReal* dir)
{
    s->dir[0] = dir[0];
    s->dir[1] = dir[1];
    s->dir[2] = dir[2];
    s->depth = DBL_MAX;
    s->v[0] = a0;
    s->v[1] = a1;
    s->v[2] = a2;
    s->v[3] = b0;
    s->v[4] = b1;
    s->v[5] = b2;
    s->normal[0] = na;
    s->normal[1] = nb;
}

// Mac 0x4c4126 (name inferred)
int MEAPI VincTriTriSeparationCompute(VincTriTriSeparation* s)
{
    if (SetupPlanes(s) == 0) {
        CoplanarSeparation(s);
    } else {
        int a = 0, b = 1;
        if (VertexEdgeSeparation(s, &a, &b) != 1) {
            int c = 1, d = 0;
            VertexEdgeSeparation(s, &c, &d);
        }
    }
    return s->depth != DBL_MAX;
}

} // extern "C"
