// LEGO Digital Designer — restored source
// TU 387 "LuaSliderControl.cpp": Mac 0x375db0-0x378fab, Win 0x45f210-0x46139d.
#include "LuaSliderControl.h"
#include "LuaBindings.h"
#include "XMLDOMNode.h"
#include "StringStream.h"
#include "Preferences.h"
#include "ButtonControl.h"
#include "ControlManager.h"

#include <math.h>

using namespace LuaPlus;

namespace LEGO {

namespace {

// |a - b| at least 1e-8f (the inc / dec buttons' enable test; both builds compare with the
// double of 1e-8f).
inline bool Differs(Real64 a, Real64 b)
{
    return (Real64)1e-8f <= fabs(a - b);
}

inline Real32& Component(Vector2& v, UInt32 axis) { return axis ? v.y : v.x; }
inline Real32 Component(const Vector2& v, UInt32 axis) { return axis ? v.y : v.x; }

LuaSliderControl* CheckSlider(lua_State* L)                                 // Mac 0x378c50
{
    return CheckBoundObject<LuaSliderControl>(LuaState::CastState(L), 1);
}

// self:SetMinMaxStep([min, max, step]) (missing: keep). Mac 0x376030
int LuaSliderSetMinMaxStep(lua_State* L)
{
    LuaSliderControl* self = CheckSlider(L);
    Real64 min = luaL_optnumber(L, 2, self->GetMin());
    Real64 max = luaL_optnumber(L, 3, self->GetMax());
    Real64 step = luaL_optnumber(L, 4, self->GetStep());
    self->SetRange(min, max, step);
    return 0;
}

// self:GetMinMaxStep() -> min, max, step. Mac 0x3760d0
int LuaSliderGetMinMaxStep(lua_State* L)
{
    LuaSliderControl* self = CheckSlider(L);
    lua_pushnumber(L, self->GetMin());
    lua_pushnumber(L, self->GetMax());
    lua_pushnumber(L, self->GetStep());
    return 3;
}

// self:SetValue([value]). Mac 0x376160
int LuaSliderSetValue(lua_State* L)
{
    LuaSliderControl* self = CheckSlider(L);
    self->SetValue(luaL_optnumber(L, 2, self->GetValue()));
    return 0;
}

// self:GetValue(). Mac 0x376290
int LuaSliderGetValue(lua_State* L)
{
    LuaSliderControl* self = CheckSlider(L);
    lua_pushnumber(L, self->GetValue());
    return 1;
}

// self:SetFactor([factor]): value = min + (max - min) * factor. Mac 0x3762e0
int LuaSliderSetFactor(lua_State* L)
{
    LuaSliderControl* self = CheckSlider(L);
    Real64 range = self->GetMax() - self->GetMin();
    Real64 factor = luaL_optnumber(L, 2, (self->GetValue() - self->GetMin()) / range);
    self->SetValue(range * factor + self->GetMin());
    return 0;
}

// self:GetFactor(). Mac 0x376430
int LuaSliderGetFactor(lua_State* L)
{
    LuaSliderControl* self = CheckSlider(L);
    lua_pushnumber(L, (self->GetValue() - self->GetMin()) / (self->GetMax() - self->GetMin()));
    return 1;
}

// self:IncreaseSteps(steps). Mac 0x3764a0
int LuaSliderIncreaseSteps(lua_State* L)
{
    LuaSliderControl* self = CheckSlider(L);
    Real64 steps = luaL_checknumber(L, 2);
    self->SetValue(steps * self->GetStep() + self->GetValue());
    return 0;
}

// self:Increase(delta). Mac 0x3765d0
int LuaSliderIncrease(lua_State* L)
{
    LuaSliderControl* self = CheckSlider(L);
    Real64 delta = luaL_checknumber(L, 2);
    self->SetValue(delta + self->GetValue());
    return 0;
}

// self:SetVertical([vertical = true]). Mac 0x3766f0
int LuaSliderSetVertical(lua_State* L)
{
    LuaSliderControl* self = CheckSlider(L);
    self->m_axis = OptLuaBoolean(L, 2, true) ? 1 : 0;
    self->UpdateKnob();
    return 0;
}

// self:GetVertical(). Mac 0x376750
int LuaSliderGetVertical(lua_State* L)
{
    LuaSliderControl* self = CheckSlider(L);
    lua_pushboolean(L, self->m_axis == 1);
    return 1;
}

// self:SetReverse([reverse = true]). Mac 0x3767b0
int LuaSliderSetReverse(lua_State* L)
{
    LuaSliderControl* self = CheckSlider(L);
    self->m_reverse = OptLuaBoolean(L, 2, true);
    self->UpdateKnob();
    return 0;
}

// self:GetReverse(). Mac 0x376810
int LuaSliderGetReverse(lua_State* L)
{
    LuaSliderControl* self = CheckSlider(L);
    lua_pushboolean(L, self->m_reverse);
    return 1;
}

// self:SetProportional([proportional = true]). Mac 0x376860
int LuaSliderSetProportional(lua_State* L)
{
    LuaSliderControl* self = CheckSlider(L);
    self->m_proportional = OptLuaBoolean(L, 2, true);
    self->UpdateKnob();
    return 0;
}

// self:GetProportional(). Mac 0x3768c0
int LuaSliderGetProportional(lua_State* L)
{
    LuaSliderControl* self = CheckSlider(L);
    lua_pushboolean(L, self->m_proportional);
    return 1;
}

// self:SetProportionalOverUnder([visible, total]). Mac 0x376910
int LuaSliderSetProportionalOverUnder(lua_State* L)
{
    LuaSliderControl* self = CheckSlider(L);
    Real64 visible = luaL_optnumber(L, 2, self->m_visible);
    Real64 total = luaL_optnumber(L, 3, self->m_total);
    self->m_visible = visible;
    self->m_total = total;
    self->UpdateKnob();
    return 0;
}

// self:GetProportionalOverUnder() -> visible, total. Mac 0x376990
int LuaSliderGetProportionalOverUnder(lua_State* L)
{
    LuaSliderControl* self = CheckSlider(L);
    lua_pushnumber(L, self->m_visible);
    lua_pushnumber(L, self->m_total);
    return 2;
}

// self:SetRespondToParentMousePressed([respond = true]): the track takes mouse input. Mac 0x376a00
int LuaSliderSetRespondToParentMousePressed(lua_State* L)
{
    LuaSliderControl* self = CheckSlider(L);
    bool respond = OptLuaBoolean(L, 2, true);
    self->m_respondToParent = respond;
    Control* track = self->GetTrack();
    if (respond && track) {
        track->SetInputFlags(track->GetInputFlags() | 1);
        track->SetHitTestable(true);
    }
    return 0;
}

// self:GetRespondToParentMousePressed(). Mac 0x376a80
int LuaSliderGetRespondToParentMousePressed(lua_State* L)
{
    LuaSliderControl* self = CheckSlider(L);
    lua_pushboolean(L, self->m_respondToParent);
    return 1;
}

// self:SetIncButton([control]). Mac 0x376ad0
int LuaSliderSetIncButton(lua_State* L)
{
    LuaSliderControl* self = CheckSlider(L);
    Control* button = OptLuaControl(LuaState::CastState(L), 2, 0);
    self->SetButton(button, self->m_incButton, self->GetMax());
    return 0;
}

// self:SetDecButton([control]). Mac 0x376b40
int LuaSliderSetDecButton(lua_State* L)
{
    LuaSliderControl* self = CheckSlider(L);
    Control* button = OptLuaControl(LuaState::CastState(L), 2, 0);
    self->SetButton(button, self->m_decButton, self->GetMin());
    return 0;
}

// self:SetPreferenceName([name = ""]). Mac 0x376bb0
int LuaSliderSetPreferenceName(lua_State* L)
{
    LuaSliderControl* self = CheckSlider(L);
    self->SetPreferenceName(OptLuaString(L, 2, String()));
    return 0;
}

// self:GetPreferenceName(). Mac 0x376c60
int LuaSliderGetPreferenceName(lua_State* L)
{
    LuaSliderControl* self = CheckSlider(L);
    PushLuaString(L, self->m_preferenceName);
    return 1;
}

// SliderControl.Create(self, parent): a new ButtonControl (no parent, flags 7, empty rectangle,
// hit-testable), bound with input flags 1. Mac 0x376ca0
int LuaSliderCreate(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    LuaObject self = GetUnboundSelf(state);
    ControlManager* manager = (ControlManager*)lua_touserdata(L, lua_upvalueindex(1));
    RefPointer<ButtonControl> control = new ButtonControl(manager);
    control->Create(0, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, true);
    RefPointer<LuaSliderControl> object = new LuaSliderControl;
    LuaObject parent(state, 2);
    LuaObject after(state);
    object->Initialize(self, parent, after, control.Get(), 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 1, true);
    return 0;
}

const luaL_Reg kFunctions[] = {                                            // Mac 0x75266c
    { "SetMinMaxStep", LuaSliderSetMinMaxStep },
    { "GetMinMaxStep", LuaSliderGetMinMaxStep },
    { "SetValue", LuaSliderSetValue },
    { "GetValue", LuaSliderGetValue },
    { "SetFactor", LuaSliderSetFactor },
    { "GetFactor", LuaSliderGetFactor },
    { "IncreaseSteps", LuaSliderIncreaseSteps },
    { "Increase", LuaSliderIncrease },
    { "SetVertical", LuaSliderSetVertical },
    { "GetVertical", LuaSliderGetVertical },
    { "SetReverse", LuaSliderSetReverse },
    { "GetReverse", LuaSliderGetReverse },
    { "SetProportional", LuaSliderSetProportional },
    { "GetProportional", LuaSliderGetProportional },
    { "SetProportionalOverUnder", LuaSliderSetProportionalOverUnder },
    { "GetProportionalOverUnder", LuaSliderGetProportionalOverUnder },
    { "SetRespondToParentMousePressed", LuaSliderSetRespondToParentMousePressed },
    { "GetRespondToParentMousePressed", LuaSliderGetRespondToParentMousePressed },
    { "SetIncButton", LuaSliderSetIncButton },
    { "SetDecButton", LuaSliderSetDecButton },
    { "SetPreferenceName", LuaSliderSetPreferenceName },
    { "GetPreferenceName", LuaSliderGetPreferenceName },
    { 0, 0 }
};

} // namespace

// ================================================================================================

LuaSliderControl::LuaSliderControl()                                        // Mac 0x375db0, Win 0x460030
    : m_axis(1), m_reverse(false), m_keepFraction(false), m_proportional(false),
      m_respondToParent(false), m_visible(0.0), m_total(0.0),
      m_min(0.0), m_max(0.0), m_step(1.0), m_value(0.0), m_track(0), m_updating(0),
      m_dragging(false), m_dragOffset()
{
}

LuaSliderControl::~LuaSliderControl()                                       // Mac 0x377460 (D2 0x377350)
{
}

// The step grid from min (the integer part of the quotient, then rounded to the nearer grid
// point), clamped to [min, max].
Real64 LuaSliderControl::Snap(Real64 value) const
{
    Real64 offset = value - m_min;
    Real64 snapped = offset;
    if (m_step != 0.0) {
        snapped = (Real64)(Int64)(offset / m_step) * m_step;
        Real64 rest = offset - snapped;
        if (0.0 <= rest) {
            if (m_step * 0.5 <= rest)
                snapped = m_step + snapped;
        } else if (rest <= m_step * -0.5) {
            snapped = snapped - m_step;
        }
    }
    snapped = snapped + m_min;
    Real64 clamped = snapped <= m_max ? snapped : m_max;
    return snapped < m_min ? m_min : clamped;
}

void LuaSliderControl::SetValue(Real64 value)
{
    Real64 snapped = Snap(value);
    if (m_value != snapped) {
        m_value = snapped;
        UpdateKnob();
        OnValueChanged();
    }
}

// Mac 0x376f20
void LuaSliderControl::SetRange(Real64 min, Real64 max, Real64 step)
{
    Real64 old = m_value;
    Real64 oldMin = m_min;
    Real64 oldMax = m_max;
    m_min = min;
    m_max = max;
    m_step = step;
    Real64 value = old;
    if (m_keepFraction)
        value = (max - min) * ((old - oldMin) / (oldMax - oldMin)) + min;
    Real64 snapped = Snap(value);
    if (old != snapped) {
        m_value = snapped;
        UpdateKnob();
        OnValueChanged();
    } else {
        UpdateKnob();
    }
}

// Mac 0x377090
void LuaSliderControl::SetButton(Control* button, RefPointer<Control>& slot, Real64 limit)
{
    if (slot) {
        ControlObserver::Detach(slot.Get());
        if (slot.Get() != button)
            slot = button;
    } else if (button) {
        slot = button;
    }
    if (slot) {
        ControlObserver::Attach(slot.Get());
        slot->SetInputFlags(slot->GetInputFlags() | 1);
        slot->SetHitTestable(true);
        slot->SetEnabled(Differs(m_value, limit));
    }
}

// Mac 0x377180
void LuaSliderControl::SetPreferenceName(const String& name)
{
    m_preferenceName = name;
    if (m_preferenceName.IsEmpty())
        return;
    Real64 value = m_value;
    {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        String text;
        if (preferences->GetValue(m_preferenceName, text))
            FromString(value, text);
    }
    SetValue(value);
}

void LuaSliderControl::OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args) // Mac 0x3779e0, Win 0x460120
{
    LuaButtonControl::OnLayoutApplied(layout, args);
}

void LuaSliderControl::DefineCallbacks(const XMLDOMNode* layout, LuaLayoutArgs& args) // Mac 0x3779f0, Win 0x460130
{
    LuaControl::DefineCallbacks(layout, args);
    DefineLayoutFunction(layout, "OnValueChanged", "value", args);
    DefineLayoutFunction(layout, "OnParentPressed", "inc, pressedFactor", args);
    DefineLayoutFunction(layout, "OnButton", "inc", args);
}

bool LuaSliderControl::Attach(Control* control, const String& name)        // Mac 0x377a70, Win 0x460190
{
    return LuaButtonControl::Attach(control, name);
}

void LuaSliderControl::Detach()                                             // Mac 0x377aa0, Win 0x4601b0
{
    if (m_track) {
        ControlObserver::Detach(m_track);
        m_track = 0;
    }
    if (m_dragging) {
        GetTargetControl()->ReleaseMouseCapture();
        m_dragging = false;
    }
    SetButton(0, m_incButton, m_max);
    SetButton(0, m_decButton, m_min);
    LuaButtonControl::Detach();
}

void LuaSliderControl::OnDestroyed(Control* control)                        // Mac 0x377bf0, Win 0x460250
{
    if (ControlAugmentation::GetControl() != control)
        return;
    if (m_track)
        ControlObserver::Detach(m_track);
    m_track = control->GetParent();
    if (!m_track)
        return;
    ControlObserver::Attach(m_track);
    if (m_respondToParent) {
        m_track->SetInputFlags(m_track->GetInputFlags() | 1);
        m_track->SetHitTestable(true);
    }
    UpdateKnob();
}

void LuaSliderControl::OnRectangleChanged(Control* control, const Vector2& oldPosition,
                                          const Vector2& oldSize)          // Mac 0x377d30, Win 0x4602c0
{
    LuaControl::OnRectangleChanged(control, oldPosition, oldSize);
    if (m_updating == 0 && (ControlAugmentation::GetControl() == control || m_track == control))
        UpdateKnob();
}

void LuaSliderControl::OnMouseClicked(Control* control, const Vector2& position, UInt32 button) // Mac 0x377de0, Win 0x460310
{
    LuaButtonControl::OnMouseClicked(control, position, button);
    if (m_incButton.Get() == control)
        OnButton(true);
    if (m_decButton.Get() == control)
        OnButton(false);
}

void LuaSliderControl::OnMouseMove(Control* control, const Vector2& position) // Mac 0x377e90, Win 0x460360
{
    if (m_dragging && ControlAugmentation::GetControl() == control)
        DragTo(position);
}

// Mac 0x377f20, Win 0x460390
void LuaSliderControl::OnMousePressed(Control* control, const Vector2& position, UInt32 button)
{
    LuaControl::OnMousePressed(control, position, button);
    if (button != 0)
        return;
    if (ControlAugmentation::GetControl() == control) {
        GetTargetControl()->CaptureMouse();
        m_dragging = true;
        m_dragOffset = position;
    }
    if (!m_respondToParent || m_track != control)
        return;
    Real32 pressed = Component(position, m_axis);
    Real64 factor = (Real64)pressed / (Real64)Component(m_track->GetSize(), m_axis);
    if (1.0 <= factor)
        factor = 1.0;
    if (m_reverse)
        factor = 1.0 - factor;
    Control* knob = GetTargetControl();
    bool increase;
    if (Component(knob->GetPosition(), m_axis) <= pressed) {
        if (pressed <= Component(knob->GetPosition(), m_axis) + Component(knob->GetSize(), m_axis))
            return;
        increase = !m_reverse;
    } else {
        increase = m_reverse;
    }
    OnParentPressed(increase, factor);
}

// Mac 0x378110, Win 0x4604d0
void LuaSliderControl::OnMouseReleased(Control* control, const Vector2& position, UInt32 button)
{
    LuaControl::OnMouseReleased(control, position, button);
    if (button == 0 && m_dragging && ControlAugmentation::GetControl() == control) {
        DragTo(position);
        m_dragging = false;
        GetTargetControl()->ReleaseMouseCapture();
    }
}

// self:OnParentPressed(inc, factor) (CallLuaMethod<bool, double>, Mac 0x378d20).
// Mac 0x378210, Win 0x460ea0
void LuaSliderControl::OnParentPressed(bool increase, Real64 factor)
{
    bool handled;
    {
        LuaObject self = LEGO::GetLuaObject(static_cast<BindableLuaObject*>(this));
        handled = CallLuaMethod(self, "OnParentPressed", false, 0, increase, factor);
    }
    if (handled)
        return;
    Real64 tenth = (m_max - m_min) / 10.0;
    SetValue(increase ? tenth + m_value : m_value - tenth);
}

// self:OnButton(inc) (CallLuaMethod<bool>, Mac 0x196870). Mac 0x378460, Win 0x460740
void LuaSliderControl::OnButton(bool increase)
{
    bool handled;
    {
        LuaObject self = LEGO::GetLuaObject(static_cast<BindableLuaObject*>(this));
        handled = CallLuaMethod(self, "OnButton", false, 0, increase);
    }
    if (handled)
        return;
    Real64 tenth = (m_max - m_min) / 10.0;
    SetValue(increase ? tenth + m_value : m_value - tenth);
}

// self:OnValueChanged(value) (CallLuaMethod<double>, Mac 0x378e30). Mac 0x3786a0, Win 0x4611d0
void LuaSliderControl::OnValueChanged()
{
    if (!m_preferenceName.IsEmpty()) {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        String text;
        ToString(m_value, text);
        preferences->SetValue(m_preferenceName, text);
    }
    LuaObject self = LEGO::GetLuaObject(static_cast<BindableLuaObject*>(this));
    CallLuaMethod(self, "OnValueChanged", false, 0, m_value);
}

// Mac 0x3787e0, Win 0x460850
void LuaSliderControl::UpdateKnob()
{
    if (!m_track)
        return;
    ++m_updating;
    Control* knob = GetTargetControl();
    bool shown = false;
    bool hide = fabs(m_min - m_max) < m_step * 0.5;
    if (!hide && m_proportional && fabs(m_total) < (Real64)1e-8f)
        hide = true;
    if (hide) {
        knob->SetShown(false);
    } else {
        if (m_proportional) {
            Vector2 size(knob->GetSize());
            Real32 length = (Real32)floor((m_visible / m_total) * (Real64)Component(m_track->GetSize(), m_axis));
            Real32 minimum = Component(knob->GetMinimumSize(), m_axis);
            if (length <= minimum)
                length = minimum;
            Component(size, m_axis) = length;
            knob->SetSize(size);
        }
        Vector2 free(m_track->GetSize());
        free -= knob->GetSize();
        Real64 factor = (m_value - m_min) / (m_max - m_min);
        if (m_reverse)
            factor = 1.0 - factor;
        Vector2 position(knob->GetPosition());
        Component(position, m_axis) = (Real32)((Real64)Component(free, m_axis) * factor);
        knob->SetPosition(position);
        knob->SetShown(true);
        shown = true;
    }
    if (m_incButton)
        m_incButton->SetEnabled(shown && Differs(m_value, m_max));
    if (m_decButton)
        m_decButton->SetEnabled(shown && Differs(m_value, m_min));
    --m_updating;
}

// Mac 0x378a90, Win 0x460a60
void LuaSliderControl::DragTo(const Vector2& position)
{
    Control* knob = GetTargetControl();
    Vector2 free(m_track->GetSize());
    free -= knob->GetSize();
    Vector2 at(knob->GetPosition());
    at -= m_dragOffset;
    at += position;
    Real64 factor = (Real64)Component(at, m_axis) / (Real64)Component(free, m_axis);
    if (m_reverse)
        factor = 1.0 - factor;
    SetValue((m_max - m_min) * factor + m_min);
}

void RegisterLuaSliderControl(LuaState* state, ControlManager* manager)     // Mac 0x375f60, Win 0x45fbc0
{
    LuaObject luaClass = CreateLuaClass(state, "SliderControl", "Control");
    luaClass.Register(kFunctions, 0);
    lua_checkstack(state->GetCState(), 1);
    lua_pushlightuserdata(state->GetCState(), manager);
    luaClass.Register("Create", LuaSliderCreate, 1);
}

} // namespace LEGO
