// LEGO Digital Designer — restored source
// LEGO::Thread — OS thread (pthreads / Win32) with its own error root; either started (Start():
// OnThreadStart / Run / OnThreadEnd) or attached (Attach() adopts the caller, e.g. main()). The
// current Thread is kept in ThreadHook's TLS slot. TU "ThreadHook.cpp" (Mac 0xdc30-0xedcf, Win
// 0x42d400-0x42df80). Vtable Mac 0x703c60 {D1 0xe590, D0 0xe5a0, OnThreadStart 0xeb80, OnThreadEnd
// 0xeb90, Run 0xeba0, OnAttach 0xebb0, OnDetach 0xebc0}; Win 0xa1fbc8 {sdd 0x42d7a0, 0x459790
// 0x459790 0x4f5b70 0x459790 0x459790 (shared ICF bodies)}.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "Error.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace LEGO {

// Win calls GetCurrentThreadId(), Mac calls pthread_self() at the same spot in
// TraceErrorHandler::Dump, so this was most likely an inline platform helper.
// (name inferred)
// PORT64: the ID type is pointer-sized off Windows (pthread_t is a pointer on macOS); both
// original builds are 32-bit, where it stays UInt32. Windows thread IDs are 32-bit DWORDs,
// also on Win64.
#if defined(_WIN32) || !defined(__LP64__)
typedef UInt32 ThreadID;
#else
typedef UInt64 ThreadID;
#endif

inline ThreadID GetCurrentThreadID()
{
#if defined(_WIN32)
    return (ThreadID)::GetCurrentThreadId();
#else
    return (ThreadID)(size_t)::pthread_self();
#endif
}

// sizeof == 0x70 on both builds.
class Thread {
public:
    Thread();                                                         // Mac 0xe020/0xe160  Win 0x42d710
    virtual ~Thread();                                                // Mac 0xe170/0xe590/0xe5a0  Win 0x42d7c0/0x42d7a0

    // Creates the OS thread; returns false if it is already running/attached or creation
    // failed. priority: -128 (idle) .. 127 (time critical), 0 = normal.
    bool Start(Int8 priority);                                        // Mac 0xe5e0  Win 0x42d8c0 (name inferred)
    // Adopts the calling OS thread (which must not have a Thread yet). (name inferred)
    bool Attach();                                                    // Mac 0xea30  Win 0x42d9c0
    // Undoes Attach(); only works from the attached thread itself. (name inferred)
    bool Detach();                                                    // Mac 0xe390  Win 0x42dab0
    // Busy-waits until the thread function has returned. Always returns true. (name inferred)
    bool Wait();                                                      // Mac 0xde10  Win 0x42d600
    bool IsRunning() const;                                           // Win 0x42d420 (Mac inlined) (name inferred)
    bool GetResult() const { return m_result; }                       // (name inferred) value returned by Run()

    Int8 GetPriority() const;                                         // Win 0x42db90 (Mac inlined) (name inferred)
    void SetPriority(Int8 priority);                                  // Win 0x42dc20 (Mac inlined) (name inferred)

    ErrorRoot& GetErrorRoot() { return m_errorRoot; }                 // (name inferred)

    // Thread object of the calling OS thread (0 if none).
    static Thread* GetCurrent();                                      // Mac 0xdd20
    static bool    IsMainThread();                                    // Mac 0xdd00 (name inferred)
    // Keeps an object alive until the current thread's objects are released. (names inferred)
    static void    AddObject(Object* object);                         // Mac 0xde40  Win 0x42d640
    static void    ReleaseObjects();                                  // Mac 0xdfd0  Win 0x42d6d0

protected:
    virtual void OnThreadStart() {}                                   // Mac 0xeb80 (name inferred)
    virtual void OnThreadEnd() {}                                     // Mac 0xeb90 (name inferred)
    virtual bool Run() { return true; }                               // Mac 0xeba0 (name inferred)
    virtual void OnAttach() {}                                        // Mac 0xebb0 (name inferred)
    virtual void OnDetach() {}                                        // Mac 0xebc0 (name inferred)

private:
    Thread(const Thread&);
    Thread& operator=(const Thread&);

#if defined(_WIN32)
    static unsigned __stdcall ThreadMain(void* thread);               // Win 0x42dca0 (name inferred)
#else
    static void* ThreadMain(void* thread);                            // Mac 0xe7d0 (name inferred)
#endif
    void Lock();
    void Unlock();

public:
    ErrorRoot                      m_errorRoot;   // +0x04 (Mac sizeof 0x24, Win 0x38)
private:
#if defined(_WIN32)
    DWORD                          m_threadID;    // +0x3c (name inferred)
    CRITICAL_SECTION               m_lock;        // +0x40
#else
    pthread_t                      m_threadID;    // +0x28 (name inferred)
    pthread_mutex_t                m_lock;        // +0x2c (recursive)
#endif
    bool                           m_result;      // +0x58 return value of Run()
    bool                           m_attached;    // +0x59 thread adopted by Attach()
    volatile bool                  m_running;     // +0x5a set while the thread function runs
    bool                           m_unknown5b;   // +0x5b only zeroed by the Win ctor (purpose unknown)
    BasicArray<RefPointer<Object> > m_objects;    // +0x5c see AddObject()
#if defined(_WIN32)
    HANDLE                         m_handle;      // +0x6c
#else
    pthread_t                      m_handle;      // +0x6c
#endif
};

} // namespace LEGO
