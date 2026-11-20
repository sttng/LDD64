// LEGO Digital Designer — restored source
// The LEGO Lua glue: the shared LuaPlus state with the "Object" class system, binding of C++
// objects (BindableLuaObject) to Lua tables, and the argument / call helpers of every binding.
// TU 323 "LuaBindings.cpp" (names inferred; Lua-side names are original strings): Mac
// 0x2cf090-0x2d0ecf, Win 0x445370-0x446e5f.
//
// Lua object model (CreateLuaState):
//  * A class is a global table `Name` with `classMetatable` {__index = Name}; a derived class
//    has the base's classMetatable as metatable (lookups walk Name -> Base -> … -> Object).
//    Instances are tables with the class's classMetatable (Object:New / CreateLuaInstance).
//  * AttachLuaObject binds a C++ object: table.__object = boxed userdata (metatable
//    registry._GarbageCollector, __gc = DestroyLuaObject), registry.objectMap[object] = table
//    (weak values), one reference held, m_luaState set, BindLuaObject(table) called.
//  * Object:Destroy() (= DestroyLuaObject) or __gc unbinds: UnbindLuaObject(), m_luaState = 0,
//    the reference released.
// Bindings: RegisterLuaFoo creates the class (CreateLuaClass) and registers a luaL_Reg table;
// Foo.Create(self) does AttachLuaObject(GetUnboundSelf(state), new LuaFoo); methods fetch self
// with CheckBoundObject<LuaFoo>(state, 1) (e.g. LuaPreferencesObserver.cpp). C++ -> Lua:
// CallLuaMethod(object, "OnSomething", raw, results, args…).
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "BasicString.h"
#include "BindableLuaObject.h"
#include "LuaPlus.h"

#include <typeinfo>

namespace LEGO {

// --- state and class system -------------------------------------------------------------------

// LuaState::Create(true) + panic handler, registry._GarbageCollector {__gc}, registry.objectMap
// (weak values), the Object class (Object.Destroy, Object:New/Create/FindMember/CallMethod in
// Lua), globals Trace / ReleaseTrace / _ERRORREPORTER. Null when the state cannot be created.
LuaPlus::LuaState* CreateLuaState();                                               // Mac 0x2cf090, Win 0x445590

// New global class table `name` with classMetatable = {__index = class}.
LuaPlus::LuaObject CreateLuaClass(LuaPlus::LuaState* state, const char* name);    // Mac 0x2cf470, Win 0x4457c0
// The same, derived from the global class `baseName`.
LuaPlus::LuaObject CreateLuaClass(LuaPlus::LuaState* state, const char* name,
                                  const char* baseName);                           // Mac 0x2cf7d0, Win 0x445880
// metatable(object) = global[baseName].classMetatable.
void SetLuaBaseClass(LuaPlus::LuaObject& object, const char* baseName);           // Mac 0x2cf860, Win 0x4459c0
// A new (empty) instance table of the global class `className`.
LuaPlus::LuaObject CreateLuaInstance(LuaPlus::LuaState* state, const char* className); // Mac 0x2cf900, Win 0x445920

// --- binding C++ objects ----------------------------------------------------------------------

// Binds `object` to the instance table (see the object model above).
void AttachLuaObject(LuaPlus::LuaObject& table, BindableLuaObject* object);       // Mac 0x2cf9a0, Win 0x445a60
// Object:Destroy() / __gc of the bound userdata: unbinds the C++ object of a table or userdata.
void DestroyLuaObject(LuaPlus::LuaObject& object);                                 // Mac 0x2cfc80, Win 0x445bf0
// The instance table bound to `object` (objectMap lookup; nil when unbound/collected).
LuaPlus::LuaObject GetLuaObject(BindableLuaObject* object);                        // Mac 0x2cfe90, Win 0x445de0
// The C++ object bound to a table (its __object) or held by a userdata; null otherwise.
BindableLuaObject* GetBoundObject(LuaPlus::LuaObject& object);                     // Mac 0x2cff80, Win 0x445ec0

// objectMap entries keyed by an arbitrary pointer (AdapterBindableLuaObjectBase<T, B>
// Bind/UnbindLuaObject key the table by the adapted object).
void SetObjectMapEntry(LuaPlus::LuaObject& table, void* key);                     // Mac 0x2d0030, Win 0x445fb0
void ClearObjectMapEntry(LuaPlus::LuaState* state, void* key);                    // Mac 0x2d00a0, Win 0x446010
LuaPlus::LuaObject GetObjectMapEntry(LuaPlus::LuaState* state, void* key);       // Mac 0x2d0100, Win 0x446060

// Argument 1 of a Lua "Create" function: must be a table without a bound object (raises a Lua
// argument error otherwise: "should be a table" / "already have a C++ object bound").
LuaPlus::LuaObject GetUnboundSelf(LuaPlus::LuaState* state);                      // Mac 0x2d01b0, Win 0x446110
// Raises "<expected> expected, got <got | unbound Lua object>" for argument `narg`.
void LuaTypeError(LuaPlus::LuaObject& object, int narg, const char* got,
                  const char* expected);                                           // Mac 0x2d02a0, Win 0x4461e0
// The same for a plain lua_State (restoration helper, see CheckBoundObject).
void RaiseLuaTypeError(lua_State* L, int narg, const char* got, const char* expected);

// The bound object of argument `index` as a T (dynamic_cast), or a Lua type error naming the
// RTTI names of T and of the bound object. Inline in every binding (e.g. Mac 0x2b5c00
// LuaCommand, 0x38b9c0 LuaPreferencesObserver; Win 0x46b6c0).
// NOTE (restoration): Lua errors leave C++ frames by longjmp, skipping destructors, so a live
// LuaObject would stay in the state's object list (the Win original's MSVC longjmp unwinds, the
// restored builds do not). Checks therefore raise only after their LuaObjects went out of scope;
// bindings should do the same (no LuaObject alive across a luaL_* check / error).
template <class T>
T* CheckBoundObject(LuaPlus::LuaState* state, int index)
{
    T* result;
    const char* got = 0;
    {
        LuaPlus::LuaObject object(state, index);
        BindableLuaObject* bound = GetBoundObject(object);
        result = bound ? dynamic_cast<T*>(bound) : 0;
        if (!result) {
            BindableLuaObject* other = GetBoundObject(object);
            got = other ? typeid(*other).name() : 0;
        }
    }
    if (!result)
        RaiseLuaTypeError(state->GetCState(), index, got, typeid(T).name());
    return result;
}

// --- arguments and values ---------------------------------------------------------------------

bool   CheckLuaBoolean(lua_State* L, int narg);                                    // Mac 0x2d03b0
bool   CheckLuaBoolean(LuaPlus::LuaState* state, int narg);                         // Mac 0x2d03e0
bool   OptLuaBoolean(lua_State* L, int narg, bool def);                            // Mac 0x2d0410
bool   OptLuaBoolean(LuaPlus::LuaState* state, int narg, bool def);                 // Mac 0x2d0440
// A boolean as is, anything else: true unless nil/none.
bool   LuaToBoolean(LuaPlus::LuaState* state, int index);                          // Mac 0x2d0470, Win 0x4462b0
bool   LuaToBoolean(LuaPlus::LuaObject& object);                                   // Mac 0x2d04c0, Win 0x4462f0
// Wide strings (and numbers, which lua_tolwstring converts in place) as UTF-16, byte strings
// decoded as UTF-8; luaL_checklstring raises the error for other types.
String CheckLuaString(lua_State* L, int narg);                                     // Mac 0x2d0500, Win 0x446330
String CheckLuaString(LuaPlus::LuaState* state, int narg);                          // Mac 0x2d0570, Win 0x4463d0
String OptLuaString(lua_State* L, int narg, const String& def);                    // Mac 0x2d05e0, Win 0x446440
String OptLuaString(LuaPlus::LuaState* state, int narg, const String& def);         // Mac 0x2d0670, Win 0x4464c0
// Like CheckLuaString without the error: an empty String for other types.
String LuaToString(lua_State* L, int index);                                       // Mac 0x2d06a0, Win 0x446530
// Strings, wide strings, numbers ("%.14g"); an empty String otherwise.
String LuaToString(LuaPlus::LuaObject& object);                                    // Mac 0x2d0720, Win 0x4465d0
// Pushes the UTF-8 encoding of `s`.
void   PushLuaString(lua_State* L, const String& s);                               // Mac 0x2d0820, Win 0x4466c0
void   PushLuaString(LuaPlus::LuaState* state, const String& s);                    // Mac 0x2d08d0, Win 0x446740
// object.Lookup(UTF-8 of `path`) ("a.b.c"; a leading '.' starts at the globals).
LuaPlus::LuaObject LookupLuaObject(LuaPlus::LuaObject& object, const String& path); // Mac 0x2d08f0, Win 0x446760

// --- calls ------------------------------------------------------------------------------------

// Defines `function table:name(params) <body> end` by running a chunk with the table as `...`.
bool DefineLuaMethod(LuaPlus::LuaObject& table, const char* name, const char* params,
                     const AString& body);                                         // Mac 0x2d0a60, Win 0x446940
bool DefineLuaMethod(LuaPlus::LuaObject& table, const char* name, const char* params,
                     const String& body);                                          // Mac 0x2d0c70, Win 0x446a40

// self:name() when self is a table and the function exists (`raw`: RawGetByName in the instance
// table only, else GetByName through the class chain / __index). Errors go to _ERRORREPORTER.
// True when the call succeeded.
// Flag: Mac 0x2d0d10: true -> 0x3e0800 = RawGetByName (operator[] -> luaH_get),
// false -> 0x3e04b6 = GetByName (luaV_gettable); Win 0x446ab0: false -> 0x5d0350 (GetByName
// inline), true -> 0x5d04f0. Every caller passes false (class callbacks such as
// ShowControlCommand:Do are found).
bool CallLuaMethod(LuaPlus::LuaObject& self, const char* name, bool raw, int results); // Mac 0x2d0d10, Win 0x446ab0
bool CallLuaMethod(BindableLuaObject* object, const char* name, bool raw, int results); // Mac 0x2d0df0, Win 0x446c20

// lua_CFunction returning nothing: registered as "New" and "Create" of the adapter classes
// (BrickDatabase, LDDApplication, SceneManager …) so Lua cannot create them. (name inferred)
int LuaReturnNothing(lua_State* L);                                                 // Mac 0x2d0e80, Win 0x51e7a0

} // namespace LEGO

namespace LuaPlus {
// Pushes the UTF-8 encoding of `s` as the next argument.
LuaCall& operator<<(LuaCall& call, const LEGO::String& s);                         // Mac 0x2d09a0, Win 0x4468b0
// (additive) A LuaObject argument of the CallLuaMethod templates (they take
// `const A&`; LuaPlus only has operator<<(LuaCall&, LuaObject&), which does not change it).
inline LuaCall& operator<<(LuaCall& call, const LuaObject& value)
{
    return call << const_cast<LuaObject&>(value);
}
}

namespace LEGO {

// self:name(a1 …) — header templates instantiated by the bindings (String argument: Mac
// 0x383b10 / BindableLuaObject* form 0x383740, Win 0x46b730).
template <class A1>
bool CallLuaMethod(LuaPlus::LuaObject& self, const char* name, bool raw, int results,
                   const A1& a1)
{
    if (!self.IsTable())
        return false;
    LuaPlus::LuaObject function = raw ? self.RawGetByName(name) : self.GetByName(name);
    if (function.IsNil())
        return false;
    LuaPlus::LuaCall call(function);
    call << self << a1;
    return call.m_state->PCall(call.m_numArgs, results);
}

template <class A1, class A2>
bool CallLuaMethod(LuaPlus::LuaObject& self, const char* name, bool raw, int results,
                   const A1& a1, const A2& a2)
{
    if (!self.IsTable())
        return false;
    LuaPlus::LuaObject function = raw ? self.RawGetByName(name) : self.GetByName(name);
    if (function.IsNil())
        return false;
    LuaPlus::LuaCall call(function);
    call << self << a1 << a2;
    return call.m_state->PCall(call.m_numArgs, results);
}

template <class A1, class A2, class A3>
bool CallLuaMethod(LuaPlus::LuaObject& self, const char* name, bool raw, int results,
                   const A1& a1, const A2& a2, const A3& a3)
{
    if (!self.IsTable())
        return false;
    LuaPlus::LuaObject function = raw ? self.RawGetByName(name) : self.GetByName(name);
    if (function.IsNil())
        return false;
    LuaPlus::LuaCall call(function);
    call << self << a1 << a2 << a3;
    return call.m_state->PCall(call.m_numArgs, results);
}

template <class A1>
bool CallLuaMethod(BindableLuaObject* object, const char* name, bool raw, int results,
                   const A1& a1)
{
    LuaPlus::LuaObject self = GetLuaObject(object);
    A1 copy(a1);                                       // (Mac 0x383740 copies a String argument)
    return CallLuaMethod(self, name, raw, results, copy);
}

template <class A1, class A2>
bool CallLuaMethod(BindableLuaObject* object, const char* name, bool raw, int results,
                   const A1& a1, const A2& a2)
{
    LuaPlus::LuaObject self = GetLuaObject(object);
    return CallLuaMethod(self, name, raw, results, a1, a2);
}

template <class A1, class A2, class A3>
bool CallLuaMethod(BindableLuaObject* object, const char* name, bool raw, int results,
                   const A1& a1, const A2& a2, const A3& a3)
{
    LuaPlus::LuaObject self = GetLuaObject(object);
    return CallLuaMethod(self, name, raw, results, a1, a2, a3);
}

} // namespace LEGO
