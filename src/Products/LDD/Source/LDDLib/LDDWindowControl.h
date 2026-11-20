// LEGO Digital Designer — restored source
// LEGO::LDDWindowControl — the LDD-styled WindowControl: its frame is a 9-quad vertex buffer
//   textured from "Graphics/LDDgraphics01.png" (black title with a blue shadow while active),
//   optional minimize / restore buttons in the title bar (with the application's click sound),
//   and a minimum frame size kept when the content asks for a size. Used for ModalControl (the
//   LDD dialogs) and the Lua console window.
//
// TU 071 "LDDWindowControl.cpp" (Mac 0x162010-0x16412b, Win 0x606ebe-0x608090). The TU also
// holds the weak copies of ControlAugmentationControlObserver (header-only, Mac 0x163740-
// 0x163990) and BasicArray<Control*> helpers. The constructor is inline (Mac 0x13f680 emitted in
// TU 060, Win 0x5c8d52); here it is defined out of line in LDDWindowControl.cpp.
// Vtables: Mac 0x710ac8 (49 entries: WindowControl's 47 with OnResized 38, OnWindowActivated 39,
//   OnWindowDeactivated 40, OnMinimized 44, OnRestored 45, UpdateCursor 46 overridden + Create 47
//   + the Mac-only primary entry 48 of its ControlObserver::OnMouseClicked overrider 0x162f30 /
//   thunk 0x162f60); Win 0x9c9c8c (42 = WindowControl's 41 + Create 41; overrides Win 35-40).
// Mac layout: WindowControl +0x000..+0x200, minimize button +0x200, restore button +0x204,
//   minimum frame size +0x208, Object +0x210 (sizeof 0x218). Win: buttons +0x21c / +0x220, size
//   +0x224 (Object at the end).
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Geometry.h"
#include "WindowControl.h"
#include "ButtonControl.h"

namespace LEGO {

class LDDWindowControl : public WindowControl {
public:
    // Mac 0x13f680 (inline, emitted in TU 060), Win 0x5c8d52
    explicit LDDWindowControl(ControlManager* manager);
    virtual ~LDDWindowControl();                                    // Mac 0x163bb0/0x163c10, Win sdd 0x5c96b7

    // ---- WindowControl overrides (Mac slot / Win slot) ----
    // The frame positions for the new size; title-bar buttons at the right end; the title's
    // right edge (at least its left edge).
    virtual void OnResized(const Vector2& oldSize);                 // 38/35 Mac 0x162fa0, Win 0x606ef4
    // The title shadow follows the activation.
    virtual void OnWindowActivated();                               // 39/36 Mac 0x163490, Win 0x606cac
    virtual void OnWindowDeactivated();                             // 40/37 Mac 0x1634a0, Win 0x606cb4
    // Minimize button hidden, restore button shown (and back).
    virtual void OnMinimized();                                     // 44/38 Mac 0x1634b0, Win 0x6072e0
    virtual void OnRestored();                                      // 45/39 Mac 0x1634e0, Win 0x6072ff
    // Resize cursors of the drag mode or the frame part under `position` (only while resizable).
    virtual void UpdateCursor(const Vector2& position);             // 46/40 Mac 0x163510, Win 0x606cbc

    // ---- new ----
    // WindowControl::Initialize, keyboard input off, title rectangle / colours / margins /
    // minimum size, the frame texture and its vertex buffer (opaque when the texture format is
    // 0); with `titleButtons` the minimize / restore buttons (with click sounds when
    // `buttonSounds`). ModalControl passes (false, false), the Lua console (true, false).
    // Mac 0x162010 (slot 47), Win 0x607777 (slot 41)
    virtual bool Create(bool titleButtons, bool buttonSounds);

    // ---- ControlObserver: the title-bar buttons (left button) ----
    virtual void OnMouseClicked(Control* control, const Vector2& position, UInt32 button); // 48/– Mac 0x162f30 (thunk 0x162f60), Win 0x606ebe
    using Control::OnMouseClicked;

    // The frame size for a content of `size` (margins added, at least the minimum frame size);
    // grows the window when it is smaller and makes it the drag minimum. Mac 0x162e60, Win 0x606df4
    void SetContentSize(const Vector2& size);

protected:
    // The minimize (+0x200) and restore (+0x204) buttons: 13 x 13 four-state images of the frame
    // texture (row 0 / row 32, columns 52, 68, 84, 100), set up hidden-able at (0, 3), observed.
    // False when a button fails to initialize. Mac 0x162620, Win 0x60731e
    bool CreateTitleButtons(bool buttonSounds);

    RefPointer<ButtonControl> m_minimizeButton;    // Mac +0x200, Win +0x21c
    RefPointer<ButtonControl> m_restoreButton;     // Mac +0x204, Win +0x220
    Vector2                   m_minimumFrameSize;  // Mac +0x208, Win +0x224 (30 x 28)
                                                   // Object Mac +0x210
};

} // namespace LEGO
