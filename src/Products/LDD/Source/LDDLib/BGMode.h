// LEGO Digital Designer — restored source
// LEGO::BGMode — base of the building-guide modes (BGAutomaticMode): a BrickMode that walks
//   the guide of its BGApplication part by part / step by step and animates the parts that
//   come or go (each part flies in along its insert direction, or out, in 500 ms; the parts of
//   a step start 66 ms apart). While the animation runs the mode draws the guide's bricks
//   itself into the scene control's queue (OnEvent05), from meshes it builds per brick.
//
// TU 366 "BGMode.cpp": Mac 0x34ce50-0x352ecb, Win 0x65b51c-0x65e6cd (with gaps; template code
// of BasicArray<Matrix3D> / BasicArray<RefPointer<Mesh> > / BasicArray<PrimitiveGeometry> and
// the map / deque helpers in the same range). Mac 0x352e50 = <iostream> (not code).
//
// RTTI (Mac): vmi {BrickMode +0, UpdateObserver +0x280, BrickConnectSceneControlObserver
//   +0x288}, virtual base Object (+0x2ec, sizeof 0x2f4; BGMode itself is abstract).
// Mac vtable 0x74d84c (66 entries): BrickMode's 50 (OnFinalRelease 2 = 0x3509d0 overridden),
//   then the new slots 50..65 (see the declarations); 61 / 62 are the Mac-only primary entries
//   of the UpdateObserver::Update and BrickConnectSceneControlObserver::OnEvent05 overriders.
//   Win vtable 0x9d6f14 (58 entries: BrickMode's 44, then 44..57 = Mac 50..60, 63..65),
//   UpdateObserver-in 0x9d6e60 {Update 0x65d528}, BCSC-observer-in 0x9d6e0c {OnEvent05
//   0x65e053}.
// Win layout: ControlObserver +0x268, UpdateObserver +0x2a0, BCSC observer +0x2ac, Object +0x318.
//
// Mac layout (constructor 0x34ce50, C2 with VTT):
//   +0x000 BrickMode … +0x280 UpdateObserver (subject +0x284), +0x288 BCSC observer (subject
//   +0x28c), +0x290 m_brickGeometry, +0x2ac m_activeParts, +0x2d8 m_playing, +0x2d9 m_animating,
//   +0x2dc m_hiddenBricks, +0x2ec Object.
// Names: the class / nested type names are RTTI (BGMode::ActivePart, ::BrickGeometry,
//   ::PrimitiveGeometry); everything else is inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "BasicDeque.h"
#include "ObjectRefPointer.h"
#include "Geometry.h"
#include "Updater.h"
#include "Mesh.h"
#include "StorageScene2.h"
#include "BrickMode.h"
#include "BrickConnectSceneControlObserver.h"

namespace LEGO {

class BGApplication;
class BIBrick;
class BIEvent;

class PrimitiveElement;
class Render;
class RenderQueue;
class BricksControl;
class PrimitiveElementBrick;

class BGMode : public BrickMode, public UpdateObserver, public BrickConnectSceneControlObserver {
public:
    // A part being animated (element of the deque m_activeParts). sizeof 0x2c (Mac; deque
    // buffers of 11). (RTTI name; members inferred)
    struct ActivePart {
        RefPointer<BIEvent>                 m_event;       // +0x00 its brick event
        UInt32                              m_mode;        // +0x04 1 = comes (insert), 2 = goes
        UInt64                              m_startTime;   // +0x08 GetMilliseconds
        BasicMap<PrimitiveElement*, Matrix3D> m_transforms; // +0x10 the current transform per element
    };
    // The meshes of one primitive element (a flexible part: of all of its elements, keyed by
    // the first). sizeof 0x14. (RTTI name; members inferred)
    struct PrimitiveGeometry {
        PrimitiveGeometry() : m_element(0) {}
        BasicArray<RefPointer<Mesh> > m_meshes;    // +0x00
        PrimitiveElement*             m_element;   // +0x10
    };
    // The geometry of one BIBrick (its value in m_brickGeometry). (RTTI name; members inferred)
    struct BrickGeometry {
        BasicArray<PrimitiveGeometry> m_primitives;   // +0x00 (vtable 0x7780e8)
    };

    // Mac 0x34ce50 (C2), Win (inlined in BGAutomaticMode's 0x625200)
    BGMode(ControlManager* manager, Window* window, BrickConnectSceneControl* sceneControl);
    virtual ~BGMode();                                                      // Mac 0x350270/0x3505e0, Win sdd 0x65e6cd

    // ---- navigation (Mac idx / Win slot) ----
    // One part forward: the application's NextPart (the camera of a passed step is taken),
    // the part flies in, the view updates. False at the end. (BGAutomaticMode records the
    // action.)
    virtual bool NextPart(bool substeps);                                   // 50/44 Mac 0x34d2d0, Win 0x65d45f
    // One part back: it flies out.
    virtual bool PrevPart(bool substeps);                                   // 51/45 Mac 0x34d5c0, Win 0x65d4c4
    // One step forward: its bricks (also appended to `bricks`) fly in one after the other;
    // without `animate` the animation is dropped and the scene control shows the placed
    // bricks at once. `setCamera`: the step's camera. The first argument is not used.
    virtual bool NextStep(bool unused, bool animate, BasicArray<BIBrick*>& bricks, bool setCamera); // 52/46 Mac 0x34d660, Win 0x65d8ae
    // One step back (no camera).
    virtual bool PrevStep(bool unused, bool animate);                       // 53/47 Mac 0x34deb0, Win 0x65db15
    // Auto-play on / off (Update steps on while it is on); UpdateCommands. True.
    // NOTE: Play takes one argument it ignores (Win 0x65b51c returns with `ret 4`; its meaning
    // and type are unknown — no caller in the restored TUs). (names inferred)
    virtual bool Play(bool unused);                                         // 54/48 Mac 0x34e420, Win 0x65b51c
    virtual bool Pause();                                                   // 55/49 Mac 0x34e450, Win 0x65b530
    virtual bool IsPlaying() const;                                         // 56/50 Mac 0x34e480, Win 0x65b542
    // To the first / last part (the application's Rewind / FastForward), then UpdateView.
    virtual void Rewind();                                                  // 57/51 Mac 0x34e490, Win 0x65b549
    virtual void FastForward();                                             // 58/52 Mac 0x34e4e0, Win 0x65b577
    // NOTE: computes the corner direction and the primitives of the generator's events and
    // drops both (an empty debug feature in the release builds).
    virtual void ShowDirection(UInt32 corner);                              // 59/53 Mac 0x350ee0, Win 0x65c859
    virtual BGApplication* GetApplication() = 0;                            // 60/54

    // ---- UpdateObserver: the animation clock ----
    // While playing and nothing moves: NextStep (and Pause at the end). Then the parts whose
    // 500 ms are over leave the animation, the others move, the scene control redraws.
    virtual void Update();                                                  // 61/– Mac 0x350b60 (thunk 0x350ec0), Win 0x65d528
    // ---- BrickConnectSceneControlObserver: draws the guide's bricks ----
    // Every brick placed in the application's scene plus the animated ones (and the primitives
    // of an animated substep), unless a brick intersects m_hiddenBricks: its meshes (built on
    // first use) at the element transform, or the animated one, times `transform`, into `queue`.
    virtual void OnEvent05(const Matrix3D& transform, Render* render, RenderQueue* overlayQueue,
                           RenderQueue* queue);                             // 62/– Mac 0x34e9e0 (thunk 0x34f7f0), Win 0x65e053

    // The scene control's camera looks along the stored camera (position, target at its
    // distance, the current up vector). Nothing for null.
    virtual void SetCamera(const StorageScene2::Camera* camera);            // 63/55 Mac 0x34fee0, Win 0x65b5a5
    // The scene control's camera into `camera` (inverse view matrix, target distance).
    virtual void GetCamera(StorageScene2::Camera* camera);                  // 64/56 Mac 0x34ffa0, Win 0x65b61c
    // Re-evaluates the enable state of the navigation commands.
    virtual void UpdateCommands() = 0;                                      // 65/57

    // ---- non-virtual ----
    // OnEvent05(identity, the scene manager's render, queue, queue) (BGScreenShotCommand,
    // HTMLBIGenerator). Mac 0x34e9a0, Win 0x65b9cf
    void QueueParts(RenderQueue* queue);
    // A preview control (BricksControl, material set 2, 100 x 100) of the primitives of a
    // substep (recursively), at the database camera of the first primitive's design; null for
    // a brick without primitives. Mac 0x34faf0
    BricksControl* CreateBrickPreview(BIBrick* brick);

protected:
    // Observers, then BrickMode's.
    virtual void OnFinalRelease();                                          // 2/– Mac 0x3509d0

    // The application's events (top level + substeps) into a new BIStorage; on success it
    // becomes the scene control's building instructions, else those are reset. Mac 0x34d1b0
    bool StoreBuildingGuide();
    // Drops the geometry, m_hiddenBricks, the animation; not playing. Mac 0x34d0e0, Win 0x65d844
    void Reset();
    // `event`'s brick starts to come (`insert`) or go `delay` ms from now; the animation is
    // brought to that time. Mac 0x34d370, Win 0x65d1a0
    void StartPart(BIEvent* event, bool insert, UInt64 delay);
    // Moves every animated element to its place at `time`: the translation goes linearly from
    // the start to the end transform (offset by the brick's insert direction) over 500 ms,
    // overshooting to 110 %. Mac 0x34e530
    void UpdateParts(UInt64 time);
    // The meshes of the bricks into m_brickGeometry (replacing theirs). Mac 0x34f0e0
    void BuildGeometry(const BasicArray<BIBrick*>& bricks);
    // The scene control's render data from the parts of the application's placed primitives.
    // (restoration helper; inlined twice: NextStep / PrevStep)
    void ShowPlacedParts();
    // The bricks (the substeps' primitives recursively) as PrimitiveElementBricks. Mac 0x34fd70
    static void CollectBricks(BIBrick* brick, BasicArray<ObjectRefPointer<PrimitiveElementBrick> >& bricks);

    // ---- data (Mac offsets) ----
    BasicMap<BIBrick*, BrickGeometry>     m_brickGeometry;   // +0x290 (map vtable 0x778108)
    BasicDeque<ActivePart>                m_activeParts;     // +0x2ac (vtable 0x778120)
    bool                                  m_playing;         // +0x2d8
    bool                                  m_animating;       // +0x2d9 set by StartPart, cleared when done
    // Bricks not drawn (no writer in the restored TUs). (name inferred)
    BasicArray<BIBrick*>                  m_hiddenBricks;    // +0x2dc
                                                             // Object +0x2ec
};

} // namespace LEGO
