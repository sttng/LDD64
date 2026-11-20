// Vortex 5.0.0 build 800 "vortexldd41" - VxBaseUserData (restored).
// Mac TU 459 0x431ec4-0x43249d (file guess Vx/VxBaseUserData.cpp); Win 0x763050-0x7634b0.
#include "Vx/VxBase.h"
#include "VxData/Container.h"

namespace Vx {

// getStaticClassType Mac 0x431ec4 (0x198), isOfClassType Mac 0x431ece / Win 0x7630e0,
// getStaticClassTypes Mac 0x43206a, isOfType Mac 0x4323f2 / Win 0x763180
VX_IMPLEMENT_CLASS_TYPES(VxBaseUserData, VxBase, 0x198)

// Mac 0x431f06, Win 0x763050
VxBaseUserData::VxBaseUserData()
    : mUserDataFields(0)
    , mUserData(0)
{
}

// D2 Mac 0x432002 / Win 0x763070 (D1 Mac 0x432056, D0 Mac 0x431fa6 / Win 0x7634b0)
VxBaseUserData::~VxBaseUserData()
{
    if (mUserDataFields)
        delete mUserDataFields;
}

} // namespace Vx
