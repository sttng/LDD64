// LEGO Digital Designer — restored source
// LEGO::BasicDeque<T> — vptr + libstdc++ std::deque (40 bytes, 512-byte nodes); partial.
// Instances: Pair<RefPointer<XMLDOMNode>, String> (Mac vtable 0x770d6c {D1 0x22f150, D0 0x22f280},
// Win sdd 0x534df0), Pair<RefPointer<XMLDOMNodeAString>, AString> (Mac 0x770d88 {0x22fce0,
// 0x22fe10}, Win sdd 0x534e10).
#pragma once
#include <deque>
#include "LEGO/Types.h"

namespace LEGO {

template <class T>
class BasicDeque {
public:
    typedef typename std::deque<T>::iterator       Iterator;
    typedef typename std::deque<T>::const_iterator ConstIterator;

    BasicDeque() {}                                                   // Mac 0x22f180 / 0x22fd10
    // NOTE: the XML parse states copy-construct their deque from a default-constructed
    // temporary (Mac 0x22f0b0 calls 0x22f180 then the std::deque copy ctor 0x22f670).
    BasicDeque(const BasicDeque& o) : m_deque(o.m_deque) {}
    virtual ~BasicDeque() {}                                          // Mac 0x22f150/0x22f280

    BasicDeque& operator=(const BasicDeque& o) { m_deque = o.m_deque; return *this; }

    bool     IsEmpty() const    { return m_deque.empty(); }
    UInt32   GetSize() const    { return static_cast<UInt32>(m_deque.size()); }
    void     PushBack(const T& v) { m_deque.push_back(v); }           // Mac 0x22f930 / 0x230640 (_M_push_back_aux)
    void     PopBack()          { m_deque.pop_back(); }
    void     PushFront(const T& v) { m_deque.push_front(v); }
    void     PopFront()         { m_deque.pop_front(); }
    T&       Back()             { return m_deque.back(); }
    // (additive)
    T&       operator[](UInt32 i)       { return m_deque[i]; }
    const T& operator[](UInt32 i) const { return m_deque[i]; }
    const T& Back() const       { return m_deque.back(); }
    T&       Front()            { return m_deque.front(); }
    const T& Front() const      { return m_deque.front(); }
    void     Clear()            { m_deque.clear(); }
    Iterator      Begin()       { return m_deque.begin(); }
    Iterator      End()         { return m_deque.end(); }
    ConstIterator Begin() const { return m_deque.begin(); }
    ConstIterator End() const   { return m_deque.end(); }

private:
    std::deque<T> m_deque;   // Mac +0x04 (40 bytes)
};

} // namespace LEGO
