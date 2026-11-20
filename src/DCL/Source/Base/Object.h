// LEGO Digital Designer — restored source
// LEGO::Object — reference-counted root class; LEGO::RefPointer smart pointer;
// LEGO::Mutex — shared recursive mutex (name inferred).
//
// TU "Object.cpp": Mac 0x91c0-0x946f, Win 0x4124e0.. (Object), 0x42e740.. (Mutex release, inlined)
#pragma once
#include "LEGO/Types.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace LEGO {

// sizeof == 8. Vtable (Mac): D1 0x9220, D0 0x9240, OnFinalRelease 0x9250,
// AddRef 0x9260, Release 0x9280.  Win: 0x4124f0 0x459790 0x428d60 0x412520.
class Object {
public:
    Object() : m_refCount(0) {}                                       // Mac 0x91c0/0x91e0  Win 0x4124e0
    Object(const Object&) : m_refCount(0) {}
    Object& operator=(const Object&) { return *this; }
    virtual ~Object() {}                                              // Mac 0x9220

    virtual void OnFinalRelease() {}                                  // Mac 0x9250 (name inferred)
    // const: the original instantiates RefPointer<const T> (Win RTTI e.g.
    // BasicArray<RefPointer<const StorageScene2::Brick>>, RefPointer<const AbstractBrick>).
    virtual void AddRef() const;                                      // Mac 0x9260
    virtual void Release() const;                                     // Mac 0x9280

    Int32 GetRefCount() const { return m_refCount; }

private:
    mutable volatile Int32 m_refCount;                                // +0x04
};

// Intrusive smart pointer. Works for classes that derive from Object virtually too.
template <class T>
class RefPointer {
public:
    RefPointer() : m_p(0) {}
    RefPointer(T* p) : m_p(p) { if (m_p) m_p->AddRef(); }
    RefPointer(const RefPointer& o) : m_p(o.m_p) { if (m_p) m_p->AddRef(); }
    template <class U> RefPointer(const RefPointer<U>& o) : m_p(o.Get()) { if (m_p) m_p->AddRef(); }
    ~RefPointer() { if (m_p) m_p->Release(); }

    RefPointer& operator=(T* p)
    {
        if (p) p->AddRef();
        T* old = m_p;
        m_p = p;
        if (old) old->Release();
        return *this;
    }
    RefPointer& operator=(const RefPointer& o) { return *this = o.m_p; }
    template <class U> RefPointer& operator=(const RefPointer<U>& o) { return *this = o.Get(); }

    T* Get() const        { return m_p; }
    T* operator->() const { return m_p; }
    T& operator*() const  { return *m_p; }
    operator T*() const   { return m_p; }
    bool IsNull() const   { return m_p == 0; }

private:
    T* m_p;
};

// Shared, reference-counted recursive mutex (pthread / CRITICAL_SECTION).
// Mac: ctor 0x9340, AddRef 0x92c0, Release 0x92e0.  Win: release inlined, destroy 0x412550.
class Mutex {
public:
    Mutex();
    void AddRef();
    void Release();
    void Lock();
    void Unlock();

private:
    ~Mutex();
    friend class MutexLocker;
#if defined(_WIN32)
    CRITICAL_SECTION m_cs;        // +0x00 (24 bytes)
#else
    pthread_mutex_t  m_mutex;     // +0x00 (44 bytes on i386 Mac)
#endif
    Int32            m_refCount;  // Win +0x18, Mac +0x2c
};

// Locks a Mutex for its lifetime and keeps it alive meanwhile.
// Mac: ctor 0x93b0, dtor 0x93d0.
class MutexLocker {
public:
    explicit MutexLocker(Mutex* m);
    ~MutexLocker();

private:
    Mutex* m_mutex;
};

} // namespace LEGO
