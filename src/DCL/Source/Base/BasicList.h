// LEGO Digital Designer — restored source
// LEGO::BasicList<T> — vptr + libstdc++ std::list (partial). BasicList<ErrorHandler*>:
// vtable {D1, D0} (Mac dtor 0x6580/0x6aa0), list hook/unhook calls in ErrorRoot.
#pragma once
#include <list>
#include "LEGO/Types.h"

namespace LEGO {

template <class T>
class BasicList {
public:
    typedef typename std::list<T>::iterator       Iterator;
    typedef typename std::list<T>::const_iterator ConstIterator;

    BasicList() {}
    virtual ~BasicList() {}

    void PushBack(const T& v) { m_list.push_back(v); }
    void Remove(const T& v)   { m_list.remove(v); }
    Iterator Begin()          { return m_list.begin(); }
    Iterator End()            { return m_list.end(); }
    // --- used by TU 012 (MonolithTreeDirectory) ---
    ConstIterator Begin() const { return m_list.begin(); }
    ConstIterator End() const   { return m_list.end(); }
    bool     IsEmpty() const  { return m_list.empty(); }
    UInt32   GetSize() const  { return static_cast<UInt32>(m_list.size()); }
    T&       Front()          { return m_list.front(); }
    void     PopFront()       { m_list.pop_front(); }
    Iterator Erase(Iterator it) { return m_list.erase(it); }

private:
    std::list<T> m_list;
};

} // namespace LEGO
