// LEGO Digital Designer — restored source
// LEGO::SpyOnThisAndParentAugmentation — a ControlAugmentationControlObserver that also observes
// the control's parent (re-targeted when the parent changes). Base of SliderAugmentation and
// other augmentations reacting to their control's and its parent's notifications.
// TU 078 "SpyOnThisAndParentAugmentation.cpp" (Mac 0x16ba30-0x16be3b, Win 0x510be3-0x510c7c) and
// TU 079 (Mac 0x16be40-0x16bf6b: the parent-changed handler, Win 0x51c1fe; a second object file
// of the class, name unknown); both bodies in SpyOnThisAndParentAugmentation.cpp.
// RTTI (Mac): si, base ControlAugmentationControlObserver. Vtables 0x712dac {GetControl 0x113da0,
// Attach 0x16ba30, Detach 0x16bac0, D1 0x16bb30, D0 0x16bb90, OnFinalRelease 0x163790,
// OnParentChanged 0x16be40 (Mac-only primary entry)}, ControlObserver-in 0x712dd4 (slot 7 =
// thunk 0x16be90), Object-in 0x712e74. Win vtable {GetControl, Attach 0x510be3, Detach 0x510c13},
// sdd 0x510c7c.
// Mac layout: CACO +0x00..+0x2c, Control* m_parent +0x2c, Object +0x30. Win: m_parent +0x28.
// Class name from RTTI; the rest inferred.
#pragma once
#include "LEGO/Types.h"
#include "ControlAugmentationControlObserver.h"

namespace LEGO {

class SpyOnThisAndParentAugmentation : public ControlAugmentationControlObserver {
public:
    SpyOnThisAndParentAugmentation() : m_parent(0) {}
    virtual ~SpyOnThisAndParentAugmentation() {}                           // Mac 0x16bb30/0x16bb90, Win 0x510c32 (sdd 0x510c7c)

    // Attaches (as ControlAugmentationControlObserver) and observes the control's parent.
    virtual bool Attach(Control* control, const String& name);              // Mac 0x16ba30, Win 0x510be3
    // Stops observing the parent, then detaches.
    virtual void Detach();                                                  // Mac 0x16bac0, Win 0x510c13

    Control* GetParent() const { return m_parent; }

protected:
    // ControlObserver slot 7 (sent after a control's parent changed; named OnDestroyed in Control.h):
    // when it is the augmented control, observe its new parent instead.
    virtual void OnDestroyed(Control* control);                             // Mac 0x16be40 (thunk 0x16be90), Win 0x51c1fe

    Control* m_parent;   // Mac +0x2c, Win +0x28 (observed, not referenced)
};

} // namespace LEGO
