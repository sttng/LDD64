// LEGO Digital Designer — restored source
// TU 170 (Mac 0x22b970-0x22bccf; file name inferred): the string conversions of the palette
// name set (PaletteNames.h). Static initialiser Mac 0x22bc90 = `#include <iostream>`.
// Win: no out-of-line copies (inlined into LDDApplication::LoadPalettes / the start mode).
#include "PaletteNames.h"
#include "BasicArray.h"
#include "StringStream.h"

#include <iostream>

namespace LEGO {

// Mac 0x22b970
void ToString(const PaletteNameSet& names, String& out)
{
    for (PaletteNameSet::ConstIterator it = names.Begin(); it != names.End();) {
        String version;
        ToString(it->m_version, version);
        out << it->m_name << "|" << version;
        ++it;
        if (it == names.End())
            break;
        out << ",";
    }
}

// Mac 0x22ba50
bool FromString(PaletteNameSet& names, const String& text)
{
    names.Clear();
    BasicArray<String> entries;
    text.Split(UniChar16(','), entries);
    for (BasicArray<String>::ConstIterator it = entries.Begin(); it != entries.End(); ++it) {
        const UInt32 bar = it->Find(UniChar16('|'), 0);
        UInt32 major;
        if (bar == String::npos || !FromString(major, it->Substring(bar + 1, String::npos)))
            major = 0;
        names.Insert(PaletteName(it->Substring(0, bar), Version(major, 0, 0, 0)));
    }
    return true;
}

} // namespace LEGO
