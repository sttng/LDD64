// LEGO Digital Designer — restored source
// ToString / FromString overloads (file and names inferred). Mac TU 061 (0x1423d0-0x142b7f): 25
// weak inline functions, no own code (static init 0x142b40); Win: all inlined (func_match pairs
// Mac 0x142530 with String::operator<<(UInt32) 0x402380).
// ToString appends to `out`; FromString parses at `position` (advanced) or the start of `text`.
// Components are separated by exactly one arbitrary character ("1 2", "1,2"); the Optional forms
// take "#" (then ',', whitespace or end) for "unchanged". Preferences.cpp: ParseBool (Mac 0x142710).
#pragma once
#include "LEGO/String.h"
#include "Geometry.h"

namespace LEGO {

inline void ToString(const String& value, String& out) { out << value; }          // Mac 0x1423d0
inline void ToString(const char* value, String& out) { out << String(value); }    // Mac 0x1423f0

inline bool FromString(String& value, const String& text, UInt32& position)        // Mac 0x142450
{
    value = text.Substring(position, String::npos);
    position = text.GetLength();
    return true;
}

inline bool FromString(String& value, const String& text)                          // Mac 0x1424e0
{
    value = text;
    return true;
}

inline bool FromString(UInt8& value, const String& text)                           // Mac 0x142500
{
    UInt32 position = 0;
    return text.Parse(value, position);
}

inline void ToString(UInt32 value, String& out) { out << value; }                  // Mac 0x142530
inline bool FromString(UInt32& value, const String& text, UInt32& position)        // Mac 0x142550
{
    return text.Parse(value, position);
}
inline bool FromString(UInt32& value, const String& text)                          // Mac 0x142570
{
    UInt32 position = 0;
    return text.Parse(value, position);
}

inline bool FromString(UInt64& value, const String& text)                          // Mac 0x1425a0
{
    UInt32 position = 0;
    return text.Parse(value, position);
}

inline void ToString(Real32 value, String& out) { out << value; }                  // Mac 0x1425d0
inline bool FromString(Real32& value, const String& text, UInt32& position)        // Mac 0x142600
{
    return text.Parse(value, position);
}
inline bool FromString(Real32& value, const String& text)                          // Mac 0x142620
{
    UInt32 position = 0;
    return text.Parse(value, position);
}

inline void ToString(Real64 value, String& out) { out << value; }                  // Mac 0x142650
inline bool FromString(Real64& value, const String& text)                          // Mac 0x142680
{
    UInt32 position = 0;
    return text.Parse(value, position);
}

// "yes" / "no" (String::operator<<(bool) writes "1" / "0").
inline void ToString(const bool& value, String& out) { out << (value ? "yes" : "no"); } // Mac 0x1426b0
inline bool FromString(bool& value, const String& text, UInt32& position)          // Mac 0x1426f0
{
    return text.Parse(value, position);
}
inline bool FromString(bool& value, const String& text)                            // Mac 0x142710
{
    UInt32 position = 0;
    return text.Parse(value, position);
}

inline void ToString(const Vector2& value, String& out)                            // Mac 0x142740
{
    out << value.x << " " << value.y;
}

inline bool FromString(Vector2& value, const String& text, UInt32& position)       // Mac 0x1427a0
{
    if (!text.Parse(value.x, position))
        return false;
    ++position;
    return text.Parse(value.y, position);
}

// `keyword` (case-insensitive) at `position`, followed by `separator`, whitespace or the end
// of `text`; advances past the keyword.
inline bool ParseKeyword(const String& keyword, const String& text, UInt32& position,
                         UniChar16 separator)                                      // Mac 0x142aa0
{
    if (text.CompareNoCase(position, keyword.GetLength(), keyword) != 0)
        return false;
    UInt32 end = position + keyword.GetLength();
    if (text.GetLength() != end && text.GetAt(end) != separator && !text.IsWhitespaceAt(end))
        return false;
    position = end;
    return true;
}

inline bool FromStringOptional(Real32& value, const String& text, UInt32& position) // Mac 0x1427f0
{
    if (position >= text.GetLength())
        return false;
    if (ParseKeyword(String("#"), text, position, ','))
        return true;
    return text.Parse(value, position);
}

// (additive) "#" (keep) or a value; false at the end of `text`. Mac 0x2c06a0
// (bool, emitted in TU 315) / 0x2c4310 (UInt32, TU 315).
inline bool FromStringOptional(bool& value, const String& text, UInt32& position)
{
    if (position >= text.GetLength())
        return false;
    if (ParseKeyword(String("#"), text, position, ','))
        return true;
    return FromString(value, text, position);
}
inline bool FromStringOptional(UInt32& value, const String& text, UInt32& position)
{
    if (position >= text.GetLength())
        return false;
    if (ParseKeyword(String("#"), text, position, ','))
        return true;
    return FromString(value, text, position);
}
// (additive) The same for a String (the rest of `text`). Mac 0x2bd900 (emitted in
// TU 314).
inline bool FromStringOptional(String& value, const String& text, UInt32& position)
{
    if (position >= text.GetLength())
        return false;
    if (ParseKeyword(String("#"), text, position, ','))
        return true;
    return FromString(value, text, position);
}

inline bool FromString(Vector3& value, const String& text, UInt32& position)       // Mac 0x1428a0
{
    if (!text.Parse(value.x, position))
        return false;
    ++position;
    if (!text.Parse(value.y, position))
        return false;
    ++position;
    return text.Parse(value.z, position);
}

inline void ToString(const Rect& value, String& out)                               // Mac 0x142900
{
    ToString(value.m_min, out);
    out << " ";
    ToString(value.m_max, out);
}

inline bool FromString(Rect& value, const String& text, UInt32& position)          // Mac 0x1429a0
{
    if (!text.Parse(value.m_min.x, position))
        return false;
    ++position;
    if (!text.Parse(value.m_min.y, position))
        return false;
    ++position;
    if (!text.Parse(value.m_max.x, position))
        return false;
    ++position;
    return text.Parse(value.m_max.y, position);
}

inline bool FromStringOptional(Rect& value, const String& text, UInt32& position)  // Mac 0x142a20
{
    if (!FromStringOptional(value.m_min.x, text, position))
        return false;
    ++position;
    if (!FromStringOptional(value.m_min.y, text, position))
        return false;
    ++position;
    if (!FromStringOptional(value.m_max.x, text, position))
        return false;
    ++position;
    return FromStringOptional(value.m_max.y, text, position);
}

} // namespace LEGO
