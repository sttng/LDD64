// LEGO Digital Designer — restored source
// LEGO::ObjectRegistry — process-wide service objects keyed by a FourCC ('OFAC' ObjectFactory,
// 'STOR' root directory); ObjectRegistryInstantiator creates one on first Get();
// ObjectRegistryObserver is an optional veto/notify interface. TU Mac 0xaed0-0xb37f, Win
// 0x42ce30-0x42d14f. Inline Get/Register/UnregisterAll: Mac 0x9550, 0xa280 (TU 008), 0x2550
// (TU 000); Win 0x4183c0, 0x418530, 0x422290. Usage: ObjectFactory::GetInstance (Mac 0x9470).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicMap.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace LEGO {

class ObjectRegistry;

// Abstract creator passed to ObjectRegistry::Get. Derived classes (ObjectFactoryInstantiator,
// RootDirectoryInstantiator, TextLocaleInstantiator, PreferencesInstantiator, ...) are small
// stack objects holding only the vptr.
// Vtable Mac: {D1, D0, CreateObject} (no vtable of its own emitted: abstract, inline dtor).
// Win 0x9d311c: {scalar_deleting_dtor 0x4183a0, CreateObject = _purecall}.
class ObjectRegistryInstantiator {
public:
    virtual ~ObjectRegistryInstantiator() {}                         // Win 0x4183a0 (sdd)
    // Returns a new, not yet registered object for the requested key. (name inferred)
    // Mac slot +0x8, Win slot +0x4; returns RefPointer<Object> through a hidden sret pointer.
    // NOTE: constness unknown (no symbols); const so temporaries can be passed to Get().
    virtual RefPointer<Object> CreateObject() const = 0;
};

// Interface a registered object may implement (looked up with dynamic_cast).
// Mac: vmi class, virtual base Object; vtable {D1, D0, OnObjectRegistered (+8),
// OnObjectUnregistered (+0xc)}.  Win: own vfptr {OnObjectRegistered, OnObjectUnregistered},
// destructor lives in the Object vtable. No vtable emitted for the class itself.
class ObjectRegistryObserver : public virtual Object {
public:
    // Return false to veto the registration. (name inferred)
    virtual bool OnObjectRegistered(ObjectRegistry* registry, UInt32 id) = 0;
    // Called after the object was removed by UnregisterAll. (name inferred)
    virtual void OnObjectUnregistered(ObjectRegistry* registry, UInt32 id) = 0;
};

// Mac sizeof 0x54, Win sizeof 0x4c.
// Mac vtable 0x75f7ec {D1 0xb080, D0 0xb110, OnFinalRelease 0xb150}; Object-in-ObjectRegistry
// vtable 0x75f810 {virtual thunks 0xb180, 0xb1a0, 0xb1e0, AddRef 0x9260, Release 0x9280}.
// Win: no own vfptr; Object vtable 0xa1fb48 at +0x44 {0x42d410 (vtordisp thunk -> sdd 0x42cf60),
// 0x42d400 (thunk -> OnFinalRelease 0x42cf50), AddRef, Release}.
class ObjectRegistry : public virtual Object {
public:
    typedef BasicMap<UInt32, RefPointer<Object> > ObjectMap;

    ObjectRegistry();                                                 // Mac 0xaf70, Win 0x42ce30
    virtual ~ObjectRegistry();                                        // Mac 0xb080/0xb110, Win 0x42ced0/0x42cf60

    // The shared instance (created on first call, never destroyed before exit). (name inferred)
    static ObjectRegistry* GetInstance();                             // Mac 0xaed0, Win 0x42cf90

    // Returns the object registered under `id`; if there is none, asks `instantiator` to create
    // it and registers it. Returns null when creation or registration fails. (name inferred)
    RefPointer<Object> Get(UInt32 id, const ObjectRegistryInstantiator& instantiator); // Mac 0x9550, Win 0x4183c0

    // The object registered under `id`, or null; never creates one. (name inferred; additive;
    // inlined in StandardTextTooltipAugmentation Mac 0x207de0 / 0x207b00)
    RefPointer<Object> Find(UInt32 id);                               // Win 0x5139e8

    // Registers `object` under `id`. Fails when the id is taken or the object (if it is an
    // ObjectRegistryObserver) vetoes. (name inferred)
    bool Register(UInt32 id, Object* object);                         // Mac 0xa280, Win 0x418530

    // Removes every object, notifying observers. (name inferred)
    void UnregisterAll();                                             // Mac 0x2550, Win 0x422290

protected:
    // Clears the registry before the last reference goes away.
    virtual void OnFinalRelease();                                    // Mac 0xb150, Win 0x42cf50

private:
    ObjectRegistry(const ObjectRegistry&);
    ObjectRegistry& operator=(const ObjectRegistry&);

    // Embedded recursive mutex with inline ctor/dtor (name inferred). It is a member object:
    // both builds destroy it *after* m_objects (Mac dtor 0xb080: map erase, then
    // pthread_mutex_destroy; Win 0x42ced0: map dtor, then DeleteCriticalSection), and the Win
    // ctor opens a new EH state after InitializeCriticalSection.
    class RecursiveMutex {
    public:
        RecursiveMutex()
        {
#if defined(_WIN32)
            InitializeCriticalSection(&m_cs);
#else
            pthread_mutexattr_t attr;
            pthread_mutexattr_init(&attr);
            pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
            pthread_mutex_init(&m_mutex, &attr);
            pthread_mutexattr_destroy(&attr);
#endif
        }
        ~RecursiveMutex()
        {
#if defined(_WIN32)
            DeleteCriticalSection(&m_cs);
#else
            pthread_mutex_destroy(&m_mutex);
#endif
        }
        void Lock()
        {
#if defined(_WIN32)
            EnterCriticalSection(&m_cs);
#else
            pthread_mutex_lock(&m_mutex);
#endif
        }
        void Unlock()
        {
#if defined(_WIN32)
            LeaveCriticalSection(&m_cs);
#else
            pthread_mutex_unlock(&m_mutex);
#endif
        }
    private:
        RecursiveMutex(const RecursiveMutex&);
        RecursiveMutex& operator=(const RecursiveMutex&);
#if defined(_WIN32)
        CRITICAL_SECTION m_cs;      // 0x18 bytes
#else
        pthread_mutex_t  m_mutex;   // 0x2c bytes
#endif
    };

    class ScopedLock {                                                // (name inferred)
    public:
        explicit ScopedLock(RecursiveMutex& m) : m_mutex(m) { m_mutex.Lock(); }
        ~ScopedLock() { m_mutex.Unlock(); }
    private:
        ScopedLock(const ScopedLock&);
        ScopedLock& operator=(const ScopedLock&);
        RecursiveMutex& m_mutex;
    };

    // Layout (Mac / Win):  vptr +0x00 / vbptr +0x00
    RecursiveMutex m_lock;        // Mac +0x04 (0x2c), Win +0x04 (0x18)
    ObjectMap      m_objects;     // Mac +0x30, Win +0x1c
    // Mac: virtual base Object at +0x4c.  Win: vtordisp +0x40, virtual base Object at +0x44.
};

// ---------------------------------------------------------------------------
// Inline members (weak/COMDAT in the original)
// ---------------------------------------------------------------------------

// Win 0x5139e8 (inlined on the Mac)
inline RefPointer<Object> ObjectRegistry::Find(UInt32 id)
{
    ScopedLock lock(m_lock);
    ObjectMap::Iterator it = m_objects.Find(id);
    if (it != m_objects.End())
        return it->second;
    return RefPointer<Object>();
}

// Mac 0x9550, Win 0x4183c0
inline RefPointer<Object> ObjectRegistry::Get(UInt32 id, const ObjectRegistryInstantiator& instantiator)
{
    ScopedLock lock(m_lock);
    ObjectMap::Iterator it = m_objects.Find(id);
    if (it != m_objects.End())
        return it->second;

    RefPointer<Object> object = instantiator.CreateObject();
    if (!object.IsNull() && !Register(id, object))
        object = 0;
    return object;
}

// Mac 0xa280, Win 0x418530
inline bool ObjectRegistry::Register(UInt32 id, Object* object)
{
    ScopedLock lock(m_lock);
    if (m_objects.Find(id) != m_objects.End())
        return false;

    ObjectRegistryObserver* observer = dynamic_cast<ObjectRegistryObserver*>(object);
    if (observer && !observer->OnObjectRegistered(this, id))
        return false;

    m_objects.Insert(id, RefPointer<Object>(object));
    return true;
}

// Mac 0x2550 (emitted in TU 000), Win 0x422290
inline void ObjectRegistry::UnregisterAll()
{
    ScopedLock lock(m_lock);
    while (!m_objects.IsEmpty()) {
        ObjectMap::Iterator it = m_objects.Begin();
        RefPointer<Object> object = it->second;
        UInt32 id = it->first;
        m_objects.Erase(it);
        if (!object.IsNull()) {
            ObjectRegistryObserver* observer = dynamic_cast<ObjectRegistryObserver*>(object.Get());
            if (observer)
                observer->OnObjectUnregistered(this, id);
        }
    }
}

} // namespace LEGO
