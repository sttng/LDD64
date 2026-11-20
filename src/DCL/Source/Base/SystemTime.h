// LEGO Digital Designer — restored source
// Wall-clock helpers for timing and tracing (file and function names inferred: no symbols,
// no strings). TU 017 Mac 0x1cdc0-0x1ce6f; Win 0x436180 / 0x436190 (+ inlined copies).
#pragma once
#include "LEGO/Types.h"

namespace LEGO {

// Milliseconds since an arbitrary origin (Mac: Unix epoch, Win: system start).
Int64 GetMilliseconds();                                               // Mac 0x1cdc0, Win 0x436180
// Microseconds since an arbitrary origin (Mac: Unix epoch, Win: performance counter).
Int64 GetMicroseconds();                                               // Mac 0x1ce00, Win inlined (0x42df40, 0x42df90, 0x42dfe0, ...)

} // namespace LEGO
