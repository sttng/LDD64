// Vortex 5.0.0 build 800 "vortexldd41" - VxDynamicsContact (restored).
// Mac TU 482 (0x44602a-0x446118; static initialiser 0x4460bf: <iostream> + MdtConstraintMaxSize).
// The object pointer IS the MdtContactID (Vx/VxIntersect.h). The TU also holds two template
// helpers emitted here first and used by VxFrame / VxUniverse code (Mac 0x4460b0: a pointer
// getter, 0x4460ba: the VxArray<T*> element-destroy loop, empty); they come from the headers.
#include "Vx/VxIntersect.h"
#include "Mdt/MdtConstraint.h"
#include <iostream>

namespace Vx {

// Mac 0x44604c, Win 0x7251e0: enables / disables the contact constraint when its state differs;
// returns the resulting state (name inferred)
bool VxDynamicsContact::setEnabled(bool enable)
{
    MdtContactID c = getMdtContact();
    if (!enable) {
        if (MdtConstraintIsEnabled(c))
            MdtConstraintDisable(c);
    } else if (!MdtConstraintIsEnabled(c)) {
        MdtConstraintEnable(c);
    }
    return MdtConstraintIsEnabled(c) != 0;
}

// Mac 0x44609c, Win 0x784740 (folded with MdtContactGetPenetration)
VxReal VxDynamicsContact::getPenetration() const
{
    return MdtContactGetPenetration(getMdtContact());
}

} // namespace Vx
