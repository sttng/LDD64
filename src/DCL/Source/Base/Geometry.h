// LEGO Digital Designer — restored source
// Geometry primitives (vectors, rects, boxes, spheres, cones, planes, frustums, 4x4 matrices)
// and the distance / intersection functions. Mac TU 127 (0x1dc3e0-0x1e5d2f; 221 functions, bounds
// from the analysis), Win 0x404d90-0x40b480 (many
// members inlined), math helpers 0x404aa0-0x404cd0 (MathUtil.h).
// Names from RTTI: Vector2, Vector3, Matrix3D; the rest inferred. All of Geometry.cpp is
// out-of-line on Mac; the inline members here exist only as Win COMDAT copies.
// Conventions: Matrix3D is column-major (m[column * 4 + row], translation m[12..14]),
// p' = M * p; angles in radians; EPSILON (1e-5) for approximate comparisons.
#pragma once
#include "LEGO/Types.h"
#include "MathUtil.h"

namespace LEGO {

class Matrix3D;

// ===========================================================================================
// Vector2 — 8 bytes {x +0, y +4}.                                     Mac 0x1dc3e0-0x1dc730
// ===========================================================================================
class Vector2 {
public:
    Vector2();                                              // Mac 0x1dc3e0, Win 0x404d90
    Vector2(Real32 x, Real32 y);                            // Mac 0x1dc400, Win 0x404da0
    Vector2(const Vector2& v);                              // Mac 0x1dc420, Win 0x404dc0

    Real32 GetArea() const;                                 // Mac 0x1dc440, Win 0x404de0 x*y (name inferred)
    bool   IsZero() const;                                  // Mac 0x1dc460, Win 0x404df0 (exact)
    void   Set(Real32 x, Real32 y);                         // Mac 0x1dc490 (name inferred)
    Vector2& operator=(const Vector2& v);                   // Mac 0x1dc4b0, Win 0x404e10

    Vector2& operator+=(const Vector2& v);                  // Mac 0x1dc4d0
    Vector2  operator+(Real32 s) const;                     // Mac 0x1dc500 (adds s to both)
    Vector2  operator+(const Vector2& v) const;             // Mac 0x1dc530, Win 0x404e30
    Vector2& operator-=(const Vector2& v);                  // Mac 0x1dc560
    Vector2  operator-(const Vector2& v) const;             // Mac 0x1dc590, Win 0x404e50
    void     Scale(Real32 sx, Real32 sy);                   // Mac 0x1dc5c0, Win 0x404e70 (name inferred)
    Vector2& operator*=(const Vector2& v);                  // Mac 0x1dc5f0 (component-wise)
    Vector2  operator*(Real32 s) const;                     // Mac 0x1dc620
    void     Divide(Real32 sx, Real32 sy);                  // Mac 0x1dc650 (name inferred)

    // |dx| <= epsilon && |dy| <= epsilon. Inlined on Mac.            Win 0x404e90 (ESI/EDI)
    bool IsEqual(const Vector2& v, Real32 epsilon) const
    {
        return Abs(x - v.x) <= epsilon && Abs(y - v.y) <= epsilon;
    }
    bool operator==(const Vector2& v) const;                // Mac 0x1dc680 (EPSILON)
    bool operator!=(const Vector2& v) const;                // Mac 0x1dc6d0, Win 0x404f00

    operator Real32*();                                     // Mac 0x1dc720, Win 0x404f30
    operator const Real32*() const;                         // Mac 0x1dc730

    Real32 x;   // +0x00
    Real32 y;   // +0x04
};

// ===========================================================================================
// IntVector2 — 8 bytes {x, y} Int32 (planar connection grids). (name inferred)
// ===========================================================================================
class IntVector2 {
public:
    IntVector2();                                           // Mac 0x1dc740, Win 0x404f40
    IntVector2(Int32 x, Int32 y);                           // Mac 0x1dc760, Win 0x404f50
    IntVector2(const IntVector2& v);                        // Mac 0x1dc780
    IntVector2& operator+=(const IntVector2& v);            // Mac 0x1dc7a0
    IntVector2  operator+(const IntVector2& v) const;       // Mac 0x1dc7c0
    IntVector2  operator*(Int32 s) const;                   // Mac 0x1dc7f0

    Int32 x;    // +0x00
    Int32 y;    // +0x04
};

// ===========================================================================================
// UIntVector2 — 8 bytes {x, y} UInt32 (bitmap / texture sizes). Only construction exists;
// the signedness is a guess. (name inferred)
// ===========================================================================================
class UIntVector2 {
public:
    UIntVector2();                                          // Mac 0x1dc810
    UIntVector2(UInt32 x, UInt32 y);                        // Mac 0x1dc830
    UIntVector2(const UIntVector2& v);                      // Mac 0x1dc850
    void Set(UInt32 x, UInt32 y);                           // Mac 0x1dc870 (name inferred)

    UInt32 x;   // +0x00
    UInt32 y;   // +0x04
};

// ===========================================================================================
// Vector3 — 12 bytes {x +0, y +4, z +8}.                              Mac 0x1dc890-0x1dd700
// ===========================================================================================
class Vector3 {
public:
    Vector3();                                              // Mac 0x1dc890, Win 0x404f70
    Vector3(Real32 x, Real32 y, Real32 z);                  // Mac 0x1dc8b0, Win 0x404f80
    Vector3(const Vector3& v);                              // Mac 0x1dc8e0, Win 0x404fa0

    Real32 GetLength() const;                               // Mac 0x1dc900, Win 0x404fc0
    Real32 GetLengthSquared() const;                        // Mac 0x1dc940, Win 0x404ff0
    bool   IsZero() const;                                  // Mac 0x1dc980 (exact)
    bool   IsAlmostZero() const;                            // Mac 0x1dc9c0, Win 0x405020 (|c| < EPSILON)
    void   Clear();                                         // Mac 0x1dca10, Win 0x405090
    void   SetLength(Real32 length);                        // Mac 0x1dca30, Win 0x4050a0 (no-op for 0)
    void   Normalize();                                     // Mac 0x1dcaa0, Win 0x405120

    Real32  Dot(const Vector3& v) const;                    // Mac 0x1dcb10, Win 0x405130
    Real32  operator*(const Vector3& v) const;              // Mac 0x1dcb50, Win 0x405150 (dot)
    Vector3 Cross(const Vector3& v) const;                  // Mac 0x1dcb90, Win 0x405180 (this x v)
    Vector3 operator^(const Vector3& v) const;              // Mac 0x1dcc00, Win 0x4051e0 (cross)
    // Unit normal of the triangle a, b, c: normalize((a - b) x (a - c)).
    void    SetNormal(const Vector3& a, const Vector3& b, const Vector3& c); // Mac 0x1dcc70, Win 0x4051f0

    Vector3  operator-(const Vector3& v) const;             // Mac 0x1dcd50, Win 0x4053b0
    void     Set(const Vector3& v);                         // Mac 0x1dcd90 (name inferred)
    Vector3& operator=(const Vector3& v);                   // Mac 0x1dcdb0, Win 0x4052b0
    Vector3& operator+=(const Vector3& v);                  // Mac 0x1dcdd0, Win 0x4052e0
    Vector3  operator+(Real32 s) const;                     // Mac 0x1dce10, Win 0x405320 (adds s to all)
    Vector3  operator+(const Vector3& v) const;             // Mac 0x1dce50, Win 0x405340
    void     Subtract(const Vector3& v);                    // Mac 0x1dce90 (name inferred)
    Vector3& operator-=(const Vector3& v);                  // Mac 0x1dced0, Win 0x405370
    Vector3  operator-() const;                             // Mac 0x1dcf10, Win 0x4053e0 (sign flip)
    void     Scale(Real32 sx, Real32 sy, Real32 sz);        // Mac 0x1dcf50, Win 0x405400 (name inferred)
    Vector3& operator*=(Real32 s);                          // Mac 0x1dcf90, Win 0x405430
    Vector3  operator*(Real32 s) const;                     // Mac 0x1dcfd0, Win 0x405450
    Vector3& operator/=(Real32 s);                          // Mac 0x1dd010
    void     Divide(Real32 s);                              // Mac 0x1dd050 (name inferred)
    Vector3  operator/(Real32 s) const;                     // Mac 0x1dd090, Win 0x405480

    // this = a point of the line (origin this, unit direction): distance of `point` from the
    // line, squared, below `epsilon`? (name inferred)
    bool IsOnLine(const Vector3& direction, const Vector3& point, Real32 epsilon) const; // Mac 0x1dd0d0, Win 0x4054a0
    Real32 GetDistanceSquared(const Vector3& v) const;      // Mac 0x1dd190 (name inferred)
    // Same line: directions equal or opposite within epsilon and `origin` on this line.
    bool IsSameLine(const Vector3& direction, const Vector3& origin,
                    const Vector3& otherDirection, Real32 epsilon) const; // Mac 0x1dd1e0, Win 0x405550 (name inferred)
    bool IsEqual(const Vector3& v, Real32 epsilon) const;   // Mac 0x1dd350, Win 0x4056a0 (<= epsilon)
    // A vector perpendicular to this one: axis x this, with the axis of the smallest
    // component (not normalized).
    Vector3 GetPerpendicular() const;                       // Mac 0x1dd3b0, Win 0x4055d0 (name inferred)
    bool operator==(const Vector3& v) const;                // Mac 0x1dd4a0 (EPSILON)
    bool operator!=(const Vector3& v) const;                // Mac 0x1dd510
    // Lexicographic with EPSILON tolerance per component.
    bool operator<(const Vector3& v) const;                 // Mac 0x1dd570, Win 0x405730

    operator Real32*();                                     // Mac 0x1dd5f0
    operator const Real32*() const;                         // Mac 0x1dd600

    Real32 GetPitch() const;                                // Mac 0x1dd610, Win 0x4057d0 asin(-y/|v|) (name inferred)
    Real32 GetYaw() const;                                  // Mac 0x1dd670, Win 0x405830 atan2(x, z) (name inferred)

    Real32 x;   // +0x00
    Real32 y;   // +0x04
    Real32 z;   // +0x08
};

// ===========================================================================================
// Rect — 16 bytes, two corners {min +0, max +8} (min/max are not enforced; Normalize sorts
// them). (name inferred)                                             Mac 0x1dd710-0x1ddfdf
// ===========================================================================================
class Rect {
public:
    Rect(Real32 left, Real32 top, Real32 right, Real32 bottom);         // Mac 0x1dd710, Win 0x4058e0
    Rect(const Vector2& position, Real32 width, Real32 height);         // Mac 0x1dd740, Win 0x405900
    Rect(const Vector2& position, const Vector2& size);                 // Mac 0x1dd780, Win 0x405930
    explicit Rect(const Vector2& size);                                 // Mac 0x1dd7c0, Win 0x405960
    Rect(const Rect& r);                                                // Mac 0x1dd7f0, Win 0x405980

    void  Set(Real32 left, Real32 top, Real32 right, Real32 bottom);    // Mac 0x1dd820, Win 0x4059a0
    void  Set(const Vector2& position, const Vector2& size);            // Mac 0x1dd850
    Rect& operator=(const Rect& r);                                     // Mac 0x1dd890, Win 0x4059c0
    void  Set(const Rect& r);                                           // Mac 0x1dd8c0, Win 0x4059f0

    Real32 GetWidth() const;                                // Mac 0x1dd8f0, Win 0x405c20
    Real32 GetHeight() const;                               // Mac 0x1dd910, Win 0x405c60
    void   SetPosition(const Vector2& position);            // Mac 0x1dd930, Win 0x405a20 (keeps size)
    void   Offset(const Vector2& delta);                    // Mac 0x1dd980
    void   SetSize(const Vector2& size);                    // Mac 0x1dd9b0, Win 0x405a70
    void   Normalize();                                     // Mac 0x1dd9e0, Win 0x405a90
    void   Scale(const Vector2& factor);                    // Mac 0x1dda20, Win 0x405ae0
    void   Divide(const Vector2& divisor);                  // Mac 0x1dda70, Win 0x405b10
    bool   IsValid() const;                                 // Mac 0x1ddac0, Win 0x405b40 (min < max)
    bool   IsZero() const;                                  // Mac 0x1ddaf0, Win 0x405b70
    Vector2 GetSize() const;                                // Mac 0x1ddb40, Win 0x405bb0
    Vector2 GetAbsoluteSize() const;                        // Mac 0x1ddb70, Win 0x405bd0
    Real32 GetAbsoluteWidth() const;                        // Mac 0x1ddbe0, Win 0x405c30
    Real32 GetAbsoluteHeight() const;                       // Mac 0x1ddc20, Win 0x405c70
    bool   IsFlipped() const;                               // Mac 0x1ddc60, Win 0x405ca0 (right < left)
    Rect   GetFlipped() const;                              // Mac 0x1ddc80, Win 0x405cc0 (swap left/right)
    operator Real32*();                                     // Mac 0x1ddcb0
    Vector2 GetTopRight() const;                            // Mac 0x1ddcc0, Win 0x405d00 (right, top)
    Vector2 GetBottomLeft() const;                          // Mac 0x1ddce0, Win 0x405d20 (left, bottom)
    Vector2& GetMax();                                      // Mac 0x1ddd00
    const Vector2& GetMin() const { return m_min; }
    bool   operator==(const Rect& r) const;                 // Mac 0x1ddd10, Win 0x405d40
    bool   operator!=(const Rect& r) const;                 // Mac 0x1ddd90, Win 0x405d90
    bool   Contains(const Vector2& point) const;            // Mac 0x1dde10, Win 0x405df0 (max exclusive)
    bool   Contains(const Rect& r) const;                   // Mac 0x1dde50, Win 0x405e40
    Rect&  operator|=(const Rect& r);                       // Mac 0x1ddea0, Win 0x405e90 (union)
    void   Union(const Rect& r);                            // Mac 0x1ddef0
    Rect&  operator&=(const Rect& r);                       // Mac 0x1ddf40, Win 0x405ef0 (intersection)
    void   Intersect(const Rect& r);                        // Mac 0x1ddf90

    Vector2 m_min;   // +0x00 left, top
    Vector2 m_max;   // +0x08 right, bottom
};

// ===========================================================================================
// IntRect — 16 bytes {left, top, right, bottom} Int32. The Win constructor 0x405f50 is
// identical to MFC's CRect(int,int,int,int) (FID match). (name inferred)
// ===========================================================================================
class IntRect {
public:
    IntRect(Int32 left, Int32 top, Int32 right, Int32 bottom); // Mac 0x1ddfe0, Win 0x405f50
    Int32 GetWidth() const;                                 // Mac 0x1de010
    Int32 GetHeight() const;                                // Mac 0x1de020
    bool  IsValid() const;                                  // Mac 0x1de030
    void  Intersect(const IntRect& r);                      // Mac 0x1de050

    Int32 m_left, m_top, m_right, m_bottom;                 // +0x00 +0x04 +0x08 +0x0c
};

// ===========================================================================================
// BoundingBox — 24 bytes, axis-aligned {min +0, max +0xc}. (name inferred)
//                                                                     Mac 0x1de090-0x1de54f
// ===========================================================================================
class BoundingBox {
public:
    BoundingBox(Real32 minX, Real32 minY, Real32 minZ,
                Real32 maxX, Real32 maxY, Real32 maxZ);     // Mac 0x1de090, Win 0x405f70
    BoundingBox(const Vector3& position, const Vector3& size); // Mac 0x1de0e0, Win 0x405fa0
    explicit BoundingBox(const Vector3& size);              // Mac 0x1de130, Win 0x405fe0 (min 0)
    BoundingBox(const BoundingBox& b);                      // Mac 0x1de160, Win 0x406000
    BoundingBox& operator=(const BoundingBox& b);           // Mac 0x1de190, Win 0x406030

    Real32 GetWidth() const;                                // Mac 0x1de1d0, Win 0x4061f0
    Real32 GetHeight() const;                               // Mac 0x1de1f0, Win 0x406200
    Real32 GetDepth() const;                                // Mac 0x1de210, Win 0x406210
    void   Translate(const Vector3& delta);                 // Mac 0x1de230, Win 0x406080
    BoundingBox& operator+=(const Vector3& delta);          // Mac 0x1de280, Win 0x4060e0
    bool   IsZero() const;                                  // Mac 0x1de2d0, Win 0x4060f0
    // min = +FLT_MAX, max = -FLT_MAX: ready for Add/Union.
    void   Reset();                                         // Mac 0x1de330, Win 0x406140
    Vector3 GetSize() const;                                // Mac 0x1de360, Win 0x406160
    Vector3 GetCenter() const;                              // Mac 0x1de3a0, Win 0x406180
    Real32 GetVolume() const;                               // Mac 0x1de410, Win 0x4061c0
    BoundingBox& operator|=(const BoundingBox& b);          // Mac 0x1de450, Win 0x406220
    void   Union(const BoundingBox& b);                     // Mac 0x1de4d0
    // Grow to contain `point`. Always inlined on Mac.                       Win 0x4062c0 (EDX)
    void Add(const Vector3& point)
    {
        if (point.x < m_min.x) m_min.x = point.x;
        if (point.y < m_min.y) m_min.y = point.y;
        if (point.z < m_min.z) m_min.z = point.z;
        if (m_max.x < point.x) m_max.x = point.x;
        if (m_max.y < point.y) m_max.y = point.y;
        if (m_max.z < point.z) m_max.z = point.z;
    }

    Vector3 m_min;   // +0x00
    Vector3 m_max;   // +0x0c
};

// Distances between points. (names inferred)
Real32 Distance(const Vector2& a, const Vector2& b);          // Mac 0x1de550, Win 0x408d50
Real32 DistanceSquared(const Vector2& a, const Vector2& b);   // Mac 0x1de590, Win 0x408d00
Real32 Distance(const Vector3& a, const Vector3& b);          // Mac 0x1de640, Win 0x408e10
// Win 0x408da0 — the Mac build has only the member Vector3::GetDistanceSquared (0x1dd190).
inline Real32 DistanceSquared(const Vector3& a, const Vector3& b) { return a.GetDistanceSquared(b); }

// ===========================================================================================
// Sphere — 16 bytes {center +0, radius +0xc}. (name inferred)        Mac 0x1de5c0-0x1de83f
// ===========================================================================================
class Sphere {
public:
    Sphere(const Vector3& center, Real32 radius);           // Mac 0x1de5c0
    explicit Sphere(Real32 radius);                         // Mac 0x1de5f0, Win 0x406340 (center 0)
    Sphere(const Sphere& s);                                // Mac 0x1de620
    // Bounding sphere of a box: its center, radius = center..min distance.
    explicit Sphere(const BoundingBox& box);                // Mac 0x1de690, Win 0x406360
    Sphere& operator=(const Sphere& s);                     // Mac 0x1de730
    void    Set(const BoundingBox& box);                    // Mac 0x1de750, Win 0x4063d0
    bool    Intersects(const Sphere& s) const;              // Mac 0x1de7f0, Win 0x406440 (touching counts)

    Vector3 m_center;   // +0x00
    Real32  m_radius;   // +0x0c
};

// ===========================================================================================
// Cone — 48 bytes; the sphere test is Eberly's TestIntersection(Sphere, Cone) with the
// precomputed trigonometry. (name inferred)                          Mac 0x1de840-0x1dea3f
// ===========================================================================================
class Cone {
public:
    Cone(const Vector3& apex, const Vector3& axis, Real32 angle); // Mac 0x1de840, Win 0x406490
    bool Intersects(const Sphere& s) const;                 // Mac 0x1de8f0, Win 0x406500

    Vector3 m_apex;        // +0x00
    Vector3 m_axis;        // +0x0c unit length expected
    Real32  m_angle;       // +0x18 half angle
    Real32  m_cos;         // +0x1c
    Real32  m_sin;         // +0x20
    Real32  m_cosSquared;  // +0x24
    Real32  m_sinSquared;  // +0x28
    Real32  m_invSin;      // +0x2c
};

// ===========================================================================================
// Plane — 16 bytes {normal +0, distance +0xc}: normal * p + distance = 0. (name inferred)
//                                                                     Mac 0x1dea40-0x1decbf
// ===========================================================================================
class Plane {
public:
    Plane();                                                // Mac 0x1dea40, Win 0x4066a0 (y = 0 plane)
    Plane(Real32 a, Real32 b, Real32 c, Real32 d);          // Mac 0x1dea60
    // Recomputes the distance so that the plane passes through `point`.
    void SetPoint(const Vector3& point);                    // Mac 0x1dea90, Win 0x4067f0
    Plane(const Vector3& normal, const Vector3& point);     // Mac 0x1deae0, Win 0x4066c0
    // Through three points, normal (p1 - p0) x (p2 - p0) normalized.
    Plane(const Vector3& p0, const Vector3& p1, const Vector3& p2); // Mac 0x1deb40, Win 0x406710
    Plane(const Plane& p);                                  // Mac 0x1dec60, Win 0x406820
    Real32 GetDistance(const Vector3& point) const;         // Mac 0x1dec80, Win 0x40a4c0 (signed; Win cdecl)

    Vector3 m_normal;     // +0x00
    Real32  m_distance;   // +0x0c
};

// ===========================================================================================
// Frustum — 76 bytes: four side planes through an apex (pick volume). (name inferred)
//                                                                     Mac 0x1decc0-0x1df27f
// ===========================================================================================
class Frustum {
public:
    enum Containment { OUTSIDE = 0, INSIDE = 1, INTERSECTING = 2 };   // (names inferred)

    Frustum();                                              // Mac 0x1decc0, Win 0x406850
    // planes[i] = Plane(apex, corner[i], corner[i+1 mod 4])
    Frustum(const Vector3& apex, const Vector3& corner0, const Vector3& corner1,
            const Vector3& corner2, const Vector3& corner3); // Mac 0x1ded20, Win 0x406890
    // INSIDE when the sphere lies completely on the positive side of all four planes.
    Containment Classify(const Sphere& s) const;            // Mac 0x1df130, Win 0x4069b0

    Plane   m_planes[4];   // +0x00
    Vector3 m_apex;        // +0x40
};

// ===========================================================================================
// Matrix3D — 64 bytes, column-major 4x4 (OpenGL layout).               Mac 0x1df280-0x1e356f
// Element stride 64 confirmed by BasicArray<Matrix3D> (Mac 0x237e60: sar 6).
// ===========================================================================================
class Matrix3D {
public:
    static const Matrix3D& Identity();                      // Mac 0x1df280, Win 0x408510 (function static)

    Matrix3D();                                             // Mac 0x1df350, Win 0x406c20 (identity)
    Matrix3D(const Matrix3D& m);                            // Mac 0x1df480, Win 0x406c40
    // Arguments in memory order (column by column).
    Matrix3D(Real32 m00, Real32 m01, Real32 m02, Real32 m03,
             Real32 m04, Real32 m05, Real32 m06, Real32 m07,
             Real32 m08, Real32 m09, Real32 m10, Real32 m11,
             Real32 m12, Real32 m13, Real32 m14, Real32 m15); // Mac 0x1df4e0
    // this = a * b (b applied first). `this` must not alias a or b.
    void Multiply(const Matrix3D& a, const Matrix3D& b);    // Mac 0x1df590, Win 0x407260
    Matrix3D(const Matrix3D& a, const Matrix3D& b);         // Mac 0x1df6c0, Win 0x406c60 (= a * b)

    void SetIdentity();                                     // Mac 0x1df6d0, Win 0x406c80
    void SetRotationX(Real32 angle);                        // Mac 0x1df800, Win 0x406ca0
    void SetRotationY(Real32 angle);                        // Mac 0x1df990, Win 0x406d00
    void SetRotationZ(Real32 angle);                        // Mac 0x1dfb20
    // R = Ry(yaw) * Rx(pitch) * Rz(roll), no translation; GetEulerAngles inverts it.
    void SetEulerAngles(Real32 pitch, Real32 yaw, Real32 roll);         // Mac 0x1dfcb0, Win 0x406d60
    void GetEulerAngles(Real32& pitch, Real32& yaw, Real32& roll) const; // Mac 0x1dfe60, Win 0x406e90
    // Rotation about `axis` (normalized here) by `angle`, no translation.
    void SetRotation(const Vector3& axis, Real32 angle);    // Mac 0x1dff40, Win 0x406f10
    // Rotation about the axis through `center`, followed by `translation`.
    void SetRotation(const Vector3& axis, Real32 angle, const Vector3& translation,
                     const Vector3& center);                // Mac 0x1e0120, Win 0x407030
    void SetTranslation(const Vector3& t);                  // Mac 0x1e0390, Win 0x4080d0
    void SetScaling(Real32 sx, Real32 sy, Real32 sz);       // Mac 0x1e0440, Win 0x407110 (identity + diagonal)
    // Normalizes the three axis columns; false (unchanged) if one of them is ~zero.
    bool RemoveScaling();                                   // Mac 0x1e0590, Win 0x407140
    void SetTranslationMatrix(const Vector3& t);            // Mac 0x1e07e0, Win 0x407230 (identity + t)
    // this = inverse(m); false (this unchanged) if |det| < FLT_EPSILON. `this` may be m.
    bool SetInverse(const Matrix3D& m);                     // Mac 0x1e0920, Win 0x407310
    void SetLookAt(const Vector3& eye, const Vector3& target, const Vector3& up); // Mac 0x1e1400, Win 0x407a90
    // Projection with z mapped to [0, 1] (far plane 1 / near plane 0, right-handed).
    void SetPerspective(Real32 fovX, Real32 fovY, Real32 zNear, Real32 zFar);       // Mac 0x1e1630, Win 0x407c40
    void SetPerspectiveFovY(Real32 fovY, Real32 aspect, Real32 zNear, Real32 zFar); // Mac 0x1e1750, Win 0x407cc0
    void SetOrthographic(const Rect& rect, Real32 zNear, Real32 zFar); // Mac 0x1e1840, Win 0x407d30
    // Rotate the three axis columns in place (translation untouched).
    void RotateX(Real32 angle);                             // Mac 0x1e1950, Win 0x407dd0
    void RotateY(Real32 angle);                             // Mac 0x1e1ac0, Win 0x407e00
    void RotateZ(Real32 angle);                             // Mac 0x1e1c30, Win 0x407e20
    // X, then Y, then Z (each column as a vector). Never called directly on Mac (only the
    // three wrappers above, with the constant angles folded).       Win 0x407e40 (ESI = this)
    void Rotate(Real32 angleX, Real32 angleY, Real32 angleZ);
    Vector3& GetTranslation();                              // Mac 0x1e1da0, Win 0x4080f0
    const Vector3& GetTranslation() const { return *(const Vector3*)&m[12]; }
    void Multiply(const Matrix3D& m);                       // Mac 0x1e1db0, Win 0x408100 (this = this * m)
    void Translate(const Vector3& delta);                   // Mac 0x1e1e20, Win 0x4080a0
    Matrix3D& operator*=(const Matrix3D& m);                // Mac 0x1e1e60, Win 0x408140
    bool IsIdentity() const;                                // Mac 0x1e1ee0 (bitwise compare)
    // Rotation part as axis (unit) + angle in [0, PI]; (0,1,0) / 0 for no rotation.
    void GetAxisAngle(Vector3& axis, Real32& angle) const;  // Mac 0x1e1fd0, Win 0x408180
    operator Real32*();                                     // Mac 0x1e2260
    operator const Real32*() const;                         // Mac 0x1e2270
    // Lexicographic over the 16 elements with tolerance (Scene::Matrix3DLess).
    bool IsLess(const Matrix3D& m, Real32 epsilon) const;   // Mac 0x1e2280, Win 0x4083c0
    bool IsEqual(const Matrix3D& m, Real32 epsilon) const;  // Mac 0x1e2460, Win 0x408430
    bool IsRotationEqual(const Matrix3D& m, Real32 epsilon) const; // Mac 0x1e2610, Win 0x408480 (3x3 part)
    bool operator==(const Matrix3D& m) const;               // Mac 0x1e2700, Win 0x4084f0
    bool operator!=(const Matrix3D& m) const;               // Mac 0x1e2720
    // Solves a * X = b for X, result in b. Mac: LAPACK sgesv (a is overwritten with its LU
    // factors); Win: b = inverse(a) * b, a untouched. false if a is singular.
    static bool Solve(Matrix3D& a, Matrix3D& b);            // Mac 0x1e2750, Win 0x4085d0

    Real32 m[16];   // +0x00 column-major
};

// Point / direction transforms (p' = M * p). (names inferred)
Vector3  Rotate(const Vector3& v, const Matrix3D& m);         // Mac 0x1e03b0, Win 0x4086b0 (3x3 only)
Vector3  operator*(const Vector3& p, const Matrix3D& m);      // Mac 0x1e27b0, Win 0x408650
Vector3& operator*=(Vector3& p, const Matrix3D& m);           // Mac 0x1e2850, Win 0x408710
void     RotateInPlace(Vector3& v, const Matrix3D& m);        // Mac 0x1e28f0, Win 0x408760
// Box of the eight transformed corners. (names inferred)
BoundingBox operator*(const BoundingBox& b, const Matrix3D& m);  // Mac 0x1e2980, Win 0x4087d0
BoundingBox Rotate(const BoundingBox& b, const Matrix3D& m);     // Mac 0x1e2fd0, Win 0x408980
Real32 TranslationDistanceSquared(const Matrix3D& a, const Matrix3D& b); // Mac 0x1e3570, Win 0x408e40

// ===========================================================================================
// Free distance / intersection functions.                             Mac 0x1e35c0-0x1e5cef
// "direction" parameters are expected to be unit length unless stated. (names inferred
// except RayCylinderIntersect, which the Mac assert names)
// ===========================================================================================
// Closest point of the line through `origin` to `point`. Inlined on Mac.  Win 0x408f70
inline Vector3 ClosestPointOnLine(const Vector3& origin, const Vector3& direction, const Vector3& point)
{
    Real32 t = (point - origin) * direction;
    return Vector3(direction.x * t + origin.x, direction.y * t + origin.y, direction.z * t + origin.z);
}
Real32  PointRayDistanceSquared(const Vector3& origin, const Vector3& direction,
                                const Vector3& point);                       // Mac 0x1e35c0, Win 0x408eb0
Real32  PointLineDistance(const Vector3& origin, const Vector3& direction,
                          const Vector3& point);                             // Mac 0x1e36d0, Win 0x408ff0
Vector3 ClosestPointOnSegment(const Vector3& start, const Vector3& end,
                              const Vector3& point);                         // Mac 0x1e3790, Win 0x409020
Real32  PointSegmentDistanceSquared(const Vector3& start, const Vector3& end,
                                    const Vector3& point);                   // Mac 0x1e38e0
// Closest points of the lines (a0, a1) and (b0, b1); false if parallel.
bool ClosestPointsOnLines(const Vector3& a0, const Vector3& a1, const Vector3& b0,
                          const Vector3& b1, Vector3& pointA, Vector3& pointB,
                          Real32& s, Real32& t);                             // Mac 0x1e3a60, Win 0x409190
// Line parameters s, t of the closest points (a0 + s*(a1-a0), b0 + t*(b1-b0)); outputs the
// two directions and w = a0 - b0. false if the lines are parallel.
bool LineLineParameters(const Vector3& a0, const Vector3& a1, const Vector3& b0,
                        const Vector3& b1, Real32& s, Real32& t, Vector3& directionA,
                        Vector3& directionB, Vector3& w);                    // Mac 0x1e3b60, Win 0x4092d0
// Segments (originA + s*dirA, originB + t*dirB), s, t clamped to [0, 1] (Sunday's
// algorithm); w = originA - originB. false if the segments are parallel.
bool SegmentSegmentParameters(const Vector3& originA, const Vector3& directionA,
                              const Vector3& originB, const Vector3& directionB,
                              Real32& s, Real32& t, Vector3& w);             // Mac 0x1e3ed0, Win 0x409650
// Unit directions with lengths; outputs distances along each segment.
void SegmentSegmentClosest(const Vector3& originA, const Vector3& directionA, Real32 lengthA,
                           const Vector3& originB, const Vector3& directionB, Real32 lengthB,
                           Real32& distanceA, Real32& distanceB);            // Mac 0x1e42a0, Win 0x409b60
// Segments given by end points; s, t in [0, 1].
void SegmentSegmentClosest(const Vector3& a0, const Vector3& a1, const Vector3& b0,
                           const Vector3& b1, Real32& s, Real32& t, Vector3& directionA,
                           Vector3& directionB, Vector3& w);                 // Mac 0x1e4430, Win 0x409d50
// Line of intersection of two planes (point, unit direction); false if parallel.
bool PlanePlaneIntersect(const Plane& a, const Plane& b, Vector3& point,
                         Vector3& direction);                                // Mac 0x1e4610, Win 0x40a010
Real32  RaySegmentDistanceSquared(const Vector3& origin, const Vector3& direction,
                                  const Vector3& start, const Vector3& end); // Mac 0x1e48b0, Win 0x40a270
Vector3 ProjectOntoPlane(const Plane& plane, const Vector3& point);          // Mac 0x1e4be0, Win 0x40a4f0
bool RayPlaneIntersect(const Vector3& origin, const Vector3& direction, const Plane& plane,
                       Real32& t);                                           // Mac 0x1e4c60, Win 0x40a560
bool RayPlaneIntersect(const Vector3& origin, const Vector3& direction, const Plane& plane,
                       Vector3& point);                                      // Mac 0x1e4d00, Win 0x40a5f0
// Ray from p0 through p1 (no upper bound on the parameter).
bool RayPlaneIntersectPoints(const Vector3& p0, const Vector3& p1, const Plane& plane,
                             Vector3& point);                                // Mac 0x1e4df0
bool LinePlaneIntersect(const Vector3& origin, const Vector3& direction, const Plane& plane,
                        Vector3& point);                                     // Mac 0x1e4f00, Win 0x40a680
bool LinePlaneIntersectPoints(const Vector3& p0, const Vector3& p1, const Plane& plane,
                              Vector3& point);                               // Mac 0x1e4fe0, Win 0x40a770
// First hit of the ray (from inside: the exit point); `point` may be NULL.
bool RaySphereIntersect(const Vector3& origin, const Vector3& direction, const Vector3& center,
                        Real32 radius, Vector3* point);                      // Mac 0x1e50e0, Win 0x40a7b0
// Capped cylinder (center, axis, half height, radius); nearest hit in front of the ray.
bool RayCylinderIntersect(const Vector3& origin, const Vector3& direction, const Vector3& center,
                          const Vector3& axis, Real32 halfHeight, Real32 radius,
                          Vector3* point);                                   // Mac 0x1e5200, Win 0x40a930
// Moeller-Trumbore, front faces only (det >= EPSILON); t, u, v as fractions.
bool RayTriangleIntersect(const Vector3& origin, const Vector3& direction, const Vector3& v0,
                          const Vector3& v1, const Vector3& v2, Real32& t, Real32& u,
                          Real32& v);                                        // Mac 0x1e5ad0, Win 0x40b260

} // namespace LEGO
