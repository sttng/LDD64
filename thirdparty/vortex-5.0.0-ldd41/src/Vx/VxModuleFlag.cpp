// Vortex 5.0.0 build 800 "vortexldd41" - Mac TU 480 (0x445ebe-0x445f3f; restored).
// One function that clears a byte flag (Mac 0x779318, initialised 0); VxFrame's
// destructor calls it (Mac 0x4583xx: after deleting the worlds, before 0x4bfe24). No reader of
// the flag survives in either binary (its users were dead-stripped). File and all names inferred.
#include <iostream>

namespace Vx {

bool gVxModuleInitialized = false; // Mac 0x779318 (name inferred)

// Mac 0x445ebe (name inferred)
void vxResetModuleInitialized()
{
    gVxModuleInitialized = false;
}

} // namespace Vx
