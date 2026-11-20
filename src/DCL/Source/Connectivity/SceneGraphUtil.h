// LEGO Digital Designer — restored source
// Small array helpers of the connectivity scene graph (TU 025). Not an original header: the
// original inlines std::find / std::vector operations on the BasicArray members everywhere
// (the 4-way unrolled find loops of the Mac pseudo code are std::find; the erase helpers
// are BasicArray instances such as Mac 0x99d00, 0x9ba30, 0xd0c00, 0xd0ce0, 0xd1040, 0xd1190;
// PushBackUnique: Mac 0xa0bd0, 0xa3150).
#pragma once
#include "LEGO/Types.h"
#include "BasicArray.h"

namespace LEGO {
namespace SceneGraph {

// First element equal to `value` (End() if none). std::find
template <class T, class V>
inline typename BasicArray<T>::Iterator Find(BasicArray<T>& array, const V& value)
{
    typename BasicArray<T>::Iterator it = array.Begin();
    for (; it != array.End(); ++it)
        if (*it == value)
            break;
    return it;
}

template <class T, class V>
inline bool Contains(const BasicArray<T>& array, const V& value)
{
    for (typename BasicArray<T>::ConstIterator it = array.Begin(); it != array.End(); ++it)
        if (*it == value)
            return true;
    return false;
}

// Removes the first element equal to `value`, keeping the order (vector::erase(find)).
// Nothing happens when there is none.
template <class T, class V>
inline void EraseValue(BasicArray<T>& array, const V& value)
{
    typename BasicArray<T>::Iterator it = Find(array, value);
    if (it != array.End())
        array.Erase((UInt32)(it - array.Begin()));
}

// Removes the first element equal to `value` by moving the last element into its place
// (Mac 0x99d00 / 0x9ba30: *it = back, pop_back).
template <class T, class V>
inline void EraseValueUnordered(BasicArray<T>& array, const V& value)
{
    typename BasicArray<T>::Iterator it = Find(array, value);
    if (it == array.End())
        return;
    if (it != array.End() - 1)
        *it = array.Back();
    array.PopBack();
}

// The erase of the scene graph's merge / split / remove code. NOTE: platform difference: the Win
// original erases these arrays by moving the last element into the place (0x4a0bd0; callers
// Win 0x478040, 0x478440, 0x478650, 0x479310, 0x479790, 0x4799a0, 0x47a240, 0x47a340, 0x47a530,
// 0x48ccd0, 0x48cff0), the Mac original keeps the order. The orders are the walk order of
// Scene::Pick and of the connection searches, so they decide which connection a new brick makes
// first, and with it which rigid system's frame is kept.
template <class T, class V>
inline void EraseValueScene(BasicArray<T>& array, const V& value)
{
#if defined(_WIN32)
    EraseValueUnordered(array, value);
#else
    EraseValue(array, value);
#endif
}

// push_back unless already present (Mac 0xa0bd0 / 0xa3150).
template <class T>
inline void PushBackUnique(BasicArray<T>& array, const T& value)
{
    if (!Contains(array, value))
        array.PushBack(value);
}

// insert(end, range) (Mac 0x9a6c0 / 0x9bda0).
template <class T>
inline void Append(BasicArray<T>& array, const BasicArray<T>& other)
{
    for (typename BasicArray<T>::ConstIterator it = other.Begin(); it != other.End(); ++it)
        array.PushBack(*it);
}

} // namespace SceneGraph
} // namespace LEGO
