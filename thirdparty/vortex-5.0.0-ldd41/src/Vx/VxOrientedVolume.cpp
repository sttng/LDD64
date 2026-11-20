// Vortex 5.0.0 build 800 "vortexldd41" - computeSmallestEnclosing (slots 10/11) of VxBox,
// VxSphere, VxCapsule, VxCylinder, with the oriented volumes and the minimal sphere (restored).
// Mac TU 510 0x46c880-0x474767 (static initialiser 0x474702: <iostream> only),
// Win 0x76a970-0x76fb70 + folded bodies 0x713eb0, 0x7f3490, 0x7fd560. File name inferred.
//
// Not reached in LDD. Sphere: Welzl as in WildMagic3's MinSphere3 (epsilon 1e-14, rand()
// shuffle). Box: turn about the extreme points' "torque" axis (35 steps), then per axis 5 angles
// 0.1 pi apart + golden-section search on half the surface. Capsule / cylinder: turn about the
// same kind of axis, keep the smallest volume. Class names from the RTTI, function names from the
// messages where they exist; the rest inferred.
#include "Vx/VxGeometry.h"
#include "Vx/VxBox.h"
#include "Vx/VxSphere.h"
#include "Vx/VxCapsule.h"
#include "Vx/VxCylinder.h"
#include "Vx/VxGeometryUtils.h"
#include "Vx/VxQuaternion.h"
#include "Vx/VxResponse.h"
#include "Vx/VxMessage.h"

#include <float.h>
#include <math.h>
#include "Me/MeCrtMath.h"
#include <stdlib.h>
#include <iostream>

// NOTE: the oriented volumes and the minimal sphere live in a file-level anonymous namespace (the
// RTTI names are N12_GLOBAL__N_1...: outside namespace Vx).
namespace {

using namespace Vx;

// The caller's points as a VxArray for the point-set helpers (Mac: the begin / end words at
// -0x20 / -0x1c of the callers; no destructor runs). NOTE: a view with VxArray's layout that does
// not own the points (only begin and end are read). (name inferred)
struct PointArray {
    PointArray(const VxVector3* points, int count)
        : mBegin(const_cast<VxVector3*>(points)), mEnd(const_cast<VxVector3*>(points) + count), mCapacity(mEnd),
          mPad(0)
    {
    }
    const VxArray<VxVector3>& array() const { return *reinterpret_cast<const VxArray<VxVector3>*>(this); }
    bool empty() const { return mEnd == mBegin; }

    VxVector3* mBegin;
    VxVector3* mEnd;
    VxVector3* mCapacity;
    int mPad;
};

// Win 0x76c260 (Mac inlined): a random direction (zero if all three draws are 0) (name inferred)
inline VxVector3 randomUnitVector()
{
    VxVector3 r;
    r[0] = vxRandomSymmetric(1.0);
    r[1] = vxRandomSymmetric(1.0);
    r[2] = vxRandomSymmetric(1.0);
    VxReal n2 = r[0] * r[0] + r[1] * r[1] + r[2] * r[2];
    if (n2 > 0.0) {
        VxReal inv = 1.0 / sqrt(n2);
        r[0] = r[0] * inv;
        r[1] = r[1] * inv;
        r[2] = r[2] * inv;
    }
    return r;
}

// Win 0x76fb90 (Mac inlined): rotation 3x3 = identity, translation zero (column 3 untouched) -
// the PCA fallback (name inferred)
inline void setIdentity(VxTransform& tm)
{
    tm.m[2][2] = 1.0;
    tm.m[1][1] = 1.0;
    tm.m[0][0] = 1.0;
    tm.m[3][2] = 0;
    tm.m[3][1] = 0;
    tm.m[3][0] = 0;
    tm.m[2][1] = 0;
    tm.m[2][0] = 0;
    tm.m[1][2] = 0;
    tm.m[1][0] = 0;
    tm.m[0][2] = 0;
    tm.m[0][1] = 0;
}

// --------------------------------------------------------------------------------------
// The oriented volumes: a frame (rows 0-2 = the axes, row 3 = the centre) and the shape's
// dimensions. Mac: vptr +0, transform +4; Win: vptr +0, transform +8 (aligned). No vtable of
// OrientedVolume is emitted; the dtors (Mac D1 0x46da36 box, 0x46fd76 capsule, 0x4715dc
// cylinder, 0x46fdba axial; D0 0x46da0c, 0x46fd4c, 0x46fd90, 0x46da50; Win: one scalar deleting
// dtor 0x76a9a0 for all) are the implicit ones.
class OrientedVolume {
public:
    explicit OrientedVolume(const VxTransform& tm) : mTransform(tm) {}
    virtual ~OrientedVolume() {}
    // slot 2: the volume (the axial search minimises it)
    virtual VxReal volume() const = 0;
    // slot 3, Mac 0x46c888, Win 0x713eb0 (folded): improve the fit after the extents (cylinder)
    virtual void refine(const VxArray<VxVector3>& points) { (void)points; }
    // slot 4: dimensions (and, unless keepCenter, the centre) from the points' extents along the
    // current axes; maxPoints / minPoints get the extreme points per axis
    virtual void computeExtents(const VxArray<VxVector3>& points, bool keepCenter, VxVector3 maxPoints[3],
                                VxVector3 minPoints[3]) = 0;
    // slot 5: axes from the principal axes of the points, then the extents
    virtual void computeSmallestEnclosingPCA(const VxArray<VxVector3>& points, bool keepCenter) = 0;
    // slot 6: iterative improvement of the axes
    virtual void computeSmallestEnclosingIterative(const VxArray<VxVector3>& points, int lockAxis, bool keepCenter) = 0;
    // slot 7: the unit axis of the "torque" of the extreme points about the shape (the
    // direction to turn the frame) and its magnitude
    virtual VxVector3 computeRotationAxis(const VxVector3 minPoints[3], const VxVector3 maxPoints[3],
                                          VxReal& magnitude) const = 0;

    VxVector3& axis(int i) { return *reinterpret_cast<VxVector3*>(mTransform.m[i]); }
    const VxVector3& axis(int i) const { return *reinterpret_cast<const VxVector3*>(mTransform.m[i]); }
    VxVector3& center() { return *reinterpret_cast<VxVector3*>(mTransform.m[3]); }
    const VxVector3& center() const { return *reinterpret_cast<const VxVector3*>(mTransform.m[3]); }

    VxTransform mTransform; // Mac +0x04, Win +0x08
};

// Mac 0x46dd59 (EAX = points, EDX = axis, ECX = volume, XMM0 = angle; stack: keepCenter), Win
// 0x76c2f0: turn the frame by angle about axis, then the extents
void rotateVolume(const VxArray<VxVector3>& points, const VxVector3& rotationAxis, OrientedVolume* v, VxReal angle,
                  bool keepCenter)
{
    VxQuaternion q(rotationAxis, angle);
    for (int i = 0; i < 3; ++i) {
        VxVector3& a = v->axis(i);
        a = q.rotate(a);
    }
    VxVector3 maxPoints[3], minPoints[3];
    v->computeExtents(points, keepCenter, maxPoints, minPoints);
}

// Mac vtable 0x75e4c0, Win (see the slots). mDims are the full extents.
class OrientedBox : public OrientedVolume {
public:
    OrientedBox(const VxTransform& tm, const VxVector3& dims) : OrientedVolume(tm)
    {
        mDims[0] = dims[0];
        mDims[1] = dims[1];
        mDims[2] = dims[2];
    }
    // Mac 0x46c89c, Win 0x76aa20
    virtual VxReal volume() const { return mDims[0] * mDims[1] * mDims[2]; }
    virtual void computeExtents(const VxArray<VxVector3>& points, bool keepCenter, VxVector3 maxPoints[3],
                                VxVector3 minPoints[3]);
    virtual void computeSmallestEnclosingPCA(const VxArray<VxVector3>& points, bool keepCenter);
    virtual void computeSmallestEnclosingIterative(const VxArray<VxVector3>& points, int lockAxis, bool keepCenter);
    virtual VxVector3 computeRotationAxis(const VxVector3 minPoints[3], const VxVector3 maxPoints[3],
                                          VxReal& magnitude) const;
    // Mac 0x46c944 (EAX = this), Win 0x76aaf0: half the surface (the box search's cost)
    VxReal halfSurface() const { return mDims[0] * mDims[1] + mDims[1] * mDims[2] + mDims[0] * mDims[2]; }

    VxReal mDims[3]; // Mac +0x84, Win +0x88
};

// Mac vtable 0x75e540. mHeight: full length along axis 2 (capsule: the core).
class OrientedAxialVolume : public OrientedVolume {
public:
    OrientedAxialVolume(const VxTransform& tm, VxReal height, VxReal radius)
        : OrientedVolume(tm), mHeight(height), mRadius(radius)
    {
    }
    // Mac 0x46c880, Win 0x7fd560 (folded)
    virtual VxReal volume() const { return 0; }
    // Mac 0x46c88e, Win 0x76a970
    virtual void computeExtents(const VxArray<VxVector3>& points, bool keepCenter, VxVector3 maxPoints[3],
                                VxVector3 minPoints[3])
    {
        (void)points; (void)keepCenter; (void)maxPoints; (void)minPoints;
    }
    // Mac 0x46c894, Win 0x7f3490 (folded)
    virtual void computeSmallestEnclosingPCA(const VxArray<VxVector3>& points, bool keepCenter)
    {
        (void)points; (void)keepCenter;
    }
    virtual void computeSmallestEnclosingIterative(const VxArray<VxVector3>& points, int lockAxis, bool keepCenter);
    // Mac 0x46e280, Win 0x76a980: zero (magnitude not written)
    virtual VxVector3 computeRotationAxis(const VxVector3 minPoints[3], const VxVector3 maxPoints[3],
                                          VxReal& magnitude) const
    {
        (void)minPoints; (void)maxPoints; (void)magnitude;
        return VxVector3();
    }

    VxReal mHeight; // Mac +0x84, Win +0x88
    VxReal mRadius; // Mac +0x8c, Win +0x90
};

// Mac vtable 0x75e500
class OrientedCapsule : public OrientedAxialVolume {
public:
    OrientedCapsule(const VxTransform& tm, VxReal height, VxReal radius) : OrientedAxialVolume(tm, height, radius) {}
    // Mac 0x46c8fe, Win 0x76aa70
    virtual VxReal volume() const { return mRadius * 3.141592653589793 * mRadius * (mRadius * 1.3333333333333333 + mHeight); }
    virtual void computeExtents(const VxArray<VxVector3>& points, bool keepCenter, VxVector3 maxPoints[3],
                                VxVector3 minPoints[3]);
    virtual void computeSmallestEnclosingPCA(const VxArray<VxVector3>& points, bool keepCenter);
    virtual VxVector3 computeRotationAxis(const VxVector3 minPoints[3], const VxVector3 maxPoints[3],
                                          VxReal& magnitude) const;
};

// Mac vtable 0x75e580
class OrientedCylinder : public OrientedAxialVolume {
public:
    OrientedCylinder(const VxTransform& tm, VxReal height, VxReal radius) : OrientedAxialVolume(tm, height, radius) {}
    // Mac 0x46c8c8, Win 0x76aa40
    virtual VxReal volume() const { return mRadius * mRadius * 3.141592653589793 * mHeight; }
    virtual void refine(const VxArray<VxVector3>& points);
    virtual void computeExtents(const VxArray<VxVector3>& points, bool keepCenter, VxVector3 maxPoints[3],
                                VxVector3 minPoints[3]);
    virtual void computeSmallestEnclosingPCA(const VxArray<VxVector3>& points, bool keepCenter);
    virtual VxVector3 computeRotationAxis(const VxVector3 minPoints[3], const VxVector3 maxPoints[3],
                                          VxReal& magnitude) const;
};

// --------------------------------------------------------------------------------------
// OrientedBox

// Mac 0x46da7a, Win 0x76b9d0
void OrientedBox::computeExtents(const VxArray<VxVector3>& points, bool keepCenter, VxVector3 maxPoints[3],
                                 VxVector3 minPoints[3])
{
    VxReal dims[3];
    VxVector3 c;
    if (!keepCenter) {
        for (int i = 0; i < 3; ++i) {
            VxVector3 a = axis(i);
            VxReal maxValue, minValue;
            vxExtremePoints(points, a, maxPoints[i], minPoints[i], maxValue, minValue);
            dims[i] = fabs(maxValue - minValue);
            VxReal m = minValue + fabs(maxValue - minValue) * 0.5;
            c[0] = m * a[0] + c[0];
            c[1] = m * a[1] + c[1];
            c[2] = m * a[2] + c[2];
        }
    } else {
        c = center();
        for (int i = 0; i < 3; ++i) {
            VxVector3 a = axis(i);
            VxReal maxValue, minValue;
            vxExtremePoints(points, a, maxPoints[i], minPoints[i], maxValue, minValue);
            VxReal p = c[0] * a[0] + c[1] * a[1] + c[2] * a[2];
            VxReal d1 = fabs(maxValue - p);
            VxReal d2 = fabs(minValue - p);
            dims[i] = d1 <= d2 ? d2 + d2 : d1 + d1;
        }
    }
    mDims[0] = dims[0];
    mDims[1] = dims[1];
    mDims[2] = dims[2];
    center() = c;
}

// Mac 0x46fdd4, Win 0x76bb60: the principal axes, matched to the nearest world axes
void OrientedBox::computeSmallestEnclosingPCA(const VxArray<VxVector3>& points, bool keepCenter)
{
    VxVector3 mean;
    VxReal axes[3][3];
    VxVector3 eigenvalues;
    VxReal cov[3][3];
    if (!vxComputePrincipalAxes(points, axes, eigenvalues, cov, mean)) {
        VxWarning(0, "OrientedBox::computeSmallestEnclosingPCA: could not perform eigenvector decomposition of "
                     "covariance matrix.\n");
    } else {
        VxTransform identity;
        VxReal best = -DBL_MAX;
        int row = 0, col = 0;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) {
                VxReal d = fabs(identity.m[i][0] * axes[0][j] + identity.m[i][1] * axes[1][j]
                                + identity.m[i][2] * axes[2][j]);
                if (best < d) {
                    row = i;
                    best = d;
                    col = j;
                }
            }
        mTransform.m[row][0] = axes[0][col];
        mTransform.m[row][1] = axes[1][col];
        mTransform.m[row][2] = axes[2][col];
        int row1 = (row + 1) % 3;
        int c1 = (col + 1) % 3;
        int c2 = (col + 2) % 3;
        int c = fabs(axes[0][c2] * identity.m[row1][0] + axes[1][c2] * identity.m[row1][1]
                     + axes[2][c2] * identity.m[row1][2])
                        < fabs(axes[0][c1] * identity.m[row1][0] + axes[1][c1] * identity.m[row1][1]
                               + axes[2][c1] * identity.m[row1][2])
                    ? c1
                    : c2;
        mTransform.m[row1][0] = axes[0][c];
        mTransform.m[row1][1] = axes[1][c];
        mTransform.m[row1][2] = axes[2][c];
        axis((row1 + 1) % 3) = axis(row).cross(axis(row1));
        if (!mTransform.isValid()) {
            VxWarning(0, "OrientedBox::computeSmallestEnclosingPCA: computed transformation matrix is invalid. "
                         "Falling back to identity.\n");
            setIdentity(mTransform);
        }
    }
    VxVector3 maxPoints[3], minPoints[3];
    computeExtents(points, keepCenter, maxPoints, minPoints);
}

// Mac 0x470f4e, Win 0x76bec0: each extreme point pushes with the lever of the other two
// dimensions along its axis (+ at the maximum, - at the minimum)
VxVector3 OrientedBox::computeRotationAxis(const VxVector3 minPoints[3], const VxVector3 maxPoints[3],
                                           VxReal& magnitude) const
{
    const VxVector3 c = center();
    const VxReal lever[3] = {mDims[1] + mDims[2], mDims[2] + mDims[0], mDims[1] + mDims[0]};
    VxVector3 t;
    for (int i = 0; i < 3; ++i) {
        const VxVector3& a = axis(i);
        VxVector3 f(lever[i] * a[0], lever[i] * a[1], lever[i] * a[2]);
        t += f.cross(maxPoints[i] - c);
        VxReal s = -lever[i];
        VxVector3 g(s * a[0], s * a[1], s * a[2]);
        t += g.cross(minPoints[i] - c);
    }
    magnitude = sqrt(t[0] * t[0] + t[1] * t[1] + t[2] * t[2]);
    if (magnitude > 1e-14) {
        VxReal inv = 1.0 / magnitude;
        t = VxVector3(inv * t[0], inv * t[1], inv * t[2]);
    }
    return t;
}

// Mac 0x472618, Win 0x76c390: lockAxis -1 (free) or 0-2 (rotate about that axis only)
void OrientedBox::computeSmallestEnclosingIterative(const VxArray<VxVector3>& points, int lockAxis, bool keepCenter)
{
    if (lockAxis > 2) {
        VxWarning(0, "OrientedBox::computeSmallestEnclosingIterative: locking all axes is not supported.\n");
        return;
    }
    if (volume() < 1e-14)
        return;
    VxReal bestSurface = halfSurface();
    VxVector3 maxPoints[3], minPoints[3];
    for (int i = 0; i < 3; ++i) {
        VxReal maxValue, minValue;
        vxExtremePoints(points, axis(i), maxPoints[i], minPoints[i], maxValue, minValue);
    }
    // 1. descent: turn about the torque axis (or the locked axis), shrinking the step when the
    // surface grows
    VxVector3 w;
    VxReal angle = -0.1;
    OrientedBox best(*this);
    OrientedBox work(*this);
    VxReal previous = bestSurface;
    for (int iteration = 0; iteration < 35; ++iteration) {
        if (lockAxis < 0) {
            VxReal magnitude;
            w = work.computeRotationAxis(minPoints, maxPoints, magnitude);
            if (magnitude < 1e-14) {
                // stuck: a random axis
                angle = -0.1;
                w = randomUnitVector();
            }
        } else {
            w = axis(lockAxis);
        }
        VxQuaternion q(w, angle);
        for (int i = 0; i < 3; ++i) {
            VxVector3& a = work.axis(i);
            a = q.rotate(a);
        }
        work.computeExtents(points, keepCenter, maxPoints, minPoints);
        VxReal s = work.halfSurface();
        if (bestSurface > s) {
            best = work;
            bestSurface = s;
        }
        if (s > previous)
            angle = angle * 0.6666666666666666;
        previous = s;
    }
    // 2. per axis: 5 samples 0.1 pi apart, then a golden-section search around the best one
    OrientedBox base(best);
    for (int i = 0; i < 3; ++i) {
        if (!(lockAxis < 0 || lockAxis == i))
            continue;
        w = base.axis(i);
        int bestSample = 0;
        for (int k = 0; k < 5; ++k) {
            angle = (VxReal)k * 0.3141592653589793;
            work = base;
            rotateVolume(points, w, &work, angle, keepCenter);
            VxReal s = work.halfSurface();
            if (bestSurface > s) {
                best = work;
                bestSurface = s;
                bestSample = k;
            }
        }
        const VxReal sampled = bestSurface;
        VxReal a = (VxReal)bestSample * 0.3141592653589793;
        VxReal lo = (VxReal)(bestSample - 1) * 0.3141592653589793;
        // NOTE: the bracket ends are evaluated (rotated) but their surfaces are never used
        work = base;
        rotateVolume(points, w, &work, lo, keepCenter);
        VxReal hi = (VxReal)(bestSample + 1) * 0.3141592653589793;
        work = base;
        rotateVolume(points, w, &work, hi, keepCenter);
        VxReal b = 0.10053096491487339 + a;
        work = base;
        rotateVolume(points, w, &work, b, keepCenter);
        VxReal fb = work.halfSurface();
        VxReal fa = bestSurface;
        if (fabs(hi - lo) > 0.05) {
            fa = bestSurface;
            int n = 0;
            do {
                if (fa > fb) {
                    VxReal x = 0.68 * b + 0.32 * hi;
                    work = base;
                    rotateVolume(points, w, &work, x, keepCenter);
                    VxReal fx = work.halfSurface();
                    fa = fb;
                    lo = a;
                    a = b;
                    b = x;
                    fb = fx;
                } else {
                    VxReal x = 0.68 * a + 0.32 * lo;
                    work = base;
                    rotateVolume(points, w, &work, x, keepCenter);
                    VxReal fx = work.halfSurface();
                    hi = b;
                    b = a;
                    a = x;
                    fb = fa;
                    fa = fx;
                }
                ++n;
            } while (fabs(hi - lo) > 0.05 && n < 15);
        }
        angle = fa < fb ? a : b;
        work = base;
        rotateVolume(points, w, &work, angle, keepCenter);
        VxReal s = work.halfSurface();
        if (sampled > s) {
            base = work;
            best = work;
            bestSurface = s;
        } else {
            base = best;
        }
    }
    mTransform = best.mTransform;
    mDims[0] = best.mDims[0];
    mDims[1] = best.mDims[1];
    mDims[2] = best.mDims[2];
}

// --------------------------------------------------------------------------------------
// OrientedAxialVolume

// Mac 0x4715f6, Win 0x76ede0: turn about the torque axis by a tenth of its magnitude, keeping
// the frame with the smallest volume
void OrientedAxialVolume::computeSmallestEnclosingIterative(const VxArray<VxVector3>& points, int lockAxis,
                                                            bool keepCenter)
{
    if (lockAxis != -1) {
        VxWarning(0, "OrientedAxialVolume::computeSmallestEnclosingIterative: locking axes is not supported.\n");
        return;
    }
    VxReal bestVolume = volume();
    if (bestVolume < 1e-14)
        return;
    VxVector3 maxPoints[3], minPoints[3];
    for (int i = 0; i < 3; ++i) {
        VxReal maxValue, minValue;
        vxExtremePoints(points, axis(i), maxPoints[i], minPoints[i], maxValue, minValue);
    }
    VxVector3 w;
    OrientedAxialVolume best(*this);
    for (int iteration = 0; iteration < 35; ++iteration) {
        VxReal magnitude;
        w = computeRotationAxis(minPoints, maxPoints, magnitude);
        if (magnitude < 1e-14)
            break;
        VxReal angle = magnitude * 0.1;
        VxQuaternion q(w, angle);
        for (int i = 0; i < 3; ++i) {
            VxVector3& a = axis(i);
            a = q.rotate(a);
        }
        computeExtents(points, keepCenter, maxPoints, minPoints);
        if (keepCenter) {
            // the extents with a kept centre do not refresh the side axes' extreme points
            VxReal maxValue, minValue;
            vxExtremePoints(points, axis(0), maxPoints[0], minPoints[0], maxValue, minValue);
            vxExtremePoints(points, axis(1), maxPoints[1], minPoints[1], maxValue, minValue);
        }
        VxReal v = volume();
        if (v < bestVolume) {
            best.mTransform = mTransform;
            best.mHeight = mHeight;
            best.mRadius = mRadius;
            bestVolume = v;
        }
    }
    mTransform = best.mTransform;
    mHeight = best.mHeight;
    mRadius = best.mRadius;
}

// --------------------------------------------------------------------------------------
// OrientedCapsule

// Mac 0x46f920, Win 0x76b390: radius = the farthest point from the axis line; the core from the
// points' reach along the axis inside that radius. NOTE: the extreme-point arrays are not
// written.
void OrientedCapsule::computeExtents(const VxArray<VxVector3>& points, bool keepCenter, VxVector3 maxPoints[3],
                                     VxVector3 minPoints[3])
{
    (void)maxPoints;
    (void)minPoints;
    VxVector3 c;
    if (!keepCenter) {
        for (const VxVector3* p = points.begin(); p != points.end(); ++p) {
            c[0] = c[0] + (*p)[0];
            c[1] = c[1] + (*p)[1];
            c[2] = c[2] + (*p)[2];
        }
        VxReal inv = 1.0 / (VxReal)(unsigned int)points.size();
        c[0] = c[0] * inv;
        c[1] = c[1] * inv;
        c[2] = inv * c[2];
    } else {
        c = center();
    }
    VxVector3 a = axis(2);
    VxVector3 farthest, foot;
    VxReal r = vxFarthestPointFromLine(points, c, a, farthest, foot);
    VxReal lo = -DBL_MAX;
    VxReal hi = DBL_MAX;
    for (const VxVector3* p = points.begin(); p != points.end(); ++p) {
        VxReal dz = (*p)[2] - c[2];
        VxReal dy = (*p)[1] - c[1];
        VxReal dx = (*p)[0] - c[0];
        VxReal x = mTransform.m[0][0] * dx + mTransform.m[0][1] * dy + mTransform.m[0][2] * dz;
        VxReal y = mTransform.m[1][0] * dx + mTransform.m[1][1] * dy + mTransform.m[1][2] * dz;
        VxReal t = dx * a[0] + dy * a[1] + dz * a[2];
        VxReal h = sqrt(fabs(r * r - (x * x + y * y)));
        VxReal top = t + h;
        if (hi <= top)
            top = hi;
        hi = top;
        VxReal bottom = t - h;
        if (bottom <= lo)
            bottom = lo;
        lo = bottom;
    }
    mRadius = r;
    if (!keepCenter) {
        VxReal m = (hi + lo) * 0.5;
        center() = VxVector3(m * a[0] + c[0], m * a[1] + c[1], m * a[2] + c[2]);
        mHeight = lo - hi;
    } else {
        VxReal l = fabs(lo);
        VxReal u = fabs(hi);
        mHeight = l <= u ? u + u : l + l;
    }
}

// Mac 0x470398, Win 0x76b880: axis 2 = the principal axis, axis 0 = the second one
void OrientedCapsule::computeSmallestEnclosingPCA(const VxArray<VxVector3>& points, bool keepCenter)
{
    VxVector3 mean;
    VxReal axes[3][3];
    VxVector3 eigenvalues;
    VxReal cov[3][3];
    if (!vxComputePrincipalAxes(points, axes, eigenvalues, cov, mean)) {
        VxWarning(0, "OrientedCapsule::computeSmallestEnclosingPCA: could not perform eigenvector decomposition of "
                     "covariance matrix.\n");
    } else {
        mTransform.m[2][0] = axes[0][0];
        mTransform.m[2][1] = axes[1][0];
        mTransform.m[2][2] = axes[2][0];
        mTransform.m[0][0] = axes[0][1];
        mTransform.m[0][1] = axes[1][1];
        mTransform.m[0][2] = axes[2][1];
        axis(1) = axis(2).cross(axis(0));
        if (!mTransform.isValid()) {
            VxWarning(0, "OrientedCapsule::computeSmallestEnclosingPCA: computed transformation matrix is invalid. "
                         "Falling back to identity.\n");
            setIdentity(mTransform);
        }
    }
    VxVector3 maxPoints[3], minPoints[3];
    computeExtents(points, keepCenter, maxPoints, minPoints);
}

// Mac 0x4705d6, Win 0x76b650: the extreme points' offsets from the core segment (centre -+
// axis * height) about the centre
VxVector3 OrientedCapsule::computeRotationAxis(const VxVector3 minPoints[3], const VxVector3 maxPoints[3],
                                               VxReal& magnitude) const
{
    const VxVector3 c = center();
    const VxVector3& a = axis(2);
    VxVector3 h(a[0] * mHeight, a[1] * mHeight, a[2] * mHeight);
    VxVector3 p0(c[0] - h[0], c[1] - h[1], c[2] - h[2]);
    VxVector3 p1(c[0] + h[0], c[1] + h[1], c[2] + h[2]);
    VxVector3 t;
    VxVector3 closest;
    for (int i = 0; i < 3; ++i) {
        vxPointSegmentDistance(minPoints[i], p0, p1, closest);
        t += (minPoints[i] - closest).cross(minPoints[i] - c);
        vxPointSegmentDistance(maxPoints[i], p0, p1, closest);
        t += (maxPoints[i] - closest).cross(maxPoints[i] - c);
    }
    magnitude = sqrt(t[0] * t[0] + t[1] * t[1] + t[2] * t[2]);
    if (magnitude > 1e-14) {
        VxReal inv = 1.0 / magnitude;
        t = VxVector3(inv * t[0], inv * t[1], inv * t[2]);
    }
    return t;
}

// --------------------------------------------------------------------------------------
// OrientedCylinder

// Mac 0x46f5ec, Win 0x76adb0: move the centre towards the farthest point from the axis while
// the radius beyond mRadius shrinks (35 steps, halving the step after a worse one)
void OrientedCylinder::refine(const VxArray<VxVector3>& points)
{
    VxVector3 farthest, foot;
    VxVector3 previous = center();
    const VxVector3 a = axis(2);
    const VxReal r0 = mRadius;
    VxReal best = DBL_MAX;
    VxReal step = 0.5;
    VxVector3 c = previous;
    for (int i = 0; i < 35; ++i) {
        VxReal d = vxFarthestPointFromLine(points, c, a, farthest, foot);
        VxReal e = d - r0;
        if (e < 1e-14)
            break;
        if (best < e) {
            c = previous;
            step = step * 0.5;
        } else {
            previous = c;
            VxReal k = (step * e) / d;
            c[0] = c[0] + k * (farthest[0] - foot[0]);
            c[1] = c[1] + k * (farthest[1] - foot[1]);
            c[2] = c[2] + k * (farthest[2] - foot[2]);
            bool better = e < best;
            best = e;
            if (better)
                step = 0.5;
        }
    }
    if (best < vxFarthestPointFromLine(points, c, a, farthest, foot) - r0)
        c = previous;
    mRadius = vxFarthestPointFromLine(points, c, a, farthest, foot);
    center() = c;
}

// Mac 0x46deb0, Win 0x76afa0: radius from the side extents (then refine), height from axis 2
void OrientedCylinder::computeExtents(const VxArray<VxVector3>& points, bool keepCenter, VxVector3 maxPoints[3],
                                      VxVector3 minPoints[3])
{
    VxVector3 c;
    VxReal maxValue, minValue;
    if (!keepCenter) {
        VxReal half[2];
        for (int i = 0; i < 2; ++i) {
            VxVector3 a = axis(i);
            vxExtremePoints(points, a, maxPoints[i], minPoints[i], maxValue, minValue);
            half[i] = fabs(maxValue - minValue) * 0.5;
            VxReal m = minValue + half[i];
            c[0] = c[0] + m * a[0];
            c[1] = c[1] + m * a[1];
            c[2] = c[2] + m * a[2];
        }
        VxVector3 a = axis(2);
        vxExtremePoints(points, a, maxPoints[2], minPoints[2], maxValue, minValue);
        mHeight = fabs(maxValue - minValue);
        VxReal m = fabs(maxValue - minValue) * 0.5 + minValue;
        c[0] = c[0] + m * a[0];
        c[1] = m * a[1] + c[1];
        c[2] = c[2] + m * a[2];
        center() = c;
        mRadius = half[1] <= half[0] ? half[0] : half[1];
        refine(points);
    } else {
        c = center();
        VxVector3 a = axis(2);
        vxExtremePoints(points, a, maxPoints[2], minPoints[2], maxValue, minValue);
        VxReal p = c[0] * a[0] + c[1] * a[1] + c[2] * a[2];
        VxReal d1 = fabs(maxValue - p);
        VxReal d2 = fabs(minValue - p);
        mHeight = d1 <= d2 ? d2 + d2 : d1 + d1;
        VxVector3 farthest, foot;
        mRadius = vxFarthestPointFromLine(points, c, a, farthest, foot);
    }
}

// Mac 0x470976, Win 0x76b1e0: axis 2 = the eigenvector whose eigenvalue stands apart most
void OrientedCylinder::computeSmallestEnclosingPCA(const VxArray<VxVector3>& points, bool keepCenter)
{
    VxVector3 mean;
    VxReal axes[3][3];
    VxVector3 e;
    VxReal cov[3][3];
    if (!vxComputePrincipalAxes(points, axes, e, cov, mean)) {
        VxWarning(0, "OrientedCylinder::computeSmallestEnclosingPCA: could not perform eigenvector decomposition of "
                     "covariance matrix.\n");
    } else {
        unsigned int k;
        if (fabs(e[0] - e[2]) <= fabs(e[0] - e[1]))
            k = fabs(e[0] - e[2]) < fabs(e[1] - e[2]) ? 1 : 0;
        else
            k = fabs(e[0] - e[1]) < fabs(e[1] - e[2]) ? 2 : 0;
        mTransform.m[2][0] = axes[0][k];
        mTransform.m[2][1] = axes[1][k];
        mTransform.m[2][2] = axes[2][k];
        k = (k + 1) % 3;
        mTransform.m[0][0] = axes[0][k];
        mTransform.m[0][1] = axes[1][k];
        mTransform.m[0][2] = axes[2][k];
        axis(1) = axis(2).cross(axis(0));
        if (!mTransform.isValid()) {
            VxWarning(0, "OrientedCylinder::computeSmallestEnclosingPCA: computed transformation matrix is invalid. "
                         "Falling back to identity.\n");
            setIdentity(mTransform);
        }
    }
    VxVector3 maxPoints[3], minPoints[3];
    computeExtents(points, keepCenter, maxPoints, minPoints);
}

// Mac 0x470c34, Win 0x76abd0: as the capsule's, about the axis line
VxVector3 OrientedCylinder::computeRotationAxis(const VxVector3 minPoints[3], const VxVector3 maxPoints[3],
                                                VxReal& magnitude) const
{
    const VxVector3 c = center();
    const VxVector3 a = axis(2);
    VxVector3 t;
    VxVector3 foot;
    for (int i = 0; i < 3; ++i) {
        vxPointLineDistance(minPoints[i], c, a, foot);
        t += (minPoints[i] - foot).cross(minPoints[i] - c);
        vxPointLineDistance(maxPoints[i], c, a, foot);
        t += (maxPoints[i] - foot).cross(maxPoints[i] - c);
    }
    magnitude = sqrt(t[0] * t[0] + t[1] * t[1] + t[2] * t[2]);
    if (magnitude > 1e-14) {
        VxReal inv = 1.0 / magnitude;
        t = VxVector3(inv * t[0], inv * t[1], inv * t[2]);
    }
    return t;
}

// --------------------------------------------------------------------------------------
// The minimal sphere (WildMagic3 MinSphere3; radius squared until the end)

struct Sphere {
    VxVector3 center;
    VxReal radius;
};

class MinSphere {
public:
    // Win 0x76f170 (out of line, unused; inlined into 0x76f720), Mac inlined
    MinSphere() : mEpsilon(1e-14)
    {
        mUpdate[0] = 0;
        mUpdate[1] = &MinSphere::updateSupport1;
        mUpdate[2] = &MinSphere::updateSupport2;
        mUpdate[3] = &MinSphere::updateSupport3;
        mUpdate[4] = &MinSphere::updateSupport4;
    }
    bool compute(const VxVector3* points, int n, Sphere& sphere);

private:
    struct Support {
        // the point i (as a pointer into perm) coincides with a support point
        bool contains(int i, const VxVector3** perm, VxReal epsilon) const
        {
            for (int k = 0; k < quantity; ++k) {
                VxVector3 d = *perm[i] - *perm[index[k]];
                if (d[0] * d[0] + d[1] * d[1] + d[2] * d[2] < epsilon)
                    return true;
            }
            return false;
        }
        int quantity;
        int index[4];
    };
    typedef Sphere (MinSphere::*UpdateFn)(int i, const VxVector3** perm, Support& support);

    // Mac 0x46c97a (EDX = point, ECX = sphere), Win 0x76ab80
    static bool contains(const VxVector3& p, const Sphere& s, VxReal& distDiff)
    {
        VxVector3 d = p - s.center;
        distDiff = (d[0] * d[0] + d[1] * d[1] + d[2] * d[2]) - s.radius;
        return distDiff <= 0.0;
    }
    static Sphere exactSphere1(const VxVector3& p)
    {
        Sphere s;
        s.center = p;
        s.radius = 0.0;
        return s;
    }
    // Mac 0x46c9d2, Win 0x76d690
    static Sphere exactSphere2(const VxVector3& p0, const VxVector3& p1)
    {
        Sphere s;
        s.center = (p0 + p1) * 0.5;
        VxVector3 d = p1 - p0;
        s.radius = (d[0] * d[0] + d[1] * d[1] + d[2] * d[2]) * 0.25;
        return s;
    }
    Sphere exactSphere3(const VxVector3& p0, const VxVector3& p1, const VxVector3& p2);
    Sphere exactSphere4(const VxVector3& p0, const VxVector3& p1, const VxVector3& p2, const VxVector3& p3);
    Sphere updateSupport1(int i, const VxVector3** perm, Support& support);
    Sphere updateSupport2(int i, const VxVector3** perm, Support& support);
    Sphere updateSupport3(int i, const VxVector3** perm, Support& support);
    Sphere updateSupport4(int i, const VxVector3** perm, Support& support);

    VxReal mEpsilon;
    UpdateFn mUpdate[5];
};

// Mac 0x46ca9e (EAX = result, EDX = this, ECX = p0; stack p1, p2), Win 0x76d720: the circle
// through three points
Sphere MinSphere::exactSphere3(const VxVector3& p0, const VxVector3& p1, const VxVector3& p2)
{
    VxVector3 a = p0 - p2;
    VxVector3 b = p1 - p2;
    VxReal aa = a[0] * a[0] + a[1] * a[1] + a[2] * a[2];
    VxReal ab = a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    VxReal bb = b[0] * b[0] + b[1] * b[1] + b[2] * b[2];
    VxReal det = aa * bb - ab * ab;
    Sphere s;
    if (fabs(det) > mEpsilon) {
        VxReal halfInvDet = 0.5 / det;
        VxReal u0 = halfInvDet * bb * (aa - ab);
        VxReal u1 = halfInvDet * aa * (bb - ab);
        VxReal u2 = 1.0 - u0 - u1;
        s.center = p0 * u0 + p1 * u1 + p2 * u2;
        VxVector3 t = a * u0 + b * u1;
        s.radius = t[0] * t[0] + t[1] * t[1] + t[2] * t[2];
    } else {
        s.center = VxVector3();
        s.radius = DBL_MAX;
    }
    return s;
}

// Mac 0x46cd50 (EAX = result, EDX = this, ECX = p0; stack p1, p2, p3), Win 0x76d8e0: the
// sphere through four points
Sphere MinSphere::exactSphere4(const VxVector3& p0, const VxVector3& p1, const VxVector3& p2, const VxVector3& p3)
{
    VxVector3 e10 = p0 - p3;
    VxVector3 e20 = p1 - p3;
    VxVector3 e30 = p2 - p3;
    VxReal m[3][3];
    m[0][0] = e10[0] * e10[0] + e10[1] * e10[1] + e10[2] * e10[2];
    m[0][1] = e10[0] * e20[0] + e10[1] * e20[1] + e10[2] * e20[2];
    m[0][2] = e10[0] * e30[0] + e10[1] * e30[1] + e10[2] * e30[2];
    m[1][0] = m[0][1];
    m[1][1] = e20[0] * e20[0] + e20[1] * e20[1] + e20[2] * e20[2];
    m[1][2] = e20[0] * e30[0] + e20[1] * e30[1] + e20[2] * e30[2];
    m[2][0] = m[0][2];
    m[2][1] = m[1][2];
    m[2][2] = e30[0] * e30[0] + e30[1] * e30[1] + e30[2] * e30[2];
    VxReal rhs[3];
    rhs[0] = 0.5 * m[0][0];
    rhs[1] = 0.5 * m[1][1];
    rhs[2] = 0.5 * m[2][2];
    VxReal inv[3][3];
    inv[0][0] = m[1][1] * m[2][2] - m[1][2] * m[2][1];
    inv[0][1] = m[0][2] * m[2][1] - m[0][1] * m[2][2];
    inv[0][2] = m[0][1] * m[1][2] - m[0][2] * m[1][1];
    inv[1][0] = m[1][2] * m[2][0] - m[1][0] * m[2][2];
    inv[1][1] = m[0][0] * m[2][2] - m[0][2] * m[2][0];
    inv[1][2] = m[0][2] * m[1][0] - m[0][0] * m[1][2];
    inv[2][0] = m[1][0] * m[2][1] - m[1][1] * m[2][0];
    inv[2][1] = m[0][1] * m[2][0] - m[0][0] * m[2][1];
    inv[2][2] = m[0][0] * m[1][1] - m[0][1] * m[1][0];
    VxReal det = m[0][0] * inv[0][0] + m[0][1] * inv[1][0] + m[0][2] * inv[2][0];
    Sphere s;
    if (fabs(det) > mEpsilon) {
        VxReal invDet = 1.0 / det;
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c)
                inv[r][c] *= invDet;
        VxReal u[4];
        for (int r = 0; r < 3; ++r) {
            u[r] = 0.0;
            for (int c = 0; c < 3; ++c)
                u[r] += inv[r][c] * rhs[c];
        }
        u[3] = 1.0 - u[0] - u[1] - u[2];
        s.center = p0 * u[0] + p1 * u[1] + p2 * u[2] + p3 * u[3];
        VxVector3 t = e10 * u[0] + e20 * u[1] + e30 * u[2];
        s.radius = t[0] * t[0] + t[1] * t[1] + t[2] * t[2];
    } else {
        s.center = VxVector3();
        s.radius = DBL_MAX;
    }
    return s;
}

// Mac 0x46d39c, Win 0x76d650
Sphere MinSphere::updateSupport1(int i, const VxVector3** perm, Support& support)
{
    const VxVector3& p0 = *perm[support.index[0]];
    const VxVector3& p1 = *perm[i];
    Sphere s = exactSphere2(p0, p1);
    support.quantity = 2;
    support.index[1] = i;
    return s;
}

// Mac 0x46ea6c, Win 0x76dd00
Sphere MinSphere::updateSupport2(int i, const VxVector3** perm, Support& support)
{
    const VxVector3& p0 = *perm[support.index[0]];
    const VxVector3& p1 = *perm[support.index[1]];
    const VxVector3& p2 = *perm[i];
    Sphere spheres[3];
    VxReal minRadius = DBL_MAX;
    VxReal distDiff;
    int index = -1;
    spheres[0] = exactSphere2(p0, p2);
    if (contains(p1, spheres[0], distDiff)) {
        minRadius = spheres[0].radius;
        index = 0;
    }
    spheres[1] = exactSphere2(p1, p2);
    if (spheres[1].radius < minRadius && contains(p0, spheres[1], distDiff)) {
        minRadius = spheres[1].radius;
        index = 1;
    }
    Sphere minimal;
    if (index != -1) {
        minimal = spheres[index];
        support.index[1 - index] = i;
    } else {
        minimal = exactSphere3(p0, p1, p2);
        VxAssert(minimal.radius <= minRadius,
                 "MinSphere::updateSupport2: radius resulting from exactSphere3() larger than current minimal radius.\n");
        support.quantity = 3;
        support.index[2] = i;
    }
    return minimal;
}

// Mac 0x46e29c, Win 0x76def0
Sphere MinSphere::updateSupport3(int i, const VxVector3** perm, Support& support)
{
    const VxVector3& p0 = *perm[support.index[0]];
    const VxVector3& p1 = *perm[support.index[1]];
    const VxVector3& p2 = *perm[support.index[2]];
    const VxVector3& p3 = *perm[i];
    Sphere spheres[6];
    VxReal minRadius = DBL_MAX;
    VxReal distDiff;
    int index = -1;
    spheres[0] = exactSphere2(p0, p3);
    if (contains(p1, spheres[0], distDiff) && contains(p2, spheres[0], distDiff)) {
        minRadius = spheres[0].radius;
        index = 0;
    }
    spheres[1] = exactSphere2(p1, p3);
    if (spheres[1].radius < minRadius && contains(p0, spheres[1], distDiff) && contains(p2, spheres[1], distDiff)) {
        minRadius = spheres[1].radius;
        index = 1;
    }
    spheres[2] = exactSphere2(p2, p3);
    if (spheres[2].radius < minRadius && contains(p0, spheres[2], distDiff) && contains(p1, spheres[2], distDiff)) {
        minRadius = spheres[2].radius;
        index = 2;
    }
    spheres[3] = exactSphere3(p0, p1, p3);
    if (spheres[3].radius < minRadius && contains(p2, spheres[3], distDiff)) {
        minRadius = spheres[3].radius;
        index = 3;
    }
    spheres[4] = exactSphere3(p0, p2, p3);
    if (spheres[4].radius < minRadius && contains(p1, spheres[4], distDiff)) {
        minRadius = spheres[4].radius;
        index = 4;
    }
    spheres[5] = exactSphere3(p1, p2, p3);
    if (spheres[5].radius < minRadius && contains(p0, spheres[5], distDiff)) {
        minRadius = spheres[5].radius;
        index = 5;
    }
    Sphere minimal;
    switch (index) {
    case 0:
        minimal = spheres[0];
        support.quantity = 2;
        support.index[1] = i;
        break;
    case 1:
        minimal = spheres[1];
        support.quantity = 2;
        support.index[0] = i;
        break;
    case 2:
        minimal = spheres[2];
        support.quantity = 2;
        support.index[0] = support.index[2];
        support.index[1] = i;
        break;
    case 3:
        minimal = spheres[3];
        support.index[2] = i;
        break;
    case 4:
        minimal = spheres[4];
        support.index[1] = i;
        break;
    case 5:
        minimal = spheres[5];
        support.index[0] = i;
        break;
    default:
        minimal = exactSphere4(p0, p1, p2, p3);
        VxAssert(minimal.radius <= minRadius,
                 "MinSphere::updateSupport3: radius resulting from exactSphere4() larger than current minimal radius.\n");
        support.quantity = 4;
        support.index[3] = i;
        break;
    }
    return minimal;
}

// Mac 0x46ed56 (tables 0x6441a0 / 0x644140 / 0x644100), Win 0x76e3f0
Sphere MinSphere::updateSupport4(int i, const VxVector3** perm, Support& support)
{
    const VxVector3* pt[4] = {perm[support.index[0]], perm[support.index[1]], perm[support.index[2]],
                              perm[support.index[3]]};
    const VxVector3& p4 = *perm[i];
    // permutations of type 1, 2, 3 (the new point plus 1, 2 or 3 support points)
    int t1[4][4] = {{0, 1, 2, 3}, {1, 0, 2, 3}, {2, 0, 1, 3}, {3, 0, 1, 2}};
    int t2[6][4] = {{0, 1, 2, 3}, {0, 2, 1, 3}, {0, 3, 1, 2}, {1, 2, 0, 3}, {1, 3, 0, 2}, {2, 3, 0, 1}};
    int t3[4][4] = {{0, 1, 2, 3}, {0, 1, 3, 2}, {0, 2, 3, 1}, {1, 2, 3, 0}};
    Sphere spheres[14];
    VxReal minRadius = DBL_MAX;
    int index = -1;
    VxReal distDiff, minDistDiff = DBL_MAX;
    int minIndex = -1;
    int k = 0;
    for (int j = 0; j < 4; ++j, ++k) {
        spheres[k] = exactSphere2(*pt[t1[j][0]], p4);
        if (spheres[k].radius < minRadius) {
            if (contains(*pt[t1[j][1]], spheres[k], distDiff) && contains(*pt[t1[j][2]], spheres[k], distDiff)
                && contains(*pt[t1[j][3]], spheres[k], distDiff)) {
                minRadius = spheres[k].radius;
                index = k;
            } else if (distDiff < minDistDiff) {
                minDistDiff = distDiff;
                minIndex = k;
            }
        }
    }
    for (int j = 0; j < 6; ++j, ++k) {
        spheres[k] = exactSphere3(*pt[t2[j][0]], *pt[t2[j][1]], p4);
        if (spheres[k].radius < minRadius) {
            if (contains(*pt[t2[j][2]], spheres[k], distDiff) && contains(*pt[t2[j][3]], spheres[k], distDiff)) {
                minRadius = spheres[k].radius;
                index = k;
            } else if (distDiff < minDistDiff) {
                minDistDiff = distDiff;
                minIndex = k;
            }
        }
    }
    for (int j = 0; j < 4; ++j, ++k) {
        spheres[k] = exactSphere4(*pt[t3[j][0]], *pt[t3[j][1]], *pt[t3[j][2]], p4);
        if (spheres[k].radius < minRadius) {
            if (contains(*pt[t3[j][3]], spheres[k], distDiff)) {
                minRadius = spheres[k].radius;
                index = k;
            } else if (distDiff < minDistDiff) {
                minDistDiff = distDiff;
                minIndex = k;
            }
        }
    }
    // NOTE: round-off can leave every candidate failing; the least violating one is taken then
    // (and with none at all, index -1 reads before the array, as in the original)
    if (index == -1)
        index = minIndex;
    Sphere minimal = spheres[index];
    switch (index) {
    case 0:
        support.quantity = 2;
        support.index[1] = i;
        break;
    case 1:
        support.quantity = 2;
        support.index[0] = i;
        break;
    case 2:
        support.quantity = 2;
        support.index[0] = support.index[2];
        support.index[1] = i;
        break;
    case 3:
        support.quantity = 2;
        support.index[0] = support.index[3];
        support.index[1] = i;
        break;
    case 4:
        support.quantity = 3;
        support.index[2] = i;
        break;
    case 5:
        support.quantity = 3;
        support.index[1] = i;
        break;
    case 6:
        support.quantity = 3;
        support.index[1] = support.index[3];
        support.index[2] = i;
        break;
    case 7:
        support.quantity = 3;
        support.index[0] = i;
        break;
    case 8:
        support.quantity = 3;
        support.index[0] = support.index[3];
        support.index[2] = i;
        break;
    case 9:
        support.quantity = 3;
        support.index[0] = support.index[3];
        support.index[1] = i;
        break;
    case 10:
        support.index[3] = i;
        break;
    case 11:
        support.index[2] = i;
        break;
    case 12:
        support.index[1] = i;
        break;
    case 13:
        support.index[0] = i;
        break;
    }
    return minimal;
}

// Mac inlined into 0x46d406, Win 0x76f1b0: the minimal sphere of the points in random order;
// sphere is written only when the squared radius is finite. NOTE: the messages name it
// "MinSphere::MinSphere" (Win: a member called after the inlined constructor). (name inferred)
bool MinSphere::compute(const VxVector3* points, int n, Sphere& sphere)
{
    if (n < 1) {
        VxWarning(0, "MinSphere::MinSphere: insufficient number of input points.\n");
        return false;
    }
    Sphere minimal;
    {
        const VxVector3** perm = new const VxVector3*[n];
        for (int i = 0; i < n; ++i)
            perm[i] = &points[i];
        for (int i = n - 1; i > 0; --i) {
            int j = rand() % (i + 1);
            if (j != i) {
                const VxVector3* t = perm[i];
                perm[i] = perm[j];
                perm[j] = t;
            }
        }
        minimal = exactSphere1(*perm[0]);
        Support support;
        support.quantity = 1;
        support.index[0] = 0;
        int i = 1;
        while (i < n) {
            if (!support.contains(i, perm, mEpsilon)) {
                VxReal distDiff;
                if (!contains(*perm[i], minimal, distDiff)) {
                    UpdateFn update = mUpdate[support.quantity];
                    Sphere s = (this->*update)(i, perm, support);
                    if (s.radius > minimal.radius) {
                        minimal = s;
                        i = 0;
                        continue;
                    }
                }
            }
            ++i;
        }
        delete[] perm;
    }
    if (!(minimal.radius < DBL_MAX)) {
        VxWarning(0, "MinSphere::MinSphere: minimal sphere's computed squared radius is infinity.\n");
        return false;
    }
    sphere.radius = sqrt(minimal.radius);
    sphere.center = minimal.center;
    return true;
}

// Win 0x76f720 (cdecl; Mac inlined): the minimal sphere of the points (name inferred)
bool computeMinimalSphere(const VxArray<VxVector3>& points, Sphere& sphere)
{
    MinSphere ms;
    return ms.compute(points.begin(), points.size(), sphere);
}

// Mac inlined into 0x46d406, Win 0x76ab20 (cdecl): the radius about the sphere's kept centre (the
// messages call it VxSphere::_computeSmallestEnclosingRadius)
void computeSmallestEnclosingRadius(const VxArray<VxVector3>& points, Sphere& sphere)
{
    VxVector3 farthest;
    VxReal r = vxFarthestPoint(points, sphere.center, farthest);
    if (r < 1e-14) {
        VxWarning(0, "VxSphere::_computeSmallestEnclosingRadius: computed sphere radius invalid. enforcing minimum "
                     "radius.\n");
        r = 1e-14;
    }
    sphere.radius = r;
}

} // namespace

namespace Vx {

// --------------------------------------------------------------------------------------
// The fits (private members; the capsule / cylinder / sphere ones have VxArray overloads on Win)

// Mac 0x46d406, Win 0x76f790
void VxSphere::_computeSmallestEnclosing(const VxVector3* points, int count, VxTransform& transform, bool keepCenter)
{
    PointArray view(points, count);
    if (view.empty()) {
        VxWarning(0, "VxSphere::computeSmallestEnclosing: insufficient number of points (%d) specified.\n", count);
        return;
    }
    Sphere s;
    s.radius = getRadius();
    s.center = transform.getTranslation();
    if (!keepCenter)
        computeMinimalSphere(view.array(), s);
    else
        computeSmallestEnclosingRadius(view.array(), s);
    if (s.radius < 1e-14) {
        VxWarning(0, "VxSphere::computeSmallestEnclosing: computed sphere radius invalid. enforcing minimum radius.\n");
        s.radius = 1e-14;
    }
    setRadius(s.radius);
    transform.setTranslation(s.center);
}

// Win 0x76f9e0 (Win only)
void VxSphere::_computeSmallestEnclosing(const VxArray<VxVector3>& points, VxTransform& transform, bool keepCenter)
{
    _computeSmallestEnclosing(points.begin(), points.size(), transform, keepCenter);
}

// Mac 0x471cf0, Win 0x76f570
void VxCapsule::_computeSmallestEnclosing(const VxVector3* points, int count, VxTransform& transform, bool keepAxes,
                                          bool keepCenter)
{
    PointArray view(points, count);
    if (view.empty()) {
        VxWarning(0, "VxCapsule::computeSmallestEnclosing: insufficient number of points (%d) specified.\n", count);
        return;
    }
    VxReal h = getHeight();
    VxReal r = getRadius();
    OrientedCapsule v(transform, h, r);
    if (!keepAxes) {
        v.OrientedCapsule::computeSmallestEnclosingPCA(view.array(), keepCenter);
        v.OrientedAxialVolume::computeSmallestEnclosingIterative(view.array(), -1, keepCenter);
    } else {
        VxVector3 maxPoints[3], minPoints[3];
        v.OrientedCapsule::computeExtents(view.array(), keepCenter, maxPoints, minPoints);
    }
    if (v.mRadius < 1e-14 || v.mHeight < 1e-14) {
        VxWarning(0, "VxCapsule::computeSmallestEnclosing: computed capsule dimensions invalid. enforcing minimum "
                     "dimensions.\n");
        v.mRadius = 1e-14 <= v.mRadius ? v.mRadius : 1e-14;
        v.mHeight = 1e-14 <= v.mHeight ? v.mHeight : 1e-14;
    }
    setRadius(v.mRadius);
    setHeight(v.mHeight);
    transform = v.mTransform;
}

// Win 0x76f960 (Win only)
void VxCapsule::_computeSmallestEnclosing(const VxArray<VxVector3>& points, VxTransform& transform, bool keepAxes,
                                          bool keepCenter)
{
    _computeSmallestEnclosing(points.begin(), points.size(), transform, keepAxes, keepCenter);
}

// Mac 0x472184, Win 0x76f3a0
void VxCylinder::_computeSmallestEnclosing(const VxVector3* points, int count, VxTransform& transform, bool keepAxes,
                                           bool keepCenter)
{
    PointArray view(points, count);
    if (view.empty()) {
        VxWarning(0, "VxCylinder::computeSmallestEnclosing: insufficient number of points (%d) specified.\n", count);
        return;
    }
    VxReal h = getHeight();
    VxReal r = getRadius();
    OrientedCylinder v(transform, h, r);
    if (!keepAxes) {
        v.OrientedCylinder::computeSmallestEnclosingPCA(view.array(), keepCenter);
        v.OrientedAxialVolume::computeSmallestEnclosingIterative(view.array(), -1, keepCenter);
    } else {
        VxVector3 maxPoints[3], minPoints[3];
        v.OrientedCylinder::computeExtents(view.array(), keepCenter, maxPoints, minPoints);
    }
    if (v.mRadius < 1e-14 || v.mHeight < 1e-14) {
        VxWarning(0, "VxCylinder::computeSmallestEnclosing: computed cylinder dimensions invalid. enforcing minimum "
                     "dimensions.\n");
        v.mRadius = 1e-14 <= v.mRadius ? v.mRadius : 1e-14;
        v.mHeight = 1e-14 <= v.mHeight ? v.mHeight : 1e-14;
    }
    setRadius(v.mRadius);
    setHeight(v.mHeight);
    transform = v.mTransform;
}

// Win 0x76f8d0 (Win only)
void VxCylinder::_computeSmallestEnclosing(const VxArray<VxVector3>& points, VxTransform& transform, bool keepAxes,
                                           bool keepCenter)
{
    _computeSmallestEnclosing(points.begin(), points.size(), transform, keepAxes, keepCenter);
}

// --------------------------------------------------------------------------------------
// The overrides. The VxArray overloads (Mac slot 10, Win the later slot) forward to the pointer
// overloads through the vtable. `flag` keeps the transform's position (and, for the sphere,
// fits the radius only).

// Mac 0x46f4fc, Win 0x76eb50
void VxBox::computeSmallestEnclosing(const VxArray<VxVector3>& points, VxTransform& transform, int lockAxis, bool flag)
{
    computeSmallestEnclosing(points.begin(), points.size(), transform, lockAxis, flag);
}

// Mac 0x474232, Win 0x76eb90: lockAxis -1 = free, 0-2 = rotate about that axis only, 3 = keep
// the axes (extents only)
void VxBox::computeSmallestEnclosing(const VxVector3* points, int count, VxTransform& transform, int lockAxis, bool flag)
{
    if (!((unsigned int)(lockAxis + 1) < 5)) {
        VxWarning(0, "VxBox::computeSmallestEnclosing: setting lockedAxes to %d is not allowed.\n", lockAxis);
        return;
    }
    PointArray view(points, count);
    if (view.empty()) {
        VxWarning(0, "VxBox::computeSmallestEnclosing: insufficient number of points (%d) specified.\n", count);
        return;
    }
    VxVector3 dims;
    getDimensions(dims);
    OrientedBox box(transform, dims);
    if (lockAxis == 3) {
        VxVector3 maxPoints[3], minPoints[3];
        box.OrientedBox::computeExtents(view.array(), flag, maxPoints, minPoints);
    } else if (lockAxis == -1) {
        box.OrientedBox::computeSmallestEnclosingPCA(view.array(), flag);
        box.OrientedBox::computeSmallestEnclosingIterative(view.array(), -1, flag);
    } else {
        VxVector3 maxPoints[3], minPoints[3];
        box.OrientedBox::computeExtents(view.array(), flag, maxPoints, minPoints);
        box.OrientedBox::computeSmallestEnclosingIterative(view.array(), lockAxis, flag);
    }
    if (box.mDims[0] < 1e-14 || box.mDims[1] < 1e-14 || box.mDims[2] < 1e-14) {
        VxWarning(0, "VxBox::computeSmallestEnclosing: computed box dimensions invalid. enforcing minimum dimensions.\n");
        box.mDims[0] = 1e-14 <= box.mDims[0] ? box.mDims[0] : 1e-14;
        box.mDims[1] = 1e-14 <= box.mDims[1] ? box.mDims[1] : 1e-14;
        box.mDims[2] = 1e-14 <= box.mDims[2] ? box.mDims[2] : 1e-14;
    }
    setDimensions(box.mDims[0], box.mDims[1], box.mDims[2]);
    transform = box.mTransform;
}

// Mac 0x46f4ac, Win 0x76f130
void VxSphere::computeSmallestEnclosing(const VxArray<VxVector3>& points, VxTransform& transform, int lockAxis,
                                        bool flag)
{
    computeSmallestEnclosing(points.begin(), points.size(), transform, lockAxis, flag);
}

// Mac 0x46d9b0, Win 0x76f9a0
void VxSphere::computeSmallestEnclosing(const VxVector3* points, int count, VxTransform& transform, int lockAxis,
                                        bool flag)
{
    if (lockAxis == -1)
        _computeSmallestEnclosing(points, count, transform, flag);
    else
        VxWarning(0, "VxSphere::computeSmallestEnclosing: locking axes is not supported.\n");
}

// Mac 0x46f54c, Win 0x76eb10
void VxCapsule::computeSmallestEnclosing(const VxArray<VxVector3>& points, VxTransform& transform, int lockAxis,
                                         bool flag)
{
    computeSmallestEnclosing(points.begin(), points.size(), transform, lockAxis, flag);
}

// Mac 0x4720e0, Win 0x76f910: lockAxis 2 / 3 keep the axes, -1 = free
void VxCapsule::computeSmallestEnclosing(const VxVector3* points, int count, VxTransform& transform, int lockAxis,
                                         bool flag)
{
    if ((unsigned int)(lockAxis - 2) < 2)
        _computeSmallestEnclosing(points, count, transform, true, flag);
    else if (lockAxis == -1)
        _computeSmallestEnclosing(points, count, transform, false, flag);
    else
        VxWarning(0, "VxCapsule::computeSmallestEnclosing: locking axis %d is not supported.\n", lockAxis);
}

// Mac 0x46f59c, Win 0x76ead0
void VxCylinder::computeSmallestEnclosing(const VxArray<VxVector3>& points, VxTransform& transform, int lockAxis,
                                          bool flag)
{
    computeSmallestEnclosing(points.begin(), points.size(), transform, lockAxis, flag);
}

// Mac 0x472574, Win 0x76f880
void VxCylinder::computeSmallestEnclosing(const VxVector3* points, int count, VxTransform& transform, int lockAxis,
                                          bool flag)
{
    if ((unsigned int)(lockAxis - 2) < 2)
        _computeSmallestEnclosing(points, count, transform, true, flag);
    else if (lockAxis == -1)
        _computeSmallestEnclosing(points, count, transform, false, flag);
    else
        VxWarning(0, "VxCylinder::computeSmallestEnclosing: locking axis %d is not supported.\n", lockAxis);
}

} // namespace Vx
