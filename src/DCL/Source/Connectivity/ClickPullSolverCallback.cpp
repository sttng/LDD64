// LEGO Digital Designer — restored source
// LEGO::ClickPullSolverCallback and ClickPullSolverCallback::Anchor. See
// ClickPullSolverCallback.h.
//
// Original object file: ClickPullSolverCallback.cpp (Mac TU 348 0x32dc50-0x32f46f; Win
// 0x4b18f0-0x4b292f; QueryInterface folded with StringPullSolverCallback's, Win 0x4d43a0).
#include "ClickPullSolverCallback.h"
#include "AxleAligner.h"
#include "PlanarAligner.h"
#include "PrimitiveElement.h"
#include "PrimitiveRigidElement.h"
#include "RigidElement.h"

#include <math.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <libkern/OSAtomic.h>
#endif

namespace LEGO {

// ===========================================================================================
// ClickPullSolverCallback::Anchor
// ===========================================================================================

// Mac 0x32eb50, Win 0x4b1d60
ClickPullSolverCallback::Anchor::Anchor(Scene* scene, AnchorPhysicsSolverAnchor* anchor, const Vector3& origin,
                                        const Vector3& direction, const Vector3& point)
    : m_anchor(anchor), m_scene(scene)
{
    m_anchor->SetUserData(this);
    RigidElement* body = m_anchor->GetElement()->GetRigidElement();
    const BasicArray<RefPointer<PrimitiveRigidElement> >& rigids = body->GetElements();
    for (UInt32 r = 0; r < rigids.GetSize(); ++r) {
        const BasicArray<RefPointer<PrimitiveElement> >& elements = rigids[r]->GetElements();
        for (UInt32 e = 0; e < elements.GetSize(); ++e) {
            PrimitiveElement* element = elements[e];
            Matrix3D transform;
            element->GetTransform(transform);
            for (UInt32 f = 0; f < element->GetFieldCount(); ++f) {
                const ConnectionField* field = element->GetField(f);
                Matrix3D frame(transform, field->GetTransform());
                field->Pick(frame, point, 9.0f, m_picked);
            }
        }
    }
    Update(origin, direction);
}

// Mac 0x32f100 (D1) / 0x32f180 (D0), Win 0x4b2800 / sdd 0x4b1f30
ClickPullSolverCallback::Anchor::~Anchor()
{
}

// Mac 0x32ee40, Win 0x4b2570
void ClickPullSolverCallback::Anchor::FindTarget(Scene::PickResult& elements, const Vector3& origin,
                                                 const Vector3& direction)
{
    m_target.m_element = 0;
    m_target.m_field = 0;
    Real32 best = 3.4028235e+38f;   // FLT_MAX
    for (UInt32 type = 0; type != ConnectionField::TYPE_COUNT; ++type) {
        const BasicArray<ConnectionField::PickerAccumulator::PickedFeature>& picked = m_picked.m_features[type];
        for (UInt32 p = 0; p < picked.GetSize(); ++p) {
            const ConnectionField::PickerAccumulator::PickedFeature& own = picked[p];
            for (Scene::PickResult::Iterator it = elements.Begin(); it != elements.End(); ++it) {
                PrimitiveElement* element = it->m_element;
                if (element->GetRigidElement()->GetRigidElement()->GetBody() == m_anchor->GetElement())
                    continue;
                Matrix3D transform;
                element->GetTransform(transform);
                for (UInt32 f = 0; f < element->GetFieldCount(); ++f) {
                    const ConnectionField* field = element->GetField(f);
                    if (!own.m_field->IsCompatible(*field))
                        continue;
                    Matrix3D frame(transform, field->GetTransform());
                    Matrix3D offset;
                    Real32 distance;
                    UInt32 index;
                    if (!field->Pick(frame, origin, direction, 1000000.0f, distance, index, offset))
                        continue;
                    Real32 gain = best - distance;
                    if (!(0.0f < gain) || !(0.0001f < fabsf(gain)))
                        continue;
                    if (!own.m_field->IsCompatible(own.m_index, *field, index))
                        continue;
                    best = distance;
                    m_offset = offset;
                    m_field.m_element = static_cast<PrimitiveElement*>(own.m_field->GetContext());
                    m_field.m_field = own.m_field;
                    m_field.m_index = own.m_index;
                    m_target.m_element = element;
                    m_target.m_field = field;
                    m_target.m_index = index;
                }
            }
        }
    }
}

// Mac 0x32dfe0, Win 0x4b2000
void ClickPullSolverCallback::Anchor::Update(const Vector3& origin, const Vector3& direction)
{
    RigidElement* body = m_anchor->GetElement()->GetRigidElement();
    Matrix3D bodyTransform;
    body->GetTransform(bodyTransform);
    // the cone around the ray that just covers the body's bounding sphere
    Sphere bounds(body->m_bounding);
    Vector3 center = bounds.m_center * bodyTransform;
    Real32 angle = atanf(bounds.m_radius / (center - origin).GetLength());
    Scene::PickResult elements;
    Scene::HideAwareConePicker picker(Cone(origin, direction, angle), &elements);
    m_scene->Pick(picker);
    FindTarget(elements, origin, direction);
    if (m_target.m_element != 0) {
        Matrix3D transform;
        m_target.m_element->GetTransform(transform);
        Matrix3D frame(transform, m_target.m_field->GetTransform());
        Matrix3D target(frame, m_offset);
        m_anchor->SetTargetTransform(target);
    } else {
        Vector3 point = m_anchor->GetLocalTransform().GetTranslation() * bodyTransform;
        Plane plane(direction, point);
        Vector3 hit;
        if (RayPlaneIntersect(origin, direction, plane, hit)) {
            Matrix3D target(m_anchor->GetTargetTransform());
            target.SetTranslation(hit);
            m_anchor->SetTargetTransform(target);
        }
    }
    if (m_aligner) {
        if (m_aligner->IsPair(m_field, m_target))
            m_aligner->Start();
        else
            m_aligner->Stop();
    }
    Step(0.0f);
}

// Mac 0x32e730, Win 0x4b2410
void ClickPullSolverCallback::Anchor::Step(Real32 time)
{
    if (m_aligner) {
        m_aligner->Step(time);
        if (m_aligner->IsDone())
            m_aligner = 0;
        if (m_aligner)
            return;
    }
    if (m_target.m_element == 0) {
        m_anchor->SetPointAttraction(m_anchor->GetLocalTransform().GetTranslation(),
                                     m_anchor->GetTargetTransform().GetTranslation());
        return;
    }
    UInt32 type = m_target.m_field->GetType();
    if (type == ConnectionField::TYPE_AXLE)
        m_aligner = new AxleAligner(m_anchor, m_field, m_target);
    else if (type == ConnectionField::TYPE_PLANAR)
        m_aligner = new PlanarAligner(m_anchor, m_field, m_target);
    if (m_aligner)
        m_aligner->Start();
}

// ===========================================================================================
// ClickPullSolverCallback
// ===========================================================================================

// Mac 0x32dcb0, Win 0x4b1910
ClickPullSolverCallback::ClickPullSolverCallback(Scene* scene)
    : m_adding(false), m_scene(scene), m_refCount(1), m_solver(0)
{
}

// Mac 0x32f210 (D1) / 0x32f280 (D0), Win sdd 0x4b18f0 (body 0x4b1890)
ClickPullSolverCallback::~ClickPullSolverCallback()
{
}

// Mac 0x32ea70, Win 0x4d43a0
Int32 ClickPullSolverCallback::QueryInterface(const InterfaceID& id, void** object)
{
    if (memcmp(&id, &AnchorPhysicsSolverCallback::GetIID(), sizeof id) != 0) {
        *object = 0;
#if defined(_WIN32)
        return (Int32)0x80004005;
#else
        return (Int32)0x80000008;
#endif
    }
    *object = this;
    AddRef();
    return 0;
}

// Mac 0x32eaf0, Win 0x4b1d10
UInt32 ClickPullSolverCallback::AddRef()
{
#if defined(_WIN32)
    return (UInt32)InterlockedIncrement((LONG*)&m_refCount);
#else
    return (UInt32)OSAtomicAdd32(1, (int32_t*)&m_refCount);
#endif
}

// Mac 0x32eb10, Win 0x4b1d30
UInt32 ClickPullSolverCallback::Release()
{
#if defined(_WIN32)
    Int32 count = InterlockedDecrement((LONG*)&m_refCount);
#else
    Int32 count = OSAtomicAdd32(-1, (int32_t*)&m_refCount);
#endif
    if (count == 0)
        delete this;
    return (UInt32)count;
}

// Mac 0x32dd00, Win 0x4b1980
AnchorPhysicsSolverAnchor* ClickPullSolverCallback::AddAnchor(PhysicsRigidElement* body, const Matrix3D& local,
                                                              const Matrix3D& target, UInt32 count,
                                                              PhysicsRigidElement* const* bodies,
                                                              const Vector3& origin, const Vector3& direction,
                                                              PrimitiveElement* /*element*/, const Vector3& point)
{
    if (m_solver == 0 || !m_anchors.IsEmpty())
        return 0;
    AnchorPhysicsSolver* solver = 0;
    if (m_solver->QueryInterface(AnchorPhysicsSolver::GetIID(), (void**)&solver) < 0)
        return 0;
    m_adding = true;
    AnchorPhysicsSolverAnchor* anchor = solver->CreateAnchor(body, local, target, count, bodies);
    RefPointer<Anchor> added = new Anchor(m_scene, anchor, origin, direction, point);
    m_anchors.PushBack(added);
    m_adding = false;
    solver->Release();
    return anchor;
}

// Mac 0x32df90, Win 0x4b1ac0
void ClickPullSolverCallback::UpdateAnchors(const Vector3& origin, const Vector3& direction)
{
    for (UInt32 i = 0; i < m_anchors.GetSize(); ++i)
        m_anchors[i]->Update(origin, direction);
}

// Mac 0x32e360, Win 0x4b1b00
void ClickPullSolverCallback::CanConnect(Scene::ConnectResult& result)
{
    if (m_anchors.IsEmpty())
        return;
    AnchorPhysicsSolverAnchor* anchor = m_anchors[0]->GetSolverAnchor();
    UInt32 count = anchor->GetElementCount();
    PhysicsRigidElement* const* bodies = anchor->GetElements();
    BasicArray<RigidElement*> rigids;
    for (UInt32 i = 0; i < count; ++i)
        rigids.PushBack(bodies[i]->GetRigidElement());
    m_scene->CanConnect(rigids, result);
}

// Mac 0x32e680, Win 0x4b1bf0
UInt32 ClickPullSolverCallback::GetState() const
{
    if (m_anchors.IsEmpty())
        return 1;
    return m_anchors[0]->GetSolverAnchor()->GetState();
}

// Mac 0x32e6b0, Win 0x4b1c10
void ClickPullSolverCallback::OnBegin(PhysicsSolver* solver)
{
    m_solver = solver;
}

// Mac 0x32e6c0, Win 0x40be80
void ClickPullSolverCallback::OnEnd(bool& /*result*/)
{
}

// Mac 0x32e6d0, Win 0x40be80
void ClickPullSolverCallback::OnResult(bool& /*success*/)
{
}

// Mac 0x32e6e0, Win 0x4b1c20
void ClickPullSolverCallback::OnStep(Real32 time)
{
    for (UInt32 i = 0; i < m_anchors.GetSize(); ++i)
        m_anchors[i]->Step(time);
}

// Mac 0x32e8a0, Win 0x40be80
void ClickPullSolverCallback::OnAnchorAdded(AnchorPhysicsSolverAnchor* /*anchor*/)
{
}

// Mac 0x32e8b0 (erase 0x32e930), Win 0x4b1c50
void ClickPullSolverCallback::OnAnchorRemoved(AnchorPhysicsSolverAnchor* anchor)
{
    RefPointer<Anchor> removed = static_cast<Anchor*>(anchor->GetUserData());
    for (UInt32 i = 0; i < m_anchors.GetSize(); ++i) {
        if (m_anchors[i] == removed) {
            m_anchors.Erase(i);
            break;
        }
    }
}

// Mac 0x32ea60, Win 0x40be80
void ClickPullSolverCallback::OnAnchorChanged(AnchorPhysicsSolverAnchor* /*anchor*/)
{
}

} // namespace LEGO
