// LEGO Digital Designer — restored source
// String <-> byte-encoding helpers; all names inferred. TU 015 Mac 0x1a250-0x1a94f
// (CoreFoundation), Win 0x432270-0x432b1f (WideCharToMultiByte / MultiByteToWideChar).
// Encodings: 0 = UTF-8, 1 = UTF-16BE, 2 = UTF-16LE (both builds).
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "BasicString.h"

namespace LEGO {

// Byte encodings understood by EncodeString / DecodeString. (names inferred)
enum TextEncoding {
    kTextEncodingUTF8    = 0,   // Mac kCFStringEncodingUTF8    0x08000100, Win CP_UTF8 65001
    kTextEncodingUTF16BE = 1,   // Mac kCFStringEncodingUTF16BE 0x10000100, Win byte swap
    kTextEncodingUTF16LE = 2    // Mac kCFStringEncodingUTF16LE 0x14000100, Win plain copy
};

// Encodes `count` characters of `s` starting at `start` (both clipped to the string).
// UTF-16 results hold 2 bytes per character (no BOM, no terminator in the length).
// An empty range or an unknown encoding yields an empty (but allocated) AString.
AString EncodeString(TextEncoding encoding, const String& s,
                     UInt32 start = 0, UInt32 count = String::npos);           // Mac 0x1a2f0, Win 0x4324a0

// Decodes `count` code units (bytes for UTF-8, 16-bit units for UTF-16) at `data`.
// Mac drops one trailing NUL code unit; Win stops at the first NUL for UTF-8 / UTF-16BE and keeps
// every unit for UTF-16LE (see the NOTE in the .cpp).
String DecodeString(TextEncoding encoding, const void* data, UInt32 count);     // Mac 0x1a630, Win 0x4327e0

// Whole string as UTF-8; null AString for an empty string (String::ToUTF8).
AString StringToUTF8(const String& s);                                         // Mac 0x1a830, Win 0x432a10

#if defined(_WIN32)
// Windows only: conversion through a Windows code page (WideCharToMultiByte /
// MultiByteToWideChar), used by the Win32 registry / file name code (Win 0x412a10..0x417270).
// The count/length travels in ECX in the /LTCG build.
AString EncodeStringCodePage(const String& s, UInt32 codePage,
                             UInt32 count = String::npos);                     // Win 0x432270
String  DecodeStringCodePage(const char* s, UInt32 length, UInt32 codePage);   // Win 0x4323c0
#else
// Mac only: new CFString for `count` characters at `start` (clipped); the constant CFSTR("")
// for an empty range. The caller releases the result.
CFStringRef CreateCFSubstring(const String& s, UInt32 start, UInt32 count);    // Mac 0x1a250
// String from a CFString (out-of-line copy of the String(CFStringRef) constructor).
String MakeString(CFStringRef cf);                                             // Mac 0x1a2c0
#endif

} // namespace LEGO
