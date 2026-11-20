// LEGO Digital Designer — restored source
// LEGO::ObjectFactory (creates objects by std::type_info), SpecificObjectFactory (one concrete
// type), AbstractObjectFactory (the specific factories of an abstract type),
// ObjectFactoryInstantiator, TypeInfoLess, ObjectFactoryArgs. Registry API (ObjectRegistry::Get
// Mac 0x9550): ObjectRegistry.h. TU "ObjectFactoryInstantiator.cpp" Mac 0x9470-0xaecf, Win
// 0x41c940-0x41d4ff.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicMap.h"
#include "ObjectRegistry.h"
#include <typeinfo>

namespace LEGO {

// Orders type_info pointers (libstdc++/Darwin: by name pointer, inlined; MSVC: type_info::before).
struct TypeInfoLess {
    bool operator()(const std::type_info* a, const std::type_info* b) const
    {
        return a->before(*b) != 0;
    }
};

// Base of the argument blocks passed to factories; concrete factories dynamic_cast it to their
// own args struct (e.g. MemoryStorageFactory). RTTI: struct (.?AU). Win vtable 0x9c9328 {0x424ba0}.
struct ObjectFactoryArgs {
    virtual ~ObjectFactoryArgs() {}                                   // Win 0x424ba0 (sdd)
};

// Abstract, header-only (Mac vtable/dtors emitted first in TU 014: D2 0x18f40, D1 0x18fa0,
// D0 0x19000). sizeof 0x10.
// Mac vtable 0x75fa90 {D1, D0, OnFinalRelease, AddRef, Release, CreateObject(pure),
// Initialize 0x19070, Shutdown 0x19080}.
// Win vtable 0x9c9670 {sdd 0x42e7c0, OnFinalRelease, AddRef, Release, _purecall, 0x4f5b70, 0x459790}.
class SpecificObjectFactory : public Object {
public:
    SpecificObjectFactory() : m_registrationCount(0) {}
    virtual ~SpecificObjectFactory() {}                               // Mac 0x18fa0/0x19000, Win 0x42e7c0

    // Creates a new object (not AddRef'd); null if `args` are not understood. (name inferred)
    virtual Object* CreateObject(const ObjectFactoryArgs& args) = 0;  // Mac +0x14, Win +0x10
    // Called on the first registration; false aborts it. (name inferred)
    virtual bool Initialize() { return true; }                        // Mac 0x19070, Win 0x4f5b70
    // (name inferred) — never called from this TU.
    virtual void Shutdown() {}                                        // Mac 0x19080, Win 0x459790

    // Counts one registration with an ObjectFactory; the first one adopts the factory's mutex
    // and calls Initialize(). Inlined on Mac (0x9810, 0x99d0), Win COMDAT 0x41d510.
    // (name inferred)
    bool AddRegistration(Mutex* mutex)
    {
        if (m_registrationCount == 0) {
            m_mutex = mutex;
            if (!Initialize()) {
                m_mutex = 0;
                return false;
            }
        }
        ++m_registrationCount;
        return true;
    }

    Mutex*  GetMutex() const          { return m_mutex.Get(); }      // (name inferred)
    UInt32  GetRegistrationCount() const { return m_registrationCount; } // (name inferred)

protected:
    RefPointer<Mutex> m_mutex;              // +0x08 shared with the owning ObjectFactory
    UInt32            m_registrationCount;  // +0x0c
};

typedef BasicMap<const std::type_info*, RefPointer<SpecificObjectFactory>, TypeInfoLess> SpecificObjectFactoryMap; // (name inferred)

// Mac sizeof 0x28, Win 0x30. Mac vtable 0x703c30 {D1 0x9e30, D0 0x9e40, OnFinalRelease, AddRef,
// Release, CreateObject 0x9d60}. Win vtable 0xa1f57c {sdd 0x41cbd0, .., 0x41d420}.
class AbstractObjectFactory : public Object {
public:
    AbstractObjectFactory() {}                                        // inlined Mac 0x99d0, Win 0x41cac0
    virtual ~AbstractObjectFactory();                                 // Mac 0x9e30 (D1) / 0xa520 (D2) / 0x9e40 (D0), Win 0x41cb40/0x41cbd0

    // Uses the factory registered for exactly `type`; otherwise the first registered factory
    // that produces an object. (name inferred)
    virtual Object* CreateObject(const std::type_info& type, const ObjectFactoryArgs& args); // Mac 0x9d60, Win 0x41d420

    void SetFactory(const std::type_info& type, SpecificObjectFactory* factory) // (name inferred; inlined)
    {
        m_factories[&type] = factory;
    }

private:
    RefPointer<Mutex>        m_mutex;       // +0x08 (never set in the restored code; released in dtor)
    SpecificObjectFactoryMap m_factories;   // +0x0c
};

// Mac sizeof 0x44, Win 0x54. Mac vtable 0x763938 {D1 0x9fe0, D0 0x9ff0, OnFinalRelease, AddRef,
// Release}. Win vtable 0xa1f568 {sdd 0x41caa0, ...}.
class ObjectFactory : public Object {
public:
    // ObjectRegistry key of the shared instance.
    enum { kRegistryId = 0x4f464143 };                                // 'OFAC' (name inferred)

    ObjectFactory();                                                  // Mac 0x9ee0, Win 0x41c940
    virtual ~ObjectFactory();                                         // Mac 0x9fe0 (D1) / 0xa1c0 (D2) / 0x9ff0 (D0), Win 0x41ca00/0x41caa0

    // The shared factory, held by the ObjectRegistry under 'OFAC'. (name inferred)
    static RefPointer<ObjectFactory> GetInstance();                   // Mac 0x9470, Win 0x41cd50

    // Creates an object of (or derived from) `type`; null when no factory can. (name inferred)
    Object* CreateObject(const std::type_info& type, const ObjectFactoryArgs& args); // Mac 0x96b0, Win 0x41ce30

    template <class T>
    T* Create(const ObjectFactoryArgs& args)                          // convenience (not in binaries)
    {
        return dynamic_cast<T*>(CreateObject(typeid(T), args));
    }

    // Registers `factory` for the concrete `type`. Fails if `type` is already known. (name inferred)
    bool RegisterFactory(const std::type_info& type, SpecificObjectFactory* factory); // Mac 0x9810, Win 0x41cfd0

    // Registers `factory` for `type` and also makes it reachable through `abstractType`.
    // (name inferred)
    bool RegisterFactory(const std::type_info& type, SpecificObjectFactory* factory,
                         const std::type_info& abstractType);         // Mac 0x99d0, Win 0x41d180

private:
    ObjectFactory(const ObjectFactory&);
    ObjectFactory& operator=(const ObjectFactory&);

    typedef BasicMap<const std::type_info*, RefPointer<AbstractObjectFactory>, TypeInfoLess> AbstractFactoryMap;

    RefPointer<Mutex>        m_mutex;              // +0x08 (Mac & Win)
    AbstractFactoryMap       m_abstractFactories;  // +0x0c  (Mac 0x1c bytes, Win 0x24)
    SpecificObjectFactoryMap m_specificFactories;  // Mac +0x28, Win +0x30
};

// Creates the shared ObjectFactory for ObjectRegistry::Get('OFAC', ...).
// Mac vtable 0x75f778 {D1 0x96a0, D0 0x9e80, CreateObject 0x9e90}.
// Win vtable 0xa1f5a4 {sdd 0x41cce0, CreateObject 0x41cc40}.
class ObjectFactoryInstantiator : public ObjectRegistryInstantiator {
public:
    ObjectFactoryInstantiator() {}
    virtual ~ObjectFactoryInstantiator();                             // Mac 0x96a0/0x9e80, Win 0x41cbf0/0x41cce0
    virtual RefPointer<Object> CreateObject() const;                  // Mac 0x9e90, Win 0x41cc40
};

} // namespace LEGO
