// LEGO Digital Designer — restored source
// LEGO::BGAutomaticMode — the building-guide mode ("bgAutomaticMode"): a BGMode whose guide
//   comes from a BGAutomaticApplication (generated, or stored with the model). Owns the
//   navigation commands (LDD.commands.bgAutomaticMode, short-cuts), step slider, "n / total"
//   texts, step preview panel (2 x 2 cells) and substep previews; layout from Lua "bgAutomaticMode".
//
// TU 367 "BGAutomaticMode.cpp": Mac 0x352ed0-0x35973b, Win 0x62476c-0x627523 (with gaps). The
// TU also emits the KeyboardShortCut / ControlManagerWindowObserver::AddKeyboardShortCut inline
// code (Mode.h: Mac 0x3555a0, 0x359460, D1/D0 0x3595a0 / 0x3595d0, Win sdd 0x624b12) and
// Observer<Command> thunks 0x359620 / 0x359660. Mac 0x3596c0 = <iostream> + a header static.
//
// RTTI (Mac): vmi {BGMode +0, PreferencesObserver +0x2ec, SliderControlObserver +0x2f4,
//   BGAutomaticApplicationObserver +0x308}, virtual base Object +0x3a8 (operator new 0x3b0;
//   Win 0x3e8, Object +0x3e0).
// Mac vtable 0x74e77c (73 entries): BGMode's 66 with Enter 45 = 0x357bf0, Exit 46 = 0x3586b0,
//   CanEnter 47 = 0x358920, NextPart..PrevStep 50..53 = 0x359090..0x3590f0, GetApplication 60 =
//   0x359000, UpdateCommands 65 = 0x359110 overridden; then the Mac-only primary entries of the
//   secondary-base overriders 66 OnValueChanged 0x3592c0, 67 OnPreferenceChanged 0x359170,
//   68 OnCandidatesChanged 0x358990, 69 OnViewUpdated 0x3589b0 (also the BCSC observer's
//   OnEvent08), 70 OnStepChanged 0x359010, 71 OnProgress 0x359060; 72 = the one new virtual
//   0x359080. Win vtable 0x9d011c (59 entries: 58 = 0x67e76a).
// Names: the class, the Lua names and KeyboardShortCut are evidence; the rest is inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "Preferences.h"
#include "SliderControl.h"
#include "BGMode.h"
#include "BGAutomaticApplicationObserver.h"
#include "BGAutomaticApplication.h"   // (complete: the observer's inline Detach)

namespace LEGO {

class EditMode;
class ViewMode;
class Command;
class Texture;
class SimpleSound;
class RenderControl;
class BricksControl;
class BGAutomaticApplication;
class GenerateBuildingGuideCommand;
class NextStepBICommand;
class NextPartBICommand;
class PrevStepBICommand;
class PrevPartBICommand;
class ReplayPartBICommand;
class BGScreenShotCommand;

class BGAutomaticMode : public BGMode, public PreferencesObserver, public SliderControlObserver,
                        public BGAutomaticApplicationObserver {
public:
    // A new BGAutomaticApplication on the render of the scene control's manager.
    // Mac 0x352f90, Win 0x625200 (same arguments as BrickMode)
    BGAutomaticMode(ControlManager* manager, Window* window, BrickConnectSceneControl* sceneControl);
    virtual ~BGAutomaticMode();                                             // Mac 0x3567a0/0x356c60 (body 0x355d80), Win sdd 0x6259b3

    // BrickMode::Create, the commands and short-cuts, the textures, preview panel, frames,
    // texts and slider, the Lua commands / slider and the Lua "bgAutomaticMode" Create. False
    // on failure (AppMain: error 0xb). Mac 0x3536a0, Win 0x6260dd
    bool Initialize(EditMode* editMode, ViewMode* viewMode, Control* parent, Control* after);
    using BrickMode::Initialize;

    // ---- Mode ----
    // Building-guide mode of the scene control, the application starts on the scene (and
    // the stored guide; nothing more when that fails), observers, colours, tools, program,
    // texts, the guide is generated and stored as the scene's building instructions.
    virtual void Enter();                                                   // 45/40 Mac 0x357bf0, Win 0x6270b7
    // The reverse; the guide goes back into the building instructions (a new BIStorage).
    virtual void Exit();                                                    // 46/41 Mac 0x3586b0, Win 0x625d61
    // With building instructions; else only after GenerateBuildingGuideCommand made some.
    virtual bool CanEnter();                                                // 47/42 Mac 0x358920, Win 0x624ef8

    // ---- BGMode: record the last action (UpdateSubstepPreview), then BGMode's ----
    virtual bool NextPart(bool substeps);                                   // 50/44 Mac 0x359090, Win 0x62476c (2)
    virtual bool PrevPart(bool substeps);                                   // 51/45 Mac 0x3590b0, Win 0x62477b (1)
    virtual bool NextStep(bool unused, bool animate, BasicArray<BIBrick*>& bricks, bool setCamera); // 52/46 Mac 0x3590d0, Win 0x62478a (4)
    virtual bool PrevStep(bool unused, bool animate);                       // 53/47 Mac 0x3590f0, Win 0x624799 (3)
    virtual BGApplication* GetApplication();                                // 60/54 Mac 0x359000, Win 0x624f3f
    // UpdateEnable of the five navigation commands.
    virtual void UpdateCommands();                                          // 65/57 Mac 0x359110, Win 0x624f46

    // ---- secondary bases ----
    // Slider: steps back / forward until the current step reaches the value.
    virtual void OnValueChanged(SliderControl* slider, Real32 value, Real32 oldValue); // 66/– Mac 0x3592c0 (thunk 0x359420), Win 0x62587e
    // PLAYSOUND changed: the pause sound is loaded again. NOTE: the mode never attaches to the
    // Preferences (both builds), so this is not called.
    virtual void OnPreferenceChanged(const String& key);                    // 67/– Mac 0x359170 (thunk 0x3592a0), Win 0x624f81
    virtual void OnCandidatesChanged();                                     // 68/– Mac 0x358990 (thunk 0x3589a0; empty)
    // The step texts, commands, selection cleared, slider range / value.
    virtual void OnViewUpdated();                                           // 69/– Mac 0x3589b0 (thunk 0x358bf0), Win 0x625ec4
    // UpdatePreviews, UpdateSubstepPreview.
    virtual void OnStepChanged(bool inStep);                                // 70/– Mac 0x359010 (thunk 0x359030), Win 0x62750b
    virtual void OnProgress(Real32 progress);                               // 71/– Mac 0x359060 (thunk 0x359070; empty)
    // (the BCSC observer's slot 8 is the same function as OnViewUpdated on Mac, thunk 0x358bd0)
    virtual void OnEvent08();

    // The one new virtual: returns 0 (meaning unknown). Mac 0x359080, Win 0x67e76a (name inferred)
    virtual UInt32 Unknown72();                                             // 72/58

protected:
    // Exit when current; observers; BGMode's. Mac 0x3571d0 (thunks 0x357300 … 0x357420)
    virtual void OnFinalRelease();

    // "<current>" (+0x324) right-aligned before "/<total>" (+0x328), bottom right of the scene.
    // Mac 0x3558d0
    void SetStepTexts(UInt32 current, UInt32 total);
    // The step previews and the substep previews go; slider 0..1; texts 0 / 0. Mac 0x355ac0, Win 0x624ceb
    void ClearPreviews();
    // The texts and the preview panel shown / hidden (hidden: all previews go, frames hidden).
    // Mac 0x355c00, Win 0x62581b
    void ShowOverlays(bool show);
    // The bricks of the current step in the preview cells (from the 4-group the current part
    // is in), or one big preview of a step that is a single substep. Mac 0x357440, Win 0x6259d5
    void UpdatePreviews();
    // In a substep: "ShowPreview" (Lua) with the model so far (+0x354) and the finished substep
    // (+0x358) and the frames; at the top level "HidePreview". Mac 0x358c10, Win 0x62735a
    void UpdateSubstepPreview();
    // The two substep previews (null / !show: none). Mac 0x357900 / 0x3579f0
    void SetSubstepPreview(bool show, BIBrick* brick);
    void SetModelPreview(bool show, BIBrick* brick);
    // A preview of `brick` on the scene control (200 x 173, bottom right at (-20, -offsetY)),
    // the frame image part of the icon texture. Mac 0x357ae0
    BricksControl* CreateSubstepPreview(BIBrick* brick, Real32 offsetY, const Rect& imageRect);

    // ---- data (Mac offsets) ----
    UInt32                         m_lastAction;      // +0x310 1 PrevPart, 2 NextPart, 3 PrevStep, 4 NextStep
    RefPointer<EditMode>           m_editMode;        // +0x314
    RefPointer<SimpleSound>        m_pauseSound;      // +0x318 "IA_pause.mp3"
    RefPointer<Texture>            m_icons;           // +0x31c Graphics/General/Icon_BI2.png
    RefPointer<Texture>            m_graphics;        // +0x320 Graphics/LDDgraphics01.png (slider)
    RefPointer<RenderControl>      m_currentText;     // +0x324 font 16
    RefPointer<RenderControl>      m_totalText;       // +0x328 font 13
    RefPointer<SliderControl>      m_slider;          // +0x32c (Lua LDD.controls.bgAutomaticMode.slider)
    RefPointer<RenderControl>      m_previewPanel;    // +0x330 200 x 200
    RefPointer<RenderControl>      m_previewCells[4]; // +0x334 100 x 100
    RefPointer<BricksControl>      m_previews[4];     // +0x344
    RefPointer<BricksControl>      m_modelPreview;    // +0x354
    RefPointer<BricksControl>      m_substepPreview;  // +0x358
    RefPointer<RenderControl>      m_frames[3];       // +0x35c
    bool                           m_unused368;       // +0x368 (only initialised)
    UInt32                         m_unused36c;       // +0x36c (only initialised)
    UInt32                         m_unused370;       // +0x370 (only initialised)
    // Exit stops the application instead of storing its guide (never set). (name inferred)
    bool                           m_keepGuide;       // +0x374
    UInt32                         m_unused378;       // +0x378 (only initialised)
    UInt32                         m_unused37c;       // +0x37c
    UInt32                         m_unused380;       // +0x380
    RefPointer<Command>            m_editNavigation;  // +0x384 ModeNavigationCommand(editMode)
    RefPointer<NextStepBICommand>  m_nextStep;        // +0x388
    RefPointer<NextPartBICommand>  m_nextPart;        // +0x38c
    RefPointer<PrevStepBICommand>  m_prevStep;        // +0x390
    RefPointer<PrevPartBICommand>  m_prevPart;        // +0x394
    RefPointer<ReplayPartBICommand> m_replayPart;     // +0x398
    RefPointer<GenerateBuildingGuideCommand> m_generateGuide; // +0x39c
    RefPointer<BGScreenShotCommand> m_screenShot;     // +0x3a0
    RefPointer<BGAutomaticApplication> m_application; // +0x3a4
                                                      // Object +0x3a8
};

} // namespace LEGO
