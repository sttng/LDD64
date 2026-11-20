// LEGO Digital Designer — restored source
// LEGO::String — UTF-16 string. Mac (String.mm, TU 273 0x288ca0-0x28decf): CFString wrapper,
// 16 bytes, exactly one of m_string/m_mutable non-null. Win (String.cpp): copy-on-write
// BasicString<UniChar16>, 4 bytes. Method names inferred; addresses are Mac unless noted.
// `char*` input is ASCII on Mac (kCFStringEncodingASCII = 0x600).
#pragma once
#include "LEGO/Types.h"
#include "BasicString.h"

#if !defined(_WIN32)
typedef const struct __CFString* CFStringRef;
typedef struct __CFString*       CFMutableStringRef;
#endif

namespace LEGO {

typedef UInt16 UniChar16;
template <class T> class BasicArray;

class String {
public:
    enum { npos = 0xFFFFFFFF };

    // --- construction ------------------------------------------------------
    String();                                                     // 0x288cd0
    String(const String& s);                                      // 0x288d20
    String(const String& s, UInt32 start, UInt32 count = npos);   // 0x288d80
    String(const UniChar16* chars, UInt32 length);                // 0x288e20
    explicit String(const UniChar16* zeroTerminated);             // 0x288e70
    explicit String(const AString& ascii);                        // 0x288ee0
    String(const char* ascii, UInt32 length);                     // 0x288f30
    String(const char* ascii);                                    // 0x289000
    String(UInt32 count, UniChar16 fill);                         // 0x289040
#if !defined(_WIN32)
    explicit String(CFStringRef cf);                              // 0x2891b0
    CFStringRef GetCFString() const;                              // 0x288d00
#endif
    ~String();                                                    // 0x289240

    // --- assignment ----------------------------------------------------------
    String& operator=(const String& s);                           // 0x289c40
    String& operator=(const char* ascii);                         // 0x28cfe0
    String& operator=(const AString& ascii);                      // 0x28cfa0
    String& Assign(const String& s, UInt32 start, UInt32 count);  // 0x289b10
    String& Assign(const char* ascii, UInt32 length);             // 0x28cea0

    // --- access --------------------------------------------------------------
    UInt32           GetLength() const;                           // 0x288d60
    bool             IsEmpty() const;                             // 0x289360
    UniChar16        GetAt(UInt32 index) const;                   // 0x289280
    UniChar16        operator[](UInt32 index) const { return GetAt(index); }
    const UniChar16* GetBuffer() const;                           // 0x2892b0 (zero terminated)
    String           Substring(UInt32 start, UInt32 count = npos) const; // 0x289390
    bool             IsCharAt(UInt32 index, UniChar16 c) const;   // 0x289980 (npos = last)
    bool             IsWhitespaceAt(UInt32 index) const;          // 0x289a10 (npos = last)
    static bool      IsWhitespace(UniChar16 c);                   // 0x289ab0
    static bool      IsDigit(UniChar16 c);                        // 0x289ae0
    bool             IsASCII() const;                             // 0x28d210
    AString          ToAString() const;                           // 0x28d2a0 (ASCII)
    AString          ToUTF8() const;                              // 0x28d870 -> 0x1a830
    String           ToXML() const;                               // 0x28d8a0 (& < > ' " escaped)
    String           ScanWord(UInt32* position) const;            // 0x2897e0 (NSScanner)

    // --- modification -------------------------------------------------------
    String& Append(const String& s);                              // 0x289cc0
    String& Append(const char* ascii);                            // 0x28d110
    String& Append(const AString& ascii);                         // 0x28d020
    String& Append(UInt32 count, UniChar16 c);                    // 0x289440
    String& Append(UniChar16 c);                                  // 0x289e80 (name inferred;
                                                                  //  UniChar16 == UInt16, so not <<)
    String& Insert(UInt32 pos, const String& s);                  // 0x28b310 (npos = append)
    String& Insert(UInt32 pos, UInt32 count, UniChar16 c);        // 0x28b410
    String& Replace(UInt32 start, UInt32 count, const String& s); // 0x28b5e0
    String& ReplaceAll(const String& from, const String& to);     // 0x28b8e0
    String& ReplaceAll(const String& from, UInt32 fromStart, UInt32 fromCount,
                       const String& to, UInt32 toStart, UInt32 toCount); // 0x28b720
    String& Erase(UInt32 start, UInt32 count = npos);             // 0x289640
    void    Clear();                                              // 0x289710
    void    Trim();                                               // 0x289770 (whitespace)
    void    TrimRight(const String& chars);                       // 0x28c500
    void    ToLower();                                            // 0x289900

    String& operator+=(const String& s)  { return Append(s); }
    String& operator+=(const char* s)    { return Append(s); }
    String& operator+=(const AString& s) { return Append(s); }    // 0x28d1f0

    // --- stream-style formatting ------------------------------------------
    String& operator<<(const String& s);                          // 0x289eb0
    String& operator<<(const char* ascii);                        // 0x289e60
    String& operator<<(bool b);                                   // 0x289f30 ("1"/"0")
    String& operator<<(UInt16 v);                                 // 0x28a020 "%hu"
    String& operator<<(Int32 v);                                  // 0x28a140 (number formatter)
    String& operator<<(UInt32 v);                                 // 0x289d40 "%u"
    String& operator<<(UInt64 v);                                 // 0x28a290 "%qu"
    String& operator<<(Real32 v);                                 // 0x28a3b0 "%.30G", |v|<1e-5 -> 0
    String& operator<<(Real64 v);                                 // 0x28a500 "%.60G", |v|<1e-8 -> 0

    // --- parsing (pos is advanced past the parsed text) --------------------
    bool Parse(bool& out, UInt32& pos) const;                     // 0x28a650 0/1/no/yes/false/true
    bool Parse(UInt8& out, UInt32& pos) const;                    // 0x28acd0
    bool Parse(UInt32& out, UInt32& pos) const;                   // 0x28b0a0
    bool Parse(UInt64& out, UInt32& pos) const;                   // 0x28ad20
    bool Parse(Real32& out, UInt32& pos) const;                   // 0x28b1b0
    bool Parse(Real64& out, UInt32& pos) const;                   // 0x28b260

    // --- searching (all return npos when not found) --------------------------
    UInt32 Find(const String& s, UInt32 start = 0) const;         // 0x28b990
    UInt32 Find(UniChar16 c, UInt32 start = 0) const;             // 0x28ba10
    UInt32 FindNoCase(const String& s, UInt32 start = 0) const;   // 0x28bab0
    UInt32 ReverseFind(const UniChar16* s, UInt32 start, UInt32 length) const; // 0x28bb30
    UInt32 ReverseFind(UniChar16 c, UInt32 start = npos) const;   // 0x28bbe0
    UInt32 FindFirstOf(const String& set, UInt32 start = 0) const;     // 0x28b0e0
    UInt32 FindLastOf(const String& set, UInt32 start = npos) const;   // 0x28bc10
    UInt32 FindLastOf(const UniChar16* set, UInt32 start, UInt32 n) const; // 0x28bcf0
    UInt32 FindLastOf(UniChar16 c, UInt32 start = npos) const;         // 0x28bdd0
    UInt32 FindFirstNotOf(const String& set, UInt32 start = 0) const;  // 0x28be00
    UInt32 FindFirstNotOf(const UniChar16* set, UInt32 start, UInt32 n) const; // 0x28bf00
    UInt32 FindFirstNotOf(const UniChar16* set, UInt32 start = 0) const;   // 0x28c000
    UInt32 FindLastNotOf(const String& set, UInt32 start = npos) const;    // 0x28c040
    UInt32 FindLastNotOf(const UniChar16* set, UInt32 start, UInt32 n) const; // 0x28c120
    UInt32 FindLastNotOf(const UniChar16* set, UInt32 start = npos) const;  // 0x28c200

    // --- comparison ---------------------------------------------------------
    bool  operator==(const String& s) const;                      // 0x28c280
    bool  operator!=(const String& s) const;                      // 0x28c240
    bool  operator<(const String& s) const;                       // 0x28c2c0
    bool  StartsWith(const String& s) const;                      // 0x28c300
    bool  StartsWith(const UniChar16* s, UInt32 n) const;         // 0x28c340
    bool  StartsWith(UniChar16 c) const;                          // 0x28c3f0
    bool  EndsWith(const UniChar16* s, UInt32 n) const;           // 0x28c420
    bool  EndsWith(UniChar16 c) const;                            // 0x28c4d0
    Int32 CompareNoCase(const String& s) const;                   // 0x28c5a0
    Int32 CompareNoCase(UInt32 start, UInt32 count, const String& s) const; // 0x28c5e0
    Int32 CompareNoCase(UInt32 start, UInt32 count, const String& s,
                        UInt32 sStart, UInt32 sCount) const;      // 0x28c680
    bool  EqualsNoCase(const String& s) const;                    // 0x28c7d0
    bool  EqualsNoCase(UInt32 start, UInt32 count, const String& s) const; // 0x28ac20
    bool  EqualsNoCase(UInt32 start, UInt32 count, const String& s,
                       UInt32 sStart, UInt32 sCount) const;       // 0x28c820
    bool  LessNoCase(const String& s) const;                      // 0x28c850

    // --- splitting / joining ---------------------------------------------------
    void Split(const String& separator, BasicArray<String>& out) const; // 0x28c950
    void Split(UniChar16 separator, BasicArray<String>& out) const;     // 0x28c890
    void SplitLines(BasicArray<String>& out) const;                     // 0x28d420
    void Join(const String& separator, const BasicArray<String>& parts); // 0x28cdb0
    void Join(UniChar16 separator, const BasicArray<String>& parts);     // 0x28ccf0

private:
#if defined(_WIN32)
    BasicString<UniChar16> m_str;                                 // +0x00
#else
    void MakeMutable();     // inlined everywhere in the Mac build
    void ReleaseAll();
    void AppendDecimal(UInt64 v);   // LDD64: operator<< of the unsigned types
    CFStringRef        m_string;      // +0x00 immutable representation
    CFMutableStringRef m_mutable;     // +0x04 mutable representation
    UniChar16*         m_charCache;   // +0x08 GetBuffer() cache
    UInt32             m_peakLength;  // +0x0c longest length seen by Erase() (purpose unclear)
#endif
};

// Case-insensitive ordering used by BasicMap<String, …, StringCaseInsensitiveLess<String>>
template <class S>
struct StringCaseInsensitiveLess {
    bool operator()(const S& a, const S& b) const { return a.LessNoCase(b); }
};

} // namespace LEGO
