// LEGO Digital Designer — restored source
// LEGO::StringPullSolverCallback — solver callback of the StringPullTool, registered by
//   Scene::CreateStringPullSolverCallback (Mac 0xb1b90, Win 0x480dd0). Each solver anchor gets an
//   Anchor (its user data) that, whenever the target moves, looks for an axle of another body
//   near the target and aligns one of its own axles with it (AxleAligner); without one the
//   anchor just pulls its point to the target.
// TU "StringPullSolverCallback.cpp" (Mac TU 349 0x32f470-0x3305ef; Win 0x4d40d0-0x4d51cf,
// 0x4f9340, 0x4a1430).
// RTTI: class LEGO::StringPullSolverCallback : AnchorPhysicsSolverCallback.
//   Mac sizeof 0x20 (operator new in Mac 0xb1b90); vtable 0x74c4c8 {QueryInterface 0x32fc70,
//       AddRef 0x32fcf0, Release 0x32fd10, OnBegin 0x32f5c0, OnEnd 0x32f5d0, OnResult 0x32f620,
//       OnStep 0x32f630, OnAnchorAdded 0x32f7a0, OnAnchorRemoved 0x32f960, OnAnchorChanged
//       0x32fb10, D1 0x330390, D0 0x330400}. Constructor Mac 0x32f4d0.
//   Win vtable 0xa25608 {0x4d43a0, 0x4d43e0, 0x4d4400, 0x4f9340, 0x4d41d0, 0x40be80, 0x4d41e0,
//       0x4d4210, 0x4d42c0, 0x4d4380, sdd 0x4d40d0}. Constructor Win 0x4d40f0.
// RTTI StringPullSolverCallback::Anchor : Object. Mac sizeof 0x48, vtable 0x74c500 {D1
//   0x330340, D0 0x330350, Object slots}; Win sizeof 0x50 (the multimap is 0x24 bytes), vtable
//   0xa25638 {sdd 0x4d45f0}.
// Member and method names inferred.
#pragma once
#include "LEGO/Types.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "BrickPhysics.h"
#include "Object.h"
#include "Geometry.h"
#include "Aligner.h"

namespace LEGO {

class Scene;
class PrimitiveElement;
class ConnectionField;

class StringPullSolverCallback : public AnchorPhysicsSolverCallback {
public:
    class Anchor : public Object {
    public:
        typedef BasicMultimap<float, Aligner::Field, Less<float> > FieldMap;

        // Registers as `anchor`'s user data and collects the fields of every element of the
        // anchored body by their distance to the anchor target (GetFieldDistance).
        // Mac 0x32fd50, Win 0x4d4430
        Anchor(Scene* scene, AnchorPhysicsSolverAnchor* anchor);
        // Mac 0x3302a0 (D1 0x330340, D0 0x330350), Win 0x4d4b20 / sdd 0x4d45f0
        virtual ~Anchor();

        // The anchor's target moved: the fields of the elements within 1.0 of the target are
        // searched for a new target (FindTarget); a running aligner is restarted for the same
        // pair, else stopped; then Step(0). Mac 0x32fb30, Win 0x4d4610
        void Update();
        // One solver step: the aligner steps (and goes when done); without one, a target field
        // gets a new AxleAligner (axles only), else the point is pulled to the target.
        // Mac 0x32f680, Win 0x4d4770
        void Step(Real32 time);
        // Squared distance of an axle field (at `elementTransform`) to the anchor target if
        // below 1.0, else FLT_MAX (also for every other field). Mac 0x32ffe0, Win 0x4d48c0
        Real32 GetFieldDistance(const ConnectionField* field, const Matrix3D& elementTransform);
        // The nearest field of the `elements` (other bodies) compatible with one of the own
        // fields, trying the own fields in the order of their distance until one has a match.
        // Mac 0x330130, Win 0x4d4970
        void FindTarget(const BasicArray<Pair<PrimitiveElement*, Matrix3D> >& elements);

        AnchorPhysicsSolverAnchor* GetSolverAnchor() const { return m_anchor; }
        const Aligner::Field& GetField() const { return m_field; }
        const Aligner::Field& GetTarget() const { return m_target; }
        const FieldMap& GetFields() const { return m_fields; }
        Aligner* GetAligner() const { return m_aligner; }

    private:
        AnchorPhysicsSolverAnchor* m_anchor;    // +0x08 (not referenced)
        Scene*                     m_scene;     // +0x0c
        Aligner::Field             m_field;     // +0x10 own field of the found pair
        Aligner::Field             m_target;    // +0x1c target field (m_index stays 0)
        FieldMap                   m_fields;    // +0x28 own fields by distance
        RefPointer<Aligner>        m_aligner;   // +0x44
    };

    // Mac 0x32f4d0, Win 0x4d40f0: no anchors, one reference (the creator's).
    explicit StringPullSolverCallback(Scene* scene);

    // Only AnchorPhysicsSolverCallback::GetIID(). Mac 0x32fc70, Win 0x4d43a0
    virtual Int32  LEGO_COM_CALL QueryInterface(const InterfaceID& id, void** object);
    virtual UInt32 LEGO_COM_CALL AddRef();                                   // Mac 0x32fcf0, Win 0x4d43e0
    virtual UInt32 LEGO_COM_CALL Release();                                  // Mac 0x32fd10, Win 0x4d4400
    virtual void OnBegin(PhysicsSolver* solver);                             // Mac 0x32f5c0, Win 0x4f9340
    // The anchors go. Mac 0x32f5d0, Win 0x4d41d0
    virtual void OnEnd(bool& result);
    virtual void OnResult(bool& success);                                    // Mac 0x32f620, Win 0x40be80 (empty)
    // Anchor::Step of every anchor. Mac 0x32f630, Win 0x4d41e0
    virtual void OnStep(Real32 time);
    // A new Anchor for `anchor`. Mac 0x32f7a0, Win 0x4d4210
    virtual void OnAnchorAdded(AnchorPhysicsSolverAnchor* anchor);
    // Its Anchor goes. Mac 0x32f960, Win 0x4d42c0
    virtual void OnAnchorRemoved(AnchorPhysicsSolverAnchor* anchor);
    // Its Anchor::Update. Mac 0x32fb10, Win 0x4d4380
    virtual void OnAnchorChanged(AnchorPhysicsSolverAnchor* anchor);
    virtual ~StringPullSolverCallback();                                     // Mac 0x330390 / 0x330400, Win sdd 0x4d40d0

    // AnchorPhysicsSolver::CreateAnchor of the running solver (NULL when no solver has begun
    // or it is no anchor solver). Mac 0x32f520, Win 0x4d4160
    AnchorPhysicsSolverAnchor* CreateAnchor(PhysicsRigidElement* body, const Matrix3D& local,
                                            const Matrix3D& target, UInt32 count,
                                            PhysicsRigidElement* const* bodies);

    const BasicArray<RefPointer<Anchor> >& GetAnchors() const { return m_anchors; }
    PhysicsSolver* GetSolver() const { return m_solver; }

    // ---- data (Mac offsets; public for the tests) ------------------------------------------
    BasicArray<RefPointer<Anchor> > m_anchors;    // +0x04
    Scene*                          m_scene;      // +0x14
    UInt32                          m_refCount;   // +0x18 1
    PhysicsSolver*                  m_solver;     // +0x1c 0 (OnBegin; not referenced)

private:
    StringPullSolverCallback(const StringPullSolverCallback&);
    StringPullSolverCallback& operator=(const StringPullSolverCallback&);
};

} // namespace LEGO
