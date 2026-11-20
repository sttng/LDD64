// LEGO Digital Designer — restored source (ShowStartHelp declared by 4.5)
// LEGO::EditMode — the building mode, the visible start screen of LDD: the brick palette in a
//   sliding side pane, the tool box, the filter / colour pop-ups, the info field and the 3D scene.
//   Initialize creates the tools, controls and commands, publishes them to Lua (LDD.tools,
//   LDD.controls.editMode, LDD.commands.editMode) and runs the Lua object "editMode":Create(),
//   which builds the rest of the layout (Scripts/EditMode/*).
// File-local classes of EditMode.cpp (RTTI names, declared here so tests can reach them):
//   PickupBricksObserver (a pressed palette / template item goes to the click-stick tool),
//   ConnectedSelectionTool, SameColorSelectionTool, SameShapeSelectionTool,
//   SameColorAndDesignSelectionTool (SelectionTools with fixed filters), SetToolCommand,
//   SetSelectionToolCommand, SelectActiveSelectionToolCommand, SelectNextSelectionToolCommand,
//   SetFreeBuildToolCommand, SetBrandCommand, RemoveMaterialFilterCommand, HideMaterialsCommand,
//   ToggleDoCanConnectPhysicsTestCommand.
//
// TU 085 "EditMode.cpp" (one object): Mac 0x185550-0x196ea0 (sub-TUs 085a/085b/085c), Win
//   0x627f45-0x62d414 + 0x62d695 (Initialize) + 0x449ba0 / 0x44aaa0 / 0x460cb0 (template copies).
//   The TU also emits inline constructors of other classes (ClickStickTool 0x18b720,
//   PaletteMaterialControl 0x18bf60, HingeAlign/StringPull/ClickPullTool 0x18c750/0x18c8f0/
//   0x18ca20, MaterialGridControl 0x18cb50, FilterControl 0x18cf80, CreateTestModelCommand
//   0x18dfe0, SavePalettesCommand 0x18e130, BoxFilterControl 0x18ffe0): they are declared in the
//   headers of those classes (their items restore them).
//
// Mac layout (constructor 0x185550; sizeof 0x308 + Object = 0x310):
//   BrickMode +0x000..+0x280, LDDApplicationObserver +0x280 (subject +0x284), BrickPaletteObserver
//   +0x288 (subject +0x28c), MaterialGridControlObserver +0x290 (subject +0x294), members
//   +0x298..+0x308 below, Object +0x308.
// Win (constructor 0x62ba52, destructor 0x629158; sizeof 0x338 + Object): observers +0x2a0 /
//   +0x2ac / +0x2b8 (subjects +0x2a8 / +0x2b4 / +0x2c0), members = Mac + 0x2c,
//   vtordisp +0x334, Object +0x338 (sizeof 0x340; the restoration's diff tests).
// Virtual slots: Mac vtable 0x71543c (57 entries) = BrickMode's 50 with Enter 45 = 0x18e2d0,
//   Exit 46 = 0x18e3d0, CanExit 48 = 0x18e4d0, OnFinalRelease 2 = 0x18f3e0, then the primary
//   entries of the secondary-base overriders 50-56 (0x18f640, 0x18f780, 0x18f8c0, 0x18fab0,
//   0x18fae0, 0x18fb10, 0x18fb40; thunks 0x18f760 / 0x18f820 / 0x18fa90 / 0x18fac0 / 0x18faf0 /
//   0x18fb20 / 0x18fc40). Win vtable 0x9d0d0c (44: Enter 0x6283d0, Exit 0x628485, CanExit
//   0x628535), LDDApplicationObserver-in 0x9d0c44, BrickPaletteObserver-in 0x9d0c24,
//   MaterialGridControlObserver-in 0x9d0c14, Object-in 0x9d0c00 (sdd 0x6292ce / 0x62d02b).
// Names: ShowStartHelp / HideStartHelp / SetFreeBuildMode / UpdateBrickPaletteControl are the Lua
// callbacks EditMode calls (the C++ names are inferred from them); the rest is inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "Geometry.h"
#include "BrickMode.h"
#include "LDDApplication.h"
#include "BrickPalette.h"
#include "BrickConnectSceneManager.h"
#include "MaterialGridControl.h"
#include "BricksControlItemPressedNotifier.h"
#include "BricksClipBoard.h"
#include "Preferences.h"
#include "SimpleSound.h"
#include "Command.h"
#include "SelectionTool.h"

namespace LEGO {

class BGAutomaticMode;
class BrickConnectSceneControlTool;
class BrickPaletteControl;
class FilterControl;
class LDDPaneContainerControl;
class InfoBoxControl;
class RenderControl;
class ClickStickTool;
class AnimateShowHideControlCommand;
class GenerateBuildingGuideCommand;
class ToolboxMaterialItemControl;
class Texture;
class SimpleSound;
class PrimitiveElementBrick;

class EditMode : public BrickMode,
                 public LDDApplicationObserver,
                 public BrickPaletteObserver,
                 public MaterialGridControlObserver {
public:
    // Mac 0x185550 (C1), Win 0x62ba52: the clip board, the rest empty.
    EditMode(ControlManager* manager, Window* window, BrickConnectSceneControl* sceneControl);
    virtual ~EditMode();                                                    // Mac 0x18eb90/0x18ef70 (body 0x18e5d0), Win 0x629158 / sdd 0x62bcdc

    // BrickMode::Create below `parent`, then the tools, palettes, pop-ups and commands (see the
    // header comment) and the Lua "editMode":Create(); the side pane with the Lua palette; the
    // brick palette is observed. False when any step fails. AppMain passes its BGAutomaticMode and
    // ViewMode (only used for the (unused) navigation commands, as in the original).
    // Mac 0x185940, Win 0x62d695 (name inferred)
    bool Initialize(BGAutomaticMode* bgMode, Mode* viewMode, Control* parent, Control* after);
    using BrickMode::Initialize;   // (RenderControl::Initialize stays visible)

    // Selects the default tool and calls the Lua "ShowStartHelp" (the LDD / Mindstorms / LDD
    // Extended tabs). Callers: ShowStartHelpCommand (Mac 0x277200), StartupDelayedCommand
    // (Mac 0x277580). Mac 0x18e510, Win 0x628557 (name from the Lua callback)
    void ShowStartHelp();

    // ---- Mode ----
    // Shows the side pane, re-activates the last tool (single selection tool by default), grid
    // and wires on, restores the camera unless the scene is empty, tells the application it is
    // ready (with the building guide command), BrickMode::Enter.
    virtual void Enter();                                                   // 45/40 Mac 0x18e2d0, Win 0x6283d0
    // No tool, not ready, selection cleared, every brick shown, camera kept (unless the scene
    // is empty), side pane hidden, BrickMode::Exit.
    virtual void Exit();                                                    // 46/41 Mac 0x18e3d0, Win 0x628485
    // Remembers the active tool for the next Enter; true.
    virtual bool CanExit();                                                 // 48/43 Mac 0x18e4d0, Win 0x628535

    // ---- LDDApplicationObserver ----
    // No tool; Lua "SetFreeBuildMode"(LDD Extended), palette filter cleared, filter boxes rebuilt.
    virtual void OnBrandChanged(Brand brand);                               // 50/– Mac 0x18f640 (thunk 0x18f760), Win 0x62cdf3
    // Lua "HideStartHelp"; true (the document may change).
    virtual bool OnQuery10();                                               // 51/– Mac 0x18f780 (thunk copy 0x18f820), Win 0x6285aa

    // ---- BrickPaletteObserver: every change → UpdateBrickPaletteControl ----
    virtual void OnBagAdded(const BrickPalette::Bag* bag);                  // 52/– Mac 0x18f8c0 (thunk 0x18fa90), Win 0x628cd2
    virtual void OnBrickAdded(const BrickPalette::Brick* brick);            // 53/– Mac 0x18fab0 (thunk 0x18fac0), Win 0x628cd2
    virtual void OnPaletteChanged();                                        // 54/– Mac 0x18fae0 (thunk 0x18faf0), Win 0x628cc7

    // ---- MaterialGridControlObserver ----
    // Filters the palette by `material` (LDD Extended: selects it for the free-build bricks).
    virtual void OnMaterialSelected(UInt32 material);                       // 56/– Mac 0x18fb40 (thunk 0x18fc40), Win 0x6285f1

    // ---- accessors (names inferred; the original reads the members directly) ----
    BricksClipBoard*      GetClipBoard() const { return m_clipBoard; }
    BrickPaletteControl*  GetBrickPaletteControl() const { return m_brickPaletteControl; }
    FilterControl*        GetFilterControl() const { return m_filterControl; }
    MaterialGridControl*  GetMaterialGridControl() const { return m_materialGridControl; }
    ClickStickTool*       GetClickStickTool() const { return m_clickStickTool; }
    BrickConnectSceneControlTool* GetDefaultTool() const;
    BrickConnectSceneControlTool* GetLastActiveTool() const { return m_lastActiveTool; }
    const Vector3&        GetSavedCameraPosition() const { return m_cameraPosition; }
    const Vector3&        GetSavedCameraTarget() const { return m_cameraTarget; }
    const Vector3&        GetSavedCameraUp() const { return m_cameraUp; }

protected:
    // Exit when current; leaves the material grid, the brick palette and the application.
    virtual void OnFinalRelease();                                          // 2/– Mac 0x18f3e0
    // BrickPaletteObserver: attached to a palette → UpdateBrickPaletteControl.
    virtual void OnAttached(BrickPalette* palette);                         // 55/– Mac 0x18fb10 (thunk 0x18fb20), Win 0x628cd2
    using BrickPaletteObserver::OnDetached;
    using ControlObserver::OnAttached;
    using ControlObserver::OnDetached;
    using TextLayoutObserver::OnAttached;
    using TextureObserver::OnAttached;
    using LDDApplicationObserver::OnAttached;
    using LDDApplicationObserver::OnDetached;
    using MaterialGridControlObserver::OnAttached;
    using MaterialGridControlObserver::OnDetached;

private:
    // One BoxFilterControl per brick box of the application's palette (grouped by brand, boxes
    // with their own colour grid first), the current brand's colour-grid box selected; the
    // filter pop-up sized to the widest box. Mac 0x18d1a0, Win 0x62c352 (name inferred)
    void UpdateFilterControl();
    // The toggle button of `bag` in the filter pop-up (null when Create fails).
    // Mac 0x18fc60 (Win inlined) (name inferred)
    RefPointer<Control> CreateFilterItem(const BrickPalette::Bag* bag);
    // The palette's bricks (timed), Lua "UpdateBrickPaletteControl" of editMode.palette, the
    // colour grid and the tool box swatch. Mac 0x18f8d0, Win 0x62867b
    void UpdateBrickPaletteControl();

    // ---- data (Mac offset / Win offset) ----
    RefPointer<BricksClipBoard>              m_clipBoard;            // +0x298 / +0x2c4
    RefPointer<SelectionTool>                m_singleSelectionTool;  // +0x29c / +0x2c8 the default tool
    BrickConnectSceneControlTool*            m_lastActiveTool;       // +0x2a0 / +0x2cc (not referenced)
    RefPointer<GenerateBuildingGuideCommand> m_buildingGuideCommand; // +0x2a4 / +0x2d0
    RefPointer<BrickPaletteControl>          m_brickPaletteControl;  // +0x2a8 / +0x2d4
    RefPointer<FilterControl>                m_filterControl;        // +0x2ac / +0x2d8
    BasicArray<RefPointer<Control> >         m_filterItems;          // +0x2b0 / +0x2dc (BoxFilterControls)
    Vector3                                  m_cameraPosition;       // +0x2c0 / +0x2ec saved by Exit
    Vector3                                  m_cameraTarget;         // +0x2cc / +0x2f8
    Vector3                                  m_cameraUp;             // +0x2d8 / +0x304
    RefPointer<LDDPaneContainerControl>      m_paneContainer;        // +0x2e4 / +0x310 the side pane
    RefPointer<InfoBoxControl>               m_infoControl;          // +0x2e8 / +0x314
    RefPointer<RenderControl>                m_infoControlContainer; // +0x2ec / +0x318
    RefPointer<ClickStickTool>               m_clickStickTool;       // +0x2f0 / +0x31c
    RefPointer<AnimateShowHideControlCommand> m_animateShowHidePalette; // +0x2f4 / +0x320
    RefPointer<Control>                      m_unused2f8;            // +0x2f8 / +0x324 NOTE: never set (both builds)
    RefPointer<MaterialGridControl>          m_materialGridControl;  // +0x2fc / +0x328
    RefPointer<ToolboxMaterialItemControl>   m_materialItemControl;  // +0x300 / +0x32c
    RefPointer<Texture>                      m_texture;              // +0x304 / +0x330 EditMode_Final.png
                                                                     // Object +0x308 / +0x338
};

// ===========================================================================================
// PickupBricksObserver — Mac vtable 0x7157cc {D1 0x1904f0, D0 0x190610, OnFinalRelease 0x190770,
// OnAttached, OnDetached, OnBrickPressed 0x190980, OnModelPressed 0x190b30, OnPreferenceChanged
// 0x190b70}, PreferencesObserver-in 0x7157f8, Object-in 0x715828. Win: constructor 0x62ce75,
// OnBrickPressed 0x62a1ee, OnModelPressed 0x628796, OnPreferenceChanged 0x6287c7.
// Mac layout: BricksControlItemPressedObserver +0 (subjects +0x04), PreferencesObserver +0x14
// (subject +0x18), scene manager +0x1c, scene control +0x20, click-stick tool +0x24, pick-up
// sound +0x28, Object +0x2c (sizeof 0x34).
// ===========================================================================================
class PickupBricksObserver : public BricksControlItemPressedObserver, public PreferencesObserver {
public:
    // Loads the pick-up sound, observes the preferences. Mac 0x18bce0, Win 0x62ce75
    PickupBricksObserver(BrickConnectSceneManager* sceneManager,
                         BrickConnectSceneControl* sceneControl, ClickStickTool* clickStickTool);
    virtual ~PickupBricksObserver();                                        // Mac 0x1904f0/0x190610 (body 0x1903a0)

    // The click-stick tool becomes active (sound) and takes the brick; the scene gets the
    // keyboard focus and the "carry" cursor 101.
    virtual void OnBrickPressed(PrimitiveElementBrick* brick);              // 5 Mac 0x190980, Win 0x62a1ee
    // The click-stick tool becomes active and imports the model.
    virtual void OnModelPressed(StorageScene2* model);                      // 6 Mac 0x190b30, Win 0x628796
    // "PLAYSOUND" changed: reloads the sound.
    virtual void OnPreferenceChanged(const String& key);                    // 7 Mac 0x190b70 (thunk 0x190ca0), Win 0x6287c7

    SimpleSound* GetSound() const { return m_sound; }                       // (test accessor)

protected:
    // Leaves the preferences and every notifier (without OnDetached), Object::OnFinalRelease.
    virtual void OnFinalRelease();                                          // 2 Mac 0x190770 (thunks 0x190850, 0x1908e0), Win 0x62cfa3

private:
    // The pick-up sound ("IA_pick-up.mp3", LDDUtilities::CreateSound). (helper; inlined twice)
    void LoadSound();

    RefPointer<BrickConnectSceneManager> m_sceneManager;    // +0x1c
    BrickConnectSceneControl*            m_sceneControl;    // +0x20 (not referenced)
    RefPointer<ClickStickTool>           m_clickStickTool;  // +0x24
    RefPointer<SimpleSound>              m_sound;           // +0x28
};

// ===========================================================================================
// The selection tools with fixed filters (SelectionTool(clickStickTool, false, true) + filters +
// cursor). Mac sizeof 0x128 (Object +0x120); destructors only (D1/D0 Mac 0x192180/0x1921e0,
// 0x192430/0x192490, 0x1926e0/0x192740, 0x192990/0x1929f0; Win sdd 0x62b4c8 / 0x62b4ea /
// 0x62b50c / 0x62b52e).
// ===========================================================================================
// The parts of the picked part's model; cursor 103. Mac 0x18c0e0, Win 0x629b16
class ConnectedSelectionTool : public SelectionTool {
public:
    explicit ConnectedSelectionTool(ClickStickTool* clickStickTool);
    virtual ~ConnectedSelectionTool() {}
};
// The scene's parts with the picked colour; cursor 104. Mac 0x18c210, Win 0x629c6b
class SameColorSelectionTool : public SelectionTool {
public:
    explicit SameColorSelectionTool(ClickStickTool* clickStickTool);
    virtual ~SameColorSelectionTool() {}
};
// The scene's parts with the picked design; cursor 105. Mac 0x18c3b0, Win 0x629e16
class SameShapeSelectionTool : public SelectionTool {
public:
    explicit SameShapeSelectionTool(ClickStickTool* clickStickTool);
    virtual ~SameShapeSelectionTool() {}
};
// The scene's parts with the picked colour and design; cursor 106. Mac 0x18c550, Win 0x629fc1
class SameColorAndDesignSelectionTool : public SelectionTool {
public:
    explicit SameColorAndDesignSelectionTool(ClickStickTool* clickStickTool);
    virtual ~SameColorAndDesignSelectionTool() {}
};

// ===========================================================================================
// SetToolCommand — a toggle command that activates a tool; toggled while it is the active tool.
// Mac vtable 0x76bf9c {D1 0x193920, D0 0x193980, OnFinalRelease 0x1939f0, Do 0x193a80, SetEnable,
// GetEnable, UpdateEnable 0x193aa0, OnActiveToolChanged 0x193ac0}, observer-in 0x76bfc8, Object-in
// 0x76bffc. Win: constructor 0x62c722, Do 0x62ab37.
// Mac layout: Command +0 (enabled +0x24, toggleable +0x26 = 1), BrickConnectSceneManagerObserver
// +0x28 (subject +0x2c), tool +0x30, Object +0x34 (sizeof 0x3c).
// ===========================================================================================
class SetToolCommand : public Command, public BrickConnectSceneManagerObserver {
public:
    // Mac 0x18d9b0 (C1; C2 0x1945c0 with VTT), Win 0x62c722
    SetToolCommand(BrickConnectSceneManager* sceneManager, BrickConnectSceneControlTool* tool,
                   bool enabled);
    virtual ~SetToolCommand() {}                                            // Mac 0x193920/0x193980, Win sdd 0x62c1d2

    // The scene manager's active tool becomes the tool.
    virtual void Do();                                                      // 3 Mac 0x193a80, Win 0x62ab37
    // Always enabled.
    virtual void UpdateEnable();                                            // 6 Mac 0x193aa0
    // Toggled exactly while `current` is the tool.
    virtual void OnActiveToolChanged(BrickConnectSceneControlTool* previous,
                                     BrickConnectSceneControlTool* current); // 7 Mac 0x193ac0 (thunk 0x193be0)

    BrickConnectSceneControlTool* GetTool() const { return m_tool; }

protected:
    // Leaves the scene manager, detaches the command's observers, Object::OnFinalRelease.
    virtual void OnFinalRelease();                                          // 2 Mac 0x1939f0 (thunks 0x193bc0, 0x193cf0)
    // (the observer's overloads stay visible)
    using BrickConnectSceneManagerObserver::OnAttached;
    using BrickConnectSceneManagerObserver::OnDetached;

    RefPointer<BrickConnectSceneControlTool> m_tool;   // +0x30
};

typedef BasicArray<RefPointer<BrickConnectSceneControlTool> > ToolArray;

// The selection tool commands: the tool of the command follows the active tool when that is one
// of `tools`. Mac vtable 0x76c40c {…, OnActiveToolChanged 0x193fe0, SelectTool (pure)}; layout:
// SetToolCommand, tools +0x34, Object +0x44 (sizeof 0x4c). Constructor C2 0x193e10 (with VTT).
class SetSelectionToolCommand : public SetToolCommand {
public:
    // The command's tool is tools[0]. Mac 0x193e10 (C2), Win 0x62c7db
    SetSelectionToolCommand(BrickConnectSceneManager* sceneManager, const ToolArray& tools);
    virtual ~SetSelectionToolCommand() {}                                   // Mac 0x193f10/0x193f70, Win sdd 0x62c1f1

    // When `current` is one of the tools: SelectTool(it); then the toggle state.
    virtual void OnActiveToolChanged(BrickConnectSceneControlTool* previous,
                                     BrickConnectSceneControlTool* current); // 7 Mac 0x193fe0 (thunk 0x1941d0)
    // `it` points at the active tool in the tools. (name inferred)
    virtual void SelectTool(const RefPointer<BrickConnectSceneControlTool>* it) = 0; // 8

    const ToolArray& GetTools() const { return m_tools; }

protected:
    ToolArray m_tools;   // +0x34
};

// The command's tool becomes the active selection tool ("selectActiveSelectionTool").
// Mac vtable 0x76c334, Win constructor 0x62c86c.
class SelectActiveSelectionToolCommand : public SetSelectionToolCommand {
public:
    SelectActiveSelectionToolCommand(BrickConnectSceneManager* sceneManager, const ToolArray& tools)
        : SetSelectionToolCommand(sceneManager, tools) {}                   // (Mac inlined in Initialize 0x188a1d)
    virtual ~SelectActiveSelectionToolCommand() {}                          // Mac 0x1942d0/0x194330
    virtual void UpdateEnable();                                            // 6 Mac 0x1943a0
    virtual void SelectTool(const RefPointer<BrickConnectSceneControlTool>* it); // 8 Mac 0x1943c0, Win 0x62c8ee
};

// The command's tool becomes the selection tool after the active one (cyclic)
// ("selectNextSelectionTool"). Mac vtable 0x76c9f8, Win constructor 0x62c8ff.
class SelectNextSelectionToolCommand : public SetSelectionToolCommand {
public:
    SelectNextSelectionToolCommand(BrickConnectSceneManager* sceneManager, const ToolArray& tools)
        : SetSelectionToolCommand(sceneManager, tools) {}                   // (Mac inlined in Initialize 0x188ace)
    virtual ~SelectNextSelectionToolCommand() {}                            // Mac 0x194890/0x1948f0
    virtual void UpdateEnable();                                            // 6 Mac 0x194960
    virtual void SelectTool(const RefPointer<BrickConnectSceneControlTool>* it); // 8 Mac 0x194980, Win 0x62c989
};

// "materialTool": the material tool, or for LDD Extended the paint bucket. Mac vtable 0x76cd98
// {…, OnAttached 0x194cc0, OnBrandChanged 0x194cd0}; layout: SetToolCommand, LDDApplicationObserver
// +0x34 (subject +0x38), material tool +0x3c, paint bucket +0x40, Object +0x44 (sizeof 0x4c).
class SetFreeBuildToolCommand : public SetToolCommand, public LDDApplicationObserver {
public:
    // Observes LDDApplication::GetInstance(). Mac 0x18dc50, Win 0x62cb91
    SetFreeBuildToolCommand(BrickConnectSceneManager* sceneManager,
                            BrickConnectSceneControlTool* materialTool,
                            BrickConnectSceneControlTool* paintBucketTool);
    virtual ~SetFreeBuildToolCommand() {}                                   // Mac 0x194b90/0x194bf0, Win sdd 0x62cd49

    virtual void OnBrandChanged(Brand brand);                               // 9 Mac 0x194cd0 (thunk 0x194f60)

protected:
    virtual void OnFinalRelease();                                          // 2 Mac 0x194c60 (thunks 0x194db0, 0x194ee0, 0x195060)
    virtual void OnAttached(LDDApplication* application);                   // 8 Mac 0x194cc0 (thunk 0x194f40)
    using LDDApplicationObserver::OnDetached;
    using SetToolCommand::OnAttached;
    using SetToolCommand::OnDetached;

private:
    // The tool for the application's brand; an active old tool is deactivated first.
    // Mac 0x1951b0, Win 0x62ccb0 (→ 0x62cc61) (name inferred)
    void UpdateTool();

    RefPointer<BrickConnectSceneControlTool> m_materialTool;     // +0x3c
    RefPointer<BrickConnectSceneControlTool> m_paintBucketTool;  // +0x40
};

// "setlddBrand" / "setMindstormsBrand" / "setLDDExtendedBrand": LDDApplication::SetBrand;
// enabled while another brand is set. Mac vtable 0x76d1cc {…, Do 0x1953f0, UpdateEnable 0x195410,
// OnAttached 0x195450, OnBrandChanged 0x195460}; layout: Command (enabled 0), LDDApplicationObserver
// +0x28 (private, subject +0x2c), brand +0x30, Object +0x34 (sizeof 0x3c).
class SetBrandCommand : public Command, private LDDApplicationObserver {
public:
    // Observes LDDApplication::GetInstance(). Mac 0x18ddb0, Win 0x62c9fd
    explicit SetBrandCommand(Brand brand);
    virtual ~SetBrandCommand() {}                                           // Mac 0x195260/0x1952d0, Win sdd 0x62cb5e

    virtual void Do();                                                      // 3 Mac 0x1953f0, Win 0x62ca9a
    virtual void UpdateEnable();                                            // 6 Mac 0x195410, Win 0x62caaa
    Brand GetBrand() const { return m_brand; }

protected:
    virtual void OnFinalRelease();                                          // 2 Mac 0x195360 (thunks 0x195590, 0x195730), Win 0x62cb7d
    using LDDApplicationObserver::OnDetached;
    virtual void OnAttached(LDDApplication* application);                   // 7 Mac 0x195450 (thunk 0x1955b0)
    virtual void OnBrandChanged(Brand brand);                               // 8 Mac 0x195460 (thunk 0x1955d0), Win 0x62cace

private:
    Brand m_brand;   // +0x30
};

// "removeMaterialFilterCommand": removes the palette's material filter and the grid's selection
// (not for LDD Extended). Mac vtable 0x76d32c {…, Do 0x195840}; layout: Command, edit mode +0x28,
// Object +0x2c (sizeof 0x34; constructor inlined in Initialize, Win 0x62b87a).
class RemoveMaterialFilterCommand : public Command {
public:
    explicit RemoveMaterialFilterCommand(EditMode* editMode) : Command(true), m_editMode(editMode) {}
    virtual ~RemoveMaterialFilterCommand() {}                               // Mac 0x195750/0x1957c0
    virtual void Do();                                                      // 3 Mac 0x195840, Win 0x62b8f6
private:
    EditMode* m_editMode;   // +0x28
};

// "hideMaterialsCommand": a toggle command; the palette hides its colour variants while toggled.
// Mac vtable 0x76d3e0 {…, Do 0x195b60}; layout: Command (toggleable), edit mode +0x28, Object
// +0x2c (sizeof 0x34). Constructor Mac 0x18ded0, Win 0x62b9a8.
class HideMaterialsCommand : public Command {
public:
    explicit HideMaterialsCommand(EditMode* editMode);
    virtual ~HideMaterialsCommand() {}                                      // Mac 0x195a70/0x195ae0
    virtual void Do();                                                      // 3 Mac 0x195b60, Win 0x62ba36
private:
    EditMode* m_editMode;   // +0x28
};

// "toggleDoCanConnectPhysicsTest" (developer): toggles the scene's collision test.
// Mac vtable 0x76d490 {…, Do 0x195da0, UpdateEnable 0x195dd0}; layout: Command, scene control
// +0x28, flag +0x2c (true), Object +0x30 (sizeof 0x38; constructor inlined, Win 0x62b010).
class ToggleDoCanConnectPhysicsTestCommand : public Command {
public:
    explicit ToggleDoCanConnectPhysicsTestCommand(BrickConnectSceneControl* sceneControl)
        : Command(true), m_sceneControl(sceneControl), m_test(true) {}
    virtual ~ToggleDoCanConnectPhysicsTestCommand() {}                      // Mac 0x195cb0/0x195d20
    virtual void Do();                                                      // 3 Mac 0x195da0, Win 0x62b087
    virtual void UpdateEnable();                                            // 6 Mac 0x195dd0
    bool GetTest() const { return m_test; }
private:
    BrickConnectSceneControl* m_sceneControl;   // +0x28
    bool                      m_test;           // +0x2c
};

} // namespace LEGO
