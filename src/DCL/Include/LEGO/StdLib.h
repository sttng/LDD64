// LEGO Digital Designer — restored source
// Thin C runtime wrappers (TU 044, Mac 0x10af70-0x10b75f). File and function names inferred,
// except StrLen/MemMove/MemSet/MemCopy which follow the callers' usage.
#pragma once
#include "LEGO/Types.h"
#include <stddef.h>

namespace LEGO {

UInt32 StrLen(const char* s);                                         // 0x10af70
void*  MemMove(void* dst, const void* src, size_t n);                 // 0x10af80
void*  MemSet(void* dst, int c, size_t n);                            // 0x10afb0
void*  MemCopy(void* dst, const void* src, size_t n);                 // 0x10afe0
const void* MemChr(const void* p, char c, size_t n);                  // 0x10b010
int    StrNCmp(const char* a, const char* b, size_t n);               // 0x10b030
int    StrNICmp(const char* a, const char* b, size_t n);              // 0x10b040
int    StrCmp(const char* a, const char* b);                          // 0x10b0a0
int    StrICmp(const char* a, const char* b);                         // 0x10b0b0
bool   StrILess(const char* a, const char* b);                        // 0x10b100
bool   IsSpace(char c);                                               // 0x10b150
UInt8  StrToUInt8(const char* s, char** end, int radix);              // 0x10b480
UInt32 StrToUInt32(const char* s, char** end, int radix);             // 0x10b4e0 (+ 0x10b540, 0x10b5a0)
Real32 StrToReal32(const char* s, char** end);                        // 0x10b600
void   UInt32ToStr(UInt32 value, char* buffer, UInt32 radix);         // 0x10b620
void   SkipSpaces(const char** p);                                    // 0x10b6a0

} // namespace LEGO
