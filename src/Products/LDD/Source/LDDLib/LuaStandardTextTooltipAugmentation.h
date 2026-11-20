// LEGO Digital Designer — restored source
// LEGO::LuaStandardTextTooltipAugmentation — Lua class "StandardTextTooltipAugmentation" (base
//   "TooltipAugmentation"): a StandardTextTooltipAugmentation (fixed text in the shared 'STTC'
//   tooltip control). StandardTextTooltipAugmentation.Create(self [, control]),
//   self:SetText(text), self:SetLocText(key). Layout attributes `text`, `locText` (then `clipper`
//   of LuaTooltipAugmentation).
//
// TU 312 "LuaStandardTextTooltipAugmentation.cpp": Mac 0x2ba740-0x2bb02b, Win 0x459260-0x45990f
// (Win in the DCL range like StandardTextTooltipAugmentation itself; kept next to it here).
// RTTI si LuaTooltipAugmentation. Mac vtable 0x746d8c {D1 0x2baba0, D0 0x2bac60, BindLuaObject
// 0x291a70, UnbindLuaObject 0x2bad30, OnLayoutApplied 0x2bad80, OnFinalRelease 0x2bace0} +
// Object-in 0x746dbc. Win: dtor 0x459700 (sdd 0x459260), Unbind 0x4597a0, OnLayoutApplied
// 0x4597f0. Layout (Mac): LuaTooltipAugmentation +0x00..+0x10, RefPointer<Standard…> +0x10,
// Object +0x14 (sizeof 0x1c, operator new 0x1c in Create). Method/member names inferred.
#pragma once
#include "LEGO/Types.h"
#include "LuaTooltipAugmentation.h"
#include "StandardTextTooltipAugmentation.h"

namespace LEGO {

class LuaStandardTextTooltipAugmentation : public LuaTooltipAugmentation {
public:
    LuaStandardTextTooltipAugmentation() {}                                 // (inlined in Create, Mac 0x2ba7c0)
    virtual ~LuaStandardTextTooltipAugmentation() {}                        // Mac 0x2baba0/0x2bac60, Win 0x459700 (sdd 0x459260)

    // Releases both augmentations (the text one first). Mac slot 3 0x2bad30, Win 0x4597a0
    virtual void UnbindLuaObject();
    // LuaTooltipAugmentation's, then `text` -> SetText(text), `locText` -> SetText(translation).
    virtual void OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args); // Mac 0x2bad80, Win 0x4597f0

    StandardTextTooltipAugmentation* GetTextTooltip() const { return m_textTooltip; }

    static LuaStandardTextTooltipAugmentation* Check(LuaPlus::LuaState* state, int narg); // Mac 0x2baee0

protected:
    virtual void OnFinalRelease();                                          // Mac 0x2bace0 (thunk 0x2bad00)

private:
    friend int LuaStandardTextTooltipAugmentationCreate(lua_State* L);

    RefPointer<StandardTextTooltipAugmentation> m_textTooltip;   // Mac +0x10
};

// StandardTextTooltipAugmentation.Create(self [, control]). Mac 0x2ba7c0 (Win inlined/folded)
int LuaStandardTextTooltipAugmentationCreate(lua_State* L);

// Registers the Lua class StandardTextTooltipAugmentation (base TooltipAugmentation) {Create,
// SetText, SetLocText}. Mac 0x2ba740, Win 0x459530. (name inferred)
void RegisterLuaStandardTextTooltipAugmentation(LuaPlus::LuaState* state);

} // namespace LEGO
