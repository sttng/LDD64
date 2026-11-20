// LDD64 Linux (NOT restored code): the CoreFoundation subset of CoreFoundation/CoreFoundation.h
// for the Linux-hosted Mac branch. Strings are UTF-16 vectors with a refcount.
#include "CoreFoundation/CoreFoundation.h"

#include <map>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

namespace {
enum Kind { kString = 1, kArray, kCharSet, kLocale, kFormatter, kUUID };
struct Obj { int kind; int refs; bool constant; Obj(int k) : kind(k), refs(1), constant(false) {} virtual ~Obj() {} };
struct Str : Obj { std::vector<UniChar> c; Str() : Obj(kString) {} };
struct Arr : Obj { std::vector<const void*> v; Arr() : Obj(kArray) {} };
struct CSet : Obj { int which; CSet(int w) : Obj(kCharSet), which(w) { constant = true; } };
struct Misc : Obj { Misc(int k) : Obj(k) {} };
struct UUIDObj : Obj { unsigned char b[16]; UUIDObj() : Obj(kUUID) { memset(b, 0, sizeof b); } };

Str* S(CFTypeRef p) { return (Str*)(Obj*)p; }
Str* NewStr() { return new Str(); }
Str* NewStr(const UniChar* p, size_t n) { Str* s = new Str(); s->c.assign(p, p + n); return s; }

UniChar Fold(UniChar c) { return (c >= 'A' && c <= 'Z') ? c + 32 : c; }

bool Eq(UniChar a, UniChar b, CFOptionFlags o) { return (o & kCFCompareCaseInsensitive) ? Fold(a) == Fold(b) : a == b; }

int Cmp(const UniChar* a, size_t na, const UniChar* b, size_t nb, CFOptionFlags o)
{
    size_t n = na < nb ? na : nb;
    for (size_t i = 0; i < n; ++i) {
        UniChar x = a[i], y = b[i];
        if (o & kCFCompareCaseInsensitive) { x = Fold(x); y = Fold(y); }
        if (x != y) return x < y ? -1 : 1;
    }
    return na == nb ? 0 : (na < nb ? -1 : 1);
}

// Decodes `n` bytes of UTF-8 (stops early at a NUL byte only when n is (size_t)-1).
Str* FromUTF8N(const char* s, size_t n)
{
    Str* r = NewStr();
    const unsigned char* p = (const unsigned char*)s;
    const unsigned char* e = n == (size_t)-1 ? 0 : p + n;
    while (e ? p < e : *p != 0) {
        unsigned cp = *p++;
        if (cp >= 0xC0) {
            int extra = cp >= 0xF0 ? 3 : cp >= 0xE0 ? 2 : 1;
            cp &= (0x3F >> extra);
            while (extra-- && (e ? p < e : *p != 0)) cp = (cp << 6) | (*p++ & 0x3F);
        }
        if (cp >= 0x10000) { cp -= 0x10000; r->c.push_back(0xD800 + (cp >> 10)); r->c.push_back(0xDC00 + (cp & 0x3FF)); }
        else r->c.push_back((UniChar)cp);
    }
    return r;
}

Str* FromUTF8(const char* s) { return FromUTF8N(s, (size_t)-1); }

std::string ToUTF8(const Str* s)
{
    std::string o;
    for (size_t i = 0; i < s->c.size(); ++i) {
        unsigned cp = s->c[i];
        if (cp >= 0xD800 && cp < 0xDC00 && i + 1 < s->c.size()) cp = 0x10000 + ((cp - 0xD800) << 10) + (s->c[++i] - 0xDC00);
        if (cp < 0x80) o += (char)cp;
        else if (cp < 0x800) { o += (char)(0xC0 | (cp >> 6)); o += (char)(0x80 | (cp & 0x3F)); }
        else if (cp < 0x10000) { o += (char)(0xE0 | (cp >> 12)); o += (char)(0x80 | ((cp >> 6) & 0x3F)); o += (char)(0x80 | (cp & 0x3F)); }
        else { o += (char)(0xF0 | (cp >> 18)); o += (char)(0x80 | ((cp >> 12) & 0x3F)); o += (char)(0x80 | ((cp >> 6) & 0x3F)); o += (char)(0x80 | (cp & 0x3F)); }
    }
    return o;
}

long Find(const Str* s, const Str* f, CFRange r, CFOptionFlags o)
{
    long n = (long)f->c.size();
    if (n == 0 || r.length < n) return -1;
    if (o & kCFCompareBackwards) {
        for (long i = r.location + r.length - n; i >= r.location; --i) {
            bool ok = true;
            for (long k = 0; ok && k < n; ++k) ok = Eq(s->c[i + k], f->c[k], o);
            if (ok) return i;
            if (o & kCFCompareAnchored) break;
        }
    } else {
        for (long i = r.location; i + n <= r.location + r.length; ++i) {
            bool ok = true;
            for (long k = 0; ok && k < n; ++k) ok = Eq(s->c[i + k], f->c[k], o);
            if (ok) return i;
            if (o & kCFCompareAnchored) break;
        }
    }
    return -1;
}
} // namespace

extern "C" {

void CFRelease(CFTypeRef cf)
{
    if (!cf) { fprintf(stderr, "CFRelease(NULL)\n"); abort(); }
    Obj* o = (Obj*)cf;
    if (o->constant) return;
    if (--o->refs == 0) {
        if (o->kind == kArray) {
            Arr* a = (Arr*)o;
            (void)a;
        }
        delete o;
    }
}

CFTypeRef CFRetain(CFTypeRef cf) { Obj* o = (Obj*)cf; if (!o->constant) ++o->refs; return cf; }

CFStringRef __CFStringMakeConstantString(const char* s)
{
    static std::map<std::string, Str*>* table = new std::map<std::string, Str*>();
    Str*& e = (*table)[s];
    if (!e) { e = FromUTF8(s); e->constant = true; }
    return (CFStringRef)e;
}

CFStringRef CFStringCreateCopy(CFAllocatorRef, CFStringRef s) { const Str* x = S(s); return (CFStringRef)NewStr(x->c.data(), x->c.size()); }
CFMutableStringRef CFStringCreateMutableCopy(CFAllocatorRef, CFIndex, CFStringRef s) { return (CFMutableStringRef)CFStringCreateCopy(0, s); }
CFMutableStringRef CFStringCreateMutable(CFAllocatorRef, CFIndex) { return (CFMutableStringRef)NewStr(); }

CFStringRef CFStringCreateWithCString(CFAllocatorRef, const char* s, CFStringEncoding e)
{
    if (!s) return 0;
    if (e == kCFStringEncodingASCII)
        for (const unsigned char* p = (const unsigned char*)s; *p; ++p)
            if (*p > 0x7F) return 0;                 // CF fails on non-ASCII input
    return (CFStringRef)FromUTF8(s);
}

CFStringRef CFStringCreateWithCharacters(CFAllocatorRef, const UniChar* c, CFIndex n) { return (CFStringRef)NewStr(c, n); }

// UTF-8 / ASCII / UTF-16BE / UTF-16LE only; isExternalRepresentation is ignored (no BOM handling).
CFStringRef CFStringCreateWithBytes(CFAllocatorRef, const unsigned char* b, CFIndex n, CFStringEncoding e, Boolean)
{
    if (!b && n) return 0;
    if (e == kCFStringEncodingUTF16BE || e == kCFStringEncodingUTF16LE) {
        if (n & 1) return 0;
        Str* r = NewStr();
        for (CFIndex i = 0; i + 1 < n; i += 2)
            r->c.push_back(e == kCFStringEncodingUTF16BE ? (UniChar)((b[i] << 8) | b[i + 1])
                                                         : (UniChar)(b[i] | (b[i + 1] << 8)));
        return (CFStringRef)r;
    }
    if (e == kCFStringEncodingASCII)
        for (CFIndex i = 0; i < n; ++i)
            if (b[i] > 0x7F) return 0;
    return (CFStringRef)FromUTF8N((const char*)b, (size_t)n);
}

CFStringRef CFStringCreateWithSubstring(CFAllocatorRef, CFStringRef s, CFRange r)
{
    const Str* x = S(s);
    if (r.location < 0 || r.location + r.length > (CFIndex)x->c.size()) { fprintf(stderr, "CFStringCreateWithSubstring: range out of bounds\n"); abort(); }
    return (CFStringRef)NewStr(x->c.data() + r.location, r.length);
}

CFStringRef CFStringCreateWithFormat(CFAllocatorRef, const void*, CFStringRef format, ...)
{
    std::string f = ToUTF8(S(format));
    // CF's %qu / %hu are printf-compatible except %q -> %ll
    std::string pf;
    for (size_t i = 0; i < f.size(); ++i) {
        if (f[i] == 'q' && i > 0 && pf.size() && (isdigit((unsigned char)pf.back()) || pf.back() == '%' || pf.back() == '.')) { pf += "ll"; continue; }
        pf += f[i];
    }
    char buf[512];
    va_list ap;
    va_start(ap, format);
    vsnprintf(buf, sizeof buf, pf.c_str(), ap);
    va_end(ap);
    return (CFStringRef)FromUTF8(buf);
}

CFIndex CFStringGetLength(CFStringRef s) { return (CFIndex)S(s)->c.size(); }
UniChar CFStringGetCharacterAtIndex(CFStringRef s, CFIndex i) { return S(s)->c.at(i); }
void CFStringGetCharacters(CFStringRef s, CFRange r, UniChar* b) { memcpy(b, S(s)->c.data() + r.location, r.length * sizeof(UniChar)); }

Boolean CFStringGetCString(CFStringRef s, char* buf, CFIndex size, CFStringEncoding e)
{
    const Str* x = S(s);
    std::string o;
    if (e == kCFStringEncodingASCII) {
        for (size_t i = 0; i < x->c.size(); ++i) { if (x->c[i] > 0x7F) return 0; o += (char)x->c[i]; }
    } else if (e == kCFStringEncodingUTF16BE || e == kCFStringEncodingUTF16LE) {
        // code units in the requested byte order, then a single NUL byte (as CF does)
        for (size_t i = 0; i < x->c.size(); ++i) {
            char hi = (char)(x->c[i] >> 8), lo = (char)(x->c[i] & 0xFF);
            if (e == kCFStringEncodingUTF16BE) { o += hi; o += lo; } else { o += lo; o += hi; }
        }
    } else {
        o = ToUTF8(x);
    }
    if ((CFIndex)o.size() + 1 > size) return 0;
    memcpy(buf, o.c_str(), o.size() + 1);
    return 1;
}

const char* CFStringGetCStringPtr(CFStringRef, CFStringEncoding) { return 0; }  // allowed by the CF contract

void CFStringAppend(CFMutableStringRef s, CFStringRef a) { const Str* y = S(a); std::vector<UniChar> t(y->c); S(s)->c.insert(S(s)->c.end(), t.begin(), t.end()); }
void CFStringAppendCharacters(CFMutableStringRef s, const UniChar* c, CFIndex n) { S(s)->c.insert(S(s)->c.end(), c, c + n); }
void CFStringInsert(CFMutableStringRef s, CFIndex i, CFStringRef a) { std::vector<UniChar> t(S(a)->c); S(s)->c.insert(S(s)->c.begin() + i, t.begin(), t.end()); }
void CFStringDelete(CFMutableStringRef s, CFRange r) { S(s)->c.erase(S(s)->c.begin() + r.location, S(s)->c.begin() + r.location + r.length); }

void CFStringReplace(CFMutableStringRef s, CFRange r, CFStringRef a)
{
    std::vector<UniChar> t(S(a)->c);
    std::vector<UniChar>& c = S(s)->c;
    c.erase(c.begin() + r.location, c.begin() + r.location + r.length);
    c.insert(c.begin() + r.location, t.begin(), t.end());
}

CFIndex CFStringFindAndReplace(CFMutableStringRef s, CFStringRef find, CFStringRef repl, CFRange r, CFOptionFlags o)
{
    Str* x = S(s);
    std::vector<UniChar> f(S(find)->c), t(S(repl)->c);
    if (f.empty()) return 0;
    std::vector<UniChar> out(x->c.begin(), x->c.begin() + r.location);
    CFIndex n = 0, i = r.location, end = r.location + r.length;
    while (i < end) {
        bool ok = i + (CFIndex)f.size() <= end;
        for (size_t k = 0; ok && k < f.size(); ++k) ok = Eq(x->c[i + k], f[k], o);
        if (ok) { out.insert(out.end(), t.begin(), t.end()); i += f.size(); ++n; }
        else out.push_back(x->c[i++]);
    }
    out.insert(out.end(), x->c.begin() + end, x->c.end());
    x->c.swap(out);
    return n;
}

Boolean CFStringFindWithOptions(CFStringRef s, CFStringRef f, CFRange r, CFOptionFlags o, CFRange* res)
{
    long i = Find(S(s), S(f), r, o);
    if (i < 0) return 0;
    if (res) *res = CFRangeMake(i, S(f)->c.size());
    return 1;
}

CFComparisonResult CFStringCompare(CFStringRef a, CFStringRef b, CFOptionFlags o)
{
    return (CFComparisonResult)Cmp(S(a)->c.data(), S(a)->c.size(), S(b)->c.data(), S(b)->c.size(), o);
}

CFComparisonResult CFStringCompareWithOptions(CFStringRef a, CFStringRef b, CFRange r, CFOptionFlags o)
{
    // HOST-ONLY: the i386 program passes UInt32 starts (EqualsNoCase(len - 4, 4, ".lif") on a
    // 2-character name gives location -2: an out-of-range read that compares unequal). Emulate the
    // 32-bit CFIndex and answer "not equal" for any range outside the string.
    long loc = (long)(int)r.location, len = (long)(int)r.length;
    if (loc < 0 || len < 0 || loc + len > (long)S(a)->c.size())
        return kCFCompareLessThan;
    return (CFComparisonResult)Cmp(S(a)->c.data() + loc, len, S(b)->c.data(), S(b)->c.size(), o);
}

Boolean CFStringHasPrefix(CFStringRef s, CFStringRef p)
{
    const Str *x = S(s), *y = S(p);
    return y->c.size() <= x->c.size() && std::equal(y->c.begin(), y->c.end(), x->c.begin());
}

Boolean CFStringHasSuffix(CFStringRef s, CFStringRef p)
{
    const Str *x = S(s), *y = S(p);
    return y->c.size() <= x->c.size() && std::equal(y->c.begin(), y->c.end(), x->c.end() - y->c.size());
}

// Unicode general category Zs (Unicode 9/10, as in macOS 10.13 CF)
static bool IsZs(UniChar c)
{
    return c == 0x20 || c == 0xA0 || c == 0x1680 || (c >= 0x2000 && c <= 0x200A) || c == 0x202F ||
           c == 0x205F || c == 0x3000;
}
// kCFCharacterSetWhitespaceAndNewline: Z* + U+000A-U+000D + U+0085 (and tab)
static bool IsWS(UniChar c)
{
    return IsZs(c) || (c >= 0x09 && c <= 0x0D) || c == 0x85 || c == 0x2028 || c == 0x2029;
}

void CFStringTrimWhitespace(CFMutableStringRef s)
{
    std::vector<UniChar>& c = S(s)->c;
    size_t b = 0, e = c.size();
    while (b < e && IsWS(c[b])) ++b;
    while (e > b && IsWS(c[e - 1])) --e;
    c = std::vector<UniChar>(c.begin() + b, c.begin() + e);
}

void CFStringLowercase(CFMutableStringRef s, CFLocaleRef) { for (size_t i = 0; i < S(s)->c.size(); ++i) S(s)->c[i] = Fold(S(s)->c[i]); }

CFArrayRef CFStringCreateArrayBySeparatingStrings(CFAllocatorRef, CFStringRef s, CFStringRef sep)
{
    Arr* a = new Arr();
    const Str *x = S(s), *p = S(sep);
    size_t start = 0;
    for (size_t i = 0; p->c.size() && i + p->c.size() <= x->c.size();) {
        if (std::equal(p->c.begin(), p->c.end(), x->c.begin() + i)) {
            a->v.push_back(NewStr(x->c.data() + start, i - start));
            i += p->c.size();
            start = i;
        } else {
            ++i;
        }
    }
    a->v.push_back(NewStr(x->c.data() + start, x->c.size() - start));
    return (CFArrayRef)a;  // NOTE: elements leak in the shim (CF would release them with the array)
}

CFStringRef CFStringCreateByCombiningStrings(CFAllocatorRef, CFArrayRef arr, CFStringRef sep)
{
    Str* r = NewStr();
    const Arr* a = (const Arr*)(const Obj*)arr;
    for (size_t i = 0; i < a->v.size(); ++i) {
        if (i) r->c.insert(r->c.end(), S(sep)->c.begin(), S(sep)->c.end());
        r->c.insert(r->c.end(), S(a->v[i])->c.begin(), S(a->v[i])->c.end());
    }
    return (CFStringRef)r;
}

CFMutableArrayRef CFArrayCreateMutable(CFAllocatorRef, CFIndex, const void*) { return (CFMutableArrayRef) new Arr(); }
void CFArrayAppendValue(CFMutableArrayRef a, const void* v) { ((Arr*)(Obj*)a)->v.push_back(v); }
CFIndex CFArrayGetCount(CFArrayRef a) { return (CFIndex)((const Arr*)(const Obj*)a)->v.size(); }
const void* CFArrayGetValueAtIndex(CFArrayRef a, CFIndex i) { return ((const Arr*)(const Obj*)a)->v.at(i); }

CFCharacterSetRef CFCharacterSetGetPredefined(CFCharacterSetPredefinedSet set)
{
    static CSet* sets[8];
    if (!sets[set]) sets[set] = new CSet(set);
    return (CFCharacterSetRef)sets[set];
}

Boolean CFCharacterSetIsCharacterMember(CFCharacterSetRef cs, UniChar c)
{
    switch (((const CSet*)(const Obj*)cs)->which) {
    case kCFCharacterSetWhitespace: return c == '\t' || IsZs(c);           // Zs + tab
    case kCFCharacterSetWhitespaceAndNewline: return IsWS(c);
    case kCFCharacterSetDecimalDigit: return c >= '0' && c <= '9';
    case kCFCharacterSetControl: return c < 0x20 || c == 0x7F;
    }
    return 0;
}

CFLocaleRef CFLocaleGetSystem(void) { static Misc* l = 0; if (!l) { l = new Misc(kLocale); l->constant = true; } return (CFLocaleRef)l; }

CFNumberFormatterRef CFNumberFormatterCreate(CFAllocatorRef, CFLocaleRef, CFNumberFormatterStyle) { return (CFNumberFormatterRef) new Misc(kFormatter); }

CFStringRef CFNumberFormatterCreateStringWithValue(CFAllocatorRef, CFNumberFormatterRef, CFNumberType t, const void* v)
{
    char buf[64];
    switch (t) {
    case kCFNumberSInt32Type: case kCFNumberIntType: snprintf(buf, sizeof buf, "%d", *(const int32_t*)v); break;
    case kCFNumberSInt64Type: case kCFNumberLongLongType: snprintf(buf, sizeof buf, "%lld", (long long)*(const int64_t*)v); break;
    case kCFNumberFloat32Type: snprintf(buf, sizeof buf, "%g", *(const float*)v); break;
    case kCFNumberFloat64Type: snprintf(buf, sizeof buf, "%g", *(const double*)v); break;
    default: return 0;
    }
    return (CFStringRef)FromUTF8(buf);
}

Boolean CFNumberFormatterGetValueFromString(CFNumberFormatterRef, CFStringRef s, CFRange* r, CFNumberType t, void* v)
{
    CFRange rr = r ? *r : CFRangeMake(0, CFStringGetLength(s));
    std::string txt;
    for (CFIndex i = 0; i < rr.length; ++i) txt += (char)S(s)->c[rr.location + i];
    const char* b = txt.c_str();
    char* e = 0;
    switch (t) {
    case kCFNumberFloat32Type: { float f = strtof(b, &e); if (e == b) return 0; *(float*)v = f; break; }
    case kCFNumberFloat64Type: { double d = strtod(b, &e); if (e == b) return 0; *(double*)v = d; break; }
    default: { long l = strtol(b, &e, 10); if (e == b) return 0; *(int*)v = (int)l; break; }
    }
    if (r) r->length = (CFIndex)(e - b);
    return 1;
}

CFUUIDRef CFUUIDCreate(CFAllocatorRef)
{
    UUIDObj* u = new UUIDObj();
    FILE* f = fopen("/dev/urandom", "rb");
    if (!f || fread(u->b, 1, 16, f) != 16) {
        for (int i = 0; i < 16; ++i) u->b[i] = (unsigned char)(rand() & 0xff);
    }
    if (f) fclose(f);
    u->b[6] = (unsigned char)((u->b[6] & 0x0f) | 0x40);   // version 4
    u->b[8] = (unsigned char)((u->b[8] & 0x3f) | 0x80);   // variant 10
    return (CFUUIDRef)(Obj*)u;
}

CFStringRef CFUUIDCreateString(CFAllocatorRef, CFUUIDRef uuid)
{
    const unsigned char* b = ((const UUIDObj*)(const Obj*)uuid)->b;
    char buf[40];
    snprintf(buf, sizeof buf, "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
             b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8], b[9], b[10], b[11], b[12], b[13], b[14], b[15]);
    return (CFStringRef)FromUTF8(buf);
}

// Hex digits are read in order, dashes skipped; anything else ends the parse (the rest of the
// bytes stays zero, so a malformed string gives a partly or completely nil UUID).
CFUUIDRef CFUUIDCreateFromString(CFAllocatorRef, CFStringRef s)
{
    if (!s) return 0;
    UUIDObj* u = new UUIDObj();
    const Str* str = S(s);
    int nibble = 0;
    for (size_t i = 0; i < str->c.size() && nibble < 32; ++i) {
        UniChar c = str->c[i];
        int v;
        if (c >= '0' && c <= '9') v = c - '0';
        else if (c >= 'a' && c <= 'f') v = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') v = c - 'A' + 10;
        else if (c == '-') continue;
        else break;
        u->b[nibble / 2] = (unsigned char)(u->b[nibble / 2] | (nibble % 2 ? v : v << 4));
        ++nibble;
    }
    return (CFUUIDRef)(Obj*)u;
}

CFUUIDBytes CFUUIDGetUUIDBytes(CFUUIDRef uuid)
{
    CFUUIDBytes r;
    memcpy(&r, ((const UUIDObj*)(const Obj*)uuid)->b, 16);
    return r;
}

} // extern "C"
