// LEGO Digital Designer — restored source
// LEGO::PlanarAligner. See PlanarAligner.h.
//
// Original object file: PlanarAligner.cpp (Mac TU 352 0x333560-0x33463f; Win 0x4d5345-0x4d61ff).
#include "PlanarAligner.h"
#include "BrickPhysics.h"
#include "ConnectionFieldPlanar.h"
#include "PrimitiveElement.h"
#include "PrimitiveRigidElement.h"

namespace LEGO {

namespace {

// Abs as the SSE andps mask. (helper)
inline Real32 AbsF(Real32 v)
{
    return v < 0.0f ? -v : v;
}

// Point `index` of a planar field's grid (row-major, width + 1 points per row; 0.4 apart) in the
// field frame `frame`. (inlined twice in Mac 0x333900)
Vector3 GridPoint(const ConnectionField* field, UInt32 index, const Matrix3D& frame)
{
    UInt32 columns = static_cast<const ConnectionFieldPlanar*>(field)->GetWidth() + 1;
    return Vector3((Real32)(index % columns) * 0.4f, 0.0f, (Real32)(index / columns) * 0.4f) * frame;
}

} // namespace

// Mac 0x333560 (C1 0x333680)
PlanarAligner::PlanarAligner(AnchorPhysicsSolverAnchor* anchor, const Field& field, const Field& target)
    : Aligner(anchor, field, target), m_state(STATE_NEW), m_stepCount(0)
{
}

// Mac 0x3344f0 (D1 0x3345b0, D0 0x3345c0), Win 0x4d5af0 / sdd 0x4d53f0
PlanarAligner::~PlanarAligner()
{
    // NOTE: only in state 5; a destroyed aligner in state 4 leaves the features switched off
    if (m_state == STATE_JOINED && !m_features.IsEmpty())
        m_anchor->EnableCollisions(&m_features[0], m_features.GetSize());
}

// Mac 0x333900, Win 0x4d5ba0
void PlanarAligner::Compute()
{
    const ConnectionField* own = m_field.m_field;
    const ConnectionField* other = m_target.m_field;

    Matrix3D targetElement;
    m_target.m_element->GetTransform(targetElement);
    Matrix3D targetFrame(targetElement, other->GetTransform());
    Matrix3D ownInBody(m_field.m_element->GetRigidElement()->GetLocalTransform(),
                       m_field.m_element->GetLocalTransform());
    Matrix3D ownBodyFrame(ownInBody, own->GetTransform());

    m_targetNormal = Rotate(Vector3(0.0f, 1.0f, 0.0f), targetFrame);
    m_targetPosition = GridPoint(other, m_target.m_index, targetFrame);
    m_normal = Rotate(Vector3(0.0f, 1.0f, 0.0f), ownBodyFrame);
    m_position = GridPoint(own, m_field.m_index, ownBodyFrame);
    m_point = m_position;
    Plane plane;
    if ((other->GetKind() & 1) == 0) {
        // studs: approach from below the target grid
        m_approach = m_targetPosition + Rotate(Vector3(0.0f, -1.0f, 0.0f), targetFrame);
        m_hold = m_targetPosition + Rotate(Vector3(0.0f, -10.0f, 0.0f), targetFrame);
        plane = Plane(-m_targetNormal, m_targetPosition);
    } else {
        m_approach = m_targetPosition + Rotate(Vector3(0.0f, 1.0f, 0.0f), targetFrame);
        m_hold = m_targetPosition + Rotate(Vector3(0.0f, 10.0f, 0.0f), targetFrame);
        plane = Plane(m_targetNormal, m_targetPosition);
    }
    m_plane = plane;
}

// (inlined in Mac 0x333690 / 0x333f70 three times)
void PlanarAligner::PullAround(const Matrix3D& body)
{
    Vector3 projected = ProjectOntoPlane(m_plane, m_position * body);
    Vector3 direction = projected - m_targetPosition;
    direction.SetLength(1.0f);
    m_anchor->SetPointAttraction(m_position, (m_targetPosition + direction) + m_plane.m_normal * 2.0f);
}

// Mac 0x333690, Win 0x4d5410
void PlanarAligner::Start()
{
    if (m_state == STATE_DONE || m_state == STATE_NEW) {
        Compute();
        Matrix3D body;
        m_anchor->GetElement()->GetTransform(body);
        Real32 distance = m_plane.GetDistance(m_position * body);
        if (0.0f > distance && AbsF(distance) > 0.001f) {
            m_state = STATE_AROUND;
            PullAround(body);
        } else {
            if (Distance(m_targetPosition, m_position * body) <= 1.0f) {
                Vector3 position = m_position * body;
                Vector3 normal = Rotate(m_normal, body);
                if (m_targetPosition.IsSameLine(m_targetNormal, position, normal, 0.001f)) {
                    m_anchor->SetCylindrical(m_position, m_normal, m_targetPosition, m_targetNormal);
                    Join();
                    Step(0.0f);
                    return;
                }
            }
            m_state = STATE_APPROACH;
            m_anchor->SetPointAttraction(m_point, m_approach);
        }
    } else if (m_state == STATE_HELD) {
        Join();
    }
    Step(0.0f);
}

// Mac 0x333cc0, Win 0x4d6170
void PlanarAligner::Join()
{
    m_state = STATE_JOINING;
    m_stepCount = 0;
    if ((m_target.m_field->GetKind() & 1) == 0)
        m_field.m_element->GetFeature(*m_field.m_field, m_field.m_index, m_features);
    else
        m_target.m_element->GetFeature(*m_target.m_field, m_target.m_index, m_features);
    if (!m_features.IsEmpty())
        m_anchor->DisableCollisions(&m_features[0], m_features.GetSize());
    m_anchor->SetPointAttraction(m_position, m_targetPosition);
    m_anchor->SetBallAndSocket(m_position, m_targetPosition);
}

// Mac 0x333d70, Win 0x4d55d0
void PlanarAligner::Stop()
{
    switch (m_state) {
    case STATE_NEW: {
        Compute();
        Matrix3D body;
        m_anchor->GetElement()->GetTransform(body);
        if (!m_targetPosition.IsEqual(m_position * body, 0.001f)) {
            m_state = STATE_DONE;
        } else {
            Vector3 position = m_position * body;
            Vector3 normal = Rotate(m_normal, body);
            UInt32 state = STATE_DONE;
            if (m_targetPosition.IsSameLine(m_targetNormal, position, normal, 0.001f)) {
                m_anchor->SetCylindrical(m_position, m_normal, m_targetPosition, m_targetNormal);
                m_anchor->SetPointAttraction(m_point, m_hold);
                state = STATE_HELD;
            }
            m_state = state;
        }
        break;
    }
    case STATE_GUIDED:
        m_anchor->ClearCylindrical();
        // fall through
    case STATE_AROUND:
    case STATE_APPROACH:
        m_anchor->SetPointAttraction(m_anchor->GetLocalTransform().GetTranslation(),
                                     m_anchor->GetTargetTransform().GetTranslation());
        m_state = STATE_DONE;
        break;
    case STATE_JOINING:
    case STATE_JOINED:
        m_anchor->ClearBallAndSocket();
        m_anchor->SetPointAttraction(m_point, m_hold);
        m_state = STATE_HELD;
        break;
    default:
        break;
    }
}

// Mac 0x333f50, Win 0x4d5770
bool PlanarAligner::IsDone() const
{
    return m_state == STATE_DONE;
}

// Mac 0x333f70, Win 0x4d5780
void PlanarAligner::Step(Real32 /*time*/)
{
    ++m_stepCount;
    Matrix3D body;
    m_anchor->GetElement()->GetTransform(body);
    switch (m_state) {
    case STATE_AROUND: {
        Real32 distance = m_plane.GetDistance(m_position * body) + -0.1f;
        if (distance < 0.0f && 1e-05f < AbsF(distance)) {
            PullAround(body);
            return;
        }
        m_state = STATE_APPROACH;
        m_anchor->SetPointAttraction(m_point, m_approach);
    }
        // fall through
    case STATE_APPROACH: {
        Real32 distance = m_plane.GetDistance(m_position * body);
        if (distance < 0.0f && 1e-05f < AbsF(distance)) {
            m_state = STATE_AROUND;
            PullAround(body);
            return;
        }
        if (1.0f <= Distance(m_approach, m_point * body))
            return;
        m_state = STATE_GUIDED;
        m_stepCount = 0;
        m_anchor->SetCylindrical(m_position, m_normal, m_targetPosition, m_targetNormal);
        return;
    }
    case STATE_GUIDED: {
        Real32 distance = m_plane.GetDistance(m_position * body);
        if (distance < 0.0f && 1e-05f < AbsF(distance)) {
            m_anchor->ClearCylindrical();
            PullAround(body);
            m_state = STATE_AROUND;
            return;
        }
        UInt32 state = m_anchor->GetState();
        if (state == 1)
            Stop();
        else if (state == 2)
            Join();
        if (m_stepCount < 501)
            return;
        break;
    }
    case STATE_JOINING: {
        UInt32 state = m_anchor->GetState();
        if (state == 1) {
            Stop();
        } else if (state == 2) {
            m_state = STATE_JOINED;
            m_anchor->ClearPointAttraction();
        }
        if (m_stepCount < 251)
            return;
        break;
    }
    case STATE_JOINED:
        if (m_anchor->GetState() > 1)
            return;
        break;
    case STATE_HELD:
        if (Distance(m_targetPosition, m_position * body) > 0.25f) {
            m_state = STATE_DONE;
            if (!m_features.IsEmpty()) {
                m_anchor->EnableCollisions(&m_features[0], m_features.GetSize());
                m_features.Clear();
            }
            m_anchor->ClearCylindrical();
        }
        return;
    default:
        return;
    }
    Stop();
}

} // namespace LEGO
