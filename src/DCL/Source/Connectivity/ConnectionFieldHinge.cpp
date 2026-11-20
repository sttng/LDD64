// LEGO Digital Designer — restored source
// TU "ConnectionFieldHinge.cpp"  Mac TU 330, 0x2def90-0x2e016f (16 functions); Win 0x4df330-
// 0x4dfe90. See ConnectionFieldHinge.h. Initialiser Mac 0x2e0130: <iostream> only.
#include "ConnectionFieldHinge.h"

#include <iostream>
#include <math.h>

namespace LEGO {

namespace {

// acos with the argument ends snapped (inlined everywhere in this file). (name inferred)
inline Real32 AngleOf(Real32 d)
{
    if (d > 0.99999f)
        return 0.0f;
    if (-0.99999f > d)
        return PI;
    return (Real32)acos((double)d);
}

} // namespace

// Mac 0x2def90
ConnectionFieldHinge* ConnectionFieldHinge::Create(const Matrix3D& transform, UInt32 kind,
                                                   bool oriented, bool hasLimits,
                                                   Real32 minAngle, Real32 maxAngle,
                                                   Real32 flipMinAngle, Real32 flipMaxAngle)
{
    return new ConnectionFieldHinge(transform, kind, oriented, hasLimits, minAngle, maxAngle,
                                    flipMinAngle, flipMaxAngle);
}

// Win 0x4df3f0
ConnectionFieldHinge::ConnectionFieldHinge(const Matrix3D& transform, UInt32 kind, bool oriented,
                                           bool hasLimits, Real32 minAngle, Real32 maxAngle,
                                           Real32 flipMinAngle, Real32 flipMaxAngle)
    : ConnectionFieldPointHelper(transform, TYPE_HINGE, kind), m_group(kind >> 1),
      m_oriented(oriented), m_reversed(false), m_hasLimits(hasLimits), m_minAngle(minAngle),
      m_maxAngle(maxAngle), m_flipMinAngle(flipMinAngle), m_flipMaxAngle(flipMaxAngle)
{
}

// Win 0x4df520; Mac inlined in Clone
ConnectionFieldHinge::ConnectionFieldHinge(const ConnectionFieldHinge& o)
    : Object(), ConnectionFieldPointHelper(o), m_group(o.m_group), m_oriented(o.m_oriented),
      m_reversed(o.m_reversed), m_hasLimits(o.m_hasLimits), m_minAngle(o.m_minAngle),
      m_maxAngle(o.m_maxAngle), m_flipMinAngle(o.m_flipMinAngle), m_flipMaxAngle(o.m_flipMaxAngle)
{
}

// Mac 0x2e0010 (D1), 0x2e0040 (D0), 0x2e0090 / 0x2e00d0 (thunks); Win 0x4df4c0 / 0x4df5e0
ConnectionFieldHinge::~ConnectionFieldHinge()
{
}

// Mac 0x2dfd10, Win 0x4dfc40
// The male (even kind) side provides the limits; NOTE: for a female `this` the flip limits of
// `other` are used whatever the direction (both builds). `attributes` must not be NULL.
void ConnectionFieldHinge::Connect(ConnectionField& other, const ConnectionIndices& indices,
                                   Attributes* attributes)
{
    ConnectionFieldHinge& hinge = static_cast<ConnectionFieldHinge&>(other);
    hinge.m_occupied = true;
    m_occupied = true;
    UInt32 reversed = indices.m_reversed;
    if ((m_kind & 1) == 0) {
        hinge.m_reversed = reversed != 0;
        attributes->m_hasLimits = m_hasLimits;
        if (reversed == 0) {
            attributes->m_lowerLimit.x = m_minAngle;
            attributes->m_upperLimit.x = m_maxAngle;
        } else {
            attributes->m_lowerLimit.x = m_flipMinAngle;
            attributes->m_upperLimit.x = m_flipMaxAngle;
        }
    } else {
        m_reversed = reversed != 0;
        attributes->m_hasLimits = hinge.m_hasLimits;
        attributes->m_lowerLimit.x = -hinge.m_flipMaxAngle;
        attributes->m_upperLimit.x = -hinge.m_flipMinAngle;
    }
    GetFrame(true, other, indices, attributes->m_axis, attributes->m_normal, attributes->m_position);
    GetFrame(false, other, indices, attributes->m_otherAxis, attributes->m_otherNormal,
             attributes->m_otherPosition);
    NotifyConnect(*this, other, indices, attributes);
}

// Mac 0x2dfdf0, Win 0x4dfd20
void ConnectionFieldHinge::Disconnect(ConnectionField& other, const ConnectionIndices& indices)
{
    ConnectionFieldHinge& hinge = static_cast<ConnectionFieldHinge&>(other);
    hinge.m_occupied = false;
    m_occupied = false;
    hinge.m_reversed = false;
    m_reversed = false;
    NotifyDisconnect(*this, other, indices);
}

// Mac 0x2dfee0, Win 0x4df330
void ConnectionFieldHinge::Clone(ConnectionField*& copy) const
{
    copy = new ConnectionFieldHinge(*this);
}

// Mac 0x2dfe20, Win 0x4dfd70
// Axis -Y of the field, +Y once connected reversed; normal -X.
void ConnectionFieldHinge::GetFrame(bool self, const ConnectionField& other,
                                    const ConnectionIndices&, Vector3& axis, Vector3& normal,
                                    Vector3& position) const
{
    static const Real32 s_axisSign[2] = { -1.0f, 1.0f };            // Mac 0x63c400
    const ConnectionFieldHinge& field = self ? *this : static_cast<const ConnectionFieldHinge&>(other);
    const Matrix3D& m = field.m_transform;
    axis = Rotate(Vector3(0, s_axisSign[field.m_reversed ? 1 : 0], 0), m);
    normal = Rotate(Vector3(-1.0f, 0, 0), m);
    position = m.GetTranslation();
}

// Mac 0x2df090, Win 0x4df6b0
UInt32 ConnectionFieldHinge::CanConnect(const Matrix3D& transform, const ConnectionField& other,
                                        const Matrix3D& otherTransform,
                                        ConnectionIndices& indices) const
{
    if (other.GetType() != m_type)
        return 0;
    if (!(transform.GetTranslation().GetDistanceSquared(otherTransform.GetTranslation()) < 0.0016f))
        return 0;
    static const Vector3 s_xAxis(1, 0, 0);                          // Mac 0x77ede0
    static const Vector3 s_yAxis(0, 1, 0);                          // Mac 0x77edf0
    Vector3 y = Rotate(s_yAxis, transform);
    Vector3 otherY = Rotate(s_yAxis, otherTransform);
    bool same = y.IsEqual(otherY, 0.01f);
    if (!same) {
        if (m_oriented || !y.IsEqual(-otherY, 0.01f))
            return 0;
    }
    if ((other.GetKind() ^ m_kind) != 1)
        return CONNECTION_BLOCKED;

    const ConnectionFieldHinge& hinge = static_cast<const ConnectionFieldHinge&>(other);
    if (hinge.m_hasLimits || m_hasLimits) {
        Vector3 x = Rotate(s_xAxis, transform);
        Vector3 otherX = Rotate(s_xAxis, otherTransform);
        if (!x.IsEqual(otherX, EPSILON)) {
            Vector3 n;
            const ConnectionFieldHinge* limits;
            bool flip;
            if ((m_kind & 1) == 0) {
                n = x ^ otherX;
                n.Normalize();
                flip = y.IsEqual(n, 0.01f);
                limits = this;
            } else {
                n = otherX ^ x;
                n.Normalize();
                flip = otherY.IsEqual(n, 0.01f);
                limits = &hinge;
            }
            Real32 angle = AngleOf(x * otherX);
            if (flip)
                angle = -angle;
            Real32 maxAngle;
            if (same) {
                if (angle < limits->m_minAngle + -0.001f)
                    return CONNECTION_BLOCKED;
                maxAngle = limits->m_maxAngle;
            } else {
                if (angle < limits->m_flipMinAngle + -0.001f)
                    return CONNECTION_BLOCKED;
                maxAngle = limits->m_flipMaxAngle;
            }
            if (maxAngle + 0.001f < angle)
                return CONNECTION_BLOCKED;
        }
    }
    indices.m_reversed = !same;
    return CONNECTION_FREE;
}

// Mac 0x2df550, Win 0x4dfe90 (+ inlined parts)
UInt32 ConnectionFieldHinge::Snap(const Matrix3D& transform, const ConnectionFieldHinge& other,
                                  const Matrix3D& otherTransform, const Vector3& point,
                                  Real32 maxDistance, Real32& distance, Matrix3D& snapped) const
{
    if (IsOccupied() || other.IsOccupied())
        return 0;
    static const Vector3 s_xAxis(1, 0, 0);                          // Mac 0x775d48
    static const Vector3 s_yAxis(0, 1, 0);                          // Mac 0x775d5c
    Vector3 y = Rotate(s_yAxis, transform);
    Vector3 otherY = Rotate(s_yAxis, otherTransform);
    Real32 d = y * otherY;
    if (!(0.70710677f <= d)) {
        if (d > -0.70710677f)
            return 0;
        if (m_oriented)
            return 0;
    }
    if (0.99999f > d && d > -0.99999f) {
        Vector3 axis;
        Real32 angle;
        if (d >= 0.0f) {
            axis = otherY ^ y;
            angle = AngleOf(d);
        } else {
            axis = y ^ otherY;
            angle = PI - AngleOf(d);
        }
        snapped.SetRotation(axis, angle);
        snapped *= otherTransform;
    } else {
        snapped = otherTransform;
    }

    if (other.m_hasLimits || m_hasLimits) {
        otherY = Rotate(s_yAxis, snapped);
        Vector3 x = Rotate(s_xAxis, transform);
        Vector3 otherX = Rotate(s_xAxis, snapped);
        Matrix3D rotation;
        Vector3 n;
        bool same = y.IsEqual(otherY, 0.01f);
        Real32 angle;
        Real32 correction;
        if ((m_kind & 1) == 0) {
            n = x ^ otherX;
            if (n != Vector3()) {
                n.Normalize();
                bool flip = y.IsEqual(n, 0.01f);
                angle = AngleOf(x * otherX);
                if (flip)
                    angle = -angle;
            } else {
                angle = x.IsEqual(otherX, EPSILON) ? 0.0f : PI;
            }
            // NOTE: with the limits of this field the correction for the same direction has
            // the opposite sign of all other cases (both builds).
            if (same) {
                if (angle > m_maxAngle)
                    correction = -(m_maxAngle - angle);
                else
                    correction = angle < m_minAngle ? -(m_minAngle - angle) : 0.0f;
            } else {
                if (angle > m_flipMaxAngle)
                    correction = m_flipMaxAngle - angle;
                else
                    correction = angle < m_flipMinAngle ? m_flipMinAngle - angle : 0.0f;
            }
        } else {
            n = otherX ^ x;
            if (n.IsAlmostZero()) {
                angle = x.IsEqual(otherX, EPSILON) ? 0.0f : PI;
            } else {
                n.Normalize();
                bool flip = otherY.IsEqual(n, 0.01f);
                angle = AngleOf(x * otherX);
                if (flip)
                    angle = -angle;
            }
            if (same) {
                if (angle > other.m_maxAngle)
                    correction = other.m_maxAngle - angle;
                else
                    correction = angle < other.m_minAngle ? other.m_minAngle - angle : 0.0f;
            } else {
                if (angle > other.m_flipMaxAngle)
                    correction = other.m_flipMaxAngle - angle;
                else
                    correction = angle < other.m_flipMinAngle ? other.m_flipMinAngle - angle : 0.0f;
            }
        }
        rotation.SetRotation(otherY, correction);
        rotation.Multiply(snapped);
        snapped = rotation;
    }

    snapped.SetTranslation(transform.GetTranslation());
    Real32 dist = point.GetDistanceSquared(snapped.GetTranslation());
    distance = dist;
    return dist >= maxDistance ? 0 : CONNECTION_SNAP;
}

// Mac 0x2df4a0, Win 0x4dfb30
UInt32 ConnectionFieldHinge::CanConnect(const Matrix3D& transform, const ConnectionField& other,
                                        const Matrix3D& otherTransform, Real32 maxDistance,
                                        Real32& distance, Matrix3D& snapped) const
{
    Real32 d = otherTransform.GetTranslation().GetDistanceSquared(transform.GetTranslation());
    if (maxDistance <= d || 0.4f <= d)
        return 0;
    return Snap(transform, static_cast<const ConnectionFieldHinge&>(other), otherTransform,
                otherTransform.GetTranslation(), maxDistance, distance, snapped);
}

// Mac 0x2dfc50, Win 0x4dfba0
UInt32 ConnectionFieldHinge::CanConnect(const Matrix3D& transform, const ConnectionField& other,
                                        const Matrix3D& otherTransform, const Vector3& origin,
                                        const Vector3&, Real32 maxDistance, Real32& distance,
                                        Matrix3D& snapped) const
{
    Vector3 ray = otherTransform.GetTranslation() - origin;
    ray.Normalize();
    if (!(PointRayDistanceSquared(origin, ray, transform.GetTranslation()) < 0.4f))
        return 0;
    return Snap(transform, static_cast<const ConnectionFieldHinge&>(other), otherTransform, origin,
                maxDistance, distance, snapped);
}

} // namespace LEGO
