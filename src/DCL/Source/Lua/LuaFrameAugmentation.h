// LEGO Digital Designer — restored source
// LEGO::LuaFrameAugmentation — Lua class "FrameAugmentation" (base "ControlAugmentation"): a
// FrameAugmentation (keeps its control at a margin inside its parent), attached as "frame".
// self:SetFrame([left [, top [, right [, bottom]]]]) keeps missing values; layout attribute
// `frame="left top right bottom"` ("#" = keep).
// TU 409 "LuaFrameAugmentation.cpp": Mac 0x38dd00-0x38e8ab, Win 0x45b3f0-0x45b93f (also the
// inline FrameAugmentation ctor, Mac 0x38dd00).
// RTTI vmi {LuaControlAugmentation +0, FrameAugmentation +0xc}, virtual base Object.
// Mac primary vtable 0x756e5c {D1 0x38e220, D0 0x38e310, BindLuaObject 0x291a70,
// UnbindLuaObject 0x38e5e0, OnLayoutApplied 0x38e600, OnFinalRelease 0x38e410}; ctor Mac
// 0x38de00 (C1), Win 0x45b3f0; Win dtor 0x45b7a0, sdd 0x45b4b0, OnFinalRelease 0x45b830,
// OnLayoutApplied 0x45b840.
// Layout (Mac): LuaLayout +0x00..+0x0c, FrameAugmentation +0x0c..+0x48, Object +0x48
// (sizeof 0x50). Method names inferred.
#pragma once
#include "LEGO/Types.h"
#include "LuaControlAugmentation.h"
#include "FrameAugmentation.h"

namespace LEGO {

class LuaFrameAugmentation : public LuaControlAugmentation, public FrameAugmentation {
public:
    LuaFrameAugmentation() {}                                               // Mac 0x38de00, Win 0x45b3f0
    virtual ~LuaFrameAugmentation() {}                                      // Mac 0x38e220/0x38e310, Win 0x45b7a0 (sdd 0x45b4b0)

    // Detach(). Mac 0x38e5e0
    virtual void UnbindLuaObject();
    // LuaLayout's, then `frame` (FromStringOptional on the current frame) -> SetFrame.
    virtual void OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args); // Mac 0x38e600, Win 0x45b840

    static LuaFrameAugmentation* Check(LuaPlus::LuaState* state, int narg);  // Mac 0x38e760

protected:
    // FrameAugmentation's (ControlAugmentationControlObserver), then the LuaControlAugmentation
    // (Object) one. Mac 0x38e410 (thunks 0x38e480 … 0x38e560), Win 0x45b830
    virtual void OnFinalRelease();
};

// Registers FrameAugmentation (base ControlAugmentation) {SetFrame, Create}.
// Mac 0x38deb0, Win 0x45b6e0. (name inferred)
void RegisterLuaFrameAugmentation(LuaPlus::LuaState* state);

} // namespace LEGO
