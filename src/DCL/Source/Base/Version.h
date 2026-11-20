// LEGO Digital Designer — restored source
// LEGO::Version — "major.minor.build.revision" (names inferred; no vtable, shape from the
// comparisons and "." formatting). TU 176, Mac 0x233780-0x233abf; Win: only ToString out of
// line (0x53a440).
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"

namespace LEGO {

// 16 bytes.
class Version {
public:
    Version() : m_major(0), m_minor(0), m_build(0), m_revision(0) {}
    Version(UInt32 major, UInt32 minor = 0, UInt32 build = 0, UInt32 revision = 0)
        : m_major(major), m_minor(minor), m_build(build), m_revision(revision) {}

    // Lexicographic over the four components.
    bool operator<(const Version& v) const;    // Mac 0x233780
    bool operator<=(const Version& v) const;   // Mac 0x2337d0
    bool operator>(const Version& v) const;    // Mac 0x233810
    bool operator>=(const Version& v) const;   // Mac 0x233860
    bool operator!=(const Version& v) const;   // Mac 0x2338a0
    bool operator==(const Version& v) const { return !(*this != v); }   // (not located)

    UInt32 m_major;      // +0x00
    UInt32 m_minor;      // +0x04
    UInt32 m_build;      // +0x08
    UInt32 m_revision;   // +0x0c
};

// Appends "major[.minor[.build[.revision]]]": trailing zero components are left out.
void ToString(const Version& value, String& out);                        // Mac 0x2338d0, Win 0x53a440
// Parses up to four '.'-separated numbers at `position`. Stops successfully at the end of
// `text` or at any character other than '.'; components not present are left unchanged.
bool FromString(Version& value, const String& text, UInt32& position);   // Mac 0x233970
// From the start of `text`. Inlined into GetXMLAttribute<Version> (BrickDatabase.cpp info.xml:
// Mac 0x7d900, Win 0x4c7200).
inline bool FromString(Version& value, const String& text)
{
    UInt32 position = 0;
    return FromString(value, text, position);
}

} // namespace LEGO
