// LEGO Digital Designer — restored source
// LEGO::Country / LEGO::Countries — the country list LDDWebCommunicator reads from the server
//   (home TU 059 LDDWebCommunicator.cpp: destructor Mac 0x137160, PushBack 0x136f..); Format is
//   TU 194 (Countries.cpp).
// RTTI: Countries (si : BasicArray<Country>), BasicArray<Country> (Mac vtable 0x765c84,
//   Win sdd 0x632fd9). Country: 3 Strings (Mac sizeof 0x30). Member names inferred.
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "BasicArray.h"

namespace LEGO {

struct Country {
    String m_code;    // Mac +0x00 (written first by Format)
    String m_name;    // Mac +0x10
    String m_extra;   // Mac +0x20 (meaning unknown)
};

class Countries : public BasicArray<Country> {
public:
    Countries() {}
    virtual ~Countries() {}                                         // Mac 0x137160 …

    // "code|name,code|name,…" appended to `out` (BrickConnectSceneControl's version dump,
    // Mac 0x3b230). TU 194, Mac 0x2475e0 (Win inlined / not found). (name inferred)
    void Format(String& out) const;
};

} // namespace LEGO
