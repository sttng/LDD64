// LEGO Digital Designer — restored source
// TU 015 (file name inferred)  Mac 0x1a250-0x1a94f (6 functions), Win 0x432270-0x432b1f.
// String <-> byte-encoding helpers: CoreFoundation on Mac, Win32 code-page API on Windows.
// Static init 0x1a910. Mac-only CF helpers 0x1a250, 0x1a2c0; Win-only code-page pair 0x432270,
// 0x4323c0. Also emitted on Win: 0x432b20 = AString reserve (BasicString.h).
#include "StringConversion.h"
#include "LEGO/StdLib.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <CoreFoundation/CoreFoundation.h>
#endif

#include <string.h>
#include <iostream>

namespace LEGO {

namespace {

// Number of characters of [start, start + count) inside a string of `length` characters
// (0 when start is past the end). Inlined in every function below.
inline UInt32 ClipCount(UInt32 length, UInt32 start, UInt32 count)
{
    if (start >= length)
        return 0;
    UInt32 rest = length - start;
    return count < rest ? count : rest;
}

// Result for an empty range / unknown encoding: an allocated, empty buffer whose first two
// bytes are zero (an empty UTF-16 string, too). Mac: new[] 0x2f, capacity 0x22;
// Win: 0x432b20(2). (name inferred)
inline AString EmptyEncoding()
{
    AString r;
    if (r.Reserve(2, true)) {
        r.GetWritableBuffer()[1] = 0;
        r.SetLength(0);
    }
    return r;
}

#if !defined(_WIN32)
inline CFStringEncoding ToCFEncoding(TextEncoding encoding)
{
    switch (encoding) {
    case kTextEncodingUTF16BE: return kCFStringEncodingUTF16BE;   // 0x10000100
    case kTextEncodingUTF16LE: return kCFStringEncodingUTF16LE;   // 0x14000100
    default:                   return kCFStringEncodingUTF8;      // 0x08000100
    }
}
#endif

} // namespace

#if !defined(_WIN32)

// ===========================================================================
// Mac (CoreFoundation)
// ===========================================================================

// Mac 0x1a250 (no Win twin)
CFStringRef CreateCFSubstring(const String& s, UInt32 start, UInt32 count)
{
    UInt32 n = ClipCount(s.GetLength(), start, count);
    if (n == 0)
        return CFSTR("");               // NOTE: constant string, the callers CFRelease it anyway
    return CFStringCreateWithSubstring(0, s.GetCFString(), CFRangeMake(start, n));
}

// Mac 0x1a2c0 (no Win twin)
String MakeString(CFStringRef cf)
{
    return String(cf);
}

// Mac 0x1a2f0
AString EncodeString(TextEncoding encoding, const String& s, UInt32 start, UInt32 count)
{
    UInt32 n = ClipCount(s.GetLength(), start, count);
    if (n != 0) {
        switch (encoding) {
        case kTextEncodingUTF16LE:
        case kTextEncodingUTF16BE: {
            AString r;
            if (r.Reserve(n * 2 + 1, true)) {
                r.GetWritableBuffer()[n * 2 + 1] = 0;
                r.SetLength(n * 2);
            }
            CFStringRef sub = CreateCFSubstring(s, start, n);
            CFStringGetCString(sub, r.GetWritableBuffer(), n * 2 + 1, ToCFEncoding(encoding));
            CFRelease(sub);
            return r;
        }
        case kTextEncodingUTF8: {
            CFStringRef sub = CreateCFSubstring(s, start, n);
            UInt32 size = (n * 6) | 1;
            AString r;
            r.Reserve(size, true);
            CFStringGetCString(sub, r.GetWritableBuffer(), size, kCFStringEncodingUTF8);
            CFRelease(sub);
            r.SetLength(StrLen(r.GetWritableBuffer()));
            return r;
        }
        }
    }
    return EmptyEncoding();
}

// Mac 0x1a630
String DecodeString(TextEncoding encoding, const void* data, UInt32 count)
{
    if (count != 0) {
        CFStringRef cf = 0;
        switch (encoding) {
        case kTextEncodingUTF16LE:
        case kTextEncodingUTF16BE: {
            UInt32 n = ((const UInt16*)data)[count - 1] != 0 ? count : count - 1;   // drop a final NUL
            cf = CFStringCreateWithBytes(0, (const UInt8*)data, n * 2, ToCFEncoding(encoding), false);
            break;
        }
        case kTextEncodingUTF8: {
            UInt32 n = ((const char*)data)[count - 1] != 0 ? count : count - 1;
            cf = CFStringCreateWithBytes(0, (const UInt8*)data, n, kCFStringEncodingUTF8, false);
            break;
        }
        default:
            return String();
        }
        String r;
        if (!cf)
            return r;                   // invalid input
        r = String(cf);
        CFRelease(cf);
        return r;
    }
    return String();
}

// Mac 0x1a830 (called by String::ToUTF8, Mac 0x28d870)
AString StringToUTF8(const String& s)
{
    AString r;
    if (s.IsEmpty())
        return r;
    if (r.Reserve((s.GetLength() * 6) | 1, true)) {
        CFStringGetCString(s.GetCFString(), r.GetWritableBuffer(), r.GetCapacity(), kCFStringEncodingUTF8);
        r.SetLength(StrLen(r.GetWritableBuffer()));
    }
    return r;
}

#else // _WIN32

// ===========================================================================
// Windows (Win32 code-page API)
// ===========================================================================

// Win 0x4324a0
AString EncodeString(TextEncoding encoding, const String& s, UInt32 start, UInt32 count)
{
    UInt32 n = ClipCount(s.GetLength(), start, count);
    if (n == 0)
        return EmptyEncoding();
    const UniChar16* src = s.GetBuffer() + start;
    switch (encoding) {
    case kTextEncodingUTF8: {
        AString r;
        r.Reserve(n * 6 + 1, true);
        char* out = r.GetWritableBuffer();
        int written = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)src, (int)n, out,
                                          (int)r.GetCapacity(), 0, 0);
        r.SetLength((UInt32)written);
        return r;
    }
    case kTextEncodingUTF16BE: {
        AString r;
        if (r.Reserve(n * 2 + 1, true))
            r.SetLength(n * 2);
        char* out = r.GetWritableBuffer();
        for (UInt32 i = 0; i < n; ++i) {
            out[i * 2] = (char)(src[i] >> 8);           // byte swap
            out[i * 2 + 1] = (char)(src[i] & 0xFF);
        }
        return r;
    }
    case kTextEncodingUTF16LE: {
        AString r;
        if (r.Reserve(n * 2 + 1, true))
            r.SetLength(n * 2);
        memcpy(r.GetWritableBuffer(), src, n * 2);
        return r;
    }
    }
    return EmptyEncoding();
}

// Win 0x4327e0
// NOTE: the UTF-8 and UTF-16BE results are built with the zero-terminated String constructor
// (Win 0x401570 at 0x4329c4 / 0x432912): they end at the first NUL code unit. UTF-16LE is
// copied with its length (0x401510), so NULs - also a trailing one - are kept (the Mac build
// drops one trailing NUL only).
String DecodeString(TextEncoding encoding, const void* data, UInt32 count)
{
    if (count == 0)
        return String();
    switch (encoding) {
    case kTextEncodingUTF8: {
        BasicString<UniChar16> buffer;
        buffer.Reserve(count + 1, true);
        int n = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)data, (int)count,
                                    (LPWSTR)buffer.GetWritableBuffer(), (int)buffer.GetCapacity());
        buffer.SetLength((UInt32)n);
        return String(buffer.GetBuffer());              // up to the first NUL
    }
    case kTextEncodingUTF16BE: {
        BasicString<UniChar16> buffer;
        if (buffer.Reserve(count, true))
            buffer.SetLength(count);
        const UInt8* src = (const UInt8*)data;
        UniChar16* out = buffer.GetWritableBuffer();
        for (UInt32 i = 0; i < count; ++i)
            out[i] = (UniChar16)((src[i * 2] << 8) | src[i * 2 + 1]);   // byte swap
        return String(buffer.GetBuffer());              // up to the first NUL
    }
    case kTextEncodingUTF16LE:
        return String((const UniChar16*)data, count);
    }
    return String();
}

// Win 0x432a10 (called by String::ToUTF8, Win 0x404410; String in EDI)
AString StringToUTF8(const String& s)
{
    AString r;
    UInt32 length = s.GetLength();
    if (length == 0)
        return r;
    r.Reserve(length * 6 + 1, true);
    int n = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)s.GetBuffer(), (int)length,
                                r.GetWritableBuffer(), (int)r.GetCapacity(), 0, 0);
    r.SetLength((UInt32)n);
    return r;
}

// Win 0x432270 (count in ECX)
AString EncodeStringCodePage(const String& s, UInt32 codePage, UInt32 count)
{
    AString r;
    UInt32 length = s.GetLength();
    UInt32 n = count < length ? count : length;
    if (n == 0) {
        r.Reserve(1, true);
        return r;
    }
    r.Reserve(n * 2 + 1, true);                       // room for double-byte code pages
    int written = WideCharToMultiByte(codePage, 0, (LPCWSTR)s.GetBuffer(), (int)n,
                                      r.GetWritableBuffer(), (int)r.GetCapacity(), 0, 0);
    r.SetLength((UInt32)written);
    return r;
}

// Win 0x4323c0 (length in ECX)
// NOTE: the result is built with the zero-terminated String constructor (Win 0x401570 at
// 0x43244a): it ends at the first NUL.
String DecodeStringCodePage(const char* s, UInt32 length, UInt32 codePage)
{
    BasicString<UniChar16> buffer;
    buffer.Reserve(length + 1, true);
    if (length != 0) {
        int n = MultiByteToWideChar(codePage, 0, s, (int)length,
                                    (LPWSTR)buffer.GetWritableBuffer(), (int)buffer.GetCapacity());
        buffer.SetLength((UInt32)n);
    }
    return String(buffer.GetBuffer());
}

#endif // _WIN32

} // namespace LEGO
