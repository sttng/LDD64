// Vortex 5.0.0 (build 800, custom "vortexldd41") - basic types.
// Restored header. Names of types/functions not seen in a string or symbol
// are marked "(name inferred)".
//
// VxReal is double in this build: every Vortex math routine uses x87 qword / SSE2 sd
// operations (Mac 0x47fda4 VxTransform::orthonormalize, Win x87 fld qword), and the glue
// converts LEGO floats with cvtss2sd before each call.
#ifndef VX_VXPLATFORM_H
#define VX_VXPLATFORM_H

#include <stddef.h>

namespace Vx {

typedef double VxReal;
typedef VxReal VxReal3[3];
typedef VxReal VxReal4[4];
typedef VxReal VxReal4x4[4][4];

} // namespace Vx

// Compile-time check usable in C++03 headers and tests.
#define VX_STATIC_ASSERT_CAT2(a, b) a##b
#define VX_STATIC_ASSERT_CAT(a, b) VX_STATIC_ASSERT_CAT2(a, b)
#define VX_STATIC_ASSERT(cond) \
    typedef char VX_STATIC_ASSERT_CAT(vx_static_assert_, __LINE__)[(cond) ? 1 : -1]

// Layout notes used throughout the headers:
//  * "Mac +0xNN" = offset in the i386 Darwin build (GCC 4.x, libstdc++ COW std::string of 4
//    bytes, double aligned to 4 inside structs).
//  * "Win +0xNN" = offset in the MSVC 2008 build (std::string 28 bytes, double aligned to 8).
//  * tests/test_vortex_headers.cpp checks the declared members against those numbers.

#endif
