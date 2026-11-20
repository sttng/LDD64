// Vortex 5.0.0 build 800 "vortexldd41" - VxFluidInteractionUniverseUpdate and the default fluid
// interaction forces of the primitive shapes (restored). Mac TU 486
// 0x44847e-0x453913 (static initialiser 0x45388e: <iostream>, sBoxCellSize), Win 0x744fc0-0x74b7d0
// + dtor 0x70cc60 (the Win-only fluid states in between, 0x7453b0-0x745780, 0x74b3b0-0x74b4e0,
// 0x74b720, 0x74b820-0x74c060, are not restored).
//
// Not reached in LDD (no fluid state is created). applyFluidInteractionForce (dynamic parts) runs
// the force function per fluid state, densest first, until one touches the geometry. Buoyancy =
// submerged volume * density * gravity * data.m38 along mUp, at the submerged centre; drag =
// -0.5 * c * density * |v|^2 * area against the relative velocity (c from data.m18, defaults
// below); lift from the angular velocity (data.m30). Win only: the no-cap capsule / cylinder
// forces (0x749670 / 0x7496d0), vxSetFluidBoxCellSize (0x745820) and the uncalled triple-product
// helpers 0x745030 / 0x745090 / 0x744ff0 (not restored). Mesh forces are not diff-tested
// (Win-only geometry classes). Win calls the shape functions with custom /LTCG conventions.
// Names of locals and helpers inferred.
#include "Vx/VxFluidInteractionUniverseUpdate.h"
#include "Vx/VxFluidState.h"
#include "Vx/VxCollisionGeometry.h"
#include "Vx/VxGeometry.h"
#include "Vx/VxConvexMesh.h"
#include "Vx/VxTriangleMesh.h"
#include "Vx/VxPart.h"
#include "VxCollisionInternal.h"

#include <math.h>
#include "Me/MeCrtMath.h"
#include <algorithm>
#include <iostream>

namespace Vx {

namespace {

const VxReal kPi = 3.141592653589793;

// File statics (Mac __data 0x779328-0x77934f, Win .data 0xabc498-0xabc4bf; not const, so both
// builds read them from memory - Win folded the 0.6f and 4.0 ones). (names inferred)
VxReal sSphereDragCoefficient = 0.4f;      // Mac 0x779328, Win 0xabc498
VxReal sAxialSideDragCoefficient = 0.6f;   // Mac 0x779330, Win 0xabc4a0
VxReal sDragCoefficient = 1.0;             // Mac 0x779338, Win 0xabc4a8 (box faces, cylinder caps, meshes)
VxReal sFaceLeverScale = 2.0;              // Mac 0x779340, Win 0xabc4b0 (box face drag: lever point)
VxReal sCapLeverScale = 4.0;               // Mac 0x779348, Win 0xabc4b8 (cylinder cap drag: lever point)
// Mac 0x77cddc (made by the static initialiser 0x45388e), Win 0xabc4c0 (0x745820): the largest
// cell of the box buoyancy grid per axis
VxVector3 sBoxCellSize(1.0, 1.0, 1.0);

// The forces of one force function call (Mac: inlined, 0x58 bytes on the stack; Win 0x60 bytes:
// ctor 0x74b750, add 0x74b640, dtor 0x74b7d0). The destructor applies the totals: the fluid's
// subscriber (if any) sees them first and may veto (returning false). (names inferred)
struct FluidForce {
    // Win 0x74b750 (ECX = this; cdecl(geometry, fluid state)); Mac inlined
    FluidForce(VxCollisionGeometry* cg, VxFluidState* state)
        : mGeometry(cg), mState(state), mFullyImmersed(false), mEmpty(true)
    {
        mPart = static_cast<VxPart*>(cg->mEntity);
        mPart->getCOMPosition(mCOM);
        mTorque.setZero();
        mForce.setZero();
    }
    // Win 0x74b7d0
    ~FluidForce()
    {
        if (mEmpty)
            return;
        VxFluidState::Subscriber* s = mState->getSubscriber();
        if (s == 0 || s->notifyForce(mGeometry, mForce, mTorque, mFullyImmersed)) {
            mPart->addForce(mForce);
            mPart->addTorque(mTorque);
        }
    }
    // Win 0x74b640: force f at the world point p (torque about the part's centre of mass)
    void add(const VxVector3& f, const VxVector3& p)
    {
        mForce += f;
        VxVector3 r = p - mCOM;
        mTorque += r.cross(f);
        mEmpty = false;
    }

    VxCollisionGeometry* mGeometry; // +0x00
    VxPart* mPart;                  // +0x04
    VxFluidState* mState;           // +0x08
    VxVector3 mForce;               // Mac +0x0c, Win +0x10
    VxVector3 mTorque;              // Mac +0x24, Win +0x28
    VxVector3 mCOM;                 // Mac +0x3c, Win +0x40
    bool mFullyImmersed;            // Mac +0x54, Win +0x58 (passed to the subscriber)
    bool mEmpty;                    // Mac +0x55, Win +0x59
};

// Win 0x744fc0 (Mac inlined): the area of the segment of height h cut off a circle of radius r
// (name inferred)
inline VxReal circularSegmentArea(VxReal h, VxReal r)
{
    VxReal a = acos(1.0 - h / r);
    a = a + a;
    return (a - sin(a)) * (r * 0.5 * r);
}

// point p (shape frame) -> world under tm
inline VxVector3 transformPoint(const VxTransform& tm, const VxVector3& p)
{
    return VxVector3(p[0] * tm.m[0][0] + p[1] * tm.m[1][0] + p[2] * tm.m[2][0] + tm.m[3][0],
                     p[0] * tm.m[0][1] + p[1] * tm.m[1][1] + p[2] * tm.m[2][1] + tm.m[3][1],
                     p[0] * tm.m[0][2] + p[1] * tm.m[1][2] + p[2] * tm.m[2][2] + tm.m[3][2]);
}

// buoyancy scale (VxFluidInteractionData::m38, 1 without data)
inline VxReal buoyancyScale(VxCollisionGeometry* cg)
{
    return cg->getFluidInteractionData() == 0 ? 1.0 : cg->getFluidInteractionData()->m38;
}

} // namespace

// --------------------------------------------------------------------------------------
// VxFluidInteractionUniverseUpdate

// D1 Mac 0x4487c0 (D2 0x4486c8), D0 0x4485c8; Win 0x70cc60 (D2 0x74b2d0)
VxFluidInteractionUniverseUpdate::~VxFluidInteractionUniverseUpdate()
{
    for (VxFluidState** i = mFluidStates.begin(); i != mFluidStates.end(); ++i)
        delete *i;
}

// Mac 0x44856a, Win 0x74b200
void VxFluidInteractionUniverseUpdate::update(VxUniverse* u)
{
    (void)u;
    if (mFluidStates.size() == 0)
        return;
    for (VxCollisionGeometry** g = mGeometries.begin(); g != mGeometries.end(); ++g)
        for (VxFluidState** s = mFluidStates.begin(); s != mFluidStates.end(); ++s)
            if ((*g)->applyFluidInteractionForce(*s))
                break;
}

// Mac 0x4487d4, Win 0x74b5d0
void VxFluidInteractionUniverseUpdate::addCollisionGeometry(VxCollisionGeometry* cg)
{
    mGeometries.pushBackUnique(cg);
}

// Mac 0x44848a, Win 0x74b540
bool VxFluidInteractionUniverseUpdate::removeCollisionGeometry(VxCollisionGeometry* cg)
{
    return mGeometries.remove(cg);
}

// Win 0x74b2c0 (Win only)
VxFluidState* VxFluidInteractionUniverseUpdate::getFluidState(int i) const
{
    return mFluidStates.begin()[i];
}

// Win 0x74b580 (Win only): before the first state that is not denser
void VxFluidInteractionUniverseUpdate::addFluidState(VxFluidState* s)
{
    for (VxFluidState** i = mFluidStates.begin(); i != mFluidStates.end(); ++i)
        if (*i == s)
            return;
    VxFluidState** pos = mFluidStates.begin();
    while (pos != mFluidStates.end() && (*pos)->mDensity > s->mDensity)
        ++pos;
    mFluidStates.insert(pos, s);
}

// Win 0x74b370 (Win only)
bool VxFluidInteractionUniverseUpdate::removeFluidState(VxFluidState* s)
{
    return mFluidStates.remove(s);
}

namespace {
// the std::sort predicate of sortFluidStates (Win: inlined into 0x74c060 / 0x74b6c0)
bool denser(const VxFluidState* a, const VxFluidState* b)
{
    return a->mDensity > b->mDensity;
}
} // namespace

// Win 0x74b610 (Win only; std::sort internals 0x74c060, 0x74b6c0, 0x74b880 ...)
void VxFluidInteractionUniverseUpdate::sortFluidStates()
{
    std::sort(mFluidStates.begin(), mFluidStates.end(), denser);
}

// --------------------------------------------------------------------------------------
// Composite: every child with a force function (Mac 0x448510, Win 0x7452c0)
bool vxCompositeFluidInteractionForce(VxCollisionGeometry* cg, void* fluidState)
{
    VxCompositeCollisionGeometry* c = static_cast<VxCompositeCollisionGeometry*>(cg);
    for (int i = 0; i < c->getCollisionGeometryCount(); ++i) {
        VxCollisionGeometry* child = c->getCollisionGeometry(i);
        if (child->hasFluidInteractionForceFn())
            child->applyFluidInteractionForce(fluidState);
    }
    return true;
}

// --------------------------------------------------------------------------------------
// Sphere (Mac 0x44e5fb: EAX = geometry, EDX = transform, ECX = fluid state, XMM0 = radius;
// Win 0x748c50: ECX = geometry, ESI = fluid state, cdecl(transform, radius)). Also capsules
// shorter than a tenth of their radius. (name inferred)
static bool sphereFluidForce(VxCollisionGeometry* cg, const VxTransform& tm, VxFluidState* state, VxReal r)
{
    const VxVector3 center = tm.getTranslation();
    const VxReal surface = state->getSurfaceHeight(center);
    VxVector3 p; // buoyancy centre / drag point
    FluidForce ff(cg, state);
    const VxReal scale = buoyancyScale(cg);
    VxReal h = center[0] * state->mUp[0] + center[1] * state->mUp[1] + center[2] * state->mUp[2];
    bool touches = false;
    if (h - r > surface)
        return touches; // completely above the surface
    VxReal volume, dragScale, area;
    bool fullyImmersed;
    if (surface > h + r) {
        // completely below the surface
        p = center;
        if (cg->getFluidInteractionData() != 0)
            p = transformPoint(tm, cg->getFluidInteractionData()->m00);
        ff.mFullyImmersed = true;
        volume = r * ((r * 12.566370614359172 * r) / 3.0);
        dragScale = 1.0;
        area = dragScale;
        fullyImmersed = true;
    } else if (h > surface) {
        // centre above the surface: a submerged cap of height d
        VxReal d = r - (h - surface);
        volume = d * 1.0471975511965976 * d * (r * 3.0 - d);
        area = d / r;
        dragScale = area * 0.5;
        VxReal off = (h - surface) + d / 3.0;
        p.set(center[0] - off * state->mUp[0], center[1] - off * state->mUp[1], center[2] - off * state->mUp[2]);
        fullyImmersed = false;
    } else {
        // centre below the surface: an emerged cap of height d
        VxReal d = r + (h - surface);
        volume = r * 4.1887902047863905 * r * r - d * 1.0471975511965976 * d * (r * 3.0 - d);
        area = 1.0;
        dragScale = ((r - d) / r + 1.0) * 0.5;
        VxReal off = d / 3.0;
        p.set(center[0] - off * state->mUp[0], center[1] - off * state->mUp[1], center[2] - off * state->mUp[2]);
        fullyImmersed = false;
    }
    if (scale > 0) {
        VxReal f = volume * scale * state->getDensity(p) * state->mGravity;
        ff.add(VxVector3(f * state->mUp[0], f * state->mUp[1], f * state->mUp[2]), p);
    }
    // drag and lift from the velocity relative to the fluid
    VxVector3 fluidVelocity, v;
    state->getVelocity(center, fluidVelocity);
    ff.mPart->getLinearVelocity(v);
    v -= fluidVelocity;
    VxReal v2 = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    if (v2 > 1e-07) {
        VxReal speed = sqrt(v2);
        VxReal inv = 1.0 / speed;
        v[0] = v[0] * inv;
        v[1] = v[1] * inv;
        v[2] = inv * v[2];
        VxVector3 dir = v;
        VxReal drag = sSphereDragCoefficient;
        if (cg->getFluidInteractionData() != 0)
            drag = cg->getFluidInteractionData()->m18[0];
        if (drag > 0) {
            VxReal c = 0;
            if (!fullyImmersed)
                c = -(state->mUp[0] * dir[0] + state->mUp[1] * dir[1] + dir[2] * state->mUp[2]);
            if (fullyImmersed || c <= -0.9 || c >= 0.9 || dragScale <= 1e-07) {
                p.set(r * (dir[0] * 0.5) * dragScale + center[0], r * (dir[1] * 0.5) * dragScale + center[1],
                      r * (dir[2] * 0.5) * dragScale + center[2]);
            } else {
                p.set(r * dir[0] * dragScale + center[0], r * dir[1] * dragScale + center[1],
                      r * dir[2] * dragScale + center[2]);
                VxReal dh = h - surface;
                if (dh > 0 && r > dh) {
                    VxReal k = dh + r;
                    VxVector3 t((p[0] - state->mUp[0] * 0.5 * k) - center[0], (p[1] - state->mUp[1] * 0.5 * k) - center[1],
                                (p[2] - state->mUp[2] * 0.5 * k) - center[2]);
                    t.normalize();
                    p.set(r * t[0] + center[0], r * t[1] + center[1], r * t[2] + center[2]);
                    speed = (dir[0] * t[0] + dir[1] * t[1] + dir[2] * t[2]) * speed;
                    dir = t;
                } else if (dh < 0 && -dh < r) {
                    VxReal k = r - dh;
                    VxVector3 t((p[0] - state->mUp[0] * 0.5 * k) - center[0], (p[1] - state->mUp[1] * 0.5 * k) - center[1],
                                (p[2] - state->mUp[2] * 0.5 * k) - center[2]);
                    t.normalize();
                    if (-(state->mUp[0] * t[0] + state->mUp[1] * t[1] + state->mUp[2] * t[2]) > c) {
                        p.set(r * t[0] + center[0], r * t[1] + center[1], r * t[2] + center[2]);
                        dir = t;
                    }
                }
            }
            VxReal f = v2 * (-drag * state->getDensity(p)) * kPi * (r * r) * dragScale * area;
            ff.add(VxVector3(f * dir[0], f * dir[1], f * dir[2]), p);
        }
        // lift (Magnus) from the angular velocity across the flow
        if (cg->getFluidInteractionData() != 0) {
            VxReal lift = cg->getFluidInteractionData()->m30;
            if (lift > 0) {
                VxVector3 w;
                ff.mPart->getAngularVelocity(w);
                VxReal d = w[0] * v[0] + v[1] * w[1] + v[2] * w[2];
                w[0] = w[0] - v[0] * d;
                w[1] = w[1] - v[1] * d;
                w[2] = w[2] - d * v[2];
                VxVector3 l = w.cross(v);
                VxReal n = sqrt(l[0] * l[0] + l[1] * l[1] + l[2] * l[2]);
                if (n > 1e-07) {
                    VxReal inv2 = 1.0 / n;
                    l.set(l[0] * inv2, l[1] * inv2, l[2] * inv2);
                    VxReal f = lift * 8.0 * 0.5 * kPi * state->getDensity(center) * r * r * r
                               * sqrt(w[0] * w[0] + w[1] * w[1] + w[2] * w[2]) * speed;
                    ff.add(VxVector3(l[0] * f, l[1] * f, l[2] * f), center);
                }
            }
        }
    }
    touches = !ff.mEmpty;
    return touches;
}

// --------------------------------------------------------------------------------------
// Box (Mac 0x44fbcd cdecl(geometry, transform, half extents, fluid state, scale); Win 0x747c00:
// ESI = fluid state, cdecl(geometry, transform, half extents, scale)). Buoyancy from a grid of
// at most sBoxCellSize-sized cells (at least 2 per axis), drag on 4 quadrants per face, applied at
// the face points sFaceLeverScale quadrant offsets out. (name inferred)
static bool boxFluidForce(VxCollisionGeometry* cg, const VxTransform& tm, const VxReal half[3], VxFluidState* state,
                          VxReal scaleParam)
{
    VxVector3 minCorner, maxCorner;
    cg->getBoundingBox(minCorner, maxCorner);
    VxReal lo, hi;
    state->getSurfaceHeightRange(minCorner, maxCorner, lo, hi);
    const VxReal scale = buoyancyScale(cg);
    FluidForce ff(cg, state);
    VxTransform R = tm; // rows flipped to point up
    const VxVector3& up = state->mUp;
    const VxReal h = R.m[3][0] * up[0] + R.m[3][1] * up[1] + R.m[3][2] * up[2];
    VxReal a[3]; // |axis . up|
    VxReal ext = 0;
    for (int i = 0; i < 3; ++i) {
        VxReal d = R.m[i][0] * up[0] + R.m[i][1] * up[1] + R.m[i][2] * up[2];
        a[i] = d;
        if (d < 0) {
            R.m[i][0] = -R.m[i][0];
            R.m[i][1] = -R.m[i][1];
            R.m[i][2] = -R.m[i][2];
            a[i] = -d;
        }
        ext = ext + half[i] * a[i];
    }
    bool touches = false;
    if (h - ext > hi)
        return touches;
    bool fullyImmersed;
    if (lo > h + ext) {
        // completely below the surface: one force at the centre of buoyancy
        VxVector3 p = tm.getTranslation();
        if (cg->getFluidInteractionData() != 0)
            p = transformPoint(tm, cg->getFluidInteractionData()->m00);
        ff.mFullyImmersed = true;
        VxReal v = 8.0 * half[0] * half[1] * half[2] * scale;
        VxReal f = v * state->getDensity(p) * state->mGravity * scaleParam;
        ff.add(VxVector3(f * up[0], f * up[1], f * up[2]), p);
        fullyImmersed = true;
    } else {
        // partly immersed: the cells
        int n[3];
        VxReal cell[3];
        VxReal proj = 0; // half height of a cell along up
        for (int i = 0; i < 3; ++i) {
            n[i] = (int)(half[i] / sBoxCellSize[i]);
            if (n[i] < 2)
                n[i] = 2;
            cell[i] = half[i] / (VxReal)n[i];
            proj = cell[i] * a[i] + proj;
        }
        const VxReal cellVolume = cell[0] * 8.0 * cell[1] * cell[2];
        const VxReal fullCell = scale * cellVolume;
        const VxReal span = proj * -2.0;
        for (int ix = 0; ix < n[0]; ++ix) {
            VxReal o = ((VxReal)ix + 0.5) * (cell[0] + cell[0]) - half[0];
            VxVector3 qx(o * R.m[0][0] + R.m[3][0], o * R.m[0][1] + R.m[3][1], o * R.m[0][2] + R.m[3][2]);
            for (int iy = 0; iy < n[1]; ++iy) {
                o = ((VxReal)iy + 0.5) * (cell[1] + cell[1]) - half[1];
                VxVector3 qy(o * R.m[1][0] + qx[0], o * R.m[1][1] + qx[1], o * R.m[1][2] + qx[2]);
                for (int iz = 0; iz < n[2]; ++iz) {
                    o = ((VxReal)iz + 0.5) * (cell[2] + cell[2]) - half[2];
                    VxVector3 q(o * R.m[2][0] + qy[0], o * R.m[2][1] + qy[1], o * R.m[2][2] + qy[2]);
                    VxReal hq = q[0] * up[0] + q[1] * up[1] + q[2] * up[2];
                    VxReal s = state->getSurfaceHeight(q);
                    if (hq - proj > s)
                        continue;
                    VxReal top = proj + hq;
                    VxReal f;
                    if (s > top)
                        f = fullCell * state->getDensity(q) * state->mGravity * scaleParam;
                    else
                        f = ((top - s) / span + 1.0) * cellVolume * scale * state->getDensity(q) * state->mGravity
                            * scaleParam;
                    ff.add(VxVector3(f * up[0], f * up[1], f * up[2]), q);
                }
            }
        }
        fullyImmersed = false;
    }
    // drag: -0.5 * coefficient per face axis (the x coefficient switches it on)
    VxReal coef[3] = {sDragCoefficient, sDragCoefficient, sDragCoefficient};
    if (cg->getFluidInteractionData() != 0) {
        const VxVector3& c = cg->getFluidInteractionData()->m18;
        coef[0] = c[0];
        coef[1] = c[1];
        coef[2] = c[2];
    }
    if (coef[0] > 0) {
        coef[0] = coef[0] * -0.5;
        coef[1] = coef[1] * -0.5;
        coef[2] = coef[2] * -0.5;
        const VxVector3 center = tm.getTranslation();
        VxVector3 fluidVelocity, v;
        state->getVelocity(center, fluidVelocity);
        ff.mPart->getAngularVelocity(v);
        VxReal w2 = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
        static const int kNext[3] = {1, 2, 0}; // Mac 0x643f6c
        static const int kPrev[3] = {2, 0, 1}; // Mac 0x643f60
        if (w2 < 1e-07 && fullyImmersed) {
            // not spinning and fully immersed: one force per face from the linear velocity
            ff.mPart->getLinearVelocity(v);
            v -= fluidVelocity;
            for (int ax = 0; ax < 3; ++ax) {
                const VxReal* n = R.m[ax];
                VxReal vn = v[0] * n[0] + v[1] * n[1] + v[2] * n[2];
                if (fabs(vn) > 1e-07) {
                    VxReal sign = vn <= 0 ? -1.0 : 1.0;
                    VxVector3 p(sign * (half[ax] * n[0]) + center[0], sign * (half[ax] * n[1]) + center[1],
                                sign * (half[ax] * n[2]) + center[2]);
                    VxReal f = sign * (4.0 * half[kNext[ax]] * half[kPrev[ax]] * coef[ax] * (vn * vn))
                               * state->getDensity(p);
                    ff.add(VxVector3(f * n[0], f * n[1], f * n[2]), p);
                }
            }
        } else {
            static const VxReal kSide[2] = {-1.0, 1.0};
            for (int ax = 0; ax < 3; ++ax) {
                const int bi = kNext[ax], ci = kPrev[ax];
                const VxReal hb = half[bi], hc = half[ci];
                const VxReal quarter = hb * hc;
                VxVector3 eb(hb * (0.5 * R.m[bi][0]), hb * (0.5 * R.m[bi][1]), hb * (0.5 * R.m[bi][2]));
                VxVector3 ec(hc * (0.5 * R.m[ci][0]), hc * (0.5 * R.m[ci][1]), hc * (0.5 * R.m[ci][2]));
                VxVector3 sum(ec[0] + eb[0], ec[1] + eb[1], ec[2] + eb[2]);
                VxVector3 diff(eb[0] - ec[0], eb[1] - ec[1], eb[2] - ec[2]);
                const VxReal* n = R.m[ax];
                VxReal proj = 0;
                for (int side = 0; side < 2; ++side) {
                    const VxReal sign = kSide[side];
                    const VxReal ha = half[ax];
                    VxVector3 fc(sign * n[0] * ha + center[0], sign * n[1] * ha + center[1], sign * n[2] * ha + center[2]);
                    VxVector3 pts[4], lever[4];
                    if (!fullyImmersed)
                        proj = fabs(sum[0] * up[0] + sum[1] * up[1] + sum[2] * up[2]);
                    for (int k = 0; k < 3; ++k) {
                        pts[0][k] = (eb[k] + fc[k]) + ec[k];
                        pts[1][k] = (eb[k] + fc[k]) - ec[k];
                        pts[2][k] = (fc[k] - eb[k]) + ec[k];
                        pts[3][k] = (fc[k] - eb[k]) - ec[k];
                        lever[0][k] = sum[k] * sFaceLeverScale + fc[k];
                        lever[1][k] = diff[k] * sFaceLeverScale + fc[k];
                        lever[2][k] = fc[k] - diff[k] * sFaceLeverScale;
                        lever[3][k] = fc[k] - sum[k] * sFaceLeverScale;
                    }
                    const VxReal span = proj + proj;
                    for (int k = 0; k < 4; ++k) {
                        VxReal frac = quarter;
                        if (!fullyImmersed) {
                            VxReal s = state->getSurfaceHeight(pts[k]);
                            VxReal hk = pts[k][0] * up[0] + pts[k][1] * up[1] + pts[k][2] * up[2];
                            if (hk - proj > s)
                                continue;
                            if (proj + hk > s)
                                frac = (((s - hk) + proj) / span) * quarter;
                        }
                        ff.mPart->getVelocityAtPoint(pts[k], v);
                        v -= fluidVelocity;
                        VxReal vn = v[0] * n[0] + v[1] * n[1] + v[2] * n[2];
                        if (sign * vn > 1e-07) {
                            VxReal f = frac * coef[ax] * (vn * vn) * sign * state->getDensity(pts[k]);
                            ff.add(VxVector3(f * n[0], f * n[1], f * n[2]), lever[k]);
                        }
                    }
                }
            }
        }
    }
    touches = !ff.mEmpty;
    return touches;
}

// --------------------------------------------------------------------------------------
// Cylinder / capsule (Mac 0x4488d6: EAX = geometry, EDX = transform, ECX = fluid state, XMM0 =
// radius, XMM1 = half length, stack (top level, flat caps); Win 0x7459f0 cdecl(geometry,
// transform, radius, half length, fluid state, top level, flat caps)). The shape is the cylinder of
// the axis (transform row 2); capsules come in with the half length of the equal-volume cylinder
// (radius * 4/3 + height) / 2. Buoyancy: one slab when the surface crosses the side evenly, else
// 5 slices along the axis (circular segments); a near-vertical, wide or wavy cylinder at the top
// level is split into four quarter-radius cylinders (recursion without drag). Drag on the caps
// (flatCaps: coefficient m18[2], 4 quadrants) and on the side (m18[0], two halves), lift from the
// spin around the axis when fully immersed. (names inferred)
static bool axialFluidForce(VxCollisionGeometry* cg, const VxTransform& tm, VxFluidState* state, VxReal r, VxReal hh,
                            bool top, bool flatCaps)
{
    VxVector3 minCorner, maxCorner;
    cg->getBoundingBox(minCorner, maxCorner);
    VxReal lo, hi;
    state->getSurfaceHeightRange(minCorner, maxCorner, lo, hi);
    VxVector3 axis = tm.getAxis(2);
    VxVector3 pos = tm.getTranslation(); // centre, then the drag sample point
    FluidForce ff(cg, state);
    const VxReal scale = buoyancyScale(cg);
    const VxVector3& up = state->mUp;
    const VxReal hc = up[0] * pos[0] + up[1] * pos[1] + up[2] * pos[2];
    const VxReal s = state->getSurfaceHeight(pos);
    VxReal ext = (axis[0] * up[0] + axis[1] * up[1] + axis[2] * up[2]) * hh; // axial half extent along up
    if (ext < 0) {
        ext = -ext;
        axis = -axis;
    }
    VxVector3 radial; // the radius direction closest to up
    VxReal rext;      // radial extent along up
    if (hh - ext > hh * 0.001) {
        VxVector3 t = up.cross(axis);
        radial = axis.cross(t);
        VxReal n = sqrt(radial[0] * radial[0] + radial[1] * radial[1] + radial[2] * radial[2]);
        if (n > 1e-07) {
            VxReal inv = 1.0 / n;
            radial.set(radial[0] * inv, radial[1] * inv, radial[2] * inv);
            rext = (radial[0] * up[0] + radial[1] * up[1] + radial[2] * up[2]) * r;
            if (rext < 0) {
                rext = -rext;
                radial = -radial;
            }
        } else {
            rext = 0;
        }
    } else {
        rext = 0;
    }
    bool result = false;  // a sub-cylinder touched the fluid
    bool fullyImmersed;   // Mac -0x66a
    bool sliced = false;  // Mac -0x669: buoyancy from the 5 slices
    const VxReal total = rext + ext;
    if (hc - total >= s - 1e-07)
        return false; // completely above the surface
    if (!(s + 1e-07 < hc + total)) {
        // completely below the surface
        if (top && cg->getFluidInteractionData() != 0)
            pos = transformPoint(tm, cg->getFluidInteractionData()->m00);
        ff.mFullyImmersed = true;
        VxReal f = r * kPi * r * hh * 2.0 * scale * state->getDensity(pos) * state->mGravity;
        ff.add(VxVector3(f * up[0], f * up[1], f * up[2]), pos);
        fullyImmersed = true;
    } else if (top && ((ext > hh * 0.7 && r > hh) || (ext > hh * 0.99 && hi - lo > hh * 0.05))) {
        // split into four cylinders of half the radius around the axis
        VxVector3 e1, e2;
        if (rext > r * 0.7) {
            e1 = radial;
            e2 = e1.cross(axis);
        } else {
            e1 = axis;
            e1[fabs(axis[0]) > fabs(axis[1]) ? 1 : 0] += 1.0;
            e2 = e1.cross(axis);
            e2.normalize();
            e1 = e2.cross(axis);
        }
        VxTransform sub = tm;
        const VxReal r2 = r * 0.5;
        const VxVector3* dirs[2] = {&e1, &e2};
        for (int k = 0; k < 2; ++k) {
            const VxVector3& e = *dirs[k];
            VxVector3 o(r * e[0] * 0.5, r * e[1] * 0.5, r * e[2] * 0.5);
            sub.setTranslation(VxVector3(o[0] + pos[0], o[1] + pos[1], o[2] + pos[2]));
            if (axialFluidForce(cg, sub, state, r2, hh, false, flatCaps))
                result = true;
            sub.setTranslation(VxVector3(pos[0] - o[0], pos[1] - o[1], pos[2] - o[2]));
            if (axialFluidForce(cg, sub, state, r2, hh, false, flatCaps))
                result = true;
        }
        fullyImmersed = false;
    } else {
        VxReal volume[5];
        VxVector3 point[5];
        int count;
        if (fabs(s - hc) <= (ext - rext) - 1e-07) {
            // the surface crosses the side only: one slab
            VxReal d = (hh * (s - hc)) / ext;
            VxReal len = hh + d;
            VxReal off = len * -0.5 + d;
            point[0].set(axis[0] * off + pos[0], axis[1] * off + pos[1], axis[2] * off + pos[2]);
            volume[0] = r * kPi * r * len;
            count = 1;
        } else {
            // 5 slices along the axis
            const VxReal w = (hh + hh) / 5.0;
            const VxReal start = w * 0.5 - hh;
            const VxReal disc = r * kPi * r;
            const VxReal slab = disc * w;
            const VxVector3 rad = radial;
            count = 0;
            for (int i = 0; i < 5; ++i) {
                VxReal o = (VxReal)i * w + start;
                point[count].set(axis[0] * o + pos[0], axis[1] * o + pos[1], axis[2] * o + pos[2]);
                VxVector3& p = point[count];
                VxReal sh = state->getSurfaceHeight(p);
                VxReal hp = p[0] * up[0] + p[1] * up[1] + p[2] * up[2];
                if (hp - rext >= sh)
                    continue;
                if (!(sh < rext + hp)) {
                    volume[count] = slab;
                } else if (hp > sh) {
                    // slice centre above the surface: the submerged circular segment
                    VxReal d = ((hp - sh) * r) / rext;
                    VxReal t = r - d;
                    volume[count] = circularSegmentArea(t, r) * w;
                    VxReal off = d + t / 3.0;
                    p.set(p[0] - rad[0] * off, p[1] - rad[1] * off, p[2] - rad[2] * off);
                } else {
                    // slice centre below the surface: the disc minus the emerged segment
                    VxReal t = r - ((sh - hp) * r) / rext;
                    volume[count] = (disc - circularSegmentArea(t, r)) * w;
                    VxReal off = t / 3.0;
                    p.set(p[0] - rad[0] * off, p[1] - rad[1] * off, p[2] - rad[2] * off);
                }
                ++count;
            }
            sliced = true;
        }
        for (int i = 0; i < count; ++i) {
            VxReal f = scale * volume[i] * state->getDensity(point[i]) * state->mGravity;
            ff.add(VxVector3(f * up[0], f * up[1], f * up[2]), point[i]);
        }
        fullyImmersed = false;
    }
    if (!top)
        return !ff.mEmpty || result;

    // drag and lift (top level only)
    VxReal capDrag = sDragCoefficient, sideDrag = sAxialSideDragCoefficient;
    if (cg->getFluidInteractionData() != 0) {
        capDrag = cg->getFluidInteractionData()->m18[2];
        sideDrag = cg->getFluidInteractionData()->m18[0];
    }
    const VxReal capCoef = flatCaps ? capDrag * -0.5 : 0.0;
    const VxReal sideCoef = sideDrag * -0.5;
    VxVector3 fluidVelocity, w, v;
    state->getVelocity(pos, fluidVelocity);
    ff.mPart->getAngularVelocity(w);
    const VxVector3 center = tm.getTranslation();
    if (capDrag > 0 || sideDrag > 0) {
        VxReal w2 = w[0] * w[0] + w[1] * w[1] + w[2] * w[2];
        if (w2 < 1e-07 && fullyImmersed) {
            // not spinning, fully immersed: one cap force and one side force from the linear velocity
            ff.mPart->getLinearVelocity(v);
            v -= fluidVelocity;
            VxReal va = axis[0] * v[0] + axis[1] * v[1] + axis[2] * v[2];
            if (fabs(va) > 1e-07) {
                VxReal sign = va > 0 ? 1.0 : -1.0;
                VxVector3 p(sign * (axis[0] * hh) + pos[0], sign * (axis[1] * hh) + pos[1], sign * (axis[2] * hh) + pos[2]);
                VxReal f = sign * (r * kPi * r * capCoef * (va * va)) * state->getDensity(p);
                ff.add(VxVector3(axis[0] * f, axis[1] * f, axis[2] * f), p);
            }
            VxVector3 t = v.cross(axis);
            radial = axis.cross(t);
            VxReal n = sqrt(radial[0] * radial[0] + radial[1] * radial[1] + radial[2] * radial[2]);
            if (n > 1e-07) {
                VxReal inv = 1.0 / n;
                VxVector3 d(radial[0] * inv, radial[1] * inv, radial[2] * inv);
                VxReal vr = d[0] * v[0] + d[1] * v[1] + d[2] * v[2];
                if (vr > 1e-07) {
                    VxReal sign = vr > 0 ? 1.0 : -1.0;
                    radial = d;
                    VxVector3 p(r * d[0] + pos[0], r * d[1] + pos[1], r * d[2] + pos[2]);
                    VxReal f = sign * (r * 4.0 * hh * sideCoef * (vr * vr)) * state->getDensity(p);
                    ff.add(VxVector3(d[0] * f, d[1] * f, d[2] * f), p);
                }
            }
        } else {
            static const VxReal kSide[2] = {-1.0, 1.0};
            if (capDrag > 0) {
                // caps: 4 quadrants each
                const VxReal quarter = r * kPi * r * 0.25;
                const VxReal hr = r * 0.5;
                VxVector3 e0(hr * tm.m[0][0], hr * tm.m[0][1], hr * tm.m[0][2]);
                VxVector3 e1(hr * tm.m[1][0], hr * tm.m[1][1], hr * tm.m[1][2]);
                VxReal proj = fullyImmersed ? 0.0
                                            : fabs((e0[0] + e1[0]) * up[0] + (e0[1] + e1[1]) * up[1] + (e0[2] + e1[2]) * up[2]);
                VxVector3 sum(e1[0] + e0[0], e1[1] + e0[1], e1[2] + e0[2]);
                VxVector3 diff(e0[0] - e1[0], e0[1] - e1[1], e0[2] - e1[2]);
                const VxReal span = proj + proj;
                for (int side = 0; side < 2; ++side) {
                    const VxReal sign = kSide[side];
                    VxVector3 cc(hh * (axis[0] * sign) + center[0], hh * (axis[1] * sign) + center[1],
                                 hh * (axis[2] * sign) + center[2]);
                    VxVector3 pts[4], lever[4];
                    for (int k = 0; k < 3; ++k) {
                        pts[0][k] = (e0[k] + cc[k]) + e1[k];
                        pts[1][k] = (e0[k] + cc[k]) - e1[k];
                        pts[2][k] = (cc[k] - e0[k]) + e1[k];
                        pts[3][k] = (cc[k] - e0[k]) - e1[k];
                        lever[0][k] = sum[k] * sCapLeverScale + cc[k];
                        lever[1][k] = diff[k] * sCapLeverScale + cc[k];
                        lever[2][k] = cc[k] - diff[k] * sCapLeverScale;
                        lever[3][k] = cc[k] - sum[k] * sCapLeverScale;
                    }
                    for (int k = 0; k < 4; ++k) {
                        VxReal frac = quarter;
                        if (!fullyImmersed) {
                            VxReal sh = state->getSurfaceHeight(pts[k]);
                            VxReal hk = pts[k][0] * up[0] + pts[k][1] * up[1] + pts[k][2] * up[2];
                            if (hk - proj > sh)
                                continue;
                            if (proj + hk > sh)
                                frac = (((sh - hk) + proj) / span) * quarter;
                        }
                        ff.mPart->getVelocityAtPoint(pts[k], v);
                        v -= fluidVelocity;
                        VxReal vn = axis[0] * v[0] + axis[1] * v[1] + axis[2] * v[2];
                        if (sign * vn > 1e-07) {
                            VxReal f = frac * capCoef * (vn * vn) * sign * state->getDensity(pts[k]);
                            ff.add(VxVector3(axis[0] * f, axis[1] * f, axis[2] * f), lever[k]);
                        }
                    }
                }
            }
            if (sideDrag > 0) {
                // side: two halves, sampled at their middles, applied at the ends
                const VxReal halfLen = hh * 0.5;
                const VxReal halfExt = ext * 0.5;
                const VxReal sideArea = r * 4.0 * hh * 0.5;
                const VxReal negHalfExt = -halfExt;
                const VxReal limit = rext + halfExt;
                for (int i = 0; i < 2; ++i) {
                    VxReal o = ((VxReal)(i * 2) + 1.0) * halfLen - hh;
                    pos.set(axis[0] * o + center[0], axis[1] * o + center[1], axis[2] * o + center[2]);
                    VxReal o2 = ((VxReal)i + (VxReal)i - 1.0) * hh;
                    VxVector3 end(axis[0] * o2 + center[0], axis[1] * o2 + center[1], axis[2] * o2 + center[2]);
                    ff.mPart->getVelocityAtPoint(pos, v);
                    v -= fluidVelocity;
                    VxReal depth = 0;
                    if (!fullyImmersed) {
                        VxReal hp = pos[0] * up[0] + pos[1] * up[1] + pos[2] * up[2];
                        depth = hp - state->getSurfaceHeight(pos);
                        if (depth > limit)
                            continue;
                    }
                    VxVector3 t = v.cross(axis);
                    radial = axis.cross(t);
                    VxReal n = sqrt(radial[0] * radial[0] + radial[1] * radial[1] + radial[2] * radial[2]);
                    if (!(n > 1e-07))
                        continue;
                    VxReal inv = 1.0 / n;
                    radial.set(radial[0] * inv, radial[1] * inv, radial[2] * inv);
                    VxReal vr = radial[0] * v[0] + radial[1] * v[1] + radial[2] * v[2];
                    if (!(vr > 1e-07))
                        continue;
                    VxReal sign = vr > 0 ? 1.0 : -1.0;
                    const VxVector3 d = radial;
                    VxVector3 p(r * d[0] + pos[0], r * d[1] + pos[1], r * d[2] + pos[2]);
                    VxReal area;
                    if (fullyImmersed) {
                        area = sideArea;
                    } else if (!sliced) {
                        if (depth > halfExt)
                            continue;
                        if (depth > negHalfExt)
                            area = (1.0 - depth / halfExt) * 0.5 * sideArea;
                        else
                            area = sideArea;
                    } else {
                        VxReal du = d[0] * up[0] + d[1] * up[1] + d[2] * up[2];
                        if (du > 0) {
                            if (depth >= 0)
                                continue;
                            if (rext > -depth)
                                area = (-depth / rext) * sideArea;
                            else
                                area = sideArea;
                        } else if (depth > 0 && rext > depth) {
                            area = ((rext - depth) / rext) * sideArea;
                        } else {
                            area = sideArea;
                        }
                        if (fabs(du) < 0.9) {
                            // near the surface: turn the drag point towards the waterline
                            VxVector3 ua = up.cross(axis);
                            VxVector3 rr = axis.cross(ua);
                            if (depth > 0 && rext > depth) {
                                VxReal k = rext + depth;
                                VxVector3 q((p[0] - rr[0] * 0.5 * k) - pos[0], (p[1] - rr[1] * 0.5 * k) - pos[1],
                                            (p[2] - rr[2] * 0.5 * k) - pos[2]);
                                q.normalize();
                                p.set(r * q[0] + pos[0], r * q[1] + pos[1], r * q[2] + pos[2]);
                                vr = (d[0] * q[0] + d[1] * q[1] + d[2] * q[2]) * vr;
                                radial = q;
                            } else if (depth < 0 && rext > -depth) {
                                VxReal k = rext - depth;
                                VxVector3 q((p[0] - rr[0] * 0.5 * k) - pos[0], (p[1] - rr[1] * 0.5 * k) - pos[1],
                                            (p[2] - rr[2] * 0.5 * k) - pos[2]);
                                q.normalize();
                                if (-(rr[0] * q[0] + rr[1] * q[1] + rr[2] * q[2]) > -(rr[0] * d[0] + rr[1] * d[1] + rr[2] * d[2])) {
                                    p.set(r * q[0] + pos[0], r * q[1] + pos[1], r * q[2] + pos[2]);
                                    radial = q;
                                }
                            }
                        }
                    }
                    VxReal f = sideCoef * area * (vr * vr) * sign * state->getDensity(p);
                    ff.add(VxVector3(f * radial[0], f * radial[1], f * radial[2]), end);
                }
            }
            if (!fullyImmersed)
                return !ff.mEmpty || result;
        }
    } else if (!fullyImmersed) {
        return !ff.mEmpty || result;
    }
    // lift from the spin around the axis (fully immersed only)
    VxReal w2 = w[0] * w[0] + w[1] * w[1] + w[2] * w[2];
    if (w2 > 1e-07 && cg->getFluidInteractionData() != 0) {
        VxReal lift = cg->getFluidInteractionData()->m30;
        if (lift > 0) {
            ff.mPart->getLinearVelocity(v);
            v -= fluidVelocity;
            VxReal speed = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
            if (speed > 1e-07) {
                VxReal inv = 1.0 / speed;
                v.set(v[0] * inv, v[1] * inv, v[2] * inv);
                VxReal d = w[0] * v[0] + v[1] * w[1] + v[2] * w[2];
                w.set(w[0] - v[0] * d, w[1] - v[1] * d, w[2] - v[2] * d);
                VxReal wa = axis[0] * w[0] + axis[1] * w[1] + axis[2] * w[2];
                w.set(axis[0] * wa, wa * axis[1], axis[2] * wa);
                VxVector3 l = w.cross(v);
                VxReal ln = sqrt(l[0] * l[0] + l[1] * l[1] + l[2] * l[2]);
                if (ln > 1e-07) {
                    VxReal li = 1.0 / ln;
                    l.set(l[0] * li, l[1] * li, l[2] * li);
                    VxReal f = lift * 8.0 * 0.5 * kPi * state->getDensity(center) * r * r * r
                               * sqrt(w[0] * w[0] + w[1] * w[1] + w[2] * w[2]) * speed;
                    ff.add(VxVector3(l[0] * f, l[1] * f, l[2] * f), center);
                }
            }
        }
    }
    return !ff.mEmpty || result;
}

// --------------------------------------------------------------------------------------
// Meshes. The triangle and face code works in the geometry's frame (vertices, normals, the
// velocities and the fluid plane are taken into it); the FluidForce's centre of mass is replaced
// by the local one and the totals are rotated back to world at the end.

// Mac 0x44c643 (EAX = point, EDX = normal, ECX = transform (unused), XMM0 = area, XMM1 =
// density; stack: velocity, angular velocity, fluid velocity, coefficients, forces), Win
// 0x7451f0: the drag of the area `area` around p with outward normal n, if it moves out
// (name inferred)
static void pointDrag(const VxVector3& p, const VxVector3& n, const VxTransform& tm, VxReal area, VxReal rho,
                      const VxVector3& v0, const VxVector3& w, const VxVector3& vf, const VxVector3& coef,
                      FluidForce& ff)
{
    (void)tm;
    VxVector3 v(((w[1] * p[2] - w[2] * p[1]) + v0[0]) - vf[0], ((w[2] * p[0] - p[2] * w[0]) + v0[1]) - vf[1],
                ((w[0] * p[1] - w[1] * p[0]) + v0[2]) - vf[2]);
    VxReal vn = n[0] * v[0] + n[1] * v[1] + n[2] * v[2];
    if (vn > 1e-07) {
        VxReal x = -area * (vn * vn) * rho;
        ff.add(VxVector3(x * (n[0] * 0.5) * coef[0], x * (n[1] * 0.5) * coef[1], x * (n[2] * 0.5) * coef[2]), p);
    }
}

// Mac 0x44c834 (EAX, EDX, ECX = the corners, XMM0 = density; stack: normal, transform,
// velocity, angular velocity, fluid velocity, coefficients, forces), Win 0x745840: the drag of a
// triangle, sampled at the midpoints between its corners and its centroid, a third of the area
// each (name inferred)
static void triangleDrag(const VxReal* p0, const VxReal* p1, const VxReal* p2, VxReal rho, const VxVector3& n,
                         const VxTransform& tm, const VxVector3& v0, const VxVector3& w, const VxVector3& vf,
                         const VxVector3& coef, FluidForce& ff)
{
    VxReal e1x = p1[0] - p0[0], e1y = p1[1] - p0[1], e1z = p1[2] - p0[2];
    VxReal area = (((e1y * n[0] - e1x * n[1]) * (p2[2] - p0[2]) + (p2[1] - p0[1]) * (e1x * n[2] - e1z * n[0]))
                   + (p2[0] - p0[0]) * (e1z * n[1] - e1y * n[2]))
                  * 0.5 / 3.0;
    VxVector3 g(((p0[0] + p1[0]) + p2[0]) / 6.0, ((p0[1] + p1[1]) + p2[1]) / 6.0, ((p0[2] + p1[2]) + p2[2]) / 6.0);
    const VxReal* corners[3] = {p0, p1, p2};
    for (int k = 0; k < 3; ++k) {
        const VxReal* c = corners[k];
        VxVector3 s(0.5 * c[0] + g[0], 0.5 * c[1] + g[1], 0.5 * c[2] + g[2]);
        pointDrag(s, n, tm, area, rho, v0, w, vf, coef, ff);
    }
}

namespace {
// The signed volume of the tetrahedron (p, a, b, apex) and its centroid moment (Mac inlined, Win
// 0x7450d0) (name inferred)
inline void addTetrahedron(const VxReal* p, const VxReal* a, const VxReal* b, const VxVector3& apex, VxVector3& moment,
                           VxReal& volume)
{
    VxReal ax = a[0] - p[0], ay = a[1] - p[1], az = a[2] - p[2];
    VxReal bx = b[0] - p[0], by = b[1] - p[1], bz = b[2] - p[2];
    VxReal dv = ((apex[0] - p[0]) * (ay * bz - by * az) + (apex[1] - p[1]) * (bx * az - bz * ax)
                 + (apex[2] - p[2]) * (by * ax - bx * ay))
                / -6.0;
    VxReal k = dv * 0.25;
    for (int i = 0; i < 3; ++i)
        moment[i] = (((p[i] + a[i]) + b[i]) + apex[i]) * k + moment[i];
    volume = dv + volume;
}

// the point where the edge from a (distance da) to b (distance db) crosses the plane
inline VxVector3 crossing(const VxReal* a, VxReal da, const VxReal* b, VxReal db)
{
    VxReal t = da / (da - db);
    return VxVector3((b[0] - a[0]) * t + a[0], (b[1] - a[1]) * t + a[1], (b[2] - a[2]) * t + a[2]);
}

// v in the frame of tm's rotation
inline VxVector3 toLocal(const VxTransform& tm, const VxVector3& v)
{
    return VxVector3(v[0] * tm.m[0][0] + v[1] * tm.m[0][1] + v[2] * tm.m[0][2],
                     v[0] * tm.m[1][0] + v[1] * tm.m[1][1] + v[2] * tm.m[1][2],
                     v[0] * tm.m[2][0] + v[1] * tm.m[2][1] + v[2] * tm.m[2][2]);
}

// v rotated back to world
inline VxVector3 toWorld(const VxTransform& tm, const VxVector3& v)
{
    return VxVector3(v[0] * tm.m[0][0] + v[1] * tm.m[1][0] + v[2] * tm.m[2][0],
                     v[0] * tm.m[0][1] + v[1] * tm.m[1][1] + v[2] * tm.m[2][1],
                     v[0] * tm.m[0][2] + v[1] * tm.m[1][2] + v[2] * tm.m[2][2]);
}
} // namespace

namespace {
// The state the mesh force functions share (Mac / Win: locals of each function; the triangle
// code below is inlined into both) (names inferred)
struct MeshFluid {
    MeshFluid(VxFluidState* state, const VxTransform& tm)
        : mTM(tm), mState(state), mFull(false), mForces(0), mVolume(0)
    {
    }

    // the fluid plane in the mesh frame (normal mNormal, offset mPlane; mApex = the surface point
    // below the origin). Returns false when the bounding sphere is above the surface.
    bool setUp(VxCollisionGeometry* cg)
    {
        VxVector3 center;
        VxReal radius;
        cg->getBoundingSphere(center, radius);
        mSurface = mState->getSurfaceHeight(center);
        const VxVector3& up = mState->mUp;
        VxReal hc = up[0] * center[0] + up[1] * center[1] + up[2] * center[2];
        if (hc - radius >= mSurface)
            return false;
        mFull = mSurface >= hc + radius;
        mNormal.set(up[0] * mTM.m[0][0] + up[1] * mTM.m[0][1] + up[2] * mTM.m[0][2],
                    up[0] * mTM.m[1][0] + up[1] * mTM.m[1][1] + up[2] * mTM.m[1][2],
                    up[0] * mTM.m[2][0] + up[1] * mTM.m[2][1] + up[2] * mTM.m[2][2]);
        VxReal d = (up[0] * mTM.m[3][0] + up[1] * mTM.m[3][1] + up[2] * mTM.m[3][2]) - mSurface;
        VxVector3 qw((mTM.m[3][0] - d * up[0]) - mTM.m[3][0], (mTM.m[3][1] - up[1] * d) - mTM.m[3][1],
                     (mTM.m[3][2] - up[2] * d) - mTM.m[3][2]);
        mApex = toLocal(mTM, qw);
        mPlane = mApex[0] * mNormal[0] + mApex[1] * mNormal[1] + mApex[2] * mNormal[2];
        return true;
    }

    // the velocities in the mesh frame and the drag coefficients (after the forces exist)
    void setUpDrag(VxCollisionGeometry* cg, FluidForce& ff)
    {
        mForces = &ff;
        VxVector3 tmp = ff.mCOM;
        ff.mCOM = toLocal(mTM, VxVector3(tmp[0] - mTM.m[3][0], tmp[1] - mTM.m[3][1], tmp[2] - mTM.m[3][2]));
        mCoef.set(sDragCoefficient, sDragCoefficient, sDragCoefficient);
        if (cg->getFluidInteractionData() != 0)
            mCoef = cg->getFluidInteractionData()->m18;
        VxPart* part = static_cast<VxPart*>(cg->mEntity);
        mDensity = mState->getDensity(mTM.getTranslation());
        part->getVelocityAtPoint(mTM.getTranslation(), tmp);
        mVelocity = toLocal(mTM, tmp);
        part->getAngularVelocity(tmp);
        mAngularVelocity = toLocal(mTM, tmp);
    }

    VxReal distance(const VxReal* p) const
    {
        return (mNormal[0] * p[0] + mNormal[1] * p[1] + mNormal[2] * p[2]) - mPlane;
    }

    void drag(const VxReal* a, const VxReal* b, const VxReal* c, const VxVector3& n)
    {
        triangleDrag(a, b, c, mDensity, n, mTM, mVelocity, mAngularVelocity, mFluidVelocity, mCoef, *mForces);
    }

    // the submerged part of triangle vtx (normal n, signed distances dist): drag, and the
    // tetrahedra to mApex unless the mesh is fully immersed
    void clipTriangle(const VxReal* const vtx[3], const VxReal dist[3], const VxVector3& n)
    {
        int below = 0, last = -1;
        for (int k = 0; k < 3; ++k)
            if (dist[k] < 0) {
                ++below;
                last = k;
            }
        if (below == 1) {
            int j = (last + 1) % 3;
            VxVector3 a = crossing(vtx[last], dist[last], vtx[j], dist[j]);
            int k = (j + 1) % 3;
            VxVector3 b = crossing(vtx[last], dist[last], vtx[k], dist[k]);
            drag(vtx[last], a.data(), b.data(), n);
            if (!mFull)
                addTetrahedron(vtx[last], a.data(), b.data(), mApex, mMoment, mVolume);
        } else if (below == 2) {
            // the corner above the surface and the two below it (in order after it)
            int above, i1, i2;
            int j = (last + 1) % 3;
            if (dist[j] > 0) {
                above = j;
                i1 = (last + 2) % 3;
                i2 = last;
            } else {
                above = (last + 2) % 3;
                i1 = last;
                i2 = j;
            }
            VxVector3 c1 = crossing(vtx[i1], dist[i1], vtx[above], dist[above]);
            VxVector3 c2 = crossing(vtx[i2], dist[i2], vtx[above], dist[above]);
            drag(vtx[i1], vtx[i2], c2.data(), n);
            if (!mFull)
                addTetrahedron(vtx[i1], vtx[i2], c2.data(), mApex, mMoment, mVolume);
            drag(vtx[i1], c2.data(), c1.data(), n);
            if (!mFull)
                addTetrahedron(vtx[i1], c2.data(), c1.data(), mApex, mMoment, mVolume);
        } else if (below == 3) {
            drag(vtx[0], vtx[1], vtx[2], n);
            if (!mFull)
                addTetrahedron(vtx[0], vtx[1], vtx[2], mApex, mMoment, mVolume);
        }
    }

    // buoyancy at the submerged centroid, then the totals back to world. Returns false (no
    // forces) when nothing is submerged.
    bool finish(VxCollisionGeometry* cg, VxGeometry* g)
    {
        FluidForce& ff = *mForces;
        if (mFull) {
            mMoment = g->getCenterOfMass();
            mVolume = g->getVolume();
        } else if (!(mVolume > 1e-07)) {
            ff.mForce.setZero();
            ff.mTorque.setZero();
            return false;
        } else {
            VxReal inv = 1.0 / mVolume;
            mMoment[0] = mMoment[0] * inv;
            mMoment[1] = mMoment[1] * inv;
            mMoment[2] = inv * mMoment[2];
        }
        VxReal f = buoyancyScale(cg) * mVolume * mDensity * mState->mGravity;
        ff.add(VxVector3(mNormal[0] * f, mNormal[1] * f, mNormal[2] * f), mMoment);
        ff.mForce = toWorld(mTM, ff.mForce);
        ff.mTorque = toWorld(mTM, ff.mTorque);
        return true;
    }

    const VxTransform& mTM;
    VxFluidState* mState;
    bool mFull;              // the whole mesh is below the surface
    FluidForce* mForces;
    VxReal mSurface;
    VxVector3 mNormal;       // up in the mesh frame
    VxVector3 mApex;
    VxReal mPlane;
    VxVector3 mFluidVelocity, mVelocity, mAngularVelocity, mCoef;
    VxReal mDensity;
    VxVector3 mMoment;       // submerged volume * centroid
    VxReal mVolume;
};
} // namespace

// Mac 0x44cadf, Win 0x749850: buoyancy from the submerged part of the closed mesh (tetrahedra
// from the clipped triangles to the surface point below the origin), drag per submerged
// triangle. NOTE: the fully-immersed flag is never set here, and the buoyancy is applied even
// with a zero data.m38.
bool vxTriangleMeshFluidInteractionForce(VxCollisionGeometry* cg, void* fluidState)
{
    VxFluidState* state = static_cast<VxFluidState*>(fluidState);
    const VxTransform& tm = *cg->getTransformPtr();
    VxTriangleMesh* mesh = static_cast<VxTriangleMesh*>(cg->getGeometry());
    MeshFluid m(state, tm);
    if (!m.setUp(cg))
        return false;
    if (!m.mFull) {
        // the root box of the OBB tree against the plane
        const VxVector3& nl = m.mNormal;
        VxTransform obb;
        VxReal hx = 0, hy = 0, hz = 0;
        mesh->getBoundingBox(obb, hx, hy, hz);
        VxReal ext = fabs(nl[0] * obb.m[0][0] + nl[1] * obb.m[0][1] + nl[2] * obb.m[0][2]) * hx
                     + fabs(nl[0] * obb.m[1][0] + nl[1] * obb.m[1][1] + nl[2] * obb.m[1][2]) * hy
                     + fabs(nl[0] * obb.m[2][0] + nl[1] * obb.m[2][1] + nl[2] * obb.m[2][2]) * hz;
        VxReal oc = nl[0] * obb.m[3][0] + nl[1] * obb.m[3][1] + nl[2] * obb.m[3][2];
        if (oc - ext >= m.mPlane)
            return false;
        if (m.mPlane >= ext + oc)
            m.mFull = true;
    }
    VxVector3 vf;
    state->getVelocity(tm.getTranslation(), vf);
    m.mFluidVelocity = toLocal(tm, vf);
    FluidForce ff(cg, state);
    m.setUpDrag(cg, ff);
    for (int i = 0; i < mesh->getTriangleCount(); ++i) {
        VxVector3 n;
        mesh->getTriangleNormal(i, n);
        const VxReal* vtx[3];
        mesh->getTriangleVertices(i, vtx[0], vtx[1], vtx[2]);
        VxReal dist[3];
        for (int k = 0; k < 3; ++k)
            dist[k] = m.distance(vtx[k]);
        m.clipTriangle(vtx, dist, n);
    }
    return m.finish(cg, mesh);
}

// Mac 0x451aea, Win 0x74a3f0: as the triangle mesh, over the faces of the hull (fans from the
// first vertex: triangles (0, k, k-1)); the support points along -+up decide whether it is out
// of or below the surface. NOTE: as for meshes, the fully-immersed flag is never set.
bool vxConvexMeshFluidInteractionForce(VxCollisionGeometry* cg, void* fluidState)
{
    VxFluidState* state = static_cast<VxFluidState*>(fluidState);
    const VxTransform& tm = *cg->getTransformPtr();
    VxConvexMesh* mesh = static_cast<VxConvexMesh*>(cg->getGeometry());
    MeshFluid m(state, tm);
    if (!m.setUp(cg))
        return false;
    if (!m.mFull) {
        const VxVector3& up = state->mUp;
        VxVector3 p;
        VxVector3 dir = up;
        mesh->getMaximumPoint(tm, dir, p);
        if (m.mSurface >= up[0] * p[0] + up[1] * p[1] + up[2] * p[2]) {
            m.mFull = true;
        } else {
            dir = VxVector3(-up[0], -up[1], -up[2]);
            mesh->getMaximumPoint(tm, dir, p);
            if (p[0] * up[0] + p[1] * up[1] + p[2] * up[2] >= m.mSurface)
                return false;
        }
    }
    VxVector3 vf;
    state->getVelocity(tm.getTranslation(), vf);
    m.mFluidVelocity = toLocal(tm, vf);
    FluidForce ff(cg, state);
    m.setUpDrag(cg, ff);
    for (int f = 0; f < mesh->getFaceCount(); ++f) {
        VxVector3 n;
        VxVector3 pts[3];
        mesh->getFaceNormal(f, n);
        mesh->getFaceVertex(f, 0, pts[0]);
        mesh->getFaceVertex(f, 1, pts[1]);
        VxReal dist[3];
        dist[0] = m.distance(pts[0].data());
        dist[1] = m.distance(pts[1].data());
        for (int k = 2; k < mesh->getFaceVertexCount(f); ++k) {
            pts[2] = pts[1];
            dist[2] = dist[1];
            mesh->getFaceVertex(f, k, pts[1]);
            dist[1] = m.distance(pts[1].data());
            const VxReal* vtx[3] = {pts[0].data(), pts[1].data(), pts[2].data()};
            m.clipTriangle(vtx, dist, n);
        }
    }
    return m.finish(cg, mesh);
}

// --------------------------------------------------------------------------------------
// The default force functions of the primitives (VxCollisionGeometry::setDefaultFluidInteraction-
// ForceFn picks them by geometry class). The transform is the model's world transform.

// Mac 0x44fb85, Win 0x7495d0
bool vxSphereFluidInteractionForce(VxCollisionGeometry* cg, void* fluidState)
{
    const VxTransform& tm = *cg->getTransformPtr();
    VxReal r = static_cast<VxSphere*>(cg->getGeometry())->getRadius();
    return sphereFluidForce(cg, tm, static_cast<VxFluidState*>(fluidState), r);
}

// Mac 0x451a33, Win 0x7497d0: half extents, scale 1
bool vxBoxFluidInteractionForce(VxCollisionGeometry* cg, void* fluidState)
{
    const VxTransform& tm = *cg->getTransformPtr();
    VxBox* box = static_cast<VxBox*>(cg->getGeometry());
    VxVector3 half(0, 0, 0);
    box->getDimensions(half);
    half[0] = half[0] * 0.5;
    half[1] = half[1] * 0.5;
    half[2] = half[2] * 0.5;
    return boxFluidForce(cg, tm, half.data(), static_cast<VxFluidState*>(fluidState), 1.0);
}

// Mac 0x44c5c4, Win 0x749610
bool vxCylinderFluidInteractionForce(VxCollisionGeometry* cg, void* fluidState)
{
    const VxTransform& tm = *cg->getTransformPtr();
    VxCylinder* c = static_cast<VxCylinder*>(cg->getGeometry());
    VxReal h = c->getHeight();
    VxReal r = c->getRadius();
    return axialFluidForce(cg, tm, static_cast<VxFluidState*>(fluidState), r, h * 0.5, true, true);
}

// Mac 0x44faca, Win 0x749730: a sphere when the core is shorter than a tenth of the radius, else
// the cylinder of equal volume
bool vxCapsuleFluidInteractionForce(VxCollisionGeometry* cg, void* fluidState)
{
    const VxTransform& tm = *cg->getTransformPtr();
    VxCapsule* c = static_cast<VxCapsule*>(cg->getGeometry());
    VxReal h = c->getHeight();
    VxReal r = c->getRadius();
    if (r * 0.1 > h)
        return sphereFluidForce(cg, tm, static_cast<VxFluidState*>(fluidState), r);
    return axialFluidForce(cg, tm, static_cast<VxFluidState*>(fluidState), r, (h + r * 1.3333333333333333) * 0.5, true,
                           true);
}

// Win 0x749670 (Win only): the capsule as a cylinder of its core length without cap drag
bool vxCapsuleFluidInteractionForceNoCaps(VxCollisionGeometry* cg, void* fluidState)
{
    const VxTransform& tm = *cg->getTransformPtr();
    VxCapsule* c = static_cast<VxCapsule*>(cg->getGeometry());
    VxReal h = c->getHeight();
    VxReal r = c->getRadius();
    return axialFluidForce(cg, tm, static_cast<VxFluidState*>(fluidState), r, h * 0.5, true, false);
}

// Win 0x7496d0 (Win only, no caller)
bool vxCylinderFluidInteractionForceNoCaps(VxCollisionGeometry* cg, void* fluidState)
{
    const VxTransform& tm = *cg->getTransformPtr();
    VxCylinder* c = static_cast<VxCylinder*>(cg->getGeometry());
    VxReal h = c->getHeight();
    VxReal r = c->getRadius();
    return axialFluidForce(cg, tm, static_cast<VxFluidState*>(fluidState), r, h * 0.5, true, false);
}

// Win 0x745820 (Win only, cdecl(x, y, z))
void vxSetFluidBoxCellSize(VxReal x, VxReal y, VxReal z)
{
    sBoxCellSize[0] = x;
    sBoxCellSize[1] = y;
    sBoxCellSize[2] = z;
}

} // namespace Vx
