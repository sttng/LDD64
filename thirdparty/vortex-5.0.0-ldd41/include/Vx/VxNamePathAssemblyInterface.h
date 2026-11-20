// Vortex 5.0.0 build 800 "vortexldd41" - the name-path interfaces of the dynamics objects
// (restored header; src/Vx/VxNamePathAssemblyInterface.cpp = Mac TU 505
// 0x464682-0x4673a3, Win 0x822710-0x823060). Each gives the parent and children of one class
// (Vx/VxNamePathBaseInterface.h); TU 505's static initialiser registers them for VxFrame,
// VxUniverse, VxMechanism and VxAssemblyBase (both the assembly one), VxCollisionSensor,
// VxCollisionGeometry and VxConstraint. Mac vtables 0x75e248 (frame), 0x75e260 (universe),
// 0x75e278 (assembly), 0x75e290 (sensor), 0x75e2a8 (constraint), 0x75e2c0 (collision geometry);
// shared Win destructor 0x823060. Method names inferred.
#ifndef VX_VXNAMEPATHASSEMBLYINTERFACE_H
#define VX_VXNAMEPATHASSEMBLYINTERFACE_H

#include "Vx/VxNamePathBaseInterface.h"

namespace Vx {

// parent: none; children: the frame's universes
class VxNamePathFrameInterface : public VxNamePathBaseInterface {
public:
    virtual ~VxNamePathFrameInterface();                                         // Mac D1 0x46619c / D0 0x466172
    virtual void getChildren(VxBase* object, VxArray<VxBase*>& children) const;  // Mac 0x464932, Win 0x822b20
};

// parent: the frame; children: the root mechanism, and its sub-assemblies when the universe's
// flag (Mac +0x8c) is set
class VxNamePathUniverseInterface : public VxNamePathBaseInterface {
public:
    virtual ~VxNamePathUniverseInterface();                                      // Mac D1 0x466158 / D0 0x46612e
    virtual VxBase* getParent(VxBase* object) const;                             // Mac 0x4661b6, Win 0x823050 (VxFrame::instance())
    virtual void getChildren(VxBase* object, VxArray<VxBase*>& children) const;  // Mac 0x464c82, Win 0x822a90
};

// parent: the parent assembly, else the universe; children: parts, constraints,
// sub-assemblies, then the embedded controller
class VxNamePathAssemblyInterface : public VxNamePathBaseInterface {
public:
    virtual ~VxNamePathAssemblyInterface();                                      // Mac D1 0x466114 / D0 0x4660ea
    virtual VxBase* getParent(VxBase* object) const;                             // Mac 0x464734, Win 0x8227f0
    virtual void getChildren(VxBase* object, VxArray<VxBase*>& children) const;  // Mac 0x465c2c, Win 0x8229b0
};

// parent: the sensor's assembly; children: its collision geometries
class VxNamePathCollisionSensorInterface : public VxNamePathBaseInterface {
public:
    virtual ~VxNamePathCollisionSensorInterface();                               // Mac D1 0x4660d0 / D0 0x4660a6
    virtual VxBase* getParent(VxBase* object) const;                             // Mac 0x464790, Win 0x8227b0
    virtual void getChildren(VxBase* object, VxArray<VxBase*>& children) const;  // Mac 0x4658ca, Win 0x822940
};

// parent: the constraint's assembly (VxConstraint +0x34); no children
class VxNamePathConstraintInterface : public VxNamePathBaseInterface {
public:
    virtual ~VxNamePathConstraintInterface();                                    // Mac D1 0x466048 / D0 0x46601e
    virtual VxBase* getParent(VxBase* object) const;                             // Mac 0x4647dc, Win 0x822770
};

// parent: the owning part / sensor; no children
class VxNamePathCollisionGeometryInterface : public VxNamePathBaseInterface {
public:
    virtual ~VxNamePathCollisionGeometryInterface();                             // Mac D1 0x46608c / D0 0x466062
    virtual VxBase* getParent(VxBase* object) const;                             // Mac 0x464828, Win 0x822730
};

} // namespace Vx

#endif
