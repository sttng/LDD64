// Vortex 5.0.0 build 800 "vortexldd41" - GatherAllConstraintsAndSensors::visitEntity (restored).
// Mac TU 499 (0x45c44f-0x45c53f: the function, two VxArray copy helpers and
// the <iostream> initialiser 0x45c4da): a file of its own in the original (name from the class).
#include "GatherAllConstraints.h"
#include <iostream>

namespace Vx {

// Mac 0x45c4c8 (Win: folded into the GatherAllCollisionSensor body)
int GatherAllConstraintsAndSensors::visitEntity(VxEntity* e)
{
    GatherAllCollisionSensor::visitEntity(e);
    return 0;
}

} // namespace Vx
