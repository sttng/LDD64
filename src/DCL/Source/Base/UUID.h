// LEGO Digital Designer — restored source
// LEGO::UUID (names inferred), not polymorphic. TU 172 Mac 0x230fa0-0x23128f (CFUUID); Win
// 0x41f540-0x41f730 (RPCRT4). 36-character text, upper-case hex on Mac, lower-case
// on Win. Users: AppMain session id, GroupToLibraryCommand (Mac 0x26fd70), single-instance
// hand-off (Win 0x5c7e5b / 0x5c7ef1).
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"

#if defined(_WIN32)
#include <rpc.h>
#else
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace LEGO {

class UUID {
public:
    // The nil UUID (Mac: no CFUUID; Win: UuidCreateNil).
    UUID();                                                   // Mac 0x230fa0, Win 0x41f540
    // NOTE: no destructor in either build: the Mac CFUUID is never released (leak, as the
    // original; FromString and Generate release a previous one). Fixed: released, and copies
    // retain it (low).
#if !defined(_WIN32)
    ~UUID();
    UUID(const UUID& other);
    UUID& operator=(const UUID& other);
#endif

    // A new random UUID. (name inferred)
    void Generate();                                          // Mac 0x230fb0, Win 0x41f550
    // The 36-character form; empty for the nil UUID. (name inferred)
    void GetString(String& text) const;                       // Mac 0x230fe0, Win 0x41f560

private:
    friend bool FromString(UUID& value, const String& text, UInt32& position);
#if defined(_WIN32)
    ::UUID      m_uuid;   // Win +0x00 (16 bytes)
#else
    CFUUIDRef   m_uuid;   // Mac +0x00 (null = nil)
#endif
};

// Appends GetString(). (name inferred)
String& operator<<(String& text, const UUID& uuid);                          // Mac 0x231070, Win 0x41f5e0
// ToString / FromString overloads (StringStream.h family). FromString reads 36 characters at
// `position`; false (and the nil UUID) when they are missing or invalid (Mac also rejects an
// all-zero UUID; Win parses the whole rest of the text with UuidFromStringW).
void ToString(const UUID& value, String& out);                               // Mac 0x2310e0, Win 0x41f660
bool FromString(UUID& value, const String& text, UInt32& position);          // Mac 0x231140, Win 0x41f6d0

} // namespace LEGO
