// LEGO Digital Designer — restored source
// LEGO::DescriptionBrickFilter — the brick palette's text search: a brick passes when the
// search text approximately occurs in its description (LDDUtilities::GetBrickDescription,
// lower case). LEGO::StringMatch — the approximate substring matcher: a weighted edit distance
// (substitution 2, or 4 for digits / '/' / 'x' / blanks; insertion and deletion 2, 4 for
// digits / '/' / 'x', 1 for blanks), free leading and trailing text, limited to
// min(2 * longer length, 100).
//
// Mac TU 073 "DescriptionBrickFilter.cpp" (0x1641b0-0x16610b, with BrickFilter.h's classes);
// Win 0x65f257-0x66024b.
//
// RTTI: class StringMatch : Object. Mac sizeof 0x2c (constructor 0x164f70, vtable 0x7112d4
//   {D1 0x165110, D0 0x165120}); Win vftable 0x9d7300 {sdd 0x65f424}, constructor 0x65f377.
// RTTI: class DescriptionBrickFilter : BrickFilter. Mac sizeof 0x60 (Object +0x58, vtable
//   0x71127c {Accept 0x164a70, 0x165160, 0x165170, D1 0x165580, D0 0x1655e0}); Win constructor
//   0x660077, vftable 0x9d7338 {0x6601b5, …}, sdd 0x66024b.
// Method and member names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicMap.h"
#include "BrickFilter.h"

namespace LEGO {

class BrickDatabase;

class StringMatch : public Object {
public:
    // Mac 0x164f70, Win 0x65f377
    explicit StringMatch(const String& pattern);
    // Mac 0x165090 (D1 0x165110, D0 0x165120), Win 0x65f257 / sdd 0x65f424
    virtual ~StringMatch();

    // The pattern occurs in `text` at a cost <= `maxCost` (see the header comment); an empty
    // text matches when min(2 * pattern length, 100) <= maxCost. Mac 0x164b80, Win 0x65f520 (same costs, written as increments)
    bool Match(const String& text, UInt8 maxCost) const;

    const String& GetPattern() const { return m_pattern; }

private:
    StringMatch(const StringMatch&);
    StringMatch& operator=(const StringMatch&);

    String  m_pattern;        // Mac +0x08
    UInt8*  m_rows;           // +0x18 two rows of m_rowSize bytes (rewritten by Match)
    UInt8*  m_isBlank;        // +0x1c per pattern character: IsWhitespaceAt
    UInt8*  m_isSeparator;    // +0x20 per pattern character: '/', 'x' or a digit
    UInt32  m_rowSize;        // +0x24 length + 1
    UInt32  m_length;         // +0x28
};

class DescriptionBrickFilter : public BrickFilter {
public:
    // Mac 0x1648e0, Win 0x660077
    DescriptionBrickFilter(const BrickDatabase* database, const String& text);
    virtual ~DescriptionBrickFilter() {}                                       // Mac 0x165580 / 0x1655e0, Win sdd 0x66024b

    // Without search text every brick passes; else the (cached) StringMatch result with
    // maximum cost 2 over the (cached) lower-case description. Mac 0x164a70, Win 0x6601b5
    virtual bool Accept(const BrickPalette::Brick* brick) const;

    // New search text (lower-cased); clears the results. Mac 0x1647c0
    void SetText(const String& text);
    // Forgets descriptions and results. Mac 0x164a10
    void ClearCaches();
    const String& GetText() const { return m_text; }

private:
    // The cached lower-case description. Mac 0x164e80
    const String& GetDescription(const BrickPalette::Brick* brick) const;

    String                                                  m_text;          // Mac +0x08
    const BrickDatabase*                                    m_database;      // +0x18
    RefPointer<StringMatch>                                 m_match;         // +0x1c null for an empty text
    mutable BasicMap<const BrickPalette::Brick*, String>    m_descriptions;  // +0x20
    mutable BasicMap<const BrickPalette::Brick*, bool>      m_results;       // +0x3c
};

} // namespace LEGO
