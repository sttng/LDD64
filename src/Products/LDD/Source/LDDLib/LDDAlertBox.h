// LEGO Digital Designer — restored source
// LEGO::LDDAlertBox — LDD's own alert dialog (a ModalControl): an icon of
//   "Graphics/General/Popup final.png" (by alert style), a title (font 1) and a message (font
//   7), up to three LDD buttons (the first is the default, the second or else the first the
//   cancel button) and an optional "do not show again" check box. LDDApplication keeps one
//   (+0x9c) and routes Application::Alert through it once it exists; Run shows it modally
//   (Application::RunModal) and returns the button pressed.
//
// Home TU 064 "LDDAlertBox.cpp" (Mac 0x150650-0x1523cb; Win 0x64fe15-0x6507ff). The constructor
// is inline: its only out-of-line copy is emitted in LDDApplication.cpp (Mac 0x173180 C2-style
// with the VTT 0x7021ec; Win inlined into LoadAssets 0x602179).
// Mac layout (constructor 0x173180): ModalControl +0x000..+0x204, ControlObserver +0x204
//   (vptr, BasicArray<Control*> subjects +0x208), result +0x218 (not initialised), title +0x21c,
//   icon +0x220, message +0x224, buttons +0x228 / +0x22c / +0x230, check box +0x234 (its box
//   +0x238, its label +0x23c), Object +0x240; sizeof 0x248 (operator new in LoadAssets).
// Mac vtable 0x70d1ec: ModalControl's 49 with D1 0x1515f0 / D0 0x151650, OnFinalRelease 0x1516c0
//   (2), Initialize 0x150650 (38), OnAccept 0x151580 / OnCancel 0x1515b0 (47/48) overridden;
//   Run (49, 0x151030) and the Mac-only primary entry 50 (0x1514d0) of its
//   ControlObserver::OnMouseClicked overrider (thunk 0x151530).
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "ModalControl.h"
#include "Control.h"
#include "RenderControl.h"
#include "ToggleControl.h"
#include "LDDButtonControl.h"

namespace LEGO {

class LDDAlertBox : public ModalControl, public ControlObserver {
public:
    // Mac 0x173180 (inline; emitted in LDDApplication.cpp)
    explicit LDDAlertBox(ControlManager* manager)
        : ModalControl(manager),
          m_title(0), m_icon(0), m_message(0), m_button1(0), m_button2(0), m_button3(0),
          m_checkBox(0), m_checkBoxImage(0), m_checkBoxLabel(0),
          // NOTE: the result is not initialised by the original: a modal loop that ends
          // without a button press returns garbage; fixed: 0, the first button (low)
          m_result(0)
    {}
    virtual ~LDDAlertBox();                                                  // Mac 0x1515f0/0x151650, Win sdd 0x5fd92a

    // ModalControl::Initialize, then the content (390 x 200): icon at (10, 10), title text
    // (79, 10)-(width - 10, 31), message text at (79, 55), three LDD buttons (bottom right,
    // second left of the first, bottom left; observed) and the hidden check box (bottom left).
    // False when a part fails or the texture is missing. Mac 0x150650 (slot 38), Win 0x6503f8
    virtual bool Initialize();

    // Return / Space: result 0; Escape: result 1 when the cancel button is the second button
    // (else 0); then ModalControl's and Application::StopModal.
    virtual void OnAccept();                                                 // 47 Mac 0x151580, Win 0x64fe15
    virtual void OnCancel();                                                 // 48 Mac 0x1515b0, Win 0x650201

    // Shows the alert modally and returns the button pressed (0 = button1 …). `style` picks the
    // icon (0 warning, 1, 2, 3; others none); empty button texts hide buttons 2 / 3;
    // `dontShowAgain` (may be null) is shown as the check box with `checkBoxText` (when that
    // is not empty) and receives its state. (signature from the calls Mac 0x177c80 / 0x177d50)
    virtual UInt32 Run(UInt32 style, const String& message, const String& title,
                       const String& button1, const String& button2, const String& button3,
                       bool* dontShowAgain, const String& checkBoxText);    // 49 Mac 0x151030, Win 0x64fe2d

    // ---- ControlObserver: a button was clicked (any mouse button) -> result, StopModal ----
    virtual void OnMouseClicked(Control* control, const Vector2& position, UInt32 button); // 50 Mac 0x1514d0 (thunk 0x151530), Win 0x6501c9
    using ModalControl::OnMouseClicked;

protected:
    // Leaves the watched buttons, then ModalControl's part.
    virtual void OnFinalRelease();                                           // 2 Mac 0x1516c0

    // LDDButtonControl::Create("", content, (0, 0)) and observes the button. Mac 0x150f70
    bool CreateButton(LDDButtonControl* button);

    UInt32                     m_result;          // +0x218 index of the pressed button
    RefPointer<RenderControl>  m_title;           // +0x21c
    RefPointer<RenderControl>  m_icon;            // +0x220
    RefPointer<RenderControl>  m_message;         // +0x224
    RefPointer<LDDButtonControl> m_button1;       // +0x228 (default)
    RefPointer<LDDButtonControl> m_button2;       // +0x22c
    RefPointer<LDDButtonControl> m_button3;       // +0x230
    RefPointer<ToggleControl>  m_checkBox;        // +0x234
    RefPointer<RenderControl>  m_checkBoxImage;   // +0x238
    RefPointer<RenderControl>  m_checkBoxLabel;   // +0x23c
                                                  // Object +0x240
};

} // namespace LEGO
