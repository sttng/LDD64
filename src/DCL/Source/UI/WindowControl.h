// LEGO Digital Designer — restored source
// LEGO::WindowControl — a floating window inside the main window: a textured frame, a title
// (TextLayoutRender, "title bar" system font, optional shadow), margins around one content
// control; left-button dragging (title bar) and resizing (edges, corners), minimize / restore
// (only the title bar stays), activation (to the front when the focus enters). Base of
// LDDWindowControl, ModalControl …
// TU 029 "WindowControl.cpp" (Mac 0xee970-0xf10fb; Win 0x50fcaa-0x510ab9); also emits the inline
// Font::SystemTypeFontArgs / TextLayout::CreateArgs dtors (Mac 0xefaa0, 0xefad0, 0xf0e30,
// 0xf0e40). Directory UI/ (guess).
//
// Bases: Control (+0), ControlManagerObserver (Mac +0x158, subject +0x15c; Win +0x16c),
//   ControlObserver (Mac +0x160, subjects +0x164; Win +0x178), virtual base Object
//   (Mac +0x200, Win +0x220). Mac ctor 0xee970 (C2 with VTT; no C1 is emitted).
// Virtual slots: Mac vtable 0x707c9c (47 entries), Win 0x9c9afc (41 entries).
//   Overrides: IsInside (3/0, always true), OnRectangleChanged (8/5), OnMouseMove (18/15),
//   OnMousePressed (19/16), OnMouseReleased (20/17), Draw (33/30).
//   New: Initialize (36/33), CreateTitleLayout (37/34), OnResized (38/35), OnWindowActivated
//   (39/36), OnWindowDeactivated (40/37), Mac-only entries 41-43 (the overriders of
//   ControlManagerObserver 13 / 12 and ControlObserver 9), OnMinimized (44/38), OnRestored
//   (45/39), UpdateCursor (46/40).
// Names: "Minimize", "Restore", "OnMinimized", "contentRect" from LuaWindowControl; the rest
// inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "Geometry.h"
#include "Color.h"
#include "Control.h"
#include "ControlManager.h"
#include "ControlDragHelper.h"

namespace LEGO {

class Texture;
class VertexBuffer;
class TextLayoutRender;

class WindowControl : public Control, public ControlManagerObserver, public ControlObserver {
public:
    // Drag modes (m_dragMode): ControlDragHelper's edge bits, kDragMove for the title bar,
    // kDragNone when nothing is dragged. (names inferred)
    enum {
        kDragMove = 0x01,
        kDragNone = 0x31
    };

    // Mac 0xee970 (C2), Win 0x50fce2
    explicit WindowControl(ControlManager* manager);
    virtual ~WindowControl();                                         // Mac 0xf0920/0xf0980, Win 0x50fccc (sdd 0x50fcaa)

    // ---- Control overrides (Mac slot / Win slot) ----
    // Every point is inside (the window takes all clicks on its frame).
    virtual bool IsInside(const Vector2& point) const;                // 3/0   Mac 0xf0aa0, Win 0x51611f
    // Keeps the minimum size (m_dragHelper.m_minimumSize) or, minimized, the title-bar size;
    // OnResized; lays out the content; sets the title's maximum width.
    virtual void OnRectangleChanged(const Vector2& oldPosition, const Vector2& oldSize); // 8/5 Mac 0xefae0, Win 0x510290
    // Drags / resizes while a drag runs, else updates the cursor.
    virtual void OnMouseMove(const Vector2& position);                // 18/15 Mac 0xefd20, Win 0x510423
    // Left button on a frame part: starts dragging it (limits = the parent's rectangle).
    virtual void OnMousePressed(const Vector2& position, UInt32 button);  // 19/16 Mac 0xefe40, Win 0x51048c
    virtual void OnMouseReleased(const Vector2& position, UInt32 button); // 20/17 Mac 0xf0040, Win 0x5104fc
    // White content area (no content), the frame geometry, the title (shadow first).
    virtual void Draw(ControlDrawArgs& args);                         // 33/30 Mac 0xf0160, Win 0x510564
    // (the ControlObserver handlers of the same names stay the defaults)
    using ControlObserver::OnMouseMove;
    using ControlObserver::OnMousePressed;
    using ControlObserver::OnMouseReleased;

    // ---- new virtuals ----
    // Mouse + keyboard input, then observes the manager (keyboard focus, mouse over).
    virtual bool Initialize();                                        // 36/33 Mac 0xeec70, Win 0x50fead
    // The title layout: "title bar" system font, truncated, maximum width = title rect width.
    // Returns a new layout (unreferenced) or null.
    virtual TextLayoutRender* CreateTitleLayout();                    // 37/34 Mac 0xef830, Win 0x510118
    // The size changed (not called for the minimum-size corrections). Empty.
    virtual void OnResized(const Vector2& oldSize);                   // 38/35 Mac 0xf0ab0, Win 0x50fca7
    // The keyboard focus entered / left the window. Empty.
    virtual void OnWindowActivated();                                 // 39/36 Mac 0xf0ac0, Win 0x512edf
    virtual void OnWindowDeactivated();                               // 40/37 Mac 0xf0ad0, Win 0x512edf
    // ---- overriders of the observer bases (Mac primary entries 41-43) ----
    // Activation follows the keyboard focus; focus on the window goes to the content.
    virtual void OnManagerKeyboardFocusChanged(bool flag, Control* previous, Control* current,
                                               UInt32 modifiers);     // 41/– Mac 0xf04d0 (thunk 0xf0590), Win 0x5107ee
    // The mouse left the window: any drag ends.
    virtual void OnManagerMouseOverChanged(Control* previous, Control* current); // 42/– Mac 0xf05c0 (thunk 0xf05e0), Win 0x510874
    // The content resized itself: the window follows (not minimized).
    virtual void OnRectangleChanged(Control* control, const Vector2& oldPosition,
                                    const Vector2& oldSize);          // 43/– Mac 0xf0600 (thunk 0xf0680), Win 0x51088d
    // Empty.
    virtual void OnMinimized();                                       // 44/38 Mac 0xf0ae0, Win 0x512edf
    virtual void OnRestored();                                        // 45/39 Mac 0xf0af0, Win 0x512edf
    // Cursor of the drag mode, or of the frame part under `position`.
    virtual void UpdateCursor(const Vector2& position);               // 46/40 Mac 0xf0700, Win 0x510ab9

    // ---- non-virtual ----
    // The content control (shown, enabled, active; first child; laid out into the margins).
    // NOTE: original quirk: the previous content stays observed (Detach is called after the pointer
    // was cleared).
    void SetContent(Control* content);                                // Mac 0xeef10, Win 0x50fed1
    Control* GetContent() const { return m_content; }
    // The title text (the layout is created on demand); false without a layout.
    bool SetTitle(const String& title);                               // Mac 0xef5e0, Win 0x50ff6a
    // Resizing with the mouse (kept while minimized, applied on restore).
    void SetResizable(bool resizable);                                // Mac 0xef670, Win 0x50ffa2
    void Minimize();                                                  // Mac 0xef690, Win 0x50ffbe
    void Restore();                                                   // Mac 0xef760, Win 0x51006b
    bool IsMinimized() const { return m_minimized; }
    // The frame part at `position` (local coordinates): kDragNone outside the window and in the
    // content area, kDragMove on the title bar, else the edge bits. Mac 0xefee0, Win 0x5108e4
    UInt32 HitTest(const Vector2& position) const;

protected:
    // Observers detached, then Control::OnFinalRelease.
    virtual void OnFinalRelease();                                    // 2/– Mac 0xf09f0

    // The content rectangle from the margins; the content is not observed meanwhile.
    // Win 0x510b55 (inlined on Mac)
    void LayoutContent();
    // Size of the window around the content (title bar only when `minimized`).
    Vector2 GetFrameSize(bool minimized) const;
    // The content's top: max(top margin, title bar height).
    Real32 GetContentTop() const;

    // ---- data (Mac offset / Win offset) ----
    RefPointer<Control>          m_content;           // +0x174 / +0x190
    bool                         m_active;            // +0x178 / +0x194 the focus is inside
    RefPointer<Texture>          m_frameTexture;      // +0x17c / +0x198 (set by subclasses)
    RefPointer<VertexBuffer>     m_frameBuffer;       // +0x180 / +0x19c
    bool                         m_frameOpaque;       // +0x184 / +0x1a0 true: drawn without blending
    RefPointer<TextLayoutRender> m_titleLayout;       // +0x188 / +0x1a4
    Rect                         m_titleRect;         // +0x18c / +0x1a8 area of the title text
    Color                        m_titleColor;        // +0x19c / +0x1b8 white
    Color                        m_titleShadowColor;  // +0x1ac / +0x1c8 white
    bool                         m_titleShadow;       // +0x1bc / +0x1d8
    bool                         m_minimized;         // +0x1bd / +0x1d9
    bool                         m_resizable;         // +0x1be / +0x1da (applied to m_dragHelper)
    Real32                       m_marginLeft;        // +0x1c0 / +0x1dc
    Real32                       m_marginTop;         // +0x1c4 / +0x1e0
    Real32                       m_marginRight;       // +0x1c8 / +0x1e4
    Real32                       m_marginBottom;      // +0x1cc / +0x1e8
    Real32                       m_titleBarHeight;    // +0x1d0 / +0x1ec
    Real32                       m_borderWidth;       // +0x1d4 / +0x1f0 corner size of the resize edges
    UInt32                       m_dragMode;          // +0x1d8 / +0x1f4 kDragNone
    ControlDragHelper            m_dragHelper;        // +0x1dc / +0x1f8 (minimum size m_minimumSize;
                                                      //   resizable, movable, limited = true)
                                                      // Object +0x200 / +0x220
};

} // namespace LEGO
