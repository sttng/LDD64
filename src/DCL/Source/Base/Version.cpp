// LEGO Digital Designer — restored source
// LEGO::Version (TU 176, Mac 0x233780-0x233abf). See Version.h. (file name inferred)
#include "Version.h"
#include "StringStream.h"

#include <iostream>

namespace LEGO {

bool Version::operator<(const Version& v) const                                   // Mac 0x233780
{
    if (m_major < v.m_major)
        return true;
    if (m_major == v.m_major) {
        if (m_minor < v.m_minor)
            return true;
        if (m_minor == v.m_minor) {
            if (m_build < v.m_build)
                return true;
            if (m_build == v.m_build)
                return m_revision < v.m_revision;
        }
    }
    return false;
}

bool Version::operator<=(const Version& v) const                                  // Mac 0x2337d0
{
    if (v.m_major < m_major)
        return false;
    if (m_major == v.m_major) {
        if (v.m_minor < m_minor)
            return false;
        if (m_minor == v.m_minor) {
            if (v.m_build < m_build)
                return false;
            if (m_build == v.m_build)
                return m_revision <= v.m_revision;
        }
    }
    return true;
}

bool Version::operator>(const Version& v) const                                   // Mac 0x233810
{
    if (v.m_major < m_major)
        return true;
    if (m_major == v.m_major) {
        if (v.m_minor < m_minor)
            return true;
        if (m_minor == v.m_minor) {
            if (v.m_build < m_build)
                return true;
            if (m_build == v.m_build)
                return v.m_revision < m_revision;
        }
    }
    return false;
}

bool Version::operator>=(const Version& v) const                                  // Mac 0x233860
{
    if (m_major < v.m_major)
        return false;
    if (m_major == v.m_major) {
        if (m_minor < v.m_minor)
            return false;
        if (m_minor == v.m_minor) {
            if (m_build < v.m_build)
                return false;
            if (m_build == v.m_build)
                return v.m_revision <= m_revision;
        }
    }
    return true;
}

bool Version::operator!=(const Version& v) const                                  // Mac 0x2338a0
{
    return m_major != v.m_major || m_minor != v.m_minor || m_build != v.m_build ||
           m_revision != v.m_revision;
}

void ToString(const Version& value, String& out)                                  // Mac 0x2338d0, Win 0x53a440
{
    ToString(value.m_major, out);
    if (value.m_minor != 0 || value.m_build != 0 || value.m_revision != 0) {
        out << ".";
        ToString(value.m_minor, out);
        if (value.m_build != 0 || value.m_revision != 0) {
            out << ".";
            ToString(value.m_build, out);
            if (value.m_revision != 0) {
                out << ".";
                ToString(value.m_revision, out);
            }
        }
    }
}

bool FromString(Version& value, const String& text, UInt32& position)              // Mac 0x233970
{
    if (!FromString(value.m_major, text, position))
        return false;
    if (position == text.GetLength() || !text.IsCharAt(position, '.'))
        return true;
    ++position;
    if (!FromString(value.m_minor, text, position))
        return false;
    if (position == text.GetLength() || !text.IsCharAt(position, '.'))
        return true;
    ++position;
    if (!FromString(value.m_build, text, position))
        return false;
    if (position == text.GetLength() || !text.IsCharAt(position, '.'))
        return true;
    ++position;
    return FromString(value.m_revision, text, position);
}

} // namespace LEGO
