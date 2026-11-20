// LEGO Digital Designer — restored source
// TU 305 "LuaPreferences.cpp" (name inferred; no class of its own): the Lua library `base`.
// Mac 0x2b3b20-0x2b45bf, Win 0x46bac0-0x46c2e5 (registration Win 0x46c220).
#include "LuaPreferences.h"
#include "LuaBindings.h"
#include "Preferences.h"
#include "StringStream.h"
#include "SystemTime.h"
#include "Platform.h"
#include "TextLocale.h"
#include "Version.h"
#include "Application.h"

using namespace LuaPlus;

#if defined(LDD_LINUX)
// LDD64 Linux (not restored code): set while CheckMenuAccess (LuaWindow.cpp) runs a menu's <Access>
// script, so the Linux build gets the Windows menus (Exit, Edit > Preferences, Help > About and
// the Windows short-cuts) in place of the Mac's, whose missing items Cocoa's application menu holds.
bool g_linuxMenuAccessAsWindows = false;
#endif

namespace LEGO {

namespace {

// Platform suffix: Win 0x41fdd0 (platform singleton member, "WIN"); inlined on the Mac as the
// literal "MAC" right after the singleton accessor (Mac 0x2b3bb0). See TextLocale.cpp.
inline String GetPlatformName()
{
#if defined(_WIN32)
    return String("WIN");
#else
    return String("MAC");
#endif
}

// base.MillisecCurrentTime() -> number (32-bit millisecond clock).
int LuaMillisecCurrentTime(lua_State* L)                                   // Mac 0x2b3b60, Win 0x46bac0
{
    lua_pushnumber(L, (lua_Number)(UInt32)GetMilliseconds());
    return 1;
}

// base.GetPlatformShortName() -> "MAC" / "WIN".
int LuaGetPlatformShortName(lua_State* L)                                  // Mac 0x2b3bb0, Win 0x46baf0
{
    LuaState* state = LuaState::CastState(L);
    Platform::GetInstance();
#if defined(LDD_LINUX)
    if (::g_linuxMenuAccessAsWindows) {
        PushLuaString(state, String("WIN"));
        return 1;
    }
#endif
    PushLuaString(state, GetPlatformName());
    return 1;
}

// base.GetPreference(key) -> string or nil.
int LuaGetPreference(lua_State* L)                                         // Mac 0x2b3c30, Win 0x46bb70
{
    LuaState* state = LuaState::CastState(L);
    String value;
    bool found;
    {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        found = preferences->GetValue(CheckLuaString(state, 1), value);
    }
    if (found)
        PushLuaString(state, value);
    else
        state->PushNil();
    return 1;
}

// base.GetPreferenceAsBool(key) -> boolean or nil (0/1/no/yes/false/true).
int LuaGetPreferenceAsBool(lua_State* L)                                   // Mac 0x2b3d40, Win 0x46bc70
{
    LuaState* state = LuaState::CastState(L);
    bool value = false;
    bool found;
    {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        found = preferences->GetValue(CheckLuaString(state, 1), value);
    }
    if (found)
        lua_pushboolean(L, value);
    else
        state->PushNil();
    return 1;
}

// base.GetPreferenceAsNumber(key) -> number or nil.
int LuaGetPreferenceAsNumber(lua_State* L)                                 // Mac 0x2b3e70, Win 0x46bd50
{
    LuaState* state = LuaState::CastState(L);
    Real64 value = 0;
    bool found;
    {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        String key = CheckLuaString(state, 1);
        String text;
        found = preferences->GetValue(key, text) && FromString(value, text);
    }
    if (found)
        lua_pushnumber(L, value);
    else
        state->PushNil();
    return 1;
}

// base.SetPreference(key, value): booleans as "yes"/"no", numbers via String << Real64,
// (wide) strings as they are; nil removes the key; other types raise an argument error.
int LuaSetPreference(lua_State* L)                                         // Mac 0x2b3fa0, Win 0x46be40
{
    LuaState::CastState(L);
    String key = CheckLuaString(L, 1);
    switch (lua_type(L, 2)) {
    case LUA_TNONE:
    case LUA_TNIL: {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        preferences->RemoveValue(key);
        break;
    }
    case LUA_TBOOLEAN: {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        bool value = lua_toboolean(L, 2) != 0;
        String text;
        ToString(value, text);
        preferences->SetValue(key, text);
        break;
    }
    case LUA_TNUMBER: {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        Real64 value = lua_tonumber(L, 2);
        String text;
        ToString(value, text);
        preferences->SetValue(key, text);
        break;
    }
    case LUA_TSTRING:
    case LUA_TWSTRING: {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        preferences->SetValue(key, LuaToString(L, 2));
        break;
    }
    default:
        // The original appends to an AString (leaked by luaL_argerror's longjmp); the String key above
        // is not destroyed either (both builds).
        luaL_argerror(L, 2, lua_pushfstring(L, "Cannot set preference values of type %s.",
                                            lua_typename(L, lua_type(L, 2))));
        break;
    }
    return 0;
}

// base.GetApplicationVersion() -> "major.minor.build.revision".
int LuaGetApplicationVersion(lua_State* L)                                 // Mac 0x2b4410, Win 0x46c0a0
{
    LuaState* state = LuaState::CastState(L);
    Version version;
    Application::GetInstance()->GetVersion(version);
    String text;
    ToString(version, text);
    PushLuaString(state, text);
    return 1;
}

// base.GetLocText(key) -> the localised text of `key`.
int LuaGetLocText(lua_State* L)                                            // Mac 0x2b44a0, Win 0x46c150
{
    LuaState* state = LuaState::CastState(L);
    String key = CheckLuaString(state, 1);
    LocalizedString text(key);
    PushLuaString(state, text);
    return 1;
}

// base.GetBuildType() -> "RELEASE".
int LuaGetBuildType(lua_State* L)                                          // Mac 0x2b4540, Win 0x46c200
{
    LuaState::CastState(L);
    lua_pushstring(L, "RELEASE");
    return 1;
}

const luaL_Reg kBaseFunctions[] = {                                        // Mac 0x74530c (Win on the stack)
    { "MillisecCurrentTime", LuaMillisecCurrentTime },
    { "GetPlatformShortName", LuaGetPlatformShortName },
    { "GetPreference", LuaGetPreference },
    { "GetPreferenceAsBool", LuaGetPreferenceAsBool },
    { "GetPreferenceAsNumber", LuaGetPreferenceAsNumber },
    { "SetPreference", LuaSetPreference },
    { "GetApplicationVersion", LuaGetApplicationVersion },
    { "GetLocText", LuaGetLocText },
    { "GetBuildType", LuaGetBuildType },
    { 0, 0 }
};

} // namespace

void RegisterLuaBaseLibrary(LuaState* state)                               // Mac 0x2b3b20, Win 0x46c220
{
    lua_State* L = state->GetCState();
    luaL_register(L, "base", kBaseFunctions);
    lua_settop(L, -2);
}

} // namespace LEGO
