// LEGO Digital Designer — restored source
// TU 323 "LuaBindings.cpp" (name inferred): the LEGO Lua glue.
// Mac 0x2cf090-0x2d0ecf, Win 0x445370-0x446e5f.
#include "LuaBindings.h"
#include "Trace.h"
#include "StringConversion.h"

#include <stdio.h>

using namespace LuaPlus;

namespace LEGO {

namespace {

// The Lua half of the Object class (run once by CreateLuaState).
const char kObjectScript[] =
    "function Object:New()\n"
    "\tlocal new_inst = {}\n"
    "\tsetmetatable( new_inst, self.classMetatable )\n"
    "\tnew_inst:Create()\n"
    "\treturn new_inst\n"
    "end\n"
    "function Object:Create()\n"
    "end\n"
    "function Object:FindMember(memberName, chainName)\n"
    "\tlocal f = self[memberName]\n"
    "\tif f then\n"
    "\t\treturn self, f\n"
    "\telse\n"
    "\t\tlocal c = self[chainName]\n"
    "\t\tif c then\n"
    "\t\t\treturn c:FindMember(memberName, chainName)\n"
    "\t\telse\n"
    "\t\t\treturn nil, nil\n"
    "\t\tend\n"
    "\tend\n"
    "end\n"
    "function Object:CallMethod(functionName, chainName, ...)\n"
    "\tlocal o, m = self:FindMember(functionName, chainName)\n"
    "\tif o then\n"
    "\t\treturn m(o, ...)\n"
    "\telse\n"
    "\t\tTrace(\"Found no object to handle function call \" .. functionName .. \"() in chain \" .. chainName .. \".\")\n"
    "\tend\n"
    "end\n";

// lua_atpanic handler. (name inferred)
// NOTE: the "%s" is not formatted; the message is appended after the format text.
int LuaPanic(lua_State* L)                                                 // Mac 0x2cf370, Win 0x4454f0
{
    String text("PANIC: unprotected error in call to Lua API (%s)\n");
    Trace(text << lua_tostring(L, -1), true);
    return 0;
}

// _GarbageCollector.__gc and Object.Destroy. (name inferred)
int LuaGarbageCollect(lua_State* L)                                        // Mac 0x2cf400, Win 0x445470
{
    LuaObject object(LuaState::CastState(L), 1);
    DestroyLuaObject(object);
    return 0;
}

// The text of argument 1 for Trace / ReleaseTrace. (helper, inlined in both)
String TraceText(lua_State* L)
{
    if (lua_iswstring(L, 1))
        return String((const UniChar16*)lua_towstring(L, 1));
    const char* text = lua_tostring(L, 1);
    return String(text ? text : "Cannot coerce type for Trace().");
}

// Lua Trace(text). NOTE: the text is built and discarded (debug-only trace in this build).
int LuaTrace(lua_State* L)                                                 // Mac 0x2cf560, Win 0x445220
{
    String text = TraceText(L);
    return 0;
}

// Lua ReleaseTrace(text): traced in every build. (name from the Lua function)
int LuaReleaseTrace(lua_State* L)                                          // Mac 0x2cf5f0, Win 0x4452b0
{
    Trace(TraceText(L), true);
    return 0;
}

// _ERRORREPORTER(message, source, line) called by LuaState::ReportError.
int LuaErrorReporter(lua_State* L)                                         // Mac 0x2cf6d0, Win 0x445370
{
    const char* message = lua_tostring(L, 1);
    const char* source = lua_tostring(L, 2);
    int line = (int)lua_tointeger(L, 3);   // PORT64: lua_Integer is 64-bit there
    String text("<Lua error> source: '");
    text << source << "', line: " << (UInt32)line << ", caused: " << message;
    Trace(text, true);
    return 0;
}

} // namespace

LuaState* CreateLuaState()                                                 // Mac 0x2cf090, Win 0x445590
{
    LuaState* state = LuaState::Create(true);
    if (state) {
        state->AtPanic(LuaPanic);
        LuaObject globals = state->GetGlobals();
        LuaObject registry = state->GetRegistry();
        LuaObject collector = registry.CreateTable("_GarbageCollector", 0, 0);
        collector.Register("__gc", LuaGarbageCollect, 0);
        LuaObject objectMap = registry.CreateTable("objectMap", 0, 0);
        LuaObject weakValues;
        weakValues.AssignNewTable(objectMap.GetState(), 0, 0);
        weakValues.SetString("__mode", "v", -1);
        objectMap.SetMetaTable(weakValues);
        LuaObject object = CreateLuaClass(state, "Object");
        object.Register("Destroy", LuaGarbageCollect, 0);
        state->DoString(kObjectScript);
        globals.Register("Trace", LuaTrace, 0);
        globals.Register("ReleaseTrace", LuaReleaseTrace, 0);
        globals.Register("_ERRORREPORTER", LuaErrorReporter, 0);
    }
    return state;
}

LuaObject CreateLuaClass(LuaState* state, const char* name)                // Mac 0x2cf470, Win 0x4457c0
{
    LuaObject globals = state->GetGlobals();
    LuaObject result = globals.CreateTable(name, 0, 0);
    LuaObject classMetatable = result.CreateTable("classMetatable", 0, 0);
    classMetatable.Set("__index", result);
    return result;
}

LuaObject CreateLuaClass(LuaState* state, const char* name, const char* baseName) // Mac 0x2cf7d0, Win 0x445880
{
    LuaObject globals = state->GetGlobals();                               // NOTE: unused (both builds)
    LuaObject result = CreateLuaClass(state, name);
    SetLuaBaseClass(result, baseName);
    return result;
}

void SetLuaBaseClass(LuaObject& object, const char* baseName)              // Mac 0x2cf860, Win 0x4459c0
{
    LuaObject base = object.GetState()->GetGlobal(baseName);
    LuaObject classMetatable = base.GetByName("classMetatable");
    object.SetMetaTable(classMetatable);
}

LuaObject CreateLuaInstance(LuaState* state, const char* className)       // Mac 0x2cf900, Win 0x445920
{
    LuaObject globals = state->GetGlobals();                               // NOTE: unused (both builds)
    LuaObject result;
    result.AssignNewTable(state, 0, 0);
    SetLuaBaseClass(result, className);
    return result;
}

void AttachLuaObject(LuaObject& table, BindableLuaObject* object)          // Mac 0x2cf9a0, Win 0x445a60
{
    LuaState* state = table.GetState();
    LuaObject registry = state->GetRegistry();
    LuaObject box;
    box.AssignUserData(state, object);
    box.SetMetaTable(registry.GetByName("_GarbageCollector"));
    table.Set("__object", box);
    LuaObject key;
    key.AssignLightUserData(state, object);
    registry.GetByName("objectMap").SetObject(key, table);
    object->AddRef();                                                      // released by DestroyLuaObject
    object->m_luaState = state;
    object->BindLuaObject(table);
}

// (not in the original; H23) True for a box made by AttachLuaObject: a full userdata whose
// metatable is the registry's _GarbageCollector (Lua scripts cannot set a userdata's metatable).
static bool IsAttachedBox(LuaObject& box)
{
    lua_State* L = box.GetCState();
    if (L == 0)
        return false;
    box.Push();
    bool attached = false;
    if (lua_type(L, -1) == LUA_TUSERDATA && lua_getmetatable(L, -1)) {
        lua_getfield(L, LUA_REGISTRYINDEX, "_GarbageCollector");
        attached = lua_rawequal(L, -1, -2) != 0;
        lua_settop(L, -3);
    }
    lua_settop(L, -2);
    return attached;
}

// NOTE: DestroyLuaObject and GetBoundObject take any userdata in "__object" (or the object
// itself) for a BindableLuaObject (both builds): a script that stores another userdata there
// (or passes one) has foreign memory used as a C++ object; fixed: only boxes made by
// AttachLuaObject are taken (H23).
void DestroyLuaObject(LuaObject& object)                                   // Mac 0x2cfc80, Win 0x445bf0
{
    if (object.IsTable()) {
        LuaObject box = object.GetByName("__object");
        if (!box.IsNil() && IsAttachedBox(box)) {
            BindableLuaObject* bound = (BindableLuaObject*)box.GetUserData();
            box.SetMetaTable(LuaObject(object.GetState()));               // no __gc any more
            object.SetNil("__object");
            LuaObject key;
            key.AssignLightUserData(object.GetState(), bound);
            LuaObject registry = object.GetState()->GetRegistry();
            LuaObject objectMap = registry.GetByName("objectMap");
            objectMap.SetNil(key);
            bound->UnbindLuaObject();
            bound->m_luaState = 0;
            bound->Release();
        }
    } else if (object.IsUserData() && IsAttachedBox(object)) {
        BindableLuaObject* bound = (BindableLuaObject*)object.GetUserData();
        bound->UnbindLuaObject();
        bound->m_luaState = 0;
        bound->Release();
    } else {
        (void)object.IsNil();                                              // NOTE: test without effect (both builds)
    }
}

LuaObject GetLuaObject(BindableLuaObject* object)                          // Mac 0x2cfe90, Win 0x445de0
{
    LuaState* state = object->GetLuaState();
    LuaObject key;
    key.AssignLightUserData(state, object);
    LuaObject objectMap = state->GetRegistry().GetByName("objectMap");
    return objectMap.GetByObject(key);
}

BindableLuaObject* GetBoundObject(LuaObject& object)                       // Mac 0x2cff80, Win 0x445ec0
{
    if (object.IsTable()) {
        LuaObject box = object.GetByName("__object");
        return box.IsNil() || !IsAttachedBox(box) ? 0 : (BindableLuaObject*)box.GetUserData();
    }
    if (object.IsUserData() && IsAttachedBox(object))
        return (BindableLuaObject*)object.GetUserData();
    object.IsNil();                                                        // NOTE: test without effect (both builds)
    return 0;
}

void SetObjectMapEntry(LuaObject& table, void* key)                        // Mac 0x2d0030, Win 0x445fb0
{
    lua_State* L = table.GetCState();
    lua_getfield(L, LUA_REGISTRYINDEX, "objectMap");
    lua_pushlightuserdata(L, key);
    table.Push();
    lua_settable(L, -3);
    lua_settop(L, -2);
}

void ClearObjectMapEntry(LuaState* state, void* key)                       // Mac 0x2d00a0, Win 0x446010
{
    lua_State* L = state->GetCState();
    lua_getfield(L, LUA_REGISTRYINDEX, "objectMap");
    lua_pushlightuserdata(L, key);
    lua_pushnil(L);
    lua_settable(L, -3);
    lua_settop(L, -2);
}

LuaObject GetObjectMapEntry(LuaState* state, void* key)                   // Mac 0x2d0100, Win 0x446060
{
    lua_State* L = state->GetCState();
    lua_getfield(L, LUA_REGISTRYINDEX, "objectMap");
    lua_pushlightuserdata(L, key);
    lua_gettable(L, -2);
    LuaObject result(state, -1);
    lua_settop(L, -3);
    return result;
}

LuaObject GetUnboundSelf(LuaState* state)                                  // Mac 0x2d01b0, Win 0x446110
{
    // NOTE (restoration): checked through the C API before the LuaObject exists, so the argument
    // errors do not longjmp over a live LuaObject (see CheckBoundObject); checks and messages are
    // the original's (LuaObject(state, 1).IsTable(), ["__object"]).
    lua_State* L = state->GetCState();
    if (lua_type(L, 1) != LUA_TTABLE)
        luaL_argerror(L, 1, "should be a table");
    lua_getfield(L, 1, "__object");
    bool unbound = lua_isnil(L, -1);
    lua_settop(L, -2);
    if (!unbound)
        luaL_argerror(L, 1, "already have a C++ object bound");
    return LuaObject(state, 1);
}

void LuaTypeError(LuaObject& object, int narg, const char* got, const char* expected) // Mac 0x2d02a0, Win 0x4461e0
{
    RaiseLuaTypeError(object.GetCState(), narg, got, expected);
}

void RaiseLuaTypeError(lua_State* L, int narg, const char* got, const char* expected)
{
    // The original builds the message in an AString (leaked by the longjmp); here Lua owns it.
    luaL_argerror(L, narg, lua_pushfstring(L, "%s expected, got %s", expected,
                                           got ? got : "unbound Lua object"));
}

bool CheckLuaBoolean(lua_State* L, int narg)                               // Mac 0x2d03b0
{
    return luaL_checkboolean(L, narg) != 0;
}

bool CheckLuaBoolean(LuaState* state, int narg)                            // Mac 0x2d03e0
{
    return luaL_checkboolean(state->GetCState(), narg) != 0;
}

bool OptLuaBoolean(lua_State* L, int narg, bool def)                       // Mac 0x2d0410
{
    return luaL_optboolean(L, narg, def) != 0;
}

bool OptLuaBoolean(LuaState* state, int narg, bool def)                    // Mac 0x2d0440
{
    return luaL_optboolean(state->GetCState(), narg, def) != 0;
}

bool LuaToBoolean(LuaState* state, int index)                              // Mac 0x2d0470, Win 0x4462b0
{
    lua_State* L = state->GetCState();
    if (lua_type(L, index) == LUA_TBOOLEAN)
        return lua_toboolean(L, index) != 0;
    return lua_type(L, index) > 0;
}

bool LuaToBoolean(LuaObject& object)                                       // Mac 0x2d04c0, Win 0x4462f0
{
    if (object.IsBoolean())
        return object.GetBoolean();
    if (object.IsNil())
        return false;
    return !object.IsNone();
}

String CheckLuaString(lua_State* L, int narg)                              // Mac 0x2d0500, Win 0x446330
{
    size_t length;
    const lua_WChar* wide = lua_tolwstring(L, narg, &length);
    if (wide)
        return String((const UniChar16*)wide, (UInt32)length);
    const char* bytes = luaL_checklstring(L, narg, &length);
    return DecodeString(kTextEncodingUTF8, bytes, (UInt32)length);
}

String CheckLuaString(LuaState* state, int narg)                           // Mac 0x2d0570, Win 0x4463d0
{
    return CheckLuaString(state->GetCState(), narg);
}

String OptLuaString(lua_State* L, int narg, const String& def)             // Mac 0x2d05e0, Win 0x446440
{
    if (lua_type(L, narg) <= 0)
        return def;
    return CheckLuaString(L, narg);
}

String OptLuaString(LuaState* state, int narg, const String& def)          // Mac 0x2d0670, Win 0x4464c0
{
    return OptLuaString(state->GetCState(), narg, def);
}

String LuaToString(lua_State* L, int index)                                // Mac 0x2d06a0, Win 0x446530
{
    size_t length;
    const lua_WChar* wide = lua_tolwstring(L, index, &length);
    if (wide)
        return String((const UniChar16*)wide, (UInt32)length);
    const char* bytes = lua_tolstring(L, index, &length);
    if (!bytes)
        return String();
    return DecodeString(kTextEncodingUTF8, bytes, (UInt32)length);
}

String LuaToString(LuaObject& object)                                      // Mac 0x2d0720, Win 0x4465d0
{
    if (object.IsString())
        return DecodeString(kTextEncodingUTF8, object.GetString(), (UInt32)object.StrLen());
    if (object.IsWString())
        return String((const UniChar16*)object.GetWString(), (UInt32)object.StrLen());
    if (object.IsNumber()) {
        char text[32];
        sprintf(text, "%.14g", (double)object.GetNumber());
        return String(text);
    }
    return String();
}

void PushLuaString(lua_State* L, const String& s)                          // Mac 0x2d0820, Win 0x4466c0
{
    AString utf8 = s.ToUTF8();
    lua_pushlstring(L, utf8.c_str(), utf8.GetLength());
}

void PushLuaString(LuaState* state, const String& s)                       // Mac 0x2d08d0, Win 0x446740
{
    PushLuaString(state->GetCState(), s);
}

LuaObject LookupLuaObject(LuaObject& object, const String& path)           // Mac 0x2d08f0, Win 0x446760
{
    AString utf8 = path.ToUTF8();
    return object.Lookup(utf8.c_str());
}

bool DefineLuaMethod(LuaObject& table, const char* name, const char* params, const AString& body) // Mac 0x2d0a60, Win 0x446940
{
    AString code("local __obj = ...\n");
    code.Append("function __obj:");
    code.Append(name);
    code.Append("(");
    code.Append(params);
    code.Append(")\n");
    code.Append(body);
    code.Append("\nend");
    LuaState* state = table.GetState();
    // NOTE: the chunk text doubles as the chunk name (both builds).
    if (!state->LoadBuffer(code.c_str(), code.GetLength(), code.c_str()))
        return false;
    table.Push();
    return state->PCall(1, 0);
}

bool DefineLuaMethod(LuaObject& table, const char* name, const char* params, const String& body) // Mac 0x2d0c70, Win 0x446a40
{
    return DefineLuaMethod(table, name, params, body.ToUTF8());
}

bool CallLuaMethod(LuaObject& self, const char* name, bool raw, int results) // Mac 0x2d0d10, Win 0x446ab0
{
    if (!self.IsTable())
        return false;
    LuaObject function = raw ? self.RawGetByName(name) : self.GetByName(name);
    if (function.IsNil())
        return false;
    function.Push();
    self.Push();
    return function.GetState()->PCall(1, results);
}

bool CallLuaMethod(BindableLuaObject* object, const char* name, bool raw, int results) // Mac 0x2d0df0, Win 0x446c20
{
    LuaObject self = GetLuaObject(object);
    return CallLuaMethod(self, name, raw, results);
}

// (added) Win: ICF-folded with other `return 0` functions.
int LuaReturnNothing(lua_State* /*L*/)                                      // Mac 0x2d0e80, Win 0x51e7a0
{
    return 0;
}

} // namespace LEGO

namespace LuaPlus {

LuaCall& operator<<(LuaCall& call, const LEGO::String& s)                  // Mac 0x2d09a0, Win 0x4468b0
{
    LEGO::AString utf8 = s.ToUTF8();
    lua_pushlstring(call.m_state->GetCState(), utf8.c_str(), utf8.GetLength());
    ++call.m_numArgs;
    return call;
}

} // namespace LuaPlus
