// LEGO Digital Designer — restored source
// TU "ConnectionFieldAxle.cpp"  Mac TU 327, 0x2db0a0-0x2dd97f (33 functions); Win 0x4aef60-
// 0x4b0630, 0x4b08a0. See ConnectionFieldAxle.h.
//
// Also emitted in this TU on Mac (inline members of ConnectionField.h, defined in
// ConnectionField.cpp): 0x2dd4e0 / 0x2dd510 LineHelper::IsCompatible (2), 0x2dd530
// ConnectionField::GetConnectionAngle, 0x2dd540 LineHelper::IsOccupied, 0x2dd550 / 0x2dd560
// ApplyPointGroups / SetPointsBlocked, 0x2dd570-0x2dd5f0 ~ConnectionField (D1, D0, thunks).
// Initialiser 0x2dd940: <iostream> only.
#include "ConnectionFieldAxle.h"
#include "ConnectionFieldLineFunctions.h"

#include <iostream>
#include <math.h>

namespace LEGO {

UInt32 ConnectionFieldAxle::s_connectionTable[KIND_COUNT][KIND_COUNT];

namespace {

inline void SetSymmetric(UInt32 a, UInt32 b, UInt32 value)
{
    ConnectionFieldAxle::s_connectionTable[a][b] = value;
    ConnectionFieldAxle::s_connectionTable[b][a] = value;
}

} // namespace

// Mac 0x2dcf90, Win 0x4b0630
// Default: two odd kinds (two holes) block each other (1), everything else does not interact.
void ConnectionFieldAxle::InitializeTables()
{
    for (UInt32 i = 0; i < KIND_COUNT; ++i)
        for (UInt32 j = 0; j < KIND_COUNT; ++j)
            s_connectionTable[i][j] = (i & 1) && (j & 1) ? 1 : 0;

    SetSymmetric(4, 2, 1);
    SetSymmetric(2, 3, 4);
    SetSymmetric(4, 3, 1);
    SetSymmetric(8, 3, 4);
    SetSymmetric(14, 3, 1);
    SetSymmetric(2, 5, 4);
    SetSymmetric(4, 5, 2);
    SetSymmetric(6, 5, 1);
    SetSymmetric(8, 5, 4);
    SetSymmetric(12, 5, 4);
    SetSymmetric(14, 5, 1);
    SetSymmetric(4, 7, 4);
    SetSymmetric(6, 7, 4);
    SetSymmetric(14, 7, 1);
    SetSymmetric(2, 13, 4);
    SetSymmetric(4, 13, 1);
    SetSymmetric(6, 13, 1);
    SetSymmetric(8, 13, 4);
    SetSymmetric(12, 13, 4);
    SetSymmetric(14, 13, 1);
    SetSymmetric(14, 15, 4);
    SetSymmetric(2, 17, 4);

    // Kind 19 behaves like kind 3, kind 21 like kind 7 (with a few exceptions).
    for (UInt32 i = 0; i < KIND_COUNT; ++i) {
        s_connectionTable[19][i] = s_connectionTable[3][i];
        s_connectionTable[i][19] = s_connectionTable[i][3];
    }
    SetSymmetric(19, 7, 0);
    SetSymmetric(19, 15, 0);
    for (UInt32 i = 0; i < KIND_COUNT; ++i) {
        s_connectionTable[21][i] = s_connectionTable[7][i];
        s_connectionTable[i][21] = s_connectionTable[i][7];
    }
    SetSymmetric(21, 15, 0);
}

// Mac 0x2dcf20
bool ConnectionFieldAxle::SnapToEnd(Real32* offset, Real32 length, bool anchored)
{
    Real32 value = *offset;
    if (anchored) {
        if (value <= -0.4f || 0.4f <= value)
            return false;
        length = 0.0f;
    } else {
        if (value <= length + -0.4f || length + 0.4f <= value)
            return false;
    }
    *offset = length;
    return true;
}

// Mac 0x2db0a0
ConnectionFieldAxle* ConnectionFieldAxle::Create(const Matrix3D& transform, UInt32 kind,
                                                 Real32 length, bool startCapped, bool endCapped,
                                                 bool grabbing, bool requireGrabbing)
{
    if (kind >= KIND_COUNT)
        return 0;
    return new ConnectionFieldAxle(transform, kind, length, startCapped, endCapped, grabbing,
                                   requireGrabbing, kind == 2 ? length : 0.08f);
}

// Win 0x4af0c0; Mac inlined in Create
ConnectionFieldAxle::ConnectionFieldAxle(const Matrix3D& transform, UInt32 kind, Real32 length,
                                         bool startCapped, bool endCapped, bool grabbing,
                                         bool requireGrabbing, Real32 step)
    : ConnectionFieldLineHelper(transform, TYPE_AXLE, kind, length, startCapped, endCapped),
      m_step(step), m_grabbing(grabbing), m_requireGrabbing(requireGrabbing), m_connectionCount(0)
{
}

// Win 0x4af210; Mac inlined in Clone
ConnectionFieldAxle::ConnectionFieldAxle(const ConnectionFieldAxle& o)
    : Object(), ConnectionFieldLineHelper(o), m_step(o.m_step), m_grabbing(o.m_grabbing),
      m_requireGrabbing(o.m_requireGrabbing), m_connectionCount(o.m_connectionCount)
{
}

// Mac 0x2dd820 (D1), 0x2dd850 (D0), 0x2dd8a0 / 0x2dd8e0 (Object thunks); Win 0x4af180 / 0x4af2c0
ConnectionFieldAxle::~ConnectionFieldAxle()
{
}

// Mac 0x2dd640, Win 0x4aef60
bool ConnectionFieldAxle::IsCompatible(const ConnectionField& other) const
{
    return other.GetType() == m_type && (s_connectionTable[m_kind][other.GetKind()] & 6) != 0;
}

// Mac 0x2dd680, Win 0x4aef90
// Two male fields of the same kind count as compatible as well (coaxial axles).
bool ConnectionFieldAxle::IsCompatible(UInt32, const ConnectionField& other, UInt32) const
{
    if (IsCompatible(other))
        return true;
    if (m_kind & 1)
        return false;
    return m_kind == other.GetKind();
}

// Mac 0x2dc7e0, Win 0x4afeb0
// Angle between `reference` and the Z axis of this field for two axles on `axis` through
// `center` (sign by the axis direction). NOTE: the Z axis of `otherTransform` is computed and
// not used (both builds).
UInt32 ConnectionFieldAxle::GetConnectionAngle(const Matrix3D& transform,
                                               const ConnectionField& other,
                                               const Matrix3D& otherTransform,
                                               const Vector3& axis, const Vector3& reference,
                                               const Vector3& center, Real32& angle) const
{
    if (other.GetType() != m_type)
        return 0;
    if ((s_connectionTable[m_kind][other.GetKind()] & 6) == 0)
        return 0;
    Vector3 up(0, 1, 0);
    Vector3 a = Rotate(up, transform);
    Vector3 b = Rotate(Vector3(0, 1, 0), otherTransform);
    Real32 d = axis * a;
    if (!(Abs(Abs(d) + -1.0f) < 0.001f))
        return CONNECTION_FREE;
    d = axis * b;
    if (!(Abs(Abs(d) + -1.0f) < 0.001f))
        return CONNECTION_FREE;
    Real32 distance = PointLineDistance(center, axis, transform.GetTranslation());
    Real32 otherDistance = PointLineDistance(center, axis, otherTransform.GetTranslation());
    if (!(Abs(distance - otherDistance) < 0.0001f))
        return 0;
    if (Distance(transform.GetTranslation(), otherTransform.GetTranslation()) + 0.0001f > 1.0f)
        return 0;
    Vector3 z(0, 0, 1);
    Vector3 za = Rotate(z, transform);
    Vector3 zb = Rotate(Vector3(0, 0, 1), otherTransform);
    (void)zb;
    Real32 c = Min(Max(reference * za, -1.0f), 1.0f);
    Real32 s = axis * a;
    Real32 v = 0.0f < s ? -c : c;
    Real32 result = 0.0f;
    if (!(v > 0.99999f)) {
        if (-0.99999f > v)
            result = PI;
        else
            result = (Real32)acos((double)v);
    }
    angle = result;
    return CONNECTION_FREE;
}

// Mac 0x2dcab0, Win 0x4b0100
// NOTE: `other` is assumed to be an axle (the connection table only pairs axles).
void ConnectionFieldAxle::Connect(ConnectionField& other, const ConnectionIndices& indices,
                                  Attributes* attributes)
{
    ++m_connectionCount;
    ++static_cast<ConnectionFieldAxle&>(other).m_connectionCount;
    NotifyConnect(*this, other, indices, attributes);
}

// Mac 0x2dcad0, Win 0x4b0150
void ConnectionFieldAxle::Disconnect(ConnectionField& other, const ConnectionIndices& indices)
{
    --m_connectionCount;
    --static_cast<ConnectionFieldAxle&>(other).m_connectionCount;
    NotifyDisconnect(*this, other, indices);
}

// Mac 0x2dd6c0, Win 0x4aefd0
void ConnectionFieldAxle::Clone(ConnectionField*& copy) const
{
    copy = new ConnectionFieldAxle(*this);
}

// Mac 0x2dcaf0, Win 0x4b01a0
// The point of the axle closest to the ray, snapped to 0.08 steps.
bool ConnectionFieldAxle::Pick(const Matrix3D& transform, const Vector3& origin,
                               const Vector3& direction, Real32 maxDistance, Real32& distance,
                               UInt32& index, Matrix3D& offset) const
{
    const Vector3& start = transform.GetTranslation();
    Vector3 end = Vector3(0, m_length, 0) * transform;
    if (!(RaySegmentDistanceSquared(origin, direction, start, end) <= 0.0576f))
        return false;

    Vector3 pointRay;
    Vector3 pointAxis;
    Vector3 rayEnd = origin + direction;
    const Vector3& axisStart = transform.GetTranslation();
    Vector3 axisEnd = transform.GetTranslation() + Rotate(Vector3(0, m_length, 0), transform);
    Real32 s, t;
    ClosestPointsOnLines(origin, rayEnd, axisStart, axisEnd, pointRay, pointAxis, s, t);

    Real32 beyond = -1.0f + t;
    if (beyond > 0.0f && Abs(beyond) > EPSILON)
        return false;
    if (0.0f > t && Abs(t) > EPSILON)
        return false;

    Real32 position = RoundToStep(t * m_length, 0.08f);
    pointAxis = transform.GetTranslation() + Rotate(Vector3(0, position, 0), transform);
    Real32 d = pointAxis.GetDistanceSquared(origin);
    Real32 margin = maxDistance - d;
    if (0.0f > margin && Abs(margin) > 0.0064f)
        return false;
    distance = d;
    index = 0;
    offset.SetTranslationMatrix(Vector3(0, position, 0));
    return true;
}

// Mac 0x2dcdf0, Win 0x4b0430
// Axis (0, -1, 0) of the field; reversed connections (indices.m_reversed) of a female field
// use (0, 1, 0) and the far end as position.
void ConnectionFieldAxle::GetFrame(bool self, const ConnectionField& other,
                                   const ConnectionIndices& indices, Vector3& axis,
                                   Vector3& normal, Vector3& position) const
{
    const ConnectionFieldAxle* field = self ? this : static_cast<const ConnectionFieldAxle*>(&other);
    bool flip = indices.m_reversed != 0 ? (field->m_kind & 1) != 0 : false;
    const Matrix3D& m = field->m_transform;
    axis = Rotate(Vector3(0, flip ? 1.0f : -1.0f, 0), m);
    normal = Rotate(Vector3(0, 0, 1), m);
    position = m.GetTranslation();
    if (flip)
        position += Rotate(Vector3(0, field->m_length, 0), m);
}

// Mac 0x2dd7b0, Win 0x4af060
UInt32 ConnectionFieldAxle::GetConnectionType(const ConnectionField& other,
                                              const ConnectionIndices&) const
{
    return s_connectionTable[m_kind][other.GetKind()];
}

// Mac 0x2dd7e0, Win 0x4af080 (shared with ConnectionFieldPlanar)
bool ConnectionFieldAxle::IsRigidConnection(const ConnectionField& other,
                                            const ConnectionIndices& indices) const
{
    return GetConnectionType(other, indices) == CONNECTION_RIGID;
}

// Mac 0x2dd810, Win 0x4af0a0
UInt32 ConnectionFieldAxle::GetConnectionState(bool, UInt32, const UInt32*) const
{
    return 2;
}

// Mac 0x2db460, Win 0x4b08a0
UInt32 ConnectionFieldAxle::TestPosition(bool sameDirection, const Vector3& position,
                                         const Vector3& otherPosition, const Vector3& axis,
                                         const Vector3& otherAxis,
                                         const ConnectionFieldAxle& other) const
{
    const ConnectionFieldLineHelper* f;
    const ConnectionFieldLineHelper* m;
    Real32 offset;
    UInt32 result = TestLineOverlap(*this, sameDirection, position, axis, m_length, otherPosition,
                                    otherAxis, other.m_length, other, &f, &m, &offset,
                                    s_connectionTable[m_kind][other.m_kind]);
    // Both are axles (the table pairs axles only).
    const ConnectionFieldAxle* female = static_cast<const ConnectionFieldAxle*>(f);
    const ConnectionFieldAxle* male = static_cast<const ConnectionFieldAxle*>(m);
    if ((result & 6) == 0)
        return result;
    if (female->m_requireGrabbing && !male->m_grabbing)
        return 1;

    UInt32 kind = m_kind;
    UInt32 otherKind = other.m_kind;
    if (((kind | 0x10) == 0x13 && otherKind == 2) || (kind == 2 && (otherKind | 0x10) == 0x13)) {
        // Axle in an axle hole (kinds 3 / 19): whole steps of the axle.
        Real32 maleLength = male->m_length;
        Real32 femaleLength = female->m_length;
        if (Abs(maleLength - femaleLength) < 0.0001f) {
            Real32 x = offset;
            if (!sameDirection)
                x -= femaleLength;
            return 0.005f > Abs(x) ? result : 1;
        }
        if (!(femaleLength > maleLength))
            return 1;
        Real32 step = male->m_step;
        Real32 rest = (Real32)fmodf(offset, step);
        if (!(0.005f > Abs(rest)) && !(0.005f > Abs(rest - step)))
            return 1;
        Real32 x;
        if (sameDirection) {
            if (0.0f > offset && Abs(offset) > 0.005f)
                return 1;
            x = offset - (femaleLength - maleLength);
        } else {
            Real32 front = offset - maleLength;
            if (0.0f > front && Abs(front) > 0.005f)
                return 1;
            x = offset - femaleLength;
            if (!(x > 0.0f))
                return result;
            return Abs(x) > 0.005f ? 1 : result;
        }
        if (Abs(x) > 0.005f && x > 0.0f)
            return 1;
        return result;
    }
    if ((female->m_kind | 0x10) == 0x13) {
        // Something else in an axle hole: 0.08 inside the ends.
        Real32 femaleLength = female->m_length;
        Real32 maleLength = male->m_length;
        if (Abs(maleLength - femaleLength) < 0.0001f) {
            Real32 x;
            if (sameDirection)
                x = offset + -0.08f;
            else
                x = offset - (femaleLength + 0.08f);
            return 0.005f > Abs(x) ? result : 1;
        }
        if (!(femaleLength > maleLength))
            return 1;
        Real32 step = male->m_step;
        Real32 rest = (Real32)fmodf(offset, step);
        if (!(0.005f > Abs(rest)) && !(0.005f > Abs(rest - step)))
            return 1;
        Real32 x;
        if (sameDirection) {
            Real32 front = -0.08f + offset;
            if (0.0f > front && Abs(front) > 0.005f)
                return 1;
            x = offset - (femaleLength - maleLength + -0.08f);
        } else {
            Real32 front = offset - (maleLength + 0.08f);
            if (Abs(front) > 0.005f && 0.0f > front)
                return 1;
            x = offset - (femaleLength + -0.08f);
        }
        if (Abs(x) > 0.005f && x > 0.0f)
            return 1;
        return result;
    }
    // End stop (kind 17) on an axle: only at the ends.
    Real32 length;
    if (kind == 0x11 && otherKind == 2) {
        length = other.m_length;
    } else {
        if (otherKind != 0x11 || kind != 2)
            return result;
        length = m_length;
    }
    Real32 x = Abs(offset);
    if (sameDirection) {
        if (0.001f > x)
            return 4;
        if (length + -0.08f + -0.001f > x)
            return 1;
        return 0;
    }
    if (0.001f + length > x) {
        if (x > length + -0.001f)
            return 4;
        if (x > 0.081f)
            return 1;
    }
    return 0;
}

// Mac 0x2db1c0, Win 0x4af490
UInt32 ConnectionFieldAxle::CanConnect(const Matrix3D& transform, const ConnectionField& other,
                                       const Matrix3D& otherTransform,
                                       ConnectionIndices& indices) const
{
    if (other.GetType() != m_type)
        return 0;
    const ConnectionFieldAxle& axle = static_cast<const ConnectionFieldAxle&>(other);
    Vector3 up(0, 1, 0);
    Vector3 axis = Rotate(up, transform);
    Vector3 otherAxis = Rotate(Vector3(0, 1, 0), otherTransform);
    bool same = axis.IsEqual(otherAxis, 0.01f);
    if (!same && !axis.IsEqual(-otherAxis, 0.01f))
        return 0;
    if (!transform.GetTranslation().IsOnLine(axis, otherTransform.GetTranslation(), 0.0025f))
        return 0;
    Vector3 farPoint = otherTransform.GetTranslation() + otherAxis * 10.0f;
    if (!transform.GetTranslation().IsOnLine(axis, farPoint, 0.0025f))
        return 0;
    UInt32 result = TestPosition(same, transform.GetTranslation(), otherTransform.GetTranslation(),
                                 axis, otherAxis, axle);
    if ((result & 6) == 0)
        return result;
    if ((m_kind == 4 && other.GetKind() == 5) || (m_kind == 5 && other.GetKind() == 4)) {
        Vector3 z = Rotate(Vector3(0, 0, 1), transform);
        Vector3 otherZ = Rotate(Vector3(0, 0, 1), otherTransform);
        Real32 d = z * otherZ;
        if (!(0.01f > Abs(d)) && !(0.01f > Abs(-1.0f + d)) && !(0.01f > Abs(d + 1.0f)))
            result = 1;
    }
    indices.m_reversed = (!same && result == 4) ? 1 : 0;
    return result;
}

// Mac 0x2dbc20
// Rotates `other` onto this axis (kinds 4 / 5: Z axes snapped to 0 / 90 / 180 degrees), moves
// it to the snapped position along the axis and tests the result.
UInt32 ConnectionFieldAxle::Snap(const Vector3& axis, Vector3& otherAxis, Real32 offset,
                                 Real32 dot, bool sameDirection, const Matrix3D& transform,
                                 const ConnectionFieldAxle& other, const Matrix3D& otherTransform,
                                 Matrix3D& snapped) const
{
    Vector3 c = otherAxis ^ axis;
    if (c.IsAlmostZero()) {
        snapped = otherTransform;
    } else {
        Real32 d = Min(Max(dot, -1.0f), 1.0f);
        Real32 angle = 0.0f;
        if (!(d > 0.99999f)) {
            if (-0.99999f > d)
                angle = PI;
            else
                angle = (Real32)acos((double)d);
        }
        if (!sameDirection)
            angle += -PI;
        snapped.SetRotation(c, angle);
        snapped.Multiply(otherTransform);
    }
    otherAxis = Rotate(Vector3(0, 1, 0), snapped);

    if ((m_kind == 4 && other.m_kind == 5) || (m_kind == 5 && other.m_kind == 4)) {
        Vector3 z = Rotate(Vector3(0, 0, 1), transform);
        Vector3 otherZ = Rotate(Vector3(0, 0, 1), snapped);
        Real32 d = z * otherZ;
        if (!(0.001f > Abs(d)) && !(0.001f > Abs(-1.0f + d)) && !(0.001f > Abs(1.0f + d))) {
            Real32 angle = 0.0f;
            if (!(d > 0.99999f)) {
                if (-0.99999f > d) {
                    angle = PI + -PI;
                } else {
                    angle = (Real32)acos((double)d);
                    if (angle > 2.3561945f)
                        angle += -PI;
                    else if (angle > 0.7853982f)
                        angle += -1.5707964f;
                }
            }
            Matrix3D rotation;
            rotation.SetRotation(otherZ ^ z, angle);
            rotation.Multiply(snapped);
            snapped = rotation;
        }
    }

    UInt32 kind = m_kind;
    UInt32 otherKind = other.m_kind;
    Real32 position;
    if ((kind | 0x10) == 0x13 && otherKind == 2) {
        Real32 step = other.m_step;
        position = step == 0.0f ? offset : RoundToStep(offset, step);
    } else if (kind == 0x11 && otherKind == 2) {
        Real32 end = sameDirection ? 0.0f : other.m_length;
        if (!(offset > end + -0.4f))
            return 0;
        if (!(end + 0.4f > offset))
            return 0;
        position = end;
    } else if (kind == 2 && (otherKind | 0x10) == 0x13) {
        // NOTE: kinds 3 and 19 are odd, so the step of this field is used (both builds).
        Real32 step = (otherKind & 1) == 0 ? other.m_step : m_step;
        position = step == 0.0f ? offset : RoundToStep(offset, step);
    } else if (kind == 2 && otherKind == 0x11) {
        Real32 end = sameDirection ? 0.0f : m_length;
        if (!(offset > end + -0.4f))
            return 0;
        if (!(end + 0.4f > offset))
            return 0;
        position = end;
    } else if (!ClampLinePosition(*this, other, sameDirection, offset, position)) {
        return 0;
    }

    snapped.SetTranslation(transform.GetTranslation() + axis * position);
    return TestPosition(sameDirection, transform.GetTranslation(), snapped.GetTranslation(), axis,
                        otherAxis, other);
}

// Mac 0x2db930, Win 0x4af780
UInt32 ConnectionFieldAxle::CanConnect(const Matrix3D& transform, const ConnectionField& other,
                                       const Matrix3D& otherTransform, Real32 maxDistance,
                                       Real32& distance, Matrix3D& snapped) const
{
    const ConnectionFieldAxle& axle = static_cast<const ConnectionFieldAxle&>(other);
    Vector3 axis;
    Vector3 otherAxis;
    Vector3 up(0, 1, 0);
    Real32 dot;
    bool same;
    Real32 offset;
    if (!FindClosestLines(*this, transform, axle, otherTransform, up, maxDistance, distance, axis,
                          otherAxis, dot, same, offset))
        return 0;
    return Snap(axis, otherAxis, offset, dot, same, transform, axle, otherTransform, snapped);
}

// Mac 0x2dc390, Win 0x4af9e0
UInt32 ConnectionFieldAxle::CanConnect(const Matrix3D& transform, const ConnectionField& other,
                                       const Matrix3D& otherTransform, const Vector3& origin,
                                       const Vector3& direction, Real32 maxDistance,
                                       Real32& distance, Matrix3D& snapped) const
{
    const ConnectionFieldAxle& axle = static_cast<const ConnectionFieldAxle&>(other);
    Vector3 axis;
    Vector3 otherAxis;
    Vector3 up(0, 1, 0);
    Real32 dot;
    bool same;
    Real32 offset;
    // 0.159992 = 0.39999^2 (Mac 0x3e23d4f1)
    if (!FindClosestLinesRay(*this, transform, axle, otherTransform, origin, direction, up,
                             0.159992f, maxDistance, distance, axis, otherAxis, dot, same, offset))
        return 0;
    return Snap(axis, otherAxis, offset, dot, same, transform, axle, otherTransform, snapped);
}

} // namespace LEGO
