// LEGO Digital Designer — restored source (shared by the 3.2-3.5 bindings)
// AdapterBindableLuaObjectBase<T, B> / AdapterBindableLuaObject<T>: a BindableLuaObject (or
// LuaLayout, for LuaWindow) that gives a ref-counted C++ object T a Lua twin; objectMap is keyed
// by the adapted object. Header-only, emitted per TU (e.g. <BrickDatabase> Mac 0x3720c0-0x372270,
// TU 383); Win ICF-folds BindLuaObject Win 0x64bf4e, UnbindLuaObject Win 0x604a66. Names inferred.
//
// RTTI: Base<T, B> si B; AdapterBindableLuaObject<T> si Base<T, BindableLuaObject> (no own
// virtuals; Mac emits no own vtable, Win has its own vftable / ctor / dtor).
// Layout (B = BindableLuaObject): Mac vptr, state +0x04, T* +0x08, Object +0x0c (sizeof 0x14);
//   Win vfptr, vbptr +4, state +8, T* +0x0c, Object +0x10 (sizeof 0x18).
// Layout (B = LuaLayout): Mac T* +0x0c, Object +0x10; Win T* +0x10, Object +0x14.
#pragma once
#include "LEGO/Types.h"
#include "BindableLuaObject.h"
#include "LuaBindings.h"
#include "LuaPlus.h"

#include <typeinfo>

namespace LEGO {

template <class T, class B>
class AdapterBindableLuaObjectBase : public B {
public:
    typedef T ObjectType;

    // NOTE: AddRef is unconditional (Mac 0x371d30 inlined ctor; Win 0x604afe -> 0x4921d0).
    explicit AdapterBindableLuaObjectBase(T* object) : m_object(object) { m_object->AddRef(); }
    // Mac D1/D0 e.g. 0x372100/0x372170 (<BrickDatabase>), Win sdd e.g. 0x604acc
    virtual ~AdapterBindableLuaObjectBase() { if (m_object) m_object->Release(); }

    // objectMap[m_object] = table. Mac e.g. 0x3720c0, Win 0x64bf4e (shared)
    virtual void BindLuaObject(LuaPlus::LuaObject& table) { SetObjectMapEntry(table, m_object); }
    // objectMap[m_object] = nil. Mac e.g. 0x3720e0, Win 0x604a66 (shared)
    virtual void UnbindLuaObject() { ClearObjectMapEntry(this->m_luaState, m_object); }

    T* GetAdaptedObject() const { return m_object; }

    // The adapted object of a bound table/userdata; null when not bound to this adapter class (or
    // no object held). Win e.g. 0x604b6b (<BrickDatabase>).
    static T* GetAdaptedObject(LuaPlus::LuaObject& object)
    {
        BindableLuaObject* bound = GetBoundObject(object);
        AdapterBindableLuaObjectBase* adapter =
            bound ? dynamic_cast<AdapterBindableLuaObjectBase*>(bound) : 0;
        return adapter ? adapter->m_object : 0;
    }

    // The adapted object of argument `narg`, or a Lua type error "<T> expected, got <bound class |
    // unbound Lua object>" (RTTI names). Win e.g. 0x604c37 (+ 0x604d04 caller form); inlined on Mac
    // (e.g. 0x371c00). NOTE (restoration): raises only after the LuaObject went out of scope (see
    // CheckBoundObject in LuaBindings.h).
    static T* CheckAdaptedObject(LuaPlus::LuaState* state, int narg)
    {
        T* result;
        const char* got = 0;
        {
            LuaPlus::LuaObject object(state, narg);
            result = GetAdaptedObject(object);
            if (!result) {
                BindableLuaObject* other = GetBoundObject(object);
                got = other ? typeid(*other).name() : 0;
            }
        }
        if (!result)
            RaiseLuaTypeError(state->GetCState(), narg, got, typeid(T).name());
        return result;
    }

protected:
    T* m_object;   // Mac +0x08 / Win +0x0c (B = BindableLuaObject); one reference held
};

template <class T>
class AdapterBindableLuaObject : public AdapterBindableLuaObjectBase<T, BindableLuaObject> {
public:
    // Win e.g. 0x604bb3 (<BrickDatabase>), dtor 0x604a99
    explicit AdapterBindableLuaObject(T* object)
        : AdapterBindableLuaObjectBase<T, BindableLuaObject>(object) {}
    virtual ~AdapterBindableLuaObject() {}
};

// As CheckAdaptedObject, for adapter class A (the Lua class, e.g. LuaBrickDatabase, LuaTool; the
// original dynamic_casts to A); error names A::ObjectType. Inlined or emitted per TU: Mac e.g.
// 0x382f60 <LuaLDDApplication>, 0x292d40 <LuaSceneManager>, inline in 0x371c00
// <LuaBrickDatabase>; Win e.g. 0x604c37 / 0x606465.
// NOTE (restoration): raises only after the LuaObject went out of scope (see CheckBoundObject).
template <class A>
typename A::ObjectType* CheckAdaptedLuaObject(LuaPlus::LuaState* state, int narg)
{
    typename A::ObjectType* result = 0;
    const char* got = 0;
    {
        LuaPlus::LuaObject object(state, narg);
        BindableLuaObject* bound = GetBoundObject(object);
        A* adapter = bound ? dynamic_cast<A*>(bound) : 0;
        if (adapter)
            result = adapter->GetAdaptedObject();
        if (!result) {
            BindableLuaObject* other = GetBoundObject(object);
            got = other ? typeid(*other).name() : 0;
        }
    }
    if (!result)
        RaiseLuaTypeError(state->GetCState(), narg, got, typeid(typename A::ObjectType).name());
    return result;
}

// The same, but nil (or none) gives `def`. Mac e.g. 0x292e10 <LuaTool>.
template <class A>
typename A::ObjectType* OptAdaptedLuaObject(LuaPlus::LuaState* state, int narg,
                                            typename A::ObjectType* def)
{
    bool isNil;
    {
        LuaPlus::LuaObject object(state, narg);
        isNil = object.IsNil();
    }
    return isNil ? def : CheckAdaptedLuaObject<A>(state, narg);
}

} // namespace LEGO
