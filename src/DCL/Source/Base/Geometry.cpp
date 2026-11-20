// LEGO Digital Designer — restored source
// Base/Geometry.cpp — Mac TU 127 (0x1dc3e0-0x1e5d2f), Win 0x404d90-0x40b480. See Geometry.h.
//
// Floating point: same evaluation order in both builds. Mac rounds each operation to float
// (SSE); Win keeps x87 intermediates (53-bit) and rounds at stores (float locals, arguments,
// returns, MSVC float spills such as the squares in DistanceSquared 0x408da0). Written with
// EvalReal (MathUtil.h: double on Win, float on Mac) and (Real32) casts at those points;
// tests/diff/diff_geometry.cpp checks the Win branch bit for bit.
//
// Not restored as code: the static initialiser 0x1e5cf0 (`#include <iostream>`).
#include "Geometry.h"
#include <float.h>
#include <string.h>
#if !defined(_WIN32)
#include <assert.h>
#endif

#if defined(__APPLE__) && !defined(_WIN32)
// Accelerate.framework (CLAPACK, Fortran calling convention)
extern "C" int sgesv_(int* n, int* nrhs, float* a, int* lda, int* ipiv, float* b, int* ldb,
                      int* info);
#endif

namespace LEGO {

// ===========================================================================================
// Vector2
// ===========================================================================================
Vector2::Vector2() : x(0.0f), y(0.0f) {}                            // Mac 0x1dc3e0, Win 0x404d90
Vector2::Vector2(Real32 x_, Real32 y_) : x(x_), y(y_) {}            // Mac 0x1dc400, Win 0x404da0
Vector2::Vector2(const Vector2& v) : x(v.x), y(v.y) {}              // Mac 0x1dc420, Win 0x404dc0

Real32 Vector2::GetArea() const { return x * y; }                   // Mac 0x1dc440, Win 0x404de0
bool Vector2::IsZero() const { return x == 0.0f && y == 0.0f; }     // Mac 0x1dc460, Win 0x404df0
void Vector2::Set(Real32 x_, Real32 y_) { x = x_; y = y_; }         // Mac 0x1dc490

Vector2& Vector2::operator=(const Vector2& v)                       // Mac 0x1dc4b0, Win 0x404e10
{
    x = v.x;
    y = v.y;
    return *this;
}

Vector2& Vector2::operator+=(const Vector2& v)                      // Mac 0x1dc4d0
{
    x += v.x;
    y += v.y;
    return *this;
}

Vector2 Vector2::operator+(Real32 s) const { return Vector2(x + s, y + s); }          // Mac 0x1dc500
Vector2 Vector2::operator+(const Vector2& v) const { return Vector2(x + v.x, y + v.y); } // Mac 0x1dc530, Win 0x404e30

Vector2& Vector2::operator-=(const Vector2& v)                      // Mac 0x1dc560
{
    x -= v.x;
    y -= v.y;
    return *this;
}

Vector2 Vector2::operator-(const Vector2& v) const { return Vector2(x - v.x, y - v.y); } // Mac 0x1dc590, Win 0x404e50

void Vector2::Scale(Real32 sx, Real32 sy)                           // Mac 0x1dc5c0, Win 0x404e70
{
    x *= sx;
    y *= sy;
}

Vector2& Vector2::operator*=(const Vector2& v)                      // Mac 0x1dc5f0
{
    x *= v.x;
    y *= v.y;
    return *this;
}

Vector2 Vector2::operator*(Real32 s) const { return Vector2(x * s, y * s); }          // Mac 0x1dc620

void Vector2::Divide(Real32 sx, Real32 sy)                          // Mac 0x1dc650
{
    x /= sx;
    y /= sy;
}

bool Vector2::operator==(const Vector2& v) const { return IsEqual(v, EPSILON); }      // Mac 0x1dc680
bool Vector2::operator!=(const Vector2& v) const { return !IsEqual(v, EPSILON); }     // Mac 0x1dc6d0, Win 0x404f00

Vector2::operator Real32*() { return &x; }                          // Mac 0x1dc720, Win 0x404f30
Vector2::operator const Real32*() const { return &x; }              // Mac 0x1dc730

// ===========================================================================================
// IntVector2 / UIntVector2
// ===========================================================================================
IntVector2::IntVector2() : x(0), y(0) {}                            // Mac 0x1dc740, Win 0x404f40
IntVector2::IntVector2(Int32 x_, Int32 y_) : x(x_), y(y_) {}        // Mac 0x1dc760, Win 0x404f50
IntVector2::IntVector2(const IntVector2& v) : x(v.x), y(v.y) {}     // Mac 0x1dc780

IntVector2& IntVector2::operator+=(const IntVector2& v)             // Mac 0x1dc7a0
{
    x += v.x;
    y += v.y;
    return *this;
}

IntVector2 IntVector2::operator+(const IntVector2& v) const { return IntVector2(x + v.x, y + v.y); } // Mac 0x1dc7c0
IntVector2 IntVector2::operator*(Int32 s) const { return IntVector2(x * s, y * s); }             // Mac 0x1dc7f0

UIntVector2::UIntVector2() : x(0), y(0) {}                          // Mac 0x1dc810
UIntVector2::UIntVector2(UInt32 x_, UInt32 y_) : x(x_), y(y_) {}    // Mac 0x1dc830
UIntVector2::UIntVector2(const UIntVector2& v) : x(v.x), y(v.y) {}  // Mac 0x1dc850
void UIntVector2::Set(UInt32 x_, UInt32 y_) { x = x_; y = y_; }     // Mac 0x1dc870

// ===========================================================================================
// Vector3
// ===========================================================================================
Vector3::Vector3() : x(0.0f), y(0.0f), z(0.0f) {}                   // Mac 0x1dc890, Win 0x404f70
Vector3::Vector3(Real32 x_, Real32 y_, Real32 z_) : x(x_), y(y_), z(z_) {} // Mac 0x1dc8b0, Win 0x404f80
Vector3::Vector3(const Vector3& v) : x(v.x), y(v.y), z(v.z) {}      // Mac 0x1dc8e0, Win 0x404fa0

Real32 Vector3::GetLength() const { return Sqrt(GetLengthSquared()); }    // Mac 0x1dc900, Win 0x404fc0

Real32 Vector3::GetLengthSquared() const                            // Mac 0x1dc940, Win 0x404ff0
{
    return (Real32)((EvalReal)x * x + (EvalReal)y * y + (EvalReal)z * z);
}

bool Vector3::IsZero() const { return x == 0.0f && y == 0.0f && z == 0.0f; } // Mac 0x1dc980

bool Vector3::IsAlmostZero() const                                  // Mac 0x1dc9c0, Win 0x405020
{
    return Abs(x) < EPSILON && Abs(y) < EPSILON && Abs(z) < EPSILON;
}

void Vector3::Clear()                                               // Mac 0x1dca10, Win 0x405090
{
    z = 0.0f;
    y = 0.0f;
    x = 0.0f;
}

void Vector3::SetLength(Real32 length)                              // Mac 0x1dca30, Win 0x4050a0
{
    Real32 current = GetLength();
    if (current != 0.0f) {
        Real32 scale = length / current;
        x *= scale;
        y *= scale;
        z *= scale;
    }
}

void Vector3::Normalize() { SetLength(1.0f); }                      // Mac 0x1dcaa0, Win 0x405120

Real32 Vector3::Dot(const Vector3& v) const                         // Mac 0x1dcb10, Win 0x405130
{
    return (Real32)((EvalReal)x * v.x + (EvalReal)y * v.y + (EvalReal)z * v.z);
}

Real32 Vector3::operator*(const Vector3& v) const                   // Mac 0x1dcb50, Win 0x405150
{
    return (Real32)((EvalReal)x * v.x + (EvalReal)y * v.y + (EvalReal)z * v.z);
}

Vector3 Vector3::Cross(const Vector3& v) const                      // Mac 0x1dcb90, Win 0x405180
{
    return Vector3((Real32)((EvalReal)y * v.z - (EvalReal)z * v.y),
                   (Real32)((EvalReal)z * v.x - (EvalReal)x * v.z),
                   (Real32)((EvalReal)x * v.y - (EvalReal)y * v.x));
}

Vector3 Vector3::operator^(const Vector3& v) const { return Cross(v); } // Mac 0x1dcc00, Win 0x4051e0

void Vector3::SetNormal(const Vector3& a, const Vector3& b, const Vector3& c) // Mac 0x1dcc70, Win 0x4051f0
{
    *this = (a - b).Cross(a - c);
    Normalize();
}

Vector3 Vector3::operator-(const Vector3& v) const { return Vector3(x - v.x, y - v.y, z - v.z); } // Mac 0x1dcd50, Win 0x4053b0

void Vector3::Set(const Vector3& v)                                 // Mac 0x1dcd90
{
    x = v.x;
    y = v.y;
    z = v.z;
}

Vector3& Vector3::operator=(const Vector3& v)                       // Mac 0x1dcdb0, Win 0x4052b0
{
    x = v.x;
    y = v.y;
    z = v.z;
    return *this;
}

Vector3& Vector3::operator+=(const Vector3& v)                      // Mac 0x1dcdd0, Win 0x4052e0
{
    x += v.x;
    y += v.y;
    z += v.z;
    return *this;
}

Vector3 Vector3::operator+(Real32 s) const { return Vector3(x + s, y + s, z + s); }               // Mac 0x1dce10, Win 0x405320
Vector3 Vector3::operator+(const Vector3& v) const { return Vector3(x + v.x, y + v.y, z + v.z); } // Mac 0x1dce50, Win 0x405340

void Vector3::Subtract(const Vector3& v)                            // Mac 0x1dce90
{
    x -= v.x;
    y -= v.y;
    z -= v.z;
}

Vector3& Vector3::operator-=(const Vector3& v)                      // Mac 0x1dced0, Win 0x405370
{
    x -= v.x;
    y -= v.y;
    z -= v.z;
    return *this;
}

Vector3 Vector3::operator-() const { return Vector3(-x, -y, -z); }  // Mac 0x1dcf10, Win 0x4053e0

void Vector3::Scale(Real32 sx, Real32 sy, Real32 sz)                // Mac 0x1dcf50, Win 0x405400
{
    x *= sx;
    y *= sy;
    z *= sz;
}

Vector3& Vector3::operator*=(Real32 s)                              // Mac 0x1dcf90, Win 0x405430
{
    x *= s;
    y *= s;
    z *= s;
    return *this;
}

Vector3 Vector3::operator*(Real32 s) const { return Vector3(x * s, y * s, z * s); } // Mac 0x1dcfd0, Win 0x405450

Vector3& Vector3::operator/=(Real32 s)                              // Mac 0x1dd010
{
    x /= s;
    y /= s;
    z /= s;
    return *this;
}

void Vector3::Divide(Real32 s)                                      // Mac 0x1dd050
{
    x /= s;
    y /= s;
    z /= s;
}

Vector3 Vector3::operator/(Real32 s) const { return Vector3(x / s, y / s, z / s); } // Mac 0x1dd090, Win 0x405480

bool Vector3::IsOnLine(const Vector3& direction, const Vector3& point, Real32 epsilon) const // Mac 0x1dd0d0, Win 0x4054a0
{
    return point.GetDistanceSquared(ClosestPointOnLine(*this, direction, point)) < epsilon;
}

// Win 0x408da0 (cdecl DistanceSquared): the squares are rounded to float, the sum is not.
Real32 Vector3::GetDistanceSquared(const Vector3& v) const          // Mac 0x1dd190
{
    Real32 dx = x - v.x, dy = y - v.y, dz = z - v.z;
    Real32 xx = dx * dx, yy = dy * dy, zz = dz * dz;
    return (Real32)((EvalReal)xx + yy + zz);
}

bool Vector3::IsSameLine(const Vector3& direction, const Vector3& origin,
                         const Vector3& otherDirection, Real32 epsilon) const // Mac 0x1dd1e0, Win 0x405550
{
    if (!direction.IsEqual(otherDirection, epsilon) && !direction.IsEqual(-otherDirection, epsilon))
        return false;
    return IsOnLine(direction, origin, epsilon);
}

bool Vector3::IsEqual(const Vector3& v, Real32 epsilon) const       // Mac 0x1dd350, Win 0x4056a0
{
    return Abs(x - v.x) <= epsilon && Abs(y - v.y) <= epsilon && Abs(z - v.z) <= epsilon;
}

Vector3 Vector3::GetPerpendicular() const                           // Mac 0x1dd3b0, Win 0x4055d0
{
    Real32 ax = Abs(x);
    Real32 ay = Abs(y);
    if (ay <= ax) {
        if (ay < Abs(z))
            return Vector3(0.0f, 1.0f, 0.0f).Cross(*this);
    } else if (ax < Abs(z)) {
        return Vector3(1.0f, 0.0f, 0.0f).Cross(*this);
    }
    return Vector3(0.0f, 0.0f, 1.0f).Cross(*this);
}

bool Vector3::operator==(const Vector3& v) const { return IsEqual(v, EPSILON); }  // Mac 0x1dd4a0
bool Vector3::operator!=(const Vector3& v) const { return !IsEqual(v, EPSILON); } // Mac 0x1dd510

bool Vector3::operator<(const Vector3& v) const                     // Mac 0x1dd570, Win 0x405730
{
    Real32 d = x - v.x;
    if (Abs(d) > EPSILON)
        return d < 0.0f;
    d = y - v.y;
    if (Abs(d) > EPSILON)
        return d < 0.0f;
    d = z - v.z;
    if (Abs(d) > EPSILON)
        return d < 0.0f;
    return false;
}

Vector3::operator Real32*() { return &x; }                          // Mac 0x1dd5f0
Vector3::operator const Real32*() const { return &x; }              // Mac 0x1dd600

Real32 Vector3::GetPitch() const { return ASin(-y / GetLength()); } // Mac 0x1dd610, Win 0x4057d0

Real32 Vector3::GetYaw() const                                      // Mac 0x1dd670, Win 0x405830
{
    if (Abs(z) < EPSILON)
        return x >= 0.0f ? HALF_PI : -HALF_PI;
    Real32 angle = ATan(x / z);
    if (z < 0.0f)
        angle += x >= 0.0f ? PI : -PI;
    return angle;
}

// ===========================================================================================
// Rect
// ===========================================================================================
Rect::Rect(Real32 left, Real32 top, Real32 right, Real32 bottom)    // Mac 0x1dd710, Win 0x4058e0
    : m_min(left, top), m_max(right, bottom)
{
}

Rect::Rect(const Vector2& position, Real32 width, Real32 height)    // Mac 0x1dd740, Win 0x405900
    : m_min(position), m_max(position.x + width, position.y + height)
{
}

Rect::Rect(const Vector2& position, const Vector2& size)            // Mac 0x1dd780, Win 0x405930
    : m_min(position), m_max(position + size)
{
}

Rect::Rect(const Vector2& size) : m_min(), m_max(size) {}           // Mac 0x1dd7c0, Win 0x405960
Rect::Rect(const Rect& r) : m_min(r.m_min), m_max(r.m_max) {}       // Mac 0x1dd7f0, Win 0x405980

void Rect::Set(Real32 left, Real32 top, Real32 right, Real32 bottom) // Mac 0x1dd820, Win 0x4059a0
{
    m_min.x = left;
    m_min.y = top;
    m_max.x = right;
    m_max.y = bottom;
}

void Rect::Set(const Vector2& position, const Vector2& size)        // Mac 0x1dd850
{
    m_min = position;
    m_max = position + size;
}

Rect& Rect::operator=(const Rect& r)                                // Mac 0x1dd890, Win 0x4059c0
{
    m_min = r.m_min;
    m_max = r.m_max;
    return *this;
}

void Rect::Set(const Rect& r)                                       // Mac 0x1dd8c0, Win 0x4059f0
{
    m_min = r.m_min;
    m_max = r.m_max;
}

Real32 Rect::GetWidth() const { return m_max.x - m_min.x; }         // Mac 0x1dd8f0, Win 0x405c20
Real32 Rect::GetHeight() const { return m_max.y - m_min.y; }        // Mac 0x1dd910, Win 0x405c60

void Rect::SetPosition(const Vector2& position)                     // Mac 0x1dd930, Win 0x405a20
{
    Vector2 old = m_min;
    m_min = position;
    m_max.x = (m_max.x - old.x) + position.x;
    m_max.y = (m_max.y - old.y) + position.y;
}

void Rect::Offset(const Vector2& delta)                             // Mac 0x1dd980
{
    m_min += delta;
    m_max += delta;
}

void Rect::SetSize(const Vector2& size) { m_max = m_min + size; }   // Mac 0x1dd9b0, Win 0x405a70

void Rect::Normalize()                                              // Mac 0x1dd9e0, Win 0x405a90
{
    if (m_max.x < m_min.x) {
        Real32 t = m_min.x;
        m_min.x = m_max.x;
        m_max.x = t;
    }
    if (m_max.y < m_min.y) {
        Real32 t = m_min.y;
        m_min.y = m_max.y;
        m_max.y = t;
    }
}

void Rect::Scale(const Vector2& factor)                             // Mac 0x1dda20, Win 0x405ae0
{
    m_min *= factor;
    m_max *= factor;
}

void Rect::Divide(const Vector2& divisor)                           // Mac 0x1dda70, Win 0x405b10
{
    m_min.Divide(divisor.x, divisor.y);
    m_max.Divide(divisor.x, divisor.y);
}

bool Rect::IsValid() const { return m_min.x < m_max.x && m_min.y < m_max.y; } // Mac 0x1ddac0, Win 0x405b40
bool Rect::IsZero() const { return m_min.IsZero() && m_max.IsZero(); }        // Mac 0x1ddaf0, Win 0x405b70
Vector2 Rect::GetSize() const { return m_max - m_min; }             // Mac 0x1ddb40, Win 0x405bb0

Vector2 Rect::GetAbsoluteSize() const                               // Mac 0x1ddb70, Win 0x405bd0
{
    return Vector2(GetAbsoluteWidth(), GetAbsoluteHeight());
}

Real32 Rect::GetAbsoluteWidth() const                               // Mac 0x1ddbe0, Win 0x405c30
{
    return m_max.x < m_min.x ? m_min.x - m_max.x : m_max.x - m_min.x;
}

Real32 Rect::GetAbsoluteHeight() const                              // Mac 0x1ddc20, Win 0x405c70
{
    return m_max.y < m_min.y ? m_min.y - m_max.y : m_max.y - m_min.y;
}

bool Rect::IsFlipped() const { return m_max.x < m_min.x; }          // Mac 0x1ddc60, Win 0x405ca0
Rect Rect::GetFlipped() const { return Rect(m_max.x, m_min.y, m_min.x, m_max.y); } // Mac 0x1ddc80, Win 0x405cc0
Rect::operator Real32*() { return &m_min.x; }                       // Mac 0x1ddcb0
Vector2 Rect::GetTopRight() const { return Vector2(m_max.x, m_min.y); }   // Mac 0x1ddcc0, Win 0x405d00
Vector2 Rect::GetBottomLeft() const { return Vector2(m_min.x, m_max.y); } // Mac 0x1ddce0, Win 0x405d20
Vector2& Rect::GetMax() { return m_max; }                           // Mac 0x1ddd00

bool Rect::operator==(const Rect& r) const                          // Mac 0x1ddd10, Win 0x405d40
{
    return m_min.IsEqual(r.m_min, EPSILON) && m_max.IsEqual(r.m_max, EPSILON);
}

bool Rect::operator!=(const Rect& r) const                          // Mac 0x1ddd90, Win 0x405d90
{
    return !(m_min.IsEqual(r.m_min, EPSILON) && m_max.IsEqual(r.m_max, EPSILON));
}

bool Rect::Contains(const Vector2& p) const                         // Mac 0x1dde10, Win 0x405df0
{
    return m_min.x <= p.x && m_min.y <= p.y && p.x < m_max.x && p.y < m_max.y;
}

bool Rect::Contains(const Rect& r) const                            // Mac 0x1dde50, Win 0x405e40
{
    return m_min.x <= r.m_min.x && m_min.y <= r.m_min.y && r.m_max.x <= m_max.x &&
           r.m_max.y <= m_max.y;
}

Rect& Rect::operator|=(const Rect& r)                               // Mac 0x1ddea0, Win 0x405e90
{
    Union(r);
    return *this;
}

void Rect::Union(const Rect& r)                                     // Mac 0x1ddef0
{
    if (r.m_min.x < m_min.x) m_min.x = r.m_min.x;
    if (r.m_min.y < m_min.y) m_min.y = r.m_min.y;
    if (m_max.x < r.m_max.x) m_max.x = r.m_max.x;
    if (m_max.y < r.m_max.y) m_max.y = r.m_max.y;
}

Rect& Rect::operator&=(const Rect& r)                               // Mac 0x1ddf40, Win 0x405ef0
{
    Intersect(r);
    return *this;
}

// NOTE: no check for an empty result (IsValid() turns false).
void Rect::Intersect(const Rect& r)                                 // Mac 0x1ddf90
{
    if (m_min.x < r.m_min.x) m_min.x = r.m_min.x;
    if (m_min.y < r.m_min.y) m_min.y = r.m_min.y;
    if (r.m_max.x < m_max.x) m_max.x = r.m_max.x;
    if (r.m_max.y < m_max.y) m_max.y = r.m_max.y;
}

// ===========================================================================================
// IntRect
// ===========================================================================================
IntRect::IntRect(Int32 left, Int32 top, Int32 right, Int32 bottom)  // Mac 0x1ddfe0, Win 0x405f50
    : m_left(left), m_top(top), m_right(right), m_bottom(bottom)
{
}

Int32 IntRect::GetWidth() const { return m_right - m_left; }        // Mac 0x1de010
Int32 IntRect::GetHeight() const { return m_bottom - m_top; }       // Mac 0x1de020
bool IntRect::IsValid() const { return m_left < m_right && m_top < m_bottom; } // Mac 0x1de030

void IntRect::Intersect(const IntRect& r)                           // Mac 0x1de050
{
    if (m_left < r.m_left) m_left = r.m_left;
    if (m_top < r.m_top) m_top = r.m_top;
    if (r.m_right < m_right) m_right = r.m_right;
    if (r.m_bottom < m_bottom) m_bottom = r.m_bottom;
}

// ===========================================================================================
// BoundingBox
// ===========================================================================================
BoundingBox::BoundingBox(Real32 minX, Real32 minY, Real32 minZ,
                         Real32 maxX, Real32 maxY, Real32 maxZ)     // Mac 0x1de090, Win 0x405f70
    : m_min(minX, minY, minZ), m_max(maxX, maxY, maxZ)
{
}

BoundingBox::BoundingBox(const Vector3& position, const Vector3& size) // Mac 0x1de0e0, Win 0x405fa0
    : m_min(position), m_max(position + size)
{
}

BoundingBox::BoundingBox(const Vector3& size) : m_min(), m_max(size) {}            // Mac 0x1de130, Win 0x405fe0
BoundingBox::BoundingBox(const BoundingBox& b) : m_min(b.m_min), m_max(b.m_max) {} // Mac 0x1de160, Win 0x406000

BoundingBox& BoundingBox::operator=(const BoundingBox& b)           // Mac 0x1de190, Win 0x406030
{
    m_min = b.m_min;
    m_max = b.m_max;
    return *this;
}

Real32 BoundingBox::GetWidth() const { return m_max.x - m_min.x; }  // Mac 0x1de1d0, Win 0x4061f0
Real32 BoundingBox::GetHeight() const { return m_max.y - m_min.y; } // Mac 0x1de1f0, Win 0x406200
Real32 BoundingBox::GetDepth() const { return m_max.z - m_min.z; }  // Mac 0x1de210, Win 0x406210

void BoundingBox::Translate(const Vector3& delta)                   // Mac 0x1de230, Win 0x406080
{
    m_min += delta;
    m_max += delta;
}

BoundingBox& BoundingBox::operator+=(const Vector3& delta)          // Mac 0x1de280, Win 0x4060e0
{
    Translate(delta);
    return *this;
}

bool BoundingBox::IsZero() const { return m_min.IsZero() && m_max.IsZero(); } // Mac 0x1de2d0, Win 0x4060f0

void BoundingBox::Reset()                                           // Mac 0x1de330, Win 0x406140
{
    m_min = Vector3(FLT_MAX, FLT_MAX, FLT_MAX);
    m_max = Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
}

Vector3 BoundingBox::GetSize() const { return m_max - m_min; }      // Mac 0x1de360, Win 0x406160

Vector3 BoundingBox::GetCenter() const                              // Mac 0x1de3a0, Win 0x406180
{
    return Vector3((Real32)(((EvalReal)m_max.x - m_min.x) * 0.5f + m_min.x),
                   (Real32)(((EvalReal)m_max.y - m_min.y) * 0.5f + m_min.y),
                   (Real32)(((EvalReal)m_max.z - m_min.z) * 0.5f + m_min.z));
}

Real32 BoundingBox::GetVolume() const                               // Mac 0x1de410, Win 0x4061c0
{
    return (Real32)((EvalReal)GetWidth() * GetHeight() * GetDepth());
}

BoundingBox& BoundingBox::operator|=(const BoundingBox& b)          // Mac 0x1de450, Win 0x406220
{
    Union(b);
    return *this;
}

void BoundingBox::Union(const BoundingBox& b)                       // Mac 0x1de4d0
{
    if (b.m_min.x < m_min.x) m_min.x = b.m_min.x;
    if (b.m_min.y < m_min.y) m_min.y = b.m_min.y;
    if (b.m_min.z < m_min.z) m_min.z = b.m_min.z;
    if (m_max.x < b.m_max.x) m_max.x = b.m_max.x;
    if (m_max.y < b.m_max.y) m_max.y = b.m_max.y;
    if (m_max.z < b.m_max.z) m_max.z = b.m_max.z;
}

// ===========================================================================================
// Distances
// ===========================================================================================
Real32 Distance(const Vector2& a, const Vector2& b)                 // Mac 0x1de550, Win 0x408d50
{
    return Sqrt(DistanceSquared(a, b));
}

Real32 DistanceSquared(const Vector2& a, const Vector2& b)          // Mac 0x1de590, Win 0x408d00
{
    Real32 dx = a.x - b.x;
    Real32 dy = a.y - b.y;
    Real32 xx = dx * dx;   // Win 0x408d00: squares rounded, sum not
    Real32 yy = dy * dy;
    return (Real32)((EvalReal)xx + yy);
}

Real32 Distance(const Vector3& a, const Vector3& b)                 // Mac 0x1de640, Win 0x408e10
{
    return Sqrt(a.GetDistanceSquared(b));
}

// ===========================================================================================
// Sphere
// ===========================================================================================
Sphere::Sphere(const Vector3& center, Real32 radius) : m_center(center), m_radius(radius) {} // Mac 0x1de5c0
Sphere::Sphere(Real32 radius) : m_center(), m_radius(radius) {}     // Mac 0x1de5f0, Win 0x406340
Sphere::Sphere(const Sphere& s) : m_center(s.m_center), m_radius(s.m_radius) {} // Mac 0x1de620

Sphere::Sphere(const BoundingBox& box)                              // Mac 0x1de690, Win 0x406360
    : m_center(box.GetCenter())
{
    m_radius = Distance(m_center, box.m_min);
}

Sphere& Sphere::operator=(const Sphere& s)                          // Mac 0x1de730
{
    m_center = s.m_center;
    m_radius = s.m_radius;
    return *this;
}

void Sphere::Set(const BoundingBox& box)                            // Mac 0x1de750, Win 0x4063d0
{
    m_center = box.GetCenter();
    m_radius = Distance(m_center, box.m_min);
}

bool Sphere::Intersects(const Sphere& s) const                      // Mac 0x1de7f0, Win 0x406440
{
    Real32 r = m_radius + s.m_radius;
    return m_center.GetDistanceSquared(s.m_center) <= r * r;
}

// ===========================================================================================
// Cone
// ===========================================================================================
Cone::Cone(const Vector3& apex, const Vector3& axis, Real32 angle)  // Mac 0x1de840, Win 0x406490
    : m_apex(apex), m_axis(axis), m_angle(angle)
{
    m_cos = Cos(angle);
    m_sin = Sin(angle);
    m_cosSquared = m_cos * m_cos;
    m_sinSquared = m_sin * m_sin;
    m_invSin = 1.0f / m_sin;
}

// Eberly, "Intersection of a Sphere and a Cone": move the apex back by r/sin along the axis
// and test the center against that cone, then exclude the region behind the real apex.
bool Cone::Intersects(const Sphere& s) const                        // Mac 0x1de8f0, Win 0x406500
{
    Vector3 diff = s.m_center - m_apex;
    Vector3 d = m_axis * (m_invSin * s.m_radius) + diff;
    Real32 e = m_axis * d;
    if (e <= 0.0f || (EvalReal)e * e < (EvalReal)d.GetLengthSquared() * m_cosSquared)
        return false;
    Real32 lengthSquared = diff.GetLengthSquared();
    e = -(m_axis * diff);
    if (e <= 0.0f)
        return true;
    if ((EvalReal)e * e < (EvalReal)m_sinSquared * lengthSquared)
        return true;
    return lengthSquared <= s.m_radius * s.m_radius;
}

// ===========================================================================================
// Plane
// ===========================================================================================
Plane::Plane() : m_normal(0.0f, 1.0f, 0.0f), m_distance(0.0f) {}    // Mac 0x1dea40, Win 0x4066a0
Plane::Plane(Real32 a, Real32 b, Real32 c, Real32 d) : m_normal(a, b, c), m_distance(d) {} // Mac 0x1dea60

void Plane::SetPoint(const Vector3& point) { m_distance = -(point * m_normal); } // Mac 0x1dea90, Win 0x4067f0

Plane::Plane(const Vector3& normal, const Vector3& point)           // Mac 0x1deae0, Win 0x4066c0
    : m_normal(normal), m_distance(0.0f)
{
    m_distance = -(m_normal * point);
}

Plane::Plane(const Vector3& p0, const Vector3& p1, const Vector3& p2) // Mac 0x1deb40, Win 0x406710
    : m_normal()
{
    m_normal = (p1 - p0).Cross(p2 - p0);
    m_normal.Normalize();
    m_distance = -(m_normal * p0);
}

Plane::Plane(const Plane& p) : m_normal(p.m_normal), m_distance(p.m_distance) {} // Mac 0x1dec60, Win 0x406820

Real32 Plane::GetDistance(const Vector3& point) const               // Mac 0x1dec80, Win 0x40a4c0 (cdecl)
{
    return m_normal * point + m_distance;
}

// ===========================================================================================
// Frustum
// ===========================================================================================
Frustum::Frustum() : m_apex() {}                                    // Mac 0x1decc0, Win 0x406850

Frustum::Frustum(const Vector3& apex, const Vector3& corner0, const Vector3& corner1,
                 const Vector3& corner2, const Vector3& corner3)    // Mac 0x1ded20, Win 0x406890
{
    m_planes[0] = Plane(apex, corner0, corner1);
    m_planes[1] = Plane(apex, corner1, corner2);
    m_planes[2] = Plane(apex, corner2, corner3);
    m_planes[3] = Plane(apex, corner3, corner0);
    m_apex = apex;
}

Frustum::Containment Frustum::Classify(const Sphere& s) const       // Mac 0x1df130, Win 0x4069b0
{
    int inside = 0;
    for (UInt32 i = 0; i < 4; ++i) {
        Real32 distance = m_planes[i].GetDistance(s.m_center);
        if (distance >= 0.0f) {
            if (s.m_radius < distance)
                ++inside;
        } else if (s.m_radius < -distance) {
            return OUTSIDE;
        }
    }
    return inside == 4 ? INSIDE : INTERSECTING;
}

// ===========================================================================================
// Matrix3D
// ===========================================================================================

// Determinants with the rows given element by element. Inlined on Mac (which also shares the
// 2x2 minors); Win 0x406a30 (3x3, cdecl) and 0x406ab0 (4x4, cdecl). (names inferred)
// Win: the 2x2 minors and the 3x3 determinants are rounded to float, the sums of the 4x4
// expansion are not (spilled as doubles).
static Real32 Determinant(Real32 a1, Real32 a2, Real32 a3, Real32 b1, Real32 b2, Real32 b3,
                          Real32 c1, Real32 c2, Real32 c3)
{
    Real32 m1 = (Real32)((EvalReal)b2 * c3 - (EvalReal)b3 * c2);
    Real32 m2 = (Real32)((EvalReal)b1 * c3 - (EvalReal)b3 * c1);
    Real32 m3 = (Real32)((EvalReal)b1 * c2 - (EvalReal)b2 * c1);
    return (Real32)((EvalReal)a1 * m1 - (EvalReal)a2 * m2 + (EvalReal)a3 * m3);
}

static Real32 Determinant(Real32 a1, Real32 a2, Real32 a3, Real32 a4, Real32 b1, Real32 b2,
                          Real32 b3, Real32 b4, Real32 c1, Real32 c2, Real32 c3, Real32 c4,
                          Real32 d1, Real32 d2, Real32 d3, Real32 d4)
{
    return (Real32)((EvalReal)a1 * Determinant(b2, b3, b4, c2, c3, c4, d2, d3, d4) -
                    (EvalReal)a2 * Determinant(b1, b3, b4, c1, c3, c4, d1, d3, d4) +
                    (EvalReal)a3 * Determinant(b1, b2, b4, c1, c2, c4, d1, d2, d4) -
                    (EvalReal)a4 * Determinant(b1, b2, b3, c1, c2, c3, d1, d2, d3));
}

// 2x2 minor a*b - c*d, rounded once. (restoration helper)
static inline Real32 Minor(Real32 a, Real32 b, Real32 c, Real32 d)
{
    return (Real32)((EvalReal)a * b - (EvalReal)c * d);
}

const Matrix3D& Matrix3D::Identity()                                // Mac 0x1df280, Win 0x408510
{
    static const Matrix3D identity(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                                   0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    return identity;
}

Matrix3D::Matrix3D() { *this = Identity(); }                        // Mac 0x1df350, Win 0x406c20
Matrix3D::Matrix3D(const Matrix3D& o) { memcpy(m, o.m, sizeof m); } // Mac 0x1df480, Win 0x406c40

Matrix3D::Matrix3D(Real32 m00, Real32 m01, Real32 m02, Real32 m03,
                   Real32 m04, Real32 m05, Real32 m06, Real32 m07,
                   Real32 m08, Real32 m09, Real32 m10, Real32 m11,
                   Real32 m12, Real32 m13, Real32 m14, Real32 m15)  // Mac 0x1df4e0
{
    m[0] = m00;  m[1] = m01;  m[2] = m02;  m[3] = m03;
    m[4] = m04;  m[5] = m05;  m[6] = m06;  m[7] = m07;
    m[8] = m08;  m[9] = m09;  m[10] = m10; m[11] = m11;
    m[12] = m12; m[13] = m13; m[14] = m14; m[15] = m15;
}

void Matrix3D::Multiply(const Matrix3D& a, const Matrix3D& b)       // Mac 0x1df590, Win 0x407260
{
    for (UInt32 row = 0; row < 4; ++row) {
        for (UInt32 column = 0; column < 4; ++column) {
            const Real32* bc = &b.m[column * 4];
            m[column * 4 + row] = (Real32)((EvalReal)bc[0] * a.m[row] + (EvalReal)bc[1] * a.m[4 + row] +
                                           (EvalReal)bc[2] * a.m[8 + row] + (EvalReal)bc[3] * a.m[12 + row]);
        }
    }
}

Matrix3D::Matrix3D(const Matrix3D& a, const Matrix3D& b) { Multiply(a, b); } // Mac 0x1df6c0, Win 0x406c60

void Matrix3D::SetIdentity() { *this = Identity(); }                // Mac 0x1df6d0, Win 0x406c80

void Matrix3D::SetRotationX(Real32 angle)                           // Mac 0x1df800, Win 0x406ca0
{
    Real32 s = Sin(angle);
    Real32 c = Cos(angle);
    *this = Identity();
    m[5] = c;
    m[6] = s;
    m[9] = -s;
    m[10] = c;
}

void Matrix3D::SetRotationY(Real32 angle)                           // Mac 0x1df990, Win 0x406d00
{
    Real32 s = Sin(angle);
    Real32 c = Cos(angle);
    *this = Identity();
    m[0] = c;
    m[2] = -s;
    m[8] = s;
    m[10] = c;
}

void Matrix3D::SetRotationZ(Real32 angle)                           // Mac 0x1dfb20
{
    Real32 s = Sin(angle);
    Real32 c = Cos(angle);
    *this = Identity();
    m[0] = c;
    m[1] = s;
    m[4] = -s;
    m[5] = c;
}

void Matrix3D::SetEulerAngles(Real32 pitch, Real32 yaw, Real32 roll) // Mac 0x1dfcb0, Win 0x406d60
{
    Real32 sp = Sin(pitch), cp = Cos(pitch);
    Real32 sy = Sin(yaw), cy = Cos(yaw);
    Real32 sr = Sin(roll), cr = Cos(roll);
    Real32 spsy = sp * sy;
    Real32 spcy = sp * cy;
    m[0] = (Real32)((EvalReal)cy * cr + (EvalReal)spsy * sr);
    m[1] = cp * sr;
    m[2] = (Real32)((EvalReal)spcy * sr - (EvalReal)sy * cr);
    m[3] = 0.0f;
    m[4] = (Real32)((EvalReal)spsy * cr - (EvalReal)cy * sr);
    m[5] = cp * cr;
    m[6] = (Real32)((EvalReal)sr * sy + (EvalReal)spcy * cr);
    m[7] = 0.0f;
    m[8] = sy * cp;
    m[9] = -sp;
    m[10] = cp * cy;
    m[11] = 0.0f;
    m[12] = 0.0f;
    m[13] = 0.0f;
    m[14] = 0.0f;
    m[15] = 1.0f;
}

void Matrix3D::GetEulerAngles(Real32& pitch, Real32& yaw, Real32& roll) const // Mac 0x1dfe60, Win 0x406e90
{
    yaw = ATan2(m[8], m[10]);
    roll = ATan2(m[1], m[5]);
    Real32 s = Sin(yaw);
    Real32 c = Cos(yaw);
    pitch = ATan2(-m[9], (Real32)((EvalReal)m[10] * c + (EvalReal)m[8] * s));
}

void Matrix3D::SetRotation(const Vector3& axis, Real32 angle)       // Mac 0x1dff40, Win 0x406f10
{
    Vector3 a(axis);
    a.Normalize();
    Real32 s = Sin(angle);
    Real32 c = Cos(angle);
    Real32 t = 1.0f - c;
    // Win 0x406f10 keeps these on the x87 stack / spills them as doubles (never rounded)
    EvalReal xy = (EvalReal)a.x * a.y * t;
    EvalReal xz = (EvalReal)a.x * a.z * t;
    EvalReal yz = (EvalReal)a.y * a.z * t;
    EvalReal xs = (EvalReal)a.x * s;
    EvalReal ys = (EvalReal)a.y * s;
    EvalReal zs = (EvalReal)a.z * s;
    m[0] = (Real32)((EvalReal)a.x * a.x * t + c);
    m[1] = (Real32)(zs + xy);
    m[2] = (Real32)(xz - ys);
    m[3] = 0.0f;
    m[4] = (Real32)(xy - zs);
    m[5] = (Real32)((EvalReal)a.y * a.y * t + c);
    m[6] = (Real32)(xs + yz);
    m[7] = 0.0f;
    m[8] = (Real32)(ys + xz);
    m[9] = (Real32)(yz - xs);
    m[10] = (Real32)((EvalReal)a.z * a.z * t + c);
    m[11] = 0.0f;
    m[12] = 0.0f;
    m[13] = 0.0f;
    m[14] = 0.0f;
    m[15] = 1.0f;
}

void Matrix3D::SetRotation(const Vector3& axis, Real32 angle, const Vector3& translation,
                           const Vector3& center)                   // Mac 0x1e0120, Win 0x407030
{
    SetRotation(axis, angle);
    Vector3 p = LEGO::Rotate(-center, *this);   // rounded to float, then the two sums one by one
    p = translation + p;
    p = center + p;
    m[12] = p.x;
    m[13] = p.y;
    m[14] = p.z;
}

void Matrix3D::SetTranslation(const Vector3& t)                     // Mac 0x1e0390, Win 0x4080d0
{
    m[12] = t.x;
    m[13] = t.y;
    m[14] = t.z;
}

void Matrix3D::SetScaling(Real32 sx, Real32 sy, Real32 sz)          // Mac 0x1e0440, Win 0x407110
{
    *this = Identity();
    m[0] = sx;
    m[5] = sy;
    m[10] = sz;
}

bool Matrix3D::RemoveScaling()                                      // Mac 0x1e0590, Win 0x407140
{
    Vector3 x(m[0], m[1], m[2]);
    Vector3 y(m[4], m[5], m[6]);
    Vector3 z(m[8], m[9], m[10]);
    x.Normalize();
    y.Normalize();
    z.Normalize();
    if (x.IsAlmostZero() || y.IsAlmostZero() || z.IsAlmostZero())
        return false;
    m[0] = x.x; m[1] = x.y; m[2] = x.z;
    m[4] = y.x; m[5] = y.y; m[6] = y.z;
    m[8] = z.x; m[9] = z.y; m[10] = z.z;
    return true;
}

void Matrix3D::SetTranslationMatrix(const Vector3& t)               // Mac 0x1e07e0, Win 0x407230
{
    *this = Identity();
    SetTranslation(t);
}

// Two paths: affine matrices (last row 0 0 0 1) invert the 3x3 part and transform the
// translation; everything else uses the full cofactor expansion.
bool Matrix3D::SetInverse(const Matrix3D& source)                   // Mac 0x1e0920, Win 0x407310
{
    const Real32* s = source.m;
    Real32 r[16];
    if (s[3] == 0.0f && s[7] == 0.0f && s[11] == 0.0f && s[15] == 1.0f) {
        Real32 det = Determinant(s[0], s[1], s[2], s[4], s[5], s[6], s[8], s[9], s[10]);
        if (Abs(det) < FLT_EPSILON)
            return false;
        Real32 inv = 1.0f / det;
        r[0] = Minor(s[5], s[10], s[6], s[9]) * inv;
        r[4] = Minor(s[6], s[8], s[4], s[10]) * inv;
        r[8] = Minor(s[4], s[9], s[5], s[8]) * inv;
        r[3] = 0.0f;
        r[1] = Minor(s[9], s[2], s[1], s[10]) * inv;
        r[5] = Minor(s[10], s[0], s[2], s[8]) * inv;
        r[9] = Minor(s[8], s[1], s[9], s[0]) * inv;
        r[7] = 0.0f;
        r[2] = Minor(s[1], s[6], s[2], s[5]) * inv;
        r[6] = Minor(s[4], s[2], s[6], s[0]) * inv;
        r[10] = Minor(s[0], s[5], s[1], s[4]) * inv;
        r[11] = 0.0f;
        r[12] = (Real32)-((EvalReal)r[0] * s[12] + (EvalReal)r[4] * s[13] + (EvalReal)r[8] * s[14]);
        r[13] = (Real32)-((EvalReal)r[1] * s[12] + (EvalReal)r[5] * s[13] + (EvalReal)r[9] * s[14]);
        r[14] = (Real32)-((EvalReal)r[2] * s[12] + (EvalReal)r[6] * s[13] + (EvalReal)r[10] * s[14]);
        r[15] = 1.0f;
    } else {
        Real32 det = Determinant(s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7],
                                 s[8], s[9], s[10], s[11], s[12], s[13], s[14], s[15]);
        if (Abs(det) < FLT_EPSILON)
            return false;
        Real32 inv = 1.0f / det;
        r[0] = Determinant(s[5], s[6], s[7], s[9], s[10], s[11], s[13], s[14], s[15]) * inv;
        r[4] = Determinant(s[8], s[10], s[11], s[4], s[6], s[7], s[12], s[14], s[15]) * inv;
        r[8] = Determinant(s[4], s[5], s[7], s[8], s[9], s[11], s[12], s[13], s[15]) * inv;
        r[12] = Determinant(s[8], s[9], s[10], s[4], s[5], s[6], s[12], s[13], s[14]) * inv;
        r[1] = Determinant(s[9], s[10], s[11], s[1], s[2], s[3], s[13], s[14], s[15]) * inv;
        r[5] = Determinant(s[0], s[2], s[3], s[8], s[10], s[11], s[12], s[14], s[15]) * inv;
        r[9] = Determinant(s[8], s[9], s[11], s[0], s[1], s[3], s[12], s[13], s[15]) * inv;
        r[13] = Determinant(s[0], s[1], s[2], s[8], s[9], s[10], s[12], s[13], s[14]) * inv;
        r[2] = Determinant(s[1], s[2], s[3], s[5], s[6], s[7], s[13], s[14], s[15]) * inv;
        r[6] = Determinant(s[4], s[6], s[7], s[0], s[2], s[3], s[12], s[14], s[15]) * inv;
        r[10] = Determinant(s[0], s[1], s[3], s[4], s[5], s[7], s[12], s[13], s[15]) * inv;
        r[14] = Determinant(s[4], s[5], s[6], s[0], s[1], s[2], s[12], s[13], s[14]) * inv;
        r[3] = Determinant(s[5], s[6], s[7], s[1], s[2], s[3], s[9], s[10], s[11]) * inv;
        r[7] = Determinant(s[0], s[2], s[3], s[4], s[6], s[7], s[8], s[10], s[11]) * inv;
        r[11] = Determinant(s[4], s[5], s[7], s[0], s[1], s[3], s[8], s[9], s[11]) * inv;
        r[15] = Determinant(s[0], s[1], s[2], s[4], s[5], s[6], s[8], s[9], s[10]) * inv;
    }
    memcpy(m, r, sizeof m);
    return true;
}

void Matrix3D::SetLookAt(const Vector3& eye, const Vector3& target, const Vector3& up) // Mac 0x1e1400, Win 0x407a90
{
    Vector3 back = eye - target;
    back.Normalize();
    Real32 d = back * up;
    Vector3 newUp(up.x - back.x * d, up.y - back.y * d, up.z - back.z * d);
    newUp.Normalize();
    Vector3 side = newUp.Cross(back);
    m[0] = side.x;
    m[1] = newUp.x;
    m[2] = back.x;
    m[3] = 0.0f;
    m[4] = side.y;
    m[5] = newUp.y;
    m[6] = back.y;
    m[7] = 0.0f;
    m[8] = side.z;
    m[9] = newUp.z;
    m[10] = back.z;
    m[11] = 0.0f;
    m[12] = -(side * eye);
    m[13] = -(newUp * eye);
    m[14] = -(back * eye);
    m[15] = 1.0f;
}

void Matrix3D::SetPerspective(Real32 fovX, Real32 fovY, Real32 zNear, Real32 zFar) // Mac 0x1e1630, Win 0x407c40
{
    Real32 tx = Tan(fovX * 0.5f);
    Real32 ty = Tan(fovY * 0.5f);
    Real32 q = (Real32)(zFar / ((EvalReal)zNear - zFar));
    memset(m, 0, sizeof m);
    m[0] = 1.0f / tx;
    m[5] = 1.0f / ty;
    m[10] = q;
    m[11] = -1.0f;
    m[14] = q * zNear;
}

// `aspect` scales x: height / width.
void Matrix3D::SetPerspectiveFovY(Real32 fovY, Real32 aspect, Real32 zNear, Real32 zFar) // Mac 0x1e1750, Win 0x407cc0
{
    Real32 t = Tan(fovY * 0.5f);
    Real32 q = (Real32)(zFar / ((EvalReal)zNear - zFar));
    memset(m, 0, sizeof m);
    m[0] = aspect * (1.0f / t);
    m[5] = 1.0f / t;
    m[10] = q;
    m[11] = -1.0f;
    m[14] = q * zNear;
}

void Matrix3D::SetOrthographic(const Rect& rect, Real32 zNear, Real32 zFar) // Mac 0x1e1840, Win 0x407d30
{
    memset(m, 0, sizeof m);
    Real32 left = rect.m_min.x, top = rect.m_min.y, right = rect.m_max.x, bottom = rect.m_max.y;
    Real32 width = right - left;
    Real32 height = top - bottom;
    Real32 depth = zFar - zNear;
    m[0] = 2.0f / width;
    m[5] = 2.0f / height;
    m[10] = -2.0f / depth;
    m[12] = (Real32)-(((EvalReal)right + left) / width);
    m[13] = (Real32)-(((EvalReal)top + bottom) / height);
    m[14] = (Real32)-(((EvalReal)zNear + zFar) / depth);
    m[15] = 1.0f;
}

void Matrix3D::RotateX(Real32 angle) { Rotate(angle, 0.0f, 0.0f); } // Mac 0x1e1950, Win 0x407dd0
void Matrix3D::RotateY(Real32 angle) { Rotate(0.0f, angle, 0.0f); } // Mac 0x1e1ac0, Win 0x407e00
void Matrix3D::RotateZ(Real32 angle) { Rotate(0.0f, 0.0f, angle); } // Mac 0x1e1c30, Win 0x407e20

void Matrix3D::Rotate(Real32 angleX, Real32 angleY, Real32 angleZ)  // Win 0x407e40
{
    Real32 sx = Sin(angleX), cx = Cos(angleX);
    Real32 sy = Sin(angleY), cy = Cos(angleY);
    Real32 sz = Sin(angleZ), cz = Cos(angleZ);
    for (UInt32 column = 0; column < 3; ++column) {
        Real32* v = &m[column * 4];
        Real32 x = v[0];
        Real32 y1 = (Real32)((EvalReal)cx * v[1] - (EvalReal)sx * v[2]);
        Real32 z1 = (Real32)((EvalReal)sx * v[1] + (EvalReal)cx * v[2]);
        Real32 x2 = (Real32)((EvalReal)sy * z1 + (EvalReal)cy * x);
        v[0] = (Real32)((EvalReal)cz * x2 - (EvalReal)sz * y1);
        v[1] = (Real32)((EvalReal)sz * x2 + (EvalReal)cz * y1);
        v[2] = (Real32)((EvalReal)sy * x * -1.0f + (EvalReal)cy * z1);
    }
}

Vector3& Matrix3D::GetTranslation() { return *(Vector3*)&m[12]; }   // Mac 0x1e1da0, Win 0x4080f0

void Matrix3D::Multiply(const Matrix3D& o)                          // Mac 0x1e1db0, Win 0x408100
{
    Matrix3D copy(*this);
    Multiply(copy, o);
}

void Matrix3D::Translate(const Vector3& delta)                      // Mac 0x1e1e20, Win 0x4080a0
{
    m[12] += delta.x;
    m[13] += delta.y;
    m[14] += delta.z;
}

Matrix3D& Matrix3D::operator*=(const Matrix3D& o)                   // Mac 0x1e1e60, Win 0x408140
{
    Matrix3D copy(*this);
    Multiply(copy, o);
    return *this;
}

bool Matrix3D::IsIdentity() const                                   // Mac 0x1e1ee0
{
    return memcmp(m, Identity().m, sizeof m) == 0;
}

void Matrix3D::GetAxisAngle(Vector3& axis, Real32& angle) const     // Mac 0x1e1fd0, Win 0x408180
{
    Real32 c = (Real32)(((EvalReal)m[0] + m[5] + m[10] - 1.0f) * 0.5f);
    if (c <= -1.0f)
        c = -1.0f;
    if (c >= 1.0f)
        c = 1.0f;
    angle = ACos(c);
    if (Abs(angle) < EPSILON) {
        angle = 0.0f;
        axis = Vector3(0.0f, 1.0f, 0.0f);
        return;
    }
    if (angle >= PI - EPSILON) {
        // 180 degrees: the axis from the largest diagonal element
        if (m[5] <= m[0]) {
            if (m[0] < m[10]) {
                axis.z = Sqrt((Real32)((EvalReal)m[10] - m[0] - m[5] + 1.0f)) * 0.5f;
                Real32 h = 0.5f / axis.z;
                axis.x = m[2] * h;
                axis.y = h * m[6];
            } else {
                axis.x = Sqrt((Real32)((EvalReal)m[0] - m[5] - m[10] + 1.0f)) * 0.5f;
                Real32 h = 0.5f / axis.x;
                axis.y = m[1] * h;
                axis.z = h * m[2];
            }
        } else if (m[5] < m[10]) {
            axis.z = Sqrt((Real32)((EvalReal)m[10] - m[0] - m[5] + 1.0f)) * 0.5f;
            Real32 h = 0.5f / axis.z;
            axis.x = m[2] * h;
            axis.y = h * m[6];
        } else {
            axis.y = Sqrt((Real32)((EvalReal)m[5] - m[0] - m[10] + 1.0f)) * 0.5f;
            Real32 h = 0.5f / axis.y;
            axis.x = m[1] * h;
            axis.z = h * m[6];
        }
    } else {
        axis.x = m[6] - m[9];
        axis.y = m[8] - m[2];
        axis.z = m[1] - m[4];
    }
    axis.Normalize();
}

Matrix3D::operator Real32*() { return m; }                          // Mac 0x1e2260
Matrix3D::operator const Real32*() const { return m; }              // Mac 0x1e2270

bool Matrix3D::IsLess(const Matrix3D& o, Real32 epsilon) const      // Mac 0x1e2280, Win 0x4083c0
{
    for (UInt32 i = 0; i < 16; ++i) {
        Real32 d = m[i] - o.m[i];
        if (Abs(d) > epsilon)
            return d < 0.0f;
    }
    return false;
}

bool Matrix3D::IsEqual(const Matrix3D& o, Real32 epsilon) const     // Mac 0x1e2460, Win 0x408430
{
    for (UInt32 i = 0; i < 16; ++i) {
        if (Abs(m[i] - o.m[i]) > epsilon)
            return false;
    }
    return true;
}

bool Matrix3D::IsRotationEqual(const Matrix3D& o, Real32 epsilon) const // Mac 0x1e2610, Win 0x408480
{
    for (UInt32 column = 0; column < 3; ++column) {
        for (UInt32 row = 0; row < 3; ++row) {
            UInt32 i = column * 4 + row;
            if (Abs(m[i] - o.m[i]) > epsilon)
                return false;
        }
    }
    return true;
}

bool Matrix3D::operator==(const Matrix3D& o) const { return IsEqual(o, EPSILON); }  // Mac 0x1e2700, Win 0x4084f0
bool Matrix3D::operator!=(const Matrix3D& o) const { return !IsEqual(o, EPSILON); } // Mac 0x1e2720

bool Matrix3D::Solve(Matrix3D& a, Matrix3D& b)                      // Mac 0x1e2750, Win 0x4085d0
{
#if defined(__APPLE__) && !defined(_WIN32)
    int n = 4;
    int pivots[4];
    int info;
    sgesv_(&n, &n, a.m, &n, pivots, b.m, &n, &info);
    return info == 0;
#else
    Matrix3D inverse(Identity());
    if (!inverse.SetInverse(a))
        return false;
    inverse *= b;
    b = inverse;
    return true;
#endif
}

// ===========================================================================================
// Transforms
// ===========================================================================================
Vector3 Rotate(const Vector3& v, const Matrix3D& t)                 // Mac 0x1e03b0, Win 0x4086b0
{
    const Real32* m = t.m;
    return Vector3((Real32)((EvalReal)m[0] * v.x + (EvalReal)m[4] * v.y + (EvalReal)m[8] * v.z),
                   (Real32)((EvalReal)m[1] * v.x + (EvalReal)m[5] * v.y + (EvalReal)m[9] * v.z),
                   (Real32)((EvalReal)m[2] * v.x + (EvalReal)m[6] * v.y + (EvalReal)m[10] * v.z));
}

Vector3 operator*(const Vector3& p, const Matrix3D& t)              // Mac 0x1e27b0, Win 0x408650
{
    const Real32* m = t.m;
    return Vector3((Real32)((EvalReal)m[0] * p.x + (EvalReal)m[4] * p.y + (EvalReal)m[8] * p.z + m[12]),
                   (Real32)((EvalReal)m[1] * p.x + (EvalReal)m[5] * p.y + (EvalReal)m[9] * p.z + m[13]),
                   (Real32)((EvalReal)m[2] * p.x + (EvalReal)m[6] * p.y + (EvalReal)m[10] * p.z + m[14]));
}

Vector3& operator*=(Vector3& p, const Matrix3D& t)                  // Mac 0x1e2850, Win 0x408710
{
    p = p * t;
    return p;
}

void RotateInPlace(Vector3& v, const Matrix3D& t)                   // Mac 0x1e28f0, Win 0x408760
{
    v = Rotate(v, t);
}

BoundingBox operator*(const BoundingBox& b, const Matrix3D& t)      // Mac 0x1e2980, Win 0x4087d0
{
    BoundingBox r(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    const Vector3& lo = b.m_min;
    const Vector3& hi = b.m_max;
    r.m_max = lo * t;
    r.m_min = r.m_max;
    r.Add(hi * t);
    r.Add(Vector3(lo.x, lo.y, hi.z) * t);
    r.Add(Vector3(lo.x, hi.y, lo.z) * t);
    r.Add(Vector3(hi.x, lo.y, lo.z) * t);
    r.Add(Vector3(lo.x, hi.y, hi.z) * t);
    r.Add(Vector3(hi.x, hi.y, lo.z) * t);
    r.Add(Vector3(hi.x, lo.y, hi.z) * t);
    return r;
}

BoundingBox Rotate(const BoundingBox& b, const Matrix3D& t)         // Mac 0x1e2fd0, Win 0x408980
{
    BoundingBox r(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    const Vector3& lo = b.m_min;
    const Vector3& hi = b.m_max;
    r.m_max = Rotate(lo, t);
    r.m_min = r.m_max;
    r.Add(Rotate(hi, t));
    r.Add(Rotate(Vector3(lo.x, lo.y, hi.z), t));
    r.Add(Rotate(Vector3(lo.x, hi.y, lo.z), t));
    r.Add(Rotate(Vector3(hi.x, lo.y, lo.z), t));
    r.Add(Rotate(Vector3(lo.x, hi.y, hi.z), t));
    r.Add(Rotate(Vector3(hi.x, hi.y, lo.z), t));
    r.Add(Rotate(Vector3(hi.x, lo.y, hi.z), t));
    return r;
}

Real32 TranslationDistanceSquared(const Matrix3D& a, const Matrix3D& b) // Mac 0x1e3570, Win 0x408e40
{
    return a.GetTranslation().GetDistanceSquared(b.GetTranslation());
}

// ===========================================================================================
// Distance / intersection functions
// ===========================================================================================
Real32 PointRayDistanceSquared(const Vector3& origin, const Vector3& direction,
                               const Vector3& point)                // Mac 0x1e35c0, Win 0x408eb0 (cdecl)
{
    Vector3 w = point - origin;
    Real32 t = w * direction;
    if (t >= 0.0f)
        return point.GetDistanceSquared(Vector3(origin.x + direction.x * t, origin.y + direction.y * t,
                                                origin.z + direction.z * t));
    return point.GetDistanceSquared(origin);
}

Real32 PointLineDistance(const Vector3& origin, const Vector3& direction,
                         const Vector3& point)                      // Mac 0x1e36d0, Win 0x408ff0
{
    return Distance(point, ClosestPointOnLine(origin, direction, point));
}

Vector3 ClosestPointOnSegment(const Vector3& start, const Vector3& end, const Vector3& point) // Mac 0x1e3790, Win 0x409020
{
    Vector3 d = end - start;
    Real32 length = d.GetLength();
    if (length >= EPSILON) {
        Vector3 n(d.x / length, d.y / length, d.z / length);
        Real32 t = (point - start) * n;
        if (t < 0.0f)
            return start;
        if (length < t)
            return end;
        return Vector3(start.x + n.x * t, start.y + n.y * t, start.z + n.z * t);
    }
    return start;
}

Real32 PointSegmentDistanceSquared(const Vector3& start, const Vector3& end,
                                   const Vector3& point)            // Mac 0x1e38e0
{
    return point.GetDistanceSquared(ClosestPointOnSegment(start, end, point));
}

bool ClosestPointsOnLines(const Vector3& a0, const Vector3& a1, const Vector3& b0,
                          const Vector3& b1, Vector3& pointA, Vector3& pointB,
                          Real32& s, Real32& t)                     // Mac 0x1e3a60, Win 0x409190
{
    Vector3 w, directionA, directionB;
    if (!LineLineParameters(a0, a1, b0, b1, s, t, directionA, directionB, w))
        return false;
    pointA = Vector3(directionA.x * s + a0.x, directionA.y * s + a0.y, directionA.z * s + a0.z);
    pointB = Vector3(directionB.x * t + b0.x, directionB.y * t + b0.y, directionB.z * t + b0.z);
    return true;
}

bool LineLineParameters(const Vector3& a0, const Vector3& a1, const Vector3& b0,
                        const Vector3& b1, Real32& s, Real32& t, Vector3& u, Vector3& v,
                        Vector3& w)                                 // Mac 0x1e3b60, Win 0x4092d0
{
    u = a1 - a0;
    v = b1 - b0;
    w = a0 - b0;
    bool vZero = v.IsAlmostZero();
    bool uZero = u.IsAlmostZero();
    if (vZero) {
        if (uZero) {
            t = 0.0f;
            s = 0.0f;
        } else {
            Real32 inv = 1.0f / u.GetLength();
            Vector3 n = u * inv;
            s = (n * (b0 - a0)) * inv;
            t = 0.0f;
        }
        return true;
    }
    if (uZero) {
        Real32 inv = 1.0f / v.GetLength();
        Vector3 n = v * inv;
        s = 0.0f;
        t = (n * w) * inv;
        return true;
    }
    Real32 a = u * u;
    Real32 b = u * v;
    Real32 c = v * v;
    Real32 denominator = (Real32)((EvalReal)a * c - (EvalReal)b * b);
    if (denominator < EPSILON)
        return false;
    Real32 d = u * w;
    Real32 e = v * w;
    Real32 numerator = (Real32)((EvalReal)e * b - (EvalReal)d * c);
    s = numerator / denominator;
    t = (Real32)(((EvalReal)s * b + e) / c);
    return true;
}

bool SegmentSegmentParameters(const Vector3& originA, const Vector3& u, const Vector3& originB,
                              const Vector3& v, Real32& s, Real32& t, Vector3& w) // Mac 0x1e3ed0, Win 0x409650
{
    w = originA - originB;
    bool vZero = v.IsAlmostZero();
    bool uZero = u.IsAlmostZero();
    if (vZero) {
        if (uZero) {
            t = 0.0f;
            s = 0.0f;
        } else {
            Real32 inv = 1.0f / u.GetLength();
            Vector3 n = u * inv;
            Real32 p = (n * (originB - originA)) * inv;
            s = p < 0.0f ? 0.0f : Min(p, 1.0f);
            t = 0.0f;
        }
        return true;
    }
    if (uZero) {
        Real32 inv = 1.0f / v.GetLength();
        Vector3 n = v * inv;
        s = 0.0f;
        Real32 p = (n * w) * inv;
        t = p < 0.0f ? 0.0f : Min(p, 1.0f);
        return true;
    }
    // Dan Sunday, "Distance between Lines and Segments"
    Real32 a = u * u;
    Real32 b = u * v;
    Real32 c = v * v;
    Real32 d = u * w;
    Real32 e = v * w;
    Real32 denominator = (Real32)((EvalReal)a * c - (EvalReal)b * b);
    if (denominator < EPSILON)
        return false;
    Real32 sN = (Real32)((EvalReal)e * b - (EvalReal)d * c);
    Real32 tN = (Real32)((EvalReal)e * a - (EvalReal)d * b);
    Real32 sD = denominator;
    Real32 tD = denominator;
    if (sN < 0.0f) {
        sN = 0.0f;
        tN = e;
        tD = c;
    } else if (sN > denominator) {
        sN = denominator;
        tN = e + b;
        tD = c;
    }
    if (tN < 0.0f) {
        tN = 0.0f;
        Real32 nd = -d;
        if (nd < 0.0f) {
            sN = 0.0f;
        } else if (nd > a) {
            sN = sD;
        } else {
            sN = nd;
            sD = a;
        }
    } else if (tD < tN) {
        tN = tD;
        // Win 0x409a98: b - d stays on the x87 stack for both comparisons
        EvalReal bd = (EvalReal)b - d;
        if (bd < 0.0f) {
            sN = 0.0f;
        } else if (bd > a) {
            sN = sD;
        } else {
            sN = (Real32)bd;
            sD = a;
        }
    }
    s = Abs(sN) < EPSILON ? 0.0f : sN / sD;
    t = Abs(tN) < EPSILON ? 0.0f : tN / tD;
    return true;
}

void SegmentSegmentClosest(const Vector3& originA, const Vector3& directionA, Real32 lengthA,
                           const Vector3& originB, const Vector3& directionB, Real32 lengthB,
                           Real32& distanceA, Real32& distanceB)    // Mac 0x1e42a0, Win 0x409b60
{
    Vector3 w;
    Vector3 u = directionA * lengthA;
    Vector3 v = directionB * lengthB;
    if (SegmentSegmentParameters(originA, u, originB, v, distanceA, distanceB, w)) {
        distanceA = lengthA * distanceA;
        distanceB = lengthB * distanceB;
        return;
    }
    // parallel: project B's origin onto A, then that point onto B
    Real32 p = (originB - originA) * directionA;
    p = p < 0.0f ? 0.0f : Min(p, lengthA);
    distanceA = p;
    Vector3 q(p * directionA.x + originA.x, directionA.y * p + originA.y, p * directionA.z + originA.z);
    Real32 r = (q - originB) * directionB;
    distanceB = r < 0.0f ? 0.0f : Min(r, lengthB);
}

void SegmentSegmentClosest(const Vector3& a0, const Vector3& a1, const Vector3& b0,
                           const Vector3& b1, Real32& s, Real32& t, Vector3& u, Vector3& v,
                           Vector3& w)                              // Mac 0x1e4430, Win 0x409d50
{
    u = a1 - a0;
    v = b1 - b0;
    if (SegmentSegmentParameters(a0, u, b0, v, s, t, w))
        return;
    // parallel
    Real32 lengthA = u.GetLength();
    Vector3 nA(u.x / lengthA, u.y / lengthA, u.z / lengthA);
    Real32 p = ((b0 - a0) * nA) / lengthA;
    s = p < 0.0f ? 0.0f : Min(p, 1.0f);
    Real32 lengthB = v.GetLength();
    Vector3 nB(v.x / lengthB, v.y / lengthB, v.z / lengthB);
    Vector3 q(s * u.x + a0.x, u.y * s + a0.y, s * u.z + a0.z);
    Real32 r = ((q - b0) * nB) / lengthB;
    t = r < 0.0f ? 0.0f : Min(r, 1.0f);
}

bool PlanePlaneIntersect(const Plane& pa, const Plane& pb, Vector3& point, Vector3& direction) // Mac 0x1e4610, Win 0x40a010
{
    const Vector3& na = pa.m_normal;
    const Vector3& nb = pb.m_normal;
    Vector3 c = na.Cross(nb);
    // absolute values with a tolerance: components above -EPSILON keep their sign (the
    // sums with EPSILON are compared unrounded on Win)
    Real32 ax = 0.0f < (EvalReal)c.x + EPSILON ? c.x : -c.x;
    Real32 ay = 0.0f < (EvalReal)c.y + EPSILON ? c.y : -c.y;
    Real32 az = 0.0f < (EvalReal)c.z + EPSILON ? c.z : -c.z;
    if ((EvalReal)ax + ay + az < EPSILON)
        return false;
    int axis;
    if ((EvalReal)ax + EPSILON <= ay)
        axis = (EvalReal)ay + EPSILON <= az ? 3 : 2;
    else
        axis = (EvalReal)ax + EPSILON <= az ? 3 : 1;
    Real32 da = pa.m_distance;
    Real32 db = pb.m_distance;
    switch (axis) {
    case 3:
        point = Vector3((Real32)(((EvalReal)na.y * db - (EvalReal)nb.y * da) / c.z),
                        (Real32)(((EvalReal)nb.x * da - (EvalReal)na.x * db) / c.z), 0.0f);
        break;
    case 2:
        point = Vector3((Real32)(((EvalReal)nb.z * da - (EvalReal)na.z * db) / c.y), 0.0f,
                        (Real32)(((EvalReal)na.x * db - (EvalReal)nb.x * da) / c.y));
        break;
    default:
        point = Vector3(0.0f, (Real32)(((EvalReal)na.z * db - (EvalReal)nb.z * da) / c.x),
                        (Real32)(((EvalReal)nb.y * da - (EvalReal)na.y * db) / c.x));
        break;
    }
    c.Normalize();
    direction = c;
    return true;
}

Real32 RaySegmentDistanceSquared(const Vector3& origin, const Vector3& direction,
                                 const Vector3& start, const Vector3& end) // Mac 0x1e48b0, Win 0x40a270
{
    Vector3 p, q;
    Real32 s = 0.0f, t = 0.0f;
    if (ClosestPointsOnLines(origin, origin + direction, start, end, p, q, s, t)) {
        if (s < 0.0f)
            p = origin;
        if (t >= 0.0f) {
            if (t > 1.0f)
                q = end;
        } else {
            q = start;
        }
        return p.GetDistanceSquared(q);
    }
    // parallel
    Real32 t0 = (start - origin) * direction;
    if (t0 >= 0.0f)
        return start.GetDistanceSquared(Vector3(origin.x + direction.x * t0, origin.y + direction.y * t0,
                                                origin.z + direction.z * t0));
    Real32 t1 = (end - origin) * direction;
    if (t1 < 0.0f)
        return origin.GetDistanceSquared(t1 <= t0 ? start : end);
    return end.GetDistanceSquared(Vector3(origin.x + direction.x * t1, origin.y + direction.y * t1,
                                          origin.z + direction.z * t1));
}

Vector3 ProjectOntoPlane(const Plane& plane, const Vector3& point)  // Mac 0x1e4be0, Win 0x40a4f0
{
    Real32 d = point * plane.m_normal + plane.m_distance;
    return Vector3(point.x - plane.m_normal.x * d, point.y - plane.m_normal.y * d,
                   point.z - d * plane.m_normal.z);
}

// NOTE: when the ray is parallel to the plane, `t` receives the (near zero) denominator.
bool RayPlaneIntersect(const Vector3& origin, const Vector3& direction, const Plane& plane,
                       Real32& t)                                   // Mac 0x1e4c60, Win 0x40a560
{
    Real32 denominator = plane.m_normal * direction;
    t = denominator;
    if (Abs(denominator) < EPSILON)
        return false;
    t = (Real32)-(((EvalReal)(plane.m_normal * origin) + plane.m_distance) / denominator);
    return t >= 0.0f;
}

bool RayPlaneIntersect(const Vector3& origin, const Vector3& direction, const Plane& plane,
                       Vector3& point)                              // Mac 0x1e4d00, Win 0x40a5f0
{
    Real32 t;
    if (!RayPlaneIntersect(origin, direction, plane, t))
        return false;
    point = Vector3(direction.x * t + origin.x, direction.y * t + origin.y, direction.z * t + origin.z);
    return true;
}

bool RayPlaneIntersectPoints(const Vector3& p0, const Vector3& p1, const Plane& plane,
                             Vector3& point)                        // Mac 0x1e4df0
{
    return RayPlaneIntersect(p0, p1 - p0, plane, point);
}

bool LinePlaneIntersect(const Vector3& origin, const Vector3& direction, const Plane& plane,
                        Vector3& point)                             // Mac 0x1e4f00, Win 0x40a680
{
    Real32 denominator = plane.m_normal * direction;
    if (Abs(denominator) < EPSILON)
        return false;
    Real32 t = (Real32)-(((EvalReal)(plane.m_normal * origin) + plane.m_distance) / denominator);
    point = Vector3(direction.x * t + origin.x, direction.y * t + origin.y, direction.z * t + origin.z);
    return true;
}

bool LinePlaneIntersectPoints(const Vector3& p0, const Vector3& p1, const Plane& plane,
                              Vector3& point)                       // Mac 0x1e4fe0, Win 0x40a770
{
    return LinePlaneIntersect(p0, p1 - p0, plane, point);
}

bool RaySphereIntersect(const Vector3& origin, const Vector3& direction, const Vector3& center,
                        Real32 radius, Vector3* point)              // Mac 0x1e50e0, Win 0x40a7b0
{
    Vector3 l = center - origin;
    Real32 tca = direction * l;
    Real32 d2 = l.GetLengthSquared();
    Real32 r2 = radius * radius;
    if (!(d2 <= r2 || tca >= 0.0f))
        return false;   // outside and pointing away
    Real32 m2 = d2 - tca * tca;
    if (!(m2 <= r2))
        return false;
    if (point) {
        Real32 thc = Sqrt(r2 - m2);
        Real32 t = tca + (r2 < d2 ? -thc : thc);
        *point = Vector3((Real32)((EvalReal)direction.x * t + origin.x),
                         (Real32)((EvalReal)direction.y * t + origin.y),
                         (Real32)((EvalReal)t * direction.z + origin.z));
    }
    return true;
}

// Picks the hit of a two-root set (Mac 0x1e5a11/0x1e595e/0x1e5a39): the positive one if the
// signs differ, the nearer one if both are positive, none otherwise (a zero root counts as
// "not in front").
static bool SelectRoot(Real32 t0, Real32 t1, Real32& t)
{
    if (t0 > 0.0f && t1 < 0.0f) {
        t = t0;
        return true;
    }
    if (t0 < 0.0f && t1 > 0.0f) {
        t = t1;
        return true;
    }
    if (t0 > 0.0f && t1 > 0.0f) {
        t = t1 <= t0 ? t1 : t0;
        return true;
    }
    return false;
}

// x*x + y*y with the squares rounded to float and the sum not (Win 0x40a930 compares and
// subtracts it on the x87 stack). (restoration helper)
static EvalReal SquaredLength2(Real32 x, Real32 y)
{
    Real32 xx = x * x;
    Real32 yy = y * y;
    return (EvalReal)xx + yy;
}

// Wall terms of the ray/cylinder quadratic: a1 = p.d, a2 = |d|^2, discriminant a1^2 - c*a2
// with c = |p|^2 - r^2 (2D, in the cylinder frame). (restoration helper)
static Real32 WallDiscriminant(const Vector3& p, const Vector3& d, Real32 radiusSquared,
                               Real32& a1, Real32& a2)
{
    a1 = (Real32)((EvalReal)p.y * d.y + (EvalReal)p.x * d.x);
    a2 = (Real32)SquaredLength2(d.x, d.y);
    Real32 c = (Real32)(SquaredLength2(p.x, p.y) - radiusSquared);
    return (Real32)((EvalReal)a1 * a1 - (EvalReal)c * a2);
}

// Adapted from Wild Magic's FindIntersection(Line3, Cylinder3) (the iQuantity assert), with
// the ray reduced to its nearest hit in front of the origin.
bool RayCylinderIntersect(const Vector3& origin, const Vector3& direction, const Vector3& center,
                          const Vector3& axis, Real32 halfHeight, Real32 radius,
                          Vector3* point)                           // Mac 0x1e5200, Win 0x40a930
{
    // cylinder frame U, V, W (W = axis)
    Vector3 w(axis);
    w.Normalize();
    Vector3 u = axis.GetPerpendicular();
    u.Normalize();
    Vector3 v = w.Cross(u);
    v.Normalize();

    Vector3 diff = origin - center;
    Vector3 p(diff * u, diff * v, diff * w);
    Real32 dz = w * direction;
    Real32 radiusSquared = radius * radius;
    Real32 t;

    if (Abs(Abs(dz) - 1.0f) < EPSILON) {
        // parallel to the axis: hit an end disk
        if (SquaredLength2(p.x, p.y) > radiusSquared)
            return false;
        if (!point)
            return false;   // NOTE: original quirk, no hit reported without an output point
        t = (dz <= 0.0f ? p.z : -p.z) - halfHeight;
        if (t > 0.0f) {
            *point = Vector3(origin.x + direction.x * t, origin.y + direction.y * t,
                             origin.z + direction.z * t);
            return true;
        }
        return false;
    }

    Vector3 d(u * direction, v * direction, dz);
    if (Abs(Abs(dz)) < EPSILON) {
        // perpendicular to the axis
        if (Abs(p.z) > halfHeight)
            return false;
        Real32 a1, a2;
        Real32 discriminant = WallDiscriminant(p, d, radiusSquared, a1, a2);
        if (discriminant < 0.0f)
            return false;
        if (discriminant > 0.0f) {
            Real32 root = Sqrt(discriminant);
            if (!SelectRoot((Real32)((-(EvalReal)a1 - root) / a2), (Real32)(((EvalReal)root - a1) / a2), t))
                return false;
        } else {
            if (a1 > 0.0f)
                return false;
            t = -a1 / a2;
        }
        if (point)
            *point = Vector3(origin.x + direction.x * t, origin.y + direction.y * t,
                             origin.z + direction.z * t);
        return true;
    }

    // general case: end disks first, then the wall
    Real32 roots[2];
    UInt32 quantity = 0;
    Real32 t0 = (Real32)((-(EvalReal)halfHeight - p.z) / dz);
    Real32 x = (Real32)((EvalReal)t0 * d.x + p.x), y = (Real32)((EvalReal)t0 * d.y + p.y);
    if (SquaredLength2(x, y) <= radiusSquared)
        roots[quantity++] = t0;
    Real32 t1 = (Real32)(((EvalReal)halfHeight - p.z) / dz);
    x = (Real32)((EvalReal)t1 * d.x + p.x);
    y = (Real32)((EvalReal)t1 * d.y + p.y);
    if (SquaredLength2(x, y) <= radiusSquared)
        roots[quantity++] = t1;

    if (quantity < 2) {
        Real32 a1, a2;
        Real32 discriminant = WallDiscriminant(p, d, radiusSquared, a1, a2);
        if (discriminant < 0.0f) {
#if !defined(_WIN32)
            assert(quantity == 0);   // "iQuantity == 0", Geometry.cpp line 0xb24 (Mac only)
#endif
            return false;
        }
        Real32 lo = t1 < t0 ? t1 : t0;
        Real32 hi = t1 < t0 ? t0 : t1;
        if (discriminant > 0.0f) {
            Real32 root = Sqrt(discriminant);
            Real32 value = (Real32)((-(EvalReal)a1 - root) / a2);
            if (lo <= value && value <= hi)
                roots[quantity++] = value;
            if (quantity < 2) {
                value = (Real32)(((EvalReal)root - a1) / a2);
                if (lo <= value && value <= hi)
                    roots[quantity++] = value;
            }
        } else {
            Real32 value = -a1 / a2;
            if (lo <= value && value <= hi)
                roots[quantity++] = value;
        }
    }

    if (quantity == 2) {
        if (!SelectRoot(roots[0], roots[1], t))
            return false;
    } else if (quantity == 1 && roots[0] >= 0.0f) {
        t = roots[0];
    } else {
        return false;
    }
    if (point)
        *point = Vector3(direction.x * t + origin.x, direction.y * t + origin.y,
                         direction.z * t + origin.z);
    return true;
}

bool RayTriangleIntersect(const Vector3& origin, const Vector3& direction, const Vector3& v0,
                          const Vector3& v1, const Vector3& v2, Real32& t, Real32& u,
                          Real32& v)                                // Mac 0x1e5ad0, Win 0x40b260
{
    Vector3 edge1 = v1 - v0;
    Vector3 edge2 = v2 - v0;
    Vector3 p = direction.Cross(edge2);
    Real32 det = edge1 * p;
    if (!(det >= EPSILON))
        return false;
    Vector3 s = origin - v0;
    u = s * p;
    if (!(u >= 0.0f && u <= det))
        return false;
    Vector3 q = s.Cross(edge1);
    v = direction * q;
    if (!(v >= 0.0f && v + u <= det))
        return false;
    t = (edge2 * q) / det;
    u /= det;
    v /= det;
    return true;
}

} // namespace LEGO
