// LEGO Digital Designer — restored source
// LEGO::StringPullSolverCallback and StringPullSolverCallback::Anchor. See
// StringPullSolverCallback.h.
//
// Original object file: StringPullSolverCallback.cpp (Mac TU 349 0x32f470-0x3305ef; Win
// 0x4d40d0-0x4d51cf, OnBegin 0x4f9340 (folded), the erase helper 0x4a1430).
#include "StringPullSolverCallback.h"
#include "AxleAligner.h"
#include "ConnectionField.h"
#include "ConnectionFieldAxle.h"
#include "PrimitiveElement.h"
#include "PrimitiveRigidElement.h"
#include "RigidElement.h"
#include "Scene.h"

#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <libkern/OSAtomic.h>
#endif

namespace LEGO {

// ===========================================================================================
// StringPullSolverCallback::Anchor
// ===========================================================================================

// Mac 0x32fd50, Win 0x4d4430
StringPullSolverCallback::Anchor::Anchor(Scene* scene, AnchorPhysicsSolverAnchor* anchor)
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
                ConnectionField* field = element->GetField(f);
                Real32 distance = GetFieldDistance(field, transform);
                m_fields.Insert(distance, Aligner::Field(element, field, 0));
            }
        }
    }
}

// Mac 0x3302a0 (D1 0x330340, D0 0x330350), Win 0x4d4b20 / sdd 0x4d45f0
// NOTE: the original leaves its address as the solver anchor's user data. When the solver stops,
// OnEnd frees the anchors first and the solver's Stop then removes its anchors, so
// OnAnchorRemoved takes a reference to the freed anchor and drops it again (use after free,
// double free); fixed: the anchor detaches itself from the solver anchor.
StringPullSolverCallback::Anchor::~Anchor()
{
    if (m_anchor->GetUserData() == this)
        m_anchor->SetUserData(0);
}

// Mac 0x32ffe0, Win 0x4d48c0
Real32 StringPullSolverCallback::Anchor::GetFieldDistance(const ConnectionField* field,
                                                          const Matrix3D& elementTransform)
{
    Matrix3D frame(elementTransform, field->GetTransform());
    if (field->GetType() == ConnectionField::TYPE_AXLE) {
        Vector3 position(frame.GetTranslation());
        Vector3 end = Vector3(0.0f, static_cast<const ConnectionFieldAxle*>(field)->GetLength(), 0.0f) * frame;
        Real32 distance = PointSegmentDistanceSquared(position, end,
                                                      m_anchor->GetTargetTransform().GetTranslation());
        if (distance < 1.0f)
            return distance;
    } else if (field->GetType() == ConnectionField::TYPE_PLANAR) {
        // NOTE: the plane of the field is built and not used (both builds)
        Plane plane(Rotate(Vector3(0.0f, 1.0f, 0.0f), frame), frame.GetTranslation());
        (void)plane;
    }
    return 3.4028235e+38f;   // FLT_MAX
}

// Mac 0x330130, Win 0x4d4970
void StringPullSolverCallback::Anchor::FindTarget(const BasicArray<Pair<PrimitiveElement*, Matrix3D> >& elements)
{
    m_target.m_element = 0;
    Real32 best = 3.4028235e+38f;   // FLT_MAX
    FieldMap::Iterator it = m_fields.Begin();
    do {
        if (it == m_fields.End())
            return;
        const Aligner::Field& own = it->second;
        for (UInt32 i = 0; i < elements.GetSize(); ++i) {
            PrimitiveElement* element = elements[i].first;
            if (element->GetRigidElement()->GetRigidElement()->GetBody() == m_anchor->GetElement())
                continue;
            Matrix3D transform;
            element->GetTransform(transform);
            for (UInt32 f = 0; f < element->GetFieldCount(); ++f) {
                ConnectionField* field = element->GetField(f);
                if (!own.m_field->IsCompatible(*field))
                    continue;
                Real32 distance = GetFieldDistance(field, transform);
                if (distance < best) {
                    m_field = own;
                    m_target.m_element = element;
                    m_target.m_field = field;
                    best = distance;
                }
            }
        }
        ++it;
    } while (m_target.m_element == 0);
}

// Mac 0x32fb30, Win 0x4d4610
void StringPullSolverCallback::Anchor::Update()
{
    Sphere sphere(1.0f);
    sphere.m_center *= m_anchor->GetTargetTransform();
    BasicArray<Pair<PrimitiveElement*, Matrix3D> > elements;
    Scene::PrimitiveSpherePicker picker(sphere, &elements);
    m_scene->Pick(picker);
    FindTarget(elements);
    if (m_aligner) {
        if (m_aligner->IsPair(m_field, m_target))
            m_aligner->Start();
        else
            m_aligner->Stop();
    }
    Step(0.0f);
}

// Mac 0x32f680, Win 0x4d4770
void StringPullSolverCallback::Anchor::Step(Real32 time)
{
    if (m_aligner) {
        m_aligner->Step(time);
        if (m_aligner->IsDone() && m_aligner)
            m_aligner = 0;
        return;
    }
    if (m_target.m_element == 0) {
        m_anchor->SetPointAttraction(m_anchor->GetLocalTransform().GetTranslation(),
                                     m_anchor->GetTargetTransform().GetTranslation());
        return;
    }
    if (m_target.m_field->GetType() == ConnectionField::TYPE_AXLE)
        m_aligner = new AxleAligner(m_anchor, m_field, m_target);
    if (m_aligner)
        m_aligner->Start();
}

// ===========================================================================================
// StringPullSolverCallback
// ===========================================================================================

// Mac 0x32f4d0, Win 0x4d40f0
StringPullSolverCallback::StringPullSolverCallback(Scene* scene)
    : m_scene(scene), m_refCount(1), m_solver(0)
{
}

// Mac 0x330390 (D1) / 0x330400 (D0), Win sdd 0x4d40d0 (body 0x4d4070)
StringPullSolverCallback::~StringPullSolverCallback()
{
}

// Mac 0x32fc70, Win 0x4d43a0 (shared with ClickPullSolverCallback)
Int32 StringPullSolverCallback::QueryInterface(const InterfaceID& id, void** object)
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

// Mac 0x32fcf0, Win 0x4d43e0
UInt32 StringPullSolverCallback::AddRef()
{
#if defined(_WIN32)
    return (UInt32)InterlockedIncrement((LONG*)&m_refCount);
#else
    return (UInt32)OSAtomicAdd32(1, (int32_t*)&m_refCount);
#endif
}

// Mac 0x32fd10, Win 0x4d4400
UInt32 StringPullSolverCallback::Release()
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

// Mac 0x32f520, Win 0x4d4160
AnchorPhysicsSolverAnchor* StringPullSolverCallback::CreateAnchor(PhysicsRigidElement* body, const Matrix3D& local,
                                                                  const Matrix3D& target, UInt32 count,
                                                                  PhysicsRigidElement* const* bodies)
{
    if (m_solver == 0)
        return 0;
    AnchorPhysicsSolver* solver = 0;
    if (m_solver->QueryInterface(AnchorPhysicsSolver::GetIID(), (void**)&solver) < 0)
        return 0;
    AnchorPhysicsSolverAnchor* anchor = solver->CreateAnchor(body, local, target, count, bodies);
    solver->Release();
    return anchor;
}

// Mac 0x32f5c0, Win 0x4f9340
void StringPullSolverCallback::OnBegin(PhysicsSolver* solver)
{
    m_solver = solver;
}

// Mac 0x32f5d0, Win 0x4d41d0
// NOTE: the original keeps m_solver after the solver has ended (the scene releases it), so a
// later CreateAnchor of the tool calls the freed solver; fixed: m_solver is reset, CreateAnchor
// returns NULL.
void StringPullSolverCallback::OnEnd(bool& /*result*/)
{
    m_anchors.Clear();
    m_solver = 0;
}

// Mac 0x32f620, Win 0x40be80
void StringPullSolverCallback::OnResult(bool& /*success*/)
{
}

// Mac 0x32f630, Win 0x4d41e0
void StringPullSolverCallback::OnStep(Real32 time)
{
    for (UInt32 i = 0; i < m_anchors.GetSize(); ++i)
        m_anchors[i]->Step(time);
}

// Mac 0x32f7a0, Win 0x4d4210
void StringPullSolverCallback::OnAnchorAdded(AnchorPhysicsSolverAnchor* anchor)
{
    RefPointer<Anchor> added = new Anchor(m_scene, anchor);
    m_anchors.PushBack(added);
}

// Mac 0x32f960 (erase 0x32f9e0), Win 0x4d42c0 (erase 0x4a1430)
void StringPullSolverCallback::OnAnchorRemoved(AnchorPhysicsSolverAnchor* anchor)
{
    RefPointer<Anchor> removed = static_cast<Anchor*>(anchor->GetUserData());
    for (UInt32 i = 0; i < m_anchors.GetSize(); ++i) {
        if (m_anchors[i] == removed) {
            m_anchors.Erase(i);
            break;
        }
    }
}

// Mac 0x32fb10, Win 0x4d4380
void StringPullSolverCallback::OnAnchorChanged(AnchorPhysicsSolverAnchor* anchor)
{
    static_cast<Anchor*>(anchor->GetUserData())->Update();
}

} // namespace LEGO
