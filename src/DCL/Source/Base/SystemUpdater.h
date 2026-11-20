// LEGO Digital Designer — restored source
// LEGO::SystemUpdater — main-thread Updater ('updr') that also runs observers queued from any
//   thread (Enqueue) once each. RunInMainThreadObserver — Run() calls Update() on the main thread
//   or queues itself and blocks until done; derived Update()s (e.g. Mac 0x337dd0) call it last.
// TU Mac 0x1ddd0-0x2046b, Win 0x424060-0x424ab0 (+ 0x40bd00, 0x501150). Class names RTTI.
// Vtables: SystemUpdater Mac {D1 0x1fc90, D0 0x1fcf0, OnFinalRelease 0x1e9a0, Update 0x1fd60},
//   Win {Update 0x4241f0}, Object part {0x424b60 ...}; RunInMainThreadObserver Mac {D1 0x1e5b0,
//   D0 0x1e650, 0x1e330, OnAttached 0x1e380, OnDetached 0x1e390, Update 0x1e250}, Win {0x40be80,
//   0x40be80, Update 0x424a00}.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "Updater.h"
#include "Event.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace LEGO {

// Mac sizeof 0x68: Updater part +0x00..+0x24, mutex +0x24 (0x2c), queue +0x50,
// virtual base Object +0x60. Win sizeof 0x5c: Updater part (vptr, vbptr, arrays) +0x00..+0x28,
// CRITICAL_SECTION +0x28, queue +0x40, vtordisp +0x50, Object +0x54.
class SystemUpdater : public Updater {
public:
    enum { kRegistryId = 0x75706472 };                                // 'updr' (name inferred)

    SystemUpdater();                                                  // Mac 0x1f950, Win 0x424120
    virtual ~SystemUpdater();                                         // Mac 0x1fc90/0x1fcf0 (D2 0x202b0), Win 0x424330

    // Updater::Update, then runs every queued observer once (attached to this updater, with
    // the queue lock released while its Update() runs). (name inferred)
    virtual void Update();                                            // Mac 0x1fd60, Win 0x4241f0

    // The shared system updater (null if it could not be created). (name inferred)
    static RefPointer<Updater> GetInstance();                         // Mac 0x1ded0, Win 0x4248d0

    // Queues `observer` for the next Update() of the shared SystemUpdater (thread safe).
    // (name inferred)
    // NOTE: the shared instance is used as a SystemUpdater without a type check (as the
    // original does).
    static void Enqueue(UpdateObserver* observer);                    // Mac 0x1dde0, Win 0x424410
    // The main loop's wake-up, called by Enqueue when the queue was empty (from any thread;
    // 0: none). The main loop then calls RunQueued without waiting for its next pass, so that a
    // RunInMainThreadObserver does not wait for it.
    static void SetWake(void (*wake)());
    // Runs the queued observers as Update does, without Updater::Update (main thread).
    static void RunQueued();

private:
    SystemUpdater(const SystemUpdater&);
    SystemUpdater& operator=(const SystemUpdater&);

    // Embedded recursive mutex (same shape as ObjectRegistry's; inline ctor/dtor).
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
        CRITICAL_SECTION m_cs;
#else
        pthread_mutex_t  m_mutex;
#endif
    };

    RecursiveMutex                         m_queueLock;   // Mac +0x24, Win +0x28
    BasicArray<RefPointer<UpdateObserver> > m_queue;       // Mac +0x50, Win +0x40 (reserved 8)
};

// Creates the SystemUpdater for ObjectRegistry::Get('updr', ...).
// Mac vtable 0x75ff70 {D1 0x1e1e0, D0 0x1f8e0, CreateObject 0x1f8f0};
// Win 0xa1f778 {sdd 0x424860, CreateObject 0x4247b0}.
class SystemUpdaterInstantiator : public ObjectRegistryInstantiator {
public:
    SystemUpdaterInstantiator() {}
    virtual ~SystemUpdaterInstantiator() {}                           // Mac 0x1e1e0/0x1f8e0, Win 0x424860
    virtual RefPointer<Object> CreateObject() const;                  // Mac 0x1f8f0, Win 0x4247b0
};

// Mac layout: vptr +0, subject +4, Event +0x08 (cond +0x08, mutex +0x24, flag +0x50),
// Object +0x54 (sizeof 0x5c). Win: vptr +0, vbptr +4, subject +8, Event (HANDLE) +0x0c,
// Object +0x10 (sizeof 0x18). The constructor is inline (Mac e.g. 0x337e01-0x337e3e,
// Win 0x57cac0).
class RunInMainThreadObserver : public UpdateObserver {
public:
    RunInMainThreadObserver() {}
    virtual ~RunInMainThreadObserver() {}                             // Mac 0x1e5b0/0x1e650

    // Runs Update() on the main thread and waits until it has finished. (name inferred)
    void Run();                                                       // Mac 0x1e1f0, Win 0x4249c0 (ESI = this)

    // Detaches from the updater and wakes Run(). Derived classes do their work first.
    virtual void Update();                                            // Mac 0x1e250, Win 0x424a00

private:
    Event m_done;                                                     // Mac +0x08, Win +0x0c
};

} // namespace LEGO
