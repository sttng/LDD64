// LEGO Digital Designer — restored source
// LEGO::ControlAugmentationControlObserver — a ControlAugmentation that also observes its control
// (private ControlObserver) while attached. Base of SpyOnThisAndParentAugmentation (so of
// SliderAugmentation), the LDD drag / pop-up augmentations …
// Header-only (weak copies): Mac Attach 0x1638c0, Detach 0x163740, OnFinalRelease 0x163790
// (thunks 0x1637f0 / 0x163850), dtors 0x163930/0x163990 — coalesced into TU 071
// (LDDWindowControl); Win Attach 0x45a400, Detach 0x45a430, dtor 0x45a460.
// RTTI (Mac): vmi, bases ControlAugmentation +0, ControlObserver +0x18 (private); vtables
// 0x712ecc {GetControl, Attach, Detach, D1, D0, OnFinalRelease}, ControlObserver-in 0x712ef0,
// Object-in 0x712f90. Win: vtable 0x9cd7bc {GetControl 0x52f4b0, Attach 0x45a400,
// Detach 0x45a430}, ControlObserver-in +0x10 (0x9cd73c), Object-in 0x9cd724.
// Mac layout: ControlAugmentation vptr / control +4 / name +8, ControlObserver vptr +0x18
// (subjects +0x1c), Object +0x2c. Win: vfptr, vbptr, control +8, name +0xc, ControlObserver
// +0x10, Object +0x28. Class name from RTTI; the rest inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "ControlAugmentation.h"
#include "Control.h"

namespace LEGO {

class ControlAugmentationControlObserver : public ControlAugmentation, private ControlObserver {
public:
    ControlAugmentationControlObserver() {}
    virtual ~ControlAugmentationControlObserver() {}                        // Mac 0x163930/0x163990

    // ControlAugmentation::Attach, then observe the control. Mac 0x1638c0, Win 0x45a400
    virtual bool Attach(Control* control, const String& name)
    {
        if (!ControlAugmentation::Attach(control, name))
            return false;
        ControlObserver::Attach(control);
        return true;
    }
    // Stop observing the control, then ControlAugmentation::Detach. Mac 0x163740, Win 0x45a430
    virtual void Detach()
    {
        if (GetControl())
            ControlObserver::Detach(GetControl());
        ControlAugmentation::Detach();
    }

protected:
    // Leaves every observed control without the OnDetached notification (the inlined
    // NotifierT::RemoveObserver loop), then the Object bases. Mac 0x163790
    virtual void OnFinalRelease()
    {
        for (UInt32 i = 0; i < m_subjects.GetSize(); ++i)
            static_cast<NotifierT<Control, ControlObserver>*>(m_subjects[i])->RemoveObserver(this);
        m_subjects.Clear();
        Object::OnFinalRelease();
    }

    // The observer half for derived classes (the base is private, as in the RTTI).
    ControlObserver* AsControlObserver() { return this; }
    void ObserveControl(Control* control) { ControlObserver::Attach(control); }
    void StopObservingControl(Control* control) { ControlObserver::Detach(control); }
};

} // namespace LEGO
