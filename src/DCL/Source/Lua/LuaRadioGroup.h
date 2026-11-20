// LEGO Digital Designer — restored source
// LEGO::LuaRadioGroup — Lua class "RadioGroup" (base Layout): ToggleControls of which one is
// toggled, from <Button control="lua.path" [show="lua.path"]/> children (the control shown while
// that toggle is current). A selection change calls self:OnChanged(index) (0-based).
// TU 388 "LuaRadioGroup.cpp": Mac 0x378fb0-0x37a62b, Win 0x457900-0x458c9f; also emits
// BasicArray<Pair<RefPointer<ToggleControl>, RefPointer<Control> > > (Mac 0x37a090 PushBack,
// 0x37a410 release, Win 0x4583a0, 0x458720, 0x458940).
//
// RTTI vmi {LuaLayout +0, ToggleControlObserver +0xc}, virtual base Object.
// Mac primary vtable 0x752dac {D1 0x3794a0, D0 0x3795c0, BindLuaObject 0x291a70 (LuaLayout's),
// UnbindLuaObject 0x379880, OnLayoutApplied 0x379920, Mac-only entries of the overriders
// OnFinalRelease 0x379720 and OnToggled 0x37a260}, ToggleControlObserver-in 0x752dd4 {…,
// OnToggled thunk 0x37a2a0, …}. Win 0xa21798 {0x40be80, UnbindLuaObject 0x457e10,
// OnLayoutApplied 0x457ea0}, ToggleControlObserver-in 0xa217a8 {…, OnToggled 0x458420, …},
// Object-in 0xa217bc {sdd 0x458de0 / 0x457a70, OnFinalRelease 0x458db0 (→ 0x457e00), …}.
// Layout Mac (ctor inlined in Create 0x379030): LuaLayout +0x00..+0x0c, ToggleControlObserver
// +0x0c (subjects +0x10), m_entries +0x20, m_current +0x30, m_updating +0x34, Object +0x38
// (sizeof 0x40). Names inferred unless they are Lua / layout strings.
#pragma once
#include "LEGO/Types.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "ToggleControl.h"
#include "LuaLayout.h"
#include "LuaPlus.h"

namespace LEGO {

class LuaRadioGroup : public LuaLayout, private ToggleControlObserver {
public:
    typedef Pair<RefPointer<ToggleControl>, RefPointer<Control> > Entry;

    LuaRadioGroup() : m_current(0), m_updating(false) {}                    // inlined in Create (Mac 0x379030)
    virtual ~LuaRadioGroup();                                               // Mac 0x3794a0/0x3795c0 (D2 0x3793b0), Win 0x457d60 / sdd 0x457a70

    UInt32 GetCurrent() const { return m_current; }
    UInt32 GetCount() const { return m_entries.GetSize(); }
    // Toggles entry `index` (the others off), shows its control (the others hidden), then
    // self:OnChanged(index). Nothing when it is the current one. NOTE: no range check.
    void SetCurrent(UInt32 index);                                          // Mac 0x3792b0, Win 0x458b30

protected:
    // Drops the entries. Mac 0x379880, Win 0x457e10
    virtual void UnbindLuaObject();
    // LuaLayout's, <OnChanged> ("index"), then the <Button> children (see the header comment).
    virtual void OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args); // Mac 0x379920, Win 0x457ea0
    // Leaves the toggles, then LuaLayout's.
    virtual void OnFinalRelease();                                          // Mac 0x379720 (thunks 0x379790, 0x379800), Win 0x457e00
    // ---- ToggleControlObserver: a toggled button becomes the current one ----
    virtual void OnToggled(ToggleControl* control);                         // Mac 0x37a260 (thunk 0x37a2a0), Win 0x458420

private:
    // The toggles and the shown controls as m_current says (m_updating set meanwhile).
    void ApplyCurrent();

    BasicArray<Entry> m_entries;   // Mac +0x20
    UInt32            m_current;   // Mac +0x30 0
    bool              m_updating;  // Mac +0x34 SetCurrent is toggling
};

// Registers RadioGroup (base Layout) {Create, SetCurrent, GetCurrent, GetCount}.
// Mac 0x378fb0, Win 0x457c90 (name inferred)
void RegisterLuaRadioGroup(LuaPlus::LuaState* state);

} // namespace LEGO
