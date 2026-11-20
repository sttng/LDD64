// LEGO Digital Designer — restored source
// LEGO::LuaSlidingHideAugmentation — Lua class "SlidingHideAugmentation" (base
// "ControlAugmentation", only Create): attached as "SlidingHideAugmentation", the name
// LDDPaneContainerControl and AnimateShowHideControlCommand look up.
// TU 389 "LuaSlidingHideAugmentation.cpp": Mac 0x37a630-0x37ad6b, Win 0x458df0-0x45915f.
// RTTI vmi {LuaControlAugmentation +0, SlidingHideAugmentation +0xc}, virtual base Object.
// Mac primary vtable 0x752ffc {D1 0x37a980, D0 0x37aa90, BindLuaObject 0x291a70,
// UnbindLuaObject 0x37acd0, OnLayoutApplied 0x2cc720, OnFinalRelease 0x37abd0}; ctor Mac
// 0x37a630 (C1), Win 0x458df0; Win dtor 0x4590d0, sdd 0x458ec0.
// Layout (Mac): LuaLayout +0x00..+0x0c, SlidingHideAugmentation +0x0c..+0x70, Object +0x70
// (sizeof 0x78). Method names inferred.
#pragma once
#include "LEGO/Types.h"
#include "LuaControlAugmentation.h"
#include "SlidingHideAugmentation.h"

namespace LEGO {

class LuaSlidingHideAugmentation : public LuaControlAugmentation, public SlidingHideAugmentation {
public:
    LuaSlidingHideAugmentation() {}                                         // Mac 0x37a630, Win 0x458df0
    virtual ~LuaSlidingHideAugmentation() {}                                // Mac 0x37a980/0x37aa90, Win 0x4590d0 (sdd 0x458ec0)

    // Detach(). Mac 0x37acd0
    virtual void UnbindLuaObject();

protected:
    // SlidingHideAugmentation's, then the LuaControlAugmentation (Object) one.
    // Mac 0x37abd0 (thunks 0x37ac00 … 0x37ac90)
    virtual void OnFinalRelease();
};

// Registers SlidingHideAugmentation (base ControlAugmentation) {Create}.
// Mac 0x37a6e0, Win 0x459020. (name inferred)
void RegisterLuaSlidingHideAugmentation(LuaPlus::LuaState* state);

} // namespace LEGO
