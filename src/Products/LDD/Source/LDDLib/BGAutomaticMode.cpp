// LEGO Digital Designer — restored source
// TU 367 "BGAutomaticMode.cpp": Mac 0x352ed0-0x35973b, Win 0x62476c-0x627523. See
// BGAutomaticMode.h. Not code: Mac 0x3596c0 = <iostream> + a header static.
#include "BGAutomaticMode.h"

#include <iostream>

#include "LEGO/String.h"
#include "Texture.h"
#include "Color.h"
#include "Key.h"
#include "Updater.h"
#include "ControlManager.h"
#include "RenderControl.h"
#include "RenderControlManager.h"
#include "SliderControl.h"
#include "Scene.h"
#include "BrickPhysics.h"
#include "LuaPlus.h"
#include "LuaBindings.h"
#include "LuaScriptManager.h"
#include "LuaControl.h"
#include "LuaCommandAdapter.h"
#include "LuaBrickMode.h"
#include "LDDApplication.h"
#include "LDDUtilities.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneManager.h"
#include "BrickModeGlobalObject.h"
#include "BricksControl.h"
#include "EditMode.h"
#include "ViewMode.h"
#include "BI.h"
#include "BIStorage.h"
#include "BIBrick.h"
#include "BIEvent.h"
#include "BIGenerator.h"
#include "BIProgramGeneric.h"
#include "BGAutomaticApplication.h"
#include "GenerateBuildingGuideCommand.h"
#include "Commands/ModeNavigationCommand.h"
#include "Commands/BGModeCommand.h"
#include "Commands/UploadModelCommand.h"
#include "Commands/HTMLBICommand.h"
#include "Commands/NextStepBICommand.h"
#include "Commands/NextPartBICommand.h"
#include "Commands/PrevStepBICommand.h"
#include "Commands/PrevPartBICommand.h"
#include "Commands/ReplayPartBICommand.h"
#include "Commands/BGScreenShotCommand.h"

namespace LEGO {

namespace {

// Calls the Lua method `name` of `control`'s Lua table (no results). Mac 0x352ed0
void CallControlMethod(Control* control, const char* name)
{
    RefPointer<LuaScriptManager> scripts = LuaScriptManager::GetInstance();
    LuaPlus::LuaObject table = LuaControl::GetControlTable(scripts->GetLuaState(), control);
    CallLuaMethod(table, name, false, 0);
}

// Setup of a plain child control: no parent yet, flags 7, empty rectangle.
bool SetupControl(Control* control, bool hitTestable)
{
    return control->Setup(0, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, hitTestable);
}

} // namespace

// Mac 0x352f90, Win 0x625200
BGAutomaticMode::BGAutomaticMode(ControlManager* manager, Window* window,
                                 BrickConnectSceneControl* sceneControl)
    : BGMode(manager, window, sceneControl),
      m_lastAction(0),
      m_unused368(false),
      m_unused36c(0),
      m_unused370(0),
      m_keepGuide(false),
      m_unused378(0),
      m_unused37c(0),
      m_unused380(0)
{
    m_application = new BGAutomaticApplication(
        static_cast<RenderControlManager*>(sceneControl->GetManager())->GetRender());
}

// Mac 0x3567a0 (D1) / 0x356c60 (D0) (members and bases 0x355d80), Win sdd 0x6259b3
BGAutomaticMode::~BGAutomaticMode()
{
}

// Mac 0x3571d0
void BGAutomaticMode::OnFinalRelease()
{
    if (IsCurrent())
        Exit();
    BGAutomaticApplicationObserver::OnFinalRelease();
    SliderControlObserver::OnFinalRelease();
    PreferencesObserver::OnFinalRelease();
    BGMode::OnFinalRelease();
}

// Mac 0x3536a0, Win 0x6260dd
bool BGAutomaticMode::Initialize(EditMode* editMode, ViewMode* viewMode, Control* parent, Control* after)
{
    if (!BrickMode::Create(parent, after))
        return false;
    m_pauseSound = CreateLDDSound(String("IA_pause.mp3"));
    m_editMode = editMode;
    ControlManager* manager = GetManager();

    // ---- commands ----
    m_editNavigation = new ModeNavigationCommand(editMode);
    // NOTE: the BG-mode, view-navigation, HTML and upload commands are only referenced while
    // Initialize runs (both builds); the HTML command lives on through its Lua object.
    RefPointer<Command> bgModeCommand = new BGModeCommand(this, GetSceneControl());
    RefPointer<Command> viewNavigation = new ModeNavigationCommand(viewMode);
    m_nextStep = new NextStepBICommand(this);
    m_nextPart = new NextPartBICommand(this, true);
    m_prevStep = new PrevStepBICommand(this);
    m_prevPart = new PrevPartBICommand(this, true);
    m_replayPart = new ReplayPartBICommand(this);
    m_generateGuide = new GenerateBuildingGuideCommand(GetSceneControl());
    m_screenShot = new BGScreenShotCommand(GetSceneControl(), this);
    RefPointer<Command> html = new HTMLBICommand(GetSceneControl(), static_cast<RenderControlManager*>(manager),
                                                 GetSceneManager(), this, m_window);
    RefPointer<Command> uploadModel = new UploadModelCommand(GetSceneControl(), m_editNavigation);

    // ---- key short-cuts ----
    GetKeyboardShortCuts()->AddKeyboardShortCut(m_prevPart, kKeyLeftArrow, 0);
    GetKeyboardShortCuts()->AddKeyboardShortCut(m_nextPart, kKeyRightArrow, 0);
    GetKeyboardShortCuts()->AddKeyboardShortCut(m_prevStep, kKeyPageUp, 0);
    GetKeyboardShortCuts()->AddKeyboardShortCut(m_nextStep, kKeyPageDown, 0);
    GetKeyboardShortCuts()->AddKeyboardShortCut(m_replayPart, kKeySpace, 0);

    // ---- textures ----
    Render* render = static_cast<RenderControlManager*>(GetManager())->GetRender();
    m_icons = LoadApplicationTexture(String("Graphics/General/Icon_BI2.png"), render);
    if (!m_icons)
        return false;
    m_graphics = LoadApplicationTexture(String("Graphics/LDDgraphics01.png"),
                                        static_cast<RenderControlManager*>(GetManager())->GetRender());
    if (!m_graphics)
        return false;

    // ---- the step preview panel: 2 x 2 cells of 100 x 100, bottom right of the scene ----
    m_previewPanel = new RenderControl(manager);
    if (!SetupControl(m_previewPanel, false))
        return false;
    m_previewPanel->SetSize(Vector2(200.0f, 200.0f));
    m_previewPanel->SetAlignment(0x11, 0x11, -20.0f, -20.0f, 0);
    m_previewPanel->SetParent(GetSceneControl(), 0);
    m_previewPanel->SetEnabled(true);
    m_previewPanel->SetShown(false);
    m_previewPanel->SetTransparent(true);
    const Rect cellImages[4] = {
        Rect(Vector2(209.0f, 217.0f), 100.0f, 100.0f), Rect(Vector2(311.0f, 217.0f), 100.0f, 100.0f),
        Rect(Vector2(209.0f, 319.0f), 100.0f, 100.0f), Rect(Vector2(311.0f, 319.0f), 100.0f, 100.0f)
    };
    for (Int32 i = 0; i < 4; ++i) {
        m_previewCells[i] = new RenderControl(manager);
        if (!SetupControl(m_previewCells[i], false))
            return false;
        m_previewCells[i]->SetSize(Vector2(100.0f, 100.0f));
        m_previewCells[i]->SetPosition(Vector2((Real32)(i % 2) * 100.0f, (i > 1 ? 1.0f : 0.0f) * 100.0f));
        m_previewCells[i]->SetParent(m_previewPanel, 0);
        m_previewCells[i]->SetTransparent(true);
        m_previewCells[i]->SetImage(m_icons, &cellImages[i]);
        m_previewCells[i]->SetShown(true);
    }

    // ---- the substep preview frames (top, middle, bottom) ----
    const Rect frameImages[3] = {
        Rect(Vector2(1.0f, 29.0f), 200.0f, 34.0f), Rect(Vector2(1.0f, 65.0f), 200.0f, 41.0f),
        Rect(Vector2(1.0f, 108.0f), 200.0f, 8.0f)
    };
    for (Int32 i = 0; i < 3; ++i) {
        m_frames[i] = new RenderControl(manager);
        if (!SetupControl(m_frames[i], false))
            return false;
        m_frames[i]->SetParent(GetSceneControl(), 0);
        m_frames[i]->SetImage(m_icons, &frameImages[i]);
        m_frames[i]->SetSize(frameImages[i].GetSize());
        m_frames[i]->SetShown(false);
    }
    m_frames[0]->SetAlignment(0x11, 0x11, -20.0f, -220.0f, 0);
    m_frames[1]->SetAlignment(0x11, 0x31, 0.0f, -173.0f, m_frames[0]);
    m_frames[2]->SetAlignment(0x11, 0x31, 0.0f, -173.0f, m_frames[1]);

    // ---- the step texts ----
    m_totalText = CreateTextControl(GetSceneControl(), manager, LDDApplication::GetInstance()->GetFont(0xd),
                                    String(""));
    if (!m_totalText)
        return false;
    m_totalText->SetShown(false);
    m_currentText = CreateTextControl(GetSceneControl(), manager, LDDApplication::GetInstance()->GetFont(0x10),
                                      String(""));
    if (!m_currentText)
        return false;
    m_currentText->SetShown(false);
    SetStepTexts(0, 0);

    // ---- the step slider ----
    m_slider = new SliderControl(manager, 0.0f, 1.0f, 1.0f, 0.0f);
    if (!m_slider->Initialize())
        return false;
    SetupControl(m_slider, true);
    Rect track(Vector2(587.0f, 45.0f), Vector2(150.0f, 32.0f));
    Rect thumbUp(Vector2(587.0f, 10.0f), Vector2(10.0f, 30.0f));
    Rect thumbOver(Vector2(599.0f, 10.0f), Vector2(10.0f, 30.0f));
    Rect thumbDown(Vector2(611.0f, 10.0f), Vector2(10.0f, 30.0f));
    Rect thumbDisabled(6.0f, 0.0f, 0.0f, 0.0f);
    Rect trackMask(0.0f, 0.0f, 0.0f, 0.0f);
    Vector2 trackMaskPoint;
    Rect thumbMask(0.0f, 0.0f, 0.0f, 0.0f);
    Vector2 thumbMaskPoint;
    // NOTE: the "disabled" thumb rect is (6, 0, 0, 0) and the track y 0 (both builds).
    m_slider->SetImages(m_graphics, track, thumbUp, thumbOver, thumbDown, thumbDisabled, 0.0f, 0,
                        trackMask, trackMaskPoint, thumbMask, thumbMaskPoint);

    // ---- Lua ----
    bool failed = true;
    {
        RefPointer<LuaScriptManager> scripts = LuaScriptManager::GetInstance();
        LuaPlus::LuaState* state = scripts->GetLuaState();
        LuaPlus::LuaObject globals = state->GetGlobals();

        LuaPlus::LuaObject commands = globals.Lookup("LDD.commands.bgAutomaticMode");
        LuaPlus::LuaState* commandsState = commands.GetState();
        commands.Set("html", LuaCommandAdapter::CreateLuaObject(html, commandsState));
        commands.Set("nextStep", LuaCommandAdapter::CreateLuaObject(m_nextStep, commandsState));
        commands.Set("nextPart", LuaCommandAdapter::CreateLuaObject(m_nextPart, commandsState));
        commands.Set("prevStep", LuaCommandAdapter::CreateLuaObject(m_prevStep, commandsState));
        commands.Set("prevPart", LuaCommandAdapter::CreateLuaObject(m_prevPart, commandsState));
        commands.Set("replayPart", LuaCommandAdapter::CreateLuaObject(m_replayPart, commandsState));
        commands.Set("screenShot", LuaCommandAdapter::CreateLuaObject(m_screenShot, commandsState));

        LuaPlus::LuaObject controls = globals.Lookup("LDD.controls.bgAutomaticMode");
        controls.Set("slider", LuaControl::CreateControlTable(controls.GetState(), m_slider));

        LuaPlus::LuaObject mode = globals.GetByName("bgAutomaticMode");
        if (mode.IsTable()) {
            SetLuaBaseClass(mode, "BrickMode");
            RefPointer<LuaBrickMode> luaMode = new LuaBrickMode;
            if (luaMode->Bind(this, mode) && CallLuaMethod(mode, "Create", false, 1)) {
                LuaPlus::LuaObject created(state, -1);
                state->Pop(1);
                failed = !LuaToBoolean(created);
            }
        }
    }
    return !failed;
}

// Mac 0x3558d0
void BGAutomaticMode::SetStepTexts(UInt32 current, UInt32 total)
{
    {
        String text;
        m_totalText->SetText(text << "/" << total);
    }
    m_totalText->ResizeHeightToFitText(0);
    {
        String text;
        m_currentText->SetText(text << current);
    }
    m_currentText->ResizeHeightToFitText(0);
    m_currentText->SetAxisAlignment(0, 3, 1, 0.0f, m_totalText);
    m_currentText->SetAxisAlignment(1, 1, 1, -40.0f, 0);
    m_totalText->SetAxisAlignment(0, 3, 3, 20.0f, 0);
    m_totalText->SetAxisAlignment(1, 3, 3, 8.0f, m_currentText);
}

// Mac 0x355ac0, Win 0x624ceb
void BGAutomaticMode::ClearPreviews()
{
    for (UInt32 i = 0; i < 4; ++i)
        m_previews[i] = 0;
    m_slider->SetMinMaxStep(0.0f, 1.0f, 1.0f, 0.0f);
    SetStepTexts(0, 0);
    m_modelPreview = 0;
    m_substepPreview = 0;
}

// Mac 0x355c00, Win 0x62581b
void BGAutomaticMode::ShowOverlays(bool show)
{
    if (!show) {
        for (UInt32 i = 0; i < 4; ++i)
            m_previews[i] = 0;
        m_substepPreview = 0;
        m_modelPreview = 0;
        for (UInt32 i = 0; i < 3; ++i)
            m_frames[i]->SetShown(false);
    }
    m_currentText->SetShown(show);
    m_totalText->SetShown(show);
    m_previewPanel->SetShown(show);
}

// Mac 0x357440, Win 0x6259d5
void BGAutomaticMode::UpdatePreviews()
{
    for (UInt32 i = 0; i < 4; ++i)
        m_previews[i] = 0;
    BIGenerator* generator = m_application->GetGenerator();
    if (!generator->GetEvent(0))
        return;
    // Back to the step event before the current position: how many parts of the step are shown.
    Int32 offset = 0;
    Int32 count = 0;
    if (m_application->GetGenerator()->GetEvent(0)) {
        do {
            if (m_application->GetGenerator()->GetEvent(offset)->m_type == BIEvent::TypeStep)
                break;
            ++count;
            --offset;
        } while (m_application->GetGenerator()->GetEvent(offset));
    }
    if (!m_application->GetGenerator()->GetEvent(offset))
        return;
    BasicArray<RefPointer<BIEvent> > events;
    m_application->GetGenerator()->GetCurrentStepBricks(events);
    BasicArray<BIBrick*> bricks;
    BI::GetBricks(events, bricks);
    if (bricks.GetSize() == 1 && bricks[0]->GetType() == BIBrick::TypeSubstep) {
        m_previews[0] = CreateBrickPreview(bricks[0]);
        m_previews[0]->SetParent(m_previewPanel, 0);
        m_previews[0]->SetSize(Vector2(200.0f, 200.0f));
        m_previews[0]->SetEnabled(true);
        m_previews[0]->SetShown(true);
        return;
    }
    // The group of four the current part is in (the last full group at the step's end).
    UInt32 first = (UInt32)((count / 4) * 4);
    UInt32 size = bricks.GetSize();
    UInt32 start = first;
    if (first == size) {
        start = first - 4;
        if (first < 4)
            start = 0;
    }
    UInt32 shown = size - start;
    if (shown > 4)
        shown = 4;
    for (UInt32 i = 0; i < shown; ++i) {
        m_previews[i] = CreateBrickPreview(bricks[start + i]);
        m_previews[i]->SetParent(m_previewCells[i], 0);
        m_previews[i]->SetSize(Vector2(100.0f, 100.0f));
        m_previews[i]->SetEnabled(true);
        m_previews[i]->SetShown(true);
    }
}

// Mac 0x357ae0
BricksControl* BGAutomaticMode::CreateSubstepPreview(BIBrick* brick, Real32 offsetY, const Rect& imageRect)
{
    if (!brick)
        return 0;
    BricksControl* control = CreateBrickPreview(brick);
    // NOTE: CreateBrickPreview is null for a brick without primitives; both builds crash below.
    // Fixed: null.
    if (!control)
        return 0;
    control->SetParent(GetSceneControl(), 0);
    control->SetSize(Vector2(200.0f, 173.0f));
    control->SetEnabled(true);
    control->SetShown(true);
    control->SetAlignment(0x11, 0x11, -20.0f, -offsetY, 0);
    control->SetImage(m_icons, &imageRect);
    control->SetTransparent(false);
    return control;
}

// Mac 0x357900
void BGAutomaticMode::SetSubstepPreview(bool show, BIBrick* brick)
{
    if (!brick || !show) {
        m_substepPreview = 0;
        return;
    }
    Rect imageRect(Vector2(10.0f, 50.0f), Vector2(10.0f, 10.0f));
    m_substepPreview = CreateSubstepPreview(brick, 254.0f, imageRect);
}

// Mac 0x3579f0
void BGAutomaticMode::SetModelPreview(bool show, BIBrick* brick)
{
    if (!brick || !show) {
        m_modelPreview = 0;
        return;
    }
    Rect imageRect(Vector2(20.0f, 90.0f), Vector2(10.0f, 10.0f));
    m_modelPreview = CreateSubstepPreview(brick, 468.0f, imageRect);
}

// Mac 0x358c10, Win 0x62735a
void BGAutomaticMode::UpdateSubstepPreview()
{
    BGAutomaticApplication* application = m_application;
    if (application->GetGeneratorCount() == 1 && application->GetGenerator()->GetCurrent() == 0) {
        m_application->MarkLevel();
        return;
    }
    if (m_application->IsBelowMarkedLevel() || m_application->IsAboveMarkedLevel()) {
        UInt32 levels = m_application->GetGeneratorCount();
        if (levels == 1 || (levels == 2 && m_application->GetModelCount() >= 2)) {
            CallControlMethod(this, "HidePreview");
            m_substepPreview = 0;
            m_modelPreview = 0;
            for (UInt32 i = 0; i < 3; ++i)
                m_frames[i]->SetShown(false);
        } else {
            CallControlMethod(this, "ShowPreview");
            BIGenerator* parentGenerator = m_application->GetParentGenerator();
            RefPointer<BIBrick> model = parentGenerator->MakeBrick(0, parentGenerator->GetCurrent());
            BIGenerator* generator = m_application->GetGenerator();
            Int32 offset = 0;
            UInt32 end;
            if (generator->FindBrickOfType(BIBrick::TypeSubstep, offset, 1))
                end = generator->GetCurrent() + offset - 1;
            else
                end = generator->GetCompleteCount();
            RefPointer<BIBrick> substep = generator->MakeBrick(0, end);
            SetSubstepPreview(true, substep);
            SetModelPreview(true, model);
            for (UInt32 i = 0; i < 3; ++i)
                m_frames[i]->SetShown(true);
        }
    }
    if (m_application->IsAboveMarkedLevel()) {
        BIEvent* event = 0;
        if (m_lastAction == 1)
            event = m_application->GetGenerator()->FindNextBrickOfType(BIBrick::TypeSubstep, 1);
        if (m_lastAction == 2)
            event = m_application->GetGenerator()->FindPreviousBrickOfType(BIBrick::TypeSubstep, 0);
        if (event && m_lastAction == 1)
            m_activeParts.Clear();
    }
    m_application->MarkLevel();
}

// Mac 0x357bf0, Win 0x6270b7
void BGAutomaticMode::Enter()
{
    GetSceneControl()->SetBuildingGuideMode(true);
    GetSceneControl()->SetShowStatistics(false);
    GetSceneControl()->GetScene()->GetPhysicsScene()->Unknown5(true);
    {
        BGAutomaticApplication* application = m_application;
        Scene* scene = GetSceneControl()->GetScene();
        RefPointer<BIStorage> storage = GetSceneControl()->GetBuildingInstructions()->GetStorage();
        if (!application->Start(scene, storage))
            return;
    }
    BGAutomaticApplicationObserver::Attach(m_application);
    BrickMode::Enter();
    m_nextPart->SetSubsteps(true);
    m_prevPart->SetSubsteps(true);
    GetSceneControl()->SetGridVisible(false);
    ClearPreviews();
    GetSceneControl()->SetClearColor(Color(0.4f, 0.55f, 0.79f, 1.0f));
    GetSceneManager()->SetDefaultTool(BrickModeGlobalObject::GetNoneTool());
    GetSceneManager()->SetActiveTool(0);
    m_application->SetProgram(new BIProgramGeneric(true, true, 0.5f, 0.5f, 0.5f));
    SliderControlObserver::Attach(m_slider);
    LDDApplication::GetInstance()->SetEditCommand(m_editNavigation);
    LDDApplication::GetInstance()->SetBuildingGuideCommand(m_generateGuide);
    UpdateObserver::Attach(MainUpdater::GetInstance());
    BrickConnectSceneControlObserver::Attach(GetSceneControl());
    m_currentText->SetShown(true);
    m_totalText->SetShown(true);
    m_previewPanel->SetShown(true);
    m_keepGuide = false;
    m_application->Generate();
    m_application->UpdateView(true);
    OnViewUpdated();
    UpdatePreviews();
    StoreBuildingGuide();
    UpdateCommands();
    m_application->MarkLevel();
}

// Mac 0x3586b0, Win 0x625d61
void BGAutomaticMode::Exit()
{
    GetSceneControl()->GetScene()->GetPhysicsScene()->Unknown5(false);
    BGAutomaticApplicationObserver::Detach();
    BrickMode::Exit();
    GetSceneControl()->SetClearColor(Color(0.8f, 0.8f, 0.8f, 1.0f));
    ClearPreviews();
    ShowOverlays(false);
    BI* instructions = GetSceneControl()->GetBuildingInstructions();
    if (!m_keepGuide) {
        RefPointer<BIStorage> storage = new BIStorage;
        m_application->Finish(storage);
        instructions->SetStorage(storage);
    } else {
        m_application->Stop();
    }
    BrickConnectSceneControlObserver::Detach();
    UpdateObserver::Detach();
    SliderControlObserver::Detach(m_slider);
    GetSceneControl()->SetBuildingGuideMode(false);
    GetSceneControl()->SetShowStatistics(true);
    Reset();
}

// Mac 0x358920, Win 0x624ef8
bool BGAutomaticMode::CanEnter()
{
    if (GetSceneControl()->GetBuildingInstructions()->HasSteps())
        return true;
    if (!m_generateGuide->GetEnable())
        return false;
    m_generateGuide->Do();
    return m_generateGuide->WasGenerated();
}

// Mac 0x359090, Win 0x62476c
bool BGAutomaticMode::NextPart(bool substeps)
{
    m_lastAction = 2;
    return BGMode::NextPart(substeps);
}

// Mac 0x3590b0, Win 0x62477b
bool BGAutomaticMode::PrevPart(bool substeps)
{
    m_lastAction = 1;
    return BGMode::PrevPart(substeps);
}

// Mac 0x3590d0, Win 0x62478a
bool BGAutomaticMode::NextStep(bool unused, bool animate, BasicArray<BIBrick*>& bricks, bool setCamera)
{
    m_lastAction = 4;
    return BGMode::NextStep(unused, animate, bricks, setCamera);
}

// Mac 0x3590f0, Win 0x624799
bool BGAutomaticMode::PrevStep(bool unused, bool animate)
{
    m_lastAction = 3;
    return BGMode::PrevStep(unused, animate);
}

// Mac 0x359000, Win 0x624f3f
BGApplication* BGAutomaticMode::GetApplication()
{
    return m_application;
}

// Mac 0x359110, Win 0x624f46
void BGAutomaticMode::UpdateCommands()
{
    m_nextStep->UpdateEnable();
    m_nextPart->UpdateEnable();
    m_prevStep->UpdateEnable();
    m_prevPart->UpdateEnable();
    m_replayPart->UpdateEnable();
}

// Mac 0x3592c0 (thunk 0x359420), Win 0x62587e
void BGAutomaticMode::OnValueChanged(SliderControl* /*slider*/, Real32 value, Real32 oldValue)
{
    BasicArray<BIBrick*> bricks;
    m_application->UpdateCurrentStep();
    if (value <= oldValue) {
        while (value < (Real32)m_application->GetStepIndex())
            PrevStep(true, false);
    } else {
        while ((Real32)m_application->GetStepIndex() < value)
            NextStep(true, false, bricks, false);
    }
}

// Mac 0x359170 (thunk 0x3592a0), Win 0x624f81
void BGAutomaticMode::OnPreferenceChanged(const String& key)
{
    if (key.EqualsNoCase(String("PLAYSOUND")))
        m_pauseSound = CreateLDDSound(String("IA_pause.mp3"));
}

// Mac 0x358990 (thunk 0x3589a0)
void BGAutomaticMode::OnCandidatesChanged()
{
}

// Mac 0x3589b0 (thunks 0x358bd0 / 0x358bf0), Win 0x625ec4
void BGAutomaticMode::OnViewUpdated()
{
    m_application->UpdateCurrentStep();
    SetStepTexts(m_application->GetStepIndex(), m_application->GetStepTotal());
    UpdateCommands();
    BasicArray<RefPointer<Part> > none;
    GetSceneControl()->SetSelection(none);
    SliderControlObserver::Detach(m_slider);
    m_slider->SetMinMaxStep(0.0f, (Real32)m_application->GetStepTotal(), 1.0f,
                            (Real32)m_application->GetStepIndex());
    if (m_application->GetStepTotal() != 0)
        m_slider->SetEnabled(true);
    SliderControlObserver::Attach(m_slider);
}

// (Mac: the same function as OnViewUpdated, thunk 0x358bd0)
void BGAutomaticMode::OnEvent08()
{
    OnViewUpdated();
}

// Mac 0x359010 (thunk 0x359030), Win 0x62750b
void BGAutomaticMode::OnStepChanged(bool /*inStep*/)
{
    UpdatePreviews();
    UpdateSubstepPreview();
}

// Mac 0x359060 (thunk 0x359070)
void BGAutomaticMode::OnProgress(Real32 /*progress*/)
{
}

// Mac 0x359080, Win 0x67e76a
UInt32 BGAutomaticMode::Unknown72()
{
    return 0;
}

} // namespace LEGO
