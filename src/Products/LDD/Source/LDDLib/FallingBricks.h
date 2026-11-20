// LEGO Digital Designer — restored source
// LEGO::FallingBricks — the view mode's "deconstruction" animation: the bricks (one step per
//   ConceptBrick) fly `distance` away from the model centre (or along given directions),
//   optionally spinning, and back (reverse). It follows the main updater and draws its own
//   meshes into the scene control's queue (observer slot 5), optionally with the scene's render
//   data off (BrickConnectSceneControl +0x3bc, SetShowStatistics); at the end the observers hear
//   OnFinished.
//   NOTE: a pure key-frame animation (GetMilliseconds); no physics.
// LEGO::FallingBricksObserver — Observer<FallingBricks, FallingBricksObserver> (header-inline;
//   its vtable and destructors are emitted in TU 234 DeconstructionCommand.cpp).
//
// TU 111 "FallingBricks.cpp" (Mac 0x1ba5f0-0x1bf60b; Win 0x65a453-0x65b193 + 0x68c8ef-0x68d77b).
// The TU also holds the template instances of the class: NotifierT<FallingBricks, …> (Mac
// 0x1bd900-0x1bdd50, Win sdd 0x65a611 / 0x65a7b4), BasicArray<FallingBricks::Primitive>,
// BasicArray<FallingBricks::Brick> (Win sdd 0x65ac41 / 0x65ad4b), BasicArray<RefPointer<Mesh> >,
// BasicArray<RefPointer<ConceptBrick> > (Win sdd 0x65a64e), BasicMap<PrimitiveElement*,
// Vector3> (operator[] Mac 0x1bf2f0 = Win 0x68c8ef, node insert 0x1bf4e0, erase 0x1beb50 = Win
// 0x65acc4, Win sdd 0x65aceb), NotifierT::RemoveObserver (Mac 0x1bece0 / 0x1beed0).
//
// RTTI (Mac offsets): UpdateObserver +0x00 (subject +0x04), BrickConnectSceneControlObserver
//   +0x08 (subject +0x0c), Notifier<FallingBricksObserver> +0x10 (observers +0x14, iterations
//   +0x24), members +0x34..+0xc4 below, virtual base Object +0xc4 (sizeof 0xcc; DeconstructionCommand
//   operator new 0xcc). Win: Object +0xe4 (operator new 0xf0), members = Mac + 8 (BCSC +0x44).
// Vtables: Mac 0x71f2dc {D1 0x1bddf0, D0 0x1bde50, OnFinalRelease 0x1bdec0, OnAttached,
//   OnDetached, Update 0x1bd100, (primary entry of OnEvent05) 0x1bcfe0}, BCSC-observer-in 0x71f304
//   {5: thunk 0x1bd0d0}, Notifier-in 0x71f36c, Object-in 0x71f390. Win 0x9d6d10 {.., ..,
//   Update 0x68cf50}, BCSC-observer-in 0x9d6cbc {2: 0x68cd06}, Object-in 0x9d6ca8 {sdd 0x65ae97 ->
//   0x65b193, OnFinalRelease 0x65aec4 -> 0x65ae9f}.
// Class name and the nested Primitive / Brick from RTTI; method and member names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "Geometry.h"
#include "Notifier.h"
#include "Updater.h"
#include "Mesh.h"
#include "Render.h"
#include "RenderQueue.h"
#include "PrimitiveElement.h"
#include "BrickConnectSceneControlObserver.h"
#include "ConceptBrick.h"

namespace LEGO {

class FallingBricks;
class BrickConnectSceneControl;

// Mac vtable 0x73824c {D1 0x26dfb0, D0 0x26dfe0, OnFinalRelease 0x26dee0, OnAttached 0x26e030,
// OnDetached 0x26e040, 5: 0x26e050, 6: 0x26e060 (both empty)}, Object-in 0x738280; Win
// 0x9d6ba0 {OnAttached, OnDetached (0x65a12a), 0x5c8021 (ret), 0x6943f1 (ret 8)}, Object-in
// 0x9d6b8c {sdd 0x65a1dd -> 0x65a2dd}. Observer<FallingBricks, …>: Mac 0x26dee0 / 0x26e170 /
// 0x26e1a0 (Win 0x65a0a0 constructor, sdd 0x65a238).
class FallingBricksObserver : public Observer<FallingBricks, FallingBricksObserver> {
public:
    FallingBricksObserver() {}                                               // Win 0x65a16f
    virtual ~FallingBricksObserver() {}                                      // Mac 0x26dfb0/0x26dfe0, Win sdd 0x65a2dd

    // Never sent by FallingBricks. (name inferred)
    virtual void OnFallingBricksEvent5() {}                                  // 5/2 Mac 0x26e050
    // The animation ended (or was stopped): its concept bricks and direction. (name inferred)
    virtual void OnFinished(const BasicArray<RefPointer<ConceptBrick> >& /*conceptBricks*/,
                            bool /*reverse*/) {}                             // 6/3 Mac 0x26e060
};

class FallingBricks : public UpdateObserver,
                      public BrickConnectSceneControlObserver,
                      public Notifier<FallingBricksObserver> {
public:
    // The meshes of one primitive element (a rigid part, or a whole flexible part with its
    // first element). Mac sizeof 0x14 (array copy Mac 0x1bd1b0 / 0x1bd630, Win 0x68c96c).
    struct Primitive {
        Primitive() : m_element(0) {}
        BasicArray<RefPointer<Mesh> > m_meshes;    // +0x00
        PrimitiveElement*             m_element;   // +0x10 (not referenced): its transform
    };
    // The primitives of one concept brick (one animation step). Mac sizeof 0x10 (Win push 0x68ce17).
    struct Brick {
        BasicArray<Primitive> m_primitives;        // +0x00
    };
    // Progress curve of a step (+0x58). Other values: NOTE see DrawBrick.
    enum Easing { EasingLinear = 0, EasingQuadratic = 1, EasingSquareRoot = 2 };

    typedef BasicMap<PrimitiveElement*, Vector3> PositionMap;

    // Inline in DeconstructionCommand's constructor on Mac (0x26cb60), Win 0x65ad6d: `render`
    // referenced, nothing running.
    FallingBricks(BrickConnectSceneControl* sceneControl, Render* render);
    virtual ~FallingBricks();                                                // Mac 0x1bddf0/0x1bde50 (base 0x1be450), Win 0x65aecc / sdd 0x65b193

    // Starts the animation of `conceptBricks` (copied); a running one is stopped first. All steps
    // take `totalTime` milliseconds together: with `stepRatio` > 1 the steps start one after
    // the other, `totalTime / stepRatio / count` apart, otherwise all at once. Mac 0x1ba5f0,
    // Win 0x68d6dc
    void Start(const BasicArray<RefPointer<ConceptBrick> >& conceptBricks, bool reverse,
               UInt32 totalTime, Real32 stepRatio, Real32 distance, bool rotate, UInt32 easing,
               bool transparent, bool hideScene, const PositionMap* directions);
    // Ends the animation: the scene's render data is shown again after a reverse run, the scene
    // control and the updater are left, the meshes and positions dropped, and the observers
    // hear OnFinished. Nothing when not running. Mac 0x1bbad0, Win 0x68ce9f
    void Stop();

    // ---- accessors (names inferred) ----
    bool   IsRunning() const { return m_running; }
    bool   IsReverse() const { return m_reverse; }
    UInt32 GetElapsed() const { return m_elapsed; }
    UInt32 GetStepDelay() const { return m_stepDelay; }
    UInt32 GetStepDuration() const { return m_duration; }
    Int64  GetStartTime() const { return m_startTime; }
    Int64  GetEndTime() const { return m_endTime; }
    const BasicArray<RefPointer<ConceptBrick> >& GetConceptBricks() const { return m_conceptBricks; }
    const BasicArray<Brick>& GetBricks() const { return m_bricks; }
    const PositionMap& GetFromPositions() const { return m_fromPositions; }
    const PositionMap& GetToPositions() const { return m_toPositions; }

    // ---- UpdateObserver ----
    // Redraw; the first call starts the clock (end = now + duration + (count - 1) * delay) and
    // hides the scene's render data (`hideScene`); after the end Stop, else the elapsed time.
    virtual void Update();                                                   // 5/2 Mac 0x1bd100, Win 0x68cf50

    // ---- BrickConnectSceneControlObserver: draw the steps (forward: last step first) ----
    virtual void OnEvent05(const Matrix3D& transform, Render* render, RenderQueue* queue,
                           RenderQueue* overlay);                            // 5/2 Mac 0x1bcfe0 (thunk 0x1bd0d0), Win 0x68cd06

protected:
    // Observers, then the scene control and the updater are left. Mac 0x1bdec0, Win 0x65ae9f
    virtual void OnFinalRelease();
    using UpdateObserver::OnAttached;
    using UpdateObserver::OnDetached;

private:
    FallingBricks(const FallingBricks&);
    FallingBricks& operator=(const FallingBricks&);

    // Start with the step timing resolved. Mac 0x1ba6e0, Win 0x68d2da
    void StartSteps(const BasicArray<RefPointer<ConceptBrick> >& conceptBricks, bool reverse,
                    UInt32 stepDuration, UInt32 stepDelay, Real32 distance, bool rotate,
                    UInt32 easing, bool transparent, bool hideScene, const PositionMap* directions);
    // One Brick per concept brick: per single-element part the meshes of its primitive
    // (BrickDraw::BuildMeshes), per flexible part (once) the skinned meshes with the bone
    // transforms relative to its first element (BrickDraw::BuildFlexMeshes); empty Bricks are
    // dropped. Redraw. Mac 0x1bbe70, Win 0x68cfd3
    void BuildMeshes();
    // Queues the meshes of `brick` (step number `step`): before its start at the "from"
    // position (forward) / not at all (reverse), during the step interpolated (and spun), after
    // it not at all (forward) / at the "to" position (reverse). `render` is not used.
    // Mac 0x1bc860, Win 0x68ca04
    void DrawBrick(const Brick& brick, Matrix3D transform, Render* render, RenderQueue* queue,
                   UInt32 step);

    // ---- data (Mac offsets; Win + 8) ----
    RefPointer<Object>                     m_unused34;       // +0x34 never set (both builds)
    RefPointer<Render>                     m_render;         // +0x38 (not used otherwise)
    BrickConnectSceneControl*              m_sceneControl;   // +0x3c observed while running
    bool                                   m_hideScene;      // +0x40
    BasicArray<RefPointer<ConceptBrick> >  m_conceptBricks;  // +0x44
    bool                                   m_running;        // +0x54
    bool                                   m_reverse;        // +0x55
    bool                                   m_rotate;         // +0x56
    UInt32                                 m_easing;         // +0x58
    bool                                   m_transparent;    // +0x5c meshes at opacity 0.5 (queue slot 1)
    Int64                                  m_startTime;      // +0x60 0 until the first Update
    Int64                                  m_endTime;        // +0x68
    UInt32                                 m_elapsed;        // +0x70 ms since m_startTime
    UInt32                                 m_stepDelay;      // +0x74 ms between two step starts
    UInt32                                 m_duration;       // +0x78 ms of one step
    // Forward: "to" = exploded position, "from" = the element's position; reverse: the
    // other way round. The animation goes from m_fromPositions to m_toPositions.
    PositionMap                            m_toPositions;    // +0x7c (map +0x80)
    PositionMap                            m_fromPositions;  // +0x98 (map +0x9c)
    BasicArray<Brick>                      m_bricks;         // +0xb4
                                                             // Object +0xc4
};

} // namespace LEGO
