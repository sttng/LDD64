// LEGO Digital Designer — restored source
// LEGO::LuaExtendedTextTooltipAugmentation — Lua class "ExtendedTextTooltipAugmentation" (base
//   "TooltipAugmentation"): an ExtendedTextTooltipAugmentation. Create(self [, control]),
//   self:SetText(text) (both texts = text), self:SetLocText(key) (translations of `key` and of
//   `key .. "Extended"`). Layout attributes `text`, `locText` (+ `clipper`). Used by every
//   tooltip of the LDD layouts (<tooltipAugmentation locText="…" class="ExtendedText…"/>).
//
// TU 297 "LuaExtendedTextTooltipAugmentation.cpp": Mac 0x2a6030-0x2a6a4b, Win 0x64c9d9-0x64cdff.
// RTTI si LuaTooltipAugmentation. Mac vtable 0x74409c {D1 0x2a65b0, D0 0x2a6670, BindLuaObject
// 0x291a70, UnbindLuaObject 0x2a6740, OnLayoutApplied 0x2a6790, OnFinalRelease 0x2a66f0} +
// Object-in 0x7440cc. Win: sdd 0x64cbae, Unbind 0x64ca87, OnLayoutApplied 0x64caa0.
// Layout (Mac): LuaTooltipAugmentation +0x00..+0x10, RefPointer<ExtendedText…> +0x10, Object
// +0x14 (sizeof 0x1c). Method and member names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "LuaTooltipAugmentation.h"
#include "ExtendedTextTooltipAugmentation.h"

namespace LEGO {

class LuaExtendedTextTooltipAugmentation : public LuaTooltipAugmentation {
public:
    LuaExtendedTextTooltipAugmentation() {}                                 // (inlined in Create, Mac 0x2a60b0)
    virtual ~LuaExtendedTextTooltipAugmentation() {}                        // Mac 0x2a65b0/0x2a6670, Win sdd 0x64cbae

    // Releases both augmentations. Mac slot 3 0x2a6740, Win 0x64ca87
    virtual void UnbindLuaObject();
    // LuaTooltipAugmentation's, then `text` (both texts), `locText` (SetLocText).
    virtual void OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args); // Mac 0x2a6790, Win 0x64caa0

    ExtendedTextTooltipAugmentation* GetTextTooltip() const { return m_textTooltip; }

    // Standard text = translation of `key`, extended text = translation of key + "Extended".
    // Mac 0x2a6490, Win 0x64c9d9
    void SetLocText(const String& key);

    static LuaExtendedTextTooltipAugmentation* Check(LuaPlus::LuaState* state, int narg); // Mac 0x2a6900

protected:
    virtual void OnFinalRelease();                                          // Mac 0x2a66f0 (thunk 0x2a6710)

private:
    friend int LuaExtendedTextTooltipAugmentationCreate(lua_State* L);

    RefPointer<ExtendedTextTooltipAugmentation> m_textTooltip;   // Mac +0x10
};

// ExtendedTextTooltipAugmentation.Create(self [, control]). Mac 0x2a60b0
int LuaExtendedTextTooltipAugmentationCreate(lua_State* L);

// Registers the Lua class ExtendedTextTooltipAugmentation (base TooltipAugmentation) {Create,
// SetText, SetLocText}. Mac 0x2a6030, Win 0x64cd97. (name inferred)
void RegisterLuaExtendedTextTooltipAugmentation(LuaPlus::LuaState* state);

} // namespace LEGO
