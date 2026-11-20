// LEGO Digital Designer — restored source
// LEGO::BrickMatcher — finds the bricks of a candidate list that are closest to a brick of the
// same design: by mismatched decorations, then by transparency differences, then by the summed
// squared RGB distance of the materials (BrickMatcher::BrickDistance, compared in that order).
//
// Mac TU 271 "BrickMatcher.cpp" (0x285640-0x28647f); Win 0x5ace60-0x5ad73f.
// RTTI: class BrickMatcher : virtual Object. Mac sizeof 0x20: vptr +0 (vtable 0x73e51c
//   {D1 0x2860e0, D0 0x2861b0}), candidates +0x04, database +0x14, Object +0x18.
//   Win: vbptr layout, vftable 0xa2cc20 {sdd 0x5ace60}, destructor 0x5ad110.
// RTTI: class BrickMatcher::BrickDistance (key of BasicMultimap<BrickDistance,
//   RefPointer<AbstractBrick const>, Less<BrickDistance>>, Mac vtable 0x774c48).
// Method and member names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "Brick.h"

namespace LEGO {

class BrickDatabase;

class BrickMatcher : public virtual Object {
public:
    class BrickDistance {
    public:
        BrickDistance() : m_decorations(0), m_transparency(0), m_colors(0) {}
        // Lexicographic. (Mac inlined 0x2862e0)
        bool operator<(const BrickDistance& o) const
        {
            if (m_decorations != o.m_decorations)
                return m_decorations < o.m_decorations;
            if (m_transparency != o.m_transparency)
                return m_transparency < o.m_transparency;
            return m_colors < o.m_colors;
        }

        UInt32 m_decorations;    // +0x00 differing decorations
        UInt32 m_transparency;   // +0x04 materials transparent on one side only (alpha < 1)
        Real32 m_colors;         // +0x08 sum of squared diffuse RGB differences
    };

    // Mac 0x285640
    explicit BrickMatcher(const BrickDatabase* database) : m_database(database) {}
    virtual ~BrickMatcher() {}                                                 // Mac 0x2860e0 / 0x2861b0, Win 0x5ad110 / sdd 0x5ace60

    // Mac 0x2856a0
    void AddCandidate(const AbstractBrick* brick) { m_candidates.PushBack(RefPointer<const AbstractBrick>(brick)); }
    const BasicArray<RefPointer<const AbstractBrick> >& GetCandidates() const { return m_candidates; }

    // Appends the candidates of `brick`'s design that differ from it to `matches`, closest
    // first (equal distances in candidate order). Exact matches are not appended: the result
    // tells whether there was one. Mac 0x285870, Win 0x5acf30
    bool FindMatches(const AbstractBrick* brick, BasicArray<RefPointer<const AbstractBrick> >& matches) const;

    // 2: other design; 0: identical; 1: different (`distance` filled). Mac 0x285a60
    UInt32 Compare(const AbstractBrick* candidate, const AbstractBrick* brick, BrickDistance& distance) const;

private:
    BasicArray<RefPointer<const AbstractBrick> > m_candidates;   // Mac +0x04
    const BrickDatabase*                          m_database;     // Mac +0x14
};

} // namespace LEGO
