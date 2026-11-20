// Vortex 5.0.0 build 800 "vortexldd41" - VxDynamicsResponseInput / VxDynamicsContactInput
// (restored). Mac TU 483 0x446130-0x44664e (static initialiser 0x44660b: the
// MdtConstraint.h MdtConstraintMaxSize), Win 0x744c00-0x744f30. VxDynamicsResponseInput's type
// list is emitted at the start of the next TU (Mac 0x44664e / atexit 0x446736).
#include "Vx/VxDynamicsContactInput.h"
#include "Mdt/MdtConstraint.h"

namespace Vx {

// Mac 0x44664e
const VxArray<int>& VxDynamicsResponseInput::getStaticClassTypes()
{
    static VxArray<int> sTypes(1, getStaticClassType());
    return sTypes;
}

// getStaticClassType Mac 0x446130, isOfClassType Mac 0x44613a / Win 0x744c00,
// getStaticClassTypes Mac 0x44622c, isOfType Mac 0x4465b4 / Win 0x744ef0
VX_IMPLEMENT_CLASS_TYPES(VxDynamicsContactInput, VxDynamicsResponseInput, 0x114)

// C1 Mac 0x446170 -> C2 0x446150
VxDynamicsContactInput::VxDynamicsContactInput(MdtContactID contact)
    : mContact(contact)
{
}

// D2 Mac 0x4461b4 (D1 0x4461ce, D0 0x44618a / Win 0x744f30)
VxDynamicsContactInput::~VxDynamicsContactInput()
{
}

// Mac 0x4465fa, Win 0x744f20
const char* VxDynamicsContactInput::getClassName() const
{
    return "VxDynamicsContactInput";
}

} // namespace Vx
