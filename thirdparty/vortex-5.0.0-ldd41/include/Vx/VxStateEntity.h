// Vortex 5.0.0 build 800 "vortexldd41" - VxStateEntity: the saved state of a part or sensor
// (restored header; Mac TU 521 0x47d60a-0x47e3aa, Win 0x7f6840-0x7f7370).
// Created by VxState::save for every part of an assembly.
#ifndef VX_VXSTATEENTITY_H
#define VX_VXSTATEENTITY_H

#include "Vx/VxStateBase.h"
#include "Vx/VxTransform.h"
#include "Vx/VxVector3.h"

namespace Vx {

class VxEntity;
class VxPart;

// VxStateEntity (Mac vtable 0x75e988, 11 slots; Win 10). Mac 0xdc bytes (new(0xdc)), Win 0x110.
// (member names inferred)
class VxStateEntity : public VxStateBase {
public:
    explicit VxStateEntity(VxEntity* entity);           // C1 Mac 0x47dbc4 -> C2 0x47daa8, Win 0x7f6f10
    VxStateEntity(const VxStateEntity& other);          // C1 Mac 0x47da8e -> C2 0x47d8ec, Win 0x7f7070
    virtual ~VxStateEntity();                           // Mac D1 0x47e23a / D0 0x47e2bc, Win 0x7f72b0 (D2 0x7f71f0)
    virtual VxStateBase* clone() const;                 // slot 3 Mac 0x47e1f2, Win 0x7f7310
    virtual void save();                                // slot 4 Mac 0x47dbde, Win 0x7f69e0
    virtual void restore(bool notify);                  // slot 5 Mac 0x47e1c8, Win 0x7f6ff0
    virtual void restoreKinematic(bool notify);         // slot 6 Mac 0x47dfa8, Win 0x7f7010
    virtual void restoreMode(int mode, bool notify);    // slot 7 Mac 0x47e176, Win 0x7f7030 (0 full, 1 kinematic)
    virtual void reconnectBaseObject();                 // slot 8 Mac 0x47dbfc, Win 0x7f6af0 ("VxStateEntity::reconnectBaseObject")
    virtual void setBaseObject(VxBase* object);         // slot 9 Mac 0x47d824, Win 0x7f7180 (+ the assembly's name)
    virtual void transform(const VxTransform& t);       // slot 10 Mac 0x47dc90, Win 0x7f69f0

    // Mac 0x47d6d8, Win 0x7f6840: captures transform, control, velocities, enabled / frozen
    // flags and the merge parent (name inferred)
    void save(VxEntity* entity);
    // Mac 0x47dfd2, Win 0x7f6b90: everything, the saved merge parent included ("VxStateEntity::restore")
    void restore(VxEntity* entity, bool notify);
    // Mac 0x47de18, Win 0x7f6d00: transform and velocities only; frozen entities are skipped
    void restoreKinematic(VxEntity* entity, bool notify);

    VxTransform mTransform;            // Mac +0x1c, Win +0x38 (the constructor sets only column 3)
    VxVector3 mLinearVelocity;         // Mac +0x9c, Win +0xb8
    VxVector3 mAngularVelocity;        // Mac +0xb4, Win +0xd0
    int mControl;                      // Mac +0xcc, Win +0xe8 (VxEntity::mControl)
    bool mFrozen;                      // Mac +0xd0, Win +0xec (mInternalControl == kControlFrozen)
    bool mEnabled;                     // Mac +0xd1, Win +0xed (VxPart::isEnabled; = true)
    std::string mAssemblyName;         // Mac +0xd4, Win +0xf0 (the entity's assembly, for reconnecting)
    VxPart* mMergeParent;              // Mac +0xd8, Win +0x10c (VxPart::getMergeParent)
};

} // namespace Vx

#endif
