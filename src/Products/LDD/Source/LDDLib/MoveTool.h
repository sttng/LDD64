// LEGO Digital Designer — restored source
// LEGO::MoveTool — moves the selection along an axis ("moveTool"): a left drag of a part
//   (beyond 3 x 1 pixels) moves the selection (with the part) into a drag scene; the drag scene
//   then shows its axes, a press on one of the axis handles (cylinders along x / y / z) picks the
//   axis, and dragging moves the bricks along it (Space held: in the plane across it), snapped
//   to the scene (ClickStickCastUtility, the 2D cast). The bricks are put back (connected where
//   they were placed) when the tool goes away or the move is undone.
// LEGO::MoveTool::UndoSetContent — that undo action: puts the bricks back, default tool.
//
// TU 067 "MoveTool.cpp" (Mac 0x15b6b0-0x15e71b, Win 0x67bb5a-0x67cb9d). Not code in the TU: the
// thunks, the BasicArray instances, Mac 0x15e6a0 = <iostream> + a header static; the empty
// ClickStickCastUtility slots 2 / 3 (Mac 0x15dfd0 / 0x15dfe0, ClickStickCastUtility.h).
// RTTI: MoveTool : BrickConnectSceneControlTool +0, PreferencesObserver +0x2c,
//   ClickStickCastSimpleUtility +0x34; virtual base Object +0x2d4 (Mac sizeof 0x2dc; Win 0x2ec).
//   Mac vtable 0x70f4cc (31 entries: OnFinalRelease 0x15dbf0, OnAttached 0x15c800, OnDetached
//   0x15c840, OnEvent05 0x15d7a0, OnEvent06 0x15d8a0; new 27-30 below); ControlObserver-in
//   0x70f554, PreferencesObserver-in 0x70f5e8, cast-in 0x70f608, Object-in 0x70f660.
//   Win primary 0x9dae4c (OnAttached 0x67bb5a, OnDetached 0x67c383, OnEvent05 0x67bc16, OnEvent06
//   0x67bccd), ControlObserver-in 0x9dadcc {move 0x67c7d8, press 0x67be06, release 0x67bb7b},
//   PreferencesObserver-in 0x9dadbc {0x67bd76}, cast-in 0x9dad7c, Object-in 0x9dad68.
// MoveTool::UndoSetContent : UndoAction (Mac sizeof 0x14: vptr, scene manager +4, tool +8,
//   Object +0xc; vtable 0x70fae4 {Execute 0x15dae0, D1 0x15e140, D0 0x15e160}; Win 0x9dad60
//   {Execute 0x67c3b8}, Object-in 0x9dad4c {sdd 0x67bb86}).
// Method and member names are inferred, except the class names (RTTI).
#pragma once
#include "LEGO/Types.h"
#include "BasicArray.h"
#include "Preferences.h"
#include "UndoManager.h"
#include "BrickConnectSceneControlTool.h"
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
class PrimitiveElement;
class Part;

class MoveTool : public BrickConnectSceneControlTool,
                 public PreferencesObserver,
                 private ClickStickCastSimpleUtility {
public:
    class UndoSetContent : public UndoAction {
    public:
        UndoSetContent(BrickConnectSceneManager* sceneManager, MoveTool* tool)
            : m_sceneManager(sceneManager), m_tool(tool) {}
        virtual ~UndoSetContent() {}                                        // Mac 0x15e140/0x15e160, Win sdd 0x67bb86
        // The bricks back (Drop, without the placement), the default tool. Mac 0x15dae0,
        // Win 0x67c3b8
        virtual void Execute();

        BrickConnectSceneManager* m_sceneManager;   // Mac +0x04
        MoveTool*                 m_tool;           // Mac +0x08 (not referenced)
    };

    // Inactive, no axis; the scene manager's BrickConnect. Mac 0x15b7f0, Win 0x67c3da
    explicit MoveTool(BrickConnectSceneManager* sceneManager);
    // Mac 0x15db20 / 0x15db80 (body 0x15e210), Win sdd 0x67cb9d (body 0x67c5bb)
    virtual ~MoveTool();

    // Active; the drag scene, the green box mesh, the half transparent axes, the drop sound;
    // observes the preferences. False without a drag scene. Mac 0x15bb30, Win 0x67c03c
    bool Create(Render* render, BrickConnectSceneManager* sceneManager);

protected:
    // ---- ControlObserver (Mac primary slot; ControlObserver slot) ----
    // Left button held on the scene control: before a move, a drag beyond 3 x 1 pixels picks
    // the selection (and the pressed part) up; during a move, the axis drag (closest point of
    // the axis to the ray, or with Space the plane across the axis) and the 2D cast.
    // Mac 0x15c940 (thunk 0x15d130), Win 0x67c7d8
    virtual void OnMouseMove(Control* control, const Vector2& position);   // 27/19
    // Left button: before a move, the pressed element and point; during a move, the axis
    // handle under the mouse (nearest of the three cylinders, none: 3). Mac 0x15d150 (thunk
    // 0x15d750), Win 0x67be06
    virtual void OnMousePressed(Control* control, const Vector2& position, UInt32 button); // 28/20
    // Nothing (NOTE: reads the scene control only). Mac 0x15d770 (thunk 0x15d780), Win 0x67bb7b
    virtual void OnMouseReleased(Control* control, const Vector2& position, UInt32 button); // 29/21
    // ---- PreferencesObserver: PLAYSOUND reloads the drop sound. Mac 0x15d990 (thunk 0x15dac0),
    // Win 0x67bd76
    virtual void OnPreferenceChanged(const String& key);                    // 30

    // ---- BrickConnectSceneControlObserver ----
    // The base's; no move, no axis. Mac 0x15c800, Win 0x67bb5a
    virtual void OnAttached(BrickConnectSceneControl* control);
    // Drop, the grid back, the drawing cleared, the base's. Mac 0x15c840, Win 0x67c383
    virtual void OnDetached(BrickConnectSceneControl* control);
    using BrickConnectSceneControlTool::OnAttached;
    using BrickConnectSceneControlTool::OnDetached;
    using PreferencesObserver::OnAttached;
    using PreferencesObserver::OnDetached;
    // The frame: the moved bricks (a ghost unless connected) and the connected parts' boxes.
    // Mac 0x15d7a0, Win 0x67bc16
    virtual void OnEvent05(const Matrix3D& transform, Render* render, RenderQueue* overlayQueue,
                           RenderQueue* sceneQueue);
    // After the queues: the axes of a move, half transparent through the scene, then opaque.
    // Mac 0x15d8a0, Win 0x67bccd
    virtual void OnEvent06(Render* render);

    // Leaves the preferences, the base's. Mac 0x15dbf0 (thunks 0x15dd30, 0x15de80, 0x15e0e0)
    virtual void OnFinalRelease();

private:
    friend struct MoveToolTestAccess;   // (tests/test_clickstick.cpp)

    // The parts into the drag scene (moved), the grip at `point` (world). Mac 0x15c120,
    // Win 0x67c6d3
    void PickUp(const BasicArray<RefPointer<Part> >& parts, PrimitiveElement* picked,
                const Vector3& point, bool single);
    // Puts a move down: merged into the parts' scene at the placement (connected) or at the
    // identity (when they still connect there); the drawing and the undo action go.
    // Mac 0x15c2d0, Win 0x67c1d4
    void Drop();
    // The drawing of the drag scene; the cast begins. Mac 0x15c5b0, Win 0x67c2d0
    void RebuildDraw(PrimitiveElement* first, bool single);

    // ---- data (Mac offsets) ----
    UInt32                          m_state;            // +0x1cc 0 waiting, 1 moving
    PrimitiveElement*               m_picked;           // +0x1d0
    Vector2                         m_pressPosition;    // +0x1d4
    Vector3                         m_pickPoint;        // +0x1dc world
    Matrix3D                        m_drawTransform;    // +0x1e8 the cast placement
    Matrix3D                        m_moveTransform;    // +0x228 the dragged frame (axes)
    Vector3                         m_axisPoint;        // +0x268 the grabbed point, in the frame
    UInt32                          m_axis;             // +0x274 0-2, 3 none
    BasicArray<RefPointer<Part> >   m_connectedParts;   // +0x278
    RefPointer<Mesh>                m_boxMesh;          // +0x288
    BrickConnectSceneControl*       m_sourceControl;    // +0x28c
    RefPointer<BrickConnect>        m_brickConnect;     // +0x290
    RefPointer<Render>              m_render;           // +0x294
    RefPointer<Scene>               m_dragScene;        // +0x298
    RefPointer<SimpleSound>         m_sound;            // +0x29c
    RefPointer<Mesh>                m_axesMesh;         // +0x2a0
    BrickDraw                       m_draw;             // +0x2a4
    RefPointer<UndoSetContent>      m_undo;             // +0x2d0
                                                        // Object +0x2d4
};

} // namespace LEGO
