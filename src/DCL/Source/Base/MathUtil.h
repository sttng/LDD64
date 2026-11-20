// LEGO Digital Designer — restored source
// Scalar math helpers for Geometry.cpp and Color.cpp (file and names inferred; the name avoids
// <math.h> on case-insensitive file systems). Inlined on Mac; Win COMDATs: 0x404aa0 ATan,
// 0x404ac0 Cos, 0x404ae0 Sin, 0x404b00 Sqrt, 0x404b20 Tan, 0x404b40 / 0x404b60 / 0x404c10
// overloads, 0x404b80 SinCos, 0x404bb0 ACos, 0x404c30 IsEqual, 0x404c70 IsGreater, 0x404cb0
// IsGreaterOrEqual, 0x404cd0 IsLess; 0x401010 CRT fabs.
// Constants: Mac literals (1e-5f = 0x3727c5ac); Win reads PI 0xae15f8, EPSILON 0xae15fc, HALF_PI
// 0xb4278c (.data, set by a static initialiser, hence `PI / 2`).
#pragma once
#include "LEGO/Types.h"
#include <math.h>

namespace LEGO {

const Real32 PI      = 3.14159265f;   // (name inferred)
const Real32 HALF_PI = PI / 2;         // (name inferred)
const Real32 EPSILON = 1e-5f;          // general geometry tolerance (name inferred)

// Evaluation type of Real32 expressions (restoration aid). Win (MSVC x87, PC=53) keeps
// intermediates in double and rounds at stores; Mac (SSE) rounds each operation. Written as
// `(Real32)((EvalReal)a * b + ...)` both are reproduced bit for bit. Use float literals (0.5f);
// a Win-only double constant goes under #if defined(_WIN32).
#if defined(_WIN32)
typedef double EvalReal;
#else
typedef Real32 EvalReal;
#endif

// The Mac build calls the float CRT functions (sinf, cosf, tanf, atanf, asinf, atan2f; sqrtss
// for the square root), the Windows build the double ones and rounds the result to float.
inline Real32 Abs(Real32 x)   { return (Real32)fabs((double)x); }
#if defined(_WIN32)
inline Real32 Sqrt(Real32 x)  { return (Real32)sqrt((double)x); }    // Win 0x404b00
inline Real32 Sin(Real32 x)   { return (Real32)sin((double)x); }     // Win 0x404ae0
inline Real32 Cos(Real32 x)   { return (Real32)cos((double)x); }     // Win 0x404ac0
inline Real32 Tan(Real32 x)   { return (Real32)tan((double)x); }     // Win 0x404b20
inline Real32 ATan(Real32 x)  { return (Real32)atan((double)x); }    // Win 0x404aa0
inline Real32 ASin(Real32 x)  { return (Real32)asin((double)x); }
inline Real32 ATan2(Real32 y, Real32 x) { return (Real32)atan2((double)y, (double)x); }
#else
inline Real32 Sqrt(Real32 x)  { return sqrtf(x); }
inline Real32 Sin(Real32 x)   { return sinf(x); }
inline Real32 Cos(Real32 x)   { return cosf(x); }
inline Real32 Tan(Real32 x)   { return tanf(x); }
inline Real32 ATan(Real32 x)  { return atanf(x); }
inline Real32 ASin(Real32 x)  { return asinf(x); }
inline Real32 ATan2(Real32 y, Real32 x) { return atan2f(y, x); }
#endif

inline void SinCos(Real32 angle, Real32& s, Real32& c)              // Win 0x404b80
{
    s = Sin(angle);
    c = Cos(angle);
}

// acos with the argument snapped to the ends of its domain (Mac: inlined in
// Matrix3D::GetAxisAngle with a call to the double acos).                Win 0x404bb0
inline Real32 ACos(Real32 x)
{
    if (x > 0.99999f)
        return 0.0f;
    if (x < -0.99999f)
        return PI;
    return (Real32)acos((double)x);
}

// Tolerance comparisons.                         Win 0x404c30, 0x404c70, 0x404cb0, 0x404cd0
inline bool IsEqual(Real32 a, Real32 b, Real32 epsilon)          { return Abs(a - b) < epsilon; }
inline bool IsGreater(Real32 a, Real32 b, Real32 epsilon)        { return Abs(a - b) > epsilon && a - b > 0.0f; }
inline bool IsGreaterOrEqual(Real32 a, Real32 b, Real32 epsilon) { return b < a + epsilon; }
inline bool IsLess(Real32 a, Real32 b, Real32 epsilon)           { return Abs(a - b) > epsilon && a - b < 0.0f; }

// min/max as used by the clamping code (SSE minss/maxss operand order on Mac: the first
// operand is returned unless the comparison holds). (names inferred)
inline Real32 Min(Real32 a, Real32 b) { return a < b ? a : b; }
inline Real32 Max(Real32 a, Real32 b) { return a > b ? a : b; }

} // namespace LEGO
