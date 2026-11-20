// LEGO Digital Designer — restored source
// Base64 (RFC 4648 alphabet, '=' padding, no line breaks). File and function names inferred.
//   TU 129 Base64.cpp       Mac 0x1e8700-0x1e88df wrappers (Win 0x444870, 0x444940, 0x444a00)
//   TU 130 Base64Codec.cpp  Mac 0x1e88e0-0x1e8d2f raw codec (Win 0x4450c0, 0x4451a0, helpers
//                           0x444f10 / 0x444ff0; size helpers inlined on Win)
// Callers: MemoryDirectory::WrapMemory, UploadModelCaller (Mac 0x1b6ce0), Mac 0x2691a0 (decode).
#pragma once
#include "LEGO/Types.h"
#include "BasicString.h"
#include "BasicArray.h"

namespace LEGO {

// --- TU 129 -------------------------------------------------------------------------------

// Appends the Base64 text of `size` bytes at `data` to `out`. Always true.
bool Base64Encode(const void* data, UInt32 size, AString& out);                 // Mac 0x1e8700, Win 0x444870

// Same for the contents of `data`. NOTE: taken by value in both builds (Win 0x444940 frees
// the copied buffer itself; the Mac caller passes a temporary by invisible reference).
bool Base64Encode(BasicArray<UInt8> data, AString& out);                         // Mac 0x1e87d0, Win 0x444940

// Replaces `out` by the decoded bytes of `text`; false when fewer bytes were decoded than
// the length computed from the text (size and '=' padding).
bool Base64Decode(const AString& text, BasicArray<UInt8>& out);                  // Mac 0x1e8800, Win 0x444a00

// --- TU 130 (raw codec) ------------------------------------------------------------------

// Buffer size Base64EncodeBuffer writes for `size` input bytes, terminator included.
UInt32 Base64EncodedSize(UInt32 size);                                           // Mac 0x1e88e0, Win inlined
// Number of bytes the zero-terminated `text` decodes to (from its length and padding).
UInt32 Base64DecodedSize(const char* text);                                      // Mac 0x1e8910, Win inlined
// Writes the zero-terminated Base64 text of `size` bytes to `out`
// (Base64EncodedSize(size) bytes, zero-filled first).
void   Base64EncodeBuffer(const void* data, UInt32 size, char* out);             // Mac 0x1e8a50, Win 0x4450c0
// Decodes the zero-terminated `text` into `out` (characters outside the alphabet and '=' are
// skipped); returns the number of bytes written.
UInt32 Base64DecodeBuffer(const char* text, void* out);                          // Mac 0x1e8b60, Win 0x4451a0

} // namespace LEGO
