// LEGO Digital Designer — restored source
// TU "ConnectionFieldBall.cpp"  Mac TU 328, 0x2dd980-0x2de49f (23 functions); Win 0x4dcbc0-
// 0x4dd2e0. See ConnectionFieldBall.h.
//
// Also emitted here on Mac (ConnectionField.h inlines, defined in ConnectionField.cpp):
// 0x2de050 / 0x2de080 PointHelper::IsCompatible (2), 0x2de0c0 PointHelper::IsOccupied,
// 0x2de0d0-0x2de1a0 / 0x2de3c0-0x2de400 ~ConnectionFieldPointHelper and Object thunks.
// Initialiser 0x2de460: <iostream> only.
#include "ConnectionFieldBall.h"

#include <iostream>

namespace LEGO {

// Mac 0x2dd980
ConnectionFieldBall* ConnectionFieldBall::Create(const Matrix3D& transform, UInt32 kind)
{
    return new ConnectionFieldBall(transform, kind);
}

// Win 0x4dd2e0
ConnectionFieldBall::ConnectionFieldBall(const Matrix3D& transform, UInt32 kind)
    : ConnectionFieldPointHelper(transform, TYPE_BALL, kind), m_group(kind >> 1),
      m_lowerLimit(-PI), m_upperLimit(PI), m_twist(PI), m_flexX(0.0f), m_flexY(0.0f),
      m_hasLimits(false)
{
}

// Win 0x4dccb0; Mac inlined in Clone
ConnectionFieldBall::ConnectionFieldBall(const ConnectionFieldBall& o)
    : Object(), ConnectionFieldPointHelper(o), m_group(o.m_group), m_lowerLimit(o.m_lowerLimit),
      m_upperLimit(o.m_upperLimit), m_twist(o.m_twist), m_flexX(o.m_flexX), m_flexY(o.m_flexY),
      m_hasLimits(o.m_hasLimits)
{
}

// Mac 0x2de340 (D1), 0x2de370 (D0); Win 0x4dcc50 / 0x4dcd70
ConnectionFieldBall::~ConnectionFieldBall()
{
}

// Mac 0x2de010
void ConnectionFieldBall::SetLimits(Real32 lowerLimit, Real32 upperLimit, Real32 twist,
                                    Real32 flexX, Real32 flexY)
{
    m_lowerLimit = lowerLimit;
    m_upperLimit = upperLimit;
    m_twist = twist;
    m_flexX = flexX;
    m_flexY = flexY;
    m_hasLimits = true;
}

// Mac 0x2ddd20, Win 0x4dd070
// NOTE: the joint attributes are only filled for m_group > 499499 (kind > 998999), which no
// primitive uses (both builds); `attributes` is not checked for NULL in that case.
void ConnectionFieldBall::Connect(ConnectionField& other, const ConnectionIndices& indices,
                                  Attributes* attributes)
{
    const ConnectionFieldBall& ball = static_cast<const ConnectionFieldBall&>(other);
    static_cast<ConnectionFieldPointHelper&>(other).SetOccupied(true);
    m_occupied = true;
    if (m_group > 0x79f2b) {
        attributes->m_isFlexible = true;
        attributes->m_hasLinear = true;
        attributes->m_hasLimits = true;
        if (!m_hasLimits) {
            GetFrame(false, other, indices, attributes->m_axis, attributes->m_normal,
                     attributes->m_position);
            GetFrame(true, other, indices, attributes->m_otherAxis, attributes->m_otherNormal,
                     attributes->m_otherPosition);
            attributes->m_linearLimits = Vector3(ball.m_flexX, ball.m_flexY, ball.m_flexY);
            attributes->m_lowerLimit.x = -ball.m_upperLimit;
            attributes->m_upperLimit.x = -ball.m_lowerLimit;
            attributes->m_twist = ball.m_twist;
        } else {
            GetFrame(true, other, indices, attributes->m_axis, attributes->m_normal,
                     attributes->m_position);
            GetFrame(false, other, indices, attributes->m_otherAxis, attributes->m_otherNormal,
                     attributes->m_otherPosition);
            attributes->m_linearLimits = Vector3(m_flexX, m_flexY, m_flexY);
            attributes->m_lowerLimit.x = m_lowerLimit;
            attributes->m_upperLimit.x = m_upperLimit;
            attributes->m_twist = m_twist;
        }
    }
    NotifyConnect(*this, other, indices, attributes);
}

// Mac 0x2ddec0, Win 0x4dd1b0 (shared with ConnectionFieldFixed)
void ConnectionFieldBall::Disconnect(ConnectionField& other, const ConnectionIndices& indices)
{
    static_cast<ConnectionFieldPointHelper&>(other).SetOccupied(false);
    m_occupied = false;
    NotifyDisconnect(*this, other, indices);
}

// Mac 0x2de200, Win 0x4dcbc0
void ConnectionFieldBall::Clone(ConnectionField*& copy) const
{
    copy = new ConnectionFieldBall(*this);
}

// Mac 0x2ddee0, Win 0x4e9160 (shared with ConnectionFieldFixed)
bool ConnectionFieldBall::Pick(const Matrix3D& transform, const Vector3& origin,
                               const Vector3& direction, Real32 maxDistance, Real32& distance,
                               UInt32& index, Matrix3D& offset) const
{
    if (m_occupied)
        return false;
    Real32 d = origin.GetDistanceSquared(transform.GetTranslation());
    distance = d;
    if (!(d < maxDistance))
        return false;
    index = 0;
    offset.SetIdentity();
    return PointRayDistanceSquared(origin, direction, transform.GetTranslation()) < 0.16000001f;
}

// Mac 0x2ddf80, Win 0x4dd200
void ConnectionFieldBall::GetFrame(bool self, const ConnectionField& other,
                                   const ConnectionIndices&, Vector3& axis, Vector3& normal,
                                   Vector3& position) const
{
    const Matrix3D& m = (self ? static_cast<const ConnectionField&>(*this) : other).GetTransform();
    axis = Rotate(Vector3(0, 1, 0), m);
    normal = Rotate(Vector3(0, 0, 1), m);
    position = m.GetTranslation();
}

// Mac 0x2dda60, Win 0x4dce90
UInt32 ConnectionFieldBall::CanConnect(const Matrix3D& transform, const ConnectionField& other,
                                       const Matrix3D& otherTransform, ConnectionIndices&) const
{
    if (other.GetType() == m_type && (other.GetKind() ^ m_kind) == 1) {
        if (transform.GetTranslation().GetDistanceSquared(otherTransform.GetTranslation()) < 0.0016f)
            return CONNECTION_FREE;
    }
    return 0;
}

// Mac 0x2ddad0, Win 0x4dcee0
UInt32 ConnectionFieldBall::CanConnect(const Matrix3D& transform, const ConnectionField& other,
                                       const Matrix3D& otherTransform, Real32 maxDistance,
                                       Real32& distance, Matrix3D& snapped) const
{
    if (IsOccupied() || other.IsOccupied())
        return 0;
    Real32 d = otherTransform.GetTranslation().GetDistanceSquared(transform.GetTranslation());
    if (d < maxDistance && d < 0.4f) {
        snapped = otherTransform;
        snapped.SetTranslation(transform.GetTranslation());
        distance = d;
        return CONNECTION_SNAP;
    }
    return 0;
}

// Mac 0x2ddbe0, Win 0x4dcf80
UInt32 ConnectionFieldBall::CanConnect(const Matrix3D& transform, const ConnectionField& other,
                                       const Matrix3D& otherTransform, const Vector3& origin,
                                       const Vector3&, Real32 maxDistance, Real32& distance,
                                       Matrix3D& snapped) const
{
    if (IsOccupied() || other.IsOccupied())
        return 0;
    Vector3 ray = otherTransform.GetTranslation() - origin;
    ray.Normalize();
    if (!(PointRayDistanceSquared(origin, ray, transform.GetTranslation()) < 0.4f))
        return 0;
    snapped = otherTransform;
    snapped.SetTranslation(transform.GetTranslation());
    Real32 d = origin.GetDistanceSquared(snapped.GetTranslation());
    distance = d;
    return maxDistance <= d ? 0 : CONNECTION_SNAP;
}

} // namespace LEGO
