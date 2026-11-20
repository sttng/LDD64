// LEGO Digital Designer — restored source
// LEGO::LuaTooltipAugmentation — Lua class "TooltipAugmentation" (base "ControlAugmentation"):
// owns a C++ TooltipAugmentation (made by LuaStandardTextTooltipAugmentation /
// LuaExtendedTextTooltipAugmentation) and attaches it to a control. Layout attribute `clipper`
// (Lua path of a control): the tooltip stays inside it (TooltipAugmentation::SetAnchor).
// TU 313 "LuaTooltipAugmentation.cpp": Mac 0x2bb030-0x2bb79b, Win 0x459b30-0x459fcf.
// RTTI si LuaControlAugmentation (abstract: UnbindLuaObject stays pure). Mac vtable 0x74413c
// {D1 0x2bb380, D0 0x2bb440, BindLuaObject 0x291a70, UnbindLuaObject = pure, OnLayoutApplied
// 0x2bb510, OnFinalRelease 0x2bb4c0} + Object-in 0x74416c; ctor C2 0x2bb030 (VTT), dtor body
// D2 0x2bb300. Win: dtor 0x459e30 (sdd 0x459b30), OnLayoutApplied 0x459ec0.
// Layout (Mac): LuaLayout +0x00..+0x0c, RefPointer<TooltipAugmentation> +0x0c, Object +0x10
// (sizeof 0x18; the derived classes put their own augmentation at +0x10). Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LuaControlAugmentation.h"
#include "TooltipManager.h"
#include "LuaPlus.h"

namespace LEGO {

class XMLDOMNode;
struct LuaLayoutArgs;

class LuaTooltipAugmentation : public LuaControlAugmentation {
public:
    LuaTooltipAugmentation() {}                                             // Mac 0x2bb030 (C2)
    virtual ~LuaTooltipAugmentation() {}                                    // Mac 0x2bb380/0x2bb440 (body 0x2bb300), Win 0x459e30 (sdd 0x459b30)

    // LuaLayout's (<OnLoad>), then `clipper`: the control of the Lua table at that path (from
    // the table being laid out; nil or not a control: none) becomes the tooltip's anchor.
    // NOTE: the original does not check m_tooltip (always set by Create).
    virtual void OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args); // Mac slot 4 0x2bb510, Win 0x459ec0

    TooltipAugmentation* GetTooltipAugmentation() const { return m_tooltip; }

    // Keeps `tooltip` (the previous one is released). Mac 0x2bb2b0 (Win inlined)
    void SetTooltipAugmentation(TooltipAugmentation* tooltip);
    // m_tooltip->AttachTo(control); false without a tooltip. Mac 0x2bb280 (Win inlined)
    bool AttachTooltip(Control* control);

    // Self of the Lua methods: argument `narg` bound to a LuaTooltipAugmentation, else a Lua type
    // error. Mac 0x2bb650 (Win inlined)
    static LuaTooltipAugmentation* Check(LuaPlus::LuaState* state, int narg);

protected:
    // Object::OnFinalRelease (the original overrides it without adding anything).
    virtual void OnFinalRelease();                                          // Mac slot 5 0x2bb4c0 (thunk 0x2bb4e0)

    RefPointer<TooltipAugmentation> m_tooltip;   // Mac +0x0c
};

// Registers TooltipAugmentation (base ControlAugmentation) {SetTooltip, RemoveTooltip,
// TooltipChanged}. Mac 0x2bb070, Win 0x459c00. (name inferred)
void RegisterLuaTooltipAugmentation(LuaPlus::LuaState* state);

} // namespace LEGO
