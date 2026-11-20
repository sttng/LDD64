// LEGO Digital Designer — restored source
// TU 317 "LuaToggleControl.cpp": Mac 0x2c5c80-0x2c70db, Win 0x465dc0-0x4665af.
#include "LuaToggleControl.h"
#include "LuaBindings.h"
#include "XMLDOMNode.h"
#include "ToggleControl.h"
#include "ControlManager.h"

using namespace LuaPlus;

namespace LEGO {

namespace {

// self:GetToggled(). Mac 0x2c5d50
int LuaToggleControlGetToggled(lua_State* L)
{
    ToggleControl* control = CheckLuaToggleControl(LuaState::CastState(L), 1);
    lua_pushboolean(L, control->IsToggled());
    return 1;
}

// self:Toggle([toggled]): without an argument the state flips. Mac 0x2c5da0
int LuaToggleControlToggle(lua_State* L)
{
    ToggleControl* control = CheckLuaToggleControl(LuaState::CastState(L), 1);
    bool toggled;
    if (lua_type(L, 2) > LUA_TNIL)
        toggled = CheckLuaBoolean(L, 2);
    else
        toggled = !control->IsToggled();
    control->SetToggled(toggled);
    return 0;
}

// ToggleControl.Create(self, parent, [toggled = false]). Mac 0x2c5e20
int LuaToggleControlCreate(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    LuaObject self = GetUnboundSelf(state);
    ControlManager* manager = (ControlManager*)lua_touserdata(L, lua_upvalueindex(1));
    RefPointer<ToggleControl> control = new ToggleControl(manager);
    control->Initialize();
    RefPointer<LuaToggleControl> object = new LuaToggleControl;
    {
        LuaObject parent(state, 2);
        LuaObject after(state);
        object->Initialize(self, parent, after, control.Get(), 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
        control->SetToggled(OptLuaBoolean(L, 3, false));
    }
    return 0;
}

const luaL_Reg kFunctions[] = {                                            // Mac 0x748610
    { "GetToggled", LuaToggleControlGetToggled },
    { "Toggle", LuaToggleControlToggle },
    { 0, 0 }
};

} // namespace

LuaToggleControl::~LuaToggleControl()                                       // Mac 0x2c6100, Win 0x465dc0
{
}

// Mac 0x2c6650
void LuaToggleControl::OnFinalRelease()
{
    ToggleControlObserver::OnFinalRelease();
    LuaButtonControl::OnFinalRelease();
}

// Mac 0x2c6900, Win 0x466280
void LuaToggleControl::OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args)
{
    LuaButtonControl::OnLayoutApplied(layout, args);
    ApplyStateLayout(4, String("ToggledUp"), layout, args);
    ApplyStateLayout(5, String("ToggledOver"), layout, args);
    ApplyStateLayout(6, String("ToggledDown"), layout, args);
    ApplyStateLayout(7, String("ToggledDisabled"), layout, args);
}

// Mac 0x2c6f40, Win 0x466580
void LuaToggleControl::DefineCallbacks(const XMLDOMNode* layout, LuaLayoutArgs& args)
{
    LuaControl::DefineCallbacks(layout, args);
    DefineLayoutFunction(layout, "OnToggled", "toggled", args);
}

// NOTE: the toggle is observed without a null check (both builds; the control is always a
// ToggleControl). Mac 0x2c6a80, Win 0x4663b0
bool LuaToggleControl::Attach(Control* control, const String& name)
{
    if (!LuaButtonControl::Attach(control, name))
        return false;
    m_toggle = dynamic_cast<ToggleControl*>(GetTargetControl());
    ToggleControlObserver::Attach(m_toggle);
    if (Command* command = GetCommand())
        m_toggle->SetToggled(command->IsToggled());
    return true;
}

void LuaToggleControl::Detach()                                             // Mac 0x2c6bd0, Win 0x466410
{
    if (m_toggle) {
        ToggleControlObserver::Detach(m_toggle);
        m_toggle = 0;
    }
    LuaButtonControl::Detach();
}

void LuaToggleControl::OnAttached(Command* command)                         // Mac 0x2c6c40, Win 0x466440
{
    LuaButtonControl::OnAttached(command);
    if (m_toggle)
        m_toggle->SetToggled(command->IsToggled());
}

void LuaToggleControl::OnToggleChanged(bool toggled)                        // Mac 0x2c6ce0, Win 0x4664b0
{
    if (m_toggle)
        m_toggle->SetToggled(toggled);
}

void LuaToggleControl::OnAttached(ToggleControl* control)                   // Mac 0x2c6d40, Win 0x4664d0
{
    if (m_toggle == control) {
        if (Command* command = GetCommand())
            control->SetToggled(command->IsToggled());
    }
}

// self:OnToggled(true) (CallLuaMethod<bool>, Mac 0x196870). Mac 0x2c6dc0, Win 0x466500
void LuaToggleControl::OnToggled(ToggleControl* control)
{
    if (m_toggle != control)
        return;
    {
        LuaObject self = LEGO::GetLuaObject(static_cast<BindableLuaObject*>(this));
        CallLuaMethod(self, "OnToggled", false, 0, true);
    }
    if (Command* command = GetCommand())
        command->SetToggled(true);
}

void LuaToggleControl::OnUntoggled(ToggleControl* control)                  // Mac 0x2c6e80, Win 0x466540
{
    if (m_toggle != control)
        return;
    {
        LuaObject self = LEGO::GetLuaObject(static_cast<BindableLuaObject*>(this));
        CallLuaMethod(self, "OnToggled", false, 0, false);
    }
    if (Command* command = GetCommand())
        command->SetToggled(false);
}

ToggleControl* CheckLuaToggleControl(LuaState* state, int narg)            // Mac 0x2c6f90
{
    ToggleControl* result = 0;
    const char* got = 0;
    {
        LuaObject object(state, narg);
        BindableLuaObject* bound = GetBoundObject(object);
        LuaToggleControl* lua = bound ? dynamic_cast<LuaToggleControl*>(bound) : 0;
        if (lua)
            result = lua->GetToggle();
        if (!result) {
            BindableLuaObject* other = GetBoundObject(object);
            got = other ? typeid(*other).name() : 0;
        }
    }
    if (!result)
        RaiseLuaTypeError(state->GetCState(), narg, got, typeid(ToggleControl).name());
    return result;
}

void RegisterLuaToggleControl(LuaState* state, ControlManager* manager)     // Mac 0x2c5c80
{
    LuaObject luaClass = CreateLuaClass(state, "ToggleControl", "Control");
    luaClass.Register(kFunctions, 0);
    lua_checkstack(state->GetCState(), 1);
    lua_pushlightuserdata(state->GetCState(), manager);
    luaClass.Register("Create", LuaToggleControlCreate, 1);
}

} // namespace LEGO
