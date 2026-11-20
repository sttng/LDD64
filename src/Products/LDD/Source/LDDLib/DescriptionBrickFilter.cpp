// LEGO Digital Designer — restored source
// BrickFilter family + StringMatch + DescriptionBrickFilter (Mac TU 073), see BrickFilter.h and
// DescriptionBrickFilter.h.
#include "DescriptionBrickFilter.h"
#include "LDDUtilities.h"

namespace LEGO {

// ===========================================================================================
// CompositeBrickFilter
// ===========================================================================================

// Mac 0x164650, Win 0x65f461
bool CompositeBrickFilter::Accept(const BrickPalette::Brick* brick) const
{
    for (BasicArray<RefPointer<const BrickFilter> >::ConstIterator it = m_filters.Begin(); it != m_filters.End(); ++it)
        if (!(*it)->Accept(brick))
            return false;
    return true;
}

// Mac 0x1646a0, Win 0x65f48b
bool CompositeBrickFilter::IsCounting() const
{
    for (BasicArray<RefPointer<const BrickFilter> >::ConstIterator it = m_filters.Begin(); it != m_filters.End(); ++it)
        if ((*it)->IsCounting())
            return true;
    return false;
}

// Mac 0x1646f0, Win 0x65f4b0
UInt32 CompositeBrickFilter::GetCount(const BrickPalette::Brick* brick) const
{
    UInt32 count = 0;
    for (BasicArray<RefPointer<const BrickFilter> >::ConstIterator it = m_filters.Begin(); it != m_filters.End(); ++it)
        if ((*it)->IsCounting())
            count += (*it)->GetCount(brick);
    return count;
}

// Mac 0x165960 (array helper)
void CompositeBrickFilter::EraseUnordered(RefPointer<const BrickFilter>* position)
{
    RefPointer<const BrickFilter>* end = m_filters.End();
    if (position == end)
        return;
    if (position != end - 1)
        *position = *(end - 1);
    m_filters.PopBack();
}

// Mac 0x164250
void CompositeBrickFilter::RemoveFiltersOfType(const BrickFilter* filter)
{
    RefPointer<const BrickFilter>* it = m_filters.Begin();
    while (it != m_filters.End()) {
        if (filter->GetType() != (*it)->GetType()) {
            ++it;
            continue;
        }
        RefPointer<const BrickFilter> found = *it;
        // std::find of the same filter from the start (its first occurrence)
        RefPointer<const BrickFilter>* first = m_filters.Begin();
        while (first != m_filters.End() && first->Get() != found.Get())
            ++first;
        EraseUnordered(first);
        it = first;
    }
}

// Mac 0x1641b0, Win 0x65f9f4
void CompositeBrickFilter::SetFilter(const BrickFilter* filter)
{
    RemoveFiltersOfType(filter);
    m_filters.PushBack(RefPointer<const BrickFilter>(filter));
}

// Mac 0x164500
void CompositeBrickFilter::RemoveFilter(const BrickFilter* filter)
{
    RefPointer<const BrickFilter> keep(filter);
    RefPointer<const BrickFilter>* it = m_filters.Begin();
    while (it != m_filters.End() && it->Get() != filter)
        ++it;
    EraseUnordered(it);
}

// Mac 0x164600
void CompositeBrickFilter::Clear()
{
    m_filters.Clear();
}

// ===========================================================================================
// MaterialBrickFilter
// ===========================================================================================

// Mac 0x164750, Win 0x65f4e4
bool MaterialBrickFilter::Accept(const BrickPalette::Brick* brick) const
{
    for (BasicArray<RefPointer<BrickPalette::Part> >::ConstIterator p = brick->m_parts.Begin(); p != brick->m_parts.End(); ++p) {
        const BasicArray<UInt32>& materials = (*p)->m_materials;
        for (BasicArray<UInt32>::ConstIterator m = materials.Begin(); m != materials.End(); ++m)
            if (*m == m_material)
                return true;
    }
    return false;
}

// ===========================================================================================
// StringMatch
// ===========================================================================================

static bool IsSeparatorChar(UniChar16 c)
{
    return c == '/' || c == 'x' || String::IsDigit(c);
}

// Mac 0x164f70, Win 0x65f377
StringMatch::StringMatch(const String& pattern)
    : m_pattern(pattern), m_rows(0), m_isBlank(0), m_isSeparator(0)
{
    UInt32 length = m_pattern.GetLength();
    m_length  = length;
    m_rowSize = length + 1;
    m_rows        = new UInt8[length * 2 + 2];
    m_isBlank     = new UInt8[length];
    m_isSeparator = new UInt8[length];
    for (UInt32 i = 0; i < m_length; ++i) {
        m_isBlank[i] = m_pattern.IsWhitespaceAt(i);
        m_isSeparator[i] = IsSeparatorChar(m_pattern.GetAt(i));
    }
}

// Mac 0x165090, Win 0x65f257
// NOTE: the original frees the new[] blocks with scalar operator delete (both builds).
StringMatch::~StringMatch()
{
    delete[] m_isSeparator;
    delete[] m_isBlank;
    delete[] m_rows;
}

// Mac 0x164b80, Win 0x65f520. Byte arithmetic throughout (costs wrap modulo 256 like the original).
bool StringMatch::Match(const String& text, UInt8 maxCost) const
{
    UInt32 length = text.GetLength();
    const UniChar16* t = text.GetBuffer();
    const UniChar16* p = m_pattern.GetBuffer();
    UInt32 longer = m_length < length ? length : m_length;
    UInt32 limit = longer * 2 < 100 ? longer * 2 : 100;
    if (length == 0)
        return limit <= maxCost;

    m_rows[0] = 0;
    m_rows[m_rowSize] = 0;
    UInt8 value = 0;
    for (UInt32 i = 0; i < m_rowSize; ++i, value = (UInt8)(value + 2))
        m_rows[i] = value;

    UInt8 best = (UInt8)limit;
    for (UInt32 k = 0; k < length; ++k) {
        UInt8* cur  = m_rows + (((k + 1) & 1) ? m_rowSize : 0);
        UInt8* prev = m_rows + ((k & 1) ? m_rowSize : 0);
        UniChar16 c = t[k];
        bool separator = IsSeparatorChar(c);
        UInt8 blank = String::IsWhitespace(c) ? 1 : 0;
        UInt8 insertCost = (UInt8)((separator ? 4 : 2) - blank);
        bool plain = !separator && !blank;
        for (UInt32 j = 0; j < m_length; ++j) {
            if (c == p[j]) {
                cur[j + 1] = prev[j];
                continue;
            }
            UInt8 substitute;
            if (plain)
                substitute = (UInt8)(prev[j] + ((m_isSeparator[j] || m_isBlank[j]) ? 4 : 2));
            else
                substitute = (UInt8)(prev[j] + 4);
            UInt8 remove = (UInt8)(cur[j] + (m_isSeparator[j] ? 4 : 2) - m_isBlank[j]);
            UInt8 insert = (UInt8)(prev[j + 1] + insertCost);
            if (remove >= 100)
                remove = 100;
            if (insert >= substitute)
                insert = substitute;
            cur[j + 1] = remove < insert ? remove : insert;
        }
        if (cur[m_length] <= best)
            best = cur[m_length];
        if (best <= maxCost)
            return true;
    }
    return best <= maxCost;
}

// ===========================================================================================
// DescriptionBrickFilter
// ===========================================================================================

// Mac 0x1648e0, Win 0x660077
DescriptionBrickFilter::DescriptionBrickFilter(const BrickDatabase* database, const String& text)
    : BrickFilter(kTypeDescription), m_database(database)
{
    SetText(text);
}

// Mac 0x1647c0
void DescriptionBrickFilter::SetText(const String& text)
{
    m_results.Clear();
    m_text = text;
    m_text.ToLower();
    StringMatch* match = 0;
    if (!m_text.IsEmpty())
        match = new StringMatch(m_text);
    m_match = match;
}

// Mac 0x164a10
void DescriptionBrickFilter::ClearCaches()
{
    m_descriptions.Clear();
    m_results.Clear();
}

// Mac 0x164e80
const String& DescriptionBrickFilter::GetDescription(const BrickPalette::Brick* brick) const
{
    BasicMap<const BrickPalette::Brick*, String>::Iterator found = m_descriptions.Find(brick);
    if (found != m_descriptions.End())
        return found->second;
    String description = LDDUtilities::GetBrickDescription(m_database, brick);
    description.ToLower();
    m_descriptions[brick] = description;
    return m_descriptions[brick];
}

// Mac 0x164a70, Win 0x6601b5
bool DescriptionBrickFilter::Accept(const BrickPalette::Brick* brick) const
{
    if (m_match.IsNull())
        return true;
    BasicMap<const BrickPalette::Brick*, bool>::Iterator found = m_results.Find(brick);
    if (found != m_results.End())
        return found->second;
    bool result = m_match->Match(GetDescription(brick), 2);
    m_results[brick] = result;
    return result;
}

} // namespace LEGO
