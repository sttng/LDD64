// Vortex 5.0.0 build 800 "vortexldd41" - VxState: the saved state of an assembly (restored;
// Mac TU 518 0x4795b6-0x47a73e, Win 0x78ab30-0x78b3a0 + 0x7f3d80 /
// 0x7f3f20 / 0x7f4010). Children: a VxStateEntity per part, a VxStateConstraint per constraint,
// the sub-assemblies' states and the embedded composite controller's (createState of each).
#ifndef VX_VXSTATE_H
#define VX_VXSTATE_H

#include "Vx/VxStateBase.h"

namespace Vx {

class VxAssemblyBase;

// VxState (Mac vtable 0x75e888; slots 0 0x479e42 (dtor), 3 0x47a692, 4 0x47a342, 5 0x479eca,
// 6 0x479e56, 7 0x479e8c, 8 0x479f00; Win 0x78b370, 0x78b310, 0x78b1f0, 0x78b120, 0x78b150,
// 0x78b2d0, 0x78ac40). Mac 0x1c bytes (VxAssemblyBase::createState: new(0x1c)), Win 0x34.
// Restore modes (VxAssemblyBase::getM3d4 and the mode argument, the larger wins): 0 = full,
// 1 = kinematic, 2 = only the assembly's placement, 3 = nothing (names inferred).
class VxState : public VxStateBase {
public:
    explicit VxState(VxAssemblyBase* assembly);         // C1 Mac 0x47a3c2 -> C2 0x47a360, Win 0x78b180
    VxState(const VxState& other);                      // C1 Mac 0x479dc6 -> C2 0x479d8e, Win 0x7f3d80 / 0x7f4010
    virtual ~VxState();                                 // Mac D1 0x479e42 (-> D2 0x479e18) / D0 0x479de0, Win 0x78b370
    virtual VxStateBase* clone() const;                 // slot 3 Mac 0x47a692, Win 0x78b310
    virtual void save();                                // slot 4 Mac 0x47a342, Win 0x78b1f0
    virtual void restore(bool notify);                  // slot 5 Mac 0x479eca, Win 0x78b120
    virtual void restoreKinematic(bool notify);         // slot 6 Mac 0x479e56, Win 0x78b150
    virtual void restoreMode(int mode, bool notify);    // slot 7 Mac 0x479e8c, Win 0x78b2d0
    virtual void reconnectBaseObject();                 // slot 8 Mac 0x479f00, Win 0x78ac40

    // Mac 0x479f6c, Win 0x78add0: updates / creates the child states of the assembly's parts,
    // constraints, sub-assemblies and controller, deletes the others (name inferred)
    void save(VxAssemblyBase* assembly);
};

} // namespace Vx

#endif
