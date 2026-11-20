// LEGO Digital Designer — restored source
// LEGO::LuaControlAugmentation — Lua class "ControlAugmentation" (base "Layout", no functions):
// base of the Lua twins of the control augmentations. A LuaLayout (created from <Augmentations>
// by LuaLayout::CreateChildren: `class=`, _G[class]:New(parent, name), Create(self, control))
// that is also the C++ augmentation (derives from it, or owns it: LuaTooltipAugmentation).
// Lua "Create" functions (Mac e.g. 0x2b7090): self an unbound table, bound to a new object;
// optional argument 2 a control (table bound to a LuaControl; nil = none; else "Control
// expected, got …"), to which the augmentation attaches under a fixed name ("drag", "popup",
// "saveRect", "buttonSound", "frame", "SlidingHideAugmentation", "tooltip"). UnbindLuaObject
// (Object:Destroy / GC) detaches it.
// TU 308 "LuaControlAugmentation.cpp": Mac 0x2b6b70-0x2b6e6b, Win 0x45dc60-0x45dd7f.
// RTTI si LuaLayout (virtual base Object). Mac vtable 0x74418c {D1 0x2b6c40, D0 0x2b6d00,
// BindLuaObject 0x291a70, UnbindLuaObject = pure, OnLayoutApplied 0x2cc720 (LuaLayout's)};
// ctor C2 0x2b6b70 (VTT), dtor body D2 0x2b6c20. Win vftable via 0x45dd20 (dtor body), sdd
// 0x45dc60. No own members: Mac layout = LuaLayout (vptr, state +4, bool +8). Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "LuaLayout.h"
#include "LuaPlus.h"

namespace LEGO {

class LuaControlAugmentation : public LuaLayout {
public:
    LuaControlAugmentation() {}                                             // Mac 0x2b6b70 (C2)
    virtual ~LuaControlAugmentation() {}                                    // Mac 0x2b6c40/0x2b6d00 (body 0x2b6c20), Win sdd 0x45dc60 (body 0x45dd20)
};

// Registers the Lua class ControlAugmentation (base Layout; empty function list).
// Mac 0x2b6ba0, Win 0x45dc90. (name inferred)
void RegisterLuaControlAugmentation(LuaPlus::LuaState* state);

} // namespace LEGO
