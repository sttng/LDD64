// LEGO Digital Designer — restored source
// LEGO::LuaControl — the Lua twin of a Control (Lua class "Control", base Layout): a LuaLayout
// attached to its control as ControlAugmentation "lua"; forwards the ControlObserver events and
// optionally an Updater's OnUpdate to the table. Base of every Lua*Control binding.
// TU 315 "LuaControl.cpp": Mac 0x2bda30-0x2c47b8, Win 0x44b680-0x44f1ff; also owns the
// cursor-name table (RegisterCursorShapeName, declared in LDDCursor.h too).
//
// Layout attributes (OnLayoutApplied: the slots below, then LuaLayout's <OnLoad>, <Children>):
//   ApplyControlAttributes: parent (Lua path), position, minSize, maxSize, size, <Anchors>
//     (<hAnchor>/<vAnchor>: point, offset, dependent), hit, selectionMask…, visible, enable,
//     cursor;
//   ApplyColorAttributes (BufferedAbleRenderControl): alpha, blendColor;
//   ApplyRenderAttributes (RenderControl): transparent, image/subRect/subPosSize,
//     drawImageAsFrame, font, text, locText, hAlignment, vAlignment, hAlignmentOffset,
//     vAlignmentOffset, resizeToFitText, resizeHeightToFitText;
//   DefineCallbacks: <OnRectangleChanged> … <OnChar>, <OnUpdate>; CreateAugmentations:
//     <Augmentations>.
//
// RTTI vmi {LuaLayout +0 public, ControlAugmentation +0xc, ControlObserver +0x24, UpdateObserver
// +0x38 non-public (protected here: the derived Lua controls use them)}, virtual base Object.
// Mac primary vtable 0x73f25c (24 entries; Itanium gives the overriders of the secondary bases'
// virtuals primary slots 12-23): D1 0x2bf480, D0 0x2bf660, BindLuaObject 0x2c1380,
// UnbindLuaObject 0x2c13b0, OnLayoutApplied 0x2bf9a0, Initialize 0x2bdf40, OnFinalRelease
// 0x2bf8a0, ApplyControlAttributes 0x2bfa90, ApplyColorAttributes 0x2c0750,
// ApplyRenderAttributes 0x2c08e0, DefineCallbacks 0x2c11a0, CreateAugmentations 0x2c1300,
// Attach 0x2c13f0, Detach 0x2c14f0, OnRectangleChanged 0x2c1610, OnShown 0x2c1680, OnHidden
// 0x2c16f0, OnEnabled 0x2c1760, OnDisabled 0x2c17d0, OnMousePressed 0x2c1840, OnMouseReleased
// 0x2c19c0, OnMouseClicked 0x2c1b40, OnChar 0x2c1cc0, Update 0x2c1ec0.
// Win vtables 0xa210e0 {Bind 0x44e240, Unbind 0x44e260, OnLayoutApplied 0x44cd00, Initialize
// 0x44bb90, ApplyControlAttributes 0x44d270, ApplyColorAttributes 0x44d980,
// ApplyRenderAttributes 0x44da90, DefineCallbacks 0x44e0d0, CreateAugmentations 0x44e1d0},
// ControlAugmentation-in 0xa21108 {GetControl 0x52f4b0, Attach 0x44e290, Detach 0x44e2e0},
// ControlObserver-in 0xa2111c, UpdateObserver-in 0xa2119c {…, Update 0x44e560}, Object-in
// 0xa211ac {sdd 0x4520b0, OnFinalRelease 0x452090, …}.
// Layout Mac: LuaLayout +0x00..+0x0c, ControlAugmentation +0x0c (control +0x10, name +0x14),
//   ControlObserver +0x24 (subjects +0x28), UpdateObserver +0x38 (subject +0x3c), members below,
//   Object +0x4c (sizeof 0x54, operator new 0x54 in 0x13f790 / Control.Create).
// Win: LuaLayout +0x00..+0x10, ControlAugmentation +0x10, ControlObserver +0x20, UpdateObserver
//   +0x38, m_control +0x44, m_bufferedControl +0x48, m_renderControl +0x4c, vtordisp +0x50,
//   Object +0x54.
// Method and member names are inferred unless they are Lua strings.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "StringStream.h"
#include "Geometry.h"
#include "Updater.h"
#include "Control.h"
#include "ControlAugmentation.h"
#include "LuaLayout.h"
#include "LuaBindings.h"
#include "LuaPlus.h"

namespace LEGO {

class XMLDOMNode;
class ControlManager;
class BufferedAbleRenderControl;
class RenderControl;

class LuaControl : public LuaLayout,
                   protected ControlAugmentation,
                   protected ControlObserver,
                   protected UpdateObserver {
public:
    LuaControl();                                                           // Mac 0x2bdb50 (C1), Win 0x44b680
    virtual ~LuaControl();                                                  // Mac 0x2bf480/0x2bf660, Win 0x44cc00 (sdd 0x44b7b0)

    // ---- BindableLuaObject / LuaLayout (Mac slot / Win slot) ----
    // Keeps the control alive while the Lua table exists. Mac 2 0x2c1380, Win 0 0x44e240
    virtual void BindLuaObject(LuaPlus::LuaObject& table);
    // Releases the control, then Detach(). Mac 3 0x2c13b0, Win 1 0x44e260
    virtual void UnbindLuaObject();
    // The Apply* slots, DefineCallbacks, CreateAugmentations, LuaLayout's <OnLoad>, then the
    // <Children>. Mac 4 0x2bf9a0, Win 2 0x44cd00
    virtual void OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args);

    // ---- new virtuals ----
    // Binds this object to `control` (augmentation "lua") and the table `self`, then Control::Setup
    // with the controls of the Lua tables `parent` / `after` (nil or unbound: null). False when the
    // control refused the augmentation. Mac 5 0x2bdf40, Win 3 0x44bb90
    virtual bool Initialize(LuaPlus::LuaObject& self, LuaPlus::LuaObject& parent,
                            LuaPlus::LuaObject& after, Control* control, UInt32 flags,
                            const Rect& rectangle, UInt32 inputFlags, bool hitTestable);

protected:
    // Leaves the updater and the observed controls (qualified base OnFinalReleases).
    // Mac 6 0x2bf8a0 (thunks 0x2bf940/0x2bf960/0x2bf980), Win Object-in 0x44ccc0 / 0x452090
    virtual void OnFinalRelease();

    // The layout attributes of every control (see the header comment). Mac 7 0x2bfa90, Win 4 0x44d270
    virtual void ApplyControlAttributes(const XMLDOMNode* layout, LuaLayoutArgs& args);
    // "alpha", "blendColor" (BufferedAbleRenderControl). Mac 8 0x2c0750, Win 5 0x44d980
    virtual void ApplyColorAttributes(const XMLDOMNode* layout, LuaLayoutArgs& args);
    // RenderControl attributes (image, frame, font, text, alignment, fit). Mac 9 0x2c08e0, Win 6 0x44da90
    virtual void ApplyRenderAttributes(const XMLDOMNode* layout, LuaLayoutArgs& args);
    // Defines the callback methods from the layout's <OnRectangleChanged> … <OnUpdate> children
    // (<OnUpdate> also observes the manager's Updater). Mac 10 0x2c11a0, Win 7 0x44e0d0
    virtual void DefineCallbacks(const XMLDOMNode* layout, LuaLayoutArgs& args);
    // CreateChildren("Augmentations"). Mac 11 0x2c1300, Win 8 0x44e1d0
    virtual void CreateAugmentations(const XMLDOMNode* layout, LuaLayoutArgs& args);

    // ---- ControlAugmentation ----
    // Attaches as augmentation `name` (unless already attached to `control`), then remembers the
    // control (and its BufferedAbleRenderControl / RenderControl views) and observes it.
    // Mac 12 0x2c13f0 (thunk 0x2c14d0), Win 0x44e290
    virtual bool Attach(Control* control, const String& name);
    // Stops observing the control and the updater, then leaves the control's augmentations.
    // Mac 13 0x2c14f0 (thunk 0x2c1580), Win 0x44e2e0
    virtual void Detach();

    // ---- ControlObserver: self:OnX(…) when `control` is this object's control ----
    virtual void OnRectangleChanged(Control* control, const Vector2& oldPosition,
                                    const Vector2& oldSize);                // Mac 14 0x2c1610 (thunk 0x2c1640), Win 0x44e330
    virtual void OnShown(Control* control);                                 // Mac 15 0x2c1680 (0x2c16b0), Win 0x44e360
    virtual void OnHidden(Control* control);                                // Mac 16 0x2c16f0 (0x2c1720), Win 0x44e390
    virtual void OnEnabled(Control* control);                               // Mac 17 0x2c1760 (0x2c1790), Win 0x44e3c0
    virtual void OnDisabled(Control* control);                              // Mac 18 0x2c17d0 (0x2c1800), Win 0x44e3f0
    virtual void OnMousePressed(Control* control, const Vector2& position, UInt32 button);  // Mac 19 0x2c1840 (0x2c1900), Win 0x44e420
    virtual void OnMouseReleased(Control* control, const Vector2& position, UInt32 button); // Mac 20 0x2c19c0 (0x2c1a80), Win 0x44e460
    virtual void OnMouseClicked(Control* control, const Vector2& position, UInt32 button);  // Mac 21 0x2c1b40 (0x2c1c00), Win 0x44e4a0
    // self:OnChar(string of the character, modifiers, repeated).
    virtual void OnChar(Control* control, UniChar16 c, UInt32 modifiers, bool repeated);   // Mac 22 0x2c1cc0 (0x2c1e20), Win 0x44e4e0

    // ---- UpdateObserver: self:OnUpdate() ----
    virtual void Update();                                                  // Mac 23 0x2c1ec0 (thunk 0x2c1ef0), Win 0x44e560

public:
    // ---- non-virtual ----
    // Attach(control, "lua") and AttachLuaObject(table, this). Mac 0x2bdeb0, Win 0x44bae0
    bool Bind(Control* control, LuaPlus::LuaObject& table);

    Control*                   GetTargetControl() const { return m_control; }
    BufferedAbleRenderControl* GetBufferedControl() const { return m_bufferedControl; }
    RenderControl*             GetRenderControl() const { return m_renderControl; }

    // The LuaControl attached to `control` (its augmentation "lua"); null without one.
    // Mac 0x2bdc50, Win 0x44b7e0
    static RefPointer<LuaControl> FromControl(Control* control);
    // The Lua table of `control`'s LuaControl; nil without one. A LuaControl whose table is gone
    // is unbound (UnbindLuaObject). Mac 0x2bdd30, Win 0x44b8d0
    static LuaPlus::LuaObject GetControlTable(LuaPlus::LuaState* state, Control* control);
    // A new instance of the Lua class "Control" bound to `control` through a new LuaControl
    // (CreateLuaControlTable<LuaControl>). Mac 0x2bde10, Win 0x44ba40
    static LuaPlus::LuaObject CreateControlTable(LuaPlus::LuaState* state, Control* control);

    // Self of the Lua methods: argument `narg` bound to a LuaControl (Lua type error otherwise;
    // the error names "N4LEGO10LuaControlE"). Mac 0x2be2e0 (+ inlined copies, e.g. 0x2be090)
    static LuaControl* Check(LuaPlus::LuaState* state, int narg);

private:
    friend class LuaControlPrivate;

    Control*                   m_control;          // Mac +0x40, Win +0x44 (referenced while bound)
    BufferedAbleRenderControl* m_bufferedControl;  // Mac +0x44, Win +0x48 (dynamic_cast of m_control)
    RenderControl*             m_renderControl;    // Mac +0x48, Win +0x4c (dynamic_cast of m_control)
};

// A new instance of Lua class `className` bound to `control` through a new T (`object` receives
// it; null, and a nil result, when the control refused it).
// Header template: Mac 0x13f790 (<LuaControl>, emitted in TU 060), Win 0x450c90.
template <class T>
LuaPlus::LuaObject CreateLuaControlTable(Control* control, LuaPlus::LuaState* state,
                                         const char* className, RefPointer<T>& object)
{
    LuaPlus::LuaObject table = CreateLuaInstance(state, className);
    object = new T;
    if (!object->Bind(control, table)) {
        object = 0;
        return LuaPlus::LuaObject(state);
    }
    return table;
}

// ---- argument helpers of the control bindings (header inlines in the original, emitted in the
// first TU using them) ----

// The Control bound to argument `narg`, or a Lua type error naming Control.
// Mac 0x2b9670 (emitted in TU 310), Win inlined
Control* CheckLuaControl(LuaPlus::LuaState* state, int narg);
// The same, but `def` for a nil argument. Mac 0x2c20c0
Control* OptLuaControl(LuaPlus::LuaState* state, int narg, Control* def);
// The RenderControl bound to argument `narg`, or a Lua type error naming RenderControl.
// Mac 0x2c21b0
RenderControl* CheckLuaRenderControl(LuaPlus::LuaState* state, int narg);

// ---- cursor shape names (none, pointer, clickable … busy = 0 … 17; ids ≥ 100 registered by
// LDDCursor::RegisterCursorNames) ----

// Name -> shape (case-insensitive) and shape -> name. Mac 0x2c1fb0, Win 0x44ec40
// (declared in LDDCursor.h too)
void RegisterCursorShapeName(UInt32 shape, const AString& name);
// A registered shape name at `position` (followed by ',', whitespace or the end).
// Mac 0x2c2010, Win inlined
bool ParseCursorShape(UInt32& shape, const String& text, UInt32& position);
// "#" (keep) or ParseCursorShape. Mac 0x2bd550 (emitted in TU 314), Win 0x467a60
inline bool ParseCursorShapeOptional(UInt32& shape, const String& text, UInt32& position)
{
    if (position >= text.GetLength())
        return false;
    if (ParseKeyword(String("#"), text, position, ','))
        return true;
    return ParseCursorShape(shape, text, position);
}

// Registers Control (base Layout) {SetParent … FitToChildren, Create (upvalue `manager`, the
// ControlManager new controls are created for)}. Mac 0x2be3b0, Win 0x44c9b0. (name inferred)
void RegisterLuaControl(LuaPlus::LuaState* state, ControlManager* manager);

} // namespace LEGO
