// LEGO Digital Designer — restored source (header only)
// LEGO::Control — base of every control: a node of a ControlManager's control tree with a
// rectangle (position, size, scale, min / max size, anchors), status bits, named augmentations,
// a hit-test selection mask and ControlObservers; each notification virtual's default forwards
// to the observers' handler of the same name. LEGO::ControlObserver = ObserverN<Control, ...>.
// TU 027 "Control.cpp" (with ControlManager; Mac 0xd7130-0xeb1db, sub-TUs 027a/b/c; Win
// ≈ 0x506446-0x50cbc0). Bodies: Control.cpp; layer ctor, Invalidate,
// InvalidateBuffered, FindControlAt, augmentation map: ControlManager.cpp.
// Directory UI/ is a guess (no __FILE__ evidence).
//
// Layout (Mac, from the ObjC type encoding of -[LEGOTextInputControlTarget
// initWithTextInputControl:] / ivar LEGOTextInputControlTarget.control, objc.json):
//   ^^? {BasicArray<ControlObserver*>} {BasicArray<NotifierT<Control,ControlObserver>::Iterator*>}
//   ^{ControlManager} ^{Control}×5 {RefPointer<const SelectionMask>} {Vector2}×5 I
//   [2{AnchorAxisData={InternalAnchorData=fffff^{Control}B^{Control}}×2}]
//   {BasicArray<Control*>}×2 {StatusBitPattern=b1×12} I {String} I I B B
//   {BasicMap<String, RefPointer<ControlAugmentation>, Less<String> >}
//   {BasicList<ControlObserver*>}                                         (Control ends +0x158)
//   … TextInputControl members … ^^? i                                   (virtual base Object)
// Mac sizeof(Control) 0x160 (operator new 0x160 in ControlManager::CreateLayer 0xdc580),
// Object at +0x158 (Object-in-Control vtable 0x706f68, offset-to-top -0x158).
// Win: vfptr +0, vbptr +4, every member 4 bytes later than on Mac up to the String (Win String
// is one pointer), Object at +0x170 (vtable 0xa28014 {sdd 0x50cbc0, OnFinalRelease 0x50cba8,
// …}). Known Win offsets: size +0x4c, parent +0x2c, status +0x110 (bit 9 = byte +0x111 bit 1).
//
// Virtual slots: Mac primary vtable 0x706ec0 (36 entries); Win 0xa27f8c (33 entries):
//   Mac 0/1 D1/D0 (Win: scalar deleting dtor in the Object vtable), Mac 2 OnFinalRelease (Win:
//   Object vtable), Mac 3..35 = Win 0..32 in declaration order.
// Names: OnRectangleChanged, OnShown, OnHidden, OnEnabled, OnDisabled, OnMousePressed,
// OnMouseReleased, OnMouseClicked, OnChar are the Lua callbacks LuaControl forwards them to
// (TU 315: ControlObserver slots 9, 11-14, 20-22, 31); the others are inferred (marked).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "Geometry.h"
#include "BasicArray.h"
#include "BasicList.h"
#include "BasicMap.h"
#include "Notifier.h"
#include "ObserverN.h"
#include "BitmapSelectionMask.h"
#include "ControlAugmentation.h"

namespace LEGO {

class Control;
class ControlManager;

class Render;
class VertexBuffer;
class Texture;
struct Color;
class Matrix3D;

// The drawing state handed down the control tree (Control::DrawTree Mac 0xe2420, the manager's
// root draw Mac 0xd9470): the current control's offset and accumulated scale in the coordinates
// of the buffer being drawn, the clip rectangle there, and that buffer's size. DrawTree resets
// them (offset 0, scale 1, clip Rect(size), size) before a buffered control draws into its own
// buffer. The render-layer members behind +0x28 and the drawing helpers are restored
// (RenderControl.cpp, TU 065; RenderControlManager fills them, Mac 0x153640 / Win 0x50dd05:
// {0,0} {1,1} Rect() {0,0} render quad lines {1,1} 0). Win layout identical. (names inferred)
struct ControlDrawArgs {
    ControlDrawArgs()
        : m_clip(0.0f, 0.0f, 0.0f, 0.0f), m_render(0), m_quad(0), m_lines(0),
          m_targetScale(1.0f, 1.0f), m_state(0) {}

    Vector2       m_offset;      // +0x00 position of the control's origin in the target
    Vector2       m_scale;       // +0x08 accumulated scale
    Rect          m_clip;        // +0x10 visible part of the target
    Vector2       m_size;        // +0x20 size of the target (buffer)
    Render*       m_render;      // +0x28
    VertexBuffer* m_quad;        // +0x2c 4-vertex quad (RenderControlManager +0xdc)
    VertexBuffer* m_lines;       // +0x30 2-vertex line (RenderControlManager +0xe0)
    Vector2       m_targetScale; // +0x34 render-target pixels per unit (buffer scale)
    UInt32        m_state;       // +0x3c render state set up last: 0 none, 1 2D, 2 3D (Setup)

    enum { kStateNone = 0, kState2D = 1, kState3D = 2 };   // (names inferred)

    // (names inferred, bodies in RenderControl.cpp)
    // 2D: orthographic projection over the whole target, blending, no depth/lighting/culling,
    // world = scale + floored offset. 3D (`viewport3D`): viewport over the control's area,
    // depth test, lighting, culling. Both: scissor = clip. Mac 0x1523d0, Win 0x50d259
    void Setup(bool viewport3D);
    // World matrix = current world * `m`. Mac 0x152740, Win 0x50d48f
    void MultiplyWorldMatrix(const Matrix3D& m);
    // Textured quad (corners floored), texture coordinates `textureRect`. Mac 0x1527a0, Win 0x50d4cd
    void DrawTexture(const Rect& rect, Texture* texture, const Rect& textureRect,
                     const Color& color, bool blend);
    // Untextured quad. Mac 0x1529a0, Win 0x50d562
    void DrawRect(const Rect& rect, const Color& color, bool blend);
    // Line from `a` to `b` (floored). Mac 0x152b50, Win 0x50d5a5
    void DrawLine(const Vector2& a, const Vector2& b, const Color& color, bool blend);
    // Render target `target` (null: the screen) cleared with `clearColor`, m_targetScale = `scale`,
    // state reset (BufferedAbleRenderControl::BeginDraw, RenderRoot::BeginDraw; inlined on Mac).
    // Win 0x50d219 (name inferred)
    void BeginTarget(Texture* target, const Color& clearColor, const Vector2& scale);
    // m_clip in the drawn control's own units: (clip - offset) / scale per edge. Win 0x50cbd8
    // (inlined on Mac). (name inferred)
    Rect GetLocalClip() const;
};

// ===========================================================================================
// ControlObserver — Mac vtable 0x70589c (34 entries): ObserverN slots 0-4, handlers 5-33 (all
// empty, Mac 0x6e3e0-0x6e590 in 0x10 steps; slot 20 = 0x6be10). Win 0x9c9994 (31 entries:
// OnAttached, OnDetached, handlers = Mac slot - 3), Object-in 0x9c997c. Every handler gets the
// notifying control first (observers watch several controls).
// ===========================================================================================
class ControlObserver : public ObserverN<Control, ControlObserver> {
public:
    ControlObserver() {}
    virtual ~ControlObserver() {}                                            // Mac 0x6e2b0/0x6e300

    // (Mac slot, Win slot = Mac - 3; the Control slot that notifies = Mac slot - 1)
    virtual void OnConnected(Control* /*control*/) {}                        // 5   (name inferred)
    virtual void OnDisconnected(Control* /*control*/) {}                     // 6   (name inferred)
    virtual void OnDestroyed(Control* /*control*/) {}                        // 7   (name inferred)
    virtual void OnParentChanged(Control* /*control*/) {}                    // 8   (name inferred)
    virtual void OnRectangleChanged(Control* /*control*/, const Vector2& /*oldPosition*/,
                                    const Vector2& /*oldSize*/) {}            // 9   (Lua name)
    virtual void OnScaleChanged(Control* /*control*/, const Vector2& /*oldScale*/) {} // 10 (name inferred)
    virtual void OnShown(Control* /*control*/) {}                            // 11  (Lua name)
    virtual void OnHidden(Control* /*control*/) {}                           // 12  (Lua name)
    virtual void OnEnabled(Control* /*control*/) {}                          // 13  (Lua name)
    virtual void OnDisabled(Control* /*control*/) {}                         // 14  (Lua name)
    // Win default 0x44b280 returns true.
    virtual bool OnActivated(Control* /*control*/) { return true; }          // 15  (name inferred)
    virtual void OnDeactivated(Control* /*control*/) {}                      // 16  (name inferred)
    virtual void OnMouseEnter(Control* /*control*/, const Vector2& /*position*/) {} // 17 (name inferred)
    virtual void OnMouseLeave(Control* /*control*/, const Vector2& /*position*/) {} // 18 (name inferred)
    virtual void OnMouseMove(Control* /*control*/, const Vector2& /*position*/) {}  // 19 (name inferred)
    virtual void OnMousePressed(Control* /*control*/, const Vector2& /*position*/,
                                UInt32 /*button*/) {}                         // 20  (Lua name; Mac 0x6be10)
    virtual void OnMouseReleased(Control* /*control*/, const Vector2& /*position*/,
                                 UInt32 /*button*/) {}                        // 21  (Lua name)
    virtual void OnMouseClicked(Control* /*control*/, const Vector2& /*position*/,
                                UInt32 /*button*/) {}                         // 22  (Lua name)
    virtual void OnMouseDoubleClicked(Control* /*control*/, const Vector2& /*position*/,
                                      UInt32 /*button*/) {}                   // 23  (name inferred)
    virtual void OnMouseWheel(Control* /*control*/, UInt32 /*axis*/, Real32 /*delta*/) {} // 24 (name inferred)
    virtual void OnMouseCaptured(Control* /*control*/) {}                    // 25  (name inferred)
    virtual void OnMouseCaptureLost(Control* /*control*/) {}                 // 26  (name inferred)
    // BackgroundControlObserver 0x141390 / BrickConnectSceneControlTool 0x1a9850 override it.
    virtual void OnKeyboardFocusGained(Control* /*control*/, bool /*flag*/, Control* /*previous*/,
                                       UInt32 /*modifiers*/) {}               // 27  (name inferred)
    virtual void OnKeyboardFocusLost(Control* /*control*/, Control* /*next*/) {} // 28 (name inferred)
    virtual void OnKeyPressed(Control* /*control*/, UInt32 /*key*/, UInt32 /*modifiers*/,
                              bool /*repeat*/) {}                             // 29  (name inferred)
    virtual void OnKeyReleased(Control* /*control*/, UInt32 /*key*/, UInt32 /*modifiers*/) {} // 30 (name inferred)
    virtual void OnChar(Control* /*control*/, UniChar16 /*c*/, UInt32 /*modifiers*/,
                        bool /*repeated*/) {}                                 // 31  (Lua name)
    // Win default 0x44b280 returns true.
    virtual bool OnBufferingEnabled(Control* /*control*/) { return true; }   // 32  (name inferred)
    virtual void OnBufferingDisabled(Control* /*control*/) {}                // 33  (name inferred)
};

// ===========================================================================================
// One anchor of one axis (0x20 bytes; Lua layout attributes "hAnchor" / "vAnchor" / "Anchors"):
// ties the point `m_values[0] * size + m_values[1]` of the control (own, unscaled units) to the
// point `m_values[2] * size + m_values[3]` of the reference control, plus `m_values[4]` (parent
// units):
//   position + (v0 * size + v1) * scale + v4 = AnchorPosition (Control::LayoutAxis, Mac 0xe81f0)
// m_reference null with m_flag set: the parent (SetParent re-targets it). m_commonAncestor: the
// first common ancestor of the reference's and the control's parents (Control::UpdateAnchors,
// Mac 0xdf6c0); null = inactive. (struct name from the type encoding; member names inferred)
struct InternalAnchorData {
    // All zero (part of the Control constructor, Mac 0xde780).
    InternalAnchorData();
    // Mac 0xe1c70 (Win inlined)
    InternalAnchorData(Real32 v0, Real32 v1, Real32 v2, Real32 v3, Real32 v4,
                       Control* reference, bool referenceIsParent);

    // Stores the values and the reference (null: `owner`'s parent, m_flag set); sets
    // `referenceChanged` / `changed` when the reference / anything changed (everything is stored
    // once either is set). Win 0x50a831 / 0x50a86d (Mac inlined)
    void Set(Control* owner, Real32 v0, Real32 v1, Real32 v2, Real32 v3, Real32 v4,
             Control* reference, bool& referenceChanged, bool& changed);
    // The same from another anchor (null: no change); its m_flag picks the parent. Win 0x50a905
    void Set(Control* owner, const InternalAnchorData* anchor, bool& referenceChanged, bool& changed);
    // Alignment anchor: v0/v2 from the alignments (Control::AlignmentToFraction), v1 = v3 = 0.
    // Win 0x50a994 (Mac inlined)
    void SetAligned(Control* owner, UInt32 alignment, UInt32 referenceAlignment, Real32 offset,
                    Control* reference, bool& referenceChanged, bool& changed);
    // Drops the reference (anchor off). Win 0x50a943 tail / 0x50a9e0 (Mac inlined)
    void Clear(bool& referenceChanged);

    Real32   m_values[5];        // +0x00 own fraction, own offset, reference fraction,
                                 //       reference offset, offset (see above)
    Control* m_reference;        // +0x14
    bool     m_flag;             // +0x18 the reference is the parent
    Control* m_commonAncestor;   // +0x1c (UpdateAnchors)
};

// The two anchors (near / far edge, inferred) of one axis (0x40 bytes).
struct AnchorAxisData {
    InternalAnchorData m_anchors[2];   // +0x00, +0x20
};

// The control's status bits (12 one-bit fields; read as a UInt16 at Control +0x10c).
// Meaning from Control.cpp (Mac 0xdfa70 … 0xe2090, 0xdb0d0); names inferred.
struct StatusBitPattern {
    // bit 0: layer root (the layer ctor sets the low byte to 0xff): rectangle, scale and parent are
    // fixed (SetRectangle/SetScale/SetParent… refuse), no manager reference
    UInt16 m_fixedScale     : 1;
    UInt16 m_connected      : 1;   // bit 1: part of an attached tree (OnConnected / OnDisconnected)
    UInt16 m_visible        : 1;   // bit 2: shown and every parent visible (OnShown / OnHidden)
    UInt16 m_shown          : 1;   // bit 3: own show flag
    UInt16 m_enabledInTree  : 1;   // bit 4: enabled and every parent enabled (OnEnabled / OnDisabled)
    UInt16 m_enabled        : 1;   // bit 5: own enable flag
    UInt16 m_activeInTree   : 1;   // bit 6: OnActivated / OnDeactivated
    UInt16 m_active         : 1;   // bit 7: own flag of bit 6
    UInt16 m_mouseInside    : 1;   // bit 8: the mouse is over the control
    UInt16 m_invalid        : 1;   // bit 9: needs drawing (Invalidate sets it, Draw clears it)
    UInt16 m_bit10          : 1;   // bit 10: hit-testable without a selection mask (IsInside, SetHitTestable)
    UInt16 m_bit11          : 1;   // bit 11 (meaning unknown)
};

// ===========================================================================================
// Control
// ===========================================================================================
class Control : public Notifier<ControlObserver> {
public:
    // A child control of `manager` (not yet in the tree). Mac 0xde780 (C2), Win ≈ 0x508xxx
    explicit Control(ControlManager* manager);
    // One of the manager's three layer roots (layer 0, 1, 2: ControlManager +0x60 + 4 * layer);
    // used by ControlManager::CreateLayer. Mac 0xdc5d0
    Control(ControlManager* manager, UInt32 layer);
    virtual ~Control();                                                     // Mac 0xe7230/0xe7300

    // ---- virtual slots (Mac slot / Win slot) ------------------------------------------------
    // Point (control coordinates) inside the control: the selection mask decides when there is
    // one, else status bit 10 (SetHitTestable). (name inferred)
    virtual bool IsInside(const Vector2& point) const;                      // 3/0   Mac 0xe1ff0, Win 0x508c49
    // Notifications; each default forwards to the observers' handler of the same name
    // (Mac 0xe2770 … 0xe6d50, one 0x253-0x273-byte body each). (names: see header comment)
    virtual void OnConnected();                                             // 4/1   Mac 0xe2770
    virtual void OnDisconnected();                                          // 5/2   Mac 0xe29e0
    // Slot 6 is sent after the parent changed (SetParent, OnFinalRelease), slot 7 after the sibling
    // order changed (SetZOrder); the names are inferred.
    virtual void OnDestroyed();                                             // 6/3   Mac 0xe2c50
    virtual void OnParentChanged();                                         // 7/4   Mac 0xe2ec0
    virtual void OnRectangleChanged(const Vector2& oldPosition, const Vector2& oldSize); // 8/5 Mac 0xe3130
    virtual void OnScaleChanged(const Vector2& oldScale);                   // 9/6   Mac 0xe33f0
    virtual void OnShown();                                                 // 10/7  Mac 0xe3690
    virtual void OnHidden();                                                // 11/8  Mac 0xe3900
    virtual void OnEnabled();                                               // 12/9  Mac 0xe3b70
    virtual void OnDisabled();                                              // 13/10 Mac 0xe3de0
    // False keeps the control inactive (status bit 6). (name inferred)
    virtual bool OnActivated();                                             // 14/11 Mac 0xe4050
    virtual void OnDeactivated();                                           // 15/12 Mac 0xe42c0
    virtual void OnMouseEnter(const Vector2& position);                     // 16/13 Mac 0xe4530
    virtual void OnMouseLeave(const Vector2& position);                     // 17/14 Mac 0xe47b0
    virtual void OnMouseMove(const Vector2& position);                      // 18/15 Mac 0xe4a30
    virtual void OnMousePressed(const Vector2& position, UInt32 button);    // 19/16 Mac 0xe4cb0
    virtual void OnMouseReleased(const Vector2& position, UInt32 button);   // 20/17 Mac 0xe4f30
    virtual void OnMouseClicked(const Vector2& position, UInt32 button);    // 21/18 Mac 0xe51b0
    virtual void OnMouseDoubleClicked(const Vector2& position, UInt32 button); // 22/19 Mac 0xe5430
    // axis 0 = horizontal, 1 = vertical (ControlManager::OnMouseWheel, Mac 0xda1a0).
    virtual void OnMouseWheel(UInt32 axis, Real32 delta);                   // 23/20 Mac 0xe56b0
    // The manager's mouse capture (+0x70 with +0x91 set) starts / ends. (names inferred)
    virtual void OnMouseCaptured();                                         // 24/21 Mac 0xe5940
    virtual void OnMouseCaptureLost();                                      // 25/22 Mac 0xe5bb0
    // ControlManager::SetKeyboardFocus (Mac 0xd8a00): `previous` is the previous focus control,
    // `modifiers` the manager's modifier state (+0xa4). (signature inferred)
    virtual void OnKeyboardFocusGained(bool flag, Control* previous, UInt32 modifiers); // 26/23 Mac 0xe5e20
    virtual void OnKeyboardFocusLost(Control* next);                        // 27/24 Mac 0xe60b0
    virtual void OnKeyPressed(UInt32 key, UInt32 modifiers, bool repeat);   // 28/25 Mac 0xe6330
    virtual void OnKeyReleased(UInt32 key, UInt32 modifiers);               // 29/26 Mac 0xe65c0
    virtual void OnChar(UniChar16 c, UInt32 modifiers, bool repeated);      // 30/27 Mac 0xe6840
    // Input flag 4 (+0x110 bit 2) switched on / off: BufferedAbleRenderControl creates / releases
    // its render buffer; false refuses. (names inferred)
    virtual bool OnBufferingEnabled();                                      // 31/28 Mac 0xe6ad0
    virtual void OnBufferingDisabled();                                     // 32/29 Mac 0xe6d50
    // Drawing (called by the non-virtual tree walk Mac 0xe2420: BeginDraw, Draw, children,
    // EndDraw). Control::Draw only clears the invalid bit. (names inferred)
    virtual void Draw(ControlDrawArgs& args);                               // 33/30 Mac 0x4aec0, Win 0x455e00
    virtual void EndDraw(ControlDrawArgs& args);                            // 34/31 Mac 0x510d0 (empty), Win 0x50fca7
    virtual void BeginDraw(ControlDrawArgs& args);                          // 35/32 Mac 0x510e0 (empty), Win 0x50fca7

protected:
    // Detaches augmentations, observers, children, anchor dependents and the parent.
    virtual void OnFinalRelease();                                          // 2/–   Mac 0xe73f0, Win 0x5099d0

public:
    // ---- non-virtual (restored code uses these; addresses where known) ----------------------
    ControlManager* GetManager() const { return m_manager; }
    Control*        GetParent() const { return m_parent; }
    // Tree walk (read inline by the original: TooltipManager Mac 0x1137b0, …). (additive)
    Control*        GetNextSibling() const { return m_nextSibling; }
    Control*        GetPreviousSibling() const { return m_previousSibling; }
    Control*        GetFirstChild() const { return m_firstChild; }
    Control*        GetLastChild() const { return m_lastChild; }
    const Vector2&  GetPosition() const { return m_position; }
    // Read inline by BrickConnectSceneControl (Mac 0x45790) and LDDUtilities (Mac 0x149470).
    // NOTE: an inline field read in the original (Mac +0x48 / Win +0x4c); out of line here so the
    // test seams (tests/diff/diff_render_model.cpp) can provide it.
    const Vector2& GetSize() const;
    const Vector2& GetScale() const { return m_scale; }
    const String&  GetName() const { return m_name; }
    UInt32         GetCursor() const { return m_cursor; }
    // (additive) The second argument of the last SetCursor (+0x12c; read inline by
    // LuaControl Mac 0x2bfa90 for the "cursor" layout attribute). (name inferred)
    bool           GetCursorFlag() const { return m_flag12c; }
    const StatusBitPattern& GetStatus() const { return m_status; }
    // (additive) The mouse buttons pressed on this control and still held (bit per
    // button, m_value124; read inline by ButtonControl Mac 0xec190 and SliderAugmentation
    // Mac 0x1cb8d0). (name inferred)
    UInt32 GetPressedButtons() const { return m_value124; }
    // (additive) The input flags (+0x110; read inline by SliderAugmentation Mac
    // 0x1cb660 for SetInputFlags(flags | 1)). (name inferred)
    UInt32 GetInputFlags() const { return m_inputFlags; }
    // (additive) The selection mask (+0x3c; read inline by LuaDragAugmentation
    // Mac 0x2b7270). (name inferred)
    const SelectionMask* GetSelectionMask() const { return m_selectionMask; }
    // Marks the control (and the parents it shows through) for drawing; without `force`, only when
    // the control's rectangle intersects the parent's. Mac 0xdb0d0, Win 0x50a29c (inlined variant
    // Win 0x44b400). `force` is a bool: Mac 0xdb0d0 tests it as a byte; callers pass 0 or an
    // intersection test result.
    void Invalidate(bool force);
    // (ControlManager.cpp) Invalidate(false) on every buffered control of the
    // subtree (m_buffered) unless the manager is drawing. Mac 0xdb1c0, Win 0x50a2e2 (name inferred)
    void InvalidateBuffered();
    // (ControlManager.cpp) The topmost visible control of the subtree at `position`
    // (parent coordinates): children last to first, then IsInside. Mac 0xd8910, Win 0x509d52
    // (thiscall on the control). (name inferred)
    Control* FindControlAt(const Vector2& position);

    // ---- Control.cpp (names and signatures inferred) ----------------------------
    // Points: the control's local coordinates (own units, before its scale) vs. the manager's
    // (window) coordinates.
    // `point` (manager coordinates) into local coordinates, in place: parent chain first, then
    // -= position, *= 1 / scale. Mac 0xe9510
    void ManagerToLocal(Vector2& point) const;
    // `point` (local) into manager coordinates, then into `target`'s local ones when given.
    // Mac 0xe0780, Win 0x5085a9
    Vector2 LocalToControl(const Control* target, const Vector2& point) const;
    // Position of the control's origin in manager coordinates.
    // NOTE: original quirk: only the parent's scale is applied, not the grandparents'.
    // Mac 0xe2390, Win 0x508f60
    Vector2 GetAbsolutePosition() const;
    // `ancestor` is a (transitive) parent. Mac 0xe02a0, Win 0x45a2c0
    bool IsDescendantOf(const Control* ancestor) const;

    // All in one: input flags (| 2 with focus), shown (flags bit 0), enabled (bit 1), active
    // (bit 2), rectangle, hit-testable, parent / sibling order, keyboard focus (bit 3).
    // Mac 0xdf150, Win 0x507eaf
    bool Setup(Control* parent, Control* after, UInt32 flags, const Rect& rectangle,
               UInt32 inputFlags, bool hitTestable);
    // Moves the control below `parent` (null: out of the tree), behind sibling `after` (null: last
    // = topmost; `parent` itself: first). False for a layer root, a cycle, or when the new subtree
    // refuses activation. Mac 0xdd890, Win 0x507fcc
    bool SetParent(Control* parent, Control* after = 0);
    // Order among the siblings (`after` as in SetParent). Mac 0xdf6a0 (→ 0xe00d0), Win 0x508254
    bool SetZOrder(Control* after);
    bool SetShown(bool shown);                                              // Mac 0xdf320, Win 0x508cce
    bool SetEnabled(bool enabled);                                          // Mac 0xdf420, Win 0x508d3f
    // False when OnActivated refused.
    bool SetActive(bool active);                                            // Mac 0xdf470, Win 0x508d74
    void SetHitTestable(bool hitTestable);                                  // Mac 0xdf530, Win 0x508c65
    // Takes (true) or gives up (false) the manager's keyboard focus. Mac 0xdf580, Win 0x508daa
    bool SetKeyboardFocus(bool focus);
    void SetSelectionMask(const SelectionMask* mask);                       // Mac 0xe2030, Win 0x508ca2
    // Sets the input flags (+0x110: bit 0 mouse, bit 1 keyboard, bit 2 buffering); false when
    // OnBufferingEnabled refused.
    bool SetInputFlags(UInt32 flags);                                       // Mac 0xe2090, Win 0x508ded
    // Cursor shape while the mouse is over the control; `flag` (a byte, +0x12c) picks Cursor slot 6
    // (shape) or 7.
    void SetCursor(UInt32 cursor, bool flag);                               // Mac 0xe2330, Win 0x508f2c
    // Draws into an own buffer (input flag 4); false when OnBufferingEnabled refused.
    bool SetBuffered(bool buffered);                                        // Mac 0xe2700, Win 0x508fb5
    // The manager's mouse capture. Mac 0xe2310 (→ ControlManager::SetMouseCapture), Win 0x5079bc
    bool CaptureMouse();
    bool ReleaseMouseCapture();                                             // Mac 0xe22a0
    // (additive) The control holds the manager's mouse capture (+0x70 == this, +0x91
    // set). Win 0x50632a (Mac inlined, e.g. DialControl 0x2a49a0); inline in ControlManager.h.
    // (name inferred)
    bool HasMouseCapture() const;
    // The named augmentation into `augmentation`; false when there is none. Mac 0xdf5d0, Win 0x507f67
    bool GetAugmentation(const String& name, RefPointer<ControlAugmentation>& augmentation) const;

    // ---- rectangle (all refuse with false on a layer root) ----
    // Applies the anchors, rounds and clamps; notifies OnRectangleChanged on a change, else
    // (relayoutDependents) lays out the controls anchored to this one. Mac 0xdfe40, Win 0x50a527
    void SetRectangle(const Vector2& position, const Vector2& size, bool relayoutDependents);
    bool SetRectangle(const Rect& rectangle);                               // Mac 0xdf4c0, Win 0x50831a
    bool SetPosition(const Vector2& position);                              // Mac 0xe02d0, Win 0x508292
    bool SetSize(const Vector2& size);                                      // Mac 0xe0320, Win 0x5082d6
    // Both components > 1e-5. Mac 0xe0370, Win 0x50837f
    bool SetScale(const Vector2& scale);
    bool SetMinimumSize(const Vector2& size);                               // Mac 0xe04c0, Win 0x508415
    bool SetMaximumSize(const Vector2& size);                               // Mac 0xe0610, Win 0x5084dd
    const Vector2& GetMinimumSize() const { return m_minimumSize; }
    const Vector2& GetMaximumSize() const { return m_maximumSize; }
    // Moves the control back inside the parent's rectangle. Mac 0xe0820, Win 0x5085eb
    bool MoveInsideParent();
    // Resizes (and moves) the control to the children's bounding rectangle on the chosen sides,
    // moving the children by the opposite amount. Mac 0xe0a40, Win 0x508718
    void FitToChildren(bool top, bool left, bool bottom, bool right);
    // The same, only on the sides where the children stick out. Mac 0xe0ed0, Win 0x508796
    void GrowToChildren(bool top, bool left, bool bottom, bool right);
    // The same with margins (null: that side keeps the control's edge). Mac 0xe0fd0
    void FitToChildren(const Real32* top, const Real32* left, const Real32* bottom, const Real32* right);

    // ---- anchors (axis 0 = horizontal, 1 = vertical; alignment 0 none, 1 start, 2 centre,
    // 3 end; a null reference = the parent) ----
    // 0 for 0 and out of range, 0 / 0.5 / 1 for 1 / 2 / 3. Win 0x5089f3 (Mac: table 0x631a58)
    static Real32 AlignmentToFraction(UInt32 alignment);
    InternalAnchorData* GetAnchor(UInt32 axis, bool nearEdge);              // Mac 0xe1cc0
    // Mac 0xe1070 (Win 0x508856)
    void SetAxisAnchors(UInt32 axis, Real32 v0, Real32 v1, Real32 v2, Real32 v3, Real32 v4,
                        Control* reference0, Real32 w0, Real32 w1, Real32 w2, Real32 w3,
                        Real32 w4, Control* reference1);
    // The four anchors (horizontal near, far, vertical near, far). Mac 0xe1240, Win 0x5088f4
    void SetAnchors(Real32 a0, Real32 a1, Real32 a2, Real32 a3, Real32 a4, Control* reference0,
                    Real32 b0, Real32 b1, Real32 b2, Real32 b3, Real32 b4, Control* reference1,
                    Real32 c0, Real32 c1, Real32 c2, Real32 c3, Real32 c4, Control* reference2,
                    Real32 d0, Real32 d1, Real32 d2, Real32 d3, Real32 d4, Control* reference3);
    // Copies of the given anchors (null: keep). Mac 0xe1cf0, Win 0x508bde
    void SetAnchors(const InternalAnchorData* horizontalNear, const InternalAnchorData* horizontalFar,
                    const InternalAnchorData* verticalNear, const InternalAnchorData* verticalFar);
    // One aligned anchor on `axis` (the far one off). Mac 0xe1540, Win 0x508a09
    void SetAxisAlignment(UInt32 axis, UInt32 alignment, UInt32 referenceAlignment, Real32 offset,
                          Control* reference);
    // Both anchors of `axis` aligned. Mac 0xe16a0, Win 0x508a4e
    void SetAxisAlignments(UInt32 axis, UInt32 alignment0, UInt32 referenceAlignment0,
                           Real32 offset0, Control* reference0, UInt32 alignment1,
                           UInt32 referenceAlignment1, Real32 offset1, Control* reference1);
    // One aligned anchor per axis: alignments in bits 0-1 (horizontal) and 4-5 (vertical).
    // Mac 0xe18d0, Win 0x508ac3
    void SetAlignment(UInt32 alignment, UInt32 referenceAlignment, Real32 offsetX, Real32 offsetY,
                      Control* reference);
    // Stretches between the reference's edges with the given margins. Mac 0xe1b30, Win 0x508b28
    void SetMargins(Real32 left, Real32 right, Real32 top, Real32 bottom, Control* reference);
    // The same on one axis. Mac 0xe1c10, Win 0x508bb3
    void SetAxisMargins(UInt32 axis, Real32 nearMargin, Real32 farMargin, Control* reference);

    // Recursive draw of the subtree: children first, then a buffered control draws itself
    // (BeginDraw, Draw) and its unbuffered descendants into its buffer. Mac 0xe2420
    void DrawTree(ControlDrawArgs& args);
    bool AddAugmentation(const String& name, ControlAugmentation* augmentation); // Mac 0xe76b0, Win 0x509b30
    void RemoveAugmentation(const String& name);                            // Mac 0xe7790, Win 0x509bb1

protected:
    friend class ControlManager;

    // ---- Control.cpp helpers (names inferred) ----
    // Re-registers the anchor references (m_anchorSources / their m_dependents, common
    // ancestors) and optionally lays out. Mac 0xdf6c0, Win 0x50a390
    void UpdateAnchors(bool layout);
    // refChanged: UpdateAnchors(true), else changed: SetRectangle(position, size). Win 0x50a9f7
    void ApplyAnchorChanges(bool referenceChanged, bool changed);
    // Position / size of `axis` from the anchors, rounded, size clamped. Mac 0xe81f0, Win 0x50a6a9
    void LayoutAxis(UInt32 axis, Real32 position, Real32 size);
    // The anchor's reference point in the coordinates of this control's parent.
    // Mac 0xe8600, Win 0x50a65f
    Real32 GetAnchorPosition(const InternalAnchorData& anchor, UInt32 axis) const;
    // `dependent` anchors to this control (once). Mac 0xe7ac0 (Win inlined)
    void AddDependent(Control* dependent);
    // Status propagation down the subtree (bits 1, 6, 4, 2). Mac 0xdfa70 / 0xdfb80 / 0xdfad0 /
    // 0xdfbe0 / 0xdfca0, Win 0x509ce0 / 0x509d14 / 0x509f36 / 0x509ebf / 0x509dda
    void Connect();
    void Disconnect();
    bool UpdateActive();
    void UpdateEnabled();
    void UpdateVisible();
    // The control stops being the manager's mouse control (leave notifications). (inlined)
    void LoseMouse();
    // Sibling order without the layer-root check. Mac 0xe00d0, Win 0x509bea
    bool DoSetZOrder(Control* after);
    // Unlinks from the parent's child list (inlined in several places).
    void Unlink();
    // Links below m_parent behind `after` (SetParent semantics; inlined).
    void Link(Control* after);
    // Bounding rectangle of the children; false without children. Win 0x50aa21 (Mac inlined)
    bool GetChildrenRect(Rect& rectangle) const;
    // Mac 0xe0b10, Win 0x50aa6b
    void FitToRect(Rect& rectangle, const Real32* top, const Real32* left, const Real32* bottom,
                   const Real32* right);
    // Draws the children into the current buffer (unbuffered: Draw + recursion, buffered: EndDraw
    // = their buffer). Mac 0xe7880
    void DrawChildren(ControlDrawArgs& args);

    // ---- data (Mac offset / Win offset); Notifier<ControlObserver> occupies +0x00..+0x24 ----
    ControlManager*                 m_manager;          // +0x024 / +0x028
    Control*                        m_parent;           // +0x028 / +0x02c
    Control*                        m_previousSibling;  // +0x02c (inferred)
    Control*                        m_nextSibling;      // +0x030 (sibling walk Mac 0xdfa70)
    Control*                        m_firstChild;       // +0x034 (child walk Mac 0xdfa70)
    Control*                        m_lastChild;        // +0x038 (inferred)
    RefPointer<const SelectionMask> m_selectionMask;    // +0x03c (IsInside)
    Vector2                         m_position;         // +0x040
    Vector2                         m_size;             // +0x048 / +0x04c
    Vector2                         m_scale;            // +0x050 (1, 1)
    Vector2                         m_minimumSize;      // +0x058 (0, 0)
    Vector2                         m_maximumSize;      // +0x060 (32768, 32768)
    UInt32                          m_value68;          // +0x068 0; mark of the common-ancestor search (UpdateAnchors)
    AnchorAxisData                  m_anchors[2];       // +0x06c horizontal, vertical (inferred)
    BasicArray<Control*>            m_controlsEC;       // +0x0ec controls whose layout this one depends on (anchor sources)
    BasicArray<Control*>            m_controlsFC;       // +0x0fc controls anchored to this one (dependents)
    StatusBitPattern                m_status;           // +0x10c / +0x110
    UInt32                          m_inputFlags;       // +0x110 (SetInputFlags)
    String                          m_name;             // +0x114
    UInt32                          m_value124;         // +0x124 0, cleared when focus / capture end
    UInt32                          m_cursor;           // +0x128 1 (cursor shape for the Cursor)
    bool                            m_flag12c;          // +0x12c (meaning unknown)
    bool                            m_buffered;         // +0x12d OnBufferingEnabled accepted
    BasicMap<String, RefPointer<ControlAugmentation> > m_augmentations; // +0x130
    BasicList<ControlObserver*>     m_observerList;     // +0x14c (meaning unknown)
                                                        // Object +0x158 / +0x170

private:
    Control(const Control&);
    Control& operator=(const Control&);
};

} // namespace LEGO
