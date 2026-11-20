// LEGO Digital Designer — restored source
// LEGO::ClickPullSolverCallback — solver callback of the ClickPullTool, registered by
//   Scene::CreateClickPullSolverCallback (Mac 0xb1bf0, Win 0x480e50). The tool adds one anchor
//   at the clicked point; its Anchor (the solver anchor's user data) picks the pulled body's
//   field points near it once, then on every mouse move looks along the ray for a compatible
//   field point of another body: found, an AxleAligner / PlanarAligner aligns the fields; else
//   the target follows the ray in the plane of the anchor point.
// TU "ClickPullSolverCallback.cpp" (Mac TU 348 0x32dc50-0x32f46f; Win 0x4b18f0-0x4b292f).
// RTTI: class LEGO::ClickPullSolverCallback : AnchorPhysicsSolverCallback.
//   Mac sizeof 0x24 (operator new in Mac 0xb1bf0); vtable 0x74c450 {QueryInterface 0x32ea70,
//       AddRef 0x32eaf0, Release 0x32eb10, OnBegin 0x32e6b0, OnEnd 0x32e6c0, OnResult 0x32e6d0,
//       OnStep 0x32e6e0, OnAnchorAdded 0x32e8a0, OnAnchorRemoved 0x32e8b0, OnAnchorChanged
//       0x32ea60, D1 0x32f210, D0 0x32f280}. Constructor Mac 0x32dcb0.
//   Win vtable 0xa2478c {0x4d43a0, 0x4b1d10, 0x4b1d30, 0x4b1c10, 0x40be80, 0x40be80, 0x4b1c20,
//       0x40be80, 0x4b1c50, 0x40be80, sdd 0x4b18f0}. Constructor Win 0x4b1910.
// RTTI ClickPullSolverCallback::Anchor : Object. Mac sizeof 0xdc, vtable 0x74c488 {D1
//   0x32f100, D0 0x32f180, Object slots}; Win vtable 0xa247bc {sdd 0x4b1f30}.
// Member and method names inferred.
#pragma once
#include "LEGO/Types.h"
#include "BasicArray.h"
#include "BrickPhysics.h"
#include "Object.h"
#include "Geometry.h"
#include "Aligner.h"
#include "ConnectionField.h"
#include "Scene.h"

namespace LEGO {

class PrimitiveElement;

class ClickPullSolverCallback : public AnchorPhysicsSolverCallback {
public:
    class Anchor : public Object {
    public:
        // Registers as `anchor`'s user data, picks the field points of the anchored body
        // within 9.0 of `point` (world) and runs Update(origin, direction).
        // Mac 0x32eb50, Win 0x4b1d60
        Anchor(Scene* scene, AnchorPhysicsSolverAnchor* anchor, const Vector3& origin,
               const Vector3& direction, const Vector3& point);
        // Mac 0x32f100 / 0x32f180, Win 0x4b2800 / sdd 0x4b1f30
        virtual ~Anchor();

        // The mouse ray moved: the elements in the cone around the ray that covers the pulled
        // body are searched for a target point (FindTarget); found, the anchor target is the
        // target point with its snap offset, else the anchor point is moved along the ray's
        // plane through it. A running aligner is restarted for the same pair, else stopped;
        // then Step(0). Mac 0x32dfe0, Win 0x4b2000
        void Update(const Vector3& origin, const Vector3& direction);
        // One solver step: the aligner steps (and goes when done); without one, a target field
        // gets an AxleAligner (axles) or a PlanarAligner (planar fields), else the point is
        // pulled to the target. Mac 0x32e730, Win 0x4b2410
        void Step(Real32 time);
        // The nearest ray hit (ConnectionField::Pick, closer by more than 0.0001) of a field of
        // the picked elements (other bodies) compatible with one of the own picked points.
        // Mac 0x32ee40, Win 0x4b2570
        void FindTarget(Scene::PickResult& elements, const Vector3& origin, const Vector3& direction);

        AnchorPhysicsSolverAnchor* GetSolverAnchor() const { return m_anchor; }
        const Aligner::Field& GetField() const { return m_field; }
        const Aligner::Field& GetTarget() const { return m_target; }
        const Matrix3D& GetOffset() const { return m_offset; }
        const ConnectionField::PickerAccumulator& GetPicked() const { return m_picked; }
        Aligner* GetAligner() const { return m_aligner; }

    private:
        AnchorPhysicsSolverAnchor*          m_anchor;    // +0x08 (not referenced)
        Scene*                              m_scene;     // +0x0c
        Matrix3D                            m_offset;    // +0x10 snap offset of the target point
        Aligner::Field                      m_field;     // +0x50 own picked point of the pair
        Aligner::Field                      m_target;    // +0x5c target point (element 0: none)
        ConnectionField::PickerAccumulator  m_picked;    // +0x68 own field points near the click
        RefPointer<Aligner>                 m_aligner;   // +0xd8
    };

    // Mac 0x32dcb0, Win 0x4b1910: no anchors, one reference (the creator's).
    explicit ClickPullSolverCallback(Scene* scene);

    // Only AnchorPhysicsSolverCallback::GetIID(). Mac 0x32ea70, Win 0x4d43a0
    virtual Int32  LEGO_COM_CALL QueryInterface(const InterfaceID& id, void** object);
    virtual UInt32 LEGO_COM_CALL AddRef();                                   // Mac 0x32eaf0, Win 0x4b1d10
    virtual UInt32 LEGO_COM_CALL Release();                                  // Mac 0x32eb10, Win 0x4b1d30
    virtual void OnBegin(PhysicsSolver* solver);                             // Mac 0x32e6b0, Win 0x4b1c10
    virtual void OnEnd(bool& result);                                        // Mac 0x32e6c0, Win 0x40be80 (empty)
    virtual void OnResult(bool& success);                                    // Mac 0x32e6d0, Win 0x40be80 (empty)
    // Anchor::Step of every anchor. Mac 0x32e6e0, Win 0x4b1c20
    virtual void OnStep(Real32 time);
    virtual void OnAnchorAdded(AnchorPhysicsSolverAnchor* anchor);           // Mac 0x32e8a0, Win 0x40be80 (empty)
    // Its Anchor goes. Mac 0x32e8b0, Win 0x4b1c50
    virtual void OnAnchorRemoved(AnchorPhysicsSolverAnchor* anchor);
    virtual void OnAnchorChanged(AnchorPhysicsSolverAnchor* anchor);         // Mac 0x32ea60, Win 0x40be80 (empty)
    virtual ~ClickPullSolverCallback();                                      // Mac 0x32f210 / 0x32f280, Win sdd 0x4b18f0

    // The anchor of the click (only while the solver runs and no anchor exists yet):
    // AnchorPhysicsSolver::CreateAnchor(body, local, target, count, bodies) and its Anchor
    // (`origin` / `direction` the mouse ray, `point` the clicked point in the world). NULL
    // otherwise. NOTE: `element` is not used. Mac 0x32dd00, Win 0x4b1980
    AnchorPhysicsSolverAnchor* AddAnchor(PhysicsRigidElement* body, const Matrix3D& local, const Matrix3D& target,
                                         UInt32 count, PhysicsRigidElement* const* bodies,
                                         const Vector3& origin, const Vector3& direction,
                                         PrimitiveElement* element, const Vector3& point);
    // Anchor::Update of every anchor. Mac 0x32df90, Win 0x4b1ac0
    void UpdateAnchors(const Vector3& origin, const Vector3& direction);
    // Scene::CanConnect of the rigid elements of the first anchor's bodies (nothing without
    // an anchor). Mac 0x32e360, Win 0x4b1b00
    void CanConnect(Scene::ConnectResult& result);
    // AnchorPhysicsSolverAnchor::GetState of the first anchor; 1 without one.
    // Mac 0x32e680, Win 0x4b1bf0
    UInt32 GetState() const;

    const BasicArray<RefPointer<Anchor> >& GetAnchors() const { return m_anchors; }
    PhysicsSolver* GetSolver() const { return m_solver; }
    bool IsAdding() const { return m_adding; }

    // ---- data (Mac offsets; public for the tests) ------------------------------------------
    bool                            m_adding;     // +0x04 true during AddAnchor's CreateAnchor (not read)
    BasicArray<RefPointer<Anchor> > m_anchors;    // +0x08
    Scene*                          m_scene;      // +0x18
    UInt32                          m_refCount;   // +0x1c 1
    PhysicsSolver*                  m_solver;     // +0x20 0 (OnBegin; not referenced)

private:
    ClickPullSolverCallback(const ClickPullSolverCallback&);
    ClickPullSolverCallback& operator=(const ClickPullSolverCallback&);
};

} // namespace LEGO
