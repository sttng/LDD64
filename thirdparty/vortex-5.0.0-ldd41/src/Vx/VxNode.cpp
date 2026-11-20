// Vortex 5.0.0 build 800 "vortexldd41" - VxNode reference counting (restored).
// Mac TU 506 0x4673a4-0x467481 (file name unknown; static initialiser 0x46741c: <iostream>),
// Win 0x760b10 / 0x760b30. The node interface is (vxGetNodeInterface, Mac 0x458f3d).
#include "Vx/VxSceneGraphInterface.h"
#include <iostream>

namespace Vx {

// Mac 0x4673f6, Win 0x760b10
void VxNode::addRef()
{
    if (mHandle != 0)
        vxGetNodeInterface()->addRef(*this);
}

// Mac 0x4673c6, Win 0x760b30: the handle is cleared when the interface reports it gone
void VxNode::release()
{
    if (mHandle != 0) {
        if (vxGetNodeInterface()->release(*this) == 0)
            mHandle = 0;
    }
}

} // namespace Vx
