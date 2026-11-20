// LEGO Digital Designer — restored source
// BrickMatcher (Mac TU 271), see BrickMatcher.h.
#include "BrickMatcher.h"
#include "BrickDatabase.h"
#include "Part.h"

namespace LEGO {

// Mac 0x285a60 (named after its multimap by the call-graph spreading)
UInt32 BrickMatcher::Compare(const AbstractBrick* candidate, const AbstractBrick* brick, BrickDistance& distance) const
{
    if (brick->GetDesignID() != candidate->GetDesignID())
        return 2;

    BasicArray<const AbstractPart*> parts;
    for (UInt32 i = 0; i < candidate->GetPartCount(); ++i)
        parts.PushBack(candidate->GetPart(i));

    const Real32 opaque = 1.0f;
    for (UInt32 j = 0; j < brick->GetPartCount(); ++j) {
        const AbstractPart* target = brick->GetPart(j);
        UInt32 index = 0;
        while (index < parts.GetSize() && parts[index]->GetDesignID() != target->GetDesignID())
            ++index;
        if (index == parts.GetSize())
            continue;
        const AbstractPart* part = parts[index];
        // NOTE: the candidate part is read with the target part's material count (no range
        // check on its side; both builds).
        for (UInt32 i = 0; i < target->GetMaterialCount(); ++i) {
            MaterialColor a;
            a.shininess = 0;
            MaterialColor b;
            b.shininess = 0;
            m_database->GetMaterialColor(part->GetMaterial(i), a);
            m_database->GetMaterialColor(target->GetMaterial(i), b);
            distance.m_transparency += (b.diffuse.a < opaque) != (a.diffuse.a < opaque);
            Real32 dr = a.diffuse.r - b.diffuse.r;
            Real32 dg = a.diffuse.g - b.diffuse.g;
            Real32 db = a.diffuse.b - b.diffuse.b;
            Real32 sum = dg * dg + dr * dr;
            sum = db * db + sum;
            distance.m_colors = sum + distance.m_colors;
        }
        // NOTE: decorations 1 .. material count - 1 (not the decoration count; both builds)
        for (UInt32 i = 1; i < target->GetMaterialCount(); ++i)
            distance.m_decorations += part->GetDecoration(i) != target->GetDecoration(i);
        parts.Erase(index);
    }
    return (distance.m_decorations != 0 || distance.m_transparency != 0 || distance.m_colors != 0.0f) ? 1 : 0;
}

// Mac 0x285870, Win 0x5acf30
bool BrickMatcher::FindMatches(const AbstractBrick* brick, BasicArray<RefPointer<const AbstractBrick> >& matches) const
{
    BasicMultimap<BrickDistance, RefPointer<const AbstractBrick>, Less<BrickDistance> > sorted;
    bool exact = false;
    if (m_candidates.IsEmpty())
        return false;
    for (BasicArray<RefPointer<const AbstractBrick> >::ConstIterator it = m_candidates.Begin(); it != m_candidates.End(); ++it) {
        BrickDistance distance;
        UInt32 result = Compare(*it, brick, distance);
        if (result == 0)
            exact = true;
        else if (result == 1)
            sorted.Insert(distance, *it);
    }
    for (BasicMultimap<BrickDistance, RefPointer<const AbstractBrick>, Less<BrickDistance> >::ConstIterator it = sorted.Begin();
         it != sorted.End(); ++it)
        matches.PushBack(it->second);
    return exact;
}

} // namespace LEGO
