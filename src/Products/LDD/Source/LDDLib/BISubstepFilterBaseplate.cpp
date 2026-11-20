// LEGO Digital Designer — restored source
// TU 301 "BISubstepFilterBaseplate.cpp" (Mac 0x2af5e0-0x2af8bf): LEGO::BISubstepFilterBaseplate.
#include "BISubstepFilterBaseplate.h"
#include "BIBrick.h"
#include "BIBrickPrimitive.h"

namespace LEGO {

// Mac 0x2af680 / 0x2af740 (thunks 0x2af6e0 / 0x2af7b0), Win sdd 0x5a1710
BISubstepFilterBaseplate::~BISubstepFilterBaseplate()
{
}

// Mac 0x2af620, Win 0x5a1890
bool BISubstepFilterBaseplate::Accept(const BIBrick* substep)
{
    const BasicSet<BIBrickPrimitive*>& primitives = substep->GetPrimitiveSet();
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = primitives.Begin(); it != primitives.End(); ++it) {
        if ((*it)->HasMainGroup(218))
            return false;
    }
    return true;
}

// Mac 0x2af830 (thunk 0x2af850), Win 0x5a19c0
void BISubstepFilterBaseplate::OnFinalRelease()
{
    Object::OnFinalRelease();
}

} // namespace LEGO
