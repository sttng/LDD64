// LEGO Digital Designer — restored source
// LEGO::UploadModelDialog — the modal "upload model" dialog of UploadModelCommand ('BUMD' in the
//   ObjectRegistry): an icon, a title ("UPLOADMODELTITLE"), a text ("UPLOADMODEL") and a Cancel
//   button. While shown it runs a small state machine on the main updater: the model document
//   is saved into the "memory" directory, uploaded with a picture of the scene
//   (UploadModelCaller), the reply's result URL is resolved (GetRedirectURLCaller) and opened in
//   the browser; Cancel stops the running call.
//
// TU 128 "UploadModelDialog.cpp" (Mac 0x1e5d30-0x1e86fb; Win 0x694d75-0x695a70). The TU also
// emits the inline constructors of UploadModelCaller (Mac 0x1e7990) and GetRedirectURLCaller
// (Mac 0x1e7a70) and BasicArray<Control*> (ObserverN, Mac 0x1e8390). Static initialiser Mac
// 0x1e8680: <iostream> + a header static.
// RTTI (Mac): vmi {ModalControl +0, UpdateObserver +0x204, ControlObserver +0x20c}, virtual base
//   Object (+0x244, sizeof 0x24c). Win: UpdateObserver +0x224, ControlObserver +0x230 (?),
//   members from +0x244 (title) / +0x258 (scene control) / +0x264 (state).
// Mac vtable 0x7234ac (51 entries): ModalControl's 49 with D1 0x1e7d20 / D0 0x1e7d80,
//   OnFinalRelease 2 0x1e7b70, Initialize 38 0x1e5f80, ShowModal 45 0x1e6840, then the primary
//   entries of UpdateObserver::Update 49 (0x1e6b40, thunk 0x1e7b50) and of
//   ControlObserver::OnMouseClicked 50 (0x1e7ce0, thunk 0x1e7d00).
// Win vtable 0x9ddd1c (44 entries): Initialize 35 0x694d97, ShowModal 40 0x695a0c;
//   UpdateObserver-in 0x9ddc9c {2: 0x69529e}; ControlObserver-in 0x9ddc1c {19: 0x694abb};
//   sdd 0x694d75.
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "ModalControl.h"
#include "Control.h"
#include "RenderControl.h"
#include "Updater.h"
#include "LDDButtonControl.h"
#include "UploadModelCaller.h"
#include "GetRedirectURLCaller.h"

namespace LEGO {

class ControlManager;
class BrickConnectSceneControl;

class UploadModelDialog : public ModalControl, public UpdateObserver, public ControlObserver {
public:
    enum { kRegistryId = 0x42554d44 };   // 'BUMD' (UploadToFactoryCommand Mac 0x27a8c0)

    // The states of Update. (names inferred)
    enum State {
        kStateStart     = 0,   // ShowModal: the next update starts the upload
        kStateUpload    = 1,   // save the model, start the upload
        kStateWaiting   = 2,   // a call is running
        kStateFinished  = 3,   // a call finished (also: shown without updates)
        kStateCancelled = 4    // Cancel: stop the running call, close when it is done
    };

    // Mac 0x1e5d30, Win (inline at 0x684f3d)
    UploadModelDialog(ControlManager* manager, BrickConnectSceneControl* sceneControl);
    virtual ~UploadModelDialog();                                            // Mac 0x1e7d20/0x1e7d80, Win sdd 0x694d75

    // ModalControl::Initialize; the icon ("Graphics/General/Popup Final.png" (453, 63) 59 x 59
    // at (10, 10)), the title (79 .. 380 at y 10, wrapped, font 2), the text below it (font 6),
    // the Cancel button (bottom right 10, 10, watched); content 390 wide, at least 79 high.
    // Mac 0x1e5f80, Win 0x694d97
    virtual bool Initialize();                                               // 38/35
    // ModalControl::ShowModal, state Start, watches the main updater. Mac 0x1e6840, Win 0x695a0c
    virtual void ShowModal();                                                // 45/40
    // ModalControl::ShowModal without the updater (state Finished): shown while
    // UploadToFactoryCommand checks for new content. Mac 0x1e67e0 (Win inlined)
    void ShowWaiting();

    // ---- UpdateObserver: the state machine. Mac 0x1e6b40 (thunk 0x1e7b50), Win 0x69529e
    virtual void Update();                                                   // 49
    // ---- ControlObserver: Cancel -> state Cancelled. Mac 0x1e7ce0 (thunk 0x1e7d00),
    //      Win 0x694abb
    virtual void OnMouseClicked(Control* control, const Vector2& position, UInt32 button); // 50
    using ModalControl::OnMouseClicked;

    State GetState() const { return (State)m_state; }

protected:
    // The updater, ModalControl's part, then the watched controls. Mac 0x1e7b70 (thunks
    // 0x1e7c20-0x1e7cc0)
    virtual void OnFinalRelease();

    // ---- data (Mac offsets) ----
    RefPointer<RenderControl>             m_title;          // +0x220
    RefPointer<RenderControl>             m_icon;           // +0x224
    RefPointer<RenderControl>             m_text;           // +0x228
    RefPointer<LDDButtonControl>          m_cancel;         // +0x22c
    RefPointer<BrickConnectSceneControl>  m_sceneControl;   // +0x230 / Win +0x258
    RefPointer<UploadModelCaller>         m_upload;         // +0x234 / +0x25c
    RefPointer<GetRedirectURLCaller>      m_redirect;       // +0x238 / +0x260
    UInt32                                m_state;          // +0x23c / +0x264 (kStateFinished)
    // NOTE (both builds): never set (no writer of +0x240 in either build); "&checkprice=true"
    // would be appended to the result URL.
    bool                                  m_checkPrice;     // +0x240
    bool                                  m_cancelling;     // +0x241
    bool                                  m_redirecting;    // +0x242
                                                            // Object +0x244
};

} // namespace LEGO
