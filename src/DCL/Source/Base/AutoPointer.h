// LEGO Digital Designer — restored source
// LEGO::AutoPointer<T> — owning pointer; a copy takes the owner flag, the source keeps the
//   pointer (std::auto_ptr-like). Name from container RTTI (HTTPCommunicator chunks, Mac vtable
//   0x765ce0, Win sdd 0x633746; TCPSocket buffers). Header-only, non-polymorphic.
// Layout: T* +0x00, bool owner +0x04 (sizeof 8). Mac evidence: chunk push in
//   HTTPCommunicator::OnReceived (Mac 0x131dc0), FileDownloader buffer +0x98/+0x9c (0x1b2350,
//   0x1b3740).
// NOTE: the original frees operator new[] blocks with scalar delete too (every AutoPointer holds
//   a new[] byte block: HTTP chunks, socket and download buffers; undefined behaviour, harmless
//   with the original allocators); fixed: delete[] (low). Member names inferred.
#pragma once
#include "LEGO/Types.h"

namespace LEGO {

template <class T>
class AutoPointer {
public:
    AutoPointer() : m_pointer(0), m_owner(false) {}
    explicit AutoPointer(T* pointer) : m_pointer(pointer), m_owner(pointer != 0) {}
    // Takes the ownership over (the source keeps the pointer as a non-owning view).
    AutoPointer(const AutoPointer& other) : m_pointer(other.m_pointer), m_owner(other.m_owner)
    {
        other.m_owner = false;
    }
    ~AutoPointer()
    {
        if (m_owner && m_pointer)
            Free(m_pointer);
    }

    AutoPointer& operator=(const AutoPointer& other)
    {
        if (&other != this) {
            if (m_pointer != other.m_pointer) {
                if (m_owner && m_pointer)
                    Free(m_pointer);
                m_owner = other.m_owner;
            }
            else if (other.m_owner) {
                m_owner = true;
            }
            m_pointer = other.m_pointer;
            other.m_owner = false;
        }
        return *this;
    }

    // Replaces the pointer (frees an owned different one) and owns the new one.
    void Reset(T* pointer)
    {
        if (m_pointer != pointer) {
            if (m_owner && m_pointer)
                Free(m_pointer);
        }
        m_owner = true;
        m_pointer = pointer;
    }
    // Frees an owned pointer and becomes empty.
    void Release()
    {
        if (m_pointer) {
            if (m_owner)
                Free(m_pointer);
            m_owner = false;
            m_pointer = 0;
        }
    }

    T*   Get() const     { return m_pointer; }
    bool IsOwner() const { return m_owner; }

private:
    T*           m_pointer;   // +0x00
    mutable bool m_owner;     // +0x04

private:
    static void Free(T* pointer)
    {
        delete[] pointer;
    }
};

} // namespace LEGO
