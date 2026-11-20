// LEGO Digital Designer — restored source (bodies: TooltipManager.cpp)
// LEGO::ControlAugmentation — a named add-on of one Control (tooltip, button sound, frame, drag /
// popup / sliding-hide …), kept by name in the control's map (Control +0x130).
// LEGO::ControlManagerAugmentation — the same for a ControlManager (TooltipManager; +0xb4).
// Home TU 049 "TooltipManager.cpp" (Mac 0x111cf0-0x11437b, Win 0x44b470-0x44b65f,
// 0x512ee0-0x5138f1), bodies there. Directory UI/ (no __FILE__ evidence).
// ControlManagerAugmentation's slots 3/4 are named OnManagerAttached/OnManagerDetached because
// TooltipManager overrides them apart from its ControlManagerObserver base's OnAttached/OnDetached.
//
// ControlAugmentation (RTTI vmi, virtual base Object):
//   Mac vtable 0x70af4c {GetControl 0x113da0, Attach 0x111f10, Detach 0x111fc0, D1 0x113db0,
//   D0 0x113e40}; Object-in 0x70af78. Layout vptr, Control* +0x04, String name +0x08,
//   Object +0x18 (ControlAugmentationControlObserver has its ControlObserver base at +0x18).
//   Win vtable 0x9cd540 {GetControl 0x52f4b0 (ICF), Attach 0x44b470, Detach 0x44b4d0},
//   Object-in 0x9cd52c {sdd 0x44b630, …}; vfptr, vbptr +4, Control* +8, String +0xc, Object +0x10.
//   NOTE: the destructor is declared after the three virtuals (D1/D0 are Mac slots 3/4).
// ControlManagerAugmentation (RTTI vmi, virtual base Object):
//   Mac vtable 0x70afac {D1 0x113ee0, D0 0x113f40, OnFinalRelease 0x112e10 (Mac-only primary
//   entry), OnAttached 0x113fc0 (empty), OnDetached 0x113fd0 (empty)}; Object-in 0x70afd8.
//   Layout vptr, ControlManager* +0x04, String name +0x08, Object +0x18.
//   Win vtable 0xa28568 {OnAttached 0x50fca7, OnDetached 0x50fca7 (ICF, ret 4)}, Object-in
//   0xa28574 {sdd 0x5139bd, OnFinalRelease 0x5139e0, …}.
// Method and member names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"

namespace LEGO {

class Control;
class ControlManager;

class ControlAugmentation : public virtual Object {
public:
    ControlAugmentation() : m_control(0) {}                                 // Win 0x44b520

    // The control this augmentation is attached to (null when detached).
    virtual Control* GetControl() const { return m_control; }               // Mac 0x113da0, Win 0x52f4b0
    // Attaches to `control` under `name` (Control::AddAugmentation, Mac 0xe76b0, Win 0x509b30;
    // false when the name is taken); detaches from the previous control first.
    virtual bool Attach(Control* control, const String& name);              // Mac 0x111f10, Win 0x44b470
    // Detaches from the control (Control::RemoveAugmentation Mac 0xe7790, Win 0x509bb1/0x50b668).
    virtual void Detach();                                                  // Mac 0x111fc0, Win 0x44b4d0
    virtual ~ControlAugmentation();                                         // Mac 0x113db0/0x113e40, Win sdd 0x44b630

    const String& GetName() const { return m_name; }

protected:
    Control* m_control;   // Mac +0x04, Win +0x08
    String   m_name;      // Mac +0x08, Win +0x0c
};

class ControlManagerAugmentation : public virtual Object {
public:
    ControlManagerAugmentation() : m_manager(0) {}
    // (name inferred; inlined in TooltipManager's constructor Mac 0x112500 / Win 0x513180)
    explicit ControlManagerAugmentation(const String& name) : m_manager(0), m_name(name) {}
    virtual ~ControlManagerAugmentation();                                  // Mac 0x113ee0/0x113f40, Win 0x512f86

    ControlManager* GetManager() const { return m_manager; }
    const String&   GetName() const { return m_name; }

    // Moves to `manager` (ControlManager::AddAugmentation, slot 36; false when the name is taken
    // there): leaves the previous manager first (OnManagerDetached), then OnManagerAttached.
    // NOTE: `manager` must not be null (both originals call through it). (name inferred)
    bool Attach(ControlManager* manager);                                   // Mac 0x112360, Win 0x512f12

protected:
    // Leaves the manager (ControlManager::RemoveAugmentation, slot 37), then OnManagerDetached.
    virtual void OnFinalRelease();                                          // Mac 0x112e10, Win 0x512fc9
    virtual void OnManagerAttached(ControlManager* /*manager*/) {}          // Mac 0x113fc0, Win 0x50fca7
    virtual void OnManagerDetached(ControlManager* /*manager*/) {}          // Mac 0x113fd0, Win 0x50fca7

    friend class ControlManager;

    ControlManager* m_manager;   // Mac +0x04
    String          m_name;      // Mac +0x08 (the key in ControlManager's map)
};

} // namespace LEGO
