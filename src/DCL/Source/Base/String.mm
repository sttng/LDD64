// LEGO Digital Designer — restored source
// TU 273 (Mac 0x288ca0-0x28decf): LEGO::String, Mac OS X implementation on CFString.
// Objective-C++: ScanWord() and Parse(UInt64&) use NSScanner. All 101 functions restored. Host
// builds use the compat/ CoreFoundation shim and the HOST-ONLY fallbacks.
#include "LEGO/String.h"
#include "BasicArray.h"
#include "LEGO/StdLib.h"
#include "StringConversion.h"

#include <CoreFoundation/CoreFoundation.h>
#include <string.h>
#if defined(__OBJC__)
#import <Foundation/Foundation.h>
#endif

namespace LEGO {

namespace {

const UInt32 kASCII = kCFStringEncodingASCII;     // 0x600

inline CFStringRef Empty() { return CFSTR(""); }

// NOTE: the original converts every 8-bit string with CFStringCreateWithCString(..., ASCII),
// which returns NULL for a byte above 0x7F; that NULL is then used as the string (or appended /
// released: CFStringGetLength, CFStringAppend, CFRelease of NULL crash). fixed: if the ASCII
// conversion fails, a lossy one that widens each byte to one UTF-16 unit (Latin-1, as the Win
// String does); a NULL `s` gives the empty string.
CFStringRef CreateWithASCII(const char* s)
{
    CFStringRef r = s ? CFStringCreateWithCString(0, s, kASCII) : 0;
    if (r || !s)
        return r ? r : Empty();
    UInt32 n = StrLen(s);
    UniChar16* buf = new UniChar16[n + 1];
    for (UInt32 i = 0; i < n; ++i)
        buf[i] = (UniChar16)(unsigned char)s[i];
    r = CFStringCreateWithCharacters(0, buf, n);
    delete[] buf;
    return r ? r : Empty();
}

// Clamp [start, start+count) to a string of length len (pattern repeated inline in
// every range-taking method of the Mac build).
inline CFRange ClampRange(UInt32 len, UInt32 start, UInt32 count)
{
    // NOTE / PORT64: as Clamp in String.cpp - a wrapped "negative" start is CFIndex -2 on the
    // 32-bit original (kept: the check below is compiled out there); on 64-bit it would be a
    // range far beyond the string, so a start beyond the string is the empty range.
    if (sizeof(void*) > 4 && start > len)
        return CFRangeMake(0, 0);
    if (len < start + count) {
        if (start <= len)
            return CFRangeMake(start, len - start);
        return CFRangeMake(0, 0);
    }
    return CFRangeMake(start, count);
}

// Substring semantics of String(const String&, start, count)
inline CFStringRef CreateSub(CFStringRef s, UInt32 start, UInt32 count)
{
    if (count != 0) {
        UInt32 len = (UInt32)CFStringGetLength(s);
        if (start <= len) {
            UInt32 rest = len - start;
            UInt32 n = count < rest ? count : rest;
            if (count == String::npos)
                n = rest;
            return CFStringCreateWithSubstring(0, s, CFRangeMake(start, n));
        }
    }
    return Empty();
}

inline bool SetContains(CFStringRef set, CFStringRef s, UInt32 i)
{
    CFStringRef ch = CFStringCreateWithSubstring(0, s, CFRangeMake(i, 1));
    CFRange r;
    bool found = CFStringFindWithOptions(set, ch, CFRangeMake(0, CFStringGetLength(set)), 0, &r);
    CFRelease(ch);
    return found;
}

} // namespace

// ===========================================================================
// internal helpers (inlined in the original)
// ===========================================================================

#define GET(s) ((s).m_string ? (s).m_string : (CFStringRef)(s).m_mutable)

void String::MakeMutable()
{
    if (m_string) {
        m_mutable = CFStringCreateMutableCopy(0, 0, m_string);
        CFRelease(m_string);
        m_string = 0;
        if (m_charCache) {
            delete[] m_charCache;   // NOTE: original frees with scalar operator delete
            m_charCache = 0;
        }
    }
}

void String::ReleaseAll()
{
    if (m_string) {
        CFRelease(m_string);
        m_string = 0;
    }
    if (m_mutable) {
        CFRelease(m_mutable);
        m_mutable = 0;
    }
    if (m_charCache) {
        delete[] m_charCache;
        m_charCache = 0;
    }
}

// ===========================================================================
// construction
// ===========================================================================

String::String()                                                  // 0x288cd0
    : m_string(Empty()), m_mutable(0), m_charCache(0), m_peakLength(0) {}

String::String(const String& s)                                   // 0x288d20
    : m_string(CFStringCreateCopy(0, GET(s))), m_mutable(0), m_charCache(0), m_peakLength(0) {}

String::String(const String& s, UInt32 start, UInt32 count)       // 0x288d80
    : m_string(CreateSub(GET(s), start, count)), m_mutable(0), m_charCache(0), m_peakLength(0) {}

String::String(const UniChar16* chars, UInt32 length)             // 0x288e20
    : m_mutable(0), m_charCache(0), m_peakLength(0)
{
    m_string = (chars && length) ? CFStringCreateWithCharacters(0, chars, length) : Empty();
}

String::String(const UniChar16* z)                                // 0x288e70
    : m_mutable(0), m_charCache(0), m_peakLength(0)
{
    if (!z) {
        m_string = Empty();
    } else {
        const UniChar16* p = z;
        while (*p)
            ++p;
        m_string = CFStringCreateWithCharacters(0, z, p - z);
    }
}

String::String(const AString& a)                                  // 0x288ee0
    : m_mutable(0), m_charCache(0), m_peakLength(0)
{
    m_string = CreateWithASCII(a.c_str());
}

String::String(const char* s, UInt32 length)                      // 0x288f30
    : m_mutable(0), m_charCache(0), m_peakLength(0)
{
    if (!s) {
        m_string = Empty();
    } else {
        char* tmp = new char[length + 1];
        tmp[length] = 0;
        memcpy(tmp, s, length);
        m_string = CreateWithASCII(tmp);
        delete[] tmp;
    }
}

String::String(const char* s)                                     // 0x289000 (no null check)
    : m_string(CreateWithASCII(s)), m_mutable(0), m_charCache(0), m_peakLength(0) {}

String::String(UInt32 count, UniChar16 fill)                      // 0x289040
    : m_mutable(0), m_charCache(0), m_peakLength(0)
{
    UniChar16* buf = new UniChar16[count];
    for (UInt32 i = 0; i < count; ++i)
        buf[i] = fill;
    m_string = CFStringCreateWithCharacters(0, buf, count);
    delete[] buf;
}

String::String(CFStringRef cf)                                    // 0x2891b0
    : m_mutable(0), m_charCache(0), m_peakLength(0)
{
    m_string = cf ? CFStringCreateCopy(0, cf) : Empty();
}

CFStringRef String::GetCFString() const { return GET(*this); }    // 0x288d00

String::~String()                                                 // 0x289240
{
    if (m_charCache) {
        delete[] m_charCache;
        m_charCache = 0;
    }
    CFRelease(GET(*this));
}

// ===========================================================================
// assignment
// ===========================================================================

String& String::operator=(const String& s)                        // 0x289c40
{
    if (&s != this) {
        ReleaseAll();
        m_string = CFStringCreateCopy(0, GET(s));
    }
    return *this;
}

String& String::Assign(const String& s, UInt32 start, UInt32 count) // 0x289b10
{
    CFStringRef sub = CreateSub(GET(s), start, count);
    ReleaseAll();
    m_string = CFStringCreateCopy(0, sub);
    CFRelease(sub);
    return *this;
}

String& String::Assign(const char* s, UInt32 length)              // 0x28cea0
{
    if (m_string) {
        CFRelease(m_string);
        m_string = 0;
    } else {
        CFRelease(m_mutable);
        m_mutable = 0;
    }
    if (m_charCache) {
        delete[] m_charCache;
        m_charCache = 0;
    }
    if (!s || length == 0) {
        m_string = Empty();
    } else {
        char* tmp = new char[length + 1];
        tmp[length] = 0;
        memcpy(tmp, s, length);
        m_string = CreateWithASCII(tmp);
        delete[] tmp;
    }
    return *this;
}

String& String::operator=(const char* s)                          // 0x28cfe0 / 0x28cf60
{
    return Assign(s, s ? StrLen(s) : 0);
}

String& String::operator=(const AString& a)                       // 0x28cfa0
{
    return Assign(a.c_str(), a.GetLength());
}

// ===========================================================================
// access
// ===========================================================================

UInt32 String::GetLength() const { return (UInt32)CFStringGetLength(GET(*this)); } // 0x288d60
bool   String::IsEmpty() const   { return CFStringGetLength(GET(*this)) == 0; }    // 0x289360
UniChar16 String::GetAt(UInt32 i) const { return CFStringGetCharacterAtIndex(GET(*this), i); } // 0x289280

// 0x2892b0 — cached for immutable strings, rebuilt every call for mutable ones
const UniChar16* String::GetBuffer() const
{
    String* self = const_cast<String*>(this);
    if (m_charCache) {
        if (m_string)
            return m_charCache;
        delete[] self->m_charCache;
    }
    UInt32 len = GetLength();
    self->m_charCache = new UniChar16[len + 1];
    m_charCache[len] = 0;
    if (len)
        CFStringGetCharacters(GET(*this), CFRangeMake(0, len), m_charCache);
    return m_charCache;
}

String String::Substring(UInt32 start, UInt32 count) const        // 0x289390
{
    return String(*this, start, count);
}

bool String::IsCharAt(UInt32 index, UniChar16 c) const           // 0x289980
{
    if (index == npos)
        index = GetLength() ? GetLength() - 1 : npos;
    if (index < GetLength())
        return GetAt(index) == c;
    return false;
}

// 0x289a10 / 0x289ab0: CFCharacterSet whitespace (predefined set 2 = Unicode Zs + tab, no
// line breaks) -- the Win build uses iswspace, which includes \n \v \f \r (String.cpp).
bool String::IsWhitespaceAt(UInt32 index) const                   // 0x289a10
{
    if (index == npos)
        index = GetLength() ? GetLength() - 1 : npos;
    if (index < GetLength())
        return CFCharacterSetIsCharacterMember(
            CFCharacterSetGetPredefined(kCFCharacterSetWhitespace), GetAt(index));
    return false;
}

bool String::IsWhitespace(UniChar16 c)                            // 0x289ab0
{
    return CFCharacterSetIsCharacterMember(CFCharacterSetGetPredefined(kCFCharacterSetWhitespace), c);
}

bool String::IsDigit(UniChar16 c)                                 // 0x289ae0
{
    return CFCharacterSetIsCharacterMember(CFCharacterSetGetPredefined(kCFCharacterSetDecimalDigit), c);
}

bool String::IsASCII() const                                      // 0x28d210
{
    if (CFStringGetCStringPtr(GET(*this), kASCII))
        return true;
    UInt32 len = GetLength();
    char* buf = new char[len + 1];
    bool ok = CFStringGetCString(GET(*this), buf, len + 1, kASCII);
    delete[] buf;
    return ok;
}

AString String::ToAString() const                                 // 0x28d2a0
{
    AString r;
    const char* p = CFStringGetCStringPtr(GET(*this), kASCII);
    UInt32 len = GetLength();
    if (!p) {
        char* buf = new char[len + 1];
        if (CFStringGetCString(GET(*this), buf, len + 1, kASCII)) {
            if (r.Reserve(len, true)) {
                memcpy(r.GetWritableBuffer(), buf, len);
                r.SetLength(len);
            }
        }
        delete[] buf;
    } else if (r.Reserve(len, true)) {
        memcpy(r.GetWritableBuffer(), p, len);
        r.SetLength(len);
    }
    return r;
}

// 0x28d870 -> 0x1a830 (TU 015 StringConversion.cpp): UTF-8, capacity for 6 bytes per character
AString String::ToUTF8() const
{
    return StringToUTF8(*this);
}

// 0x28d8a0
String String::ToXML() const
{
    String r(*this);
    r.ReplaceAll(String("&"), String("&amp;"));
    r.ReplaceAll(String("<"), String("&lt;"));
    r.ReplaceAll(String(">"), String("&gt;"));
    r.ReplaceAll(String("'"), String("&apos;"));
    r.ReplaceAll(String("\""), String("&quot;"));
    return r;
}

// 0x2897e0 — next whitespace-delimited word starting at *position
String String::ScanWord(UInt32* position) const
{
#if defined(__OBJC__)
    NSScanner* scanner = [NSScanner scannerWithString:(NSString*)GET(*this)];
    if (position) {
        if (*position >= GetLength())
            return String();
        [scanner setScanLocation:*position];
    }
    NSString* word = nil;
    if (![scanner scanUpToCharactersFromSet:[NSCharacterSet whitespaceCharacterSet] intoString:&word])
        return String();
    if (position)
        *position = (UInt32)[scanner scanLocation];
    return word ? String((CFStringRef)word) : String();
#else
    // HOST-ONLY fallback with NSScanner semantics (skips leading whitespace+newlines)
    UInt32 len = GetLength(), i = position ? *position : 0;
    if (position && i >= len)
        return String();
    while (i < len && (IsWhitespace(GetAt(i)) || GetAt(i) == '\n' || GetAt(i) == '\r'))
        ++i;
    UInt32 b = i;
    while (i < len && !IsWhitespace(GetAt(i)))
        ++i;
    if (i == b)
        return String();
    if (position)
        *position = i;
    return String(*this, b, i - b);
#endif
}

// ===========================================================================
// modification
// ===========================================================================

String& String::Append(const String& s)                           // 0x289cc0
{
    MakeMutable();
    CFStringAppend(m_mutable, GET(s));
    return *this;
}

String& String::Append(const char* s)                             // 0x28d110
{
    CFStringRef t = CreateWithASCII(s);
    MakeMutable();
    CFStringAppend(m_mutable, t);
    CFRelease(t);
    return *this;
}

String& String::Append(const AString& a)                          // 0x28d020
{
    CFStringRef t = CreateWithASCII(a.c_str());
    MakeMutable();
    CFStringAppend(m_mutable, t);
    CFRelease(t);
    return *this;
}

String& String::Append(UInt32 count, UniChar16 c)                 // 0x289440
{
    UniChar16* buf = new UniChar16[count];
    for (UInt32 i = 0; i < count; ++i)
        buf[i] = c;
    CFStringRef t = CFStringCreateWithCharacters(0, buf, count);
    delete[] buf;
    MakeMutable();
    CFStringAppend(m_mutable, t);
    CFRelease(t);
    return *this;
}

String& String::Insert(UInt32 pos, const String& s)               // 0x28b310
{
    MakeMutable();
    if (pos == npos)
        CFStringAppend(m_mutable, GET(s));
    else
        CFStringInsert(m_mutable, pos, GET(s));
    return *this;
}

String& String::Insert(UInt32 pos, UInt32 count, UniChar16 c)     // 0x28b410
{
    return Insert(pos, String(count, c));
}

String& String::Replace(UInt32 start, UInt32 count, const String& s) // 0x28b5e0
{
    if (start == npos) {
        MakeMutable();
        CFStringAppend(m_mutable, GET(s));
    } else {
        if (GetLength() < start + count)
            count = GetLength() - start;
        MakeMutable();
        CFStringReplace(m_mutable, CFRangeMake(start, count), GET(s));
    }
    return *this;
}

String& String::ReplaceAll(const String& from, const String& to)  // 0x28b8e0
{
    MakeMutable();
    CFStringFindAndReplace(m_mutable, GET(from), GET(to),
                           CFRangeMake(0, CFStringGetLength(GET(*this))), 0);
    return *this;
}

String& String::ReplaceAll(const String& from, UInt32 fs, UInt32 fc,
                           const String& to, UInt32 ts, UInt32 tc)  // 0x28b720
{
    String a(from, fs, fc);
    String b(to, ts, tc);
    return ReplaceAll(a, b);
}

String& String::Erase(UInt32 start, UInt32 count)                 // 0x289640
{
    UInt32 len = GetLength();
    if (len < start)
        return *this;
    if (count > len - start)
        count = len - start;
    MakeMutable();
    UInt32 cur = GetLength();
    if (m_peakLength < cur)
        m_peakLength = cur;
    CFStringDelete(m_mutable, CFRangeMake(start, count));
    return *this;
}

void String::Clear()                                              // 0x289710
{
    if (m_string) {
        CFRelease(m_string);
    } else {
        CFRelease(m_mutable);
        m_mutable = 0;
    }
    m_string = Empty();
    m_peakLength = 0;
    if (m_charCache) {
        delete[] m_charCache;
        m_charCache = 0;
    }
}

void String::Trim()                                               // 0x289770
{
    MakeMutable();
    CFStringTrimWhitespace(m_mutable);
}

void String::TrimRight(const String& chars)                       // 0x28c500
{
    UInt32 i = FindLastNotOf(chars, npos);
    if (i == npos)
        Clear();
    else
        Erase(i + 1, npos);
}

void String::ToLower()                                            // 0x289900
{
    MakeMutable();
    CFStringLowercase(m_mutable, CFLocaleGetSystem());
}

// ===========================================================================
// formatting
// ===========================================================================

namespace {
// shared tail of every numeric operator<<: append a formatted CFString
String& AppendFormatted(String& s, CFStringRef formatted);
}

String& String::operator<<(const String& s) { return Append(s); }          // 0x289eb0
String& String::operator<<(const char* s)   { return Append(s); }          // 0x289e60
String& String::Append(UniChar16 c)         { return Append(1, c); }       // 0x289e80

String& String::operator<<(bool b)                                // 0x289f30
{
    CFStringRef t = CFStringCreateCopy(0, b ? CFSTR("1") : CFSTR("0"));
    MakeMutable();
    CFStringAppend(m_mutable, t);
    CFRelease(t);
    return *this;
}

// LDD64: the decimal digits appended directly, the same text as the original's
// CFStringCreateWithFormat ("%hu" / "%u" / "%qu") at a fraction of its cost.
void String::AppendDecimal(UInt64 v)
{
    UniChar16 digits[20];
    UInt32 n = 0;
    do {
        digits[19 - n++] = (UniChar16)('0' + v % 10);
        v /= 10;
    } while (v != 0);
    MakeMutable();
    CFStringAppendCharacters(m_mutable, (const UniChar*)digits + 20 - n, n);
}

String& String::operator<<(UInt16 v)                              // 0x28a020
{
    AppendDecimal(v);
    return *this;
}

String& String::operator<<(UInt32 v)                              // 0x289d40
{
    AppendDecimal(v);
    return *this;
}

String& String::operator<<(UInt64 v)                              // 0x28a290
{
    AppendDecimal(v);
    return *this;
}

String& String::operator<<(Real32 v)                              // 0x28a3b0
{
    double d = (v < 0 ? -v : v) < 1e-05f ? 0.0 : (double)v;
    return AppendFormatted(*this, CFStringCreateWithFormat(0, 0, CFSTR("%.30G"), d));
}

String& String::operator<<(Real64 v)                              // 0x28a500
{
    double d = (v < 0 ? -v : v) < (double)1e-08f ? 0.0 : v;      // threshold is a float literal
    return AppendFormatted(*this, CFStringCreateWithFormat(0, 0, CFSTR("%.60G"), d));
}

namespace {
// The formatter String's number reads and writes use (kCFNumberFormatterNoStyle, default locale).
// The original creates one per number, and ICU builds its parser again for each, so reading a
// large model takes very long on the Mac. One per thread (formatters aren't thread-safe), kept for
// the thread's life; it has the same settings, so the results are the same.
CFNumberFormatterRef GetNumberFormatter()
{
    static thread_local CFNumberFormatterRef f = 0;
    if (f == 0)
        f = CFNumberFormatterCreate(0, 0, kCFNumberFormatterNoStyle);
    return f;
}
}

String& String::operator<<(Int32 v)                               // 0x28a140
{
    SInt32 value = v;
    CFNumberFormatterRef f = GetNumberFormatter();
    CFStringRef s = CFNumberFormatterCreateStringWithValue(0, f, kCFNumberSInt32Type, &value);
    CFStringRef t = s ? CFStringCreateCopy(0, s) : Empty();
    MakeMutable();
    CFStringAppend(m_mutable, t);
    CFRelease(t);
    CFRelease(s);
    return *this;
}

namespace {
String& AppendFormatted(String& s, CFStringRef formatted)
{
    s << String(formatted);    // copy (or "" when formatting failed), append, release
    CFRelease(formatted);
    return s;
}
}

// ===========================================================================
// parsing
// ===========================================================================

bool String::EqualsNoCase(UInt32 start, UInt32 count, const String& s) const // 0x28ac20
{
    return CFStringCompareWithOptions(GET(*this), GET(s), ClampRange(GetLength(), start, count),
                                      kCFCompareCaseInsensitive) == kCFCompareEqualTo;
}

// 0x28a650
bool String::Parse(bool& out, UInt32& pos) const
{
    if (GetLength() == 0) {
        out = false;
        return false;
    }
    static const struct { const char* text; UInt32 n; bool value; } words[] = {
        { "0", 1, false }, { "1", 1, true }, { "no", 2, false },
        { "yes", 3, true }, { "false", 5, false }, { "true", 4, true },
    };
    for (unsigned i = 0; i < sizeof(words) / sizeof(words[0]); ++i) {
        if (EqualsNoCase(pos, words[i].n, String(words[i].text))) {
            pos += words[i].n;
            out = words[i].value;
            return true;
        }
    }
    return false;
}

// 0x28ad20 — unsigned decimal; a leading '-' makes the parse fail
bool String::Parse(UInt64& out, UInt32& pos) const
{
    static const String kStart("+-0123456789");
    UInt32 i = FindFirstOf(kStart, pos);
    if (i >= GetLength())
        return false;
    pos = i;
    bool negative = false;
    if (i < GetLength() && GetAt(i) == '-') {
        ++pos;
        negative = true;
    }
    if (IsCharAt(pos, '+'))
        ++pos;
    if (pos >= GetLength())
        return false;
    // LDD64: what the original does with NSScanner and a number formatter call per digit, done
    // directly: skip NSScanner's default charactersToBeSkipped (whitespaceAndNewlineCharacterSet),
    // so "+ 12" parses 12, then take the ASCII digits; `pos` only moves when digits follow.
    UInt32 b = pos;
    CFCharacterSetRef skip = CFCharacterSetGetPredefined(kCFCharacterSetWhitespaceAndNewline);
    while (b < GetLength() && CFCharacterSetIsCharacterMember(skip, GetAt(b)))
        ++b;
    UInt32 e = b;
    while (e < GetLength() && GetAt(e) >= '0' && GetAt(e) <= '9')
        ++e;
    if (e == b)
        return false;
    pos = e;
    if (negative)
        return false;
    UInt64 v = 0;
    for (UInt32 k = b; k < e; ++k) {
        const UInt32 d = GetAt(k) - '0';
        if (v < 0x1999999999999999ULL || (v == 0x1999999999999999ULL && d < 6))
            v = v * 10 + d;
        else
            return false;
    }
    out = v;
    return true;
}

// 0x28acd0 / 0x28b0a0: `out` is stored only on success (no clamping) -- unlike the Win build,
// which stores 0xFF / 0xFFFFFFFF on overflow (String.cpp, Win 0x402820 / 0x402890).
bool String::Parse(UInt8& out, UInt32& pos) const                 // 0x28acd0
{
    UInt64 v;
    if (!Parse(v, pos) || v > 0xFF)
        return false;
    out = (UInt8)v;
    return true;
}

bool String::Parse(UInt32& out, UInt32& pos) const                // 0x28b0a0
{
    UInt64 v;
    if (!Parse(v, pos) || (v >> 32) != 0)
        return false;
    out = (UInt32)v;
    return true;
}

namespace {
template <class T>
bool ParseReal(CFStringRef s, CFNumberType type, T& out, UInt32& pos)  // 0x28b1b0 / 0x28b260
{
    UInt32 len = (UInt32)CFStringGetLength(s);
    if (pos >= len)
        return false;
    CFRange r = CFRangeMake(pos, len - pos);
    CFNumberFormatterRef f = GetNumberFormatter();
    bool ok = CFNumberFormatterGetValueFromString(f, s, &r, type, &out);
    pos = (UInt32)(r.location + r.length);
    return ok;
}
}

bool String::Parse(Real32& out, UInt32& pos) const { return ParseReal(GET(*this), kCFNumberFloat32Type, out, pos); }
bool String::Parse(Real64& out, UInt32& pos) const { return ParseReal(GET(*this), kCFNumberFloat64Type, out, pos); }

// ===========================================================================
// searching
// ===========================================================================

UInt32 String::Find(const String& s, UInt32 start) const          // 0x28b990
{
    UInt32 len = GetLength();
    if (start >= len)
        return npos;
    CFRange r;
    if (CFStringFindWithOptions(GET(*this), GET(s), CFRangeMake(start, len - start), 0, &r))
        return (UInt32)r.location;
    return npos;
}

UInt32 String::Find(UniChar16 c, UInt32 start) const              // 0x28ba10
{
    UInt32 len = GetLength();
    if (start >= len)
        return npos;
    CFStringRef t = CFStringCreateWithCharacters(0, &c, 1);
    CFRange r;
    bool found = CFStringFindWithOptions(GET(*this), t, CFRangeMake(start, len - start), 0, &r);
    CFRelease(t);
    return found ? (UInt32)r.location : npos;
}

UInt32 String::FindNoCase(const String& s, UInt32 start) const    // 0x28bab0
{
    UInt32 len = GetLength();
    if (start >= len)
        return npos;
    CFRange r;
    if (CFStringFindWithOptions(GET(*this), GET(s), CFRangeMake(start, len - start),
                                kCFCompareCaseInsensitive, &r))
        return (UInt32)r.location;
    return npos;
}

UInt32 String::ReverseFind(const UniChar16* s, UInt32 start, UInt32 n) const // 0x28bb30
{
    UInt32 end;
    if (start < ~n) {
        end = GetLength();
        if (start + n < end)
            end = start + n;
    } else {
        end = GetLength();
    }
    CFStringRef t = CFStringCreateWithCharacters(0, s, n);
    CFRange r;
    bool found = CFStringFindWithOptions(GET(*this), t, CFRangeMake(0, end), kCFCompareBackwards, &r);
    CFRelease(t);
    return found ? (UInt32)r.location : npos;
}

UInt32 String::ReverseFind(UniChar16 c, UInt32 start) const       // 0x28bbe0
{
    return ReverseFind(&c, start, 1);
}

UInt32 String::FindFirstOf(const String& set, UInt32 start) const // 0x28b0e0
{
    if (set.GetLength() != 0)
        for (; start < GetLength(); ++start)
            if (SetContains(GET(set), GET(*this), start))
                return start;
    return npos;
}

UInt32 String::FindLastOf(const String& set, UInt32 start) const  // 0x28bc10
{
    if (set.GetLength() == 0)
        return npos;
    UInt32 i = GetLength() - 1;
    if (start < i)
        i = start;
    for (UInt32 k = i + 1; k != 0;) {
        --k;
        if (SetContains(GET(set), GET(*this), k))
            return k;
    }
    return npos;
}

UInt32 String::FindLastOf(const UniChar16* set, UInt32 start, UInt32 n) const // 0x28bcf0
{
    return FindLastOf(String(set, n), start);
}

UInt32 String::FindLastOf(UniChar16 c, UInt32 start) const        // 0x28bdd0
{
    return FindLastOf(&c, start, 1);
}

UInt32 String::FindFirstNotOf(const String& set, UInt32 start) const // 0x28be00
{
    if (start >= GetLength())
        return npos;
    if (set.GetLength() == 0)
        return start;
    for (; start < GetLength(); ++start)
        if (!SetContains(GET(set), GET(*this), start))
            return start;
    return npos;
}

UInt32 String::FindFirstNotOf(const UniChar16* set, UInt32 start, UInt32 n) const // 0x28bf00
{
    if (start >= GetLength())
        return npos;
    return FindFirstNotOf(String(set, n), start);
}

UInt32 String::FindFirstNotOf(const UniChar16* set, UInt32 start) const // 0x28c000
{
    const UniChar16* p = set;
    while (*p)
        ++p;
    return FindFirstNotOf(set, start, (UInt32)(p - set));
}

UInt32 String::FindLastNotOf(const String& set, UInt32 start) const // 0x28c040
{
    UInt32 i = GetLength() - 1;
    if (set.GetLength() == 0)
        return i;                                  // NOTE: start ignored here in the original
    if (start < i)
        i = start;
    for (UInt32 k = i + 1;;) {
        if (k == 0)
            return npos;
        --k;
        if (!SetContains(GET(set), GET(*this), k))
            return k;
    }
}

UInt32 String::FindLastNotOf(const UniChar16* set, UInt32 start, UInt32 n) const // 0x28c120
{
    return FindLastNotOf(String(set, n), start);
}

UInt32 String::FindLastNotOf(const UniChar16* set, UInt32 start) const // 0x28c200
{
    const UniChar16* p = set;
    while (*p)
        ++p;
    return FindLastNotOf(set, start, (UInt32)(p - set));
}

// ===========================================================================
// comparison
// ===========================================================================

bool String::operator==(const String& s) const { return CFStringCompare(GET(*this), GET(s), 0) == kCFCompareEqualTo; } // 0x28c280
bool String::operator!=(const String& s) const { return CFStringCompare(GET(*this), GET(s), 0) != kCFCompareEqualTo; } // 0x28c240
bool String::operator<(const String& s) const  { return CFStringCompare(GET(*this), GET(s), 0) < 0; }                 // 0x28c2c0
bool String::StartsWith(const String& s) const { return CFStringHasPrefix(GET(*this), GET(s)); }                       // 0x28c300

bool String::StartsWith(const UniChar16* s, UInt32 n) const       // 0x28c340
{
    CFStringRef t = (s && n) ? CFStringCreateWithCharacters(0, s, n) : Empty();
    bool r = CFStringHasPrefix(GET(*this), t);
    CFRelease(t);
    return r;
}

bool String::StartsWith(UniChar16 c) const { return StartsWith(&c, 1); } // 0x28c3f0

bool String::EndsWith(const UniChar16* s, UInt32 n) const         // 0x28c420
{
    CFStringRef t = (s && n) ? CFStringCreateWithCharacters(0, s, n) : Empty();
    bool r = CFStringHasSuffix(GET(*this), t);
    CFRelease(t);
    return r;
}

bool String::EndsWith(UniChar16 c) const { return EndsWith(&c, 1); } // 0x28c4d0

Int32 String::CompareNoCase(const String& s) const                // 0x28c5a0
{
    return (Int32)CFStringCompareWithOptions(GET(*this), GET(s), CFRangeMake(0, GetLength()),
                                             kCFCompareCaseInsensitive);
}

Int32 String::CompareNoCase(UInt32 start, UInt32 count, const String& s) const // 0x28c5e0
{
    return (Int32)CFStringCompareWithOptions(GET(*this), GET(s), ClampRange(GetLength(), start, count),
                                             kCFCompareCaseInsensitive);
}

Int32 String::CompareNoCase(UInt32 start, UInt32 count, const String& s,
                            UInt32 sStart, UInt32 sCount) const     // 0x28c680
{
    CFStringRef sub = CreateSub(GET(s), sStart, sCount);
    Int32 r = (Int32)CFStringCompareWithOptions(GET(*this), sub, ClampRange(GetLength(), start, count),
                                                kCFCompareCaseInsensitive);
    CFRelease(sub);
    return r;
}

bool String::EqualsNoCase(const String& s) const { return CompareNoCase(s) == 0; }   // 0x28c7d0
bool String::EqualsNoCase(UInt32 a, UInt32 b, const String& s, UInt32 c, UInt32 d) const // 0x28c820
{
    return CompareNoCase(a, b, s, c, d) == 0;
}
bool String::LessNoCase(const String& s) const { return CompareNoCase(s) < 0; }      // 0x28c850

// ===========================================================================
// splitting / joining
// ===========================================================================

void String::Split(const String& sep, BasicArray<String>& out) const // 0x28c950
{
    out.Clear();
    if (GetLength() == 0)
        return;
    if (CFStringCompare(GET(*this), GET(sep), 0) == kCFCompareEqualTo) {
        out.PushBack(String());
        return;
    }
    CFArrayRef parts = CFStringCreateArrayBySeparatingStrings(0, GET(*this), GET(sep));
    if (!parts)
        return;
    out.Reserve((UInt32)CFArrayGetCount(parts));
    for (CFIndex i = 0; i < CFArrayGetCount(parts); ++i)
        out.PushBack(String((CFStringRef)CFArrayGetValueAtIndex(parts, i)));
    CFRelease(parts);
}

// 0x28c890: Split(String(sep)) -- keeps an empty last part ("a/" -> {a, ""}), unlike the Win
// build (0x403b20) which drops it.
void String::Split(UniChar16 sep, BasicArray<String>& out) const  // 0x28c890
{
    Split(String(&sep, 1), out);
}

void String::SplitLines(BasicArray<String>& out) const            // 0x28d420
{
    String s(*this);
    s.ReplaceAll(String("\n\r"), String("\n"));
    s.ReplaceAll(String("\r\n"), String("\n"));
    s.ReplaceAll(String("\r"), String("\n"));
    s.Split(String("\n"), out);
}

void String::Join(const String& sep, const BasicArray<String>& parts) // 0x28cdb0
{
    Clear();
    if (parts.IsEmpty())
        return;
    CFMutableArrayRef a = CFArrayCreateMutable(0, parts.GetSize(), 0);
    for (BasicArray<String>::ConstIterator it = parts.Begin(); it != parts.End(); ++it)
        CFArrayAppendValue(a, it->GetCFString());
    m_string = CFStringCreateByCombiningStrings(0, a, GET(sep));
    CFRelease(a);
}

void String::Join(UniChar16 sep, const BasicArray<String>& parts) // 0x28ccf0
{
    Join(String(&sep, 1), parts);
}

#undef GET

} // namespace LEGO
