// LEGO Digital Designer — restored source
// The Lua library `base`: MillisecCurrentTime, GetPlatformShortName, GetPreference(key),
// GetPreferenceAsBool / AsNumber(key), SetPreference(key, v), GetApplicationVersion,
// GetLocText(key), GetBuildType. TU 305 "LuaPreferences.cpp" (names inferred):
// Mac 0x2b3b20-0x2b45bf, Win 0x46bac0-0x46c2e5.
#pragma once
#include "LEGO/Types.h"

namespace LuaPlus {
class LuaState;
}

namespace LEGO {

// luaL_register(L, "base", …) and pops the library table. Called first by AppMain.
void RegisterLuaBaseLibrary(LuaPlus::LuaState* state);                    // Mac 0x2b3b20, Win 0x46c220

} // namespace LEGO
