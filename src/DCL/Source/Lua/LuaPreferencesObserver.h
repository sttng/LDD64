// LEGO Digital Designer — restored source
// LEGO::LuaPreferencesObserver — Lua class "PreferencesObserver" (base Object): forwards every
// preference change to the table's OnPreferenceChanged(key). Create(self), :Connect(),
// :Disconnect().
// TU 402 "LuaPreferencesObserver.cpp": Mac 0x38b270-0x38bacf, Win 0x46b310-0x46b6bf.
// RTTI vmi {PreferencesObserver +0, BindableLuaObject +8 (Win +0xc)}, virtual base Object.
// Mac vtables 0x7560bc {D1 0x38b6f0, D0 0x38b7a0, OnFinalRelease 0x45080, OnAttached 0x52080,
// OnDetached 0x52090, OnPreferenceChanged 0x38b8b0, UnbindLuaObject 0x38b930},
// BindableLuaObject-in 0x7560e4 {0x38b720, 0x38b7f0, 0x291a70, 0x38b970}.
// Win 0xa23540 {0x40be80, 0x40be80, OnPreferenceChanged 0x46b610}, 0xa23550 {0x40be80,
// UnbindLuaObject 0x46b690}, Object-in 0xa2355c {sdd 0x46bab0 …}.
// Layout Mac: PreferencesObserver {vptr, subject +4}, BindableLuaObject {vptr +8, state +0xc},
// Object +0x10 (sizeof 0x18); Win: Object +0x1c (sizeof 0x24).
#pragma once
#include "LEGO/Types.h"
#include "Preferences.h"
#include "BindableLuaObject.h"

namespace LEGO {

class LuaPreferencesObserver : public PreferencesObserver, public BindableLuaObject {
public:
    LuaPreferencesObserver() {}                                            // inlined (Mac 0x38b2f0, Win 0x46b240)
    virtual ~LuaPreferencesObserver() {}                                   // Mac 0x38b6f0/0x38b7a0, Win 0x46b580/0x46b310

    // Calls self:OnPreferenceChanged(key) (raw lookup in the instance table).
    virtual void OnPreferenceChanged(const String& key);                  // Mac 0x38b8b0, Win 0x46b610
    // The Lua table went away: stop observing.
    virtual void UnbindLuaObject();                                        // Mac 0x38b930, Win 0x46b690
};

// Registers the Lua class PreferencesObserver {Create, Connect, Disconnect}. (name inferred)
void RegisterLuaPreferencesObserver(LuaPlus::LuaState* state);            // Mac 0x38b270, Win 0x46b4c0

} // namespace LEGO
