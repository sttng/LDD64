// LEGO Digital Designer — restored source
// LEGO::ObjectRefPointer<T> — keeps the T pointer and its Object (virtual base) pointer, counts
//   through the latter. RTTI: ObjectRefPointer<Font> (LDDApplication fonts, Mac +0x134),
//   <ProgressBarDialog> (+0xa0). Header-only; names inferred.
// Layout: T* +0x00, Object* +0x04, sizeof 8 (Mac 0x16efb0, 0x17a2d0). Assignment (Mac 0x1709d9,
//   0x171c7a) releases the old object through the old T pointer's Object.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"

namespace LEGO {

template <class T>
class ObjectRefPointer {
public:
    ObjectRefPointer() : m_p(0), m_object(0) {}
    ObjectRefPointer(T* p) : m_p(p), m_object(p) { if (m_object) m_object->AddRef(); }
    ObjectRefPointer(const ObjectRefPointer& o) : m_p(o.m_p), m_object(o.m_object)
    {
        if (m_object) m_object->AddRef();
    }
    ~ObjectRefPointer() { if (m_object) m_object->Release(); }

    ObjectRefPointer& operator=(T* p)
    {
        T* old = m_p;
        if (old == p)
            return *this;
        m_p = p;
        if (p) {
            m_object = p;
            m_object->AddRef();
        } else {
            m_object = 0;
        }
        if (old)
            static_cast<const Object*>(old)->Release();
        return *this;
    }
    ObjectRefPointer& operator=(const ObjectRefPointer& o) { return *this = o.m_p; }

    T* Get() const        { return m_p; }
    T* operator->() const { return m_p; }
    T& operator*() const  { return *m_p; }
    operator T*() const   { return m_p; }

private:
    T*      m_p;        // +0x00
    Object* m_object;   // +0x04 (the same object as its Object)
};

} // namespace LEGO
