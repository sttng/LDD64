// LEGO Digital Designer — restored source
// LEGO::AxleAligner (and the Aligner destructors). See AxleAligner.h / Aligner.h.
//
// Original object file: AxleAligner.cpp (Mac TU 347 0x32c250-0x32dc4f; Win 0x4d52a0-0x4d7b5f).
#include "AxleAligner.h"
#include "BrickPhysics.h"
#include "ConnectionFieldAxle.h"
#include "ConnectionFieldLineFunctions.h"
#include "PrimitiveElement.h"
#include "PrimitiveRigidElement.h"

namespace LEGO {

namespace {

// SSE minss a, b: a when a < b, else b (also for NaN). (helper)
inline Real32 MinSS(Real32 a, Real32 b)
{
    return a < b ? a : b;
}

// RoundToStep, skipped for a zero step (axle hole steps; Mac 0x32c87a / 0x32ca69 / 0x32cd06 /
// 0x32cda0, Win 0x4aecb0). (name inferred)
inline Real32 RoundToFieldStep(Real32 value, Real32 step)
{
    if (step == 0.0f)
        return value;
    return RoundToStep(value, step);
}

// Keeps `*offset` (the position of field b along field a) within the capped ends of both
// fields: a value at most 0.4 outside a cap is moved onto it, a value further out leaves the
// rest unchecked (false). `sameDirection`: the fields point the same way. Win 0x4d62e0 (Mac
// inlined twice into 0x32c340). (name inferred)
bool SnapToCaps(Real32* offset, const ConnectionFieldAxle* a, const ConnectionFieldAxle* b,
                bool sameDirection)
{
    Real32 v = *offset;
    if (a->IsStartCapped()) {
        if (sameDirection) {
            if (0.0f > v) {
                if (-0.4f > v)
                    return false;
                *offset = v = 0.0f;
            }
        } else {
            Real32 length = b->GetLength();
            if (length > v) {
                if (-0.4f + length > v)
                    return false;
                *offset = v = length;
            }
        }
    }
    if (a->IsEndCapped()) {
        if (sameDirection) {
            Real32 end = v + b->GetLength();
            if (end > a->GetLength()) {
                if (end > 0.4f + a->GetLength())
                    return false;
                *offset = v = a->GetLength() - b->GetLength();
            }
        } else {
            if (v > a->GetLength()) {
                if (v > 0.4f + a->GetLength())
                    return false;
                *offset = v = a->GetLength();
            }
        }
    }
    if (b->IsStartCapped()) {
        if (sameDirection) {
            if (v > 0.0f) {
                if (v > 0.4f)
                    return false;
                *offset = v = 0.0f;
            }
        } else {
            Real32 length = a->GetLength();
            if (length > v) {
                if (-0.4f + length > v)
                    return false;
                *offset = v = length;
            }
        }
    }
    if (b->IsEndCapped()) {
        Real32 length = b->GetLength();
        if (sameDirection) {
            Real32 end = v + length;
            if (a->GetLength() > end) {
                if (-0.4f + a->GetLength() > end)
                    return false;
                *offset = a->GetLength() - length;
            }
        } else {
            if (v > length) {
                if (v > 0.4f + length)
                    return false;
                *offset = length;
            }
        }
    }
    return true;
}

// Axle / hole (kinds 3, 19 with 2) or end stop / axle (0x11 with 2) pair: the two points are
// also tied by a ball-and-socket. Win 0x4d7ab0 (Mac inlined in 0x32d7c0). (name inferred)
inline bool IsSnappingPair(UInt32 kind, UInt32 targetKind)
{
    return ((targetKind | 0x10) == 0x13 && kind == 2) || (targetKind == 2 && (kind | 0x10) == 0x13) ||
           (kind == 0x11 && targetKind == 2) || (targetKind == 0x11 && kind == 2);
}

} // namespace

// Mac 0x32c250 (C1 0x32c330)
AxleAligner::AxleAligner(AnchorPhysicsSolverAnchor* anchor, const Field& field, const Field& target)
    : Aligner(anchor, field, target), m_state(STATE_NEW)
{
}

// Mac 0x32c340, Win 0x4d6560
void AxleAligner::Compute(Vector3& point, Vector3& approach, Vector3& position, Vector3& axis,
                          Vector3& targetPosition, Vector3& targetAxis, Vector3& reference,
                          Vector3& targetPoint)
{
    const ConnectionFieldAxle* own = static_cast<const ConnectionFieldAxle*>(m_field.m_field);
    const ConnectionFieldAxle* other = static_cast<const ConnectionFieldAxle*>(m_target.m_field);

    // the target axle in the world, the own axle in its body
    Matrix3D targetElement;
    m_target.m_element->GetTransform(targetElement);
    Matrix3D targetFrame(targetElement, other->GetTransform());
    Matrix3D ownElement;
    m_field.m_element->GetTransform(ownElement);
    Matrix3D ownFrame(ownElement, own->GetTransform());
    Matrix3D ownInBody(m_field.m_element->GetRigidElement()->GetLocalTransform(),
                       m_field.m_element->GetLocalTransform());
    Matrix3D ownBodyFrame(ownInBody, own->GetTransform());

    Vector3 up(0.0f, 1.0f, 0.0f);
    targetAxis = Rotate(up, targetFrame);
    targetPosition = targetFrame.GetTranslation();
    axis = Rotate(up, ownBodyFrame);
    position = ownBodyFrame.GetTranslation();

    // the grabbing male field gives the reference direction
    if ((own->GetKind() & 1) == 0) {
        if (own->IsGrabbing()) {
            m_mode = MODE_OWN_GRABS;
            reference = Rotate(Vector3(-1.0f, 0.0f, 0.0f), ownBodyFrame);
        } else {
            m_mode = MODE_FREE;
        }
    } else {
        if (other->IsGrabbing()) {
            m_mode = MODE_TARGET_GRABS;
            reference = Rotate(Vector3(-1.0f, 0.0f, 0.0f), targetFrame);
        } else {
            m_mode = MODE_FREE;
        }
    }

    Vector3 targetStart(targetFrame.GetTranslation());
    Vector3 targetEnd = Vector3(0.0f, other->GetLength(), 0.0f) * targetFrame;
    Vector3 ownStart(ownBodyFrame.GetTranslation());
    Vector3 ownEnd = Vector3(0.0f, own->GetLength(), 0.0f) * ownBodyFrame;
    Real32 startDistance = Distance(targetStart, m_anchor->GetTargetTransform().GetTranslation());
    Real32 endDistance = Distance(targetEnd, m_anchor->GetTargetTransform().GetTranslation());
    Real32 dot = Rotate(Vector3(0.0f, 1.0f, 0.0f), ownFrame) * targetAxis;
    // the end of the target axle the own one enters from
    bool fromEnd;
    if (other->IsStartCapped())
        fromEnd = false;
    else if (other->IsEndCapped())
        fromEnd = true;
    else
        fromEnd = endDistance > startDistance;

    if (m_mode == MODE_FREE) {
        if (fromEnd)
            approach = targetStart + Rotate(Vector3(0.0f, -1.0f, 0.0f), targetFrame);
        else
            approach = targetEnd + Rotate(Vector3(0.0f, 1.0f, 0.0f), targetFrame);
    } else if (m_mode == MODE_OWN_GRABS) {
        approach = ClosestPointOnSegment(targetStart, targetEnd,
                                         m_anchor->GetTargetTransform().GetTranslation());
    } else if (m_mode == MODE_TARGET_GRABS) {
        approach = targetStart + Rotate(Vector3(-0.5f, 0.0f, 0.0f), targetFrame);
    }

    // the own field's offset along the target axle, snapped
    targetPoint = ClosestPointOnSegment(targetStart, targetEnd,
                                        m_anchor->GetTargetTransform().GetTranslation());
    Real32 targetOffset = (targetPoint - targetStart) * targetAxis;
    UInt32 kind = own->GetKind();
    UInt32 otherKind = other->GetKind();
    if ((kind | 0x10) == 0x13 && otherKind == 2) {
        targetOffset = RoundToFieldStep(targetOffset, other->GetStep());
    } else if (kind == 2 && (otherKind | 0x10) == 0x13) {
        if ((otherKind & 1) == 0)   // NOTE: never (kinds 3 / 0x13 are odd)
            targetOffset = RoundToFieldStep(targetOffset, other->GetStep());
        else
            targetOffset = RoundToFieldStep(targetOffset, own->GetStep());
    } else if (kind == 2 && otherKind == 0x11) {
        ConnectionFieldAxle::SnapToEnd(&targetOffset, own->GetLength(), dot > 0.0f);
    } else if (kind == 0x11 && otherKind == 2) {
        ConnectionFieldAxle::SnapToEnd(&targetOffset, other->GetLength(), dot > 0.0f);
    } else {
        targetOffset = RoundToStep(targetOffset, 0.08f);
        SnapToCaps(&targetOffset, other, own, dot > 0.0f);
    }

    // the pulled point on the own axle, snapped
    Vector3 anchorPoint(m_anchor->GetLocalTransform().GetTranslation());
    point = ClosestPointOnSegment(ownStart, ownEnd, anchorPoint);
    Real32 offset = (point - ownStart) * axis;
    if ((otherKind | 0x10) == 0x13 && kind == 2) {
        offset = RoundToFieldStep(offset, own->GetStep());
    } else if (otherKind == 2 && (kind | 0x10) == 0x13) {
        if ((kind & 1) == 0)        // NOTE: never (kinds 3 / 0x13 are odd)
            offset = RoundToFieldStep(offset, own->GetStep());
        else
            offset = RoundToFieldStep(offset, other->GetStep());
    } else if ((kind == 0x11 && otherKind == 2) || (otherKind == 0x11 && kind == 2)) {
        ConnectionFieldAxle::SnapToEnd(&offset, other->GetLength(), dot > 0.0f);
    } else {
        offset = RoundToStep(offset, 0.08f);
        // NOTE: original quirk: both fields are the target axle here (Mac 0x32d1f4, Win 0x4d6eef)
        SnapToCaps(&offset, other, other, dot > 0.0f);
    }

    // the two offsets limit each other at the capped ends
    if (dot > 0.0f) {
        if (other->IsStartCapped())
            offset = MinSS(targetOffset, offset);
        if (other->IsEndCapped())
            offset = own->GetLength() - MinSS(own->GetLength() - offset, other->GetLength() - targetOffset);
        if (own->IsStartCapped())
            targetOffset = MinSS(targetOffset, offset);
        if (own->IsEndCapped())
            targetOffset = other->GetLength() - MinSS(own->GetLength() - offset, other->GetLength() - targetOffset);
    } else {
        if (other->IsStartCapped())
            offset = own->GetLength() - MinSS(targetOffset, own->GetLength() - offset);
        if (other->IsEndCapped())
            offset = MinSS(offset, other->GetLength() - targetOffset);
        if (own->IsStartCapped())
            targetOffset = other->GetLength() - MinSS(other->GetLength() - targetOffset, offset);
        if (own->IsEndCapped())
            targetOffset = MinSS(own->GetLength() - offset, targetOffset);
    }
    targetPoint = targetStart + targetAxis * targetOffset;
    point = ownStart + axis * offset;

    // end stops (ball fields of kind >= 499500) of the own element at the ends of the own axle
    bool stopAtStart = false;
    bool stopAtEnd = false;
    for (UInt32 i = 0; i < m_field.m_element->GetFieldCount(); ++i) {
        const ConnectionField* field = m_field.m_element->GetField(i);
        if (field->GetType() != ConnectionField::TYPE_BALL || field->GetKind() < 0x79f2c)
            continue;
        if (field->GetTransform().GetTranslation() == own->GetTransform().GetTranslation())
            stopAtStart = true;
        else if (field->GetTransform().GetTranslation() ==
                 Vector3(0.0f, own->GetLength(), 0.0f) * own->GetTransform())
            stopAtEnd = true;
    }

    // the direction the own axle has to enter
    if (m_mode < MODE_FREE) {
        if (dot > 0.0f)
            return;
        axis = -axis;
        targetPosition = targetEnd;
        return;
    }
    if (stopAtEnd && stopAtStart)
        return;
    if (stopAtStart) {
        if (fromEnd)
            return;
    } else if (stopAtEnd) {
        if (!fromEnd)
            return;
    } else if (!(!fromEnd && own->IsStartCapped()) && !(fromEnd && own->IsEndCapped())) {
        if (dot > 0.0f)
            return;
    }
    axis = -axis;
}

// Mac 0x32d3d0, Win 0x4d73f0
void AxleAligner::Start()
{
    if (m_state - 1 < 3) {
        Vector3 point, approach, position, axis, targetPosition, targetAxis, reference, targetPoint;
        UInt32 mode = m_mode;
        Compute(point, approach, position, axis, targetPosition, targetAxis, reference, targetPoint);
        if (point != m_point || approach != m_approach || position != m_position || axis != m_axis ||
            targetPosition != m_targetPosition || targetAxis != m_targetAxis ||
            reference != m_reference || targetPoint != m_targetPoint || m_mode != mode) {
            m_point = point;
            m_approach = approach;
            m_position = position;
            m_axis = axis;
            m_targetPosition = targetPosition;
            m_targetAxis = targetAxis;
            m_reference = reference;
            m_targetPoint = targetPoint;
            if (m_state != STATE_LOCKED) {
                Approach();
                Step(0.0f);
                return;
            }
            Lock();
        }
        if (m_state != STATE_APPROACH) {
            UInt32 state = m_anchor->GetState();
            if (state == 1)
                Stop();
            else if (state == 2)
                Lock();
        }
    } else if (m_state == STATE_DONE || m_state == STATE_NEW) {
        Compute(m_point, m_approach, m_position, m_axis, m_targetPosition, m_targetAxis, m_reference,
                m_targetPoint);
        Matrix3D body;
        m_anchor->GetElement()->GetTransform(body);
        Vector3 position = m_position * body;
        Vector3 axis = Rotate(m_axis, body);
        if (m_targetPosition.IsSameLine(m_targetAxis, position, axis, 0.1f))
            Lock();
        else
            Approach();
    }
    Step(0.0f);
}

// (Mac inlined in 0x32d3d0, Win 0x4d78a0)
void AxleAligner::Approach()
{
    m_anchor->SetPointAttraction(m_point, m_approach);
    m_anchor->ClearBallAndSocket();
    m_anchor->ClearAngular();
    m_anchor->ClearAngularReversed();
    m_anchor->ClearOrientation();
    m_anchor->ClearCylindrical();
    m_state = STATE_APPROACH;
}

// Mac 0x32d7c0, Win 0x4d79c0
void AxleAligner::Lock()
{
    if (m_state != STATE_LOCKED) {
        if (m_mode == MODE_FREE) {
            m_anchor->SetCylindrical(m_position, m_axis, m_targetPosition, m_targetAxis);
        } else if (m_mode == MODE_OWN_GRABS) {
            Real32 offset = 0.0f;
            m_anchor->SetAngularReversed(m_position, m_axis, m_targetPosition, m_targetAxis, m_reference, offset);
            Real32 tolerance = 0.001f;
            m_anchor->SetOrientation(tolerance);
        } else if (m_mode == MODE_TARGET_GRABS) {
            Real32 offset = 0.0f;
            m_anchor->SetAngular(m_position, m_axis, m_targetPosition, m_targetAxis, m_reference, offset);
        }
    }
    m_anchor->SetPointAttraction(m_point, m_targetPoint);
    if (IsSnappingPair(m_field.m_field->GetKind(), m_target.m_field->GetKind()))
        m_anchor->SetBallAndSocket(m_point, m_targetPoint);
    m_state = STATE_LOCKED;
}

// Mac 0x32d930, Win 0x4d7770
void AxleAligner::Stop()
{
    m_anchor->ClearPointAttraction();
    m_anchor->ClearBallAndSocket();
    m_anchor->ClearAngular();
    m_anchor->ClearAngularReversed();
    m_anchor->ClearOrientation();
    m_anchor->ClearCylindrical();
    m_state = STATE_DONE;
}

// Mac 0x32d990, Win 0x4d77c0
bool AxleAligner::IsDone() const
{
    return m_state == STATE_DONE;
}

// Mac 0x32d9b0, Win 0x4d77d0 (the guide part Win 0x4d78f0)
void AxleAligner::Step(Real32 /*time*/)
{
    Matrix3D body;
    m_anchor->GetElement()->GetTransform(body);
    if (m_state == STATE_GUIDED) {
        UInt32 state = m_anchor->GetState();
        if (state == 1)
            Stop();
        else if (state == 2)
            Lock();
    } else if (m_state == STATE_APPROACH) {
        Vector3 point = m_point * body;
        if (Distance(m_approach, point) < 2.0f) {
            if (m_mode == MODE_FREE) {
                m_anchor->SetCylindrical(m_position, m_axis, m_targetPosition, m_targetAxis);
            } else if (m_mode == MODE_OWN_GRABS) {
                Real32 offset = -0.5f;
                m_anchor->SetAngularReversed(m_position, m_axis, m_targetPosition, m_targetAxis, m_reference, offset);
                Real32 tolerance = 0.001f;
                m_anchor->SetOrientation(tolerance);
            } else if (m_mode == MODE_TARGET_GRABS) {
                static const Real32 s_offset = 0.5f;   // Mac 0x63d580, Win 0xa1ecf0
                m_anchor->SetAngular(m_position, m_axis, m_targetPosition, m_targetAxis, m_reference, s_offset);
            }
            m_state = STATE_GUIDED;
        }
    }
}

} // namespace LEGO
