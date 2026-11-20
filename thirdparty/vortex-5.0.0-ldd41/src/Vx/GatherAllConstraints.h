// Vortex 5.0.0 build 800 "vortexldd41" - the gathering visitors of VxUniverse::print (restored,
// class names from RTTI). Both derive virtually from VxVisitor (Mac RTTI:
// virtual base at -44) so that GatherAllConstraintsAndSensors has one visitor. Private header of
// src/Vx/VxUniverse.cpp (Mac TU 528) and src/Vx/GatherAllConstraintsAndSensors.cpp (TU 499).
#ifndef VX_GATHERALLCONSTRAINTS_H
#define VX_GATHERALLCONSTRAINTS_H

#include "Vx/VxVisitor.h"
#include "Vx/VxArray.h"

namespace Vx {

class VxConstraint;
class VxCollisionSensor;

// Mac vtable 0x75ed2c (as base 0x75ebec): visitConstraint 0x485e4c (push_back), dtor 0x485ba0 /
// 0x485b2a. The array starts with room for 128 (Mac 0x48626e).
class GatherAllConstraints : public virtual VxVisitor {
public:
    GatherAllConstraints() { mConstraints.reserve(0x80); }
    virtual ~GatherAllConstraints() {}
    virtual int visitConstraint(VxConstraint* c);
    VxArray<VxConstraint*> mConstraints;    // Mac +0x04
};

// Mac vtable 0x75ea8c (as base 0x75ec4c): visitEntity 0x4869ba (the entities that are collision
// sensors, i.e. parts and sensors), dtor 0x485fc8 / 0x485f40.
class GatherAllCollisionSensor : public virtual VxVisitor {
public:
    GatherAllCollisionSensor() { mSensors.reserve(0x80); }
    virtual ~GatherAllCollisionSensor() {}
    virtual int visitEntity(VxEntity* e);
    VxArray<VxCollisionSensor*> mSensors;   // Mac +0x18 in GatherAllConstraintsAndSensors
};

// Mac vtable 0x75eb4c / 0x75eb9c (ctor inline in VxUniverse::print, Mac 0x48626e / Win
// 0x70d400; dtor 0x48654e / 0x48642c); visitEntity Mac 0x45c4c8 (TU 499).
class GatherAllConstraintsAndSensors : public GatherAllConstraints, public GatherAllCollisionSensor {
public:
    GatherAllConstraintsAndSensors() {}
    virtual ~GatherAllConstraintsAndSensors() {}
    virtual int visitEntity(VxEntity* e);
};

} // namespace Vx

#endif
