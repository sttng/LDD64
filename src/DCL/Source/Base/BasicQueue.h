// LEGO Digital Designer — restored source
// LEGO::BasicQueue<T, C> — FIFO adaptor over one BasicDeque (std::queue shape, no vptr);
// partial, file name inferred. Evidence: LEGOOpenGLView.window ivar encoding.
// The Window ctor (Mac 0x205c0) builds it from a temporary deque.
#pragma once
#include "LEGO/Types.h"
#include "BasicDeque.h"

namespace LEGO {

template <class T, class C = BasicDeque<T> >
class BasicQueue {
public:
    explicit BasicQueue(const C& c = C()) : m_c(c) {}

    bool     IsEmpty() const      { return m_c.IsEmpty(); }
    UInt32   GetSize() const      { return m_c.GetSize(); }
    T&       Front()              { return m_c.Front(); }
    const T& Front() const        { return m_c.Front(); }
    T&       Back()               { return m_c.Back(); }
    const T& Back() const         { return m_c.Back(); }
    void     Push(const T& v)     { m_c.PushBack(v); }
    void     Pop()                { m_c.PopFront(); }

protected:
    C m_c;   // +0x00
};

} // namespace LEGO
