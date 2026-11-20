// Vortex 5.0.0 build 800 "vortexldd41" - VxSteppingController (restored header).
// A controller in the assembly / universe step: three step phases (Mac slots 16-18, pure here)
// driven by the owning VxAssemblyBase through its VxCompositeController, plus its universe.
// Mac TU 451 "VxSteppingController.cpp" (0x42247c-0x422bb7), Win 0x7fe920-0x7feb90.
#ifndef VX_VXSTEPPINGCONTROLLER_H
#define VX_VXSTEPPINGCONTROLLER_H

#include "Vx/VxControllerBase.h"

namespace Vx {

class VxUniverse;

// --------------------------------------------------------------------------------------
// VxSteppingController (Mac vtable 0x75ce88, 21 slots; Win 20; class id 0x180 Mac 0x42247c)
// Mac 0x304 bytes, Win 0x3f4. Abstract.
class VxSteppingController : public VxControllerBase {
public:
    // VxSteppingController::BaseSubscriber (Mac vtable 0x75ce68: D1 0x422b38, D0 0x422b0e,
    // notify 0x4224e8; Win 0x7feb60 / 0x7fea80): forwards the destruction of a watched
    // object to its controller and unsubscribes.
    class BaseSubscriber : public VxBase::Subscriber {
    public:
        explicit BaseSubscriber(VxSteppingController* owner) : mOwner(owner) {} // Mac 0x42249c / 0x4224bc, Win 0x7fe9d0
        virtual ~BaseSubscriber() {}
        virtual void notify(VxBase::Event event, VxBase* object);           // Mac 0x4224e8, Win 0x7fea80

        VxSteppingController* mOwner; // +0x04
    };

    VX_DECLARE_CLASS_TYPES();                      // slots 0/1: Mac 0x422ac8 / 0x422486, Win 0x7feb30 / 0x7fe9b0; types Mac 0x422740
    VxSteppingController();                        // Mac 0x42259a, Win 0x7fe9f0
    virtual ~VxSteppingController();               // slot 2/3 Mac 0x42272c (-> 0x4226d6) / 0x422678, Win 0x7feb90 (-> 0x7fe920)
    virtual void preStep() = 0;                    // slot 16 (name inferred)
    virtual void step() = 0;                       // slot 17 (name inferred)
    virtual void postStep() = 0;                   // slot 18 (name inferred)
    virtual void setUniverse(VxUniverse* u);       // slot 19 Mac 0x4224d6, Win 0x7fe990 (name inferred)
    // slot 20 Mac 0x40a3d6 (emitted in VxCompositeController.cpp), Win 0x713eb0: empty (name inferred)
    virtual void onBaseObjectDestroyed(VxBase* object) { (void)object; }

    VxUniverse* getUniverse() const { return mUniverse; }

    BaseSubscriber* mBaseSubscriber;  // Mac +0x2fc, Win +0x3ec (new BaseSubscriber(this)) (name inferred)
    VxUniverse* mUniverse;            // Mac +0x300, Win +0x3f0 (name inferred)
};

} // namespace Vx

#endif
