// LEGO Digital Designer — restored source
// LEGO::PaletteName / LEGO::PaletteNames — the palettes found at start-up (the name of each
// palette folder / archive and the version of its Info.baxml). LDDApplication::LoadPalettes
// (Mac 0x17b230) collects them through the PAXML loader and stores them as the preference
// "PaletteNames" ("LDD.lif|8,Mindstorms.lif|24,…"); the start mode and LDDWebCommunicator read
// them back (FromString) to compare with the palettes on the server.
//
// RTTI: Mac BasicSet<PaletteName, Less<PaletteName>> vtable 0x765c4c, PaletteNames vtable 0x765c28
// {D1 0x1337f0, D0 0x136ff0} (TU 059 "LDDWebCommunicator.cpp"); Win 0x9cdfb4 /
// 0x9cdfbc {sdd 0x6015f6}. PaletteNames adds nothing to the set (its destructor only restores the
// BasicSet vtable), so it is declared here with an inline destructor.
// The string conversions are TU 170 (Mac 0x22b970-0x22bccf; file name inferred: PaletteNames.cpp),
// Win inlined into their callers.
// Element layout Mac (set node + 0x10): String m_name +0x00, Version m_version +0x10 (0x20 bytes).
// Every name is inferred except the RTTI class names and the preference key.
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "BasicMap.h"
#include "Version.h"

namespace LEGO {

struct PaletteName {
    PaletteName() {}
    PaletteName(const String& name, const Version& version) : m_name(name), m_version(version) {}

    // Case-insensitive by name, then by version (the inlined Less<PaletteName> of the set
    // insert Mac 0x12e500 / 0x12e610).
    bool operator<(const PaletteName& other) const
    {
        if (m_name.LessNoCase(other.m_name))
            return true;
        return m_name.EqualsNoCase(other.m_name) && m_version < other.m_version;
    }

    String  m_name;      // +0x00 folder / archive name of the palette ("LDD.lif")
    Version m_version;   // +0x10 <Bag version> of its Info.baxml
};

typedef BasicSet<PaletteName, Less<PaletteName> > PaletteNameSet;

class PaletteNames : public PaletteNameSet {
public:
    PaletteNames() {}
    virtual ~PaletteNames() {}                                               // Mac 0x1337f0 / 0x136ff0, Win sdd 0x6015f6 (TU 059)
};

// Appends "name|version" for every palette, ',' separated (ToString(Version): trailing zero
// components left out). Mac 0x22b970 (TU 170)
void ToString(const PaletteNameSet& names, String& out);
// Replaces `names` by the entries of "name|major,name|major,…": only the major version is read
// (an entry without '|' or with a bad number gets version 0); always true. Mac 0x22ba50 (TU 170)
bool FromString(PaletteNameSet& names, const String& text);

} // namespace LEGO
