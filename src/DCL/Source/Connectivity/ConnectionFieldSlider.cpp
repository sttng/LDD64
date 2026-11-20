// LEGO Digital Designer — restored source
// TU "ConnectionFieldSlider.cpp"  Mac TU 333, 0x2eaaf0-0x2ebeaf (18 functions); Win 0x4de020-
// 0x4def50. See ConnectionFieldSlider.h. Initialiser Mac 0x2ebe70: <iostream> only.
// Line helpers (TestLineOverlap Mac 0x2eba20, FindClosestLines 0x2eaf50, FindClosestLinesRay
// 0x2eb680): ConnectionFieldLineFunctions.h.
#include "ConnectionFieldSlider.h"
#include "ConnectionFieldLineFunctions.h"

#include <iostream>

namespace LEGO {

// Mac 0x2eaaf0, Win 0x4de280
ConnectionFieldSlider* ConnectionFieldSlider::Create(const Matrix3D& transform, UInt32 kind,
                                                     Real32 length, bool startCapped,
                                                     bool endCapped)
{
    return new ConnectionFieldSlider(transform, kind, length, startCapped, endCapped);
}

// Win 0x4de0b0
ConnectionFieldSlider::ConnectionFieldSlider(const Matrix3D& transform, UInt32 kind, Real32 length,
                                             bool startCapped, bool endCapped)
    : ConnectionFieldLineHelper(transform, TYPE_SLIDER, kind, length, startCapped, endCapped),
      m_connectionCount(0)
{
}

// Win 0x4de1c0; Mac inlined in Clone
ConnectionFieldSlider::ConnectionFieldSlider(const ConnectionFieldSlider& o)
    : Object(), ConnectionFieldLineHelper(o), m_connectionCount(o.m_connectionCount)
{
}

// Mac 0x2ebd50 (D1), 0x2ebd80 (D0), 0x2ebdd0 / 0x2ebe10 (thunks); Win 0x4de160 / 0x4de250
ConnectionFieldSlider::~ConnectionFieldSlider()
{
}

// Mac 0x2eb9c0, Win 0x4dec20
void ConnectionFieldSlider::Connect(ConnectionField& other, const ConnectionIndices& indices,
                                    Attributes* attributes)
{
    ++m_connectionCount;
    ++static_cast<ConnectionFieldSlider&>(other).m_connectionCount;
    NotifyConnect(*this, other, indices, attributes);
}

// Mac 0x2eb9e0, Win 0x4dec70
void ConnectionFieldSlider::Disconnect(ConnectionField& other, const ConnectionIndices& indices)
{
    --m_connectionCount;
    --static_cast<ConnectionFieldSlider&>(other).m_connectionCount;
    NotifyDisconnect(*this, other, indices);
}

// Mac 0x2ebc40, Win 0x4de020
void ConnectionFieldSlider::Clone(ConnectionField*& copy) const
{
    copy = new ConnectionFieldSlider(*this);
}

// Mac 0x2eabd0, Win 0x4de300
// Collinear, overlapping gender pair: 4 (sliding) when the X axes are also parallel, else 1
// (blocked); 0 or the TestLineOverlap result before that.
UInt32 ConnectionFieldSlider::CanConnect(const Matrix3D& transform, const ConnectionField& other,
                                         const Matrix3D& otherTransform, ConnectionIndices&) const
{
    if (other.GetType() != m_type || (other.GetKind() ^ m_kind) != 1)
        return 0;
    const ConnectionFieldSlider& slider = static_cast<const ConnectionFieldSlider&>(other);
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
    const ConnectionFieldLineHelper* female;
    const ConnectionFieldLineHelper* male;
    Real32 offset;
    UInt32 result = TestLineOverlap(*this, same, transform.GetTranslation(), axis, m_length,
                                    otherTransform.GetTranslation(), otherAxis, slider.m_length,
                                    slider, &female, &male, &offset, CONNECTION_RIGID);
    if ((result & 6) == 0)
        return result;
    Vector3 x = Rotate(Vector3(1.0f, 0, 0), transform);
    Vector3 otherX = Rotate(Vector3(1.0f, 0, 0), otherTransform);
    return (Abs(x * otherX + -1.0f) < 0.01f) * 3 + 1;
}

// Mac 0x2eb140, Win 0x4def50
UInt32 ConnectionFieldSlider::Snap(const Vector3& axis, const Vector3& otherAxis, Real32 offset,
                                   Real32, bool sameDirection, const Matrix3D& transform,
                                   const ConnectionFieldSlider& other,
                                   const Matrix3D& otherTransform, Matrix3D& snapped) const
{
    snapped = transform;
    if (!sameDirection) {
        Matrix3D rotation;
        Vector3 x = Rotate(Vector3(1.0f, 0, 0), snapped);
        rotation.SetRotation(x, PI);
        rotation.Multiply(snapped);
        snapped = rotation;
    }
    Vector3 x = Rotate(Vector3(1.0f, 0, 0), snapped);
    Vector3 otherX = Rotate(Vector3(1.0f, 0, 0), otherTransform);
    if (x * otherX < 0.70710677f)
        return 0;
    Real32 position;
    if (!ClampLinePosition(*this, other, sameDirection, offset, position))
        return 0;
    snapped.SetTranslation(transform.GetTranslation() + axis * position);
    const ConnectionFieldLineHelper* female;
    const ConnectionFieldLineHelper* male;
    Real32 overlap;
    return TestLineOverlap(*this, sameDirection, transform.GetTranslation(), axis, m_length,
                           snapped.GetTranslation(), otherAxis, other.m_length, other, &female,
                           &male, &overlap, CONNECTION_RIGID);
}

// Mac 0x2eae50, Win 0x4de520
UInt32 ConnectionFieldSlider::CanConnect(const Matrix3D& transform, const ConnectionField& other,
                                         const Matrix3D& otherTransform, Real32 maxDistance,
                                         Real32& distance, Matrix3D& snapped) const
{
    const ConnectionFieldSlider& slider = static_cast<const ConnectionFieldSlider&>(other);
    Vector3 axis;
    Vector3 otherAxis;
    Vector3 up(0, 1, 0);
    Real32 dot;
    bool same;
    Real32 offset;
    if (!FindClosestLines(*this, transform, slider, otherTransform, up, maxDistance, distance, axis,
                          otherAxis, dot, same, offset))
        return 0;
    return Snap(axis, otherAxis, offset, dot, same, transform, slider, otherTransform, snapped);
}

// Mac 0x2eb570, Win 0x4de770
UInt32 ConnectionFieldSlider::CanConnect(const Matrix3D& transform, const ConnectionField& other,
                                         const Matrix3D& otherTransform, const Vector3& origin,
                                         const Vector3& direction, Real32 maxDistance,
                                         Real32& distance, Matrix3D& snapped) const
{
    const ConnectionFieldSlider& slider = static_cast<const ConnectionFieldSlider&>(other);
    Vector3 axis;
    Vector3 otherAxis;
    Vector3 up(0, 1, 0);
    Real32 dot;
    bool same;
    Real32 offset;
    // 0.159992 = 0.39999^2 (Mac 0x3e23d4f1)
    if (!FindClosestLinesRay(*this, transform, slider, otherTransform, origin, direction, up,
                             0.159992f, maxDistance, distance, axis, otherAxis, dot, same, offset))
        return 0;
    return Snap(axis, otherAxis, offset, dot, same, transform, slider, otherTransform, snapped);
}

} // namespace LEGO
