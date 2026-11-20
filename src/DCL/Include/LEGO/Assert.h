// LEGO Digital Designer — restored source
// Assertion-box state (TU 054, Mac 0x11acd0-0x11adb6; file and all names inferred — the TU
// has no strings). The mode is set from the preferences "dontshowassertbox" (1) and
// "dontshowassertboxcontinue" (2) by Preferences::Initialize; Win writes the global directly
// (Win 0x00b42758, inlined).
#pragma once
#include "LEGO/Types.h"

namespace LEGO {

enum AssertMode {
    kAssertShowBox          = 0,
    kAssertNoBox            = 1,   // "dontshowassertbox"
    kAssertNoBoxContinue    = 2    // "dontshowassertboxcontinue"
};

void   SetAssertMode(UInt32 mode);       // Mac 0x11acd0
UInt32 GetAssertMode();                  // (inlined reader of Mac 0x77d548; name inferred)
bool   IsInsideAssert();                 // Mac 0x11acf0 (caller Window slot 50)
void   EnterAssert();                    // Mac 0x11ad10 (caller Mac 0x295000)

} // namespace LEGO
