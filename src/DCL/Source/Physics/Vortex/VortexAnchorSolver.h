// LEGO Digital Designer — restored source
// LEGO::VortexAnchorSolver — the solver behind PhysicsScene::AddSolverCallback (String / Click
// pull tools, aligners): anchors pull a body frame towards a target with Vortex
// joints (point attraction, ball and socket, cylindrical, orientation lock, angular-1 /
// position-3) while the world steps; ActiveElement keeps the held bodies' damping.
// VortexAnchorSolver.cpp (Mac TU 421, 0x399830-0x39e7bf; Win 0x52ec30-0x5319bf); also emits the
// at-rest helper the other Vortex solvers share (Mac 0x399f50).

// RTTI: class LEGO::VortexAnchorSolver : BaseBrickPhysicsSimulationSolver (+0),
//   AnchorPhysicsSolver (+0x18). sizeof 0x70.
//   Mac vtable 0x757fd8 {0x399d40, 0x399d50, 0x399df0, D1 0x39e480, D0 0x39e4b0, 0x39a000,
//     0x259440 .. 0x259490, QI 0x399ba0, 0x39e530, 0x39e540, 0x39e550, CreateAnchor 0x39a020}
//     + 0x758024 (Object 0x39e490 / 0x39e4f0), 0x758040 (PhysicsSolver 0x399c20, 0x39e560 ..),
//     0x758058 (AnchorPhysicsSolver 0x399cb0, 0x39e5c0, 0x39e5e0, 0x39e600, 0x39a660).
//   Win vtable 0xa2a994 {0x459790, 0x52ecf0, 0x52eda0, sdd 0x52ec30, 0x52eee0, ..} + 0xa2a9c4,
//     0xa2a9d8 (QI 0x52ec50), 0xa2a9ec (.., CreateAnchor 0x52eef0).
// RTTI: VortexAnchorSolver::ActiveElement : Object. sizeof 0x20. Mac 0x758074 {D1 0x39a790,
//   D0 0x39a810, Object}, Win 0xa2aa04 {sdd 0x52f130, ..}.
// RTTI: VortexAnchorSolver::Anchor : Object (+0), AnchorPhysicsSolverAnchor (+8). sizeof 0x190.
//   Mac 0x758090 {D1 0x39d820, D0 0x39d870, OnFinalRelease 0x39d8e0, Object AddRef / Release,
//     the 29 interface slots 0x39ad80 .. 0x39ce90, Update 0x39cf10} + 0x758124 (thunks).
//   Win 0xa2aa20 {sdd 0x52f430, 0x531100, Object, Update 0x530910} + 0xa2aa3c (the interface:
//     QI 0x52f450 .. DisableCollisions 0x5308e0).
// Every name is inferred.
#pragma once
#include "LEGO/Types.h"
#include "Geometry.h"
#include "BasicArray.h"
#include "Object.h"
#include "BaseBrickPhysicsScene.h"

namespace Vx {
class VxUniverse;
class VxMechanism;
class VxPart;
class VxBallAndSocket;
class VxCylindrical;
class VxRPRO;
class VxAngular1Position3;
}

namespace LEGO {

class VortexPhysicsScene;
class VortexPhysicsRigidElement;
class PointGravitationAction;

class VortexAnchorSolver : public BaseBrickPhysicsSimulationSolver, public AnchorPhysicsSolver {
public:
    // A body of an anchor in the simulation: its damping scaled by its mass while held, put
    // back when the last anchor lets go.
    class ActiveElement : public Object {
    public:
        // Saves the linear / angular velocity damping, then mass * the solver's factors.
        // Mac 0x39a690, Win 0x52f060
        ActiveElement(VortexPhysicsRigidElement* element, const Real32& linearFactor, const Real32& angularFactor);
        // The saved damping back. Mac D1 0x39a790 / D0 0x39a810, Win 0x52f150 / sdd 0x52f130
        virtual ~ActiveElement();

        VortexPhysicsRigidElement* m_element;          // +0x08
        UInt32                     m_anchorCount;      // +0x0c anchors holding the body (1)
        double                     m_linearDamping;    // +0x10 saved
        double                     m_angularDamping;   // +0x18 saved
    };

    class Anchor : public Object, public AnchorPhysicsSolverAnchor {
    public:
        // `solver` gets a (COM) reference. Mac 0x39a8a0, Win inlined into CreateAnchor 0x52eef0
        Anchor(VortexAnchorSolver* solver, VortexPhysicsRigidElement* element, const Matrix3D& local,
               const Matrix3D& target, UInt32 count, PhysicsRigidElement* const* bodies);
        virtual ~Anchor();                                   // Mac D1 0x39d820 / D0 0x39d870, Win 0x531080 / sdd 0x52f430
        // Leaves the solver first. Mac 0x39d8e0, Win 0x531100
        virtual void OnFinalRelease();

        // ---- AnchorPhysicsSolverAnchor (answers its GetIID(); AddRef / Release go to Object
        // and return -1). Mac 0x39ad80 / 0x39ae90 / 0x39aed0 (thunks 0x39ae00 / 0x39aeb0 /
        // 0x39aef0), Win 0x52f450 / 0x5c57e0 / 0x52cca0
        virtual Int32  LEGO_COM_CALL QueryInterface(const InterfaceID& id, void** object);
        virtual UInt32 LEGO_COM_CALL AddRef();
        virtual UInt32 LEGO_COM_CALL Release();
        virtual void SetUserData(void* data);                // Mac 0x39af10, Win 0x52f4a0
        virtual void* GetUserData();                         // Mac 0x39af30, Win 0x52f4b0
        virtual PhysicsRigidElement* GetElement();           // Mac 0x39af50, Win 0x46ee40
        virtual const Matrix3D& GetLocalTransform();         // Mac 0x39af70, Win 0x52f4c0
        virtual const Matrix3D& GetTargetTransform();        // Mac 0x39af90, Win 0x52f4d0
        virtual UInt32 GetElementCount();                    // Mac 0x39afb0, Win 0x52f4e0
        virtual PhysicsRigidElement* const* GetElements();   // Mac 0x39aff0, Win 0x52f4f0
        // The callback's OnAnchorChanged, without a callback the point attraction moves along.
        // Mac 0x39b010, Win 0x52f500
        virtual void SetTargetTransform(const Matrix3D& target);
        virtual void Remove();                               // Mac 0x39b190, Win 0x52f560
        virtual void ClearPointAttraction();                 // Mac 0x39b410, Win 0x52f590
        // A PointGravitationAction (strength 1) on the universe's mechanism. Mac 0x39b4d0, Win 0x52f5e0
        virtual void SetPointAttraction(const Vector3& local, const Vector3& target);
        virtual void ClearBallAndSocket();                   // Mac 0x39b650, Win 0x52f6f0
        virtual void SetBallAndSocket(const Vector3& local, const Vector3& target);  // Mac 0x39b770, Win 0x52f770
        virtual void ClearCylindrical();                     // Mac 0x39ba90, Win 0x52f9d0
        virtual void SetCylindrical(const Vector3& position, const Vector3& axis, const Vector3& targetPosition,
                                    const Vector3& targetAxis);                      // Mac 0x39bb30, Win 0x52fa10
        virtual void ClearOrientation();                     // Mac 0x39bee0, Win 0x52fd70
        virtual void SetOrientation(const Real32& tolerance); // Mac 0x39bf80, Win 0x52fdb0
        virtual void ClearAngular();                         // Mac 0x39c0b0, Win 0x52feb0
        virtual void SetAngular(const Vector3& position, const Vector3& axis, const Vector3& targetPosition,
                                const Vector3& targetAxis, const Vector3& reference,
                                const Real32& offset);                               // Mac 0x39c270, Win 0x52fff0
        // NOTE: also gives the body the solver's damping again (mass * factors).
        virtual void ClearAngularReversed();                 // Mac 0x39c520, Win 0x530300
        // NOTE: `reference` is stored but not used (the target's secondary axis is targetAxis's
        // perpendicular).
        virtual void SetAngularReversed(const Vector3& position, const Vector3& axis,
                                        const Vector3& targetPosition, const Vector3& targetAxis,
                                        const Vector3& reference, const Real32& offset); // Mac 0x39c630, Win 0x5303c0
        virtual void Unknown24();                            // Mac 0x39c8c0, Win 0x459790 (empty)
        virtual void Unknown25(UInt32 a, UInt32 b);          // Mac 0x39c8e0, Win 0x424ec0 (empty)
        // 0 still moving, 1 not there and at rest (contacts too deep / a joint off its target /
        // a group joint stretched), 2 settled. Mac 0x39c900, Win 0x530680
        virtual UInt32 GetState();
        virtual void EnableCollisions(const PhysicsGeometryIdentifier* features, UInt32 count);  // Mac 0x39ce10, Win 0x5308b0
        virtual void DisableCollisions(const PhysicsGeometryIdentifier* features, UInt32 count); // Mac 0x39ce90, Win 0x5308e0
        using Object::AddRef;
        using Object::Release;

        // ---- own slot: after each solver step (`separation` = the step's joint result: 2 = a
        // joint broke, the forces drop to a quarter; else they grow by 20 % up to the maxima
        // until the joint is on its target, then the relaxation goes). `universe` unused.
        // Mac 0x39cf10, Win 0x530910
        virtual void Update(Vx::VxUniverse* universe, UInt32 separation);

        VortexAnchorSolver*                    m_solver;               // +0x0c (COM reference; 0 once removed)
        void*                                  m_userData;             // +0x10
        VortexPhysicsRigidElement*             m_element;              // +0x14
        Matrix3D                               m_local;                // +0x18 body space
        Matrix3D                               m_target;               // +0x58 world
        BasicArray<VortexPhysicsRigidElement*> m_bodies;               // +0x98 the bodies that move
        Vx::VxBallAndSocket*                   m_ballAndSocket;        // +0xa8
        Vx::VxPart*                            m_ballPart;             // +0xac (cleared with it; never set)
        Vector3                                m_ballTarget;           // +0xb0 world
        Vector3                                m_ballLocal;            // +0xbc body
        Vx::VxCylindrical*                     m_cylindrical;          // +0xc8
        Vector3                                m_cylinderPosition;     // +0xcc body
        Vector3                                m_cylinderAxis;         // +0xd8 body
        Vector3                                m_cylinderTargetPosition; // +0xe4 world
        Vector3                                m_cylinderTargetAxis;   // +0xf0 world
        Vx::VxRPRO*                            m_orientation;          // +0xfc
        Vx::VxAngular1Position3*               m_angular;              // +0x100
        Vector3                                m_angularPosition;      // +0x104 body
        Vector3                                m_angularAxis;          // +0x110 body
        Vector3                                m_angularTargetPosition; // +0x11c world
        Vector3                                m_angularTargetAxis;    // +0x128 world
        Vector3                                m_angularReference;     // +0x134 world
        Vector3                                m_angularOffsetTarget;  // +0x140 targetPosition + reference * offset
        Vx::VxAngular1Position3*               m_angularReversed;      // +0x14c
        Vector3                                m_reversedPosition;     // +0x150 body
        Vector3                                m_reversedAxis;         // +0x15c body
        Vector3                                m_reversedTargetPosition; // +0x168 world
        Vector3                                m_reversedTargetAxis;   // +0x174 world
        Vector3                                m_reversedReference;    // +0x180 (not used)
        PointGravitationAction*                m_attraction;           // +0x18c (one VxRefCount reference)

    private:
        UInt32 MovingState() const;
        Anchor(const Anchor&);
        Anchor& operator=(const Anchor&);
    };

    // `callback` is referenced twice (base and AnchorPhysicsSolverCallback). Mac 0x399830 (C1
    // 0x399b90), Win inlined into AddSolverCallback 0x524710
    VortexAnchorSolver(VortexPhysicsScene* scene, PhysicsSolverCallback* callback);
    // NOTE: the snapshot is not released here (Stop does). Mac D1 0x39e480 (body 0x39e1b0) / D0
    // 0x39e4b0, Win 0x531550 / sdd 0x52ec30
    virtual ~VortexAnchorSolver();
    virtual void   Start();                               // Mac 0x399d40, Win 0x459790 (empty)
    // Puts the snapshot back when a joint of the group is stretched, removes every anchor.
    // Mac 0x399d50, Win 0x52ecf0
    virtual void   Stop(bool& result);
    // Up to three world steps of 1/30 s with the anchors updating; the snapshot follows while
    // the joints hold (and is put back when one breaks, twice at most). 0. Mac 0x399df0, Win 0x52eda0
    virtual UInt32 Step(Real32 time);
    virtual void   OnBeginUpdate();                       // Mac 0x39a000, Win 0x52eee0: StopSolver(false)
    // Answers AnchorPhysicsSolver::GetIID(), then the base's. Mac 0x399ba0 (thunks 0x399c20 /
    // 0x399cb0), Win 0x52ec50
    virtual Int32  LEGO_COM_CALL QueryInterface(const InterfaceID& id, void** object);
    virtual UInt32 LEGO_COM_CALL AddRef();                // Mac 0x39e530 (the base's)
    virtual UInt32 LEGO_COM_CALL Release();               // Mac 0x39e540 (the base's)
    virtual PhysicsSolverCallback* GetCallback();         // Mac 0x39e550 (the base's)
    // Mac 0x39a020 (thunk 0x39a660), Win 0x52eef0
    virtual AnchorPhysicsSolverAnchor* CreateAnchor(PhysicsRigidElement* body, const Matrix3D& local,
                                                    const Matrix3D& target, UInt32 count,
                                                    PhysicsRigidElement* const* bodies);
    using Object::AddRef;
    using Object::Release;

    // The anchor's bodies join the active elements (new ones: manual motion, their joints join
    // the group); a new snapshot when one was added. Mac 0x39a230
    void AddActiveElements(Anchor* anchor);
    // The opposite (bodies frozen off; the last anchor of a body takes it out of the simulation);
    // the group and the snapshot are rebuilt when one went. Mac 0x39dd20
    void RemoveActiveElements(Anchor* anchor);
    // Takes `anchor` out (its joints / actions cleared, OnAnchorRemoved). Mac 0x39b1d0, Win 0x531630
    void RemoveAnchor(Anchor* anchor);

    // The damping factors of the active elements (Mac __data 0x778a68 / 0x778a6c: 10, 10) and the
    // force maxima of the anchors' joints (Mac 0x778a60 / 0x778a64: 10000, 10000).
    static Real32 s_linearDampingFactor;
    static Real32 s_angularDampingFactor;
    static Real32 s_maxForce;
    static Real32 s_maxAngularForce;

    VortexPhysicsScene*                           m_vortexScene;      // +0x1c
    AnchorPhysicsSolverCallback*                  m_anchorCallback;   // +0x20 referenced (the callback)
    Real32                                        m_linearFactor;     // +0x24
    Real32                                        m_angularFactor;    // +0x28
    BasicArray<RefPointer<ActiveElement> >        m_activeElements;   // +0x2c
    BasicArray<BaseBrickPhysicsConnectionHelper*> m_connections;      // +0x3c the joints of the active bodies
    PhysicsSnapshot*                              m_snapshot;         // +0x4c
    BasicArray<RefPointer<Anchor> >               m_anchors;          // +0x50
    BasicArray<Vx::VxMechanism*>                  m_mechanisms;       // +0x60 (removed / deleted by RemoveAnchor; never filled)

private:
    VortexAnchorSolver(const VortexAnchorSolver&);
    VortexAnchorSolver& operator=(const VortexAnchorSolver&);
};

// Every contact of the scene's universe is at most 0.0139 deep (VxDynamicsContact::getPenetration);
// the Vortex solvers' "at rest" test. Mac 0x399f50 (attributed to VortexAnchorSolver), Win
// inlined. (name inferred)
bool HasNoDeepContacts(VortexPhysicsScene* scene);

} // namespace LEGO
