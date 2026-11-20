// Vortex 5.0.0 build 800 "vortexldd41" - point-set helpers (restored).
// Mac TU 491 0x458334-0x458e66 (no file name or symbols), Win 0x7e31e0-0x7e3b20. See
// Vx/VxGeometryUtils.h.
#include "Vx/VxGeometryUtils.h"
#include "Vinc/VincTriangleMeshContacts.h"

#include <float.h>
#include <math.h>
#include <string.h>

namespace Vx {

// Mac 0x458334, Win 0x7e3380
void vxComputeCovariance(const VxArray<VxVector3>& points, VxReal cov[3][3], VxVector3& mean)
{
    VxReal mx = 0, my = 0, mz = 0;
    for (const VxVector3* p = points.begin(); p != points.end(); ++p) {
        mx = mx + (*p)[0];
        my = my + (*p)[1];
        mz = mz + (*p)[2];
    }
    VxReal inv = 1.0 / (VxReal)(unsigned int)points.size();
    mx = mx * inv;
    my = my * inv;
    mz = mz * inv;
    for (const VxVector3* p = points.begin(); p != points.end(); ++p) {
        VxReal dz = (*p)[2] - mz;
        VxReal dy = (*p)[1] - my;
        VxReal dx = (*p)[0] - mx;
        VxReal t[3][3] = {{dx * dx, dy * dx, dz * dx}, {dy * dx, dy * dy, dz * dy}, {dz * dx, dz * dy, dz * dz}};
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                cov[i][j] = cov[i][j] + t[i][j];
    }
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            cov[i][j] = cov[i][j] * inv;
    mean.set(mx, my, mz);
}

namespace {
inline void swapColumns(VxReal a[3][3], int c1, int c2)
{
    for (int r = 0; r < 3; ++r) {
        VxReal t = a[r][c2];
        a[r][c2] = a[r][c1];
        a[r][c1] = t;
    }
}
} // namespace

// Mac 0x45856a, Win 0x7e31e0
bool vxComputeEigenSystem(const VxReal cov[3][3], VxReal axes[3][3], VxVector3& eigenvalues)
{
    static const VxReal kIdentity[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}; // Mac 0x643fc0
    VxReal v[3][3];
    VxReal a[3][3];
    memcpy(v, kIdentity, sizeof(v));
    memcpy(a, cov, sizeof(a));
    unsigned int sweeps = (unsigned int)VincJacobi3(&a[0][0], &v[0][0], 0x65);
    memcpy(axes, v, sizeof(v));
    eigenvalues.set(a[0][0], a[1][1], a[2][2]);
    if (sweeps >= 0x24)
        return false;
    VxReal t = eigenvalues[1];
    if (t < eigenvalues[2]) {
        eigenvalues[1] = eigenvalues[2];
        eigenvalues[2] = t;
        swapColumns(axes, 1, 2);
    }
    t = eigenvalues[0];
    if (t < eigenvalues[1]) {
        eigenvalues[0] = eigenvalues[1];
        eigenvalues[1] = t;
        swapColumns(axes, 0, 1);
    }
    t = eigenvalues[1];
    if (t < eigenvalues[2]) {
        eigenvalues[1] = eigenvalues[2];
        eigenvalues[2] = t;
        swapColumns(axes, 1, 2);
    }
    return true;
}

// Mac 0x45879a, Win 0x7e34d0
bool vxComputePrincipalAxes(const VxArray<VxVector3>& points, VxReal axes[3][3], VxVector3& eigenvalues,
                            VxReal cov[3][3], VxVector3& mean)
{
    for (int i = 0; i < 3; ++i)
        cov[i][0] = cov[i][1] = cov[i][2] = 0;
    vxComputeCovariance(points, cov, mean);
    return vxComputeEigenSystem(cov, axes, eigenvalues);
}

// Mac 0x4587fe, Win 0x7e3530
VxReal vxFarthestPoint(const VxArray<VxVector3>& points, const VxVector3& center, VxVector3& out)
{
    VxReal best = -DBL_MAX;
    const VxVector3* far = 0;
    for (const VxVector3* p = points.begin(); p != points.end(); ++p) {
        VxReal d = ((*p)[0] - center[0]) * ((*p)[0] - center[0]) + ((*p)[1] - center[1]) * ((*p)[1] - center[1]) +
                   ((*p)[2] - center[2]) * ((*p)[2] - center[2]);
        if (best < d) {
            far = p;
            best = d;
        }
    }
    if (points.size() != 0 && far != 0 && far != &out)
        out = *far;
    return sqrt(best);
}

// Mac 0x4588c8, Win 0x7e3a80
void vxExtremePoints(const VxArray<VxVector3>& points, const VxVector3& dir, VxVector3& maxPoint,
                     VxVector3& minPoint, VxReal& maxValue, VxReal& minValue)
{
    maxValue = -DBL_MAX;
    minValue = DBL_MAX;
    for (const VxVector3* p = points.begin(); p != points.end(); ++p) {
        VxReal d = (*p)[0] * dir[0] + (*p)[1] * dir[1] + (*p)[2] * dir[2];
        if (d < minValue) {
            minValue = d;
            if (p != &minPoint)
                minPoint = *p;
        }
        if (d > maxValue) {
            maxValue = d;
            if (p != &maxPoint)
                maxPoint = *p;
        }
    }
}

// Mac 0x4589ac, Win 0x7e3650
VxReal vxFarthestPointFromLine(const VxArray<VxVector3>& points, const VxVector3& origin, const VxVector3& dir,
                               VxVector3& point, VxVector3& foot)
{
    VxReal best = -DBL_MAX;
    const VxVector3* far = points.end();
    for (const VxVector3* p = points.begin(); p != points.end(); ++p) {
        VxReal t = ((*p)[0] - origin[0]) * dir[0] + ((*p)[1] - origin[1]) * dir[1] + ((*p)[2] - origin[2]) * dir[2];
        VxVector3 f(dir[0] * t + origin[0], dir[1] * t + origin[1], dir[2] * t + origin[2]);
        VxReal dx = (*p)[0] - f[0];
        VxReal dy = (*p)[1] - f[1];
        VxReal dz = (*p)[2] - f[2];
        VxReal d = dx * dx + dy * dy + dz * dz;
        if (best < d) {
            far = p;
            best = d;
            foot = f;
        }
    }
    if (points.size() != 0 && far != &point)
        point = *far;
    return sqrt(best);
}

// Mac 0x458b16, Win 0x7e35c0
VxReal vxPointLineDistance(const VxVector3& p, const VxVector3& origin, const VxVector3& dir, VxVector3& foot)
{
    VxReal t = (p[0] - origin[0]) * dir[0] + (p[1] - origin[1]) * dir[1] + (p[2] - origin[2]) * dir[2];
    foot.set(dir[0] * t + origin[0], dir[1] * t + origin[1], dir[2] * t + origin[2]);
    return sqrt((p[0] - foot[0]) * (p[0] - foot[0]) + (p[1] - foot[1]) * (p[1] - foot[1]) +
                (p[2] - foot[2]) * (p[2] - foot[2]));
}

// Mac 0x458bec, Win 0x7e3750
VxReal vxPointSegmentDistance(const VxVector3& p, const VxVector3& a, const VxVector3& b, VxVector3& closest)
{
    VxReal ex = b[0] - a[0], ey = b[1] - a[1], ez = b[2] - a[2];
    VxReal px = p[0] - a[0], py = p[1] - a[1], pz = p[2] - a[2];
    VxReal t = ex * px + py * ey + pz * ez;
    if (t <= 0.0) {
        if (&a != &closest)
            closest = a;
        return sqrt(px * px + py * py + pz * pz);
    }
    VxReal len2 = ex * ex + ey * ey + ez * ez;
    if (t < len2) {
        t = t / len2;
        closest.set(ex * t + a[0], ey * t + a[1], ez * t + a[2]);
        return sqrt((p[0] - closest[0]) * (p[0] - closest[0]) + (p[1] - closest[1]) * (p[1] - closest[1]) +
                    (p[2] - closest[2]) * (p[2] - closest[2]));
    }
    if (&b != &closest)
        closest = b;
    return sqrt((p[0] - b[0]) * (p[0] - b[0]) + (p[1] - b[1]) * (p[1] - b[1]) + (p[2] - b[2]) * (p[2] - b[2]));
}

} // namespace Vx
