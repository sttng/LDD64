// LEGO Digital Designer — restored source
// LEGO::LuaLDDApplicationObserver — Lua class "LDDApplicationObserver" (base Object): an
//   LDDApplicationObserver bound to a Lua table. Lua: LDDApplicationObserver.Create(self),
//   self:Connect() (observes LDDApplication::GetInstance()), self:Disconnect(); callbacks
//   self:OnRecentModelsChanged(path), self:OnBrandChanged(brandName),
//   self:OnExtendedTooltipChanged(enabled), self:OnOnlineStatusChanged(online).
//
// TU 398 "LuaLDDApplicationObserver.cpp": Mac 0x3830b0-0x383c4f, Win 0x64aab5-0x64ae5f
// (+ 0x46b730 CallLuaMethod<String>, 0x62c020).
// RTTI vmi {LDDApplicationObserver +0, BindableLuaObject +8 (Win +0xc)}, virtual base Object.
// Mac vtables 0x75534c {D1 0x3834f0, D0 0x3835a0, OnFinalRelease 0x14f7b0, OnAttached 0x14f800,
// OnDetached 0x14f810, OnRecentModelsChanged 0x3836b0, OnBrandChanged 0x3837e0, 0x14f840,
// OnExtendedTooltipChanged 0x383890, OnOnlineStatusChanged 0x383920, 0x14f870,
// UnbindLuaObject 0x3839b0}, BindableLuaObject-in 0x755388 {0x383520, 0x3835f0, 0x291a70,
// 0x3839f0}. Win 0x9d4fc4 {…, 0x64accb, 0x64ad15, 0x65a12a, 0x64ad85, 0x64ada9, 0x5fbd39},
// 0x9d4fb8 {0x40be80, 0x64ae38}, Object-in 0x9d4fa4 {sdd 0x64acac?}.
// Layout Mac: observer {vptr, subject +4}, BindableLuaObject {vptr +8, state +0xc}, Object
// +0x10 (sizeof 0x18). Function names are inferred (Lua names are the strings).
#pragma once
#include "LEGO/Types.h"
#include "BindableLuaObject.h"
#include "LDDApplication.h"

namespace LEGO {

class LuaLDDApplicationObserver : public LDDApplicationObserver, public BindableLuaObject {
public:
    LuaLDDApplicationObserver() {}                                         // inlined (Mac 0x383130; Win 0x64aacc)
    virtual ~LuaLDDApplicationObserver() {}                                // Mac 0x3834f0/0x3835a0, Win 0x64abcf / sdd 0x64acac

    // self:OnRecentModelsChanged(path.GetPath(true))
    virtual void OnRecentModelsChanged(const Path& path);                  // Mac 0x3836b0, Win 0x64accb
    // self:OnBrandChanged(ToString(brand))
    virtual void OnBrandChanged(Brand brand);                              // Mac 0x3837e0, Win 0x64ad15
    // self:OnExtendedTooltipChanged(enabled)
    virtual void OnExtendedTooltipChanged(bool enabled);                   // Mac 0x383890, Win 0x64ad85
    // self:OnOnlineStatusChanged(online)
    virtual void OnOnlineStatusChanged(bool online);                       // Mac 0x383920, Win 0x64ada9
    // The Lua table went away: stop observing.
    virtual void UnbindLuaObject();                                        // Mac 0x3839b0 (thunk 0x3839f0), Win 0x62c020 (via 0x64ae38)
};

// Registers the Lua class LDDApplicationObserver {Create, Connect, Disconnect}.
void RegisterLuaLDDApplicationObserver(LuaPlus::LuaState* state);          // Mac 0x3830b0, Win 0x64ae68

} // namespace LEGO
