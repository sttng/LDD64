// Vortex 5.0.0 build 800 "vortexldd41" - VxControllerState (restored header).
//
// The saved state of a controller (VxControllerBase::createState): copies of its Inputs /
// Outputs / Parameters containers and, for a composite, the states of its children.
// Mac TU 448 "VxControllerState.cpp" (0x42051f-0x420e6d), Win 0x824110-0x8247d0 (+ 0x7f3d10).
#ifndef VX_VXCONTROLLERSTATE_H
#define VX_VXCONTROLLERSTATE_H

#include "Vx/VxStateBase.h"

namespace VxData {
class FieldBase;
}

namespace Vx {

class VxControllerBase;

// VxControllerState (Mac vtable 0x75cd28, 11 slots; Win 10). Mac 0x28 bytes, Win 0x40.
class VxControllerState : public VxStateBase {
public:
    explicit VxControllerState(VxControllerBase* controller);   // Mac C2 0x420d7c / C1 0x420dee, Win 0x7f3d10 / 0x8246e0
    VxControllerState(const VxControllerState& other);          // Mac C2 0x4206fa / C1 0x4207ae, Win 0x824110
    virtual ~VxControllerState();                               // slot 0/1 Mac 0x4206e6 (-> 0x42067a) / 0x420606, Win 0x8247d0
    virtual VxStateBase* clone() const;                         // slot 3 Mac 0x4207c8, Win 0x824340
    virtual void save();                                        // slot 4 Mac 0x420d5e, Win 0x824750
    virtual void restore(bool notify);                          // slot 5 Mac 0x420810, Win 0x824430
    virtual void restoreKinematic(bool notify);                 // slot 6 Mac 0x4209f6, Win 0x824490
    virtual void restoreMode(int mode, bool notify);            // slot 7 Mac 0x4208b8, Win 0x824760
    virtual void reconnectBaseObject();                         // slot 8 Mac 0x4205c4, Win 0x824320

    // copies the controller's containers and updates the child states (Mac 0x420a86, Win 0x8244f0) (name inferred)
    void save(VxControllerBase* controller);
    // writes the copies back (Mac 0x420540, Win 0x8243b0) (name inferred)
    void restoreTo(VxControllerBase* controller);

    VxData::FieldBase* mInputs;      // Mac +0x1c, Win +0x34 (clone of the Inputs container)
    VxData::FieldBase* mOutputs;     // Mac +0x20, Win +0x38
    VxData::FieldBase* mParameters;  // Mac +0x24, Win +0x3c
};

} // namespace Vx

#endif
