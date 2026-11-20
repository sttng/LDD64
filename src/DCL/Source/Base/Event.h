// LEGO Digital Designer — restored source
// LEGO::Event — inline auto-reset event, embedded by value (RunInMainThreadObserver,
// SystemUpdater.h); names inferred. Win: one HANDLE (ctor 0x4520d0 CreateEventA auto-reset,
// dtor 0x4520f0; Set/Wait inlined at Win 0x424a00 / 0x4249c0). Mac: pthread cond (0x1c) + mutex
// (0x2c) + bool, inlined (ctor e.g. Mac 0x337e19-0x337e3e, dtor 0x1e5b0, Signal 0x1e250,
// Wait 0x1e1f0).
#pragma once
#include "LEGO/Types.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#endif

namespace LEGO {

class Event {
public:
    Event()                                                          // Win 0x4520d0
    {
#if defined(_WIN32)
        m_handle = CreateEventA(NULL, FALSE, FALSE, NULL);
#else
        m_signaled = false;
        pthread_cond_init(&m_cond, NULL);
        pthread_mutex_init(&m_mutex, NULL);
#endif
    }
    ~Event()                                                         // Win 0x4520f0
    {
#if defined(_WIN32)
        CloseHandle(m_handle);
#else
        pthread_cond_destroy(&m_cond);
        pthread_mutex_destroy(&m_mutex);
#endif
    }

    // Wakes one waiter (or lets the next Wait() return at once).
    void Signal()
    {
#if defined(_WIN32)
        SetEvent(m_handle);
#else
        pthread_mutex_lock(&m_mutex);
        m_signaled = true;
        pthread_cond_signal(&m_cond);
        pthread_mutex_unlock(&m_mutex);
#endif
    }

    // Blocks until signalled, then resets.
    // NOTE (Mac): a single `if`, not a loop, around pthread_cond_wait — a spurious wake-up
    // returns early (kept from the original).
    void Wait()
    {
#if defined(_WIN32)
        WaitForSingleObject(m_handle, INFINITE);
#else
        pthread_mutex_lock(&m_mutex);
        if (!m_signaled)
            pthread_cond_wait(&m_cond, &m_mutex);
        m_signaled = false;
        pthread_mutex_unlock(&m_mutex);
#endif
    }

    // Blocks until signalled or `milliseconds` have passed; resets the event. Returns false on
    // a time-out. (additive: inlined in OperationManager::Wait, Win 0x5394e0
    // WaitForSingleObject(h, 1000) / Mac 0x2849d0 pthread_cond_timedwait.)
    // NOTE (Mac): the original deadline is now + 1 s + 1 ms (tv_usec * 1000 + 1000000 ns);
    // kept: the extra millisecond is added for every timeout.
    bool Wait(UInt32 milliseconds)
    {
#if defined(_WIN32)
        return WaitForSingleObject(m_handle, milliseconds) != WAIT_TIMEOUT;
#else
        int result = 0;
        pthread_mutex_lock(&m_mutex);
        if (!m_signaled) {
            struct timeval now;
            gettimeofday(&now, 0);
            struct timespec deadline;
            deadline.tv_sec = now.tv_sec + (time_t)(milliseconds / 1000);
            long nsec = (long)now.tv_usec * 1000 + (long)(milliseconds % 1000) * 1000000 + 1000000;
            if (nsec > 999999999) {
                deadline.tv_sec += 1;
                nsec -= 1000000000;
            }
            deadline.tv_nsec = nsec;
            result = pthread_cond_timedwait(&m_cond, &m_mutex, &deadline);
        }
        m_signaled = false;
        pthread_mutex_unlock(&m_mutex);
        return result != ETIMEDOUT;
#endif
    }

private:
    Event(const Event&);
    Event& operator=(const Event&);

#if defined(_WIN32)
    HANDLE          m_handle;     // +0x00
#else
    pthread_cond_t  m_cond;       // +0x00 (0x1c on i386 Mac)
    pthread_mutex_t m_mutex;      // +0x1c (0x2c)
    bool            m_signaled;   // +0x48
#endif
};

} // namespace LEGO
