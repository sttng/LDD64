// LEGO Digital Designer — restored source
// TU 380 "LuaMode.cpp": Mac 0x36a1f0-0x36b31b, Win 0x64bac9-0x64bde1 (+ 0x64b8ae, 0x64b8cf).
// See LuaMode.h. The Mac static initialiser 0x36b2a0 is not code.
#include "LuaMode.h"
#include "LuaBindings.h"
#include "LuaWindow.h"
#include "XMLDOMNode.h"
#include "Menu.h"

#include <typeinfo>

using namespace LuaPlus;

namespace LEGO {

namespace {

// self:IsCurrent(). Mac 0x36a2c0
int LuaModeIsCurrent(lua_State* L)
{
    Mode* mode = LuaMode::CheckMode(LuaState::CastState(L), 1);
    lua_pushboolean(L, mode->IsCurrent());
    return 1;
}

// self:CanEnter(). Mac 0x36a310
int LuaModeCanEnter(lua_State* L)
{
    Mode* mode = LuaMode::CheckMode(LuaState::CastState(L), 1);
    lua_pushboolean(L, mode->CanEnter());
    return 1;
}

// self:CanExit(). Mac 0x36a370
int LuaModeCanExit(lua_State* L)
{
    Mode* mode = LuaMode::CheckMode(LuaState::CastState(L), 1);
    lua_pushboolean(L, mode->CanExit());
    return 1;
}

const luaL_Reg kFunctions[] = {                                            // Mac 0x751090
    { "IsCurrent", LuaModeIsCurrent },
    { "CanEnter", LuaModeCanEnter },
    { "CanExit", LuaModeCanExit },
    { 0, 0 }
};

} // namespace

// NOTE (restoration): raises only after the LuaObject went out of scope (see CheckBoundObject).
Mode* LuaMode::CheckMode(LuaState* state, int narg)                          // Mac 0x36b1d0
{
    Mode* mode = 0;
    const char* got = 0;
    {
        LuaObject object(state, narg);
        BindableLuaObject* bound = GetBoundObject(object);
        LuaMode* luaMode = bound ? dynamic_cast<LuaMode*>(bound) : 0;
        if (luaMode)
            mode = luaMode->m_mode;
        if (!mode) {
            BindableLuaObject* other = GetBoundObject(object);
            got = other ? typeid(*other).name() : 0;
        }
    }
    if (!mode)
        RaiseLuaTypeError(state->GetCState(), narg, got, typeid(Mode).name());
    return mode;
}

void LuaMode::OnFinalRelease()                                               // Mac 0x36aa50
{
    ModeObserver::OnFinalRelease();
    LuaControl::OnFinalRelease();
}

void LuaMode::ApplyControlAttributes(const XMLDOMNode* layout, LuaLayoutArgs& args) // Mac 0x36ac40, Win 0x64bac9
{
    LuaControl::ApplyControlAttributes(layout, args);
    XMLDOMNode* menuNode = layout->FindChild(String("Menu"));
    if (!menuNode)
        return;
    RefPointer<Menu> menu;
    if (CreateLuaMenu(args.m_table->GetState(), menuNode, menu, args.m_table))
        m_mode->SetMenu(menu);
}

bool LuaMode::Attach(Control* control, const String& name)                  // Mac 0x36ad40, Win 0x64bdb3
{
    if (!LuaControl::Attach(control, name))
        return false;
    Control* target = GetTargetControl();
    m_mode = target ? dynamic_cast<Mode*>(target) : 0;
    // NOTE: the original registers with the mode without a null check (the control is always
    // a Mode); restored: a non-Mode control leaves the previous mode.
    if (m_mode)
        ModeObserver::Attach(m_mode);
    else
        ModeObserver::Detach();
    return true;
}

void LuaMode::Detach()                                                       // Mac 0x36b030
{
    ModeObserver::Detach();
    m_mode = 0;
    LuaControl::Detach();
}

void LuaMode::OnEnter(Mode* mode)                                            // Mac 0x36b0f0, Win 0x64b8ae
{
    if (m_mode == mode)
        CallLuaMethod(static_cast<BindableLuaObject*>(this), "OnEnter", false, 0);
}

void LuaMode::OnExit(Mode* mode)                                             // Mac 0x36b160, Win 0x64b8cf
{
    if (m_mode == mode)
        CallLuaMethod(static_cast<BindableLuaObject*>(this), "OnExit", false, 0);
}

void RegisterLuaMode(LuaState* state)                                        // Mac 0x36a1f0, Win 0x64bce4
{
    LuaObject luaClass = CreateLuaClass(state, "Mode", "Control");
    luaClass.Register(kFunctions, 0);
    luaClass.Register("New", LuaReturnNothing, 0);
    luaClass.Register("Create", LuaReturnNothing, 0);
}

} // namespace LEGO
