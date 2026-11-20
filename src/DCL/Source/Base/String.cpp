// LEGO Digital Designer — restored source
// LEGO::String, Windows implementation on BasicString<UniChar16>. From the Win build: Reserve
// 0x401350, Append 0x401e90/0x401f80, release 0x4017c0, Int32 0x402300, UInt32 0x402380, UInt64
// 0x4023f0, Real32 0x402470, literal append 0x403ff0/0x404100. Search/compare/split/parse are
// mostly inlined there; written to the String.mm contract (the restoration's tests check both).
#if defined(_WIN32)

#include "LEGO/String.h"
#include "BasicArray.h"
#include "LEGO/StdLib.h"
#include "StringConversion.h"

#include <errno.h>
#include <float.h>
#include <stdio.h>
#include <wchar.h>
#include <wctype.h>
#include <stdlib.h>
#include <string.h>

namespace LEGO {

namespace {

inline UniChar16 Fold(UniChar16 c) { return (c >= 'A' && c <= 'Z') ? (UniChar16)(c + 32) : c; }

// Win 0x4319b0 / 0x431a10 / 0x431c00 — integer to UTF-16 (digits generated reversed)
template <class U>
UInt32 FormatUnsigned(UniChar16* buf, U v, bool negative)
{
    UniChar16* p = buf;
    if (negative)
        *p++ = '-';
    UniChar16* first = p;
    do {
        U d = v % 10;
        *p++ = (UniChar16)('0' + d);
        v /= 10;
    } while (v);
    *p = 0;
    for (UniChar16 *a = first, *b = p - 1; a < b; ++a, --b) {
        UniChar16 t = *a;
        *a = *b;
        *b = t;
    }
    return (UInt32)(p - buf);
}

#if !defined(_MSC_VER) || _MSC_VER != 1500
// --- VS2008 CRT "%.<n>G" (restoration aid, not original code) ------------------------------
// The original uses swprintf "%.<n>LG" (Win 0x431c50), which other CRTs do not reproduce:
// _fltout2 (0x6edf7b) → 80-bit (__dtold 0x6edebe) → $I10_OUTPUT (0x6f25cf): 17 digits in 96-bit
// arithmetic, rounded on an 18th; %G crops zeros, 3-digit exponent ("1E+030"); NaN / inf from
// "1#INF", "1#QNAN", "1#SNAN", "1#IND" with character rounding ("1.#QNB" at precision 5). Followed
// operation by operation (e.g. 414.010223388671875 -> ...87, 7.47310638427734375 -> ...38).
// The restoration's diff tests check it against the original.

// 12-byte "long double" of the CRT: words (little endian) 0 = 16 extra bits, 1-2 = low 32
// mantissa bits, 3-4 = high 32 mantissa bits, 5 = sign and exponent.
struct LD12 {
    UInt16 w[6];
};

inline UInt32 Long12(const LD12& x, int word) { return x.w[word] | ((UInt32)x.w[word + 1] << 16); }
inline void SetLong12(LD12& x, int word, UInt32 v)
{
    x.w[word] = (UInt16)v;
    x.w[word + 1] = (UInt16)(v >> 16);
}

void Shl12(LD12& x)          // __shl_12: the 96 bits left by one
{
    UInt32 lo = Long12(x, 0), med = Long12(x, 2), hi = Long12(x, 4);
    SetLong12(x, 4, (hi << 1) | (med >> 31));
    SetLong12(x, 2, (med << 1) | (lo >> 31));
    SetLong12(x, 0, lo << 1);
}

void Shr12(LD12& x)          // __shr_12
{
    UInt32 lo = Long12(x, 0), med = Long12(x, 2), hi = Long12(x, 4);
    SetLong12(x, 0, (lo >> 1) | (med << 31));
    SetLong12(x, 2, (med >> 1) | (hi << 31));
    SetLong12(x, 4, hi >> 1);
}

void Add12(LD12& x, const LD12& y)   // __add_12: 96-bit addition
{
    UInt64 carry = 0;
    for (int word = 0; word < 6; word += 2) {
        UInt64 t = (UInt64)Long12(x, word) + Long12(y, word) + carry;
        SetLong12(x, word, (UInt32)t);
        carry = t >> 32;
    }
}

// __ld12mul: truncated 80 x 80 bit product of the mantissas (16-bit partial products whose
// weight reaches the top 96 bits), normalized, rounded to 80 bits on the 16 extra bits.
void Mul12(LD12& x, const LD12& y)
{
    UInt16 sign = (UInt16)((x.w[5] ^ y.w[5]) & 0x8000);
    UInt16 ex = (UInt16)(x.w[5] & 0x7fff), ey = (UInt16)(y.w[5] & 0x7fff);
    UInt16 sum = (UInt16)(ex + ey);
    if (ex >= 0x7fff || ey >= 0x7fff || sum > 0x7fff + 0x3ffe) {
        LD12 inf = { { 0, 0, 0, 0, 0x8000, (UInt16)(0x7fff | sign) } };
        x = inf;
        return;
    }
    if (sum <= 0x3ffe - 63) {
        LD12 zero = { { 0, 0, 0, 0, 0, 0 } };
        x = zero;
        return;
    }
    // (denormal operands do not occur here: the scaled value and the table entries are normal)
    LD12 t = { { 0, 0, 0, 0, 0, 0 } };
    for (int i = 0; i < 5; ++i) {
        for (int j = 5 - i, p = i, q = 4; j > 0; --j, ++p, --q) {
            UInt32 product = (UInt32)x.w[p] * (UInt32)y.w[q];
            UInt32 r = Long12(t, i);
            UInt32 rs = r + product;
            SetLong12(t, i, rs);
            if ((rs < r || rs < product) && i + 2 < 6)
                ++t.w[i + 2];
        }
    }
    sum = (UInt16)(sum - 0x3ffe);
    while ((Int16)sum > 0 && (Long12(t, 4) & 0x80000000u) == 0) {
        Shl12(t);
        --sum;
    }
    if ((Int16)sum <= 0) {
        --sum;
        int sticky = 0;
        while ((Int16)sum < 0) {
            if (t.w[0] & 1)
                ++sticky;
            Shr12(t);
            ++sum;
        }
        if (sticky)
            t.w[0] |= 1;
    }
    if (t.w[0] > 0x8000 || (Long12(t, 0) & 0x1ffff) == 0x18000) {   // half even on word 0
        UInt32 lo = Long12(t, 1);
        if (lo == 0xffffffffu) {
            SetLong12(t, 1, 0);
            UInt32 hi = Long12(t, 3);
            if (hi == 0xffffffffu) {
                SetLong12(t, 3, 0);
                if (t.w[5] == 0xffff) {
                    t.w[5] = 0x8000;
                    ++sum;
                } else {
                    ++t.w[5];
                }
            } else {
                SetLong12(t, 3, hi + 1);
            }
        } else {
            SetLong12(t, 1, lo + 1);
        }
    }
    if (sum >= 0x7fff) {
        LD12 inf = { { 0, 0, 0, 0, 0x8000, (UInt16)(0x7fff | sign) } };
        x = inf;
        return;
    }
    x.w[0] = t.w[1];
    x.w[1] = t.w[2];
    x.w[2] = t.w[3];
    x.w[3] = t.w[4];
    x.w[4] = t.w[5];
    x.w[5] = (UInt16)(sum | sign);
}

// _pow10pos / _pow10neg of the VS2008 CRT (0xabb020 / 0xabb180): 10^1..10^7, 10^8..10^56,
// 10^64..10^448 in steps of the octal digits of the power (the doubles need no more).
const LD12 kPow10Pos[21] = {
    { { 0x0000, 0x0000, 0x0000, 0x0000, 0xa000, 0x4002 } }, { { 0x0000, 0x0000, 0x0000, 0x0000, 0xc800, 0x4005 } },
    { { 0x0000, 0x0000, 0x0000, 0x0000, 0xfa00, 0x4008 } }, { { 0x0000, 0x0000, 0x0000, 0x0000, 0x9c40, 0x400c } },
    { { 0x0000, 0x0000, 0x0000, 0x0000, 0xc350, 0x400f } }, { { 0x0000, 0x0000, 0x0000, 0x0000, 0xf424, 0x4012 } },
    { { 0x0000, 0x0000, 0x0000, 0x8000, 0x9896, 0x4016 } }, { { 0x0000, 0x0000, 0x0000, 0x2000, 0xbebc, 0x4019 } },
    { { 0x0000, 0x0000, 0x0400, 0xc9bf, 0x8e1b, 0x4034 } }, { { 0x0000, 0xa100, 0xcced, 0x1bce, 0xd3c2, 0x404e } },
    { { 0xf020, 0xb59e, 0x2b70, 0xada8, 0x9dc5, 0x4069 } }, { { 0x5dd0, 0x25fd, 0x1ae5, 0x4f8e, 0xeb19, 0x4083 } },
    { { 0x9671, 0x95d7, 0x0e43, 0x8d05, 0xaf29, 0x409e } }, { { 0xbff9, 0x44a0, 0x81ed, 0x8f12, 0x8281, 0x40b9 } },
    { { 0x3cbf, 0xa6d5, 0xffcf, 0x1f49, 0xc278, 0x40d3 } }, { { 0xc66f, 0x8ce0, 0x80e9, 0x47c9, 0x93ba, 0x41a8 } },
    { { 0x85bc, 0x556b, 0x3927, 0xf78d, 0xe070, 0x427c } }, { { 0xddbc, 0xde8e, 0x9df9, 0xebfb, 0xaa7e, 0x4351 } },
    { { 0xe6a1, 0xe376, 0xf2cc, 0x2f29, 0x8184, 0x4426 } }, { { 0x1028, 0xaa17, 0xaef8, 0xe310, 0xc4c5, 0x44fa } },
    { { 0xa7eb, 0xf3d4, 0xebf7, 0x4ae1, 0x957a, 0x45cf } },
};
const LD12 kPow10Neg[21] = {
    { { 0xcccd, 0xcccd, 0xcccc, 0xcccc, 0xcccc, 0x3ffb } }, { { 0x3d71, 0xd70a, 0x70a3, 0x0a3d, 0xa3d7, 0x3ff8 } },
    { { 0x645a, 0xdf3b, 0x8d4f, 0x6e97, 0x8312, 0x3ff5 } }, { { 0xd3c3, 0x652c, 0xe219, 0x1758, 0xd1b7, 0x3ff1 } },
    { { 0x0fd0, 0x8423, 0x1b47, 0xac47, 0xa7c5, 0x3fee } }, { { 0xa640, 0x69b6, 0xaf6c, 0xbd05, 0x8637, 0x3feb } },
    { { 0x3d33, 0x42bc, 0xe57a, 0x94d5, 0xd6bf, 0x3fe7 } }, { { 0xfdc2, 0xcefd, 0x8461, 0x7711, 0xabcc, 0x3fe4 } },
    { { 0x4c2f, 0xe15b, 0xc44d, 0x94be, 0xe695, 0x3fc9 } }, { { 0xc492, 0x3b53, 0x4475, 0x14cd, 0x9abe, 0x3faf } },
    { { 0x67de, 0x94ba, 0x4539, 0x1ead, 0xcfb1, 0x3f94 } }, { { 0x2324, 0xe2c6, 0xbabc, 0x313b, 0x8b61, 0x3f7a } },
    { { 0x5561, 0xc159, 0xb17e, 0x7c53, 0xbb12, 0x3f5f } }, { { 0xeed7, 0x8d2f, 0xbe06, 0x8592, 0xfb15, 0x3f44 } },
    { { 0x3f24, 0xe9a5, 0xa539, 0xea27, 0xa87f, 0x3f2a } }, { { 0xac7d, 0xe4a1, 0x64bc, 0x467c, 0xddd0, 0x3e55 } },
    { { 0x7b63, 0xcc06, 0x5423, 0x8377, 0x91ff, 0x3d81 } }, { { 0xfa91, 0x193a, 0x637a, 0x4325, 0xc031, 0x3cac } },
    { { 0x8921, 0x38d1, 0x4782, 0xb897, 0xfd00, 0x3bd7 } }, { { 0x88dc, 0x0858, 0xb11b, 0xe3e8, 0xa686, 0x3b03 } },
    { { 0x84c6, 0x4245, 0xb607, 0x7599, 0xdb37, 0x3a2e } },
};

// __multtenpow12: x *= 10^power, one table factor per octal digit of |power|; a factor whose
// extra bits are >= 0x8000 is "unrounded" (low mantissa word minus one) first.
void MulTenPow12(LD12& x, int power)
{
    const LD12* table = kPow10Pos;
    if (power < 0) {
        power = -power;
        table = kPow10Neg;
    }
    for (int group = 0; power; ++group, power >>= 3) {
        int last3 = power & 7;
        if (!last3)
            continue;
        LD12 factor = table[group * 7 + last3 - 1];
        if (factor.w[0] >= 0x8000)
            SetLong12(factor, 1, Long12(factor, 1) - 1);
        Mul12(x, factor);
    }
}

// __dtold + $I10_OUTPUT(value, 17 digits): the digit string ("1#INF" etc. for the special
// values; no trailing zeros) and the decimal point position (value = 0.digits * 10^decpt).
void I10Output(double v, char* digits, int& decpt)
{
    UInt64 bits;
    memcpy(&bits, &v, sizeof bits);
    UInt32 dlo = (UInt32)bits, dhi = (UInt32)(bits >> 32);
    // __dtold
    UInt16 expn = (UInt16)((dhi >> 20) & 0x7ff);
    UInt32 manhi, manlo;
    if (expn == 0 && (dhi & 0xfffff) == 0 && dlo == 0) {
        manhi = manlo = 0;
    } else {
        UInt32 top = 0x80000000u;
        if (expn == 0) {
            expn = 0x3c01;
            top = 0;
        } else if (expn == 0x7ff) {
            expn = 0x7fff;
        } else {
            expn = (UInt16)(expn + 0x3c00);
        }
        manhi = (dlo >> 21) | ((dhi & 0xfffff) << 11) | top;
        manlo = dlo << 11;
        while (!(manhi & 0x80000000u)) {
            manhi = (manhi << 1) | (manlo >> 31);
            manlo <<= 1;
            --expn;
        }
    }
    bool negative = (dhi & 0x80000000u) != 0;
    if (expn == 0 && manhi == 0 && manlo == 0) {
        strcpy(digits, "0");
        decpt = 0;
        return;
    }
    if (expn == 0x7fff) {
        decpt = 1;
        if ((manhi == 0x80000000u && manlo == 0) || (manhi & 0x40000000u)) {
            if (!negative || manhi != 0xc0000000u)
                strcpy(digits, manhi == 0x80000000u && manlo == 0 ? "1#INF" : "1#QNAN");
            else
                strcpy(digits, manlo ? "1#QNAN" : "1#IND");
        } else {
            strcpy(digits, "1#SNAN");
        }
        return;
    }
    // estimate of the decimal exponent (log10(2) in fixed point)
    Int16 ir = (Int16)((((UInt32)(expn >> 8) + (manhi >> 24) * 2) * 0x4d - 0x134312f4u +
                        (UInt32)expn * 0x4d10) >> 16);
    LD12 x = { { 0, (UInt16)manlo, (UInt16)(manlo >> 16), (UInt16)manhi, (UInt16)(manhi >> 16), expn } };
    MulTenPow12(x, -ir);
    if (x.w[5] >= 0x3fff) {
        ++ir;
        LD12 tenth = { { 0xcccc, 0xcccc, 0xcccc, 0xcccc, 0xcccc, 0x3ffb } };
        Mul12(x, tenth);
    }
    decpt = ir;
    // fixed point 0.xxx with the integer part in the top byte, then 18 digits
    int exponent = (int)x.w[5] - 0x3ffe;
    x.w[5] = 0;
    for (int i = 0; i < 8; ++i)
        Shl12(x);
    if (exponent < 0)
        for (int i = (-exponent) & 0xff; i > 0; --i)
            Shr12(x);
    char text[20];
    for (int i = 0; i < 18; ++i) {
        LD12 t = x;
        Shl12(x);
        Shl12(x);
        Add12(x, t);
        Shl12(x);
        text[i] = (char)('0' + (x.w[5] >> 8));
        x.w[5] &= 0xff;
    }
    int last = 16;
    if (text[17] >= '5') {
        while (last >= 0 && text[last] == '9')
            text[last--] = '0';
        if (last < 0) {
            last = 0;
            ++decpt;
        }
        ++text[last];
    } else {
        while (last >= 0 && text[last] == '0')
            --last;
        if (last < 0) {
            strcpy(digits, "0");
            decpt = 0;
            return;
        }
    }
    memcpy(digits, text, last + 1);
    digits[last + 1] = 0;
}

// _cftog + _cropzeros of the VS2008 CRT for "%.<precision>G".
UInt32 FormatRealVS2008(char* out, double v, int precision)
{
    UInt64 bits;
    memcpy(&bits, &v, sizeof bits);
    bool negative = (bits >> 63) != 0;
    char digits[32];
    int decpt;
    I10Output(v, digits, decpt);
    if (precision <= 0)
        precision = 1;
    // _fptostr: `precision` significant characters, rounded on the next character
    char r[600];
    int dlen = (int)strlen(digits);
    int keep = precision < (int)sizeof(r) - 2 ? precision : (int)sizeof(r) - 2;
    for (int i = 0; i < keep; ++i)
        r[i] = i < dlen ? digits[i] : '0';
    r[keep] = 0;
    if (keep < dlen && digits[keep] >= '5') {
        int i = keep - 1;
        while (i >= 0 && r[i] == '9')
            r[i--] = '0';
        if (i < 0) {
            memmove(r + 1, r, keep);
            r[0] = '1';
            r[keep] = 0;
            ++decpt;
        } else {
            ++r[i];
        }
    }
    // significant characters without trailing zeros (%G crops them)
    int rlen = keep;
    while (rlen > 0 && r[rlen - 1] == '0')
        --rlen;
    char* o = out;
    if (negative)
        *o++ = '-';
    int magnitude = decpt - 1;
    if (rlen == 0) {
        *o++ = '0';
    } else if (magnitude < -4 || magnitude >= precision) {
        *o++ = r[0];
        if (rlen > 1) {
            *o++ = '.';
            memcpy(o, r + 1, rlen - 1);
            o += rlen - 1;
        }
        o += sprintf(o, "E%c%03d", magnitude < 0 ? '-' : '+', magnitude < 0 ? -magnitude : magnitude);
    } else if (decpt <= 0) {
        *o++ = '0';
        *o++ = '.';
        for (int i = 0; i < -decpt; ++i)
            *o++ = '0';
        memcpy(o, r, rlen);
        o += rlen;
    } else {
        for (int i = 0; i < decpt; ++i)
            *o++ = i < rlen ? r[i] : '0';
        if (rlen > decpt) {
            *o++ = '.';
            memcpy(o, r + decpt, rlen - decpt);
            o += rlen - decpt;
        }
    }
    *o = 0;
    return (UInt32)(o - out);
}
#endif

// Win 0x431c50(precision, value) — swprintf "%.<n>LG" (long double = double in VS2008); a
// decimal comma of the C locale is replaced by a point.
UInt32 FormatReal(UniChar16* buf, double v, int precision)
{
    char tmp[600];
#if defined(_MSC_VER) && _MSC_VER == 1500
    _snprintf(tmp, sizeof(tmp) - 1, "%.*G", precision, v);
    tmp[sizeof(tmp) - 1] = 0;
#else
    FormatRealVS2008(tmp, v, precision);
#endif
    UInt32 n = 0;
    for (; tmp[n]; ++n)
        buf[n] = (UniChar16)(unsigned char)tmp[n];
    buf[n] = 0;
    for (UInt32 i = 0; i < n; ++i) {
        if (buf[i] == ',') {
            buf[i] = '.';
            break;
        }
    }
    return n;
}

int Compare(const UniChar16* a, UInt32 na, const UniChar16* b, UInt32 nb, bool noCase)
{
    UInt32 n = na < nb ? na : nb;
    for (UInt32 i = 0; i < n; ++i) {
        UniChar16 x = a[i], y = b[i];
        if (noCase) {
            x = Fold(x);
            y = Fold(y);
        }
        if (x != y)
            return x < y ? -1 : 1;
    }
    return na == nb ? 0 : (na < nb ? -1 : 1);
}

inline void Clamp(UInt32 len, UInt32& start, UInt32& count)
{
    // NOTE: start + count wraps for a "negative" start (GetLength() - 4 on a shorter name, e.g.
    // the extension checks of LiffStorageDirectory / LuaScriptManager / LDDApplication): the
    // 32-bit original then indexes 2 characters before the buffer (wrapping pointer arithmetic)
    // and compares unequal - kept there (the check below is compiled out). PORT64: on 64-bit
    // that address is far outside the string, so a start beyond the string is the empty range
    // (what the else branch below does when start + count does not wrap).
    if (sizeof(void*) > 4 && start > len) {
        start = 0;
        count = 0;
        return;
    }
    if (len < start + count) {
        if (start <= len) {
            count = len - start;
        } else {
            start = 0;
            count = 0;
        }
    }
}

inline bool InSet(const String& set, UniChar16 c)
{
    for (UInt32 i = 0; i < set.GetLength(); ++i)
        if (set.GetBuffer()[i] == c)
            return true;
    return false;
}

// iswspace of the VS2008 CRT (Win 0x6cb492 = iswctype(c, _SPACE)): the CRT's _wctype table
// below 0x100 (0x85 is not a space there), GetStringTypeW(CT_CTYPE1) C1_SPACE above. The set is
// the one the original returns for 0..0xFFFF (the restoration's diff tests compare all of them).
inline bool IsSpaceChar(UniChar16 c)
{
    if (c < 0x100)
        return (c >= 0x09 && c <= 0x0D) || c == 0x20 || c == 0xA0;
    return c == 0x1680 || (c >= 0x2000 && c <= 0x200A) || c == 0x2028 || c == 0x2029 ||
           c == 0x202F || c == 0x205F || c == 0x3000;
}

// Win 0x4317f0 — UTF-16 copy of StrToInteger (StdLib.cpp, Win 0x431d30) with radix 10 and an
// unsigned result: leading iswspace skipped, optional sign; a '-' sign or an overflow is a range
// error returning ~0; no digits -> 0 and *end = s.
UInt64 WideStrToUInt64(const UniChar16* s, const UniChar16** end, bool& rangeError)
{
    const UniChar16* p = s;
    UniChar16 c;
    do {
        c = *p++;
    } while (IsSpaceChar(c));
    bool negative = false;
    if (c == '-') {
        negative = true;
        c = *p++;
    } else if (c == '+') {
        c = *p++;
    }
    bool digits = false, overflow = false;
    UInt64 acc = 0;
    for (; c >= '0' && c <= '9'; c = *p++) {
        UInt32 d = c - '0';
        digits = true;
        if (acc > 0x1999999999999999ULL || (acc == 0x1999999999999999ULL && d > 5))
            overflow = true;
        else
            acc = acc * 10 + d;
    }
    if (!digits) {
        *end = s;
        return 0;
    }
    *end = p - 1;
    if (overflow || negative) {
        rangeError = true;
        return ~(UInt64)0;
    }
    return acc;
}

} // namespace

// --- construction ------------------------------------------------------------
String::String() {}
String::String(const String& s) : m_str(s.m_str) {}
String::String(const String& s, UInt32 start, UInt32 count)
{
    UInt32 len = s.GetLength();
    if (count != 0 && start <= len) {
        UInt32 n = len - start;
        if (count < n)
            n = count;
        m_str.Assign(s.GetBuffer() + start, n);
    }
}
String::String(const UniChar16* chars, UInt32 length) { if (chars && length) m_str.Assign(chars, length); }
String::String(const UniChar16* z) { m_str.Assign(z, BasicString<UniChar16>::Length(z)); }
String::String(const AString& a)
{
    UInt32 n = a.GetLength();
    if (n && m_str.Reserve(n, true)) {
        for (UInt32 i = 0; i < n; ++i)
            m_str.GetWritableBuffer()[i] = (UniChar16)(unsigned char)a.c_str()[i];
        m_str.SetLength(n);
    }
}
String::String(const char* s, UInt32 length)
{
    if (s && length && m_str.Reserve(length, true)) {
        for (UInt32 i = 0; i < length; ++i)
            m_str.GetWritableBuffer()[i] = (UniChar16)(unsigned char)s[i];
        m_str.SetLength(length);
    }
}
String::String(const char* s)                                     // Win 0x4016e0
{
    UInt32 n = s ? StrLen(s) : 0;
    if (n && m_str.Reserve(n, true)) {
        for (UInt32 i = 0; i < n; ++i)
            m_str.GetWritableBuffer()[i] = (UniChar16)(unsigned char)s[i];
        m_str.SetLength(n);
    }
}
String::String(UInt32 count, UniChar16 fill)
{
    if (count && m_str.Reserve(count, true)) {
        for (UInt32 i = 0; i < count; ++i)
            m_str.GetWritableBuffer()[i] = fill;
        m_str.SetLength(count);
    }
}
String::~String() {}                                              // Win 0x4017c0

// --- assignment --------------------------------------------------------------
String& String::operator=(const String& s) { m_str = s.m_str; return *this; }
String& String::operator=(const char* s)   { return *this = String(s); }
String& String::operator=(const AString& a){ return *this = String(a); }
String& String::Assign(const String& s, UInt32 start, UInt32 count) { return *this = String(s, start, count); }
String& String::Assign(const char* s, UInt32 n) { return *this = (s && n) ? String(s, n) : String(); }

// --- access ------------------------------------------------------------------
UInt32 String::GetLength() const              { return m_str.GetLength(); }
bool   String::IsEmpty() const                { return m_str.GetLength() == 0; }
UniChar16 String::GetAt(UInt32 i) const       { return m_str[i]; }
const UniChar16* String::GetBuffer() const    { return m_str.GetBuffer(); }
String String::Substring(UInt32 start, UInt32 count) const { return String(*this, start, count); }

bool String::IsCharAt(UInt32 index, UniChar16 c) const
{
    if (index == npos)
        index = GetLength() ? GetLength() - 1 : npos;
    return index < GetLength() && GetAt(index) == c;
}

bool String::IsWhitespaceAt(UInt32 index) const                  // Win 0x401b40 (iswspace)
{
    if (index == npos)
        index = GetLength() ? GetLength() - 1 : npos;
    return index < GetLength() && IsSpaceChar(GetAt(index));
}

bool String::IsWhitespace(UniChar16 c) { return IsSpaceChar(c); }   // Win 0x401b80 (iswspace)
bool String::IsDigit(UniChar16 c)      { return c >= '0' && c <= '9'; }

bool String::IsASCII() const
{
    for (UInt32 i = 0; i < GetLength(); ++i)
        if (GetAt(i) > 0x7F)
            return false;
    return true;
}

AString String::ToAString() const
{
    AString r;
    if (!IsASCII())
        return r;
    UInt32 n = GetLength();
    if (n && r.Reserve(n, true)) {
        for (UInt32 i = 0; i < n; ++i)
            r.GetWritableBuffer()[i] = (char)GetAt(i);
        r.SetLength(n);
    }
    return r;
}

// Win 0x404410 -> 0x432a10 (TU 015 StringConversion.cpp, String passed in EDI)
AString String::ToUTF8() const
{
    return StringToUTF8(*this);
}

String String::ToXML() const                                      // Win 0x404470
{
    String r(*this);
    r.ReplaceAll(String("&"), String("&amp;"));
    r.ReplaceAll(String("<"), String("&lt;"));
    r.ReplaceAll(String(">"), String("&gt;"));
    r.ReplaceAll(String("'"), String("&apos;"));
    r.ReplaceAll(String("\""), String("&quot;"));
    return r;
}

// Win 0x4019b0 (EDI = this; result, position on the stack; called by NextWord 0x5a5ed0): the
// characters from *position up to the next iswspace character. Unlike the Mac NSScanner
// version (String.mm), leading white space is not skipped — at a blank the word is empty and
// *position stays — and *position is updated whenever the start lies inside the string.
String String::ScanWord(UInt32* position) const
{
    UInt32 start = position ? *position : 0;
    if (start >= GetLength())
        return String();
    UInt32 i = start;
    while (i < GetLength() && !IsSpaceChar(GetAt(i)))
        ++i;
    if (position)
        *position = i;
    return String(*this, start, i - start);
}

// --- modification ------------------------------------------------------------
String& String::Append(const String& s)       { m_str.Append(s.m_str); return *this; }       // 0x401e90
String& String::Append(const char* s)         { return Append(String(s)); }                  // 0x403ff0
String& String::Append(const AString& a)      { return Append(String(a)); }
// Win 0x402040 / 0x402210 (Append(c) = Append(1, c)).
// NOTE: the Win build appends nothing for c == 0 (0x402064: cmp word [esp+0x14], 0; je).
String& String::Append(UInt32 count, UniChar16 c) { return c ? Append(String(count, c)) : *this; }
String& String::Append(UniChar16 c)           { if (c) m_str.Append(&c, 1); return *this; }

String& String::Insert(UInt32 pos, const String& s)
{
    if (pos == npos)
        return Append(s);
    m_str.Insert(pos, s.GetBuffer(), s.GetLength());
    return *this;
}

// Win 0x402a70: pos > length (npos included) or a result of 0xFFFFFFFF characters or more throws
// RangeException*; then c == 0 inserts nothing (0x402a70 returns after the checks), as
// Append(count, c) above does for c == 0.
String& String::Insert(UInt32 pos, UInt32 count, UniChar16 c)
{
    const UInt32 len = GetLength();
    if (pos > len || len >= ~count)
        ThrowRangeException();
    if (c == 0)
        return *this;
    String fill(count, c);
    m_str.Insert(pos, fill.GetBuffer(), fill.GetLength());
    return *this;
}

// The checks of Win 0x402b70 (Replace by a BasicString): start > length or a result of 0xFFFFFFFF
// characters or more throws RangeException*.
String& String::Replace(UInt32 start, UInt32 count, const String& s)
{
    if (start == npos)
        return Append(s);
    if (start > GetLength())
        ThrowRangeException();
    if (GetLength() < start + count)
        count = GetLength() - start;
    if (GetLength() - count >= ~s.GetLength())
        ThrowRangeException();
    String tail(*this, start + count);
    m_str.Erase(start, npos);
    m_str.Append(s.m_str);
    m_str.Append(tail.m_str);
    return *this;
}

String& String::ReplaceAll(const String& from, const String& to)
{
    if (from.IsEmpty())
        return *this;
    String r;
    UInt32 i = 0, len = GetLength(), fl = from.GetLength();
    while (i < len) {
        if (i + fl <= len && Compare(GetBuffer() + i, fl, from.GetBuffer(), fl, false) == 0) {
            r.m_str.Append(to.m_str);
            i += fl;
        } else {
            r.m_str.Append(GetBuffer() + i, 1);
            ++i;
        }
    }
    return *this = r;
}

// Win 0x402e80. NOTE: the Win build clips the counts with unsigned arithmetic only, so a start
// beyond the end of `from`/`to` reads past that string (garbage); clamped to empty here.
String& String::ReplaceAll(const String& from, UInt32 fs, UInt32 fc, const String& to, UInt32 ts, UInt32 tc)
{
    return ReplaceAll(String(from, fs, fc), String(to, ts, tc));
}

String& String::Erase(UInt32 start, UInt32 count) { m_str.Erase(start, count); return *this; }
void String::Clear()  { m_str.Clear(); }

void String::Trim()
{
    UInt32 b = 0, e = GetLength();
    while (b < e && (IsSpaceChar(GetAt(b)) || GetAt(b) == '\n' || GetAt(b) == '\r'))
        ++b;
    while (e > b && (IsSpaceChar(GetAt(e - 1)) || GetAt(e - 1) == '\n' || GetAt(e - 1) == '\r'))
        --e;
    *this = String(*this, b, e - b);
}

void String::TrimRight(const String& chars)
{
    UInt32 i = FindLastNotOf(chars, npos);
    if (i == npos)
        Clear();
    else
        Erase(i + 1, npos);
}

void String::ToLower()
{
    UInt32 n = GetLength();
    if (!n)
        return;
    m_str.Reserve(n, false);
    for (UInt32 i = 0; i < n; ++i)
        m_str.GetWritableBuffer()[i] = Fold(m_str.GetWritableBuffer()[i]);
}

// --- formatting --------------------------------------------------------------
String& String::operator<<(const String& s) { return Append(s); }
String& String::operator<<(const char* s)   { return Append(s); }
String& String::operator<<(bool b)          { return Append(b ? '1' : '0'); }

String& String::operator<<(UInt16 v)
{
    UniChar16 buf[32];
    m_str.Append(buf, FormatUnsigned(buf, (UInt32)v, false));
    return *this;
}

String& String::operator<<(Int32 v)                               // 0x402300
{
    UniChar16 buf[32];
    UInt32 u = v < 0 ? (UInt32)(-(Int64)v) : (UInt32)v;
    m_str.Append(buf, FormatUnsigned(buf, u, v < 0));
    return *this;
}

String& String::operator<<(UInt32 v)                              // 0x402380
{
    UniChar16 buf[32];
    m_str.Append(buf, FormatUnsigned(buf, v, false));
    return *this;
}

String& String::operator<<(UInt64 v)                              // 0x4023f0
{
    UniChar16 buf[32];
    m_str.Append(buf, FormatUnsigned(buf, v, false));
    return *this;
}

String& String::operator<<(Real32 v)                              // 0x402470
{
    UniChar16 buf[512];
    double d = (v < 0 ? -v : v) < 1e-05f ? 0.0 : (double)v;
    m_str.Append(buf, FormatReal(buf, d, 30));
    return *this;
}

String& String::operator<<(Real64 v)                              // 0x402500 (EDI = this)
{
    UniChar16 buf[512];
    double d = (v < 0 ? -v : v) < (double)1e-08f ? 0.0 : v;
    if (d != d) {
        // the value passes the x87 stack (fld/fstp qword), which quiets a signaling NaN
        UInt64 bits;
        memcpy(&bits, &d, sizeof bits);
        bits |= 0x0008000000000000ULL;
        memcpy(&d, &bits, sizeof d);
    }
    m_str.Append(buf, FormatReal(buf, d, 60));
    return *this;
}

// --- parsing -----------------------------------------------------------------
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

// The numeric parsers (Win 0x402820-0x402a6f, /LTCG register conventions) all work the same
// way: fail without touching anything when `pos` is not inside the string; otherwise convert
// with errno = 0, ALWAYS store the converted value (the clamped one on a range error: 0xFF for
// "300" or "-1", 0 when there are no digits), move `pos` to the end of the parsed text and
// succeed when something was parsed and errno is not ERANGE. Unlike the Mac build (which skips
// to the first sign/digit and leaves `out` alone on failure), leading text other than
// whitespace is not skipped.
namespace {
template <class T, class Convert>
bool ParseNumber(const String& s, T& out, UInt32& pos, Convert convert)
{
    if (pos >= s.GetLength())
        return false;
    const UniChar16* start = s.GetBuffer() + pos;
    const UniChar16* end = start;
    bool rangeError = false;
    out = convert(start, &end, rangeError);
    pos += (UInt32)(end - start);
    return end != start && !rangeError;
}

// Win 0x401250: StrToUInt8 on UTF-16
UInt8 ConvertUInt8(const UniChar16* s, const UniChar16** end, bool& rangeError)
{
    UInt64 v = WideStrToUInt64(s, end, rangeError);
    if (v > 0xFF) {
        rangeError = true;
        return 0xFF;
    }
    return (UInt8)v;
}

// Win 0x4012a0: StrToUInt32 on UTF-16
UInt32 ConvertUInt32(const UniChar16* s, const UniChar16** end, bool& rangeError)
{
    UInt64 v = WideStrToUInt64(s, end, rangeError);
    if (v >> 32) {
        rangeError = true;
        return 0xFFFFFFFF;
    }
    return (UInt32)v;
}

UInt64 ConvertUInt64(const UniChar16* s, const UniChar16** end, bool& rangeError)
{
    return WideStrToUInt64(s, end, rangeError);
}

// wcstod (VS2008 0x6cbe2b): ERANGE on overflow / underflow. Leading white space is skipped
// here with the VS2008 iswspace set (a host wcstod may also skip e.g. U+0085); the remaining
// CRT differences (host inf/nan/hex parsing, underflow results) are reported by diff_string.
Real64 ConvertReal64(const UniChar16* s, const UniChar16** end, bool& rangeError)
{
    const UniChar16* p = s;
    while (IsSpaceChar(*p))
        ++p;
    errno = 0;
    if (iswspace(*p)) {          // the host would skip it, VS2008 does not convert
        *end = s;
        rangeError = false;
        return 0.0;
    }
    wchar_t* e = 0;
    double v = wcstod((const wchar_t*)p, &e);
    *end = (const UniChar16*)e == p ? s : (const UniChar16*)e;
    rangeError = errno == ERANGE;
    return v;
}

// Win 0x431cd0: StrToReal32 on UTF-16 (wcstod, ERANGE outside +-FLT_MAX, narrowed to float)
Real32 ConvertReal32(const UniChar16* s, const UniChar16** end, bool& rangeError)
{
    double v = ConvertReal64(s, end, rangeError);
    if (v < -FLT_MAX || v > FLT_MAX)
        rangeError = true;
    return (Real32)v;
}
} // namespace

bool String::Parse(UInt8& out, UInt32& pos) const  { return ParseNumber(*this, out, pos, ConvertUInt8); }   // Win 0x402820
bool String::Parse(UInt32& out, UInt32& pos) const { return ParseNumber(*this, out, pos, ConvertUInt32); }  // Win 0x402890
bool String::Parse(UInt64& out, UInt32& pos) const { return ParseNumber(*this, out, pos, ConvertUInt64); }  // Win 0x402900
bool String::Parse(Real32& out, UInt32& pos) const { return ParseNumber(*this, out, pos, ConvertReal32); }  // Win 0x402980
bool String::Parse(Real64& out, UInt32& pos) const { return ParseNumber(*this, out, pos, ConvertReal64); }  // Win 0x4029f0

// --- searching ---------------------------------------------------------------
UInt32 String::Find(const String& s, UInt32 start) const
{
    UInt32 len = GetLength(), n = s.GetLength();
    if (start >= len || n == 0)
        return npos;
    for (UInt32 i = start; i + n <= len; ++i)
        if (Compare(GetBuffer() + i, n, s.GetBuffer(), n, false) == 0)
            return i;
    return npos;
}

UInt32 String::Find(UniChar16 c, UInt32 start) const
{
    for (UInt32 i = start; i < GetLength(); ++i)
        if (GetAt(i) == c)
            return i;
    return npos;
}

UInt32 String::FindNoCase(const String& s, UInt32 start) const
{
    UInt32 len = GetLength(), n = s.GetLength();
    if (start >= len || n == 0)
        return npos;
    for (UInt32 i = start; i + n <= len; ++i)
        if (Compare(GetBuffer() + i, n, s.GetBuffer(), n, true) == 0)
            return i;
    return npos;
}

UInt32 String::ReverseFind(const UniChar16* s, UInt32 start, UInt32 n) const
{
    UInt32 end = GetLength();
    if (start < ~n && start + n < end)
        end = start + n;
    if (n == 0 || n > end)
        return npos;
    for (UInt32 i = end - n + 1; i-- > 0;)
        if (Compare(GetBuffer() + i, n, s, n, false) == 0)
            return i;
    return npos;
}

UInt32 String::ReverseFind(UniChar16 c, UInt32 start) const { return ReverseFind(&c, start, 1); }

UInt32 String::FindFirstOf(const String& set, UInt32 start) const
{
    if (set.GetLength())
        for (; start < GetLength(); ++start)
            if (InSet(set, GetAt(start)))
                return start;
    return npos;
}

UInt32 String::FindLastOf(const String& set, UInt32 start) const
{
    if (set.GetLength() == 0 || GetLength() == 0)
        return npos;
    UInt32 i = GetLength() - 1;
    if (start < i)
        i = start;
    for (UInt32 k = i + 1; k != 0;) {
        --k;
        if (InSet(set, GetAt(k)))
            return k;
    }
    return npos;
}

UInt32 String::FindLastOf(const UniChar16* set, UInt32 start, UInt32 n) const { return FindLastOf(String(set, n), start); }
UInt32 String::FindLastOf(UniChar16 c, UInt32 start) const { return FindLastOf(&c, start, 1); }

UInt32 String::FindFirstNotOf(const String& set, UInt32 start) const
{
    if (start >= GetLength())
        return npos;
    if (set.GetLength() == 0)
        return start;
    for (; start < GetLength(); ++start)
        if (!InSet(set, GetAt(start)))
            return start;
    return npos;
}

UInt32 String::FindFirstNotOf(const UniChar16* set, UInt32 start, UInt32 n) const
{
    if (start >= GetLength())
        return npos;
    return FindFirstNotOf(String(set, n), start);
}

UInt32 String::FindFirstNotOf(const UniChar16* set, UInt32 start) const
{
    return FindFirstNotOf(set, start, BasicString<UniChar16>::Length(set));
}

UInt32 String::FindLastNotOf(const String& set, UInt32 start) const
{
    UInt32 i = GetLength() - 1;
    if (set.GetLength() == 0)
        return i;
    if (start < i)
        i = start;
    for (UInt32 k = i + 1;;) {
        if (k == 0)
            return npos;
        --k;
        if (!InSet(set, GetAt(k)))
            return k;
    }
}

UInt32 String::FindLastNotOf(const UniChar16* set, UInt32 start, UInt32 n) const { return FindLastNotOf(String(set, n), start); }
UInt32 String::FindLastNotOf(const UniChar16* set, UInt32 start) const
{
    return FindLastNotOf(set, start, BasicString<UniChar16>::Length(set));
}

// --- comparison --------------------------------------------------------------
bool String::operator==(const String& s) const { return Compare(GetBuffer(), GetLength(), s.GetBuffer(), s.GetLength(), false) == 0; }
bool String::operator!=(const String& s) const { return !(*this == s); }
bool String::operator<(const String& s) const  { return Compare(GetBuffer(), GetLength(), s.GetBuffer(), s.GetLength(), false) < 0; }

bool String::StartsWith(const String& s) const { return StartsWith(s.GetBuffer(), s.GetLength()); }
bool String::StartsWith(const UniChar16* s, UInt32 n) const
{
    return n <= GetLength() && (n == 0 || Compare(GetBuffer(), n, s, n, false) == 0);
}
bool String::StartsWith(UniChar16 c) const { return StartsWith(&c, 1); }
bool String::EndsWith(const UniChar16* s, UInt32 n) const
{
    return n <= GetLength() && (n == 0 || Compare(GetBuffer() + GetLength() - n, n, s, n, false) == 0);
}
bool String::EndsWith(UniChar16 c) const { return EndsWith(&c, 1); }

Int32 String::CompareNoCase(const String& s) const
{
    return Compare(GetBuffer(), GetLength(), s.GetBuffer(), s.GetLength(), true);
}

Int32 String::CompareNoCase(UInt32 start, UInt32 count, const String& s) const
{
    Clamp(GetLength(), start, count);
    return Compare(GetBuffer() + start, count, s.GetBuffer(), s.GetLength(), true);
}

Int32 String::CompareNoCase(UInt32 start, UInt32 count, const String& s, UInt32 ss, UInt32 sc) const
{
    return CompareNoCase(start, count, String(s, ss, sc));
}

bool String::EqualsNoCase(const String& s) const { return CompareNoCase(s) == 0; }   // Win 0x436720
bool String::EqualsNoCase(UInt32 a, UInt32 b, const String& s) const { return CompareNoCase(a, b, s) == 0; }
bool String::EqualsNoCase(UInt32 a, UInt32 b, const String& s, UInt32 c, UInt32 d) const
{
    return CompareNoCase(a, b, s, c, d) == 0;
}
bool String::LessNoCase(const String& s) const { return CompareNoCase(s) < 0; }      // Win 0x439650

// --- splitting / joining -------------------------------------------------------
void String::Split(const String& sep, BasicArray<String>& out) const
{
    out.Clear();
    if (GetLength() == 0)
        return;
    if (*this == sep) {
        out.PushBack(String());
        return;
    }
    UInt32 start = 0;
    for (;;) {
        UInt32 i = sep.IsEmpty() ? npos : Find(sep, start);
        if (i == npos) {
            out.PushBack(String(*this, start));
            return;
        }
        out.PushBack(String(*this, start, i - start));
        start = i + sep.GetLength();
    }
}

// Win 0x403b20. Unlike Split(const String&), an empty last part (the string ends with `sep`) is
// not added: after the last separator the rest is appended only when it is not empty
// (0x403bfa-0x403c03 `if (data && start < length)`), so "a/b/" -> {a, b}, "/" -> {""}, "" -> {}.
// The VFS relies on it: Directory::Enumerate("memory/dir/") lists "dir" in "memory"
// (the restoration's diff tests).
void String::Split(UniChar16 sep, BasicArray<String>& out) const
{
    out.Clear();
    UInt32 length = GetLength();
    UInt32 start = 0;
    for (;;) {
        UInt32 i = Find(sep, start);
        if (i == npos)
            break;
        out.PushBack(String(*this, start, i - start));
        start = i + 1;
    }
    if (start < length)
        out.PushBack(String(*this, start));
}

void String::SplitLines(BasicArray<String>& out) const
{
    String s(*this);
    s.ReplaceAll(String("\n\r"), String("\n"));
    s.ReplaceAll(String("\r\n"), String("\n"));
    s.ReplaceAll(String("\r"), String("\n"));
    s.Split(String("\n"), out);
}

void String::Join(const String& sep, const BasicArray<String>& parts)
{
    Clear();
    for (BasicArray<String>::ConstIterator it = parts.Begin(); it != parts.End(); ++it) {
        if (it != parts.Begin())
            Append(sep);
        Append(*it);
    }
}

void String::Join(UniChar16 sep, const BasicArray<String>& parts) { Join(String(&sep, 1), parts); }

} // namespace LEGO

#endif // _WIN32
