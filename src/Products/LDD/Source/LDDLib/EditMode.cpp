// LEGO Digital Designer — restored source
// EditMode.cpp — LEGO::EditMode and its file-local helper classes (see EditMode.h).
// TU 085: Mac 0x185550-0x196ea0 (sub-TUs 085a ctor + Initialize, 085b operations, 085c
// destructor / virtual overrides / local classes / tail); Win 0x627f45-0x62d414, 0x62d695.
#include "EditMode.h"

#include <iostream>

#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "ProfileTimer.h"
#include "Preferences.h"
#include "RenderControl.h"
#include "RenderControlManager.h"
#include "ControlManager.h"
#include "Texture.h"
#include "Scene.h"
#include "LuaPlus.h"
#include "LuaBindings.h"
#include "LuaScriptManager.h"
#include "LuaControl.h"
#include "LuaMatrixLayoutControl.h"
#include "LuaCommandAdapter.h"
#include "SetPreferenceCommand.h"
#include "LDDApplication.h"
#include "LDDUtilities.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneManager.h"
#include "BrickModeGlobalObject.h"
#include "BrickFilter.h"
#include "PrimitiveElementBrick.h"
#include "IncludeModelOnlyFilter.h"
#include "LuaTool.h"
#include "LuaHingeTool.h"
#include "LuaBrickMode.h"
// tools
#include "ClickStickTool.h"
#include "SingleSelectionTool.h"
#include "CloneTool.h"
#include "DeleteTool.h"
#include "HideTool.h"
#include "HingeTool.h"
#include "HingeAlignTool.h"
#include "StringPullTool.h"
#include "ClickPullTool.h"
#include "SurfaceColorPickerTool.h"
#include "SurfacePaintBucketTool.h"
#include "DecorationTool.h"
#include "MaterialTool.h"
#include "MoveTool.h"
// controls
#include "BrickPaletteControl.h"
#include "PaletteMaterialControl.h"
#include "MaterialGridControl.h"
#include "ToolboxMaterialItemControl.h"
#include "FilterControl.h"
#include "BoxFilterControl.h"
#include "RecentModelsControl.h"
#include "TemplateSelectorControl.h"
#include "GroupTreeViewControl.h"
#include "InfoBoxControl.h"
#include "LDDPaneContainerControl.h"
// commands
#include "AnimateShowHideControlCommand.h"
#include "ModeNavigationCommand.h"
#include "ShowStartHelpCommand.h"
#include "GenerateBuildingGuideCommand.h"
#include "BGModeCommand.h"
#include "UploadModelCommand.h"
#include "GroupToBrickGroupCommand.h"
#include "GroupToLibraryCommand.h"
#include "NewCommand.h"
#include "OpenCommand.h"
#include "ImportCommand.h"
#include "ExportCommand.h"
#include "SaveCommand.h"
#include "SaveAsCommand.h"
#include "ExportBOMCommand.h"
#include "UndoRedoCommand.h"
#include "CutCommand.h"
#include "PasteCommand.h"
#include "DeleteCommand.h"
#include "SelectAllCommand.h"
#include "InvertSelectionCommand.h"
#include "CollapseMaingroupsCommand.h"
#include "ExpandMaingroupsCommand.h"
#include "CreateTestModelCommand.h"
#include "SavePalettesCommand.h"

namespace LEGO {

namespace {

// Timer of the palette rebuild in UpdateBrickPaletteControl (static initialiser Mac 0x196db0,
// Win 0x933b6d; listTimes = true). NOTE: the Mac initialiser also zeroes a 0x18-byte static
// of an included header (0x77d910-0x77d924, one byte 1 at +0xf) — not identified.
ProfileTimer s_setBricksTimer(String("EditMode::UpdateBrickPaletteControl: SetBricks()"), true);

// The scene manager's renderer (RenderControlManager::GetRender, ControlManager slot 38).
Render* GetRender(Control* control)
{
    return static_cast<RenderControlManager*>(control->GetManager())->GetRender();
}

// tools.<name> = Lua object of the tool / control / command (LuaObject::Set, Mac 0x142210).
void SetLuaEntry(LuaPlus::LuaObject& table, const char* name, const LuaPlus::LuaObject& value)
{
    table.Set(name, value);
}

} // namespace

// ===========================================================================================
// EditMode
// ===========================================================================================

// Mac 0x185550, Win 0x62ba52
EditMode::EditMode(ControlManager* manager, Window* window, BrickConnectSceneControl* sceneControl)
    : BrickMode(manager, window, sceneControl),
      m_clipBoard(new BricksClipBoard),
      m_singleSelectionTool(0),
      m_lastActiveTool(0)
{
}

// Mac 0x18eb90 / 0x18ef70 (member destruction 0x18e5d0), Win 0x629158
EditMode::~EditMode()
{
}

// Mac 0x18f3e0
void EditMode::OnFinalRelease()
{
    if (IsCurrent())
        Exit();
    MaterialGridControlObserver::Detach();
    BrickPaletteObserver::Detach();
    LDDApplicationObserver::Detach();
    BrickMode::OnFinalRelease();
}

// Mac 0x185940, Win 0x62d695
bool EditMode::Initialize(BGAutomaticMode* bgMode, Mode* viewMode, Control* parent, Control* after)
{
    // NOTE: read but not used (both builds).
    bool developerMode = false;
    Preferences::GetInstance()->GetValue(String("DeveloperMode"), developerMode);
    (void)developerMode;

    if (!BrickMode::Create(parent, after))
        return false;

    ControlManager* manager = GetManager();
    Render* render = GetRender(this);

    m_clickStickTool = new ClickStickTool(GetSceneManager());
    if (!m_clickStickTool->Create(render, GetSceneManager()))
        return false;

    RefPointer<Command> groupBricks = new GroupToBrickGroupCommand(GetSceneControl());
    RefPointer<Command> groupToLibrary = new GroupToLibraryCommand(GetSceneControl());

    RefPointer<PickupBricksObserver> pickup =
        new PickupBricksObserver(GetSceneManager(), GetSceneControl(), m_clickStickTool);
    m_array238.PushBack(RefPointer<Object>(pickup.Get()));

    RefPointer<PaletteMaterialControl> paletteMaterials =
        new PaletteMaterialControl(manager, GetSceneManager(), 10, 8);
    if (!paletteMaterials->Create(manager->GetLayer(1), 0))
        return false;

    m_texture = LoadApplicationTexture(String("Graphics/EditScreen/EditMode_Final.png"),
                                       GetRender(this));
    if (!m_texture)
        return false;

    m_brickPaletteControl = new BrickPaletteControl(manager, GetSceneManager(), paletteMaterials,
                                                    m_texture, true, true);
    if (!m_brickPaletteControl->Create(Vector2(64.0f, 64.0f)))
        return false;
    pickup->BricksControlItemPressedObserver::Attach(static_cast<BricksControlItemPressedNotifier*>(m_brickPaletteControl.Get()));

    m_animateShowHidePalette = new AnimateShowHideControlCommand;

    // ---- tools ----
    m_singleSelectionTool = new SingleSelectionTool(m_clickStickTool);
    if (!m_singleSelectionTool->Create(render, GetSceneManager()))
        return false;
    RefPointer<SelectionTool> multipleSelection = new SelectionTool(m_clickStickTool, true, true);
    if (!multipleSelection->Create(render, GetSceneManager()))
        return false;
    RefPointer<SelectionTool> connectedSelection = new ConnectedSelectionTool(m_clickStickTool);
    if (!connectedSelection->Create(render, GetSceneManager()))
        return false;
    RefPointer<SelectionTool> sameColorSelection = new SameColorSelectionTool(m_clickStickTool);
    if (!sameColorSelection->Create(render, GetSceneManager()))
        return false;
    RefPointer<SelectionTool> sameShapeSelection = new SameShapeSelectionTool(m_clickStickTool);
    if (!sameShapeSelection->Create(render, GetSceneManager()))
        return false;
    RefPointer<SelectionTool> sameColorAndDesignSelection =
        new SameColorAndDesignSelectionTool(m_clickStickTool);
    if (!sameColorAndDesignSelection->Create(render, GetSceneManager()))
        return false;
    RefPointer<HingeTool> hingeTool = new HingeTool;
    if (!hingeTool->Create(manager, GetSceneManager()))
        return false;
    RefPointer<HingeAlignTool> hingeAlignTool = new HingeAlignTool;
    if (!hingeAlignTool->Create(render, GetSceneManager()))
        return false;
    RefPointer<StringPullTool> stringPullTool = new StringPullTool;
    if (!stringPullTool->Create(render, GetSceneManager()))
        return false;
    RefPointer<ClickPullTool> clickPullTool = new ClickPullTool;
    if (!clickPullTool->Create(render, GetSceneManager()))
        return false;
    RefPointer<SelectionTool> cloneTool = new CloneTool(m_clickStickTool);
    if (!cloneTool->Create(render, GetSceneManager()))
        return false;
    RefPointer<SurfaceColorPickerTool> colorPickerTool = new SurfaceColorPickerTool;
    if (!colorPickerTool->Create(render, GetSceneManager()))
        return false;
    RefPointer<SurfacePaintBucketTool> paintBucketTool = new SurfacePaintBucketTool;
    if (!paintBucketTool->Create(render, GetSceneManager()))
        return false;
    RefPointer<SelectionTool> deleteTool = new DeleteTool(m_clickStickTool);
    if (!deleteTool->Create(render, GetSceneManager()))
        return false;
    RefPointer<MaterialTool> materialTool = new MaterialTool(false, new CompositeBrickFilter);
    // The brick palette control's material filter (Mac 0x186423: +0x258).
    if (!materialTool->Create(GetSceneControl(), manager, GetSceneManager(),
                              m_brickPaletteControl->GetMaterialFilter()))
        return false;
    RefPointer<DecorationTool> decorationTool = new DecorationTool;
    if (!decorationTool->Create(GetSceneControl(), manager, GetSceneManager()))
        return false;
    RefPointer<SelectionTool> hideTool = new HideTool(m_clickStickTool);
    if (!hideTool->Create(render, GetSceneManager()))
        return false;
    RefPointer<MoveTool> moveTool = new MoveTool(GetSceneManager());
    if (!moveTool->Create(render, GetSceneManager()))
        return false;

    // ---- the tool box swatch of the paint tools ----
    m_materialItemControl = new ToolboxMaterialItemControl(manager, GetSceneManager()->GetDatabase(),
                                                           GetSceneManager()->GetSceneControl());
    if (!m_materialItemControl->Create(GetSceneControl(), GetSceneControl(), 0))
        return false;
    m_materialItemControl->SetParent(0, 0);
    m_materialItemControl->SetSize(Vector2(24.0f, 24.0f));
    m_materialItemControl->SetInputFlags(m_materialItemControl->GetInputFlags() & 0xbfffffff);
    paintBucketTool->SetMaterialItemControl(m_materialItemControl);
    colorPickerTool->SetMaterialItemControl(m_materialItemControl);

    // ---- colour grid pop-up ----
    m_materialGridControl = new MaterialGridControl(manager, GetSceneManager()->GetDatabase());
    if (!m_materialGridControl->Create(manager->GetLayer(1), 0))
        return false;
    MaterialGridControlObserver::Attach(m_materialGridControl);

    // ---- brick box filter pop-up ----
    m_filterControl = new FilterControl(manager, m_brickPaletteControl);
    if (!m_filterControl->Create(manager->GetLayer(1), 0))
        return false;
    m_filterControl->SetHitTestable(true);
    m_filterControl->SetEnabled(true);
    m_filterControl->SetTransparent(true);
    m_filterControl->SetActive(true);
    UpdateFilterControl();

    RefPointer<RecentModelsControl> recentModels = new RecentModelsControl(manager, GetSceneControl());
    if (!recentModels->Create())
        return false;

    // ---- commands ----
    // NOTE: the navigation, BG mode and upload commands are created and dropped again (both builds;
    // AppMain makes the ones that are used).
    RefPointer<ModeNavigationCommand> editNavigation = new ModeNavigationCommand(this);
    RefPointer<Command> bgModeCommand = new BGModeCommand(bgMode, GetSceneControl());
    RefPointer<ModeNavigationCommand> viewNavigation = new ModeNavigationCommand(viewMode);
    m_buildingGuideCommand = new GenerateBuildingGuideCommand(GetSceneControl());
    RefPointer<Command> uploadModel = new UploadModelCommand(GetSceneControl(), editNavigation);
    LDDApplicationObserver::Attach(LDDApplication::GetInstance());

    RefPointer<Command> setSingleSelection =
        new SetToolCommand(GetSceneManager(), m_singleSelectionTool, true);
    RefPointer<Command> setMultipleSelection =
        new SetToolCommand(GetSceneManager(), multipleSelection, true);
    RefPointer<Command> setConnectedSelection =
        new SetToolCommand(GetSceneManager(), connectedSelection, true);
    RefPointer<Command> setSameColorSelection =
        new SetToolCommand(GetSceneManager(), sameColorSelection, true);
    RefPointer<Command> setSameShapeSelection =
        new SetToolCommand(GetSceneManager(), sameShapeSelection, true);
    RefPointer<Command> setSameColorAndDesignSelection =
        new SetToolCommand(GetSceneManager(), sameColorAndDesignSelection, true);

    ToolArray selectionTools;
    selectionTools.PushBack(RefPointer<BrickConnectSceneControlTool>(m_singleSelectionTool.Get()));
    selectionTools.PushBack(RefPointer<BrickConnectSceneControlTool>(multipleSelection.Get()));
    selectionTools.PushBack(RefPointer<BrickConnectSceneControlTool>(connectedSelection.Get()));
    selectionTools.PushBack(RefPointer<BrickConnectSceneControlTool>(sameColorSelection.Get()));
    selectionTools.PushBack(RefPointer<BrickConnectSceneControlTool>(sameShapeSelection.Get()));
    selectionTools.PushBack(RefPointer<BrickConnectSceneControlTool>(sameColorAndDesignSelection.Get()));

    bool failed = true;
    {
        RefPointer<LuaScriptManager> scripts = LuaScriptManager::GetInstance();
        LuaPlus::LuaState* state = scripts->GetLuaState();
        LuaPlus::LuaObject globals = state->GetGlobals();

        // ---- LDD.tools ----
        LuaPlus::LuaObject tools = globals.Lookup("LDD.tools");
        LuaPlus::LuaState* toolsState = tools.GetState();
        SetLuaEntry(tools, "singleSelectionTool", LuaTool::CreateLuaObject(m_singleSelectionTool, toolsState));
        SetLuaEntry(tools, "multipleSelectionTool", LuaTool::CreateLuaObject(multipleSelection, toolsState));
        SetLuaEntry(tools, "connectedSelectionTool", LuaTool::CreateLuaObject(connectedSelection, toolsState));
        SetLuaEntry(tools, "sameColorSelectionTool", LuaTool::CreateLuaObject(sameColorSelection, toolsState));
        SetLuaEntry(tools, "sameShapeSelectionTool", LuaTool::CreateLuaObject(sameShapeSelection, toolsState));
        SetLuaEntry(tools, "sameColorAndShapeSelectionTool",
                    LuaTool::CreateLuaObject(sameColorAndDesignSelection, toolsState));
        SetLuaEntry(tools, "hingeTool", LuaHingeTool::CreateLuaObject(hingeTool, toolsState));
        SetLuaEntry(tools, "hingeAlignTool", LuaTool::CreateLuaObject(hingeAlignTool, toolsState));
        SetLuaEntry(tools, "stringPullTool", LuaTool::CreateLuaObject(stringPullTool, toolsState));
        SetLuaEntry(tools, "clickPullTool", LuaTool::CreateLuaObject(clickPullTool, toolsState));
        SetLuaEntry(tools, "cloneTool", LuaTool::CreateLuaObject(cloneTool, toolsState));
        SetLuaEntry(tools, "colorPickerTool", LuaTool::CreateLuaObject(colorPickerTool, toolsState));
        SetLuaEntry(tools, "paintBucketTool", LuaTool::CreateLuaObject(paintBucketTool, toolsState));
        SetLuaEntry(tools, "deleteTool", LuaTool::CreateLuaObject(deleteTool, toolsState));
        SetLuaEntry(tools, "materialTool", LuaTool::CreateLuaObject(materialTool, toolsState));
        SetLuaEntry(tools, "decorationTool", LuaTool::CreateLuaObject(decorationTool, toolsState));
        SetLuaEntry(tools, "hideTool", LuaTool::CreateLuaObject(hideTool, toolsState));
        SetLuaEntry(tools, "moveTool", LuaTool::CreateLuaObject(moveTool, toolsState));
        SetLuaEntry(tools, "clickStickTool", LuaTool::CreateLuaObject(m_clickStickTool, toolsState));
        SetLuaEntry(tools, "materialItemControl",
                    LuaControl::CreateControlTable(toolsState, m_materialItemControl));

        // ---- LDD.controls.editMode ----
        LuaPlus::LuaObject controls = globals.Lookup("LDD.controls.editMode");
        LuaPlus::LuaState* controlsState = controls.GetState();
        SetLuaEntry(controls, "materialGridControl",
                    LuaControl::CreateControlTable(controlsState, m_materialGridControl));
        SetLuaEntry(controls, "filterControlBox",
                    LuaControl::CreateControlTable(controlsState, m_filterControl));
        TemplateSelectorControl* templates =
            new TemplateSelectorControl(manager, GetSceneManager(),
                                        GetSceneControl()->GetBrickPalette(), pickup, 100);
        SetLuaEntry(controls, "templatePaletteControl",
                    LuaControl::CreateControlTable(controlsState, templates));
        RefPointer<GroupTreeViewControl> groupTree =
            new GroupTreeViewControl(manager, GetSceneManager());
        groupTree->Initialize();
        SetLuaEntry(controls, "groupTreeViewControl",
                    LuaControl::CreateControlTable(controlsState, groupTree));
        SetLuaEntry(controls, "recentModelsControl",
                    LuaMatrixLayoutControl::CreateControlTable(controlsState, recentModels));

        m_infoControlContainer = new RenderControl(GetManager());
        m_infoControlContainer->Initialize();
        m_infoControlContainer->Setup(BrickModeGlobalObject::GetSceneHolder(), 0, 7,
                                      Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
        SetLuaEntry(controls, "infoControlContainer",
                    LuaControl::CreateControlTable(controlsState, m_infoControlContainer));
        m_infoControl = new InfoBoxControl(GetManager(), GetSceneManager()->GetDatabase());
        m_infoControl->Create(GetSceneControl());
        m_infoControl->Setup(m_infoControlContainer, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
        SetLuaEntry(controls, "infoControl",
                    LuaControl::CreateControlTable(controlsState, m_infoControl));

        // ---- LDD.commands.editMode ----
        LuaPlus::LuaObject commands = globals.Lookup("LDD.commands.editMode");
        LuaPlus::LuaState* commandsState = commands.GetState();
#define LDD_EDITMODE_COMMAND(name, command) \
        SetLuaEntry(commands, name, LuaCommandAdapter::CreateLuaObject(command, commandsState))
        LDD_EDITMODE_COMMAND("new", new NewCommand(GetSceneManager(), new ShowStartHelpCommand(this)));
        LDD_EDITMODE_COMMAND("open", new OpenCommand);
        LDD_EDITMODE_COMMAND("import", new ImportCommand(GetSceneManager(), m_clickStickTool));
        LDD_EDITMODE_COMMAND("export", new ExportCommand(GetSceneControl()));
        LDD_EDITMODE_COMMAND("save", new SaveCommand(GetSceneControl()));
        LDD_EDITMODE_COMMAND("saveAs", new SaveAsCommand);
        LDD_EDITMODE_COMMAND("exportBOM", new ExportBOMCommand(GetSceneControl()));
        LDD_EDITMODE_COMMAND("undo", new UndoRedoCommand(GetSceneManager(), true));
        LDD_EDITMODE_COMMAND("redo", new UndoRedoCommand(GetSceneManager(), false));
        LDD_EDITMODE_COMMAND("cut", new CutCommand(GetSceneControl(), m_clipBoard, true));
        LDD_EDITMODE_COMMAND("copy", new CutCommand(GetSceneControl(), m_clipBoard, false));
        LDD_EDITMODE_COMMAND("paste", new PasteCommand(GetSceneManager(), m_clickStickTool, m_clipBoard));
        LDD_EDITMODE_COMMAND("delete", new DeleteCommand(GetSceneControl()));
        LDD_EDITMODE_COMMAND("selectAll", new SelectAllCommand(GetSceneControl()));
        LDD_EDITMODE_COMMAND("groupBricks", groupBricks);
        LDD_EDITMODE_COMMAND("groupToLibrary", groupToLibrary);
        LDD_EDITMODE_COMMAND("selectActiveSelectionTool",
                             new SelectActiveSelectionToolCommand(GetSceneManager(), selectionTools));
        LDD_EDITMODE_COMMAND("selectNextSelectionTool",
                             new SelectNextSelectionToolCommand(GetSceneManager(), selectionTools));
        LDD_EDITMODE_COMMAND("singleSelectionTool", setSingleSelection);
        LDD_EDITMODE_COMMAND("multipleSelectionTool", setMultipleSelection);
        LDD_EDITMODE_COMMAND("connectedSelectionTool", setConnectedSelection);
        LDD_EDITMODE_COMMAND("sameColorSelectionTool", setSameColorSelection);
        LDD_EDITMODE_COMMAND("sameShapeSelectionTool", setSameShapeSelection);
        LDD_EDITMODE_COMMAND("sameColorAndShapeSelectionTool", setSameColorAndDesignSelection);
        LDD_EDITMODE_COMMAND("cloneTool", new SetToolCommand(GetSceneManager(), cloneTool, true));
        LDD_EDITMODE_COMMAND("hingeTool", new SetToolCommand(GetSceneManager(), hingeTool, true));
        LDD_EDITMODE_COMMAND("hingeAlignTool", new SetToolCommand(GetSceneManager(), hingeAlignTool, true));
        LDD_EDITMODE_COMMAND("clickPullTool", new SetToolCommand(GetSceneManager(), clickPullTool, true));
        LDD_EDITMODE_COMMAND("materialTool",
                             new SetFreeBuildToolCommand(GetSceneManager(), materialTool, paintBucketTool));
        LDD_EDITMODE_COMMAND("hideTool", new SetToolCommand(GetSceneManager(), hideTool, true));
        LDD_EDITMODE_COMMAND("moveTool", new SetToolCommand(GetSceneManager(), moveTool, true));
        LDD_EDITMODE_COMMAND("deleteTool", new SetToolCommand(GetSceneManager(), deleteTool, true));
        LDD_EDITMODE_COMMAND("generateBuildingGuide", m_buildingGuideCommand);
        LDD_EDITMODE_COMMAND("setlddBrand", new SetBrandCommand(kBrandLDD));
        LDD_EDITMODE_COMMAND("setMindstormsBrand", new SetBrandCommand(kBrandMindstorms));
        LDD_EDITMODE_COMMAND("setLDDExtendedBrand", new SetBrandCommand(kBrandLDDExtended));
        LDD_EDITMODE_COMMAND("animateShowHidePalette", m_animateShowHidePalette);
        LDD_EDITMODE_COMMAND("collapseMaingroupsCommand", new CollapseMaingroupsCommand(m_brickPaletteControl));
        LDD_EDITMODE_COMMAND("expandMaingroupsCommand", new ExpandMaingroupsCommand(m_brickPaletteControl));
        LDD_EDITMODE_COMMAND("removeMaterialFilterCommand", new RemoveMaterialFilterCommand(this));
        LDD_EDITMODE_COMMAND("hideMaterialsCommand", new HideMaterialsCommand(this));
        LDD_EDITMODE_COMMAND("invertSelection", new InvertSelectionCommand(GetSceneControl()));
        LDD_EDITMODE_COMMAND("createTestModelAllBricks",
                             new CreateTestModelCommand(GetSceneControl(), m_brickPaletteControl, false));
        LDD_EDITMODE_COMMAND("createTestModelFilteredBricks",
                             new CreateTestModelCommand(GetSceneControl(), m_brickPaletteControl, true));
        LDD_EDITMODE_COMMAND("savePalettes", new SavePalettesCommand(m_brickPaletteControl));
        LDD_EDITMODE_COMMAND("toggleDoCanConnectPhysicsTest",
                             new ToggleDoCanConnectPhysicsTestCommand(GetSceneControl()));
        // The web service of InitializeDesigner (developer menu).
        static const char* const kServers[5][2] = {
            { "changeServerToDKAAPP-PER", "http://dkaapp-per.corp.lego.com:9968/service/integration.asmx/initialiseDesigner" },
            { "changeServerTo3rdParty",   "http://ldd.3rd.corp.lego.com/service/integration.asmx/initialiseDesigner" },
            { "changeServerToLEGOcom",    "http://ldd.lego.com/service/integration.asmx/initialiseDesigner" },
            { "changeServerToWebDev",     "http://ldd.dev.corp.lego.com/service/integration.asmx/initialiseDesigner" },
            { "changeServerToWebQA",      "http://ldd.webqa.lego.com/service/integration.asmx/initialiseDesigner" }
        };
        for (UInt32 i = 0; i < 5; ++i)
            LDD_EDITMODE_COMMAND(kServers[i][0], new SetPreferenceCommand(String("initialiseDesignerURL"),
                                                                          String(kServers[i][1])));
#undef LDD_EDITMODE_COMMAND

        // ---- the Lua edit mode builds the layout ----
        LuaPlus::LuaObject editMode = globals.GetByName("editMode");
        if (editMode.IsTable()) {
            SetLuaBaseClass(editMode, "BrickMode");
            RefPointer<LuaBrickMode> luaMode = new LuaBrickMode;
            if (luaMode->Bind(this, editMode) && CallLuaMethod(editMode, "Create", false, 1)) {
                LuaPlus::LuaObject created(state, -1);
                state->Pop(1);
                if (LuaToBoolean(created)) {
                    LuaPlus::LuaObject paletteObject = globals.Lookup("editMode.palette");
                    BindableLuaObject* bound = GetBoundObject(paletteObject);
                    LuaControl* paletteControl = bound ? dynamic_cast<LuaControl*>(bound) : 0;
                    SetSidePanel(paletteControl ? paletteControl->GetTargetControl() : 0);
                    m_animateShowHidePalette->SetControl(GetSidePanel());
                    m_paneContainer = new LDDPaneContainerControl(GetManager(), GetSidePanel(), true);
                    if (m_paneContainer->Create(BrickModeGlobalObject::GetSceneHolder(), 0,
                                                m_animateShowHidePalette)) {
                        m_paneContainer->SetAxisAlignment(0, 1, 1, 0.0f, 0);
                        m_paneContainer->SetAxisMargins(1, 0.0f, 0.0f, 0);
                        // The info field ends 8 pixels left of the side pane.
                        m_infoControlContainer->SetAxisAnchors(0, 0.0f, 0.0f, 1.0f, -8.0f, 0.0f,
                                                               m_paneContainer, 1.0f, 0.0f, 1.0f,
                                                               0.0f, 0.0f, 0);
                        failed = false;
                    }
                }
            }
        }
    }
    if (failed)
        return false;

    BrickPaletteObserver::Attach(GetSceneControl()->GetBrickPalette());
    return true;
}

// Mac 0x18d1a0, Win 0x62c352
void EditMode::UpdateFilterControl()
{
    m_filterItems.Clear();
    m_filterControl->SetSize(Vector2(500.0f, 250.0f));

    // The boxes by brand (index = Bag::m_brand); boxes with their own colour grid first.
    BrickPalette::BagArray bags[3];
    BrickPalette* palette = LDDApplication::GetInstance()->GetBrickPalette();
    for (BrickPalette::BagArray::ConstIterator it = palette->GetBagsBegin();
         it != palette->GetBagsEnd(); ++it) {
        BrickPalette::BagArray& group = bags[(*it)->m_brand];
        if (!(*it)->m_brandFilter)
            group.PushBack(*it);
        else
            group.Insert(group.Begin(), 1, *it);
    }

    Real32 width = 160.0f;
    for (UInt32 brand = 0; brand < 3; ++brand) {
        for (BrickPalette::BagArray::ConstIterator it = bags[brand].Begin();
             it != bags[brand].End(); ++it) {
            RefPointer<Control> item = CreateFilterItem(*it);
            LDDApplication::GetInstance()->AddButtonSound(item);
            if (width <= item->GetSize().x)
                width = item->GetSize().x;
            m_filterItems.PushBack(item);
            const BrickPalette::Bag* bag = static_cast<BoxFilterControl*>(item.Get())->GetBag();
            if (bag->m_brand == static_cast<UInt32>(LDDApplication::GetInstance()->GetBrand())
                && bag->m_brandFilter)
                m_filterControl->SelectItem(item);
        }
    }

    if (!m_filterItems.IsEmpty()) {
        m_filterControl->SetItems(m_filterItems, Vector2(0.0f, 20.0f));
        m_filterControl->SetSize(Vector2(width + 40.0f, m_filterControl->GetSize().y));
    }
}

// Mac 0x18fc60
RefPointer<Control> EditMode::CreateFilterItem(const BrickPalette::Bag* bag)
{
    RefPointer<BoxFilterControl> item = new BoxFilterControl(GetManager(), bag);
    if (!item->Create(m_texture, Vector3(3.0f, 3.0f, 3.0f), Vector3(3.0f, 3.0f, 3.0f),
                      m_filterControl, 0))
        return RefPointer<Control>();

    // Image positions of the states in EditMode_Final.png (up, then the over / down / disabled
    // variants of the unchecked and checked box).
    static const Real32 kPositions[8][2] = {
        { 256.0f, 190.0f }, { 360.0f, 205.0f }, { 360.0f, 205.0f }, { 360.0f, 205.0f },
        { 360.0f, 223.0f }, { 360.0f, 223.0f }, { 360.0f, 223.0f }, { 360.0f, 223.0f }
    };
    for (UInt32 i = 0; i < 8; ++i) {
        item->GetImagePosition(i) = Vector2(kPositions[i][0], kPositions[i][1]);
        if (item->GetState() == i)
            item->UpdateState();
    }
    LDDApplication::GetInstance()->AddButtonSound(item);
    return RefPointer<Control>(item.Get());
}

// Mac 0x18e2d0, Win 0x6283d0
void EditMode::Enter()
{
    m_paneContainer->SetShown(true);
    GetSceneManager()->SetDefaultTool(m_singleSelectionTool);
    GetSceneManager()->SetActiveTool(m_lastActiveTool);
    GetSceneControl()->SetGridVisible(true);
    GetSceneControl()->SetAllWiresSelected(true);
    if (!GetSceneControl()->IsSceneEmpty())
        GetSceneControl()->SetCameraLookAt(m_cameraPosition, m_cameraTarget, m_cameraUp);
    LDDApplication::GetInstance()->SetReady(true);
    LDDApplication::GetInstance()->SetBuildingGuideCommand(m_buildingGuideCommand);
    BrickMode::Enter();
}

// Mac 0x18e3d0, Win 0x628485
void EditMode::Exit()
{
    GetSceneManager()->SetActiveTool(0);
    LDDApplication::GetInstance()->SetReady(false);
    GetSceneControl()->ClearSelection();
    GetSceneControl()->ShowAllBricks();
    if (!GetSceneControl()->IsSceneEmpty()) {
        m_cameraPosition = GetSceneControl()->GetCameraPosition();
        m_cameraTarget = GetSceneControl()->GetCameraTarget();
        m_cameraUp = GetSceneControl()->GetCameraUp();
    }
    m_paneContainer->SetShown(false);
    BrickMode::Exit();
}

// Mac 0x18e4d0, Win 0x628535
bool EditMode::CanExit()
{
    m_lastActiveTool = GetSceneManager()->GetActiveTool();
    if (m_lastActiveTool)
        m_lastActiveTool = m_lastActiveTool->GetTool();
    return true;
}

BrickConnectSceneControlTool* EditMode::GetDefaultTool() const
{
    return m_singleSelectionTool;
}

// Mac 0x18e510, Win 0x628557
void EditMode::ShowStartHelp()
{
    GetSceneManager()->SetActiveTool(m_singleSelectionTool);
    RefPointer<LuaControl> lua = LuaControl::FromControl(this);
    CallLuaMethod(lua.Get(), "ShowStartHelp", false, 0);
}

// Mac 0x18f640, Win 0x62cdf3
void EditMode::OnBrandChanged(Brand /*brand*/)
{
    GetSceneManager()->SetActiveTool(0);
    RefPointer<LuaControl> lua = LuaControl::FromControl(this);
    bool extended = LDDApplication::GetInstance()->IsBrandLDDExtended();
    {
        LuaPlus::LuaObject self = GetLuaObject(lua.Get());
        CallLuaMethod(self, "SetFreeBuildMode", false, 0, extended);
    }
    m_brickPaletteControl->ClearFilter();
    UpdateFilterControl();
}

// Mac 0x18f780 (copy 0x18f820), Win 0x6285aa
bool EditMode::OnQuery10()
{
    RefPointer<LuaControl> lua = LuaControl::FromControl(this);
    CallLuaMethod(lua.Get(), "HideStartHelp", false, 0);
    return true;
}

// Mac 0x18f8c0 / 0x18fab0 / 0x18fae0 / 0x18fb10 → 0x18f8d0
void EditMode::OnBagAdded(const BrickPalette::Bag* /*bag*/)        { UpdateBrickPaletteControl(); }
void EditMode::OnBrickAdded(const BrickPalette::Brick* /*brick*/)  { UpdateBrickPaletteControl(); }
void EditMode::OnPaletteChanged()                                  { UpdateBrickPaletteControl(); }
void EditMode::OnAttached(BrickPalette* /*palette*/)               { UpdateBrickPaletteControl(); }

// Mac 0x18f8d0, Win 0x62867b
void EditMode::UpdateBrickPaletteControl()
{
    BrickPalette* palette = BrickPaletteObserver::GetSubject();
    if (!palette)
        return;
    s_setBricksTimer.Start();
    m_brickPaletteControl->SetBrickPalette(palette);
    s_setBricksTimer.Stop();

    RefPointer<LuaScriptManager> scripts = LuaScriptManager::GetInstance();
    LuaPlus::LuaObject globals = scripts->GetLuaState()->GetGlobals();
    LuaPlus::LuaObject paletteObject = globals.Lookup("editMode.palette");
    CallLuaMethod(paletteObject, "UpdateBrickPaletteControl", false, 0);

    m_materialGridControl->SetMaterialGrid(
        palette->GetMaterialGrid(*GetSceneManager()->GetDatabase()),
        LDDApplication::GetInstance()->IsBrandLDDExtended() ? 0 : 0x3c);
    m_materialItemControl->SetMaterialGrid(palette->GetMaterialGrid(*GetSceneManager()->GetDatabase()));
}

// Mac 0x18fb40, Win 0x6285f1
void EditMode::OnMaterialSelected(UInt32 material)
{
    if (!LDDApplication::GetInstance()->IsBrandLDDExtended()) {
        RefPointer<BrickFilter> filter = new MaterialBrickFilter(material);
        m_brickPaletteControl->AddFilter(filter);
    } else {
        m_brickPaletteControl->SelectMaterial(material);
    }
}

// ===========================================================================================
// PickupBricksObserver
// ===========================================================================================

// Mac 0x18bce0, Win 0x62ce75
PickupBricksObserver::PickupBricksObserver(BrickConnectSceneManager* sceneManager,
                                           BrickConnectSceneControl* sceneControl,
                                           ClickStickTool* clickStickTool)
    : m_sceneManager(sceneManager), m_sceneControl(sceneControl), m_clickStickTool(clickStickTool)
{
    LoadSound();
    PreferencesObserver::Attach(Preferences::GetInstance());
}

// Mac 0x1904f0 / 0x190610 (body 0x1903a0), Win sdd 0x62a1cf
PickupBricksObserver::~PickupBricksObserver()
{
}

void PickupBricksObserver::LoadSound()
{
    m_sound = LDDUtilities::GetInstance()->CreateSound(String("IA_pick-up.mp3"));
}

// Mac 0x190770, Win 0x62cfa3
void PickupBricksObserver::OnFinalRelease()
{
    PreferencesObserver::Detach();
    // NOTE: the notifiers are left without OnDetached (both builds).
    for (UInt32 i = 0; i < m_subjects.GetSize(); ++i)
        static_cast<NotifierT<BricksControlItemPressedNotifier, BricksControlItemPressedObserver>*>(
            m_subjects[i])->RemoveObserver(this);
    m_subjects.Clear();
    Object::OnFinalRelease();
}

// Mac 0x190980, Win 0x62a1ee
void PickupBricksObserver::OnBrickPressed(PrimitiveElementBrick* brick)
{
    m_sceneManager->SetActiveTool(m_clickStickTool);
    if (m_sound)
        m_sound->Play();
    BasicArray<ObjectRefPointer<PrimitiveElementBrick> > bricks;
    bricks.PushBack(ObjectRefPointer<PrimitiveElementBrick>(brick));
    m_clickStickTool->PickUp(bricks, true, 0, false, true);
    m_sceneControl->SetKeyboardFocus(true);
    m_sceneControl->SetCursor(101, true);
}

// Mac 0x190b30, Win 0x628796
void PickupBricksObserver::OnModelPressed(StorageScene2* model)
{
    m_sceneManager->SetActiveTool(m_clickStickTool);
    LDDApplication::GetInstance()->ImportStorage(model, m_clickStickTool);
}

// Mac 0x190b70, Win 0x6287c7
void PickupBricksObserver::OnPreferenceChanged(const String& key)
{
    if (key.EqualsNoCase(String("PLAYSOUND")))
        LoadSound();
}

// ===========================================================================================
// Selection tools with fixed filters
// ===========================================================================================

// Mac 0x18c0e0, Win 0x629b16
ConnectedSelectionTool::ConnectedSelectionTool(ClickStickTool* clickStickTool)
    : SelectionTool(clickStickTool, false, true)
{
    AddFilter(new IncludeModelOnlyFilter);
    SetCursor(103);
}

// Mac 0x18c210, Win 0x629c6b
SameColorSelectionTool::SameColorSelectionTool(ClickStickTool* clickStickTool)
    : SelectionTool(clickStickTool, false, true)
{
    AddFilter(new IncludeSceneOnlyFilter);
    AddFilter(new IncludeMaterialOnlyFilter);
    SetCursor(104);
}

// Mac 0x18c3b0, Win 0x629e16
SameShapeSelectionTool::SameShapeSelectionTool(ClickStickTool* clickStickTool)
    : SelectionTool(clickStickTool, false, true)
{
    AddFilter(new IncludeSceneOnlyFilter);
    AddFilter(new IncludeDesignOnlyFilter);
    SetCursor(105);
}

// Mac 0x18c550, Win 0x629fc1
SameColorAndDesignSelectionTool::SameColorAndDesignSelectionTool(ClickStickTool* clickStickTool)
    : SelectionTool(clickStickTool, false, true)
{
    AddFilter(new IncludeSceneOnlyFilter);
    AddFilter(new IncludeMaterialOnlyFilter);
    AddFilter(new IncludeDesignOnlyFilter);
    SetCursor(106);
}

// ===========================================================================================
// Tool commands
// ===========================================================================================

// Mac 0x18d9b0, Win 0x62c722
SetToolCommand::SetToolCommand(BrickConnectSceneManager* sceneManager,
                               BrickConnectSceneControlTool* tool, bool enabled)
    : Command(enabled, true), m_tool(tool)
{
    BrickConnectSceneManagerObserver::Attach(sceneManager);
}

// Mac 0x1939f0
void SetToolCommand::OnFinalRelease()
{
    BrickConnectSceneManagerObserver::Detach();
    while (!m_observers.IsEmpty())
        DetachObserver(m_observers.Back(), this);
    Object::OnFinalRelease();
}

// Mac 0x193a80, Win 0x62ab37
void SetToolCommand::Do()
{
    BrickConnectSceneManagerObserver::GetSubject()->SetActiveTool(m_tool);
}

// Mac 0x193aa0
void SetToolCommand::UpdateEnable()
{
    SetEnable(true);
}

// Mac 0x193ac0
void SetToolCommand::OnActiveToolChanged(BrickConnectSceneControlTool* /*previous*/,
                                         BrickConnectSceneControlTool* current)
{
    SetToggled(m_tool == current);
}

// Mac 0x193e10 (C2), Win 0x62c7db
SetSelectionToolCommand::SetSelectionToolCommand(BrickConnectSceneManager* sceneManager,
                                                 const ToolArray& tools)
    : SetToolCommand(sceneManager, tools[0], true), m_tools(tools)
{
}

// Mac 0x193fe0
void SetSelectionToolCommand::OnActiveToolChanged(BrickConnectSceneControlTool* /*previous*/,
                                                  BrickConnectSceneControlTool* current)
{
    if (current) {
        RefPointer<BrickConnectSceneControlTool> keep(current);
        const RefPointer<BrickConnectSceneControlTool>* it = m_tools.Begin();
        while (it != m_tools.End() && it->Get() != current)
            ++it;
        if (it != m_tools.End())
            SelectTool(it);
    }
    SetToggled(m_tool == current);
}

// Mac 0x1943a0
void SelectActiveSelectionToolCommand::UpdateEnable()
{
    SetEnable(true);
}

// Mac 0x1943c0, Win 0x62c8ee
void SelectActiveSelectionToolCommand::SelectTool(const RefPointer<BrickConnectSceneControlTool>* it)
{
    m_tool = *it;
}

// Mac 0x194960
void SelectNextSelectionToolCommand::UpdateEnable()
{
    SetEnable(true);
}

// Mac 0x194980, Win 0x62c989
void SelectNextSelectionToolCommand::SelectTool(const RefPointer<BrickConnectSceneControlTool>* it)
{
    const RefPointer<BrickConnectSceneControlTool>* next = it + 1;
    if (next == m_tools.End())
        next = m_tools.Begin();
    m_tool = *next;
}

// Mac 0x18dc50, Win 0x62cb91
SetFreeBuildToolCommand::SetFreeBuildToolCommand(BrickConnectSceneManager* sceneManager,
                                                 BrickConnectSceneControlTool* materialTool,
                                                 BrickConnectSceneControlTool* paintBucketTool)
    : SetToolCommand(sceneManager, materialTool, true),
      m_materialTool(materialTool), m_paintBucketTool(paintBucketTool)
{
    LDDApplicationObserver::Attach(LDDApplication::GetInstance());
}

// Mac 0x194c60
void SetFreeBuildToolCommand::OnFinalRelease()
{
    LDDApplicationObserver::Detach();
    SetToolCommand::OnFinalRelease();
}

// Mac 0x194cc0
void SetFreeBuildToolCommand::OnAttached(LDDApplication* /*application*/)
{
    UpdateTool();
}

// Mac 0x194cd0
void SetFreeBuildToolCommand::OnBrandChanged(Brand /*brand*/)
{
    UpdateTool();
}

// Mac 0x1951b0, Win 0x62cc61
void SetFreeBuildToolCommand::UpdateTool()
{
    BrickConnectSceneControlTool* wanted =
        LDDApplicationObserver::GetSubject()->IsBrandLDDExtended() ? m_paintBucketTool.Get()
                                                                   : m_materialTool.Get();
    if (m_tool == wanted)
        return;
    BrickConnectSceneManager* sceneManager = BrickConnectSceneManagerObserver::GetSubject();
    if (sceneManager->GetActiveTool() == m_tool)
        sceneManager->SetActiveTool(0);
    m_tool = wanted;
}

// Mac 0x18ddb0, Win 0x62c9fd
SetBrandCommand::SetBrandCommand(Brand brand)
    : Command(false, false), m_brand(brand)
{
    LDDApplicationObserver::Attach(LDDApplication::GetInstance());
}

// Mac 0x195360, Win 0x62cb7d
void SetBrandCommand::OnFinalRelease()
{
    LDDApplicationObserver::Detach();
    while (!m_observers.IsEmpty())
        DetachObserver(m_observers.Back(), this);
    Object::OnFinalRelease();
}

// Mac 0x1953f0, Win 0x62ca9a
void SetBrandCommand::Do()
{
    LDDApplication::GetInstance()->SetBrand(m_brand);
}

// Mac 0x195410, Win 0x62caaa
void SetBrandCommand::UpdateEnable()
{
    SetEnable(LDDApplication::GetInstance()->GetBrand() != m_brand);
}

// Mac 0x195450
void SetBrandCommand::OnAttached(LDDApplication* /*application*/)
{
    UpdateEnable();
}

// Mac 0x195460, Win 0x62cace
void SetBrandCommand::OnBrandChanged(Brand brand)
{
    SetEnable(m_brand != brand);
}

// Mac 0x195840, Win 0x62b8f6
void RemoveMaterialFilterCommand::Do()
{
    if (LDDApplication::GetInstance()->IsBrandLDDExtended())
        return;
    // NOTE: 0x2c is a dummy material — RemoveFilter only looks at the filter type.
    RefPointer<BrickFilter> filter = new MaterialBrickFilter(0x2c);
    m_editMode->GetBrickPaletteControl()->RemoveFilter(filter);
    m_editMode->GetMaterialGridControl()->SetSelectedMaterial(0);
}

// Mac 0x18ded0, Win 0x62b9a8
HideMaterialsCommand::HideMaterialsCommand(EditMode* editMode)
    : Command(true, true), m_editMode(editMode)
{
    m_editMode->GetBrickPaletteControl()->SetHideMaterials(IsToggled());
}

// Mac 0x195b60, Win 0x62ba36
void HideMaterialsCommand::Do()
{
    m_editMode->GetBrickPaletteControl()->SetHideMaterials(IsToggled());
}

// Mac 0x195da0, Win 0x62b087
void ToggleDoCanConnectPhysicsTestCommand::Do()
{
    m_test = !m_test;
    m_sceneControl->GetScene()->SetTestCollisions(m_test);
}

// Mac 0x195dd0
void ToggleDoCanConnectPhysicsTestCommand::UpdateEnable()
{
    SetEnable(true);
}

} // namespace LEGO
