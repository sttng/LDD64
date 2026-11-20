// LEGO Digital Designer — restored source
// Assertion-box state — TU 054 (Mac 0x11acd0-0x11adb6). See Assert.h.
// The TU's static initialiser (Mac 0x11ad30) is `#include <iostream>`, the zero/0x01000000
// words written by every TU that includes the error-code header, and
// `mainThread = pthread_self()` (Mac 0x77d54c; no reader found yet, so not restored).
#include "LEGO/Assert.h"


namespace LEGO {

static UInt32 s_assertMode  = kAssertShowBox;   // Mac 0x77d548, Win 0xb42758
static UInt32 s_assertDepth = 0;                // Mac 0x77d550

// Mac 0x11acd0
void SetAssertMode(UInt32 mode)
{
    s_assertMode = mode;
}

UInt32 GetAssertMode()
{
    return s_assertMode;
}

// Mac 0x11acf0
bool IsInsideAssert()
{
    return s_assertDepth != 0;
}

// Mac 0x11ad10
void EnterAssert()
{
    ++s_assertDepth;
}

} // namespace LEGO
