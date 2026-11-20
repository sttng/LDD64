// LEGO Digital Designer — restored source
// LEGO::ProgressBarDialog — modal dialog with an icon, a text, a progress bar and a cancel
//   button; LDDApplication (+0xa0) shows it while a model loads ("LOADINGMODEL" /
//   "IMPORTINGMODEL") and polls IsCancelled. The dialog never closes itself: Return / Space /
//   Escape and the button only set the cancelled flag; the owner calls EndModal.
//
// TU 122 "ProgressBarDialog.cpp" (Mac 0x1d9000-0x1da3bb; Win 0x5fd572-0x5fd94c,
// 0x6508f5-0x650d23). The constructor is inline; its out-of-line copy is emitted in
// LDDApplication.cpp (Mac 0x1732f0, VTT 0x7021f0).
// Mac layout (constructor 0x1732f0): ModalControl +0x000..+0x204, ControlObserver +0x204
//   (vptr, subjects +0x208), text +0x218, icon +0x21c, progress bar +0x220, cancelled +0x224,
//   Object +0x228; sizeof 0x230. Win: text +0x23c, icon +0x240, progress bar +0x244,
//   cancelled +0x248, Object +0x250 (sdd 0x5fd94c: this - 0x250).
// Mac vtable 0x7220dc (53 slots): ModalControl's 49 overriding OnFinalRelease (2, 0x1d9a30),
//   Initialize (38, 0x1d9000), OnAccept (47, 0x1d9940), OnCancel (48, 0x1d9950); new
//   SetMessage 49 (0x1d97a0), SetProgress 50 (0x1d98c0), IsCancelled 51 (0x1d9aa0), and slot 52
//   = ControlObserver::OnMouseClicked (0x1d9900; thunk 0x1d9920 in the ControlObserver-in
//   vtable 0x722264 slot 22). Win: SetMessage 0x6508f5, SetProgress 0x6509ed, IsCancelled
//   0x5fd572 (ICF), OnMouseClicked thunk 0x650a0d, Initialize 0x650abc.
// Method names are inferred (from LDDApplication's calls Mac 0x1765c0 / 0x178c80); member
// names inferred.
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "ModalControl.h"
#include "Control.h"
#include "RenderControl.h"
#include "ProgressBarControl.h"

namespace LEGO {

class LDDButtonControl;

class ProgressBarDialog : public ModalControl, public ControlObserver {
public:
    // Mac 0x1732f0 (inline; emitted in LDDApplication.cpp)
    explicit ProgressBarDialog(ControlManager* manager)
        : ModalControl(manager), m_cancelled(false) {}
    // Mac 0x1d9960 (D1, body 0x1da1a0 with the VTT), 0x1d99c0 (D0), thunks 0x1d9ab0-0x1da0a0;
    // Win sdd 0x5fd94c
    virtual ~ProgressBarDialog();

    // ModalControl::Initialize, then the icon ("Graphics/General/Popup final.png" cut at
    // (453, 63) 59 x 59, at (10, 10) of the window content), the progress bar, the text
    // (right of the icon, as wide as the bar; font 1, transparent) and the cancel button
    // (LDDButtonControl, "CANCEL", also the default button). Mac 0x1d9000, Win 0x650abc
    virtual bool Initialize();                                               // 38

    // Return / Space / Escape: cancelled (the dialog stays up). Mac 0x1d9940 / 0x1d9950
    virtual void OnAccept();                                                 // 47
    virtual void OnCancel();                                                 // 48

    // Clears the cancelled flag, sets the text (fitted), puts the bar 4 below it, sizes the
    // content around everything and shows the dialog modally. Mac 0x1d97a0, Win 0x6508f5
    virtual void SetMessage(const String& message);                          // 49
    // `value` of `maximum` (LDDApplication passes the loader's percentage and 100).
    // Mac 0x1d98c0, Win 0x6509ed
    virtual void SetProgress(Real32 value, Real32 maximum);                  // 50
    virtual bool IsCancelled() const;                                        // 51 Mac 0x1d9aa0, Win 0x5fd572

    // --- ControlObserver: a click on the cancel button cancels. Mac 0x1d9900 (thunk
    //     0x1d9920), Win 0x650a0d
    virtual void OnMouseClicked(Control* control, const Vector2& position, UInt32 button); // 52
    using ModalControl::OnMouseClicked;   // Control::OnMouseClicked(position, button) stays visible

protected:
    // Leaves the watched controls, then ModalControl's part. Mac 0x1d9a30 (thunks 0x1d9ba0 /
    // 0x1d9d00 / 0x1d9e60 / 0x1d9fc0 / 0x1da120)
    virtual void OnFinalRelease();

    // Creates `button` (no text) in the content, watches it and anchors it to the bottom
    // right (10, 10). Mac 0x1d96b0, Win 0x650a1d (name inferred)
    bool AddButton(LDDButtonControl* button);

    // ---- data (Mac / Win offsets) ----
    RefPointer<RenderControl>      m_text;          // +0x218 / +0x23c
    RefPointer<RenderControl>      m_icon;          // +0x21c / +0x240
    RefPointer<ProgressBarControl> m_progressBar;   // +0x220 / +0x244
    bool                           m_cancelled;     // +0x224 / +0x248
                                                    // Object +0x228 / +0x250
};

} // namespace LEGO
