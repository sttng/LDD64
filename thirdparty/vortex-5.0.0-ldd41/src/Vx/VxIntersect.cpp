// Vortex 5.0.0 build 800 "vortexldd41" - VxIntersect (restored).
// Mac TU 497 0x45bfa8-0x45c140 (file guess Vx/VxIntersect.cpp), Win 0x71e960-0x71ea40.
// (IntersectSubscriber's ctor, Mac 0x45bfa8 / Win 0x71e960, is inline in Vx/VxIntersect.h.)
#include "Vx/VxIntersect.h"
#include "Vx/VxBridge.h"
#include "Vx/VxEntity.h"
#include "Vx/VxUniverse.h"

namespace Vx {

// Mac 0x45bfea, Win 0x71e970
bool VxIntersect::intersect(VxCollisionSensor* sensor, IntersectSubscriber* subscriber, bool update)
{
    VxAssert(sensor != 0, "VxIntersect::intersect: collision geometry is NULL\n");
    VxUniverse* universe = sensor->getUniverse();
    if (universe == 0) {
        VxWarning(0, "VxIntersect::intersect: This method requires the sensor to be added to the universe.\n");
        return false;
    }
    if (update)
        universe->mIntersectManager->updatePairs();
    bool found = false;
    for (int i = 0; i < sensor->getCollisionGeometryCount(); ++i) {
        if (subscriber != 0)
            subscriber->mCollisionGeometry = sensor->getCollisionGeometry(i);
        VxReal dt = universe->mTimeStep;
        int r = universe->mIntersectManager->intersect(sensor->getCollisionGeometry(i), dt, subscriber);
        if (r != 0) {
            if (r == 1)
                return true;
            found = true;
        }
    }
    return found;
}

} // namespace Vx
