// LEGO Digital Designer — restored source
// LEGO::ClickStickTool — the edit mode's building tool: carries bricks in a drag scene and snaps
//   them to the scene. Sources: palette / clip board / clone tool (PickUp), scene parts from the
//   selection tools (PickUpParts, moved or copied), an imported model (ModelLoaderHandler). The
//   drag scene follows the mouse ray (ClickStickCastUtility), arrow keys / WASD turn it, a click
//   drops it (merged with undo); with "REPEATPASTE" palette bricks are picked up again. An
//   UndoSetContent on the undo stack cancels the carrying (Ctrl+Z).
// LEGO::UndoSetContent — that undo action: the scene manager's active tool back to the default.
//
// TU 062 "ClickStickTool.cpp" (Mac 0x142b80-0x147b2b, Win 0x667dc3-0x66a238 + 0x62cff6-0x62d337).
// The constructor is inline: emitted in EditMode.cpp (Mac 0x18b720, Win 0x62d132). Not code in
// the TU: the thunks, the ClickStickCastSimpleUtility accessors (ClickStickCastUtility.h) and the
// BasicMap / BasicArray instances.
// RTTI: ClickStickTool : BrickConnectSceneControlTool +0, UpdateObserver +0x2c (private),
//   PreferencesObserver +0x34, ModelLoaderHandler +0x3c, ClickStickCastSimpleUtility +0x44
//   (private); virtual base Object +0x270 (Mac sizeof 0x278; Win 0x290).
//   Mac vtable 0x70c87c (39 entries: the BrickConnectSceneControlTool slots with OnFinalRelease
//   0x146d60, OnAttached 0x1449b0, OnDetached 0x145060, OnEvent05 0x145c30, OnEvent18 0x145d30,
//   OnEvent19 0x145d90, GetTool 0x146e00; new 27-38 below); ControlObserver-in 0x70c924,
//   UpdateObserver-in 0x70c9b8, PreferencesObserver-in 0x70c9dc, ModelLoaderHandler-in 0x70ca00,
//   ClickStickCastSimpleUtility-in 0x70ca20.
//   Win primary 0x9d212c (23: OnAttached 0x669153, OnDetached 0x668e16, OnEvent05 0x66817c,
//   OnEvent18 / 19 0x667de9, GetTool 0x62d306, Cancel 0x668d2c); ControlObserver-in 0x9d20ac,
//   PreferencesObserver-in 0x9d209c, ModelLoaderHandler-in 0x9d208c {0x669f71, 0x668fa3},
//   cast-in 0x9d204c {sdd 0x62d30d, 0x667e23, 0x667e34, …}.
// UndoSetContent : UndoAction (Mac sizeof 0x10: vptr, scene manager +4, Object +8; vtable
//   0x70d014 {Execute 0x146d40, D1 0x146f50, D0 0x146f70}; Win 0x9d7d38 {Execute 0x667eb8},
//   Object-in 0x9d7d24 {sdd 0x667ec3}).
// Method and member names are inferred, except the class names (RTTI), the preference keys and
// the trace texts.
#pragma once
#include "LEGO/Types.h"
#include "BasicArray.h"
#include "ObjectRefPointer.h"
#include "Updater.h"
#include "Preferences.h"
#include "UndoManager.h"
#include "BrickConnectSceneControlTool.h"
#include "ModelLoaderHandler.h"
#include "ClickStickCastUtility.h"
#include "BrickDraw.h"
#include "BrickConnect.h"
#include "Render.h"
#include "Mesh.h"
#include "Scene.h"
#include "SimpleSound.h"

namespace LEGO {

class Render;
class Mesh;
class SimpleSound;
class BrickConnect;
class BrickConnectSceneManager;
class PrimitiveElementBrick;
class PrimitiveElement;
class Part;

// Cancels the carrying when undone. Mac 0x146d40 / 0x146f50 / 0x146f70, Win 0x667eb8 / sdd 0x667ec3
class UndoSetContent : public UndoAction {
public:
    explicit UndoSetContent(BrickConnectSceneManager* sceneManager) : m_sceneManager(sceneManager) {}
    virtual ~UndoSetContent() {}
    // The scene manager's active tool back to the default. Mac 0x146d40, Win 0x667eb8
    virtual void Execute();

    BrickConnectSceneManager* m_sceneManager;   // Mac +0x04
};

class ClickStickTool : public BrickConnectSceneControlTool,
                       private UpdateObserver,
                       public PreferencesObserver,
                       public ModelLoaderHandler,
                       private ClickStickCastSimpleUtility {
public:
    // Inactive; the scene manager's BrickConnect. Mac 0x18b720 (inline, emitted in EditMode.cpp),
    // Win 0x62d132
    explicit ClickStickTool(BrickConnectSceneManager* sceneManager);
    // Mac 0x1466d0 / 0x1469d0 (body 0x146150), Win sdd 0x62d337 (body 0x66938a)
    virtual ~ClickStickTool();

    // Active; the drag scene (BrickConnect::NewScene), the green box mesh of the connected
    // parts, the drop sound "IA_drop.mp3"; observes the preferences. False without a drag
    // scene. Mac 0x142b80, Win 0x668b69
    bool Create(Render* render, BrickConnectSceneManager* sceneManager);
    // Takes `bricks` into the tool (they follow the mouse until dropped): the scene control
    // carries them (SetCarriedBricks), the drag scene is built from them. `repeat`: picked up
    // again after a drop (palette, with REPEATPASTE), `returnTool`: the tool that becomes
    // active again afterwards (null when it is this one), `aligned`: the bricks keep their
    // grid offset (no +0.4 snap), `keepPivot`: a single brick of the design picked last keeps
    // the pivot. Nothing to carry: the default tool. The palette passes
    // (bricks, true, 0, false, true), the clone tool (bricks, false, itself, true, false), the
    // clip board (bricks, false, 0, true, false). Mac 0x143630, Win 0x669e7b
    void PickUp(const BasicArray<ObjectRefPointer<PrimitiveElementBrick> >& bricks, bool repeat,
                BrickConnectSceneControlTool* returnTool, bool aligned, bool keepPivot);
    // Picks up scene parts: moved into the drag scene, or copies of them
    // when `copy` (modifier bit 2); `picked` / `point` (world) the grip, `single` (modifier bit
    // 1): only the picked element snaps; `returnTool` as above. Caller ClickStickPickupTool
    // (Mac 0x394f00). Mac 0x143050, Win 0x669cde
    void PickUpParts(const BasicArray<RefPointer<Part> >& parts, PrimitiveElement* picked,
                     const Vector3& point, bool single, bool copy,
                     BrickConnectSceneControlTool* returnTool);

    // ---- BrickConnectSceneControlTool ----
    // The tool to show as active: the return tool. Mac 0x146e00, Win 0x62d306
    virtual BrickConnectSceneControlTool* GetTool();                        // 23/20

    // ---- ModelLoaderHandler: an imported model / template is loaded into the drag scene ----
    // Cancels the carrying, starts the load thread on the drag scene. Mac 0x144150 (thunk
    // 0x1442b0), Win 0x669f71
    virtual void OnLoadStarted(StorageScene2* storage);                     // 27
    // Carries the loaded model (grip at the bottom centre), or closes the document when the
    // load failed; nothing loaded: the default tool. Mac 0x1442d0 (thunk 0x1446d0), Win 0x668fa3
    virtual void OnLoadFinished();                                          // 28

    // Puts the carried bricks back: moved scene parts return to their scene (MergeScene at the
    // identity when they still connect there), new ones are dropped; the drag scene is
    // cleared and the undo action removed. Mac 0x1446f0, Win 0x668d2c (name inferred)
    virtual void Cancel(BrickConnectSceneControl* control);                 // 29/22

protected:
    // ---- ControlObserver (Mac primary slot; ControlObserver slot) ----
    // The mouse position for the next update. Mac 0x145200 (thunk 0x145240), Win 0x667dc3
    virtual void OnMouseMove(Control* control, const Vector2& position);   // 30/19
    // Remembers the press (not while recording / playing). Mac 0x145280 (thunk 0x1452c0),
    // Win 0x667ee2
    virtual void OnMousePressed(Control* control, const Vector2& position, UInt32 button); // 31/20
    // A left click (moved less than 50 pixels) with a connecting placement drops the bricks:
    // merged into the scene (with undo), the drop sound; then the next bricks (palette repeat)
    // or the return tool. Mac 0x145300 (thunk 0x1456d0), Win 0x66a057
    virtual void OnMouseReleased(Control* control, const Vector2& position, UInt32 button); // 32/21
    // Without modifiers: WASD / the arrow keys turn the carried bricks (Rotate); developer
    // mode 'I' (held) places without casting. Mac 0x145700 (thunk 0x145a60), Win 0x667f0d
    virtual void OnKeyPressed(Control* control, UInt32 key, UInt32 modifiers, bool repeat); // 33/29
    // Developer mode: 'I' released. Mac 0x145a90 (thunk 0x145c00), Win 0x6680b1
    virtual void OnKeyReleased(Control* control, UInt32 key, UInt32 modifiers); // 34/30
    // ---- UpdateObserver: a moved mouse (or a playback) casts again. Mac 0x145df0 (thunk
    // 0x145e40), Win 0x668442
    virtual void Update();                                                  // 35
    // ---- PreferencesObserver: PLAYSOUND reloads the drop sound. Mac 0x145e90 (thunk 0x145fc0),
    // Win 0x668233
    virtual void OnPreferenceChanged(const String& key);                    // 36
    // ---- ClickStickCastUtility ----
    // The playback is over: quit. Mac 0x145fe0 (thunk 0x146000), Win 0x667e23
    virtual void OnPlaybackFinished(bool success);                          // 37
    // Traces the difference. Mac 0x146020 (thunk 0x1460e0), Win 0x667e34
    virtual void OnPlaybackMismatch(UInt32 frame, UInt32 recordedFlags, UInt32 flags,
                                    const Matrix3D& recordedTransform,
                                    const Matrix3D& transform);              // 38

    // ---- BrickConnectSceneControlObserver ----
    // The base's, then observes the main updater, clears the selection, rebuilds the drawing;
    // developer mode with Right / Left held starts a cast recording / playback
    // ("cstRecording.dat" in the user data directory). Mac 0x1449b0, Win 0x669153
    virtual void OnAttached(BrickConnectSceneControl* control);
    // Cancel, the grid back, leaves the updater, clears the drag scene; a running recording /
    // playback ends the run loop. Mac 0x145060, Win 0x668e16
    virtual void OnDetached(BrickConnectSceneControl* control);
    using BrickConnectSceneControlTool::OnAttached;
    using BrickConnectSceneControlTool::OnDetached;
    using PreferencesObserver::OnAttached;
    using PreferencesObserver::OnDetached;
    using UpdateObserver::OnAttached;
    using UpdateObserver::OnDetached;
    // The frame: the carried bricks at the placement (a ghost unless connected) and the boxes
    // of the connected parts. Mac 0x145c30, Win 0x66817c
    virtual void OnEvent05(const Matrix3D& transform, Render* render, RenderQueue* overlayQueue,
                           RenderQueue* sceneQueue);
    // The camera changed: cast again at the mouse. Mac 0x145d30 / 0x145d90, Win 0x667de9 (ICF)
    virtual void OnEvent18();
    virtual void OnEvent19();

    // The observer parts, then the base's. Mac 0x146d60 (thunks 0x146e10-0x146e70)
    virtual void OnFinalRelease();

private:
    friend struct ClickStickToolTestAccess;   // (the restoration's tests)

    // The drag scene from element bricks (palette bricks with their transforms, materials,
    // decorations and behaviors); the grip at the bottom centre unless `keepPivot` and the
    // single brick's design is the last one. Mac 0x1437b0, Win 0x669851
    void LoadDragScene(const BasicArray<ObjectRefPointer<PrimitiveElementBrick> >& bricks,
                       bool keepPivot);
    // The drawing of the drag scene, and the cast begins (`first`, `single`: see
    // Scene::BeginDrag). Mac 0x1433e0, Win 0x668e9c
    void RebuildDraw(PrimitiveElement* first, bool single);
    // Casts the pick ray through `position`, collects the connected parts, moves the grid under
    // the placement. Mac 0x144580, Win 0x66838e
    void UpdateCast(const Vector2& position);

    // ---- data (Mac offsets) ----
    RefPointer<BrickConnect>        m_brickConnect;     // +0x1dc
    RefPointer<Render>              m_render;           // +0x1e0
    RefPointer<Scene>               m_dragScene;        // +0x1e4
    Vector2                         m_mousePosition;    // +0x1e8
    bool                            m_mouseMoved;       // +0x1f0
    UInt32                          m_lastDesignID;     // +0x1f4 the single brick picked last
    bool                            m_repeat;           // +0x1f8
    BrickConnectSceneControlTool*   m_returnTool;       // +0x1fc
    RefPointer<SimpleSound>         m_dropSound;        // +0x200
    bool                            m_loading;          // +0x204
    BasicArray<RefPointer<Part> >   m_loadedParts;      // +0x208 flexible parts of a load
    StorageScene2*                  m_storage;          // +0x218 the model being loaded
    BrickConnectSceneControl*       m_sourceControl;    // +0x21c the parts' scene (moved parts)
    BasicArray<RefPointer<Part> >   m_connectedParts;   // +0x220
    RefPointer<Mesh>                m_boxMesh;          // +0x230
    RefPointer<UndoSetContent>      m_undo;             // +0x234
    BrickDraw                       m_draw;             // +0x238
    bool                            m_pressed;          // +0x264
    Vector2                         m_pressPosition;    // +0x268
                                                        // Object +0x270
};

} // namespace LEGO
