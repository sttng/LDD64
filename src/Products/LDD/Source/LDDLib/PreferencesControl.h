// LEGO Digital Designer — restored source
// LEGO::PreferencesControl — the preferences dialog: a ModalControl with a column of
//   preference rows (Bool / Slider rows, on Windows also the language drop-down), a "changes
//   need a restart" note and OK / Cancel / Reset preferences. Rows store their values as they
//   change; OK and Cancel save preferences.ini (Cancel after reverting). While shown it watches
//   the preferences for the dependent rows and the restart note. LDDApplication creates it in
//   LoadAssets (+0x98).
//
// TU 133 "PreferencesControl.cpp" (Mac 0x1ea100-0x1ee08b; Win 0x650f28-0x653637, the Windows
// language row Win 0x651619-0x651869, 0x651c76, 0x6520b0). The TU also emits the inline
// constructor of PreferenceControlBool (Mac 0x1edc20) and the BasicArray instances of the
// rows / controls / ObserverN subjects. Static initialiser Mac 0x1ee010: <iostream> + a header
// static.
// RTTI (Mac): vmi {ModalControl +0, ControlObserver +0x204, PreferencesObserver +0x218},
//   virtual base Object (+0x264, sizeof 0x26c); constructor Mac 0x1ea170, Win 0x65187f.
// Mac vtable 0x724a0c (51 entries): ModalControl's 49 with D1 0x1ed030 / D0 0x1ed330,
//   OnFinalRelease 2 0x1ed6a0, Initialize 38 0x1ea310, ShowModal 45 0x1ec790, EndModal 46
//   0x1ecac0, OnAccept 47 0x1ed920, OnCancel 48 0x1eda00, then the primary entries of the
//   ControlObserver::OnMouseClicked overrider 49 (0x1ed820, thunk 0x1ed8a0 at ControlObserver-in
//   slot 22) and of PreferencesObserver::OnPreferenceChanged 50 (0x1edae0, thunk 0x1edc00).
// Win vtable 0x9d61bc (44 entries): Initialize 35 0x652371, ShowModal 40 0x6535a5, EndModal 41
//   0x653544, OnAccept 42 0x6515cc, OnCancel 43 0x6515dc; ControlObserver-in 0x9d60cc {19:
//   0x65158b}; PreferencesObserver-in 0x9d60b8 {2: 0x651370}; sdd 0x653522.
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "BasicArray.h"
#include "ModalControl.h"
#include "Control.h"
#include "Preferences.h"
#include "RenderControlManager.h"
#include "PreferenceControl.h"
#include "LDDButtonControl.h"

namespace LEGO {

class RenderControlManager;
class LDDButtonControl;
#if defined(_WIN32)
template <class A, class B> struct Pair;
#endif

class PreferencesControl : public ModalControl, public ControlObserver, public PreferencesObserver {
public:
    // Mac 0x1ea170, Win 0x65187f
    explicit PreferencesControl(RenderControlManager* manager);
    virtual ~PreferencesControl();                                           // Mac 0x1ed030/0x1ed330, Win sdd 0x653522

    // ModalControl::Initialize, then the dialog (title "PREFERENCES"): the check boxes
    // SHOWINFOFIELD, SHOWTOOLTIPS, PLAYSOUND (when the application has sound), SHOWBRICK-
    // REORIENTATION, REPEATPASTE, SHOWBRICKCOUNTS, INVERTEDCAMERAX/Y; a line; the render
    // switches USESHADERSINSCENE, USESHADERSINPALETTE (disabled without shader support),
    // UseRoundEdges (enabled with the round-edge shaders and both shader switches on) and the
    // SelectedPostProcessingEffect slider (one step per effect; disabled without effects /
    // post-processing); a line; (Windows: the LANGUAGE drop-down); BRICKTOOLTIP and
    // ShowExtendedBrickToolTip in qaMode / developerMode; the CompatibilityModeLevel slider;
    // the restart note; OK / Cancel (default / cancel buttons) and Reset preferences.
    // Mac 0x1ea310, Win 0x652371
    virtual bool Initialize();                                               // 38/35
    // Every row remembers its value, the restart note hides, the preferences are watched,
    // then ModalControl::ShowModal. Mac 0x1ec790, Win 0x6535a5
    virtual void ShowModal();                                                // 45/40
    // The preferences are no longer watched, then ModalControl::EndModal. Mac 0x1ecac0,
    // Win 0x653544
    virtual void EndModal();                                                 // 46/41
    // Apply, then ModalControl::OnAccept. Mac 0x1ed920, Win 0x6515cc
    virtual void OnAccept();                                                 // 47/42
    // RevertChanges, then ModalControl::OnCancel. Mac 0x1eda00, Win 0x6515dc
    virtual void OnCancel();                                                 // 48/43

    // ---- ControlObserver: OK -> OnAccept, Cancel -> OnCancel, Reset preferences -> every
    //      row's ResetToDefault. Mac 0x1ed820 (thunk 0x1ed8a0), Win 0x65158b
    virtual void OnMouseClicked(Control* control, const Vector2& position, UInt32 button); // 49
    using ModalControl::OnMouseClicked;
    // ---- PreferencesObserver: the restart note while a changed row needs a restart; the
    //      round-edge row enabled while the round-edge shaders are loaded and both shader rows
    //      are on; the post-processing row enabled while there are effects and the scene
    //      shader row is on. Mac 0x1edae0 (thunk 0x1edc00), Win 0x651370
    virtual void OnPreferenceChanged(const String& key);                     // 50

protected:
    // The watched controls and the preferences are left, then ModalControl's part.
    // Mac 0x1ed6a0 (thunks 0x1ed760-0x1ed800)
    virtual void OnFinalRelease();

    // A check box row `key` / `label` 8 below the last row at x 10 (disabled when
    // `disabled`); the next row starts its height + 8 lower. Mac 0x1ec0b0, Win 0x651b68
    bool AddCheckBox(const String& key, const String& label, Control* parent, bool disabled,
                     bool needsRestart);
    // A separator line (1, y) 421 x 2 from "Graphics/General/Popup Final.png" (135, 201)
    // 338 x 2, 2 below the last row; the next row starts 2 lower. Mac 0x1ec270, Win 0x651ed0
    bool AddSeparator(Control* parent);
    // A slider row at x 45 with side margins 45; see AddCheckBox. Mac 0x1ec580, Win 0x651d90
    bool AddSlider(const String& key, const String& label, Control* parent, Real32 minimum,
                   Real32 maximum, Real32 step, UInt32 style, bool disabled, bool needsRestart);
    // OK: every changed row stores its value; preferences.ini is saved. Mac 0x1ed940,
    // Win 0x651431
    void Apply();
    // Cancel: every changed row shows its remembered value again; preferences.ini is saved.
    // Mac 0x1eda20, Win 0x65149c
    void RevertChanges();
#if defined(_WIN32)
    // (Windows only) The language row: a drop-down of `items` (language code, name) at x 25,
    // 6 below the last row; the next row starts its height + 10 lower. Win 0x651c76
    bool AddDropDown(const String& key, const String& label, Control* parent,
                     BasicArray<Pair<String, String> > items, bool needsRestart);
    // (Windows only) The languages: the codes listed by the "Languages" entry of
    // "Languages.loc" in the localisation directory (Platform directory 9) that have a
    // directory there and a name in the file. Win 0x6520b0
    static BasicArray<Pair<String, String> > GetLanguages();
#endif

    // ---- data (Mac / Win offsets) ----
    BasicArray<RefPointer<PreferenceControl> > m_rows;           // +0x220 / +0x248
    RefPointer<PreferenceControl>        m_useShadersInScene;    // +0x230 / +0x258
    RefPointer<PreferenceControl>        m_useShadersInPalette;  // +0x234
    RefPointer<PreferenceControl>        m_useRoundEdges;        // +0x238
    RefPointer<PreferenceControl>        m_postProcessing;       // +0x23c
    BasicArray<RefPointer<Control> >     m_controls;             // +0x240 / +0x268 separators, note parts
    Real32                               m_y;                    // +0x250 / +0x278 next row (2)
    RefPointer<LDDButtonControl>         m_buttonOK;             // +0x254
    RefPointer<LDDButtonControl>         m_buttonCancel;         // +0x258
    RefPointer<LDDButtonControl>         m_buttonReset;          // +0x25c
    RefPointer<Control>                  m_restartNote;          // +0x260 / +0x288
                                                                 // Object +0x264 / +0x28c
};

} // namespace LEGO
