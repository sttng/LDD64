// LEGO Digital Designer — restored source
// LEGO::Pair, Less, BasicMap, BasicSet, BasicMultiset (partial). Names from RTTI; each is a
// vptr + libstdc++ std::map/set (_Rb_tree seen in LiffChunk), vtable just {D1, D0}.
#pragma once
#include "LEGO/Types.h"
#include <map>
#include <set>

namespace LEGO {

template <class A, class B>
struct Pair {
    A first;
    B second;
    Pair() : first(), second() {}
    Pair(const A& a, const B& b) : first(a), second(b) {}
    bool operator<(const Pair& o) const { return first < o.first || (!(o.first < first) && second < o.second); }
    bool operator==(const Pair& o) const { return first == o.first && second == o.second; }
};

template <class A, class B>
inline Pair<A, B> MakePair(const A& a, const B& b) { return Pair<A, B>(a, b); }

// RTTI LEGO::Triple<UInt32, UInt32, UInt32> (BrickDatabase.cpp: BasicMap<UInt32, Triple<…>>,
// Mac vtable 0x762120). Member names follow Pair.
template <class A, class B, class C>
struct Triple {
    A first;
    B second;
    C third;
    Triple() : first(), second(), third() {}
    Triple(const A& a, const B& b, const C& c) : first(a), second(b), third(c) {}
};

template <class T>
struct Less {
    bool operator()(const T& a, const T& b) const { return a < b; }
};

template <class K, class V, class C = Less<K> >
class BasicMap {
public:
    typedef std::map<K, V, C>                    Map;
    typedef typename Map::iterator               Iterator;
    typedef typename Map::const_iterator         ConstIterator;

    BasicMap() {}
    virtual ~BasicMap() {}

    UInt32        GetSize() const { return (UInt32)m_map.size(); }
    bool          IsEmpty() const { return m_map.empty(); }
    Iterator      Begin()         { return m_map.begin(); }
    Iterator      End()           { return m_map.end(); }
    ConstIterator Begin() const   { return m_map.begin(); }
    ConstIterator End() const     { return m_map.end(); }
    Iterator      Find(const K& k)       { return m_map.find(k); }
    ConstIterator Find(const K& k) const { return m_map.find(k); }
    Iterator      LowerBound(const K& k) { return m_map.lower_bound(k); }
    Iterator      UpperBound(const K& k) { return m_map.upper_bound(k); }
    bool          Contains(const K& k) const { return m_map.find(k) != m_map.end(); }
    V&            operator[](const K& k) { return m_map[k]; }
    void          Insert(const K& k, const V& v) { m_map.insert(std::make_pair(k, v)); }
    void          Set(const K& k, const V& v)    { m_map[k] = v; }
    void          Erase(Iterator it)             { m_map.erase(it); }
    void          Erase(Iterator b, Iterator e)  { m_map.erase(b, e); }
    UInt32        Erase(const K& k)              { return (UInt32)m_map.erase(k); }
    void          Clear()                        { m_map.clear(); }

    Map&       Std()       { return m_map; }
    const Map& Std() const { return m_map; }

private:
    Map m_map;
};

template <class K, class C = Less<K> >
class BasicSet {
public:
    typedef std::set<K, C>               Set;
    typedef typename Set::iterator       Iterator;
    typedef typename Set::const_iterator ConstIterator;

    BasicSet() {}
    virtual ~BasicSet() {}

    UInt32        GetSize() const { return (UInt32)m_set.size(); }
    bool          IsEmpty() const { return m_set.empty(); }
    Iterator      Begin()         { return m_set.begin(); }
    Iterator      End()           { return m_set.end(); }
    ConstIterator Begin() const   { return m_set.begin(); }
    ConstIterator End() const     { return m_set.end(); }
    Iterator      Find(const K& k)       { return m_set.find(k); }
    ConstIterator Find(const K& k) const { return m_set.find(k); }
    bool          Contains(const K& k) const { return m_set.find(k) != m_set.end(); }
    bool          Insert(const K& k) { return m_set.insert(k).second; }
    void          Erase(Iterator it) { m_set.erase(it); }
    UInt32        Erase(const K& k)  { return (UInt32)m_set.erase(k); }
    void          Clear()            { m_set.clear(); }

    Set&       Std()       { return m_set; }
    const Set& Std() const { return m_set; }

private:
    Set m_set;
};

template <class K, class C = Less<K> >
class BasicMultiset {
public:
    typedef std::multiset<K, C>          Set;
    typedef typename Set::iterator       Iterator;
    typedef typename Set::const_iterator ConstIterator;

    BasicMultiset() {}
    virtual ~BasicMultiset() {}

    UInt32   GetSize() const { return (UInt32)m_set.size(); }
    Iterator Begin()         { return m_set.begin(); }
    Iterator End()           { return m_set.end(); }
    void     Insert(const K& k) { m_set.insert(k); }
    void     Clear()            { m_set.clear(); }
    // Removes every element equal to `k`; returns how many were removed (MacOSX 0x32bbe0).
    UInt32   Erase(const K& k)  { return (UInt32)m_set.erase(k); }
    UInt32   Count(const K& k) const { return (UInt32)m_set.count(k); }
    Set&     Std()              { return m_set; }

private:
    Set m_set;
};

// vptr + std::multimap (RTTI BasicMultimap<Pair<UInt32, UInt32>, UInt32, Less<…>>, BrickDatabase
// decoration mapping, Mac vtable 0x7620a8).
template <class K, class V, class C = Less<K> >
class BasicMultimap {
public:
    typedef std::multimap<K, V, C>               Map;
    typedef typename Map::iterator               Iterator;
    typedef typename Map::const_iterator         ConstIterator;

    BasicMultimap() {}
    virtual ~BasicMultimap() {}

    UInt32        GetSize() const { return (UInt32)m_map.size(); }
    bool          IsEmpty() const { return m_map.empty(); }
    Iterator      Begin()         { return m_map.begin(); }
    Iterator      End()           { return m_map.end(); }
    ConstIterator Begin() const   { return m_map.begin(); }
    ConstIterator End() const     { return m_map.end(); }
    Iterator      Find(const K& k)       { return m_map.find(k); }
    ConstIterator Find(const K& k) const { return m_map.find(k); }
    ConstIterator LowerBound(const K& k) const { return m_map.lower_bound(k); }
    ConstIterator UpperBound(const K& k) const { return m_map.upper_bound(k); }
    UInt32        Count(const K& k) const { return (UInt32)m_map.count(k); }
    // std::multimap::insert: after the last element with an equal key.
    Iterator      Insert(const K& k, const V& v) { return m_map.insert(std::make_pair(k, v)); }
    void          Clear()                        { m_map.clear(); }
    Map&       Std()       { return m_map; }
    const Map& Std() const { return m_map; }

private:
    Map m_map;
};

} // namespace LEGO
