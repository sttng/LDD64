// compat shim (NOT restored code): the subset of CoreFoundation that LDD's Mac sources use,
// implemented portably so the Mac branch can be built and tested on Linux.
#pragma once
#include <stddef.h>
#include <stdint.h>
#if !defined(COMPAT_MACTYPES_H)   // (MacTypes.h defines them in test builds)
typedef unsigned char Boolean;
typedef uint16_t      UniChar;
typedef int32_t       SInt32;
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef long          CFIndex;
typedef unsigned long CFOptionFlags;
typedef uint32_t      CFStringEncoding;
typedef const void*   CFTypeRef;
typedef const void*   CFAllocatorRef;
typedef const struct __CFString*         CFStringRef;
typedef struct __CFString*               CFMutableStringRef;
typedef const struct __CFArray*          CFArrayRef;
typedef struct __CFArray*                CFMutableArrayRef;
typedef const struct __CFCharacterSet*   CFCharacterSetRef;
typedef const struct __CFLocale*         CFLocaleRef;
typedef struct __CFNumberFormatter*      CFNumberFormatterRef;
typedef const struct __CFDictionary*     CFDictionaryRef;   // opaque only (Window.h)
typedef const void*                      CFArrayCallBacks;
typedef const struct __CFUUID*           CFUUIDRef;
typedef struct {
    uint8_t byte0, byte1, byte2, byte3, byte4, byte5, byte6, byte7,
            byte8, byte9, byte10, byte11, byte12, byte13, byte14, byte15;
} CFUUIDBytes;

typedef struct { CFIndex location; CFIndex length; } CFRange;
static inline CFRange CFRangeMake(CFIndex loc, CFIndex len) { CFRange r; r.location = loc; r.length = len; return r; }

typedef enum { kCFCompareLessThan = -1, kCFCompareEqualTo = 0, kCFCompareGreaterThan = 1 } CFComparisonResult;
enum {
    kCFCompareCaseInsensitive = 1, kCFCompareBackwards = 4, kCFCompareAnchored = 8,
    kCFCompareNonliteral = 16, kCFCompareNumerically = 64
};
enum {
    kCFStringEncodingMacRoman = 0, kCFStringEncodingASCII = 0x0600,
    kCFStringEncodingUTF8 = 0x08000100, kCFStringEncodingUTF16BE = 0x10000100,
    kCFStringEncodingUTF16LE = 0x14000100
};
typedef enum {
    kCFCharacterSetControl = 1, kCFCharacterSetWhitespace = 2, kCFCharacterSetWhitespaceAndNewline = 3,
    kCFCharacterSetDecimalDigit = 4
} CFCharacterSetPredefinedSet;
typedef enum {
    kCFNumberSInt8Type = 1, kCFNumberSInt16Type = 2, kCFNumberSInt32Type = 3, kCFNumberSInt64Type = 4,
    kCFNumberFloat32Type = 5, kCFNumberFloat64Type = 6, kCFNumberCharType = 7, kCFNumberShortType = 8,
    kCFNumberIntType = 9, kCFNumberLongType = 10, kCFNumberLongLongType = 11, kCFNumberFloatType = 12,
    kCFNumberDoubleType = 13
} CFNumberType;
typedef CFNumberType CFNumberType_t;
enum { kCFNumberFormatterNoStyle = 0 };
typedef int CFNumberFormatterStyle;

#define kCFAllocatorDefault ((CFAllocatorRef)0)

void        CFRelease(CFTypeRef cf);
CFTypeRef   CFRetain(CFTypeRef cf);

CFStringRef __CFStringMakeConstantString(const char* s);
#define CFSTR(s) __CFStringMakeConstantString("" s "")

CFStringRef        CFStringCreateCopy(CFAllocatorRef, CFStringRef s);
CFMutableStringRef CFStringCreateMutableCopy(CFAllocatorRef, CFIndex maxLength, CFStringRef s);
CFMutableStringRef CFStringCreateMutable(CFAllocatorRef, CFIndex maxLength);
CFStringRef        CFStringCreateWithCString(CFAllocatorRef, const char* s, CFStringEncoding e);
CFStringRef        CFStringCreateWithCharacters(CFAllocatorRef, const UniChar* chars, CFIndex n);
CFStringRef        CFStringCreateWithBytes(CFAllocatorRef, const unsigned char* bytes, CFIndex numBytes, CFStringEncoding e, Boolean isExternalRepresentation);
CFStringRef        CFStringCreateWithSubstring(CFAllocatorRef, CFStringRef s, CFRange r);
CFStringRef        CFStringCreateWithFormat(CFAllocatorRef, const void* formatOptions, CFStringRef format, ...);
CFIndex            CFStringGetLength(CFStringRef s);
UniChar            CFStringGetCharacterAtIndex(CFStringRef s, CFIndex i);
void               CFStringGetCharacters(CFStringRef s, CFRange r, UniChar* buffer);
Boolean            CFStringGetCString(CFStringRef s, char* buffer, CFIndex size, CFStringEncoding e);
const char*        CFStringGetCStringPtr(CFStringRef s, CFStringEncoding e);
void               CFStringAppend(CFMutableStringRef s, CFStringRef a);
void               CFStringAppendCharacters(CFMutableStringRef s, const UniChar* c, CFIndex n);
void               CFStringInsert(CFMutableStringRef s, CFIndex i, CFStringRef a);
void               CFStringDelete(CFMutableStringRef s, CFRange r);
void               CFStringReplace(CFMutableStringRef s, CFRange r, CFStringRef a);
CFIndex            CFStringFindAndReplace(CFMutableStringRef s, CFStringRef find, CFStringRef repl, CFRange r, CFOptionFlags o);
Boolean            CFStringFindWithOptions(CFStringRef s, CFStringRef find, CFRange r, CFOptionFlags o, CFRange* result);
CFComparisonResult CFStringCompare(CFStringRef a, CFStringRef b, CFOptionFlags o);
CFComparisonResult CFStringCompareWithOptions(CFStringRef a, CFStringRef b, CFRange r, CFOptionFlags o);
Boolean            CFStringHasPrefix(CFStringRef s, CFStringRef p);
Boolean            CFStringHasSuffix(CFStringRef s, CFStringRef p);
void               CFStringTrimWhitespace(CFMutableStringRef s);
void               CFStringLowercase(CFMutableStringRef s, CFLocaleRef l);
CFArrayRef         CFStringCreateArrayBySeparatingStrings(CFAllocatorRef, CFStringRef s, CFStringRef sep);
CFStringRef        CFStringCreateByCombiningStrings(CFAllocatorRef, CFArrayRef a, CFStringRef sep);

CFMutableArrayRef  CFArrayCreateMutable(CFAllocatorRef, CFIndex capacity, const void* callbacks);
void               CFArrayAppendValue(CFMutableArrayRef a, const void* v);
CFIndex            CFArrayGetCount(CFArrayRef a);
const void*        CFArrayGetValueAtIndex(CFArrayRef a, CFIndex i);

CFCharacterSetRef  CFCharacterSetGetPredefined(CFCharacterSetPredefinedSet set);
Boolean            CFCharacterSetIsCharacterMember(CFCharacterSetRef set, UniChar c);
CFLocaleRef        CFLocaleGetSystem(void);

CFNumberFormatterRef CFNumberFormatterCreate(CFAllocatorRef, CFLocaleRef, CFNumberFormatterStyle);
CFStringRef          CFNumberFormatterCreateStringWithValue(CFAllocatorRef, CFNumberFormatterRef, CFNumberType, const void* value);
Boolean              CFNumberFormatterGetValueFromString(CFNumberFormatterRef, CFStringRef s, CFRange* r, CFNumberType, void* value);

// CFUUID (UUID.cpp): random version-4 UUIDs, upper-case strings like CF.
CFUUIDRef   CFUUIDCreate(CFAllocatorRef);
CFStringRef CFUUIDCreateString(CFAllocatorRef, CFUUIDRef uuid);
CFUUIDRef   CFUUIDCreateFromString(CFAllocatorRef, CFStringRef s);
CFUUIDBytes CFUUIDGetUUIDBytes(CFUUIDRef uuid);

#ifdef __cplusplus
}
#endif
