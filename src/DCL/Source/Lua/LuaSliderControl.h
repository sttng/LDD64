// LEGO Digital Designer — restored source
// LEGO::LuaSliderControl — Lua class "SliderControl" (base Control): the bound ButtonControl is
// the knob, its parent the track. The value lies in [min, max] on a `step` grid from min; the
// knob sits at (value - min) / (max - min) of the free track length (axis y by default; reversed:
// from the far end); proportional knobs get visible / total of the track. Dragging the knob
// moves the value; a track press (if enabled) calls self:OnParentPressed(inc, pressedFactor),
// inc / dec buttons self:OnButton(inc) (each: else a tenth of the range). Every change saves
// preference `preferenceName` (when set) and calls self:OnValueChanged(value).
// TU 387 "LuaSliderControl.cpp": Mac 0x375db0-0x378fab, Win 0x45f210-0x46139d.
//
// RTTI si LuaButtonControl, virtual base Object.
// Mac primary vtable 0x75242c (33 entries: LuaButtonControl's 26 with D1 0x377460, D0 0x3776f0,
// OnLayoutApplied 0x3779e0, DefineCallbacks 0x3779f0, Attach 0x377a70, Detach 0x377aa0,
// OnRectangleChanged 0x377d30, OnMousePressed 0x377f20, OnMouseReleased 0x378110,
// OnMouseClicked 0x377de0; Mac-only entries 26/27 of the ControlObserver overriders OnDestroyed
// 0x377bf0, OnMouseMove 0x377e90; new virtuals 28-32 below), ControlObserver-in 0x7524dc.
// Win 0xa2229c {…, OnLayoutApplied 0x460120, …, DefineCallbacks 0x460130, 0x44e1d0, new 9-13:
// 0x4606e0 (OnParentPressed caller form; body 0x460ea0), OnButton 0x460740, 0x4607a0
// (OnValueChanged caller form; body 0x4611d0), UpdateKnob 0x460850, DragTo 0x460a60},
// ControlAugmentation-in 0xa222d8 {…, Attach 0x460190, Detach 0x4601b0}, ControlObserver-in
// 0xa222ec {…, OnDestroyed 0x460250, OnRectangleChanged 0x4602c0, OnMouseMove 0x460360,
// OnMousePressed 0x460390, OnMouseReleased 0x4604d0, OnMouseClicked 0x460310, …},
// Object-in 0xa22390 {sdd 0x4613c0 / 0x45f210, …}; ctor 0x460030.
// Layout Mac (ctor 0x375db0): LuaButtonControl +0x00..+0x58, members below, Object +0xbc
// (sizeof 0xc4). Win: m_track +0x98 (ControlObserver-relative +0x78), m_updating +0xa0,
// m_respondToParent +0xac. Names inferred unless they are Lua / layout strings.
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "Geometry.h"
#include "LuaButtonControl.h"

namespace LEGO {

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
class LuaSliderControl : public LuaButtonControl {
public:
    LuaSliderControl();                                                     // Mac 0x375db0, Win 0x460030
    virtual ~LuaSliderControl();                                            // Mac 0x377460/0x3776f0 (D2 0x377350), Win sdd 0x45f210

    // ---- Lua state (the Lua functions) ----
    Real64 GetMin() const { return m_min; }
    Real64 GetMax() const { return m_max; }
    Real64 GetStep() const { return m_step; }
    Real64 GetValue() const { return m_value; }
    Control* GetTrack() const { return m_track; }
    // New range; the value keeps its place (or its fraction, m_keepFraction), snapped.
    // Mac 0x376f20 (name inferred)
    void SetRange(Real64 min, Real64 max, Real64 step);
    // `value` snapped to the step grid from min and clamped to [min, max]; on a change UpdateKnob,
    // OnValueChanged. (inline in every caller on both builds; name inferred)
    void SetValue(Real64 value);
    // The inc (`increase`) / dec button: observed, mouse input, enabled while the value is not
    // `limit` (max / min). Null: none. Mac 0x377090 (name inferred)
    void SetButton(Control* button, RefPointer<Control>& slot, Real64 limit);
    // The preference the value is saved in; loads the saved value if there is one.
    // Mac 0x377180 (name inferred)
    void SetPreferenceName(const String& name);

    // fields of the Lua get / set functions
    UInt32                m_axis;             // Mac +0xa8 1 = vertical (y), 0 = horizontal (SetVertical)
    bool                  m_reverse;          // Mac +0xac
    bool                  m_keepFraction;     // Mac +0xad never set (both builds)
    bool                  m_proportional;     // Mac +0xae
    bool                  m_respondToParent;  // Mac +0xb0 track presses move the value
    Real64                m_visible;          // Mac +0x78 proportional knob: visible part ("over")
    Real64                m_total;            // Mac +0x80 proportional knob: whole ("under")
    RefPointer<Control>   m_incButton;        // Mac +0x88
    RefPointer<Control>   m_decButton;        // Mac +0x8c
    String                m_preferenceName;   // Mac +0x94

protected:
    // LuaButtonControl's (overridden without a change). Mac 0x3779e0, Win 0x460120
    virtual void OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args);
    // LuaControl's, then <OnValueChanged>, <OnParentPressed>, <OnButton>. Mac 0x3779f0, Win 0x460130
    virtual void DefineCallbacks(const XMLDOMNode* layout, LuaLayoutArgs& args);
    // LuaButtonControl's (overridden without a change). Mac 0x377a70 (thunk 0x377a80), Win 0x460190
    virtual bool Attach(Control* control, const String& name);
    // Leaves the track, ends a drag, drops the inc / dec buttons, then LuaButtonControl's.
    // Mac 0x377aa0 (thunk 0x377b40), Win 0x4601b0
    virtual void Detach();

    // ---- ControlObserver ----
    // LuaControl's; the knob or the track changed: UpdateKnob (not while it runs).
    virtual void OnRectangleChanged(Control* control, const Vector2& oldPosition,
                                    const Vector2& oldSize);                // Mac 0x377d30 (thunk 0x377d80), Win 0x4602c0
    // Knob: starts a drag (capture). Track (m_respondToParent): OnParentPressed on the side of
    // the knob that was pressed, with the pressed fraction of the track.
    virtual void OnMousePressed(Control* control, const Vector2& position, UInt32 button); // Mac 0x377f20 (thunk 0x3780f0), Win 0x460390
    // Knob, dragging: DragTo, the drag ends.
    virtual void OnMouseReleased(Control* control, const Vector2& position, UInt32 button); // Mac 0x378110 (thunk 0x378190), Win 0x4604d0
    // LuaButtonControl's; the inc / dec button: OnButton(true / false).
    virtual void OnMouseClicked(Control* control, const Vector2& position, UInt32 button); // Mac 0x377de0 (thunk 0x377e30), Win 0x460310
    // The knob's parent changed (Control::SetParent notifies this slot): it becomes the track.
    virtual void OnDestroyed(Control* control);                             // Mac 0x377bf0 (thunk 0x377c90), Win 0x460250
    // Knob, dragging: DragTo.
    virtual void OnMouseMove(Control* control, const Vector2& position);   // Mac 0x377e90 (thunk 0x377ed0), Win 0x460360

public:
    // ---- new virtuals (Mac slots 28-32, Win 9-13; public for the Lua functions) ----
    // self:OnParentPressed(increase, factor); without a Lua handler (or when it fails) the value
    // moves by (max - min) / 10. Mac 0x378210, Win 0x460ea0
    virtual void OnParentPressed(bool increase, Real64 factor);
    // self:OnButton(increase); otherwise the same move. Mac 0x378460, Win 0x460740
    virtual void OnButton(bool increase);
    // Saves the preference (when named), then self:OnValueChanged(value). Mac 0x3786a0, Win 0x4611d0
    virtual void OnValueChanged();
    // Knob length (proportional) and position on the track; hidden when the range is under half a
    // step (or the proportional total is 0); inc / dec buttons enabled. Mac 0x3787e0, Win 0x460850
    virtual void UpdateKnob();
    // The value at knob position (drag start position + `position` - drag offset). Mac 0x378a90, Win 0x460a60
    virtual void DragTo(const Vector2& position);

private:
    // `value` snapped and clamped (see SetValue). (inline in the original)
    Real64 Snap(Real64 value) const;

    Real64   m_min;            // Mac +0x58 0
    Real64   m_max;            // Mac +0x60 0
    Real64   m_step;           // Mac +0x68 1
    Real64   m_value;          // Mac +0x70 0
    Control* m_track;          // Mac +0x90 the knob's parent (observed)
    UInt32   m_updating;       // Mac +0xa4 UpdateKnob running
    bool     m_dragging;       // Mac +0xaf
    Vector2  m_dragOffset;     // Mac +0xb4 knob-local press position
};
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

// Registers SliderControl (base Control) {SetMinMaxStep … GetPreferenceName, Create (upvalue
// `manager`)}. Mac 0x375f60, Win 0x45fbc0 (name inferred)
void RegisterLuaSliderControl(LuaPlus::LuaState* state, ControlManager* manager);

} // namespace LEGO
