// LEGO Digital Designer — restored source
// LEGO::LuaButtonSoundAugmentation — Lua class "ButtonSoundAugmentation" (base
// "ControlAugmentation"): plays the application's click sound; Create(self [, control]) attaches
// it as "buttonSound". Without a sound, Create / Layout / Connect / Disconnect do nothing
// (LuaReturnNothing), so layouts with <clickSoundAugmentation> still load.
// TU 307 "LuaButtonSoundAugmentation.cpp": Mac 0x2b5ed0-0x2b6b6b, Win 0x45bf70-0x45c35f (also
// the inline ButtonSoundAugmentation ctor, Mac 0x2b5ed0).
// RTTI vmi {LuaControlAugmentation +0, ButtonSoundAugmentation +0xc}, virtual base Object.
// Mac primary vtable 0x74559c {D1 0x2b6780, D0 0x2b6890, BindLuaObject 0x291a70,
// UnbindLuaObject 0x2b6ad0, OnLayoutApplied 0x2cc720, OnFinalRelease 0x2b69d0}, secondary
// vtables 0x7455c0 (ButtonSoundAugmentation-in), 0x7455ec (ControlObserver), 0x745680
// (PreferencesObserver), Object-in 0x7456b0. Ctor Mac 0x2b6380 (C1), Win 0x45bf70; Win sdd
// 0x45c050, dtor 0x45c2d0.
// Layout (Mac): LuaLayout +0x00..+0x0c, ButtonSoundAugmentation +0x0c..+0x48, Object +0x48
// (sizeof 0x50). Method names inferred.
#pragma once
#include "LEGO/Types.h"
#include "LuaControlAugmentation.h"
#include "ButtonSoundAugmentation.h"

namespace LEGO {

class SimpleSound;

class LuaButtonSoundAugmentation : public LuaControlAugmentation, public ButtonSoundAugmentation {
public:
    explicit LuaButtonSoundAugmentation(SimpleSound* sound)                  // Mac 0x2b6380, Win 0x45bf70
        : ButtonSoundAugmentation(sound) {}
    virtual ~LuaButtonSoundAugmentation() {}                                // Mac 0x2b6780/0x2b6890, Win 0x45c2d0 (sdd 0x45c050)

    // Detach() (the augmentation leaves its control). Mac 0x2b6ad0
    virtual void UnbindLuaObject();

protected:
    // ButtonSoundAugmentation's, then the LuaControlAugmentation (Object) one.
    // Mac 0x2b69d0 (thunks 0x2b6a00 … 0x2b6a90)
    virtual void OnFinalRelease();
};

// Registers ButtonSoundAugmentation (base ControlAugmentation) with Create (upvalue `sound`).
// Mac 0x2b6440, Win 0x45c1c0. (name inferred)
void RegisterLuaButtonSoundAugmentation(LuaPlus::LuaState* state, SimpleSound* sound);

} // namespace LEGO
