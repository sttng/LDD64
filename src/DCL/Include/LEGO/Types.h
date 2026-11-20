// LEGO Digital Designer — restored source
// Portable integer typedefs.
//
// Evidence: template instantiations name `unsigned int` in the Windows RTTI and
// `unsigned long` in the Mac RTTI for the same class (e.g. BasicArray<UInt32>), so the
// original code used a typedef that maps to different builtin types per platform.
#pragma once

namespace LEGO {

typedef signed char        Int8;
typedef unsigned char      UInt8;
typedef short              Int16;
typedef unsigned short     UInt16;
#if defined(_WIN32)
typedef int                Int32;
typedef unsigned int       UInt32;
#if defined(_MSC_VER)
typedef __int64            Int64;
typedef unsigned __int64   UInt64;
#else
typedef long long          Int64;           // MinGW/clang stand-in for the MSVC types
typedef unsigned long long UInt64;
#endif
#elif defined(__LP64__)
// not an original target: keep 32-bit semantics when building tests on 64-bit hosts
typedef int                Int32;
typedef unsigned int       UInt32;
typedef long long          Int64;
typedef unsigned long long UInt64;
#else
typedef long               Int32;
typedef unsigned long      UInt32;
typedef long long          Int64;
typedef unsigned long long UInt64;
#endif
typedef float              Real32;
typedef double             Real64;

} // namespace LEGO
