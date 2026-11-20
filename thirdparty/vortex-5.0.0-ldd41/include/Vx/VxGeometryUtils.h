// Vortex 5.0.0 build 800 "vortexldd41" - point-set helpers of the smallest-enclosing-shape fits
// (restored: Mac TU 491 0x458334-0x458e66, Win 0x7e31e0-0x7e3b20), used by the
// fluid TU (Mac 510). Names inferred. Win 0x7e3890 (Win only, not reached) is not restored.
#ifndef VX_VXGEOMETRYUTILS_H
#define VX_VXGEOMETRYUTILS_H

#include "Vx/VxArray.h"
#include "Vx/VxVector3.h"

namespace Vx {

// Mac 0x458334, Win 0x7e3380: mean of the points and their covariance added to cov (then
// cov *= 1/n). cov must start zeroed (computePrincipalAxes does it).
void vxComputeCovariance(const VxArray<VxVector3>& points, VxReal cov[3][3], VxVector3& mean);
// Mac 0x45856a, Win 0x7e31e0: eigenvectors (columns of axes) and eigenvalues of cov (cyclic
// Jacobi, at most 101 sweeps), sorted by decreasing eigenvalue when it converged in < 36
// sweeps; true then
bool vxComputeEigenSystem(const VxReal cov[3][3], VxReal axes[3][3], VxVector3& eigenvalues);
// Mac 0x45879a, Win 0x7e34d0: covariance + eigen system of the points
bool vxComputePrincipalAxes(const VxArray<VxVector3>& points, VxReal axes[3][3], VxVector3& eigenvalues,
                            VxReal cov[3][3], VxVector3& mean);
// Mac 0x4587fe, Win 0x7e3530: the point farthest from center (out) and its distance
VxReal vxFarthestPoint(const VxArray<VxVector3>& points, const VxVector3& center, VxVector3& out);
// Mac 0x4588c8, Win 0x7e3a80: the points with the largest / smallest projection on dir
void vxExtremePoints(const VxArray<VxVector3>& points, const VxVector3& dir, VxVector3& maxPoint,
                     VxVector3& minPoint, VxReal& maxValue, VxReal& minValue);
// Mac 0x4589ac, Win 0x7e3650: the point farthest from the line (origin, unit dir), its foot on
// the line, and the distance
VxReal vxFarthestPointFromLine(const VxArray<VxVector3>& points, const VxVector3& origin, const VxVector3& dir,
                               VxVector3& point, VxVector3& foot);
// Mac 0x458b16, Win 0x7e35c0: distance of p from the line (origin, unit dir); foot = projection
VxReal vxPointLineDistance(const VxVector3& p, const VxVector3& origin, const VxVector3& dir, VxVector3& foot);
// Mac 0x458bec, Win 0x7e3750: distance of p from the segment a-b; closest = nearest point
VxReal vxPointSegmentDistance(const VxVector3& p, const VxVector3& a, const VxVector3& b, VxVector3& closest);

} // namespace Vx

#endif
