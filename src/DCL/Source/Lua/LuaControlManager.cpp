// LEGO Digital Designer — restored source
// TU 378 "LuaControlManager.cpp": Mac 0x368a70-0x3696eb, Win 0x448e10-0x44984f.
#include "LuaControlManager.h"
#include "LuaBindings.h"
#include "LuaControl.h"
#include "LuaWindow.h"
#include "ControlManager.h"
#include "Window.h"
#include "KeyNames.h"

using namespace LuaPlus;

namespace LEGO {

namespace {

// The ControlManager of argument `narg` (CheckAdaptedObject; Mac 0x3695a0 casts to
// LuaControlManager, the error names ControlManager).
ControlManager* CheckLuaControlManager(LuaState* state, int narg)          // Mac 0x3695a0
{
    return AdapterBindableLuaObjectBase<ControlManager, BindableLuaObject>::CheckAdaptedObject(state, narg);
}

// Pushes the Lua table of `control` (made on demand, class "Control"); shared by the getters.
void PushControlTable(LuaState* state, Control* control, bool createForNull)
{
    LuaObject table = LuaControl::GetControlTable(state, control);
    if (table.IsNil() && (createForNull || control))
        table = LuaControl::CreateControlTable(state, control);
    table.Push();
}

// self:GetBackground() -> the Lua table of layer 1 (made on demand). Mac 0x368cd0, Win 0x448f80
int LuaControlManagerGetBackground(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    ControlManager* manager = CheckLuaControlManager(state, 1);
    PushControlTable(state, manager->GetLayer(1), true);
    return 1;
}

// self:GetKeyboardFocus() -> the Lua table of the focus control, or nil. Mac 0x368da0, Win 0x449050
int LuaControlManagerGetKeyboardFocus(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    ControlManager* manager = CheckLuaControlManager(state, 1);
    PushControlTable(state, manager->GetKeyboardFocus(), false);
    return 1;
}

// self:GetWindow() -> the Lua table of the window (CreateLuaWindow on demand). Mac 0x368e70, Win 0x449130
int LuaControlManagerGetWindow(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    ControlManager* manager = CheckLuaControlManager(state, 1);
    Window* window = manager->GetWindow();
    LuaObject table = GetObjectMapEntry(state, window);
    if (table.IsNil())
        table = CreateLuaWindow(window, state);
    table.Push();
    return 1;
}

// self:IsKeyPressed(name) (key names: StringToKey). Mac 0x368f40, Win 0x449210
int LuaControlManagerIsKeyPressed(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    ControlManager* manager = CheckLuaControlManager(state, 1);
    UInt32 key;
    if (lua_isstring(L, 2)) {
        key = StringToKey::GetKey(lua_tostring(L, 2));
    } else {
        // wide strings (CheckLuaString raises the error for other types)
        AString name = CheckLuaString(state, 2).ToAString();
        key = StringToKey::GetKey(name.GetBuffer());
    }
    lua_pushboolean(L, manager->GetWindow()->IsKeyPressed(key));
    return 1;
}

// self:GetModifiers() -> control, shift, alt (modifier bits 1, 2, 4). Mac 0x3690e0, Win 0x449370
int LuaControlManagerGetModifiers(lua_State* L)
{
    ControlManager* manager = CheckLuaControlManager(LuaState::CastState(L), 1);
    UInt32 modifiers = manager->GetModifiers();
    lua_pushboolean(L, modifiers & 1);
    lua_pushboolean(L, (modifiers & 2) >> 1);
    lua_pushboolean(L, (modifiers & 4) >> 2);
    return 3;
}

const luaL_Reg kFunctions[] = {                                            // Mac 0x7506e0
    { "GetBackground", LuaControlManagerGetBackground },
    { "GetKeyboardFocus", LuaControlManagerGetKeyboardFocus },
    { "GetWindow", LuaControlManagerGetWindow },
    { "IsKeyPressed", LuaControlManagerIsKeyPressed },
    { "GetModifiers", LuaControlManagerGetModifiers },
    { 0, 0 }
};

} // namespace

LuaObject CreateLuaControlManager(ControlManager* manager, LuaState* state) // Mac 0x368a70, Win 0x448e10
{
    {
        LuaObject existing = GetObjectMapEntry(state, manager);
        if (!existing.IsNil())
            return LuaObject(state);
    }
    LuaObject table = CreateLuaInstance(state, "ControlManager");
    RefPointer<LuaControlManager> object = new LuaControlManager(manager);
    AttachLuaObject(table, object.Get());
    return table;
}

void RegisterLuaControlManager(LuaState* state)                             // Mac 0x368c00, Win 0x4493f0
{
    LuaObject luaClass = CreateLuaClass(state, "ControlManager", "Object");
    luaClass.Register(kFunctions, 0);
    luaClass.Register("New", LuaReturnNothing, 0);
    luaClass.Register("Create", LuaReturnNothing, 0);
}

} // namespace LEGO
