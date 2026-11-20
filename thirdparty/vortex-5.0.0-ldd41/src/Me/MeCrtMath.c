/* Vortex 5.0.0 build 800 "vortexldd41" - VS2008 CRT sin / cos / asin / acos / atan2 reproduced
 * bit for bit (see Me/MeCrtMath.h). NOT original Vortex code: restoration infrastructure
 * Build with SSE2 doubles and -ffp-contract=off (the Vortex
 * flags): every operation is one IEEE double operation in the original's instruction order.
 *
 * sin / cos: the MSVC 2008 SP1 SSE2 routines, sin Win 0x6e15f0 (core 0x6e160e), cos Win 0x6e1b40
 * (core 0x6e1b5e); one algorithm, identical data (sin: table 0x9ea310 + constants
 * 0x9eab10..0x9eab98; cos: a copy at 0x9eabe0 / 0x9eb3e0..0x9eb478): k = rint(x * 32 / pi),
 * r = x - k pi/32 in three parts, entry j = (k + phase) mod 64 = {cos(j pi/32) - c, sin hi,
 * sin lo, c} (c = 1, 0 or -1), two degree-8 polynomials (SSE2 lanes written as lo / hi). Ranges:
 *   |x| < 2^-252          sin: x (x * (1 - 2^-53) for zero / denormals), cos: 1 - |x|
 *   |x| >= 0x40f5 << 48  (~92000), inf, NaN: the x87 path (_CIsin / _CIcos core Win 0x6d0ce8 /
 *                        0x6d0e48): fsin / fcos at precision 53 (CW 0x27f), fprem1 by pi * 2^62
 *                        beyond the instruction's range; infinities give the indefinite NaN.
 *                        Off x86 the C library is used.
 * MeCrtSinMul / MeCrtCosMul: the product with the unrounded x87 result.
 *
 * asin / acos: the SSE2 routines Win 0x6e1f6e / 0x6e24ee (via _CIasin / _CIacos 0x6d1050 /
 * 0x6d11a0 when __use_sse2_mathfcns is set and MXCSR / CW are the defaults: always in LDD),
 * generated into MeCrtMathSse2.h.
 *
 * atan2: _CIatan2 0x6d13da -> x87 dispatcher 0x6e18c0 / 0x6df7a7 (CW 0x133f: precision 64) ->
 * core 0x6e2d3c: FPATAN(|y|, |x|), FLDPI - r for x < 0, FCHS for y < 0 (signs from FXAM: -0 is
 * negative). Zero / infinite / NaN arguments take other entries (0x6e2d95 ...) that give FPATAN's
 * values (NaN: FPATAN's NaN), except two infinities: the indefinite NaN (diff_vortex_crt_math).
 * The result stays in ST0 (64-bit mantissa): MeCrtAtan2 rounds it, MeCrtAtan2Long keeps it.
 * NOT reproduced: results below DBL_MIN (|y / x| < 2^-1022) go through the CRT's underflow
 * handler (Win 0x6e1a15 -> _87except), which can return 1 ulp less (denormal, or 0); errno. */
#define ME_CRT_MATH_NATIVE 1
#include "Me/MeCrtMath.h"

#include <string.h>

typedef unsigned long long MeCrtU64;

static double MeCrtD(MeCrtU64 u)
{
    double d;
    memcpy(&d, &u, 8);
    return d;
}

/* Win 0x9ea310 (sin) = 0x9eabe0 (cos): 64 x {T0, T1, T2, T3} */
static const MeCrtU64 kTable[64][4] = {
    {0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0x3ff0000000000000ULL}, /* 0 */
    {0xbf73b92e176d6d31ULL, 0x3fb917a6bc29b42cULL, 0xbc3e2718e0000000ULL, 0x3ff0000000000000ULL}, /* 1 */
    {0xbf93ad06011469fbULL, 0x3fc8f8b83c69a60bULL, 0xbc626d19c0000000ULL, 0x3ff0000000000000ULL}, /* 2 */
    {0xbfa60bea939d225aULL, 0x3fd294062ed59f06ULL, 0xbc75d28da0000000ULL, 0x3ff0000000000000ULL}, /* 3 */
    {0xbfb37ca1866b95cfULL, 0x3fd87de2a6aea963ULL, 0xbc672cede0000000ULL, 0x3ff0000000000000ULL}, /* 4 */
    {0xbfbe3a6873fa1279ULL, 0x3fde2b5d3806f63bULL, 0x3c5e0d8920000000ULL, 0x3ff0000000000000ULL}, /* 5 */
    {0xbfc592675bc57974ULL, 0x3fe1c73b39ae68c8ULL, 0x3c8b25dd20000000ULL, 0x3ff0000000000000ULL}, /* 6 */
    {0xbfcd0dfe53aba2fdULL, 0x3fe44cf325091dd6ULL, 0x3c68076a20000000ULL, 0x3ff0000000000000ULL}, /* 7 */
    {0x3fca827999fcef32ULL, 0x3fe6a09e667f3bcdULL, 0xbc8bdd3420000000ULL, 0x3fe0000000000000ULL}, /* 8 */
    {0x3fc133cc94247758ULL, 0x3fe8bc806b151741ULL, 0xbc82c5e120000000ULL, 0x3fe0000000000000ULL}, /* 9 */
    {0x3fac73b39ae68c87ULL, 0x3fea9b66290ea1a3ULL, 0x3c39f630e0000000ULL, 0x3fe0000000000000ULL}, /* 10 */
    {0xbf9d4a2c7f909c4eULL, 0x3fec38b2f180bdb1ULL, 0xbc76e0b180000000ULL, 0x3fe0000000000000ULL}, /* 11 */
    {0xbfbe087565455a75ULL, 0x3fed906bcf328d46ULL, 0x3c7457e620000000ULL, 0x3fe0000000000000ULL}, /* 12 */
    {0x3fa4a03176acf82dULL, 0x3fee9f4156c62ddaULL, 0x3c8760b1e0000000ULL, 0x3fd0000000000000ULL}, /* 13 */
    {0xbfac1d1f0e5967d5ULL, 0x3fef6297cff75cb0ULL, 0x3c75621720000000ULL, 0x3fd0000000000000ULL}, /* 14 */
    {0xbf9ba1650f592f50ULL, 0x3fefd88da3d12526ULL, 0xbc887df640000000ULL, 0x3fc0000000000000ULL}, /* 15 */
    {0x0000000000000000ULL, 0x3ff0000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL}, /* 16 */
    {0x3f9ba1650f592f50ULL, 0x3fefd88da3d12526ULL, 0xbc887df640000000ULL, 0xbfc0000000000000ULL}, /* 17 */
    {0x3fac1d1f0e5967d5ULL, 0x3fef6297cff75cb0ULL, 0x3c75621720000000ULL, 0xbfd0000000000000ULL}, /* 18 */
    {0xbfa4a03176acf82dULL, 0x3fee9f4156c62ddaULL, 0x3c8760b1e0000000ULL, 0xbfd0000000000000ULL}, /* 19 */
    {0x3fbe087565455a75ULL, 0x3fed906bcf328d46ULL, 0x3c7457e620000000ULL, 0xbfe0000000000000ULL}, /* 20 */
    {0x3f9d4a2c7f909c4eULL, 0x3fec38b2f180bdb1ULL, 0xbc76e0b180000000ULL, 0xbfe0000000000000ULL}, /* 21 */
    {0xbfac73b39ae68c87ULL, 0x3fea9b66290ea1a3ULL, 0x3c39f630e0000000ULL, 0xbfe0000000000000ULL}, /* 22 */
    {0xbfc133cc94247758ULL, 0x3fe8bc806b151741ULL, 0xbc82c5e120000000ULL, 0xbfe0000000000000ULL}, /* 23 */
    {0xbfca827999fcef32ULL, 0x3fe6a09e667f3bcdULL, 0xbc8bdd3420000000ULL, 0xbfe0000000000000ULL}, /* 24 */
    {0x3fcd0dfe53aba2fdULL, 0x3fe44cf325091dd6ULL, 0x3c68076a20000000ULL, 0xbff0000000000000ULL}, /* 25 */
    {0x3fc592675bc57974ULL, 0x3fe1c73b39ae68c8ULL, 0x3c8b25dd20000000ULL, 0xbff0000000000000ULL}, /* 26 */
    {0x3fbe3a6873fa1279ULL, 0x3fde2b5d3806f63bULL, 0x3c5e0d8920000000ULL, 0xbff0000000000000ULL}, /* 27 */
    {0x3fb37ca1866b95cfULL, 0x3fd87de2a6aea963ULL, 0xbc672cede0000000ULL, 0xbff0000000000000ULL}, /* 28 */
    {0x3fa60bea939d225aULL, 0x3fd294062ed59f06ULL, 0xbc75d28da0000000ULL, 0xbff0000000000000ULL}, /* 29 */
    {0x3f93ad06011469fbULL, 0x3fc8f8b83c69a60bULL, 0xbc626d19c0000000ULL, 0xbff0000000000000ULL}, /* 30 */
    {0x3f73b92e176d6d31ULL, 0x3fb917a6bc29b42cULL, 0xbc3e2718e0000000ULL, 0xbff0000000000000ULL}, /* 31 */
    {0x0000000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL, 0xbff0000000000000ULL}, /* 32 */
    {0x3f73b92e176d6d31ULL, 0xbfb917a6bc29b42cULL, 0x3c3e2718e0000000ULL, 0xbff0000000000000ULL}, /* 33 */
    {0x3f93ad06011469fbULL, 0xbfc8f8b83c69a60bULL, 0x3c626d19c0000000ULL, 0xbff0000000000000ULL}, /* 34 */
    {0x3fa60bea939d225aULL, 0xbfd294062ed59f06ULL, 0x3c75d28da0000000ULL, 0xbff0000000000000ULL}, /* 35 */
    {0x3fb37ca1866b95cfULL, 0xbfd87de2a6aea963ULL, 0x3c672cede0000000ULL, 0xbff0000000000000ULL}, /* 36 */
    {0x3fbe3a6873fa1279ULL, 0xbfde2b5d3806f63bULL, 0xbc5e0d8920000000ULL, 0xbff0000000000000ULL}, /* 37 */
    {0x3fc592675bc57974ULL, 0xbfe1c73b39ae68c8ULL, 0xbc8b25dd20000000ULL, 0xbff0000000000000ULL}, /* 38 */
    {0x3fcd0dfe53aba2fdULL, 0xbfe44cf325091dd6ULL, 0xbc68076a20000000ULL, 0xbff0000000000000ULL}, /* 39 */
    {0xbfca827999fcef32ULL, 0xbfe6a09e667f3bcdULL, 0x3c8bdd3420000000ULL, 0xbfe0000000000000ULL}, /* 40 */
    {0xbfc133cc94247758ULL, 0xbfe8bc806b151741ULL, 0x3c82c5e120000000ULL, 0xbfe0000000000000ULL}, /* 41 */
    {0xbfac73b39ae68c87ULL, 0xbfea9b66290ea1a3ULL, 0xbc39f630e0000000ULL, 0xbfe0000000000000ULL}, /* 42 */
    {0x3f9d4a2c7f909c4eULL, 0xbfec38b2f180bdb1ULL, 0x3c76e0b180000000ULL, 0xbfe0000000000000ULL}, /* 43 */
    {0x3fbe087565455a75ULL, 0xbfed906bcf328d46ULL, 0xbc7457e620000000ULL, 0xbfe0000000000000ULL}, /* 44 */
    {0xbfa4a03176acf82dULL, 0xbfee9f4156c62ddaULL, 0xbc8760b1e0000000ULL, 0xbfd0000000000000ULL}, /* 45 */
    {0x3fac1d1f0e5967d5ULL, 0xbfef6297cff75cb0ULL, 0xbc75621720000000ULL, 0xbfd0000000000000ULL}, /* 46 */
    {0x3f9ba1650f592f50ULL, 0xbfefd88da3d12526ULL, 0x3c887df640000000ULL, 0xbfc0000000000000ULL}, /* 47 */
    {0x0000000000000000ULL, 0xbff0000000000000ULL, 0x0000000000000000ULL, 0x0000000000000000ULL}, /* 48 */
    {0xbf9ba1650f592f50ULL, 0xbfefd88da3d12526ULL, 0x3c887df640000000ULL, 0x3fc0000000000000ULL}, /* 49 */
    {0xbfac1d1f0e5967d5ULL, 0xbfef6297cff75cb0ULL, 0xbc75621720000000ULL, 0x3fd0000000000000ULL}, /* 50 */
    {0x3fa4a03176acf82dULL, 0xbfee9f4156c62ddaULL, 0xbc8760b1e0000000ULL, 0x3fd0000000000000ULL}, /* 51 */
    {0xbfbe087565455a75ULL, 0xbfed906bcf328d46ULL, 0xbc7457e620000000ULL, 0x3fe0000000000000ULL}, /* 52 */
    {0xbf9d4a2c7f909c4eULL, 0xbfec38b2f180bdb1ULL, 0x3c76e0b180000000ULL, 0x3fe0000000000000ULL}, /* 53 */
    {0x3fac73b39ae68c87ULL, 0xbfea9b66290ea1a3ULL, 0xbc39f630e0000000ULL, 0x3fe0000000000000ULL}, /* 54 */
    {0x3fc133cc94247758ULL, 0xbfe8bc806b151741ULL, 0x3c82c5e120000000ULL, 0x3fe0000000000000ULL}, /* 55 */
    {0x3fca827999fcef32ULL, 0xbfe6a09e667f3bcdULL, 0x3c8bdd3420000000ULL, 0x3fe0000000000000ULL}, /* 56 */
    {0xbfcd0dfe53aba2fdULL, 0xbfe44cf325091dd6ULL, 0xbc68076a20000000ULL, 0x3ff0000000000000ULL}, /* 57 */
    {0xbfc592675bc57974ULL, 0xbfe1c73b39ae68c8ULL, 0xbc8b25dd20000000ULL, 0x3ff0000000000000ULL}, /* 58 */
    {0xbfbe3a6873fa1279ULL, 0xbfde2b5d3806f63bULL, 0xbc5e0d8920000000ULL, 0x3ff0000000000000ULL}, /* 59 */
    {0xbfb37ca1866b95cfULL, 0xbfd87de2a6aea963ULL, 0x3c672cede0000000ULL, 0x3ff0000000000000ULL}, /* 60 */
    {0xbfa60bea939d225aULL, 0xbfd294062ed59f06ULL, 0x3c75d28da0000000ULL, 0x3ff0000000000000ULL}, /* 61 */
    {0xbf93ad06011469fbULL, 0xbfc8f8b83c69a60bULL, 0x3c626d19c0000000ULL, 0x3ff0000000000000ULL}, /* 62 */
    {0xbf73b92e176d6d31ULL, 0xbfb917a6bc29b42cULL, 0x3c3e2718e0000000ULL, 0x3ff0000000000000ULL}, /* 63 */
};

/* the constants after the table (sin 0x9eab10.., cos 0x9eb3e0..): packed pairs lo / hi */
#define C10_LO 0xbfc5555555555555ULL /* -1/6 */
#define C10_HI 0xbfe0000000000000ULL /* -1/2 */
#define C20_LO 0x3f81111111111111ULL /* 1/120 */
#define C20_HI 0x3fa5555555555555ULL /* 1/24 */
#define C30_LO 0xbf2a01a01a01a01aULL /* -1/5040 */
#define C30_HI 0xbf56c16c16c16c17ULL /* -1/720 */
#define C40_LO 0x3ec71de3a556c734ULL /* 1/362880 */
#define C40_HI 0x3efa01a01a01a01aULL /* 1/40320 */
#define C_32PI 0x40245f306dc9c883ULL /* 32 / pi */
#define C_SHIFT 0x4338000000000000ULL /* 1.5 * 2^52 */
#define C_P2 0x3d90b4611a600000ULL   /* pi/32, second part (both lanes) */
#define C_P1 0x3fb921fb54400000ULL   /* pi/32, first part */
#define C_P3 0x3b63198a2e037073ULL   /* pi/32, third part */
#define C_ONE_M 0x3fefffffffffffffULL /* 1 - 2^-53 */
#define C_ONE 0x3ff0000000000000ULL

#if (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))
#define ME_CRT_X87 1
#endif

#if defined(ME_CRT_X87)
/* Win 0x9ea298: CW 0x27f, then the 80-bit pi * 2^62 used by fprem1 */
static const unsigned char kX87Data[12] = {0x7f, 0x02, 0x35, 0xc2, 0x68, 0x21, 0xa2, 0xda, 0x0f, 0xc9, 0x3e, 0x40};

/* the x87 path of _CIsin / _CIcos (Win 0x6d0ce8 / 0x6d0e48) */
static double MeCrtX87(double x, int isCos)
{
    unsigned short cw, cw53 = 0x27f;
    double r;
    MeCrtU64 u;
    memcpy(&u, &x, 8);
    if ((u & 0x7ff0000000000000ULL) == 0x7ff0000000000000ULL && (u & 0x000fffffffffffffULL) == 0)
        return MeCrtD(0xfff8000000000000ULL); /* infinity: indefinite (Win 0xabab10), EDOM */
    __asm__ __volatile__("fnstcw %0" : "=m"(cw));
    __asm__ __volatile__("fldcw %0" : : "m"(cw53));
    if (isCos)
        __asm__ __volatile__("fcos\n\t"
                             "fnstsw %%ax\n\t"
                             "testw $0x400, %%ax\n\t"
                             "jz 1f\n\t"
                             "fldt %2\n\t"
                             "fxch %%st(1)\n"
                             "2:\n\t"
                             "fprem1\n\t"
                             "fnstsw %%ax\n\t"
                             "testw $0x400, %%ax\n\t"
                             "jnz 2b\n\t"
                             "fstp %%st(1)\n\t"
                             "fcos\n"
                             "1:"
                             : "=t"(r)
                             : "0"(x), "m"(kX87Data[2])
                             : "ax", "cc");
    else
        __asm__ __volatile__("fsin\n\t"
                             "fnstsw %%ax\n\t"
                             "testw $0x400, %%ax\n\t"
                             "jz 1f\n\t"
                             "fldt %2\n\t"
                             "fxch %%st(1)\n"
                             "2:\n\t"
                             "fprem1\n\t"
                             "fnstsw %%ax\n\t"
                             "testw $0x400, %%ax\n\t"
                             "jnz 2b\n\t"
                             "fstp %%st(1)\n\t"
                             "fsin\n"
                             "1:"
                             : "=t"(r)
                             : "0"(x), "m"(kX87Data[2])
                             : "ax", "cc");
    __asm__ __volatile__("fldcw %0" : : "m"(cw));
    return r;
}
/* the x87 path's fsin / fcos result (64-bit mantissa, never stored) times y, rounded once to
 * 53 bits (CW 0x27f) */
static double MeCrtX87Mul(double x, double y, int isCos)
{
    unsigned short cw, cw53 = 0x27f;
    double r;
    __asm__ __volatile__("fnstcw %0" : "=m"(cw));
    __asm__ __volatile__("fldcw %0" : : "m"(cw53));
    if (isCos)
        __asm__ __volatile__("fcos\n\t"
                             "fnstsw %%ax\n\t"
                             "testw $0x400, %%ax\n\t"
                             "jz 1f\n\t"
                             "fldt %2\n\t"
                             "fxch %%st(1)\n"
                             "2:\n\t"
                             "fprem1\n\t"
                             "fnstsw %%ax\n\t"
                             "testw $0x400, %%ax\n\t"
                             "jnz 2b\n\t"
                             "fstp %%st(1)\n\t"
                             "fcos\n"
                             "1:\n\t"
                             "fmull %3"
                             : "=t"(r)
                             : "0"(x), "m"(kX87Data[2]), "m"(y)
                             : "ax", "cc");
    else
        __asm__ __volatile__("fsin\n\t"
                             "fnstsw %%ax\n\t"
                             "testw $0x400, %%ax\n\t"
                             "jz 1f\n\t"
                             "fldt %2\n\t"
                             "fxch %%st(1)\n"
                             "2:\n\t"
                             "fprem1\n\t"
                             "fnstsw %%ax\n\t"
                             "testw $0x400, %%ax\n\t"
                             "jnz 2b\n\t"
                             "fstp %%st(1)\n\t"
                             "fsin\n"
                             "1:\n\t"
                             "fmull %3"
                             : "=t"(r)
                             : "0"(x), "m"(kX87Data[2]), "m"(y)
                             : "ax", "cc");
    __asm__ __volatile__("fldcw %0" : : "m"(cw));
    return r;
}
#else
static double MeCrtX87(double x, int isCos)
{
    return isCos ? cos(x) : sin(x);
}
static double MeCrtX87Mul(double x, double y, int isCos)
{
    return (isCos ? cos(x) : sin(x)) * y;
}
#endif

/* the range test of the SSE2 routines: 1 when they hand over to the x87 path (large, inf, NaN) */
static int MeCrtX87Range(double x)
{
    MeCrtU64 u;
    unsigned short ax;
    memcpy(&u, &x, 8);
    ax = (unsigned short)(((unsigned short)(u >> 48) & 0x7fff) - 0x3030);
    return ax > 0x10c5 && (short)ax > 0x10c5;
}

/* Win 0x6e160e (phase 0) / 0x6e1b5e (phase 16) */
static double MeCrtSinCos(double x, int phase)
{
    MeCrtU64 u;
    unsigned short ax;
    const MeCrtU64* T;
    double y, k, t3, kP2, r1, kP3, r2, a5lo, a5hi, s7, s3, z, e1, q2lo, q2hi, a6lo, a6hi, m3, z2, m4;
    double x0, x7, x5, T0, T1, T2, T3;
    int n;

    memcpy(&u, &x, 8);
    ax = (unsigned short)(((unsigned short)(u >> 48) & 0x7fff) - 0x3030);
    if (ax > 0x10c5) {
        if ((short)ax > 0x10c5)
            return MeCrtX87(x, phase != 0); /* large, inf, NaN */
        if (phase != 0) {                    /* cos: 1 - |x| */
            u &= 0x7fffffffffffffffULL;
            return MeCrtD(C_ONE) - MeCrtD(u);
        }
        if ((unsigned short)(ax >> 4) == 0xcfd) /* zero / denormal */
            return x * MeCrtD(C_ONE_M);
        return x;
    }
    y = MeCrtD(C_32PI) * x;
    /* cvtsd2si (round to nearest even, MXCSR default) = the shifter result below */
    k = (y + MeCrtD(C_SHIFT)) - MeCrtD(C_SHIFT);
    n = (int)k;
    t3 = MeCrtD(C_P1) * k;
    kP2 = MeCrtD(C_P2) * k;
    r1 = x - t3;
    kP3 = k * MeCrtD(C_P3);
    T = kTable[(n + phase) & 63];
    T0 = MeCrtD(T[0]);
    T1 = MeCrtD(T[1]);
    T2 = MeCrtD(T[2]);
    T3 = MeCrtD(T[3]);
    r2 = r1 - kP2;
    a5lo = MeCrtD(C40_LO) * r1;
    a5hi = MeCrtD(C40_HI) * r1;
    s7 = T1 * r2;
    s3 = r1 - r2;
    a5lo = a5lo * r2;
    a5hi = a5hi * r2;
    z = r2 * r2;
    s3 = s3 - kP2;
    e1 = kP3 - s3;
    q2lo = T0 + T3;
    s7 = s7 - q2lo;
    q2lo = q2lo * r2;
    a6lo = MeCrtD(C20_LO) * z;
    a6hi = MeCrtD(C20_HI) * z;
    m3 = T3 * r2;
    q2lo = q2lo * z;
    q2hi = T1 * z;
    z2 = z * z;
    a5lo = a5lo + MeCrtD(C30_LO);
    a5hi = a5hi + MeCrtD(C30_HI);
    m4 = r2 * T0;
    a6lo = a6lo + MeCrtD(C10_LO);
    a6hi = a6hi + MeCrtD(C10_HI);
    a5lo = a5lo * z2;
    a5hi = a5hi * z2;
    x0 = m3;
    m3 = m3 + T1;
    e1 = e1 * s7;
    x7 = m4;
    m4 = m4 + m3;
    a6lo = a6lo + a5lo;
    a6hi = a6hi + a5hi;
    x5 = T1 - m3;
    m3 = m3 - m4;
    e1 = e1 + T2;
    a6lo = a6lo * q2lo;
    a6hi = a6hi * q2hi;
    x5 = x5 + x0;
    m3 = m3 + x7;
    e1 = e1 + x5;
    e1 = e1 + m3;
    e1 = e1 + a6lo;
    e1 = e1 + a6hi;
    return m4 + e1;
}

double MeCrtSin(double x)
{
    return MeCrtSinCos(x, 0);
}

double MeCrtCos(double x)
{
    return MeCrtSinCos(x, 16);
}

double MeCrtSinMul(double x, double y)
{
    MeCrtU64 u;
    memcpy(&u, &x, 8);
    /* infinities: the x87 path returns the indefinite NaN without an fsin */
    if (MeCrtX87Range(x) && (u & 0x7ff0000000000000ULL) != 0x7ff0000000000000ULL)
        return MeCrtX87Mul(x, y, 0);
    return MeCrtSinCos(x, 0) * y;
}

double MeCrtCosMul(double x, double y)
{
    MeCrtU64 u;
    memcpy(&u, &x, 8);
    if (MeCrtX87Range(x) && (u & 0x7ff0000000000000ULL) != 0x7ff0000000000000ULL)
        return MeCrtX87Mul(x, y, 1);
    return MeCrtSinCos(x, 16) * y;
}


/* ---------------------------------------------------------------------------------------------
 * asin / acos: the generated SSE2 transcriptions */
typedef unsigned int MeCrtU32;
typedef struct {
    MeCrtU64 lo, hi;
} MeCrtX;

static MeCrtU64 MeCrtBits(double d)
{
    MeCrtU64 u;
    memcpy(&u, &d, 8);
    return u;
}
#define MeCrtDbl MeCrtD

/* pmovmskb: the top bit of each of the 16 bytes */
static MeCrtU32 MeCrtMovmskb(MeCrtX x)
{
    MeCrtU32 m = 0;
    int i;
    for (i = 0; i < 8; ++i) {
        m |= (MeCrtU32)((x.lo >> (8 * i + 7)) & 1) << i;
        m |= (MeCrtU32)((x.hi >> (8 * i + 7)) & 1) << (i + 8);
    }
    return m;
}

/* pshufd: dword i of the result = dword ((imm >> 2i) & 3) of x */
static MeCrtX MeCrtPshufd(MeCrtX x, int imm)
{
    MeCrtU32 d[4], r[4];
    MeCrtX o;
    int i;
    d[0] = (MeCrtU32)x.lo;
    d[1] = (MeCrtU32)(x.lo >> 32);
    d[2] = (MeCrtU32)x.hi;
    d[3] = (MeCrtU32)(x.hi >> 32);
    for (i = 0; i < 4; ++i)
        r[i] = d[(imm >> (2 * i)) & 3];
    o.lo = (MeCrtU64)r[0] | ((MeCrtU64)r[1] << 32);
    o.hi = (MeCrtU64)r[2] | ((MeCrtU64)r[3] << 32);
    return o;
}

#include "MeCrtMathSse2.h"

double MeCrtAsin(double x)
{
    return MeCrtAsinSse2(x);
}

double MeCrtAcos(double x)
{
    return MeCrtAcosSse2(x);
}

/* ---------------------------------------------------------------------------------------------
 * atan2 and the x87 values */
#if defined(ME_CRT_X87)
MeCrtLong MeCrtAtan2Long(double y, double x)
{
    MeCrtLong r;
    unsigned short cw, cw64 = 0x033f; /* the dispatcher's 0x133f (bit 12 has no effect) */
    MeCrtU64 uy, ux;
    memcpy(&uy, &y, 8);
    memcpy(&ux, &x, 8);
    memset(&r, 0, sizeof r);
    __asm__ __volatile__("fnstcw %0" : "=m"(cw));
    __asm__ __volatile__("fldcw %0" : : "m"(cw64));
    if ((uy & 0x7fffffffffffffffULL) > 0x7ff0000000000000ULL || (ux & 0x7fffffffffffffffULL) > 0x7ff0000000000000ULL) {
        /* a NaN argument: the dispatcher's NaN entry returns FPATAN's NaN (the quieted NaN
         * operand; of two, the one with the larger significand), sign kept */
        __asm__ __volatile__("fldl %1\n\tfldl %2\n\tfpatan\n\tfstpt %0" : "=m"(r) : "m"(y), "m"(x) : "st", "st(1)");
        __asm__ __volatile__("fldcw %0" : : "m"(cw));
        return r;
    }
    if ((uy & 0x7fffffffffffffffULL) == 0x7ff0000000000000ULL && (ux & 0x7fffffffffffffffULL) == 0x7ff0000000000000ULL) {
        /* both infinite: the dispatcher's domain error, the x87 indefinite NaN (not FPATAN's
         * +-pi/4, +-3pi/4) */
        static const unsigned char kIndefinite[10] = {0, 0, 0, 0, 0, 0, 0, 0xc0, 0xff, 0xff};
        memcpy(r.x87, kIndefinite, 10);
        __asm__ __volatile__("fldcw %0" : : "m"(cw));
        return r;
    }
    __asm__ __volatile__("fldl %1\n\t"
                         "fabs\n\t"
                         "fldl %2\n\t"
                         "fabs\n\t"
                         "fpatan\n\t"
                         "testl %3, %3\n\t"
                         "jz 1f\n\t"
                         "fchs\n\t" /* pi - r as pi + (-r): same rounding, no fsub/fsubr ambiguity */
                         "fldpi\n\t"
                         "faddp\n"
                         "1:\n\t"
                         "testl %4, %4\n\t"
                         "jz 2f\n\t"
                         "fchs\n"
                         "2:\n\t"
                         "fstpt %0"
                         : "=m"(r)
                         : "m"(y), "m"(x), "r"((int)(ux >> 63)), "r"((int)(uy >> 63))
                         : "st", "st(1)", "cc");
    __asm__ __volatile__("fldcw %0" : : "m"(cw));
    return r;
}

/* the original's x87 code runs with CW 0x27f: operations below round once to 53 bits */
double MeCrtLongToDouble(MeCrtLong a)
{
    double r;
    __asm__ __volatile__("fldt %1\n\tfstpl %0" : "=m"(r) : "m"(a) : "st");
    return r;
}

MeCrtLong MeCrtLongNeg(MeCrtLong a)
{
    a.x87[9] ^= 0x80;
    return a;
}

int MeCrtLongGreater(MeCrtLong a, double b)
{
    unsigned short sw;
    __asm__ __volatile__("fldl %2\n\t"
                         "fldt %1\n\t"
                         "fcompp\n\t"
                         "fnstsw %0"
                         : "=a"(sw)
                         : "m"(a), "m"(b)
                         : "st", "st(1)", "cc");
    return (sw & 0x4500) == 0; /* C3 = C2 = C0 = 0: st0 (a) > st1 (b) */
}

double MeCrtLongAdd(MeCrtLong a, double b)
{
    unsigned short cw, cw53 = 0x27f;
    double r;
    __asm__ __volatile__("fnstcw %0" : "=m"(cw));
    __asm__ __volatile__("fldcw %0" : : "m"(cw53));
    __asm__ __volatile__("fldt %1\n\tfaddl %2\n\tfstpl %0" : "=m"(r) : "m"(a), "m"(b) : "st");
    __asm__ __volatile__("fldcw %0" : : "m"(cw));
    return r;
}

double MeCrtDoubleSubLong(double b, MeCrtLong a)
{
    unsigned short cw, cw53 = 0x27f;
    double r;
    __asm__ __volatile__("fnstcw %0" : "=m"(cw));
    __asm__ __volatile__("fldcw %0" : : "m"(cw53));
    __asm__ __volatile__("fldt %1\n\tfsubrl %2\n\tfstpl %0" : "=m"(r) : "m"(a), "m"(b) : "st");
    __asm__ __volatile__("fldcw %0" : : "m"(cw));
    return r;
}
#else
/* off x86: the value is the C library's atan2 as a double */
MeCrtLong MeCrtAtan2Long(double y, double x)
{
    MeCrtLong r;
    double d = atan2(y, x);
    memset(&r, 0, sizeof r);
    memcpy(r.x87, &d, 8);
    return r;
}
double MeCrtLongToDouble(MeCrtLong a)
{
    double d;
    memcpy(&d, a.x87, 8);
    return d;
}
MeCrtLong MeCrtLongNeg(MeCrtLong a)
{
    a.x87[7] ^= 0x80;
    return a;
}
int MeCrtLongGreater(MeCrtLong a, double b)
{
    return MeCrtLongToDouble(a) > b;
}
double MeCrtLongAdd(MeCrtLong a, double b)
{
    return MeCrtLongToDouble(a) + b;
}
double MeCrtDoubleSubLong(double b, MeCrtLong a)
{
    return b - MeCrtLongToDouble(a);
}
#endif

double MeCrtAtan2(double y, double x)
{
    return MeCrtLongToDouble(MeCrtAtan2Long(y, x));
}
