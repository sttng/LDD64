// LEGO Digital Designer — restored source
// LEGO::HingeTool — the hinge tool: a click on a part picks one of its movable joints (further
//   clicks cycle through them); a hinge (joint type 1) gets one rotation band, a ball joint
//   (type 5) three; dragging a band turns the joint. Toolbar: a dial with eight presets (0° …
//   315°) for hinges and three minus / plus / text-input sets (set 1 also drives hinges); held
//   minus / plus repeat after 750 ms; Tab / Shift-Tab cycle the ball axis; keys 0xcb / 0xcd turn
//   by ±18°. Each manipulation is one undo step (UndoHingeTool); parts the moved bodies would
//   connect to are framed green and connected at the end.
// LEGO::HingeButton — ButtonControl base of the dial's buttons (no own members).
// LEGO::SetAngleButton — a dial preset: its release sets the dial angle.
// LEGO::UndoHingeTool — the undo action of one manipulation.
//
// Home TU 056 "HingeTool.cpp" (Mac 0x11d900-0x1298bb; Win 0x67f8ed-0x68435f, 0x4562c0-
// 0x45773f). The rotation band meshes come from LDDUtilities (CreateSelectableMesh, TU 063).
//
// RTTI HingeTool: vmi {BrickConnectSceneControlTool +0, DialControlObserver +0x2c,
//   TextInputControlObserver +0x40 (private), UpdateObserver +0x54}, virtual base Object
//   (Mac +0x170; sizeof Mac 0x178, Win 0x198). Win: ControlObserver +0x0c, DialControlObserver
//   +0x34, TextInputControlObserver +0x4c, UpdateObserver +0x64; members = Mac + 0x14 (Win
//   manipulator +0x70, rigid element +0x74, element +0x78, undo +0x7c, flag +0x80, axis index
//   +0x84, dragging +0x88, plane +0x8c, start angle +0x9c, reference +0xa0, centre +0xac, axis
//   +0xb8, radius +0xc4, dial +0xc8, background +0xcc, presets +0xd0, box mesh +0xe0, connect
//   parts +0xe4, rings +0x110, materials +0x120, control setters +0x140-+0x160, repeat
//   +0x164 …). Win ctor 0x680e62 is thiscall(int mostDerived), ret 4.
// Mac vtable 0x70bb2c (38 entries): D1 0x127630, D0 0x1278c0, OnFinalRelease 0x127bb0,
//   OnAttached 0x121210, OnDetached 0x121440, OnEvent05 0x125050, OnEvent06 0x125240,
//   OnEvent19 0x125780 (slot 19), GetTool 0x127fb0, OnToolEvent24 0x127fc0, focus 0x1a9810 /
//   0x1a9890, then the Mac-only entries of the secondary-base overriders in declaration order
//   27 OnAngleChanged 0x1214e0, 28 OnMouseMove 0x121690, 29 OnMousePressed 0x122690,
//   30 OnMouseReleased 0x1245e0, 31 OnKeyPressed 0x1246d0, 32 OnMouseClicked 0x124a40,
//   33 OnMouseLeave 0x124f10, 34 OnTextEntered 0x125800, the new virtuals 35
//   ResetRingMaterials 0x125b20, 36 EndManipulation 0x1267e0, and 37 Update 0x126aa0.
//   Win primary 0x9dbe74 (24 entries: OnAttached 0x6829fa, OnDetached 0x682a30, OnEvent05
//   0x67fe74, OnEvent06 0x6805cd, OnEvent19 0x682ab9, 22 ResetRingMaterials 0x6807e7,
//   23 EndManipulation 0x6833b5); ControlObserver-in 0x9dbdf4, DialControlObserver-in 0x9dbde4,
//   TextInputControlObserver-in 0x9dbdd0, UpdateObserver-in 0x9dbdc0, Object-in 0x9dbdac
//   {sdd 0x6811e3, OnFinalRelease 0x682b73}.
// Every method / member name is inferred except the RTTI class names (HingeTool::CylinderPair
// from BasicArray<HingeTool::CylinderPair>) and the Lua names of the control setters.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "ObjectRefPointer.h"
#include "BasicArray.h"
#include "Geometry.h"
#include "Updater.h"
#include "UndoManager.h"
#include "ButtonControl.h"
#include "DialControl.h"
#include "TextInputControl.h"
#include "BrickConnectSceneControlTool.h"
#include "SelectableMesh.h"
#include "ConnectionManipulator.h"

namespace LEGO {

class ButtonControl;
class ControlManager;
class RenderControl;
class BrickConnectSceneManager;
class ConnectionManipulator;
class RigidElement;
class PrimitiveElement;
class PhysicsSnapshot;
class Material;
class Texture;
class Render;
class RenderQueue;
class Part;
class HingeTool;

// ===========================================================================================
// HingeButton / SetAngleButton
// ===========================================================================================
// Mac vtable 0x76495c (= ButtonControl's but D1 0x128840, D0 0x1288a0); Win Object-in 0x9dbbc4
// {sdd 0x681f0b}. Mac sizeof 0x220 (Object +0x218).
class HingeButton : public ButtonControl {
public:
    explicit HingeButton(ControlManager* manager) : ButtonControl(manager) {}   // (inlined)
    virtual ~HingeButton() {}                                                     // Mac 0x128840/0x1288a0
};

// A preset of the dial. Mac vtable 0x764794 (D1 0x128be0, D0 0x128c40); Win Object-in 0x9dbcbc
// {sdd 0x681f2d}. Mac sizeof 0x228: m_dial +0x218, m_angle +0x21c, Object +0x220 (Win +0x238 /
// +0x23c).
class SetAngleButton : public HingeButton {
public:
    // `angle` in degrees. (inlined in HingeTool::AddSetAngleButton, Mac 0x120b70)
    SetAngleButton(ControlManager* manager, DialControl* dial, Real32 angle);
    virtual ~SetAngleButton() {}                                                  // Mac 0x128be0/0x128c40

    // Initialize, the size of the normal image rectangle at `position` below `parent`
    // (Setup flags 7), descriptions 0-2 with `image` and the three rectangles.
    // Mac 0x120d30 (Win inlined into 0x68135f)
    bool Create(Texture* image, const Rect& normal, const Rect& over, const Rect& pressed,
                Control* parent, const Vector2& position);
    using ButtonControl::Create;

    // Sets the dial angle. NOTE: original quirk: ButtonControl::OnMouseReleased is not called
    // (the button keeps its pressed look until the mouse leaves). Mac 0x128cb0, Win 0x680dc5
    virtual void OnMouseReleased(const Vector2& position, UInt32 button);

protected:
    DialControl* m_dial;    // +0x218 not referenced
    Real32       m_angle;   // +0x21c radians
};

// ===========================================================================================
// UndoHingeTool — Mac vtable 0x70bd80 {Execute 0x127f40, D1 0x127df0, D0 0x127ec0}; Win
// {Execute 0x68296f}, Object-in {sdd 0x680add}. Mac sizeof 0x28.
// ===========================================================================================
class UndoHingeTool : public UndoAction {
public:
    typedef BasicArray<Pair<PrimitiveElement*, Matrix3D> > TransformArray;

    // The transforms and a physics snapshot of the scene control's hinged elements.
    // Mac 0x125c50
    UndoHingeTool(BrickConnectSceneControl* control, HingeTool* tool);
    // The tool forgets this action. Mac 0x127df0
    virtual ~UndoHingeTool();

    // The snapshot back (with undo) or, without one, the transforms (with undo); the tool
    // forgets this action and updates its controls. Mac 0x127f40, Win 0x68296f
    virtual void Execute();

    // The manipulation ended: the snapshot goes, only the changed transforms are kept; without
    // a change the action undoes itself (UndoManager::Undo). Mac 0x1269c0
    void Finish();
    // The manipulation is cancelled: forgets the tool, the snapshot and the transforms and lets
    // the undo manager pop the action. (inlined in HingeTool, Mac 0x123412 / 0x125780)
    void Cancel();
    // The snapshot goes (cycling to the same joint). (inlined, Mac 0x1233cb)
    void ReleaseSnapshot();

private:
    BrickConnectSceneControl* m_control;      // +0x04
    HingeTool*                m_tool;         // +0x08 (not referenced)
    TransformArray            m_transforms;   // +0x0c
    PhysicsSnapshot*          m_snapshot;     // +0x1c one reference
                                              // Object +0x20
};

// ===========================================================================================
// HingeTool
// ===========================================================================================
class HingeTool : public BrickConnectSceneControlTool,
                  public DialControlObserver,
                  private TextInputControlObserver,
                  public UpdateObserver {
public:
    // The two halves of a rotation band (front / back side; 0x10 bytes, RTTI
    // BasicArray<HingeTool::CylinderPair>, Mac destructor 0x11d900 / 0x128fb0).
    struct CylinderPair {
        ObjectRefPointer<SelectableMesh> m_first;    // +0x00
        ObjectRefPointer<SelectableMesh> m_second;   // +0x08
    };

    // Mac 0x11d9a0, Win 0x680e62
    HingeTool();
    // The dial, the presets, the band materials (Graphics/EditScreen/Rotation_Arrow.png and
    // Rotation_Arrow_Selected.png, translucent / opaque), the bands and the box meshes. `manager`
    // is the RenderControlManager (its render). False when a texture, a band (NOTE: except the
    // back of the third ball band) or a box mesh is missing; the dial's own result is ignored.
    // Mac 0x11e1b0, Win 0x681f65
    bool Create(ControlManager* manager, BrickConnectSceneManager* sceneManager);

    virtual ~HingeTool();                                                   // Mac 0x127630/0x1278c0, Win sdd 0x6829d8

    // The controls of set 1, 2 and 3 (observed as Control / text input; the text inputs take
    // real numbers with two digits). (Lua names)
    void SetMinusButton(ButtonControl* button);                             // Mac 0x120280, Win 0x6811eb
    void SetPlusButton(ButtonControl* button);                              // Mac 0x1202e0, Win 0x68120d
    void SetTextInputControl(TextInputControl* control);                    // Mac 0x120340, Win 0x68122f
    void SetMinusButton2(ButtonControl* button);                            // Mac 0x1208d0, Win 0x681267
    void SetPlusButton2(ButtonControl* button);                             // Mac 0x120930, Win 0x681289
    void SetTextInputControl2(TextInputControl* control);                   // Mac 0x120990, Win 0x6812ab
    void SetMinusButton3(ButtonControl* button);                            // Mac 0x120a20, Win 0x6812e3
    void SetPlusButton3(ButtonControl* button);                             // Mac 0x120a80, Win 0x681305
    void SetTextInputControl3(TextInputControl* control);                   // Mac 0x120ae0, Win 0x681327

    // ---- BrickConnectSceneControlObserver ----
    // The frame of the moved body (red) and the parts it would connect to (green) into the
    // overlay queue. Mac 0x125050, Win 0x67fe74
    virtual void OnEvent05(const Matrix3D& transform, Render* render, RenderQueue* overlayQueue,
                           RenderQueue* sceneQueue);
    // The bands, drawn directly (hinge: one band around the joint axis; ball: three).
    // Mac 0x125240, Win 0x6805cd
    virtual void OnEvent06(Render* render);
    // Cancels the manipulation (the undo action goes, the tool forgets the joint).
    // Mac 0x125780, Win 0x682ab9
    virtual void OnEvent19();

    // ---- secondary-base overriders (Mac primary slots 27-34) ----
    // The dial turned: the hinge by the same amount. Mac 0x1214e0 (thunk 0x121510), Win 0x683764
    virtual void OnAngleChanged(DialControl* dial, Real32 angle, Real32 oldAngle);
    // Dragging: turns the joint; else the band under the mouse is highlighted.
    // Mac 0x121690 (thunk 0x122670), Win 0x683783
    virtual void OnMouseMove(Control* control, const Vector2& position);
    // Minus / plus: repeat; left click in the scene: a band starts dragging, else picks the part
    // (a new joint, the next joint of the same part, or nothing). Mac 0x122690 (thunk 0x1245c0),
    // Win 0x682b7b
    virtual void OnMousePressed(Control* control, const Vector2& position, UInt32 button);
    // Repeat off; the scene: dragging ends. Mac 0x1245e0 (thunk 0x124650), Win 0x682a85
    virtual void OnMouseReleased(Control* control, const Vector2& position, UInt32 button);
    // 0xcd / 0xcb: -18° / +18°; Tab: next ball axis (Shift: previous). Mac 0x1246d0
    // (thunk 0x1249c0), Win 0x68424d
    virtual void OnKeyPressed(Control* control, UInt32 key, UInt32 modifiers, bool repeat);
    // Minus / plus of the three sets: ±1°. Mac 0x124a40 (thunk 0x124ef0), Win 0x684297
    virtual void OnMouseClicked(Control* control, const Vector2& position, UInt32 button);
    // Leaving minus / plus ends the repeat. Mac 0x124f10 (thunk 0x124fb0), Win 0x682aa1
    virtual void OnMouseLeave(Control* control, const Vector2& position);
    // The typed angle (degrees). Mac 0x125800 (thunk 0x125b00), Win 0x6840fd
    virtual void OnTextEntered(TextInputControl* control);

    // ---- new (Mac slot / Win slot) ----
    // Band pair 0 shows the selected texture, the others the normal one. 35/22 Mac 0x125b20,
    // Win 0x6807e7
    virtual void ResetRingMaterials();
    // Ends the undo step (UndoHingeTool::Finish) and connects the moved bodies where they fit
    // (CanConnect + Reconnect). 36/23 Mac 0x1267e0, Win 0x6833b5
    virtual void EndManipulation();

    // ---- UpdateObserver: the repeat of a held minus / plus button. NOTE: original quirk:
    // the start time is not advanced, so after the delay every pass repeats (fixed: M17). ----
    virtual void Update();                                                  // 37 Mac 0x126aa0 (thunk 0x126b50), Win 0x680157

    // ---- state (test access; names inferred) ----
    ConnectionManipulator* GetManipulator() const { return m_manipulator; }
    RigidElement*          GetManipulatedRigidElement() const { return m_rigidElement; }
    PrimitiveElement*      GetManipulatedElement() const { return m_element; }
    UInt32                 GetAxisIndex() const { return m_axisIndex; }
    bool                   IsDragging() const { return m_dragging; }
    bool                   GetFlag6c() const { return m_flag6c; }
    const Vector3&         GetCenter() const { return m_center; }
    const Vector3&         GetAxis() const { return m_axis; }
    Real32                 GetRadius() const { return m_radius; }
    DialControl*           GetDial() const { return m_dial; }
    RenderControl*         GetDialBackground() const { return m_background; }
    const BasicArray<RefPointer<Control> >& GetPresets() const { return m_presets; }
    UndoHingeTool*         GetUndoAction() const { return m_undo; }
    const BasicArray<RefPointer<Part> >& GetConnectParts() const { return m_connectParts; }
    const BasicArray<CylinderPair>& GetRings() const { return m_rings; }
    SelectableMesh*        GetHingeRing(bool front) const { return front ? m_hingeFront.Get() : m_hingeBack.Get(); }
    SelectableMesh*        GetActiveRing(bool front) const { return front ? m_activeFront : m_activeBack; }
    Material*              GetRingMaterial(UInt32 index) const { return m_materials[index]; }

protected:
    // Leaves the text inputs, the dials, the scene control and the updater.
    // Mac 0x127bb0 (thunks 0x127d50-0x127dd0), Win 0x682b73
    virtual void OnFinalRelease();
    // BrickConnectSceneControlObserver: clears the selection, the dial joins the scene control.
    // Mac 0x121210, Win 0x6829fa
    virtual void OnAttached(BrickConnectSceneControl* control);
    // Ends a manipulation, the dial leaves. Mac 0x121440, Win 0x682a30
    virtual void OnDetached(BrickConnectSceneControl* control);
    using BrickConnectSceneControlTool::OnAttached;
    using BrickConnectSceneControlTool::OnDetached;
    using DialControlObserver::OnAttached;
    using DialControlObserver::OnDetached;
    using TextInputControlObserver::OnAttached;
    using TextInputControlObserver::OnDetached;
    using UpdateObserver::OnAttached;
    using UpdateObserver::OnDetached;

private:
    friend class UndoHingeTool;

    // The dial (+ background image) and its eight presets. Mac 0x11f440, Win 0x68143f
    bool CreateDialControls(ControlManager* manager);
    // One preset at `angle` degrees (kept in m_presets). Mac 0x120b70, Win 0x68135f
    bool AddSetAngleButton(ControlManager* manager, Real32 angle, const Vector2& position,
                           Texture* image, const Rect& normal, const Rect& over, const Rect& pressed);
    // Hides the dial, disables every control set and empties the text inputs. Mac 0x121260,
    // Win 0x6826da
    void ResetControls();
    // Hinge: the dial (angle) and set 1. Mac 0x123e20, Win 0x681bfa
    void EnableHingeControls();
    // Ball: the three sets with the joint angles; the dial hidden. Mac 0x123f00, Win 0x68259a
    void EnableBallControls();
    // The joint values into the dial / text inputs. Mac 0x125990, Win 0x68280f
    void UpdateControlValues();
    // m_axis = the ball axis m_axisIndex (NOTE: original quirk: in the order (b x a, a, b) of the
    // joint frame, not the (a, b, a x b) of the bands). Mac 0x121540 (Win 0x68040d, unverified)
    void UpdateRingAxes();
    // Turns the joint by `delta`. Mac 0x121010, Win 0x68371c
    void RotateBy(Real32 delta);
    // Starts the undo step on the first change, moves the joint, updates the connect parts and
    // the controls. Mac 0x1210f0, Win 0x683663
    void Apply(bool first, Real32 delta, const Vector3* axis);
    // The parts the moved bodies would connect to. Mac 0x125d20, Win 0x68345b
    bool UpdateConnectParts();
    // The manipulated body is the joint's body 1. Win 0x680207 (Mac inlined)
    bool IsFirstBody() const;
    // m_rigidElement, else the body of m_element. (inlined)
    RigidElement* GetBody() const;
    // The band pair shown as active (`first` / `second` get the active materials, the old pair
    // the normal ones). Mac 0x1239b0 (Win 0x680064, unverified)
    void SetActiveRing(const RefPointer<SelectableMesh>& first, const RefPointer<SelectableMesh>& second);
    // SetActiveRing(m_rings[m_axisIndex]). (inlined)
    void ActivateRing();
    // Ball axis -1 / +1 (Shift-Tab / Tab). Mac 0x124740 / 0x124880, Win 0x6804c6 / 0x68054f
    void PreviousAxis();
    void NextAxis();
    // Minus / plus of ball axis `index`. Mac 0x124b50 / 0x124d20, Win 0x683f87 / 0x684042
    void BallMinus(UInt32 index);
    void BallPlus(UInt32 index);
    // A minus / plus button was pressed: repeat starts. Mac 0x123680, Win 0x682ae6
    void StartRepeat(Control* control);
    // The repeat ends. (inlined)
    void StopRepeat();
    // One of the six minus / plus buttons. Win 0x67fe36 (Mac inlined)
    bool IsStepButton(Control* control) const;
    // Draws one band half around `axis` at `center` (radius m_radius; the back half at 25 %
    // opacity). Mac 0x125500, Win 0x67f909
    void DrawRing(const Vector3& axis, const Vector3& center, SelectableMesh* mesh, Render* render,
                  bool front);
    // Redraw of the scene control unless its manager is locked. (inlined)
    void RedrawSceneControl();

    // ---- data (Mac offsets) -----------------------------------------------------------------
    RefPointer<ConnectionManipulator> m_manipulator;     // +0x5c
    RigidElement*                     m_rigidElement;    // +0x60 picked body (no element)
    PrimitiveElement*                 m_element;         // +0x64 picked element
    UndoHingeTool*                    m_undo;            // +0x68 current undo step
    bool                              m_flag6c;          // +0x6c true (not read)
    UInt32                            m_axisIndex;       // +0x70 ball axis
    bool                              m_dragging;        // +0x74
    Plane                             m_plane;           // +0x78 plane of the dragged band
    Real32                            m_startAngle;      // +0x88 hinge angle at the press (NOTE: not initialised)
    Vector3                           m_reference;       // +0x8c direction of the drag start
    Vector3                           m_center;          // +0x98 joint position (world)
    Vector3                           m_axis;            // +0xa4 ball axis (body space)
    Real32                            m_radius;          // +0xb0 2.0
    RefPointer<DialControl>           m_dial;            // +0xb4
    RefPointer<RenderControl>         m_background;      // +0xb8 the dial picture
    BasicArray<RefPointer<Control> >  m_presets;         // +0xbc SetAngleButtons
    RefPointer<SingleMesh>            m_boxMesh;         // +0xcc red frame
    BasicArray<RefPointer<Part> >     m_connectParts;    // +0xd0
    RefPointer<SingleMesh>            m_connectMesh;     // +0xe0 green frame
    RefPointer<SelectableMesh>        m_activeFront;     // +0xe4
    RefPointer<SelectableMesh>        m_activeBack;      // +0xe8
    ObjectRefPointer<SelectableMesh>  m_hingeFront;      // +0xec
    ObjectRefPointer<SelectableMesh>  m_hingeBack;       // +0xf4
    BasicArray<CylinderPair>          m_rings;           // +0xfc ball bands
    // [0]/[1] translucent arrow (normal), [2]/[3] opaque arrow (hover), [4]/[5] translucent
    // selected arrow, [6]/[7] opaque selected arrow; even: TexturedMaterial (front), odd:
    // ReverseWindingTexturedMaterial (back).
    RefPointer<Material>              m_materials[8];    // +0x10c
    RefPointer<ButtonControl>         m_minus1;          // +0x12c
    RefPointer<ButtonControl>         m_plus1;           // +0x130
    RefPointer<TextInputControl>      m_text1;           // +0x134
    RefPointer<ButtonControl>         m_minus2;          // +0x138
    RefPointer<ButtonControl>         m_plus2;           // +0x13c
    RefPointer<TextInputControl>      m_text2;           // +0x140
    RefPointer<ButtonControl>         m_minus3;          // +0x144
    RefPointer<ButtonControl>         m_plus3;           // +0x148
    RefPointer<TextInputControl>      m_text3;           // +0x14c
    RefPointer<Control>               m_repeatControl;   // +0x150
    UInt64                            m_repeatInterval;  // +0x154 50 ms
    UInt64                            m_repeatDelay;     // +0x15c 750 ms
    bool                              m_repeatFirst;     // +0x164
    UInt64                            m_repeatStart;     // +0x168
                                                         // Object +0x170
};

} // namespace LEGO
