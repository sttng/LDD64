// LEGO Digital Designer — restored source (header only)
// LEGO::BindableLuaObject — base of every C++ object with a Lua twin (LuaControl, LuaCommand,
// Lua*Observer, AdapterBindableLuaObject<T> …): holds the LuaPlus state; the object's Lua table
// is registered in the registry table "objectMap" (Lua glue TU 323, Mac 0x2d0030 / 0x2d00a0).
// Home: TU 323 "LuaBindings.cpp" (Mac 0x2cf090-0x2d0ecf, Win ≈ 0x445370-0x446c20) + inline
// bodies (Mac 0x291a70-0x291b00); DCL/Source/Lua/ because the Win addresses lie in the DCL range
// (0x448dd0). File, method and member names inferred.
//
// RTTI vmi, virtual base Object. Mac vtable 0x73f41c {D1 0x291a80, D0 0x291aa0, BindLuaObject
// 0x291a70 (empty), UnbindLuaObject pure}; Object-in {0x291ae0, 0x291b00, OnFinalRelease,
// AddRef, Release}. Layout vptr, LuaPlus::LuaState* +0x04, Object +0x08.
// Win vtable 0x9ce2bc-0x9ce2d0: primary {BindLuaObject 0x40be80 (empty, ret 4), _purecall},
// Object-in {sdd 0x448dd0, …}; vfptr, vbptr +4, state +8, Object +0xc.
// Implemented by AdapterBindableLuaObjectBase<T, B> (Mac 0x367f80 objectMap[object] = table,
// 0x367fa0 objectMap[object] = nil).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"

namespace LuaPlus {
class LuaState;
class LuaObject;
}

namespace LEGO {

class BindableLuaObject;
void AttachLuaObject(LuaPlus::LuaObject& table, BindableLuaObject* object);   // LuaBindings.h
void DestroyLuaObject(LuaPlus::LuaObject& object);                            // LuaBindings.h

class BindableLuaObject : public virtual Object {
public:
    BindableLuaObject() : m_luaState(0) {}
    virtual ~BindableLuaObject() {}                                         // Mac 0x291a80/0x291aa0, Win sdd 0x448dd0

    // Called with the object's Lua table when the Lua side is created.
    virtual void BindLuaObject(LuaPlus::LuaObject& /*table*/) {}            // Mac slot 2 0x291a70, Win slot 0 0x40be80
    // Forgets the Lua table of the object.
    virtual void UnbindLuaObject() = 0;                                     // Mac slot 3, Win slot 1

    LuaPlus::LuaState* GetLuaState() const { return m_luaState; }

protected:
    // Set by AttachLuaObject, cleared by DestroyLuaObject (TU 323).
    friend void AttachLuaObject(LuaPlus::LuaObject& table, BindableLuaObject* object);
    friend void DestroyLuaObject(LuaPlus::LuaObject& object);
    LuaPlus::LuaState* m_luaState;   // Mac +0x04, Win +0x08
};

} // namespace LEGO
