// Vortex 5.0.0 build 800 "vortexldd41" - VxSteppingController (restored).
// Mac TU 451 (0x42247c-0x422bb7), Win 0x7fe920-0x7feb90.
#include "Vx/VxSteppingController.h"

namespace Vx {

// Mac 0x42247c (0x180), 0x422486 (isOfClassType), 0x422740 (types), 0x422ac8 (isOfType);
// Win 0x7fe9b0, 0x7feab0, 0x7feb30
VX_IMPLEMENT_CLASS_TYPES(VxSteppingController, VxControllerBase, 0x180)

// Mac 0x42259a, Win 0x7fe9f0 (plain operator new(8) for the subscriber)
VxSteppingController::VxSteppingController()
    : VxControllerBase(), mBaseSubscriber(0), mUniverse(0)
{
    mBaseSubscriber = new BaseSubscriber(this);
}

// Mac D1 0x42272c (body 0x4226d6) / D0 0x422678, Win 0x7feb90 (body 0x7fe920)
VxSteppingController::~VxSteppingController()
{
    delete mBaseSubscriber;
}

// Mac 0x4224d6, Win 0x7fe990
void VxSteppingController::setUniverse(VxUniverse* u)
{
    mUniverse = u;
}

// Mac 0x4224e8, Win 0x7fea80: on the destruction event (0) tell the controller and leave the
// object's event-0 subscriber list.
void VxSteppingController::BaseSubscriber::notify(VxBase::Event event, VxBase* object)
{
    if ((int)event == 0) {
        mOwner->onBaseObjectDestroyed(object);
        object->mSubscribers.removeSubscriber(event, this);
    }
}

} // namespace Vx
