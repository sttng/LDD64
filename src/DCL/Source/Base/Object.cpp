// LEGO Digital Designer — restored source
// TU "Object.cpp"  Mac 0x91c0-0x946f
// Mutex Win side partly inlined into callers.
#include "Object.h"

#if !defined(_WIN32)
#include <libkern/OSAtomic.h>
#endif
#include <assert.h>

namespace LEGO {

// ---------------------------------------------------------------------------
// Object
// ---------------------------------------------------------------------------

// Mac 0x9260 (OSAtomicAdd32), Win 0x428d60 (InterlockedIncrement)
void Object::AddRef() const
{
#if defined(_WIN32)
    InterlockedIncrement((LONG*)&m_refCount);
#else
    OSAtomicAdd32(1, (int32_t*)&m_refCount);
#endif
}

// Mac 0x9280, Win 0x412520
void Object::Release() const
{
#if defined(_WIN32)
    Int32 n = InterlockedDecrement((LONG*)&m_refCount);
#else
    Int32 n = OSAtomicAdd32(-1, (int32_t*)&m_refCount);
#endif
    if (n < 1) {
        const_cast<Object*>(this)->OnFinalRelease();
        delete this;
    }
}

// ---------------------------------------------------------------------------
// Mutex
// ---------------------------------------------------------------------------

// Mac 0x9340 — recursive mutex
Mutex::Mutex()
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
    m_refCount = 0;
}

// NOTE: the original destroys the mutex while it is still locked (Release, ~MutexLocker:
// undefined for a CRITICAL_SECTION / pthread mutex). fixed: those unlock first, and the
// destructor asserts that the mutex is not held (assert: debug and test builds only).
Mutex::~Mutex()
{
#if defined(_WIN32)
    assert(m_cs.OwningThread == 0 && "Mutex destroyed while locked (M24)");
    DeleteCriticalSection(&m_cs);
#else
    int result = pthread_mutex_destroy(&m_mutex);   // EBUSY while locked
    assert(result == 0 && "Mutex destroyed while locked (M24)");
    (void)result;
#endif
}

void Mutex::Lock()
{
#if defined(_WIN32)
    EnterCriticalSection(&m_cs);
#else
    pthread_mutex_lock(&m_mutex);
#endif
}

void Mutex::Unlock()
{
#if defined(_WIN32)
    LeaveCriticalSection(&m_cs);
#else
    pthread_mutex_unlock(&m_mutex);
#endif
}

// Mac 0x92c0
void Mutex::AddRef()
{
    Lock();
    ++m_refCount;
    Unlock();
}

// Mac 0x92e0 (Win: inlined, e.g. in SpecificObjectFactory dtor 0x42e740)
void Mutex::Release()
{
    Lock();
    if (m_refCount-- < 2) {
        Unlock();               // no other reference is left to lock it meanwhile
        delete this;            // destroys the (locked) mutex
        return;
    }
    Unlock();
}

// ---------------------------------------------------------------------------
// MutexLocker
// ---------------------------------------------------------------------------

// Mac 0x93b0
MutexLocker::MutexLocker(Mutex* m)
    : m_mutex(m)
{
    m_mutex->Lock();
    ++m_mutex->m_refCount;
}

// Mac 0x93d0
MutexLocker::~MutexLocker()
{
    bool last = m_mutex->m_refCount-- < 2;
    m_mutex->Unlock();          // unlocked before it is destroyed
    if (last)
        delete m_mutex;
}

} // namespace LEGO
