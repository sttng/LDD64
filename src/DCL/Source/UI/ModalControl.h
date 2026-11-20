// LEGO Digital Designer — restored source
// LEGO::ModalControl — a modal dialog: a translucent grey full-window control (layer 1) with a
// centred LDDWindowControl; while shown it is the manager's modal control and handles Return /
// Space (accept, with a default button) and Escape (cancel, with a cancel button). Base of
// LDDAlertBox, ProgressBarDialog, UploadModelDialog, the preference dialogs …
// TU 125 "ModalControl.cpp" (Mac 0x1db100-0x1dc2bb; Win 0x64fa02-0x64fe0d). Names inferred.
//
// Layout (Mac): RenderControl +0x000..+0x1ec, ControlManagerObserver +0x1ec (vptr, subject
//   +0x1f0), members below, Object +0x204 (sizeof 0x20c). Win: ControlManagerObserver +0x208,
//   default button +0x214, cancel button +0x218, previous focus +0x21c, window +0x220.
// Virtual slots: Mac 0x70d4cc (49 = RenderControl's 45 + ShowModal 45, EndModal 46, OnAccept 47,
//   OnCancel 48), overriding OnKeyPressed (28) and Initialize (38); Win 0x9d5a54 (44 = 40 + 4).
//   The OnKeyPressed overrider also overrides ControlManagerObserver's key handler (one function
//   in the original: thunk Mac 0x1dc1b0, Win 0x5fd347).
#pragma once
#include "LEGO/Types.h"
#include "RenderControl.h"
#include "ControlManager.h"

namespace LEGO {

class LDDWindowControl;
class String;

class ModalControl : public RenderControl, public ControlManagerObserver {
public:
    explicit ModalControl(ControlManager* manager);                // Mac 0x1db100 (C2)
    virtual ~ModalControl();                                       // Mac 0x1dbaa0/0x1dbcc0 (body 0x1db920), Win sdd 0x64fb7e

    // Grey translucent full-window control (hidden, enabled, active) and its window: an
    // LDDWindowControl (300 x 200, not resizable, centred) with a transparent RenderControl content.
    // Mac 0x1db1b0, Win 0x64fba0 (slot 38 / 35)
    virtual bool Initialize();

    // Return / Space: OnAccept (with a default button), Escape: OnCancel (with a cancel button).
    // Nothing else (Control's handler is not called). Mac 0x1dc160, Win 0x64fb44 (slot 28 / 25)
    virtual void OnKeyPressed(UInt32 key, UInt32 modifiers, bool repeat);
    // The same for the keys the manager sees (thunk Mac 0x1dc1b0, Win 0x5fd347).
    virtual void OnManagerKeyPressed(UInt32 key, UInt32 modifiers, bool repeat);

    // ---- new virtuals (Mac slot / Win slot) ----
    // Remembers the keyboard focus, moves into layer 1, shows, watches the manager and becomes
    // its modal control. Mac 0x1db500, Win 0x64fdaf
    virtual void ShowModal();                                      // 45/40
    // The reverse; the remembered control gets the focus back. Mac 0x1db800, Win 0x64fd58
    virtual void EndModal();                                       // 46/41
    virtual void OnAccept();                                       // 47/42 Mac 0x1dc200, Win 0x64f950
    virtual void OnCancel();                                       // 48/43 Mac 0x1dc220, Win 0x64f950 (ICF)

    // The window's title. Mac 0x1db4d0, Win 0x64fb23
    bool SetTitle(const String& title);
    // The window's content control. Mac 0x1db900 (Win inlined)
    Control* GetContent() const;
    LDDWindowControl* GetWindowControl() const { return m_window; }

protected:
    // Leaves the manager, then RenderControl's part. Mac 0x1dbf30 (thunks 0x1dbfa0 / 0x1dc010 /
    // 0x1dc080 / 0x1dc0f0)
    virtual void OnFinalRelease();

    // ---- data (Mac / Win offsets) ----
    RefPointer<Control>          m_defaultButton;    // +0x1f4 / +0x214 (set by subclasses)
    RefPointer<Control>          m_cancelButton;     // +0x1f8 / +0x218 (set by subclasses)
    RefPointer<Control>          m_previousFocus;    // +0x1fc / +0x21c ShowModal .. EndModal
    RefPointer<LDDWindowControl> m_window;           // +0x200 / +0x220
                                                     // Object +0x204
};

} // namespace LEGO
