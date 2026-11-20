// LEGO Digital Designer — restored source
// TU "ConnectionFieldRail.cpp"  Mac TU 332, 0x2e98f0-0x2eaaef (21 functions); Win 0x4dbb50-
// 0x4dc8f0. See ConnectionFieldRail.h. Initialiser Mac 0x2eaab0: <iostream> only.
// Line helpers (TestLineOverlap Mac 0x2ea600, FindClosestLines 0x2e9cc0, FindClosestLinesRay
// 0x2ea1d0): ConnectionFieldLineFunctions.h.
#include "ConnectionFieldRail.h"
#include "ConnectionFieldLineFunctions.h"

#include <iostream>

namespace LEGO {

// Mac 0x2e98f0, Win 0x4dbde0
ConnectionFieldRail* ConnectionFieldRail::Create(const Matrix3D& transform, UInt32 kind,
                                                 Real32 length)
{
    return new ConnectionFieldRail(transform, kind, length);
}

// Win 0x4dbc20
ConnectionFieldRail::ConnectionFieldRail(const Matrix3D& transform, UInt32 kind, Real32 length)
    : ConnectionFieldLineHelper(transform, TYPE_RAIL, kind, length, false, false),
      m_occupied(false)
{
}

// Win 0x4dbd20; Mac inlined in Clone
ConnectionFieldRail::ConnectionFieldRail(const ConnectionFieldRail& o)
    : Object(), ConnectionFieldLineHelper(o), m_occupied(o.m_occupied)
{
}

// Mac 0x2ea990 (D1), 0x2ea9c0 (D0), 0x2eaa10 / 0x2eaa50 (thunks); Win 0x4dbcc0 / 0x4dbdb0
ConnectionFieldRail::~ConnectionFieldRail()
{
}

// Mac 0x2ea510, Win 0x4dc760
// Only the female (odd kind) side becomes occupied.
void ConnectionFieldRail::Connect(ConnectionField& other, const ConnectionIndices& indices,
                                  Attributes* attributes)
{
    ConnectionFieldRail& female = (m_kind & 1) == 0 ? static_cast<ConnectionFieldRail&>(other) : *this;
    female.m_occupied = true;
    NotifyConnect(*this, other, indices, attributes);
}

// Mac 0x2ea530, Win 0x4dc7b0
void ConnectionFieldRail::Disconnect(ConnectionField& other, const ConnectionIndices& indices)
{
    ConnectionFieldRail& female = (m_kind & 1) == 0 ? static_cast<ConnectionFieldRail&>(other) : *this;
    female.m_occupied = false;
    NotifyDisconnect(*this, other, indices);
}

// Mac 0x2ea870, Win 0x4dbb80
void ConnectionFieldRail::Clone(ConnectionField*& copy) const
{
    copy = new ConnectionFieldRail(*this);
}

// Mac 0x2ea570, Win 0x4dc800
void ConnectionFieldRail::GetFrame(bool self, const ConnectionField& other,
                                   const ConnectionIndices&, Vector3& axis, Vector3& normal,
                                   Vector3& position) const
{
    const Matrix3D& m = (self ? static_cast<const ConnectionField&>(*this) : other).GetTransform();
    axis = Rotate(Vector3(0, 1, 0), m);
    normal = Rotate(Vector3(0, 0, -1.0f), m);
    position = m.GetTranslation();
}

// Mac 0x2e99c0, Win 0x4dbe50
UInt32 ConnectionFieldRail::CanConnect(const Matrix3D& transform, const ConnectionField& other,
                                       const Matrix3D& otherTransform, ConnectionIndices&) const
{
    if (other.GetType() != m_type || (other.GetKind() ^ m_kind) != 1)
        return 0;
    const ConnectionFieldRail& rail = static_cast<const ConnectionFieldRail&>(other);
    Vector3 y = Rotate(Vector3(0, 1, 0), transform);
    Vector3 otherY = Rotate(Vector3(0, 1, 0), otherTransform);
    if (!y.IsEqual(otherY, 0.01f))
        return 0;
    Vector3 axis = Rotate(Vector3(0, 0, -1.0f), transform);
    Vector3 otherAxis = Rotate(Vector3(0, 0, -1.0f), otherTransform);
    bool same = axis.IsEqual(otherAxis, 0.01f);
    if (!same && !axis.IsEqual(-otherAxis, 0.01f))
        return 0;
    if (!transform.GetTranslation().IsOnLine(axis, otherTransform.GetTranslation(), 0.01f))
        return 0;
    const ConnectionFieldLineHelper* female;
    const ConnectionFieldLineHelper* male;
    Real32 offset;
    return TestLineOverlap(*this, same, transform.GetTranslation(), axis, m_length,
                           otherTransform.GetTranslation(), otherAxis, rail.m_length, rail,
                           &female, &male, &offset, CONNECTION_RIGID);
}

// Mac 0x2e9eb0, Win 0x4dc8f0
UInt32 ConnectionFieldRail::Snap(const Vector3& axis, const Vector3& otherAxis, Real32 offset,
                                 Real32, bool sameDirection, const Matrix3D& transform,
                                 const ConnectionFieldRail& other, const Matrix3D&,
                                 Matrix3D& snapped) const
{
    snapped = transform;
    if (!sameDirection) {
        Matrix3D rotation;
        Vector3 y = Rotate(Vector3(0, 1, 0), snapped);
        rotation.SetRotation(y, PI);
        rotation.Multiply(snapped);
        snapped = rotation;
    }
    snapped.SetTranslation(transform.GetTranslation() + axis * offset);
    const ConnectionFieldLineHelper* female;
    const ConnectionFieldLineHelper* male;
    Real32 position;
    return TestLineOverlap(*this, sameDirection, transform.GetTranslation(), axis, m_length,
                           snapped.GetTranslation(), otherAxis, other.m_length, other, &female,
                           &male, &position, CONNECTION_RIGID);
}

// Mac 0x2e9bb0, Win 0x4dbfe0
UInt32 ConnectionFieldRail::CanConnect(const Matrix3D& transform, const ConnectionField& other,
                                       const Matrix3D& otherTransform, Real32 maxDistance,
                                       Real32& distance, Matrix3D& snapped) const
{
    if (IsOccupied() || other.IsOccupied())
        return 0;
    const ConnectionFieldRail& rail = static_cast<const ConnectionFieldRail&>(other);
    Vector3 axis;
    Vector3 otherAxis;
    Vector3 up(0, 0, -1.0f);
    Real32 dot;
    bool same;
    Real32 offset;
    if (!FindClosestLines(*this, transform, rail, otherTransform, up, maxDistance, distance, axis,
                          otherAxis, dot, same, offset))
        return 0;
    return Snap(axis, otherAxis, offset, dot, same, transform, rail, otherTransform, snapped);
}

// Mac 0x2ea0b0, Win 0x4dc270
UInt32 ConnectionFieldRail::CanConnect(const Matrix3D& transform, const ConnectionField& other,
                                       const Matrix3D& otherTransform, const Vector3& origin,
                                       const Vector3& direction, Real32 maxDistance,
                                       Real32& distance, Matrix3D& snapped) const
{
    if (IsOccupied() || other.IsOccupied())
        return 0;
    const ConnectionFieldRail& rail = static_cast<const ConnectionFieldRail&>(other);
    Vector3 axis;
    Vector3 otherAxis;
    Vector3 up(0, 0, -1.0f);
    Real32 dot;
    bool same;
    Real32 offset;
    if (!FindClosestLinesRay(*this, transform, rail, otherTransform, origin, direction, up, 3.0f,
                             maxDistance, distance, axis, otherAxis, dot, same, offset))
        return 0;
    return Snap(axis, otherAxis, offset, dot, same, transform, rail, otherTransform, snapped);
}

} // namespace LEGO
