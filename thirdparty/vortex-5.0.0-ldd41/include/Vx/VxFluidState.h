// Vortex 5.0.0 build 800 "vortexldd41" - VxFluidState: a fluid the force functions of Mac TU 486
// push collision geometries through (restored header: the interface and the
// members those functions read). All names inferred.
//
// Win-only in LDD (no fluid state is ever made), but the Mac force functions use the same slots and
// members. Win-only, declared only: the concrete states (VxTerrainFluidState Win 0x9f9e7c,
// VxHeightFieldFluidState 0x9f9e98), the ctor (Win 0x74b3b0: up (0, 0, 1), velocity 0, gravity 9.8,
// density 1000, a DefaultFluidStateSub (+8 20.0, +0x10 9.8) as subscriber), the dtor (D2 Win
// 0x74bec0: deletes the subscriber), setUp (Win 0x74b140: normalises, |v| -> mGravity), getGravity
// (Win 0x74b1d0), setDensity (Win 0x745360: re-sorts the universe's fluid states), setSubscriber /
// getSubscriber (Win 0x745390 / 0x7453a0), DefaultFluidStateSub / SetDefaultFluidVisitor.
// Win vtable 0x9f9e60 (Mac slot = Win slot + 1 after the destructor): 0 scalar deleting dtor
// 0x74bf80, 1 / 2 pure, 3 0x74bf40, 4 0x74bf60, 5 0x74bf70.
#ifndef VX_VXFLUIDSTATE_H
#define VX_VXFLUIDSTATE_H

#include "Vx/VxVector3.h"
#include <string>

namespace Vx {

class VxCollisionGeometry;
class VxUniverse;

class VxFluidState {
public:
    // Gets the force and torque a fluid force function is about to apply to the geometry's part
    // and may veto it (Win vtable 0x9f9dbc: 0 = pure, 1 = scalar deleting dtor 0x74b720 - the
    // callback is declared first). flag: the force contains the full-volume buoyancy
    // (FluidForce::mFullyImmersed). (names inferred)
    class Subscriber {
    public:
        virtual bool notifyForce(VxCollisionGeometry* cg, VxVector3& force, VxVector3& torque, bool flag) = 0;
        virtual ~Subscriber() {}
    };

    VxFluidState();                                     // Win 0x74b3b0
    virtual ~VxFluidState();                            // Mac slots 0/1, Win 0x74bf80 (D2 0x74bec0)
    // slot 2 (Win 1): the fluid surface height (along mUp) above the point p
    virtual VxReal getSurfaceHeight(const VxVector3& p) const = 0;
    // slot 3 (Win 2): lowest / highest surface height over the box [minCorner, maxCorner]
    virtual void getSurfaceHeightRange(const VxVector3& minCorner, const VxVector3& maxCorner, VxReal& minHeight,
                                       VxReal& maxHeight) const = 0;
    // slot 4 (Win 3, 0x74bf40): the fluid velocity at p
    virtual void getVelocity(const VxVector3& p, VxVector3& v) const
    {
        (void)p;
        v = mVelocity;
    }
    // slot 5 (Win 4, 0x74bf60): 0 (not called by the force functions; name unknown)
    virtual VxReal vf05(const VxVector3& p) const
    {
        (void)p;
        return 0;
    }
    // slot 6 (Win 5, 0x74bf70): the fluid density at p
    virtual VxReal getDensity(const VxVector3& p) const
    {
        (void)p;
        return mDensity;
    }

    // Mac 0x44847e (out of line in TU 486), Win 0x7453a0
    Subscriber* getSubscriber() const { return mSubscriber; }

    Subscriber* mSubscriber;    // Mac +0x04, Win +0x08 (MSVC pads the vfptr: the class has doubles)
    std::string mName;          // Mac +0x08, Win +0x0c (28 bytes)
    VxVector3 mUp;              // Mac +0x0c, Win +0x28 (unit; the direction buoyancy pushes to)
    VxVector3 mVelocity;        // Mac +0x24, Win +0x40
    VxReal mGravity;            // Mac +0x3c, Win +0x58 (9.8)
    // (Mac offsets from here on follow from the Win layout; no Mac code reads them)
    VxUniverse* mUniverse;      // Mac +0x44, Win +0x60
    VxReal mDensity;            // Mac +0x48, Win +0x68 (1000; VxFluidInteractionUniverseUpdate keeps the states sorted by it)
};

} // namespace Vx

#endif
