// LEGO Digital Designer — restored source
// TU "ConnectionField.cpp"  Mac TU 384, 0x372340-0x372e1f (8 functions; <iostream> initialiser
// 0x372de0). Win: 0x46ddd0 (Attributes ctor), 0x4a1b60 (Attributes::TransformFrames),
// 0x4a2010-0x4a23c0 (accumulator, the two Pick helpers), 0x4aed50-0x4aef30 / 0x4af2f0-0x4af370
// (ConnectionField / LineHelper constructors and destructors), 0x4dbb30, 0x4dca30-0x4dcb90,
// 0x4dcda0 (PointHelper).
// Also defined here: base / helper members the Mac build emitted in TU 327 ConnectionFieldAxle.cpp
// (0x2dd4e0-0x2dd5f0) and TU 328 ConnectionFieldBall.cpp (0x2de050-0x2de1a0).
#include "ConnectionField.h"

#include <iostream>

namespace LEGO {

// =============================================================================================
// ConnectionField
// =============================================================================================

// Inlined in every Create function (Mac e.g. 0x2db0a0); Win 0x4aee00 / 0x4dca70 (helpers).
// NOTE: m_reserved is left uninitialised, as in both builds; fixed: 0 (low).
ConnectionField::ConnectionField(const Matrix3D& transform, UInt32 type, UInt32 kind)
    : m_transform(transform), m_context(0), m_observer(0), m_type(type), m_kind(kind), m_flags(0),
      m_reserved(0)
{
}

// Inlined in the Clone functions (Mac e.g. 0x2dd6c0); Win 0x4af370 (EDI = source).
ConnectionField::ConnectionField(const ConnectionField& o)
    : Object(), m_transform(o.m_transform), m_context(o.m_context), m_observer(o.m_observer),
      m_type(o.m_type), m_kind(o.m_kind), m_flags(o.m_flags), m_reserved(o.m_reserved)
{
}

// Mac 0x2dd570 (D1), 0x2dd590 (D0), 0x2dd5d0 / 0x2dd5f0 (Object thunks); Win 0x4aed60 / 0x4aed80
ConnectionField::~ConnectionField()
{
}

// Mac 0x2dd530, Win 0x4aed50
UInt32 ConnectionField::GetConnectionAngle(const Matrix3D&, const ConnectionField&, const Matrix3D&,
                                           const Vector3&, const Vector3&, const Vector3&,
                                           Real32&) const
{
    return 0;
}

// Mac 0x2dd550, Win 0x40be80
void ConnectionField::ApplyPointGroups(const BasicArray<PointGroup>&)
{
}

// Mac 0x2dd560, Win 0x40be80
void ConnectionField::SetPointsBlocked(bool)
{
}

// Mac 0x372820 (Win: inlined into every Connect)
void ConnectionField::NotifyConnect(ConnectionField& field, ConnectionField& other,
                                    const ConnectionIndices& indices, Attributes* attributes)
{
    if (field.m_observer)
        field.m_observer->OnConnect(true, field, other, indices, attributes);
    if (other.m_observer)
        other.m_observer->OnConnect(false, field, other, indices, attributes);
}

// Mac 0x372880 (Win: inlined into every Disconnect)
void ConnectionField::NotifyDisconnect(ConnectionField& field, ConnectionField& other,
                                       const ConnectionIndices& indices)
{
    if (field.m_observer)
        field.m_observer->OnDisconnect(true, field, other, indices);
    if (other.m_observer)
        other.m_observer->OnDisconnect(false, field, other, indices);
}

// =============================================================================================
// ConnectionField::Attributes
// =============================================================================================

// Win 0x46ddd0 (EAX = this); Mac: inlined at the users.
// NOTE: m_isFlexible is not initialised (both builds): copying an Attributes reads it (UBSan:
// a bool of value 144); fixed: false (low).
ConnectionField::Attributes::Attributes()
    : m_axis(0, 0, 0), m_normal(0, 0, 0), m_position(0, 0, 0),
      m_otherAxis(0, 0, 0), m_otherNormal(0, 0, 0), m_otherPosition(0, 0, 0),
      m_linear(0, 0, 0), m_linearLimits(0, 0, 0),
      m_lowerLimit(-PI, -PI, -PI), m_upperLimit(PI, PI, PI), m_twist(PI),
      m_spring(0, 0, 0), m_damping(0, 0, 0),
      m_lowerAngle(-10000.0f, -10000.0f, -10000.0f), m_upperAngle(10000.0f, 10000.0f, 10000.0f),
      m_hasLinear(false), m_hasLimits(false), m_hasSpring(false), m_hasAngles(false),
      m_isFlexible(false)
{
}

// Mac 0x372380, Win 0x4a1b60 (ESI = out, EDI = this)
void ConnectionField::Attributes::TransformFrames(bool swap, const Matrix3D& transform,
                                                  const Matrix3D& otherTransform,
                                                  Attributes& out) const
{
    if (!swap) {
        out.m_axis = Rotate(m_axis, transform);
        out.m_normal = Rotate(m_normal, transform);
        out.m_position = m_position * transform;
        out.m_otherAxis = Rotate(m_otherAxis, otherTransform);
        out.m_otherNormal = Rotate(m_otherNormal, otherTransform);
        out.m_otherPosition = m_otherPosition * otherTransform;
    } else {
        out.m_axis = Rotate(m_otherAxis, otherTransform);
        out.m_normal = Rotate(m_otherNormal, otherTransform);
        out.m_position = m_otherPosition * otherTransform;
        out.m_otherAxis = Rotate(m_axis, transform);
        out.m_otherNormal = Rotate(m_normal, transform);
        out.m_otherPosition = m_position * transform;
    }
    if (m_hasLinear) {
        out.m_linear = m_linear;
        out.m_linearLimits = m_linearLimits;
        out.m_hasLinear = true;
    }
    if (m_hasLimits) {
        if (!swap) {
            out.m_lowerLimit = m_lowerLimit;
            out.m_upperLimit = m_upperLimit;
        } else {
            out.m_lowerLimit = -m_upperLimit;
            out.m_upperLimit = -m_lowerLimit;
        }
        out.m_twist = m_twist;
        out.m_hasLimits = true;
    }
    if (m_hasSpring) {
        out.m_spring = m_spring;
        out.m_damping = m_damping;
        out.m_hasSpring = true;
    }
    if (m_hasAngles) {
        if (!swap) {
            out.m_lowerAngle = m_lowerAngle;
            out.m_upperAngle = m_upperAngle;
        } else {
            out.m_lowerAngle = -m_upperAngle;
            out.m_upperAngle = -m_lowerAngle;
        }
        out.m_hasAngles = true;
    }
}

// Mac 0x372340 (Win: inlined)
void ConnectionField::Attributes::Transform(bool swap, const Matrix3D& transform,
                                            const Matrix3D& otherTransform, Attributes& out) const
{
    TransformFrames(swap, transform, otherTransform, out);
    out.m_isFlexible = m_isFlexible;
}

// =============================================================================================
// ConnectionField::PickerAccumulator
// =============================================================================================

// Mac 0x3726b0, Win 0x4a2010 (ECX = this)
const ConnectionField::PickerAccumulator::PickedFeature*
ConnectionField::PickerAccumulator::GetBest() const
{
    const PickedFeature* best = 0;
    Real32 bestDistance = 3.4028235e+38f;                   // FLT_MAX
    for (UInt32 type = 0; type < TYPE_COUNT; ++type) {
        for (const PickedFeature* f = m_features[type].Begin(); f != m_features[type].End(); ++f) {
            if (f->m_distance < bestDistance) {
                best = f;
                bestDistance = f->m_distance;
            }
        }
    }
    return best;
}

// Inlined in both Pick helpers (Mac 0x3729f0.., Win 0x4a20d0..; PushBack: Mac
// ConnectionFieldPlanar.cpp 0x2e5190, Win 0x4a22c0).
// A feature replaces the entry of the same kind only when it is closer.
void ConnectionField::PickerAccumulator::Add(UInt32 type, const PickedFeature& feature)
{
    BasicArray<PickedFeature>& features = m_features[type];
    PickedFeature* f = features.Begin();
    for (; f != features.End(); ++f) {
        if (f->m_kind == feature.m_kind)
            break;
    }
    if (f == features.End()) {
        features.PushBack(feature);
    } else if (feature.m_distance < f->m_distance) {
        // NOTE: the kind of the entry is not rewritten (it is the same).
        f->m_field = feature.m_field;
        f->m_index = feature.m_index;
        f->m_transform = feature.m_transform;
        f->m_distance = feature.m_distance;
    }
}

// =============================================================================================
// ConnectionFieldLineHelper
// =============================================================================================

// Win 0x4aee00 (EDX = transform); Mac inlined into the Create functions.
ConnectionFieldLineHelper::ConnectionFieldLineHelper(const Matrix3D& transform, UInt32 type,
                                                     UInt32 kind, Real32 length,
                                                     bool startCapped, bool endCapped)
    : ConnectionField(transform, type, kind), m_length(length), m_startCapped(startCapped),
      m_endCapped(endCapped)
{
}

// Win 0x4af2f0 (ECX = source)
ConnectionFieldLineHelper::ConnectionFieldLineHelper(const ConnectionFieldLineHelper& o)
    : Object(), ConnectionField(o), m_length(o.m_length), m_startCapped(o.m_startCapped),
      m_endCapped(o.m_endCapped)
{
}

// Mac 0x3728d0, Win 0x4a2060
// The picked transform is the field transform moved to the point of the segment
// (origin .. (0, length, 0)) closest to `point`.
void ConnectionFieldLineHelper::Pick(const Matrix3D& transform, const Vector3& point,
                                     Real32 maxDistance, PickerAccumulator& accumulator) const
{
    const Vector3& start = transform.GetTranslation();
    Vector3 end = Vector3(0, m_length, 0) * transform;
    Vector3 closest = ClosestPointOnSegment(start, end, point);
    Real32 distance = point.GetDistanceSquared(closest);
    if (maxDistance < distance)
        return;

    PickerAccumulator::PickedFeature feature;
    feature.m_field = this;
    feature.m_index = 0;
    feature.m_transform = transform;
    feature.m_transform.SetTranslation(closest);
    feature.m_kind = m_kind;
    feature.m_distance = distance;
    accumulator.Add(m_type, feature);
}

// =============================================================================================
// ConnectionFieldPointHelper
// =============================================================================================

// Win 0x4dca70 (EDX = transform)
ConnectionFieldPointHelper::ConnectionFieldPointHelper(const Matrix3D& transform, UInt32 type,
                                                       UInt32 kind)
    : ConnectionField(transform, type, kind), m_occupied(false)
{
}

// Win 0x4dcda0 (ECX = source)
ConnectionFieldPointHelper::ConnectionFieldPointHelper(const ConnectionFieldPointHelper& o)
    : Object(), ConnectionField(o), m_occupied(o.m_occupied)
{
}

// Mac 0x372b90, Win 0x4a21c0
void ConnectionFieldPointHelper::Pick(const Matrix3D& transform, const Vector3& point,
                                      Real32 maxDistance, PickerAccumulator& accumulator) const
{
    if (m_occupied)
        return;
    Real32 distance = transform.GetTranslation().GetDistanceSquared(point);
    if (maxDistance < distance)
        return;

    PickerAccumulator::PickedFeature feature;
    feature.m_field = this;
    feature.m_index = 0;
    feature.m_transform = transform;
    feature.m_kind = m_kind;
    feature.m_distance = distance;
    accumulator.Add(m_type, feature);
}

} // namespace LEGO
