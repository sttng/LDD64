// Vortex 5.0.0 build 800 "vortexldd41" - VxStateBase (restored:
// Mac TU 519 0x47a73e-0x47b735, Win 0x7f39a0-0x7f4090). The saved state of one Vortex object plus
// its children's states. Watches the object as a VxBase subscriber (event 0, destroyed: mObject = 0)
// and keeps its name so that reconnectBaseObject can find it again.
#ifndef VX_VXSTATEBASE_H
#define VX_VXSTATEBASE_H

#include "Vx/VxBase.h"
#include "Vx/VxArray.h"
#include <string>

namespace Vx {

class VxTransform;

// VxStateBase (Mac vtable 0x75e8c8, 11 slots; Win 10). Mac 0x1c bytes, Win 0x34.
// (method names inferred unless noted)
class VxStateBase : public VxBase::Subscriber {
public:
    explicit VxStateBase(VxBase* object);                    // Mac 0x47ab3c (inlined on Win)
    VxStateBase(const VxStateBase& other);                    // Mac 0x47b14c (copies the children with clone)
    // Win 0x7f3bf0 (Win only; dead-stripped on Mac): the same as the copy constructor on an
    // existing state (the old children are deleted first)
    VxStateBase& operator=(const VxStateBase& other);
    virtual ~VxStateBase();                                   // slot 0/1 Mac 0x47b02c (-> D2 0x47af06) / 0x47ac24, Win 0x7f4060 (D2 0x7f3e50)
    virtual void notify(VxBase::Event event, VxBase* object); // slot 2 Mac 0x47a73e, Win 0x7f39a0 (mObject = 0)
    virtual VxStateBase* clone() const = 0;                   // slot 3
    virtual void save() = 0;                                  // slot 4 (capture the object's current state)
    virtual void restore(bool notify) = 0;                    // slot 5
    virtual void restoreKinematic(bool notify) = 0;           // slot 6 ("restore kinematic state")
    virtual void restoreMode(int mode, bool notify) = 0;      // slot 7
    virtual void reconnectBaseObject();                       // slot 8 Mac 0x47a7be, Win 0x7f3a60 (name from "VxState::reconnectBaseObject")
    virtual void setBaseObject(VxBase* object);               // slot 9 Mac 0x47a8b8, Win 0x7f3ca0
    // slot 10 Mac 0x47a82a, Win 0x7f3a30: forwards to the children (VxStateEntity moves its
    // saved transform / velocities by t)
    virtual void transform(const VxTransform& t);

    VxStateBase* findChildState(VxBase* object);              // Mac 0x47a7ee, Win 0x7f39f0 (this when object is ours, else a direct child's)
    VxStateBase* findStateRecursive(VxBase* object);          // Mac 0x47a74e, Win 0x7f39b0 (this or any descendant)
    void addChildState(VxStateBase* s);                       // Mac 0x47b040 (if not null and absent; inlined on Win)
    void removeChildState(VxStateBase* s);                    // Mac 0x47a860, Win 0x7f3ac0 (first occurrence, not deleted)
    // Mac 0x47ad52, Win 0x7f3b00: deletes the child states whose object is gone (recursively
    // purges the connected ones first)
    void removeDisconnectedStates();

    VxBase* getBaseObject() const { return mObject; }
    const std::string& getObjectName() const { return mObjectName; }

    VxArray<VxStateBase*> mChildren;  // Mac +0x04, Win +0x04
    VxBase* mObject;                  // Mac +0x14, Win +0x14
    std::string mObjectName;          // Mac +0x18, Win +0x18
};

} // namespace Vx

#endif
