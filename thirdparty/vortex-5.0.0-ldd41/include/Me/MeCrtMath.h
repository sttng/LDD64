/* Vortex 5.0.0 build 800 "vortexldd41" - transcendental shim for the
 * transcendentals. NOT original Vortex code: restoration infrastructure.
 *
 * The original sources call the C library's sin / cos / asin / acos / atan2 / fmod; LDD.exe uses
 * the MSVC 2008 SP1 CRT's x87 entry points (_CIsin ...). On every SSE2 machine (and in the
 * harness: __use_sse2_mathfcns Win 0xb46f1c = 1, default MXCSR, CW 0x27f):
 *   - _CIsin / _CIcos (Win 0x6d0c90 / 0x6d0df0) take the SSE2 table path (Win 0x6e15f0 /
 *     0x6e1b40) for 2^-252 <= |x| < ~92000 and the x87 fsin / fcos path (precision 53 bits) above;
 *   - _CIasin / _CIacos (Win 0x6d1050 / 0x6d11a0) take the SSE2 path (Win 0x6e1f50 / 0x6e24d0:
 *     ST0 -> xmm0 wrappers that call the SSE2 routines 0x6e1f6e / 0x6e24ee);
 *   - _CIatan2 (Win 0x6d13da) is x87 only: FPATAN on |y|, |x| at 64-bit precision, pi - r for
 *     x < 0, sign of y (core Win 0x6e2d3c); the result is left on the x87 stack with a 64-bit
 *     mantissa;
 *   - _CIfmod (Win 0x6d13fa) is FPREM: exact, like every C library's fmod.
 * On 10^6 arguments each (the restoration's diff tests) the restored build's C library
 * (zig / mingw -> wine msvcrt) differs from VS2008 in the last bit for sin ~3 %, cos ~3 %, asin
 * 0.5 %, acos 1.2 %, atan2 8 % of the arguments; fmod never. MeCrtSin / Cos / Asin / Acos / Atan2
 * reproduce VS2008 bit for bit (sin / cos transcribed, asin / acos generated into
 * MeCrtMathSse2.h, atan2 = the same x87 sequence).
 *
 * Include AFTER <math.h>. Calls are redirected only on Windows builds without the VS2008 CRT
 * (zig / mingw); MSVC and the Mac (Apple libm, as the original) use the C library.
 * ME_CRT_MATH_NATIVE disables the redirection. The CRT's errno / _matherr effects are not reproduced. */
#ifndef ME_MECRTMATH_H
#define ME_MECRTMATH_H

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

/* VS2008 CRT sin / cos (SSE2 path; x87 fallback for large |x|, infinities and NaNs) */
double MeCrtSin(double x);
double MeCrtCos(double x);

/* sin(x) * y / cos(x) * y as MSVC 2008 computes them when the result of _CIsin / _CIcos stays in
 * ST0 and is multiplied right away: on the x87 path (|x| >= ~92000) fsin / fcos deliver a 64-bit
 * mantissa (precision control does not apply to them) and the product is rounded once to 53 bits;
 * on the SSE2 path the result is a double and this is a plain product. */
double MeCrtSinMul(double x, double y);
double MeCrtCosMul(double x, double y);

/* VS2008 CRT asin / acos (the SSE2 routines; their results are doubles) and atan2 (rounded to
 * double, as when the caller stores the result) */
double MeCrtAsin(double x);
double MeCrtAcos(double x);
double MeCrtAtan2(double y, double x);

/* atan2 as an x87 value: where the original keeps _CIatan2's 64-bit-mantissa result in ST0 and
 * computes with it (precision control 53 bits: every operation rounds once to a double).
 * MeCrtLong holds the 80-bit value; off the shim it is a plain double. */
typedef struct MeCrtLong {
    unsigned char x87[16];
} MeCrtLong;
MeCrtLong MeCrtAtan2Long(double y, double x);
double MeCrtLongToDouble(MeCrtLong a);     /* store: round to double */
MeCrtLong MeCrtLongNeg(MeCrtLong a);       /* fchs (exact) */
int MeCrtLongGreater(MeCrtLong a, double b); /* a > b, exact compare */
double MeCrtLongAdd(MeCrtLong a, double b);  /* a + b, rounded once */
double MeCrtDoubleSubLong(double b, MeCrtLong a); /* b - a, rounded once */

#ifdef __cplusplus
}
#endif

#if defined(_WIN32) && !defined(_MSC_VER) && !defined(ME_CRT_MATH_NATIVE)
#define ME_CRT_MATH_SHIM 1
#undef sin
#undef cos
#undef asin
#undef acos
#undef atan2
#define sin(x) MeCrtSin(x)
#define cos(x) MeCrtCos(x)
#define asin(x) MeCrtAsin(x)
#define acos(x) MeCrtAcos(x)
#define atan2(y, x) MeCrtAtan2(y, x)
/* sin(x) * y where the original multiplies the unrounded result (see MeCrtSinMul) */
#define MeSinMul(x, y) MeCrtSinMul(x, y)
#define MeCosMul(x, y) MeCrtCosMul(x, y)
/* atan2 kept on the x87 stack (MeCrtLong) */
typedef MeCrtLong MeAtan2Long;
#define MeAtan2L(y, x) MeCrtAtan2Long(y, x)
#define MeLongToReal(a) MeCrtLongToDouble(a)
#define MeLongNeg(a) MeCrtLongNeg(a)
#define MeLongGreater(a, b) MeCrtLongGreater(a, b)
#define MeLongAdd(a, b) MeCrtLongAdd(a, b)
#define MeRealSubLong(b, a) MeCrtDoubleSubLong(b, a)
#else
#define MeSinMul(x, y) (sin(x) * (y))
#define MeCosMul(x, y) (cos(x) * (y))
typedef double MeAtan2Long;
#define MeAtan2L(y, x) atan2(y, x)
#define MeLongToReal(a) (a)
#define MeLongNeg(a) (-(a))
#define MeLongGreater(a, b) ((a) > (b))
#define MeLongAdd(a, b) ((a) + (b))
#define MeRealSubLong(b, a) ((b) - (a))
#endif

#endif
