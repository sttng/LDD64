// LEGO Digital Designer — restored source
// TU "ConnectionFieldFixed.cpp"  Mac TU 329, 0x2de4a0-0x2def8f (16 functions); Win 0x4e8aa0-
// 0x4e9440. See ConnectionFieldFixed.h. Initialiser Mac 0x2def50: <iostream> only.
#include "ConnectionFieldFixed.h"

#include <iostream>
#include <math.h>

namespace LEGO {

// Mac 0x2de4a0, Win 0x4e8d30
ConnectionFieldFixed* ConnectionFieldFixed::Create(const Matrix3D& transform, UInt32 kind,
                                                   UInt32 axes)
{
    if ((kind & ~1u) == 0x14)
        return new ConnectionFieldFixed(transform, kind, axes, 16.0f, Cos(PI / 180.0f * 78.75f));
    return new ConnectionFieldFixed(transform, kind, axes, 0.4f, Cos(PI / 180.0f * 45.0f));
}

// Win 0x4e8b40
ConnectionFieldFixed::ConnectionFieldFixed(const Matrix3D& transform, UInt32 kind, UInt32 axes,
                                           Real32 snapDistance, Real32 minCosine)
    : ConnectionFieldPointHelper(transform, TYPE_FIXED, kind), m_group(kind >> 1), m_axes(axes),
      m_snapDistance(snapDistance), m_minCosine(minCosine)
{
}

// Win 0x4e8c50; Mac inlined in Clone
ConnectionFieldFixed::ConnectionFieldFixed(const ConnectionFieldFixed& o)
    : Object(), ConnectionFieldPointHelper(o), m_group(o.m_group), m_axes(o.m_axes),
      m_snapDistance(o.m_snapDistance), m_minCosine(o.m_minCosine)
{
}

// Mac 0x2dee30 (D1), 0x2dee60 (D0), 0x2deeb0 / 0x2deef0 (thunks); Win 0x4e8bf0 / 0x4e8d00
ConnectionFieldFixed::~ConnectionFieldFixed()
{
}

// Mac 0x2dec10, Win 0x4e9110
void ConnectionFieldFixed::Connect(ConnectionField& other, const ConnectionIndices& indices,
                                   Attributes* attributes)
{
    static_cast<ConnectionFieldPointHelper&>(other).SetOccupied(true);
    m_occupied = true;
    NotifyConnect(*this, other, indices, attributes);
}

// Mac 0x2dec30, Win 0x4dd1b0
void ConnectionFieldFixed::Disconnect(ConnectionField& other, const ConnectionIndices& indices)
{
    static_cast<ConnectionFieldPointHelper&>(other).SetOccupied(false);
    m_occupied = false;
    NotifyDisconnect(*this, other, indices);
}

// Mac 0x2ded00, Win 0x4e8aa0
void ConnectionFieldFixed::Clone(ConnectionField*& copy) const
{
    copy = new ConnectionFieldFixed(*this);
}

// Mac 0x2dec50, Win 0x4e9160 (same code as ConnectionFieldBall::Pick)
bool ConnectionFieldFixed::Pick(const Matrix3D& transform, const Vector3& origin,
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

// Mac 0x2de620, Win 0x4e8e50
// Same place and axes: 2 (rigid) for a gender pair, else 1 (blocked); 0 when not aligned.
UInt32 ConnectionFieldFixed::CanConnect(const Matrix3D& transform, const ConnectionField& other,
                                        const Matrix3D& otherTransform, ConnectionIndices&) const
{
    if (other.GetType() != m_type)
        return 0;
    if (!(transform.GetTranslation().GetDistanceSquared(otherTransform.GetTranslation()) < 0.0016f))
        return 0;
    if (m_axes == 1) {
        Vector3 y = Rotate(Vector3(0, 1, 0), transform);
        Vector3 otherY = Rotate(Vector3(0, 1, 0), otherTransform);
        if (y.IsEqual(otherY, 0.01f))
            return ((other.GetKind() ^ m_kind) == 1) + 1;
    } else {
        Vector3 x = Rotate(Vector3(1, 0, 0), transform);
        Vector3 otherX = Rotate(Vector3(1, 0, 0), otherTransform);
        Vector3 y = Rotate(Vector3(0, 1, 0), transform);
        Vector3 otherY = Rotate(Vector3(0, 1, 0), otherTransform);
        if (x.IsEqual(otherX, 0.01f) && y.IsEqual(otherY, 0.01f))
            return ((other.GetKind() ^ m_kind) == 1) + 1;
    }
    return 0;
}

// Mac 0x2de890, Win 0x4e9200
UInt32 ConnectionFieldFixed::Snap(const Matrix3D& transform, const ConnectionField& other,
                                  const Matrix3D& otherTransform, const Vector3& point,
                                  Real32 maxDistance, Real32& distance, Matrix3D& snapped) const
{
    if (IsOccupied() || other.IsOccupied())
        return 0;
    Vector3 y = Rotate(Vector3(0, 1, 0), transform);
    Vector3 otherY = Rotate(Vector3(0, 1, 0), otherTransform);
    Real32 d = y * otherY;
    if (m_minCosine > d)
        return 0;
    if (m_axes == 1) {
        if (0.99999f > d) {
            Vector3 axis = otherY ^ y;
            Real32 angle = 0.0f;
            if (!(d > 0.99999f)) {
                if (-0.99999f > d)
                    angle = PI;
                else
                    angle = (Real32)acos((double)d);
            }
            snapped.SetRotation(axis, angle);
            snapped *= otherTransform;
        } else {
            snapped = otherTransform;
        }
        snapped.SetTranslation(transform.GetTranslation());
    } else {
        Vector3 x = Rotate(Vector3(1, 0, 0), transform);
        Vector3 otherX = Rotate(Vector3(1, 0, 0), otherTransform);
        if (m_minCosine > x * otherX)
            return 0;
        snapped = transform;
    }
    Real32 dist = point.GetDistanceSquared(snapped.GetTranslation());
    distance = dist;
    return dist < maxDistance ? CONNECTION_RIGID : 0;
}

// Mac 0x2de7f0, Win 0x4e9000
UInt32 ConnectionFieldFixed::CanConnect(const Matrix3D& transform, const ConnectionField& other,
                                        const Matrix3D& otherTransform, Real32 maxDistance,
                                        Real32& distance, Matrix3D& snapped) const
{
    Real32 d = otherTransform.GetTranslation().GetDistanceSquared(transform.GetTranslation());
    if (maxDistance <= d || m_snapDistance <= d)
        return 0;
    return Snap(transform, other, otherTransform, otherTransform.GetTranslation(), maxDistance,
                distance, snapped);
}

// Mac 0x2deb60, Win 0x4e9070
UInt32 ConnectionFieldFixed::CanConnect(const Matrix3D& transform, const ConnectionField& other,
                                        const Matrix3D& otherTransform, const Vector3& origin,
                                        const Vector3&, Real32 maxDistance, Real32& distance,
                                        Matrix3D& snapped) const
{
    Vector3 ray = otherTransform.GetTranslation() - origin;
    ray.Normalize();
    if (!(PointRayDistanceSquared(origin, ray, transform.GetTranslation()) < m_snapDistance))
        return 0;
    return Snap(transform, other, otherTransform, origin, maxDistance, distance, snapped);
}

} // namespace LEGO
