// LEGO Digital Designer — restored source
// TU 044 (Mac 0x10af70-0x10b75f): C runtime wrappers.
// Win copies 0x431cd0-0x43226f, checked by the diff tests.
//
#include "LEGO/StdLib.h"

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>

namespace LEGO {

UInt32 StrLen(const char* s)                          { return (UInt32)strlen(s); }
void*  MemMove(void* d, const void* s, size_t n)      { memmove(d, s, n); return d; }
void*  MemSet(void* d, int c, size_t n)               { memset(d, c, n); return d; }
void*  MemCopy(void* d, const void* s, size_t n)      { memcpy(d, s, n); return d; }
const void* MemChr(const void* p, char c, size_t n)   { return memchr(p, c, n); }
int    StrNCmp(const char* a, const char* b, size_t n) { return strncmp(a, b, n); }
int    StrCmp(const char* a, const char* b)           { return strcmp(a, b); }

int StrNICmp(const char* a, const char* b, size_t n)  // 0x10b040, Win 0x431fe0
{
    if (n == 0)
        return 0;
    unsigned char ca, cb;
    do {
        ca = (unsigned char)tolower(*a);
        cb = (unsigned char)tolower(*b);
        if (ca == 0 || ca != cb)
            break;
        ++a;
        ++b;
    } while (--n);
    return ca - cb;
}

int StrICmp(const char* a, const char* b)             // 0x10b0b0, Win 0x432060
{
    unsigned char ca, cb;
    do {
        ca = (unsigned char)tolower(*a);
        cb = (unsigned char)tolower(*b);
        if (ca == 0)
            break;
        ++a;
        ++b;
    } while (ca == cb);
    return ca - cb;
}

bool StrILess(const char* a, const char* b)           // 0x10b100
{
    unsigned char ca, cb;
    do {
        ca = (unsigned char)tolower(*a);
        cb = (unsigned char)tolower(*b);
        if (ca == 0)
            break;
        ++a;
        ++b;
    } while (ca == cb);
    return ca < cb;
}

bool IsSpace(char c) { return isspace((unsigned char)c) != 0; } // 0x10b150 (rune table, mask 0x4000)

namespace {

// 0x10b190 — copy of BSD strtoull/strtoll. flags: 1 = unsigned result,
// 2 = negative, 4 = overflow, 8 = any digits. Win 0x431d30 (flags fixed to 1 by /LTCG).
UInt64 StrToInteger(const char* nptr, char** endptr, int base, unsigned flags)
{
    const char* s = nptr;
    unsigned char c;
    do {
        c = (unsigned char)*s++;
    } while (isspace(c));
    if (c == '-') {
        flags |= 2;
        c = (unsigned char)*s++;
    } else if (c == '+') {
        c = (unsigned char)*s++;
    }
    if (base == 1 || base > 36) {
        if (endptr)
            *endptr = (char*)nptr;
        return 0;
    }
    if ((base == 0 || base == 16) && c == '0' && (*s | 0x20) == 'x') {
        c = (unsigned char)s[1];
        s += 2;
        base = 16;
    }
    if (base == 0)
        base = c == '0' ? 8 : 10;
    UInt64 cutoff = ~(UInt64)0 / (UInt64)base;
    UInt32 cutlim = (UInt32)(~(UInt64)0 % (UInt64)base);
    UInt64 acc = 0;
    for (;; c = (unsigned char)*s++) {
        UInt32 d;
        if (isdigit(c))
            d = c - '0';
        else if (isalpha(c))
            d = toupper(c) - 'A' + 10;
        else
            break;
        if (d >= (UInt32)base)
            break;
        if (acc > cutoff || (acc == cutoff && d > cutlim)) {
            flags |= 0xC;
        } else {
            flags |= 8;
            acc = acc * base + d;
        }
    }
    if (!(flags & 8)) {
        acc = 0;
        s = nptr + 1;
#if defined(_WIN32)
    } else if ((flags & 4) || (flags & 3) == 3) {
        // NOTE: the Win build (0x431f02-0x431f58) also treats a '-' sign on an unsigned
        // conversion as a range error: "-0" / "-5" give ERANGE and ~0 (the Mac build negates).
#else
    } else if (flags & 4) {
#endif
        errno = ERANGE;
        acc = (flags & 1) ? ~(UInt64)0 : ((flags & 2) ? (UInt64)1 << 63 : ~(UInt64)0 >> 1);
        flags &= ~2u;
    } else if (!(flags & 1)) {
        if ((flags & 2) ? acc > ((UInt64)1 << 63) : (Int64)acc < 0) {
            errno = ERANGE;
            acc = (flags & 2) ? (UInt64)1 << 63 : ~(UInt64)0 >> 1;
            flags &= ~2u;
        }
    }
    if (endptr)
        *endptr = (char*)(s - 1);
    return (flags & 2) ? (UInt64)(-(Int64)acc) : acc;
}

} // namespace

UInt8 StrToUInt8(const char* s, char** end, int radix)          // 0x10b480, Win 0x4320a0 (radix 10)
{
    errno = 0;
    UInt64 v = StrToInteger(s, end, radix, 1);
    if (v > 0xFF) {
        errno = ERANGE;
        return 0xFF;
    }
    return (UInt8)v;
}

UInt32 StrToUInt32(const char* s, char** end, int radix)        // 0x10b4e0, Win 0x432140 / 0x4320f0
{
    errno = 0;
    UInt64 v = StrToInteger(s, end, radix, 1);
    if (v >> 32) {
        errno = ERANGE;
        return 0xFFFFFFFF;
    }
    return (UInt32)v;
}

Real32 StrToReal32(const char* s, char** end)                   // 0x10b600, Win 0x432190
{
    errno = 0;
#if defined(_WIN32)
    // Win: strtod, ERANGE when outside +-FLT_MAX (constants Win 0xa2d788/0xa2d790), then
    // narrowed to float (an out-of-range value becomes +-infinity).
    double v = strtod(s, end);
    if (v < -FLT_MAX || v > FLT_MAX)
        errno = ERANGE;
    return (Real32)v;
#else
    return strtof(s, end);
#endif
}

void UInt32ToStr(UInt32 v, char* buf, UInt32 radix)             // 0x10b620, Win 0x4321f0
{
    int n = 0;
    do {
        UInt32 d = v % radix;
        buf[n++] = (char)(d < 10 ? '0' + d : 'a' + d - 10);
        v /= radix;
    } while (v);
    buf[n] = 0;
    for (int i = 0, j = n - 1; i < j; ++i, --j) {
        char t = buf[i];
        buf[i] = buf[j];
        buf[j] = t;
    }
}

void SkipSpaces(const char** p)                                 // 0x10b6a0, Win 0x432240
{
    while (isspace((unsigned char)**p))
        ++*p;
}

} // namespace LEGO
