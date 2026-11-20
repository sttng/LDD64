// LEGO Digital Designer — restored source
// LEGO::BaseBrickPhysicsScene — engine-independent physics world (bodies, joints, update bracket
// with dirty lists, snapshots, running solver) plus this layer's solvers: SimulationCallback /
// SimulationSolver (protocol), AnimatingSolver (gravity animation), Base / Manual / Simulation
// HingeSolver (turn a hinge with what it moves), Snapshot. VortexPhysicsScene (Mac TU 435)
// implements the engine slots.
// BaseBrickPhysicsScene.cpp (Mac TU 211, 0x254240-0x25ab6f; Win 0x5209b0-0x5216ef,
// 0x528970-0x528a5f, 0x5c56e0-0x5c776b, 0x49c250, 0x53e6c0). Scene and snapshot ctors inline on
// Mac (in VortexPhysicsScene); Win scene ctor out of line (0x520db0).

// RTTI (Mac / Win vtables):
//   BaseBrickPhysicsScene : PhysicsScene. sizeof 0x90. Mac 0x736b78 {QI 0x255730, AddRef 0x2557b0,
//     Release 0x2557d0, 3 pure, 0x256120, 0x256180, 0x255810, 0x255a70, 8 pure, 0x255ad0, 2 pure,
//     0x255af0, 0x255f50, 0x255ff0, 0x256100, 17 pure, 0x259970, 0x258590, pure, D1 0x259980,
//     D0 0x259990}; Win 0xa29d2c {0x5c5d00, 0x5c5d40, 0x5c5d60, ..., 0x5c6070, 0x5c60c0,
//     0x5c5d90, 0x5c5e00, ..., 0x40c160, ..., 0x5c5e80, 0x5c6010, 0x5c6030, 0x5c6060, ...,
//     0x520da0, 0x5c6e80, pure, sdd 0x520fd0}. NOTE: the Mac analysis says 7 for this
//     class (wrong: D1/D0 are slots 45/46, app_anchor.py corrects it).
//   BaseBrickPhysicsSimulationCallback (no base). Mac 0x778bb0 {0x2550b0, 0x255130, pure,
//     D1 0x259a70, D0 0x259a80}; Win 0xa29e2c {0x459790, 0x40be80, pure, sdd 0x5209b0}.
//   BaseBrickPhysicsSimulationSolver : SimulationCallback (+0), Object (+8), PhysicsSolver
//     (+0x10). sizeof 0x18. Mac 0x736984 {0x2550b0, 0x255130, pure, D1 0x259370, D0 0x2593f0,
//     0x259430, 0x259440, 0x259450, 0x259460, 0x259470, 0x259480, 0x259490, QI 0x254280,
//     AddRef 0x254390, Release 0x2543d0, GetCallback 0x254410}; Win 0xa29e40 (11 slots).
//   BaseBrickPhysicsAnimatingSolver : SimulationSolver, AnimatePhysicsSolver (+0x18). sizeof
//     0x38. Mac 0x7368e8, Win 0xa2d55c {0x5c5b70, 0x5c5bb0, 0x5c5c10, sdd 0x5c5770, 0x459790,
//     0x5c5c50, 0x5c5ca0, 0x5c5cc0, ...}.
//   BaseBrickPhysicsBaseHingeSolver : SimulationSolver. sizeof 0x40. Mac 0x736a30 (slot 16
//     IsFor 0x2595f0, slot 17 pure), Win 0xa29e98.
//   BaseBrickPhysicsManualHingeSolver : BaseHingeSolver. Mac 0x736ac4 (slot 17 0x254460), Win
//     0xa29ef8 (slot 12 0x5c5810).
//   BaseBrickPhysicsSimulationHingeSolver : BaseHingeSolver (abstract; Win 0xa2ac28).
//   BaseBrickPhysicsSnapshot : PhysicsSnapshot. sizeof 0xc. Mac 0x736c4c {0x259290, 0x259310,
//     0x259330, 4 pure, D1 0x2599d0, D0 0x259a10}; Win 0xa29df8.
// Every member name is inferred; method names are inferred unless noted.
#pragma once
#include "LEGO/Types.h"
#include "Geometry.h"
#include "BasicArray.h"
#include "Object.h"
#include "BrickPhysics.h"

namespace LEGO {

class BaseBrickPhysics;
class BaseBrickPhysicsRigidElement;
class BaseBrickPhysicsConnectionHelper;
class BaseBrickPhysicsScene;
class BaseBrickPhysicsSnapshot;
class BaseBrickPhysicsBaseHingeSolver;

// A body with a transform (0x44 bytes; RTTI BasicArray<TransformedElement> Mac 0x771558).
struct TransformedElement {
    TransformedElement() : m_element(0) {}
    // The transform is identity, or the body's (GetTransform) when `getTransform`. Mac 0x254240
    TransformedElement(BaseBrickPhysicsRigidElement* element, bool getTransform);

    BaseBrickPhysicsRigidElement* m_element;     // +0x00
    Matrix3D                      m_transform;   // +0x04
};

// A body moved by a hinge solver, its transform when collected and the bodies attached to it
// that move with it (0x5c bytes; RTTI BasicArray<ActiveRigidElement> Mac 0x778ff8).
struct ActiveRigidElement {
    ActiveRigidElement() : m_element(0), m_unknown54(0), m_unknown58(0) {}
    // Identity transform, no children. Win 0x520930 (Mac inlined). NOTE: the original leaves
    // +0x54 / +0x58 uninitialised (copied along); zero here.
    explicit ActiveRigidElement(BaseBrickPhysicsRigidElement* element)
        : m_element(element), m_unknown54(0), m_unknown58(0)
    {
    }

    BaseBrickPhysicsRigidElement*  m_element;     // +0x00
    Matrix3D                       m_transform;   // +0x04
    BasicArray<TransformedElement> m_children;    // +0x44
    UInt32                         m_unknown54;   // +0x54 (not referenced in this layer)
    UInt32                         m_unknown58;   // +0x58
};

// ---- solvers ---------------------------------------------------------------------------------

// The per-step protocol of a running solver (the scene calls it). Also the second base of
// VortexHingeSolverHinge / VortexMoveTogetherHinge (Mac TU 435).
class BaseBrickPhysicsSimulationCallback {
public:
    explicit BaseBrickPhysicsSimulationCallback(BaseBrickPhysicsScene* scene) : m_scene(scene) {}
    virtual void   Start() {}                               // slot 0: Mac 0x2550b0, Win 0x459790
    virtual void   Stop(bool& /*result*/) {}                // slot 1: Mac 0x255130, Win 0x40be80
    // One step of `time`: 0 continue, 1 finished, 2 failed.
    virtual UInt32 Step(Real32 time) = 0;                   // slot 2
    virtual ~BaseBrickPhysicsSimulationCallback() {}       // Mac D1 0x259a70 / D0 0x259a80, Win sdd 0x5209b0

    BaseBrickPhysicsScene* m_scene;                         // +0x04
};

class BaseBrickPhysicsSimulationSolver : public BaseBrickPhysicsSimulationCallback, public Object,
                                         public PhysicsSolver {
public:
    // `callback` is referenced. Win 0x5209d0 (Mac inlined)
    BaseBrickPhysicsSimulationSolver(BaseBrickPhysicsScene* scene, PhysicsSolverCallback* callback);
    // Releases the callback. Mac D1 0x259370 / D0 0x2593f0, Win 0x520a70 / sdd 0x520af0
    virtual ~BaseBrickPhysicsSimulationSolver();

    // ---- scene notifications (all empty here)
    virtual void OnBeginUpdate() {}                                          // slot 5: Mac 0x259430, Win 0x459790
    virtual void OnEndUpdate(BasicArray<BaseBrickPhysicsRigidElement*>& /*bodies*/,
                             BasicArray<BaseBrickPhysicsConnectionHelper*>& /*connections*/) {} // slot 6: Mac 0x259440
    virtual void OnAddRigidElement(BaseBrickPhysicsRigidElement* /*body*/, const Matrix3D& /*transform*/) {} // slot 7: Mac 0x259450
    virtual void OnRemoveRigidElement(BaseBrickPhysicsRigidElement* /*body*/) {} // slot 8: Mac 0x259460
    virtual void OnAddSceneryElement(PhysicsSceneryElement* /*element*/, const Matrix3D& /*transform*/,
                                     UInt32 /*flags*/) {}                    // slot 9: Mac 0x259470, Win 0x520a60
    virtual void OnRemoveSceneryElement(PhysicsSceneryElement* /*element*/) {} // slot 10: Mac 0x259480
    virtual void OnSnapshotRestored() {}                                     // slot 11: Mac 0x259490

    // ---- PhysicsSolver (answers PhysicsSolver::GetIID(); AddRef / Release go to Object and
    // return -1). Mac 0x254280 / 0x254390 / 0x2543d0 / 0x254410 (thunks 0x254300 / 0x2543b0 /
    // 0x2543f0 / 0x254420); Win 0x5c5790 / 0x5c57e0 / 0x52cca0 / 0x51e790
    virtual Int32  LEGO_COM_CALL QueryInterface(const InterfaceID& id, void** object);
    virtual UInt32 LEGO_COM_CALL AddRef();
    virtual UInt32 LEGO_COM_CALL Release();
    virtual PhysicsSolverCallback* GetCallback();
    using Object::AddRef;
    using Object::Release;

    PhysicsSolverCallback* m_callback;                      // +0x14 referenced
};

class BaseBrickPhysicsAnimatingSolver : public BaseBrickPhysicsSimulationSolver, public AnimatePhysicsSolver {
public:
    // Takes over `bodies` (swapped); gravity (0, -9.82, 0). Mac 0x254d40, Win 0x5c59b0
    BaseBrickPhysicsAnimatingSolver(BaseBrickPhysicsScene* scene, BasicArray<BaseBrickPhysicsRigidElement*>& bodies);
    virtual ~BaseBrickPhysicsAnimatingSolver() {}          // Mac D1 0x259730 / D0 0x2597e0, Win 0x5c56e0 / sdd 0x5c5770

    virtual void   Start();                                 // Mac 0x255060, Win 0x5c5b70
    virtual void   Stop(bool& result);                      // Mac 0x2550c0, Win 0x5c5bb0
    virtual UInt32 Step(Real32 time);                       // Mac 0x255140, Win 0x5c5c10
    virtual void   OnEndUpdate(BasicArray<BaseBrickPhysicsRigidElement*>& bodies,
                               BasicArray<BaseBrickPhysicsConnectionHelper*>& connections); // Mac 0x2551a0, Win 0x5c5c50
    virtual void   OnAddRigidElement(BaseBrickPhysicsRigidElement* body, const Matrix3D& transform); // Mac 0x2552c0, Win 0x5c5ca0
    virtual void   OnRemoveRigidElement(BaseBrickPhysicsRigidElement* body); // Mac 0x2554f0, Win 0x5c5cc0

    // Answers AnimatePhysicsSolver::GetIID() and PhysicsSolver::GetIID(). Mac 0x254ec0 (thunks
    // 0x254fa0 / 0x254fc0), Win 0x5c5a80
    virtual Int32  LEGO_COM_CALL QueryInterface(const InterfaceID& id, void** object);
    virtual UInt32 LEGO_COM_CALL AddRef();                  // Mac 0x259820
    virtual UInt32 LEGO_COM_CALL Release();                 // Mac 0x259840
    virtual PhysicsSolverCallback* GetCallback();           // Mac 0x259860
    // Mac 0x254fe0 (thunk 0x255020), Win 0x5c5b20
    virtual void SetGravity(const Vector3* gravity);
    using Object::AddRef;
    using Object::Release;

    BasicArray<BaseBrickPhysicsRigidElement*> m_bodies;     // +0x1c
    Vector3                                   m_gravity;    // +0x2c
};

class BaseBrickPhysicsBaseHingeSolver : public BaseBrickPhysicsSimulationSolver {
public:
    // Copies both arrays; no callback. Mac 0x259af0, Win 0x520b10
    BaseBrickPhysicsBaseHingeSolver(BaseBrickPhysicsScene* scene, BaseBrickPhysicsConnectionHelper* hinge, bool first,
                                    const BasicArray<ActiveRigidElement>& elements,
                                    const BasicArray<BaseBrickPhysicsConnectionHelper*>& connections);
    // Mac D1 0x2595a0 (body 0x259fa0) / D0 0x2595b0, Win 0x520bd0 / sdd 0x520c80
    virtual ~BaseBrickPhysicsBaseHingeSolver() {}

    virtual UInt32 Step(Real32 time);                       // Mac 0x254430, Win 0x4f9670: 0
    // A scene update stops the solver (StopSolver(false)). Mac 0x254440, Win 0x5c5800
    virtual void   OnBeginUpdate();
    // The solver turns `hinge` from the `first` side. Mac 0x2595f0, Win 0x520bb0
    virtual bool   IsFor(BaseBrickPhysicsConnectionHelper* hinge, bool first);  // slot 16
    // Turns the bodies by `angle` about `axis` through `center` (world); `detach` is detached
    // first. False (and nothing moved) on a collision.
    virtual bool   Solve(Real32 angle, const Vector3& axis, const Vector3& center,
                         BaseBrickPhysicsConnectionHelper* detach) = 0;      // slot 17

    BaseBrickPhysicsConnectionHelper*             m_hinge;        // +0x18
    bool                                          m_first;        // +0x1c
    BasicArray<ActiveRigidElement>                m_elements;     // +0x20 the bodies that move
    BasicArray<BaseBrickPhysicsConnectionHelper*> m_connections;  // +0x30 the hinges of the group
};

class BaseBrickPhysicsManualHingeSolver : public BaseBrickPhysicsBaseHingeSolver {
public:
    // Win 0x520ca0 (Mac inlined into BaseBrickPhysicsScene::CreateHingeManualSolver 0x258590)
    BaseBrickPhysicsManualHingeSolver(BaseBrickPhysicsScene* scene, BaseBrickPhysicsConnectionHelper* hinge, bool first,
                                      const BasicArray<ActiveRigidElement>& elements,
                                      const BasicArray<BaseBrickPhysicsConnectionHelper*>& connections)
        : BaseBrickPhysicsBaseHingeSolver(scene, hinge, first, elements, connections)
    {
    }
    virtual ~BaseBrickPhysicsManualHingeSolver() {}        // Mac D1 0x259680 / D0 0x259690, Win 0x520d20 / sdd 0x520d80
    // Mac 0x254460, Win 0x5c5810
    virtual bool Solve(Real32 angle, const Vector3& axis, const Vector3& center,
                       BaseBrickPhysicsConnectionHelper* detach);
};

// The base of VortexBrickPhysicsSimulationHingeSolver (Mac TU 424); no code of its own.
class BaseBrickPhysicsSimulationHingeSolver : public BaseBrickPhysicsBaseHingeSolver {
public:
    BaseBrickPhysicsSimulationHingeSolver(BaseBrickPhysicsScene* scene, BaseBrickPhysicsConnectionHelper* hinge,
                                          bool first, const BasicArray<ActiveRigidElement>& elements,
                                          const BasicArray<BaseBrickPhysicsConnectionHelper*>& connections)
        : BaseBrickPhysicsBaseHingeSolver(scene, hinge, first, elements, connections)
    {
    }
    virtual ~BaseBrickPhysicsSimulationHingeSolver() {}
};

// ---- snapshot --------------------------------------------------------------------------------

// Registered with its scene, which clears it at every BeginUpdate. VortexPhysicsSnapshot (Mac
// TU 435) holds the transforms.
class BaseBrickPhysicsSnapshot : public PhysicsSnapshot {
public:
    // Mac inlined into VortexPhysicsScene::CreateSnapshot 0x3ad500 (refcount 1)
    explicit BaseBrickPhysicsSnapshot(BaseBrickPhysicsScene* scene);
    // Leaves the scene's list. Mac D1 0x2599d0 / D0 0x259a10, Win 0x520ff0 / sdd 0x521020
    virtual ~BaseBrickPhysicsSnapshot();

    // Answers PhysicsSnapshot::GetIID(). Mac 0x259290, Win 0x5c71b0
    virtual Int32  LEGO_COM_CALL QueryInterface(const InterfaceID& id, void** object);
    virtual UInt32 LEGO_COM_CALL AddRef();                  // Mac 0x259310, Win 0x5c7750
    virtual UInt32 LEGO_COM_CALL Release();                 // Mac 0x259330, Win 0x5c71f0

    volatile Int32         m_refCount;                      // +0x04 1
    BaseBrickPhysicsScene* m_scene;                         // +0x08

private:
    BaseBrickPhysicsSnapshot(const BaseBrickPhysicsSnapshot&);
    BaseBrickPhysicsSnapshot& operator=(const BaseBrickPhysicsSnapshot&);
};

// ---- the scene -------------------------------------------------------------------------------

class BaseBrickPhysicsScene : public PhysicsScene {
public:
    // Refcount 1, no solver. Win 0x520db0 (Mac inlined into VortexPhysicsScene 0x3aac30)
    explicit BaseBrickPhysicsScene(BaseBrickPhysics* physics);

    // ---- IUnknown (answers PhysicsScene::GetIID()). Mac 0x255730 / 0x2557b0 / 0x2557d0,
    // Win 0x5c5d00 / 0x5c5d40 / 0x5c5d60
    virtual Int32  LEGO_COM_CALL QueryInterface(const InterfaceID& id, void** object);
    virtual UInt32 LEGO_COM_CALL AddRef();
    virtual UInt32 LEGO_COM_CALL Release();

    // ---- PhysicsScene slots implemented here
    // BeginUpdate: the solver's OnBeginUpdate (outermost bracket only), clears the snapshots,
    // LockDirty. Mac 0x256120 (returns 1), Win 0x5c6070
    virtual void Unknown6();
    // EndUpdate: UnlockDirty(true). Mac 0x256180 (returns 1), Win 0x5c60c0
    virtual void Unknown7();
    virtual void AddRigidElement(PhysicsRigidElement* body, const Matrix3D& transform); // Mac 0x255810, Win 0x5c5d90
    virtual void RemoveRigidElement(PhysicsRigidElement* body);           // Mac 0x255a70, Win 0x5c5e00
    // RestoreSnapshotImpl. Mac 0x255ad0, Win 0x40c160
    virtual void RestoreSnapshot(PhysicsSnapshot* snapshot);
    virtual AnimatePhysicsSolver* CreateAnimatingSolver(PhysicsRigidElement* const* bodies, UInt32 count,
                                                        const Vector3* gravity); // Mac 0x255af0, Win 0x5c5e80
    virtual UInt32 Step(Real32 time);                       // Mac 0x255f50, Win 0x5c6010
    virtual bool   StopSolver(bool result);                 // Mac 0x255ff0, Win 0x5c6030
    virtual PhysicsSolver* GetSolver();                     // Mac 0x256100, Win 0x5c6060

    // ---- slots 33-44 (this class's own; the engine implements the pure ones)
    virtual void SetGravity(const Vector3& gravity) = 0;                            // slot 33
    // The body takes part in the simulation (AnimatingSolver Start / OnEndUpdate).
    virtual void EnableSimulation(BaseBrickPhysicsRigidElement* body) = 0;         // slot 34
    // The same for a body moved by a hinge solver (Vortex: same body as slot 34).
    virtual void EnableManualMotion(BaseBrickPhysicsRigidElement* body) = 0;       // slot 35
    // The body stops and leaves the simulation.
    virtual void DisableSimulation(BaseBrickPhysicsRigidElement* body) = 0;        // slot 36
    virtual void Unknown37(BaseBrickPhysicsRigidElement* body) = 0;                // slot 37
    virtual void Unknown38(void* argument) = 0;                                    // slot 38 (Vortex: empty)
    virtual void Unknown39(void* argument) = 0;                                    // slot 39 (Vortex: empty)
    // True when one of the bodies (with children) collides.
    virtual bool TestElementCollisions(const BasicArray<ActiveRigidElement>& elements) = 0; // slot 40
    // Puts the snapshot's bodies back (RestoreSnapshot).
    virtual void RestoreSnapshotImpl(PhysicsSnapshot* snapshot) = 0;              // slot 41
    // The engine step of the AnimatingSolver; counts the steps here. Mac 0x259970, Win 0x520da0
    virtual void StepPhysics(Real32 time);                                         // slot 42
    // The solver of one hinge ("CreateHingeManualSolver", log string of the Vortex override
    // Mac 0x3af5b0). Mac 0x258590, Win 0x5c6e80
    virtual BaseBrickPhysicsBaseHingeSolver* CreateHingeManualSolver(
        BaseBrickPhysicsConnectionHelper* hinge, bool first, const BasicArray<ActiveRigidElement>& elements,
        const BasicArray<BaseBrickPhysicsConnectionHelper*>& connections);     // slot 43
    // The solver of a hinge group with loops ("CreateHingeSimulateSolver", Mac 0x3af680).
    virtual BaseBrickPhysicsBaseHingeSolver* CreateHingeSimulateSolver(
        BaseBrickPhysicsConnectionHelper* hinge, bool first, const BasicArray<ActiveRigidElement>& elements,
        const BasicArray<BaseBrickPhysicsConnectionHelper*>& connections) = 0; // slot 44
    // Mac D1 0x259980 (body D2 0x25a0a0) / D0 0x259990, Win 0x520eb0 / sdd 0x520fd0
    virtual ~BaseBrickPhysicsScene();

    // ---- bookkeeping (non-virtual)
    void AddConnection(BaseBrickPhysicsConnectionHelper* connection);      // Mac 0x256670 (Win inlined)
    void RemoveConnection(BaseBrickPhysicsConnectionHelper* connection);   // Mac 0x2568a0, Win 0x5c6150
    void MarkRigidElementDirty(BaseBrickPhysicsRigidElement* body);        // Mac 0x256210 (Win 0x49c250 PushBack)
    void MarkConnectionDirty(BaseBrickPhysicsConnectionHelper* connection);// Mac 0x256440 (Win 0x49c250 PushBack)
    // The dirty lists are processed when the count drops back to 0. Mac 0x2546a0
    bool LockDirty();
    // Updates the dirty bodies and joints (Update / UpdateConstraint) at the outermost unlock;
    // `notifySolver`: the running solver's OnEndUpdate first. Mac 0x2546b0, Win 0x5c60d0
    bool UnlockDirty(bool notifySolver);
    // Makes `solver` the running one (referenced), resets the step count, starts it.
    // Mac 0x255ee0, Win 0x5c6900
    void SetSolver(BaseBrickPhysicsSimulationSolver* solver);
    // Stops and drops the running solver (there must be one). Mac 0x256030, Win 0x5c69e0
    void StopRunningSolver(bool& result);

    // ---- the hinge solvers' helpers (also used by the Vortex solvers)
    // The running hinge solver for `hinge`, else a new one (StopSolver(false) first): the
    // manual solver for a single hinge, the simulating one when the group has more hinges.
    // Mac 0x257e30, Win 0x5c6aa0
    BaseBrickPhysicsBaseHingeSolver* GetHingeSolver(BaseBrickPhysicsConnectionHelper* hinge, bool first);
    // Always the simulating solver. Mac 0x2582c0, Win 0x5c6ca0
    BaseBrickPhysicsBaseHingeSolver* GetHingeSimulateSolver(BaseBrickPhysicsConnectionHelper* hinge, bool first);
    // A snapshot of the bodies and their children. Mac 0x254740, Win 0x5c64a0
    PhysicsSnapshot* CreateElementSnapshot(const BasicArray<ActiveRigidElement>& elements);
    // The children follow their body: SetEngineTransform(body now * body then^-1 * child then).
    // Mac 0x254c70, Win 0x5c6670
    void MoveChildren(const BasicArray<ActiveRigidElement>& elements);
    void MoveChildren(const ActiveRigidElement& element);                  // Mac 0x257cc0
    // The cached transforms from the engine (GetEngineTransform). Mac 0x257c30, Win 0x5c6590
    void UpdateTransforms(BasicArray<ActiveRigidElement> elements);
    // No body moving (IsMoving). Mac 0x257d60 / 0x257db0 (with children), Win 0x5c6720 / 0x5c6800
    bool IsAtRest(BasicArray<ActiveRigidElement> elements);
    bool IsAtRestWithChildren(BasicArray<ActiveRigidElement> elements);
    // Clears the joints' CheckSeparation counters. Mac 0x257b80
    void ResetSeparationCounts(const BasicArray<BaseBrickPhysicsConnectionHelper*>& connections);

    // The hinge group of `hinge` seen from `body`: every hinge of the loops through `hinge`
    // (`connections`, `hinge` first) and the bodies they join with the bodies attached to those
    // (`elements`). Mac 0x2568f0, Win 0x5c61a0
    void CollectHingeGroup(BaseBrickPhysicsConnectionHelper* hinge, BaseBrickPhysicsRigidElement* body,
                           BasicArray<BaseBrickPhysicsConnectionHelper*>& connections,
                           BasicArray<ActiveRigidElement>& elements);
    // Depth-first walk over the joints from `body` (entered through `connection`), recording
    // every closed loop in m_loops. Mac 0x2571a0, Win 0x5c6f00
    void FindLoops(BaseBrickPhysicsConnectionHelper* connection, BaseBrickPhysicsRigidElement* body);
    // Removes from every loop the joints of each other loop it contains. Mac 0x257740, Win 0x5c7120
    void RemoveContainedLoops();
    // The bodies reachable from `body` over unmarked bodies, as children of `element`.
    // Mac 0x257ae0, Win 0x5c7090
    void CollectChildren(ActiveRigidElement& element, BaseBrickPhysicsRigidElement* body);

    // ---- data -------------------------------------------------------------------------------
    BasicArray<BaseBrickPhysicsConnectionHelper*>               m_stack;           // +0x04 FindLoops path
    BasicArray<BasicArray<BaseBrickPhysicsConnectionHelper*> >  m_loops;           // +0x14
    volatile Int32                                              m_refCount;        // +0x24 1
    BaseBrickPhysics*                                           m_physics;         // +0x28
    BasicArray<BaseBrickPhysicsRigidElement*>                   m_rigidElements;   // +0x2c
    BasicArray<BaseBrickPhysicsConnectionHelper*>               m_connections;     // +0x3c
    BaseBrickPhysicsSimulationSolver*                           m_solver;          // +0x4c referenced (Object)
    UInt32                                                      m_stepCount;       // +0x50
    bool                                                        m_unknown54;       // +0x54 false (Vortex)
    Int32                                                       m_updateDepth;     // +0x58
    Int32                                                       m_dirtyLock;       // +0x5c
    BasicArray<BaseBrickPhysicsRigidElement*>                   m_dirtyRigidElements; // +0x60
    BasicArray<BaseBrickPhysicsConnectionHelper*>               m_dirtyConnections;   // +0x70
    BasicArray<BaseBrickPhysicsSnapshot*>                       m_snapshots;       // +0x80

private:
    BaseBrickPhysicsScene(const BaseBrickPhysicsScene&);
    BaseBrickPhysicsScene& operator=(const BaseBrickPhysicsScene&);
};

} // namespace LEGO
