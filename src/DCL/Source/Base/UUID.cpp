// LEGO Digital Designer — restored source
// TU 172 "UUID.cpp" (file name inferred) — Mac 0x230fa0-0x23128f; Win twin 0x41f540-0x41f730.
// See UUID.h. Mac static initialiser 0x231250: <iostream> only.
#include "UUID.h"
#include "StringConversion.h"

#include <iostream>

#if defined(_WIN32)
#if defined(_MSC_VER) || defined(__clang__)
#pragma comment(lib, "rpcrt4")
#endif
#endif

namespace LEGO {

// Mac 0x230fa0, Win 0x41f540
UUID::UUID()
{
#if defined(_WIN32)
    ::UuidCreateNil(&m_uuid);
#else
    m_uuid = 0;
#endif
}

#if !defined(_WIN32)
// (not in the original; see UUID.h)
UUID::~UUID()
{
    if (m_uuid)
        CFRelease(m_uuid);
}

UUID::UUID(const UUID& other) : m_uuid(other.m_uuid)
{
    if (m_uuid)
        CFRetain(m_uuid);
}

UUID& UUID::operator=(const UUID& other)
{
    if (other.m_uuid)
        CFRetain(other.m_uuid);
    if (m_uuid)
        CFRelease(m_uuid);
    m_uuid = other.m_uuid;
    return *this;
}
#endif

// Mac 0x230fb0, Win 0x41f550
void UUID::Generate()
{
#if defined(_WIN32)
    ::UuidCreate(&m_uuid);
#else
    if (m_uuid)
        CFRelease(m_uuid);
    m_uuid = CFUUIDCreate(0);
#endif
}

// Mac 0x230fe0, Win 0x41f560 (UUID in ESI)
void UUID::GetString(String& text) const
{
#if defined(_WIN32)
    text = String();
    RPC_STATUS status;
    if (!::UuidIsNil(const_cast< ::UUID*>(&m_uuid), &status)) {
        RPC_CSTR s = 0;
        ::UuidToStringA(const_cast< ::UUID*>(&m_uuid), &s);
        text = (const char*)s;
        ::RpcStringFreeA(&s);
    }
#else
    text.Clear();
    if (m_uuid) {
        CFStringRef s = CFUUIDCreateString(0, m_uuid);
        text = String(s);
        CFRelease(s);
    }
#endif
}

// Mac 0x231070, Win 0x41f5e0
String& operator<<(String& text, const UUID& uuid)
{
    String s;
    uuid.GetString(s);
    return text << s;
}

// Mac 0x2310e0, Win 0x41f660
void ToString(const UUID& value, String& out)
{
    String s;
    value.GetString(s);
    out << s;
}

// Mac 0x231140, Win 0x41f6d0
bool FromString(UUID& value, const String& text, UInt32& position)
{
#if defined(_WIN32)
    if (text.GetLength() < position + 36) {
        ::UuidCreateNil(&value.m_uuid);
        return false;
    }
    // NOTE: UuidFromStringW sees the whole rest of the text, not just 36 characters
    if (::UuidFromStringW((RPC_WSTR)(text.GetBuffer() + position), &value.m_uuid) != RPC_S_OK) {
        ::UuidCreateNil(&value.m_uuid);
        return false;
    }
    position += 36;
    return true;
#else
    if (value.m_uuid) {
        CFRelease(value.m_uuid);
        value.m_uuid = 0;
    }
    if (text.GetLength() < position + 36)
        return false;
    CFStringRef s = CreateCFSubstring(text, position, 36);
    value.m_uuid = CFUUIDCreateFromString(0, s);
    CFRelease(s);
    const CFUUIDBytes bytes = CFUUIDGetUUIDBytes(value.m_uuid);
    const UInt8* b = (const UInt8*)&bytes;
    bool isNil = true;   // (not `nil`: a macro of the Mac SDK's MacTypes.h)
    for (UInt32 i = 0; i < 16; ++i)
        if (b[i] != 0)
            isNil = false;
    if (isNil) {
        CFRelease(value.m_uuid);
        value.m_uuid = 0;
        return false;
    }
    position += 36;
    return true;
#endif
}

} // namespace LEGO
