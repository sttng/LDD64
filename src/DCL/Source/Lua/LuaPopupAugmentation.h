// LEGO Digital Designer — restored source
// LEGO::LuaPopupAugmentation — Lua class "PopupAugmentation" (base "ControlAugmentation"),
// attached as "popup": the control is hidden on attach and, while shown, hides again when the
// mouse leaves it (unless a control below it holds the mouse), moves to an outside control, a
// modal control changes, or its `owner` (layout: Lua path of a control) hides.
// PopupChildAlign(self, reference, child) shows it on top, moved by `child`'s position in
// `reference`'s coordinates (then back inside its parent).
// TU 310 "LuaPopupAugmentation.cpp": Mac 0x2b8430-0x2b97bb, Win 0x45a5b0-0x45af4f; also emits
// CheckLuaControl (Mac 0x2b9670, LuaControl.h).
// RTTI vmi {LuaControlAugmentation +0, ControlAugmentationControlObserver +0xc,
// ControlManagerObserver +0x38 (non-public)}, virtual base Object. Mac primary vtable 0x74625c
// {D1 0x2b8ab0, D0 0x2b8bd0, BindLuaObject 0x291a70, UnbindLuaObject 0x2b8e40, OnLayoutApplied
// 0x2b8e50, Attach 0x2b8850, Detach 0x2b8990, OnFinalRelease 0x2b8d10, OnShown 0x2b8ff0,
// OnHidden 0x2b9290, OnManagerMouseMove 0x2b9370, OnManagerMouseOverChanged 0x2b9440,
// OnManagerModalControl 0x2b9540}; ctor Mac 0x2b8430 (C1), Win 0x45a5b0. Win: sdd 0x45a6a0,
// Attach 0x45aad0, Detach 0x45ab20, dtor 0x45ab80, OnFinalRelease 0x45ac30, OnLayoutApplied
// 0x45ac70, OnShown 0x45add0, OnHidden 0x45adf0, OnManagerMouseMove 0x45ae20,
// OnManagerMouseOverChanged 0x45aec0.
// Layout (Mac): LuaLayout +0x00..+0x0c, ControlAugmentationControlObserver +0x0c..+0x38,
// ControlManagerObserver +0x38 (manager +0x3c), Control* popup +0x40, Control* owner +0x44,
// Object +0x48 (sizeof 0x50). Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "LuaControlAugmentation.h"
#include "ControlAugmentationControlObserver.h"
#include "ControlManager.h"

namespace LEGO {

class LuaPopupAugmentation : public LuaControlAugmentation,
                             public ControlAugmentationControlObserver,
                             protected ControlManagerObserver {
public:
    LuaPopupAugmentation() : m_popup(0), m_owner(0) {}                      // Mac 0x2b8430, Win 0x45a5b0
    virtual ~LuaPopupAugmentation() {}                                      // Mac 0x2b8ab0/0x2b8bd0, Win 0x45ab80 (sdd 0x45a6a0)

    // Detach() (virtual: this class's). Mac 0x2b8e40
    virtual void UnbindLuaObject();
    // LuaLayout's, then `owner`: leave the previous owner; a non-empty path names the new one
    // (observed; the pop-up hides when it is not visible). Mac 0x2b8e50, Win 0x45ac70
    virtual void OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args);
    // ControlAugmentationControlObserver::Attach; then (whatever it returned) the pop-up is the
    // augmentation's control, hidden. Mac 0x2b8850 (thunk 0x2b88f0), Win 0x45aad0
    virtual bool Attach(Control* control, const String& name);
    // Leaves the manager, forgets the pop-up, then ControlAugmentationControlObserver::Detach.
    // Mac 0x2b8990 (thunk 0x2b8a20), Win 0x45ab20
    virtual void Detach();

    Control* GetPopup() const { return m_popup; }
    Control* GetOwner() const { return m_owner; }
    ControlManager* GetObservedManager() const { return ControlManagerObserver::GetSubject(); }

    static LuaPopupAugmentation* Check(LuaPlus::LuaState* state, int narg);  // Mac 0x2b95a0

protected:
    // ControlManagerObserver's (leaves the manager), ControlAugmentationControlObserver's, then
    // the Object one. Mac 0x2b8d10, Win 0x45ac30
    virtual void OnFinalRelease();

    // ---- ControlObserver ----
    // The pop-up was shown: observe its manager. Mac 0x2b8ff0 (thunk 0x2b9270), Win 0x45add0
    virtual void OnShown(Control* control);
    // The owner was hidden: hide the pop-up; the pop-up was hidden: leave the manager.
    // Mac 0x2b9290 (thunk 0x2b9300), Win 0x45adf0
    virtual void OnHidden(Control* control);

    // ---- ControlManagerObserver ----
    // Outside the pop-up and no control below it holds the mouse (captured / buttons down):
    // hide. Mac 0x2b9370 (thunk 0x2b9420), Win 0x45ae20
    virtual void OnManagerMouseMove(const Vector2& position);
    // The mouse is outside the pop-up and `current` is not below it: hide.
    // Mac 0x2b9440 (thunk 0x2b94c0), Win 0x45aec0
    virtual void OnManagerMouseOverChanged(Control* previous, Control* current);
    // Hide. Mac 0x2b9540 (thunk 0x2b9570)
    virtual void OnManagerModalControl(bool modal, Control* control);

private:
    Control* m_popup;   // Mac +0x40 the augmented control
    Control* m_owner;   // Mac +0x44
};

// Registers PopupAugmentation (base ControlAugmentation) {Create, PopupChildAlign}.
// Mac 0x2b8520, Win 0x45aa20. (name inferred)
void RegisterLuaPopupAugmentation(LuaPlus::LuaState* state);

} // namespace LEGO
