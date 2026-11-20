// LEGO Digital Designer — restored source
// LEGO::BI — the building instructions (building guide) of the model shown by the
//   BrickConnectSceneControl: a Notifier<BuildingGuideObserver> that owns a BIStorage (+0x24,
//   the steps). BrickConnectSceneControl keeps two (Mac +0x464 / +0x468: the current one and
//   the one loaded with a model) and resets them when the scene changes.
//   BI.cpp also holds the helpers that the BI classes share (static members here): the
//   connection directions of a BIBrick, event/brick set conversions, the step split.
//
// Home TU 148 "BI.cpp" (Mac 0x20af60-0x20f5a9). Mac layout: Notifier<BuildingGuideObserver>
// +0x00..+0x24 (vptr, observers +0x04, iterators +0x14), RefPointer<BIStorage> +0x24, virtual
// base Object +0x28 (sizeof 0x30). Mac vtables 0x76f93c {D1 0x20d9d0, D0 0x20da30,
// OnFinalRelease 0x20d5f0} / Object-in-BI 0x76f960; Win vtable 0xa2b930 {sdd 0x5533d0}.
// BIStorage: BIStorage.h (TU 358).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "Notifier.h"
#include "StorageScene2.h"
#include "BIStorage.h"
#include "BIBrick.h"

namespace LEGO {

class BI;
class BIEvent;
class Brick;
class Scene;
class PrimitiveElement;
class PrimitiveConnection;

// Observer of a BI (home TU 283 GenerateBuildingGuideCommand.cpp).
// Mac vtable 0x74059c {D1 0x2966b0, D0 0x2966e0, OnFinalRelease 0x2962f0, OnAttached,
// OnDetached, pure}. (name of the slot inferred)
class BuildingGuideObserver : public Observer<BI, BuildingGuideObserver> {
public:
    virtual ~BuildingGuideObserver() {}
    // The steps of the guide were replaced (`hasSteps`: the new storage has steps) or cleared.
    virtual void OnBuildingGuideChanged(bool hasSteps) = 0;
};

class BI : public Notifier<BuildingGuideObserver> {
public:
    // A step for the guide display: its events and the camera of the step (GetSteps).
    // sizeof 0x14 (Mac): copy constructor 0x20ea70, destructor 0x20dc60 (both implicit).
    // (name inferred)
    struct Step {
        BasicArray<RefPointer<BIEvent> >  m_events;   // +0x00
        RefPointer<StorageScene2::Camera> m_camera;   // +0x10
    };

    // A new BIStorage. Mac 0x20af90 (C1; vtables 0x76f93c / 0x76f960)
    BI();
    virtual ~BI();                                                         // Mac 0x20d9d0/0x20da30, Win sdd 0x5533d0

    // The model changed: the steps are cleared, observers notified (false). Mac 0x20b0d0
    void Reset();
    // Takes over `storage` (the steps of a loaded or generated model); observers notified
    // with storage->HasSteps(). Mac 0x20b380
    void SetStorage(const RefPointer<BIStorage>& storage);
    RefPointer<BIStorage> GetStorage() const;                              // Mac 0x20b620
    void SetName(const String& name);                                      // Mac 0x20b650
    const String& GetName() const;                                         // Mac 0x20b670
    // The storage has steps; LDDApplication::SaveModel asks before
    // saving. Mac 0x20b350, Win 0x6137c1 (name inferred)
    bool HasSteps() const;

    // ---- shared helpers (BI.cpp; static members by inference, names inferred) -----------------

    // The connections of `brick`'s elements to elements outside the brick, by the primitive
    // of the other element (with a non-empty `filter`: only primitives in it): the directions
    // the connection fields allow (per field type), with the connection strength.
    // Mac 0x20c1c0
    static void ComputeConnections(BIBrick* brick, BIBrick::ConnectionMap& connections,
                                   const BasicSet<BIBrickPrimitive*>& filter);
    // 100 x (planar connection pairs + 10 per hinge / axle / ball, 20 per fixed, 1 per rail /
    // slider field connection) / the smaller bone box diagonal of the two elements.
    // Mac 0x20d3b0
    static Real32 GetConnectionStrength(const PrimitiveConnection* connection);
    // The brick is a wire assembly (ConnectivityWorldLookupTables::GetWires). Mac 0x20c190
    static bool IsWire(const Brick* brick);
    // Fills the static element -> connected elements map (Mac 0x77aeac) from the scene's
    // primitive elements. Mac 0x20b690
    static void UpdateElementGraph(Scene* scene);
    static const BasicMap<PrimitiveElement*, BasicSet<PrimitiveElement*> >& GetElementGraph();
    // Primitive set -> brick set. Mac 0x20b8b0
    static void GetBricks(const BasicSet<BIBrickPrimitive*>& primitives, BasicSet<BIBrick*>& bricks);
    // The primitives of the bricks. Mac 0x20b900
    static void GetPrimitives(const BasicSet<BIBrick*>& bricks, BasicSet<BIBrickPrimitive*>& primitives);
    // The bricks of the brick events. Mac 0x20ba70 / 0x20bce0
    static void GetBricks(const BasicArray<RefPointer<BIEvent> >& events, BasicArray<BIBrick*>& bricks);
    static void GetBricks(const BasicArray<RefPointer<BIEvent> >& events, BasicSet<BIBrick*>& bricks);
    // The primitives of the brick events' bricks. Mac 0x20bd30
    static void GetPrimitives(const BasicArray<RefPointer<BIEvent> >& events, BasicSet<BIBrickPrimitive*>& primitives);
    // Splits the events into steps: a step event sets the camera of the next step and starts
    // one, a substep brick starts one too; the other events go into the current step.
    // Mac 0x20bd80
    static void GetSteps(const BasicArray<RefPointer<BIEvent> >& events, BasicArray<Step>& steps);
    // Debug trace, empty in the release builds. Mac 0x20d4f0
    static void Trace(const String& message, UInt32 level);

    RefPointer<BIStorage> m_storage;   // +0x24
};

} // namespace LEGO
