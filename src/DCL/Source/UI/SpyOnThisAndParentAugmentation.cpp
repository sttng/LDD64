// LEGO Digital Designer — restored source
// TU 078 "SpyOnThisAndParentAugmentation.cpp" (Mac 0x16ba30-0x16be3b, Win 0x510be3-0x510c7c) and
// TU 079 (Mac 0x16be40-0x16bf6b, Win 0x51c1fe: the parent-changed handler, a second object of
// the same class).
#include "SpyOnThisAndParentAugmentation.h"

namespace LEGO {

// Mac 0x16ba30, Win 0x510be3 (Win through ControlAugmentationControlObserver::Attach 0x45a400)
bool SpyOnThisAndParentAugmentation::Attach(Control* control, const String& name)
{
    if (!ControlAugmentationControlObserver::Attach(control, name))
        return false;
    m_parent = control->GetParent();
    if (m_parent)
        ObserveControl(m_parent);
    return true;
}

// Mac 0x16bac0, Win 0x510c13
void SpyOnThisAndParentAugmentation::Detach()
{
    if (m_parent) {
        StopObservingControl(m_parent);
        m_parent = 0;
    }
    ControlAugmentationControlObserver::Detach();
}

// TU 079: Mac 0x16be40 (thunk 0x16be90), Win 0x51c1fe
void SpyOnThisAndParentAugmentation::OnDestroyed(Control* control)
{
    if (GetControl() != control)
        return;
    if (m_parent)
        StopObservingControl(m_parent);
    m_parent = control->GetParent();
    if (m_parent)
        ObserveControl(m_parent);
}

} // namespace LEGO
