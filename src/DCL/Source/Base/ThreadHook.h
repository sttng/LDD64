// LEGO Digital Designer — restored source
// LEGO::ThreadHook — singleton owning the TLS slot that maps OS threads to LEGO::Thread. A struct
// (Win RTTI ".?AUThreadHook@LEGO@@"). TU "ThreadHook.cpp" (Mac 0xdc30-0xedcf, Win 0x42d400-0x42df80).
// Vtable Mac 0x75f8c0 {D1 0xebf0, D0 0xec50, 0x9250 0x9260 0x9280}; Win 0xa1fbe4 {0x42d530, ..}.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Thread.h"

namespace LEGO {

// sizeof == 0x0c
struct ThreadHook : public Object {
    ThreadHook();                                                     // Win 0x42d450 (Mac inlined)
    virtual ~ThreadHook();                                            // Mac 0xebf0/0xec50  Win 0x42d4c0/0x42d530

    // The singleton; created on first use, released at exit.
    static ThreadHook* Get();                                         // Mac 0xdc30  Win 0x42d550 (name inferred)
    static Thread*     GetCurrentThread();                            // Mac 0xdd30 (Win inlined) (name inferred)
    static void        SetCurrentThread(Thread* thread);              // Win 0x42d6f0 (Mac inlined) (name inferred)

#if defined(_WIN32)
    DWORD         m_tlsIndex;    // +0x08 (TLS_OUT_OF_INDEXES when not allocated)
#else
    pthread_key_t m_tlsIndex;    // +0x08
#endif

    static ThreadHook* s_instance;   // Mac 0x77d024  Win 0xb42750 (set by the ctor)
};

} // namespace LEGO
