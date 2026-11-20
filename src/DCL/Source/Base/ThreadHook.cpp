// LEGO Digital Designer — restored source
// TU "ThreadHook.cpp": ThreadHook, Thread, BasicArray<RefPointer<Object> > (Mac 0xe530/0xecc0,
// Win 0x42dd70/0x42de50). Mac 0xdc30-0xedcf, Win 0x42d400-0x42df80. Obj-C++ on Mac (ThreadMain's
// NSAutoreleasePool, behind __OBJC__); host builds take the plain C++ path.
#include "ThreadHook.h"
#include "Thread.h"
#include "LEGO/String.h"

#if defined(_WIN32)
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#if defined(__OBJC__)
#import <Foundation/Foundation.h>
#endif
#endif

namespace LEGO {

#if defined(_WIN32)
// Unhandled-exception filter of the application (crash reporter, writes a dump).
// Lives in another TU (Win 0x5ce42c), not restored yet. (name inferred)
LONG HandleUnhandledException(EXCEPTION_POINTERS* info);
#endif

namespace {

// Main-thread identity, captured by the static initialiser.
// Mac 0x77d018 (static init 0xed40), Win 0xb42ef0.
const ThreadID s_mainThreadID = GetCurrentThreadID();

// Win Sleep(0) / Mac usleep(1) at the same spots: inline platform helper (name inferred).
inline void YieldThread()
{
#if defined(_WIN32)
    ::Sleep(0);
#else
    ::usleep(1);
#endif
}

// Release builds construct the message and drop it (both builds); a debug-only
// assertion/trace macro in the original. (name inferred)
inline void DebugMessage(const String&) {}

} // namespace

// ===========================================================================
// ThreadHook
// ===========================================================================

ThreadHook* ThreadHook::s_instance = 0;

// Win 0x42d450 (Mac: inlined into every Get())
ThreadHook::ThreadHook()
{
#if defined(_WIN32)
    m_tlsIndex = TLS_OUT_OF_INDEXES;
    s_instance = this;
    m_tlsIndex = ::TlsAlloc();
#else
    m_tlsIndex = 0;
    s_instance = this;
    ::pthread_key_create(&m_tlsIndex, 0);
#endif
}

// Mac 0xebf0 (D1), 0xec50 (D0); Win 0x42d4c0, 0x42d530 (scalar deleting)
ThreadHook::~ThreadHook()
{
#if defined(_WIN32)
    if (m_tlsIndex != TLS_OUT_OF_INDEXES) {
        ::TlsFree(m_tlsIndex);
        m_tlsIndex = TLS_OUT_OF_INDEXES;
    }
#else
    ::pthread_key_delete(m_tlsIndex);
    m_tlsIndex = 0;
#endif
    // NOTE: s_instance is not reset (both builds).
}

// Mac 0xdc30, Win 0x42d550
ThreadHook* ThreadHook::Get()
{
    // Mac guard 0x77d020 / RefPointer 0x77d01c (no __cxa_guard: -fno-threadsafe-statics);
    // Win guard 0xb44aa0 / RefPointer 0xb4496c.
    static RefPointer<ThreadHook> s_hook(new ThreadHook);
    return s_instance;
}

// Mac 0xdd30 (Win: inlined TlsGetValue)
Thread* ThreadHook::GetCurrentThread()
{
#if defined(_WIN32)
    return (Thread*)::TlsGetValue(Get()->m_tlsIndex);
#else
    return (Thread*)::pthread_getspecific(Get()->m_tlsIndex);
#endif
}

// Win 0x42d6f0 (Mac: inlined)
void ThreadHook::SetCurrentThread(Thread* thread)
{
#if defined(_WIN32)
    ::TlsSetValue(Get()->m_tlsIndex, thread);
#else
    ::pthread_setspecific(Get()->m_tlsIndex, thread);
#endif
}

// ===========================================================================
// Thread — static helpers
// ===========================================================================

// Mac 0xdd00
bool Thread::IsMainThread()
{
    return GetCurrentThreadID() == s_mainThreadID;
}

// Mac 0xdd20
Thread* Thread::GetCurrent()
{
    return ThreadHook::GetCurrentThread();
}

// Mac 0xde40, Win 0x42d640
void Thread::AddObject(Object* object)
{
    // NOTE: no check for a missing current Thread (both builds).
    ThreadHook::GetCurrentThread()->m_objects.PushBack(RefPointer<Object>(object));
}

// Mac 0xdfd0, Win 0x42d6d0
void Thread::ReleaseObjects()
{
    ThreadHook::GetCurrentThread()->m_objects.Clear();
}

// ===========================================================================
// Thread
// ===========================================================================

// Mac 0xe020 (C2), 0xe160 (C1); Win 0x42d710
Thread::Thread()
    : m_threadID(0),
      m_result(false),
      m_attached(false),
      m_running(false),
      m_unknown5b(false),
      m_handle(0)
{
#if defined(_WIN32)
    ::InitializeCriticalSection(&m_lock);
#else
    pthread_mutexattr_t attr;
    ::pthread_mutexattr_init(&attr);
    ::pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    ::pthread_mutex_init(&m_lock, &attr);
    ::pthread_mutexattr_destroy(&attr);
#endif
}

// Mac 0xe170 (D2), 0xe590 (D1), 0xe5a0 (D0); Win 0x42d7c0, 0x42d7a0 (scalar deleting)
Thread::~Thread()
{
    Detach();
    if (IsRunning()) {
        DebugMessage(String("Thread::~Thread(): Destroying the Thread object while the thread "
                            "is running is really bad."));
#if defined(_WIN32)
        ::TerminateThread(m_handle, (DWORD)-1);
#endif
        // NOTE: Mac leaves the running pthread alone.
    }
#if defined(_WIN32)
    if (m_handle)
        ::CloseHandle(m_handle);
#endif
    // NOTE: Mac never joins/detaches the pthread.
#if defined(_WIN32)
    ::DeleteCriticalSection(&m_lock);
#else
    ::pthread_mutex_destroy(&m_lock);
#endif
}

void Thread::Lock()
{
#if defined(_WIN32)
    ::EnterCriticalSection(&m_lock);
#else
    ::pthread_mutex_lock(&m_lock);
#endif
}

void Thread::Unlock()
{
#if defined(_WIN32)
    ::LeaveCriticalSection(&m_lock);
#else
    ::pthread_mutex_unlock(&m_lock);
#endif
}

// Win 0x42d420 (Mac: inlined, reads the flag)
bool Thread::IsRunning() const
{
#if defined(_WIN32)
    DWORD code;
    if (::GetExitCodeThread(m_handle, &code))
        return code == STILL_ACTIVE;
    return false;
#else
    return m_running;
#endif
}

// Mac 0xde10, Win 0x42d600
bool Thread::Wait()
{
    while (IsRunning())
        YieldThread();
    return true;
}

// Win 0x42db90 (Mac: inlined into Start)
Int8 Thread::GetPriority() const
{
#if defined(_WIN32)
    switch (::GetThreadPriority(m_handle)) {
    case THREAD_PRIORITY_IDLE:          return -128;
    case THREAD_PRIORITY_LOWEST:        return -80;
    case THREAD_PRIORITY_BELOW_NORMAL:  return -40;
    case THREAD_PRIORITY_NORMAL:        return 0;
    case THREAD_PRIORITY_ABOVE_NORMAL:  return 40;
    case THREAD_PRIORITY_HIGHEST:       return 80;
    case THREAD_PRIORITY_TIME_CRITICAL: return 127;
    }
    return 0;
#else
    // NOTE: policy 0 is passed literally (not SCHED_OTHER, which is 1 on Mac OS X).
    int lo = ::sched_get_priority_min(0);
    int hi = ::sched_get_priority_max(0);
    int policy;
    sched_param param;
    ::pthread_getschedparam(m_handle, &policy, &param);
#if !defined(__APPLE__)
    if (hi == lo)       // HOST-ONLY: Linux reports an empty range for policy 0
        return 0;
#endif
    return (Int8)(((param.sched_priority - lo) * 256) / (hi - lo) - 128);
#endif
}

// Win 0x42dc20 (Mac: inlined into Start)
void Thread::SetPriority(Int8 priority)
{
#if defined(_WIN32)
    int p;
    if (priority < -100)     p = THREAD_PRIORITY_IDLE;
    else if (priority < -60) p = THREAD_PRIORITY_LOWEST;
    else if (priority < -20) p = THREAD_PRIORITY_BELOW_NORMAL;
    else if (priority < 20)  p = THREAD_PRIORITY_NORMAL;
    else if (priority < 60)  p = THREAD_PRIORITY_ABOVE_NORMAL;
    else if (priority < 100) p = THREAD_PRIORITY_HIGHEST;
    else                     p = THREAD_PRIORITY_TIME_CRITICAL;
    ::SetThreadPriority(m_handle, p);
#else
    int lo = ::sched_get_priority_min(0);
    int hi = ::sched_get_priority_max(0);
    int policy;
    sched_param param;
    ::pthread_getschedparam(m_handle, &policy, &param);
    param.sched_priority = ((priority + 128) * (hi - lo)) / 256 + lo;
    ::pthread_setschedparam(m_handle, policy, &param);
#endif
}

// Mac 0xe5e0, Win 0x42d8c0
bool Thread::Start(Int8 priority)
{
    bool started = false;
    Lock();
    if (m_handle == 0) {
#if defined(_WIN32)
        unsigned id;
        m_handle = (HANDLE)::_beginthreadex(0, 0, ThreadMain, this, 0, &id);
        bool ok = m_handle != 0;
#else
        bool ok = ::pthread_create(&m_handle, 0, ThreadMain, this) == 0;
#endif
        if (ok) {
            // Start at least at the caller's priority until the thread is up.
            // NOTE: assumes the calling thread has a Thread object (both builds).
            Int8 p = ThreadHook::GetCurrentThread()->GetPriority();
            if (p < priority)
                p = priority;
            SetPriority(p);
            while (m_threadID == 0)
                YieldThread();
            SetPriority(priority);
            started = true;
        }
    }
    Unlock();
    return started;
}

// Mac 0xe7d0, Win 0x42dca0
#if defined(_WIN32)
unsigned __stdcall Thread::ThreadMain(void* p)
#else
void* Thread::ThreadMain(void* p)
#endif
{
    Thread* thread = static_cast<Thread*>(p);
#if defined(_WIN32)
#if defined(_MSC_VER)   // the zig/MinGW check build cannot do x86 SEH in a __stdcall function
    __try {
#endif
        ThreadHook::SetCurrentThread(thread);
        MSG msg;
        ::PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE);     // creates the thread's message queue
#else
    ThreadHook::SetCurrentThread(thread);
#if defined(__OBJC__)
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
#endif
#endif
        thread->m_running = true;
#if defined(_WIN32)
        thread->m_threadID = ::GetCurrentThreadId();
#else
        thread->m_threadID = ::pthread_self();
#endif
        YieldThread();
        thread->OnThreadStart();
        thread->m_result = thread->Run();
        thread->OnThreadEnd();
        thread->m_objects.Clear();
#if !defined(_WIN32) && defined(__OBJC__)
    [pool release];
#endif
        ThreadHook::SetCurrentThread(0);
        thread->m_running = false;
#if defined(_WIN32) && defined(_MSC_VER)
    } __except (HandleUnhandledException(GetExceptionInformation())) {
    }
#endif
    return 0;
}

// Mac 0xea30, Win 0x42d9c0
bool Thread::Attach()
{
    bool attached = false;
    Lock();
    if (m_handle == 0 && ThreadHook::GetCurrentThread() == 0) {
#if defined(_WIN32)
        ::DuplicateHandle(::GetCurrentProcess(), ::GetCurrentThread(), ::GetCurrentProcess(),
                          &m_handle, 0, TRUE, DUPLICATE_SAME_ACCESS);
        m_threadID = ::GetCurrentThreadId();
#else
        m_handle = ::pthread_self();
        m_threadID = m_handle;
#endif
        m_attached = true;
        m_running = true;
        ThreadHook::SetCurrentThread(this);
        OnAttach();
        attached = true;
    }
    Unlock();
    return attached;
}

// Mac 0xe390, Win 0x42dab0
bool Thread::Detach()
{
    bool detached = false;
    Lock();
#if defined(_WIN32)
    if (m_handle != 0 && m_attached && m_threadID == ::GetCurrentThreadId()) {
#else
    if (m_handle != 0 && m_attached && m_threadID == ::pthread_self()) {
#endif
        OnDetach();
        m_objects.Clear();
        ThreadHook::SetCurrentThread(0);
#if defined(_WIN32)
        ::CloseHandle(m_handle);
#endif
        m_handle = 0;
        m_threadID = 0;
        m_attached = false;
        m_running = false;
        detached = true;
    }
    Unlock();
    return detached;
}

} // namespace LEGO
