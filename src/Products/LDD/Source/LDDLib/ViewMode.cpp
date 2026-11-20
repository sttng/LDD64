// LEGO Digital Designer — restored source
// ViewMode.cpp — LEGO::ViewMode, ChangeBackgroundCommand, ViewMode::PlayBallCommand (see
// ViewMode.h). TU 115: Mac 0x1cd6b0-0x1d208b, Win 0x622ea4-0x624749, 0x659aa0-0x659c73.
#include "ViewMode.h"

#include <iostream>

#include "LEGO/String.h"
#include "SystemTime.h"
#include "Platform.h"
#include "Directory.h"
#include "RenderControlManager.h"
#include "Scene.h"
#include "Brick.h"
#include "Part.h"
#include "Primitive.h"
#include "BrickConnect.h"
#include "BrickDatabase.h"
#include "LuaPlus.h"
#include "LuaBindings.h"
#include "LuaScriptManager.h"
#include "LuaCommandAdapter.h"
#include "LuaBrickMode.h"
#include "LDDApplication.h"
#include "LDDSkybox.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneManager.h"
#include "BrickModeGlobalObject.h"
#include "EditMode.h"
#include "BGAutomaticMode.h"
#include "GenerateBuildingGuideCommand.h"
#include "Commands/ModeNavigationCommand.h"
#include "Commands/BGModeCommand.h"
#include "Commands/UploadModelCommand.h"
#include "Commands/DeconstructionCommand.h"

namespace LEGO {

namespace {

// The ball (LDD "primitive 3") and the two ball shooters of the ball game.
const UInt32 kBallDesignID = 3;
const UInt32 kShooterDesignID = 0xa23d;          // 41533
const UInt32 kFastShooterDesignID = 0xa871;      // 43121 (the ball gets a velocity)

// Control::Invalidate unless the manager holds the redraws (inline everywhere).
void RedrawControl(Control* control)
{
    if (!control->GetManager()->IsUpdateLocked())
        control->Invalidate(false);
}

} // namespace

// ===========================================================================================
// ChangeBackgroundCommand
// ===========================================================================================

// Mac 0x1d1910 / 0x1d1980, Win sdd 0x623986
ChangeBackgroundCommand::~ChangeBackgroundCommand()
{
}

// Mac 0x1d1a00, Win 0x623591
void ChangeBackgroundCommand::Do()
{
    if (m_enabled)
        m_sceneControl->NextSkybox();
}

// Mac 0x1d1a20 (Win: the folded SetEnable(true) body 0x630465)
void ChangeBackgroundCommand::UpdateEnable()
{
    SetEnable(true);
}

// ===========================================================================================
// ViewMode::PlayBallCommand
// ===========================================================================================

// Mac 0x1d14f0 / 0x1d15e0, Win sdd 0x624723
ViewMode::PlayBallCommand::~PlayBallCommand()
{
}

// Mac 0x1d14d0, Win 0x624742
void ViewMode::PlayBallCommand::Do()
{
    m_mode->PlayBall();
}

// Mac 0x1d14b0
void ViewMode::PlayBallCommand::UpdateEnable()
{
    SetEnable(true);
}

// ===========================================================================================
// ViewMode
// ===========================================================================================

// Mac 0x1cd6b0, Win 0x62309c
ViewMode::ViewMode(ControlManager* manager, Window* window, BrickConnectSceneControl* sceneControl)
    : BrickMode(manager, window, sceneControl),
      m_snapshot(0),
      m_restoreSnapshot(false),
      m_playingBall(false),
      m_lastUpdate(0),
      m_field2c4(0.01f)
{
    // NOTE: m_lastUpdate is not initialised by either build (set before every use); zero here.
}

// Mac 0x1cf470 / 0x1cf8c0 (members and bases 0x1cf160), Win 0x623280 / sdd 0x6234ad
ViewMode::~ViewMode()
{
}

Render* ViewMode::GetRender() const
{
    return static_cast<RenderControlManager*>(GetManager())->GetRender();
}

// Mac 0x1cfdb0, Win 0x624136
void ViewMode::OnFinalRelease()
{
    if (IsCurrent())
        Exit();
    if (m_snapshot) {
        m_snapshot->Release();
        m_snapshot = 0;
    }
    ControlManagerObserver::OnFinalRelease();
    SimpleSoundChangedObserver::OnFinalRelease();
    UpdateObserver::OnFinalRelease();
    BrickConnectSceneControlObserver::OnFinalRelease();
    BrickMode::OnFinalRelease();
}

// Mac 0x1cd8b0, Win 0x6239a5
bool ViewMode::Initialize(EditMode* editMode, BGAutomaticMode* bgMode, Control* parent, Control* after)
{
    if (!BrickMode::Create(parent, after))
        return false;
    Render* render = GetRender();

    m_editNavigation = new ModeNavigationCommand(editMode);
    RefPointer<Command> bgModeCommand = new BGModeCommand(bgMode, GetSceneControl());
    RefPointer<Command> viewNavigation = new ModeNavigationCommand(this);
    m_deconstruction = new DeconstructionCommand(GetSceneControl(), render);
    RefPointer<Command> changeBackground = new ChangeBackgroundCommand(GetSceneControl());
    RefPointer<Command> uploadModel = new UploadModelCommand(GetSceneControl(), m_editNavigation);
    m_buildingGuide = new GenerateBuildingGuideCommand(GetSceneControl());

    bool failed = true;
    {
        RefPointer<LuaScriptManager> scripts = LuaScriptManager::GetInstance();
        LuaPlus::LuaState* state = scripts->GetLuaState();
        LuaPlus::LuaObject globals = state->GetGlobals();

        // ---- LDD.commands.viewMode ----
        LuaPlus::LuaObject commands = globals.Lookup("LDD.commands.viewMode");
        LuaPlus::LuaState* commandsState = commands.GetState();
        commands.Set("changeBackground", LuaCommandAdapter::CreateLuaObject(changeBackground, commandsState));
        commands.Set("deconstruction", LuaCommandAdapter::CreateLuaObject(m_deconstruction, commandsState));
        if (GetSceneManager()->GetDatabase()->GetPrimitive(kBallDesignID) != 0)
            commands.Set("playBall", LuaCommandAdapter::CreateLuaObject(new PlayBallCommand(this),
                                                                              commandsState));

        // ---- the Lua view mode builds the layout ----
        LuaPlus::LuaObject viewMode = globals.GetByName("viewMode");
        if (viewMode.IsTable()) {
            SetLuaBaseClass(viewMode, "BrickMode");
            RefPointer<LuaBrickMode> luaMode = new LuaBrickMode;
            if (luaMode->Bind(this, viewMode) && CallLuaMethod(viewMode, "Create", false, 1)) {
                LuaPlus::LuaObject created(state, -1);
                state->Pop(1);
                failed = !LuaToBoolean(created);
            }
        }
    }
    return !failed;
}

// Mac 0x1ce0c0, Win 0x623e74
void ViewMode::Enter()
{
    if (!m_skybox) {
        Render* render = GetRender();
        RefPointer<Directory> skyboxes;
        {
            RefPointer<Directory> system = Platform::GetInstance()->GetSystemDirectory(9, true);
            skyboxes = system->GetDirectory(String("Graphics/Skyboxes"), false);
        }
        m_skybox = new LDDSkybox(render, skyboxes);
        m_skybox->Initialize();
    }
    SimpleSoundChangedObserver::Attach(m_skybox);
    BrickConnectSceneControl* control = GetSceneControl();
    control->SetGridVisible(false);
    control->SetSkybox(m_skybox);
    control->SetAllWiresSelected(false);
    m_sound = m_skybox->GetSound();
    if (m_sound) {
        m_sound->SetVolume(8);
        m_sound->SetLooping(true);
        m_sound->Play();
    }
    RedrawControl(control);
    control->CreatePhysicsSnapshot(m_snapshot);
    m_restoreSnapshot = false;
    GetSceneManager()->SetDefaultTool(BrickModeGlobalObject::GetNoneTool());
    GetSceneManager()->SetActiveTool(0);
    LDDApplication::GetInstance()->SetEditCommand(m_editNavigation);
    LDDApplication::GetInstance()->SetBuildingGuideCommand(m_buildingGuide);
    ControlObserver::Attach(control);
    ControlManagerObserver::Attach(GetManager());
    BrickMode::Enter();
}

// Mac 0x1cebb0, Win 0x624076
void ViewMode::Exit()
{
    if (m_deconstruction)
        m_deconstruction->Stop();
    GetSceneControl()->SetSkybox(0);
    if (m_sound)
        m_sound->Stop();
    if (m_playingBall)
        StopPlayBall();
    if (m_snapshot) {
        if (m_restoreSnapshot)
            GetSceneControl()->RestorePhysicsSnapshot(m_snapshot, false);
        m_snapshot->Release();
        m_snapshot = 0;
    }
    ControlObserver::Detach(GetSceneControl());
    ControlManagerObserver::Detach();
    SimpleSoundChangedObserver::Detach(m_skybox.Get());
    BrickMode::Exit();
}

// Mac 0x1cece0, Win 0x623df6
void ViewMode::StopPlayBall()
{
    UpdateObserver::Detach();
    BrickConnectSceneControlObserver::Detach();
    m_playingBall = false;
    BrickConnectSceneControl* control = GetSceneControl();
    control->GetScene()->GetPhysicsScene()->StopSolver(false);
    control->AddPressedButton(false);
    // (Win: out of line 0x62338b)
    for (BasicArray<PhysicsSceneryElement*>::Iterator it = m_sceneryElements.Begin();
         it != m_sceneryElements.End(); ++it) {
        PhysicsSceneryElement* element = *it;
        control->RemoveSceneryElement(element);
        element->Release();
    }
    m_sceneryElements.Clear();
}

// Mac 0x1cefe0 (thunk 0x1cf0a0), Win 0x622ea4
void ViewMode::Update()
{
    Int64 now = GetMilliseconds();
    UInt64 elapsed = (UInt64)now - (UInt64)m_lastUpdate;
    if (elapsed < 5)
        return;
    m_lastUpdate = now;
    BrickConnectSceneControl* control = GetSceneControl();
    control->GetScene()->GetPhysicsScene()->Step((Real32)elapsed / 1000.0f);
    control->NotifyPhysicsStepped();
}

// Mac 0x1d00a0 (thunk 0x1d0150), Win 0x622fb6
void ViewMode::OnSoundChanged(const RefPointer<SimpleSound>& sound, bool play)
{
    if (m_sound)
        m_sound->Stop();
    m_sound = sound;
    if (m_sound) {
        m_sound->SetLooping(play);
        m_sound->SetVolume(8);
        m_sound->Play();
    }
}

// Mac 0x1d0180 (thunk 0x1d0260), Win 0x624190
void ViewMode::OnManagerModalControl(bool modal, Control* /*control*/)
{
    if (!modal) {
        if (UpdateObserver::GetSubject() == 0 && m_playingBall) {
            UpdateObserver::Attach(GetManager());
            BrickConnectSceneControlObserver::Attach(GetSceneControl());
        }
        m_lastUpdate = GetMilliseconds();
    } else if (UpdateObserver::GetSubject() != 0) {
        UpdateObserver::Detach();
        BrickConnectSceneControlObserver::Detach();
    }
}

// Mac 0x1d0290, Win 0x6241f9
void ViewMode::PlayBall()
{
    const Primitive* ball = GetSceneManager()->GetDatabase()->GetPrimitive(kBallDesignID);
    if (!ball)
        return;
    BrickConnectSceneControl* control = GetSceneControl();
    BrickConnectSceneManager* sceneManager = GetSceneManager();
    if (!m_playingBall) {
        BrickPhysics* physics = sceneManager->GetBrickConnect()->GetPhysics();
        m_restoreSnapshot = true;
        m_playingBall = true;
        UpdateObserver::Attach(GetManager());
        BrickConnectSceneControlObserver::Attach(control);
        control->AddPressedButton(true);

        // The arena: floor 10 below the model, four walls leaning out by 45 degrees, a lid;
        // boxes of 5 * size x 10 x 5 * size half size.
        BoundingBox box(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        control->GetScene()->GetBoundingBox(box, true);
        Real32 width = box.GetWidth() * 1.5f;
        Real32 depth = 1.5f * box.GetDepth();
        Real32 size = (width > depth ? width : depth) + 50.0f;
        PhysicsGeometryBox* wall = physics->CreateBox(Vector3(size * 5.0f, 10.0f, size * 5.0f));
        PhysicsPlacedConfiguredGeometry walls[6];
        const Real32 kQuarterPi = 0.785398185f;   // (Win: PI / 180 * 45)
        walls[0].m_geometry = wall;
        walls[0].m_transform.GetTranslation().y = box.m_min.y + -10.0f;
        walls[0].m_type = 1;
        walls[1].m_geometry = wall;
        walls[1].m_transform.RotateZ(kQuarterPi);
        walls[1].m_transform.GetTranslation().x = 20.0f + size;
        walls[1].m_transform.GetTranslation().y = box.m_min.y + -10.0f;
        walls[1].m_type = 1;
        walls[2].m_geometry = wall;
        walls[2].m_transform.RotateX(kQuarterPi);
        walls[2].m_transform.GetTranslation().z = -20.0f - size;
        walls[2].m_transform.GetTranslation().y = box.m_min.y + -10.0f;
        walls[2].m_type = 1;
        walls[3].m_geometry = wall;
        walls[3].m_transform.RotateZ(-kQuarterPi);
        walls[3].m_transform.GetTranslation().x = -20.0f - size;
        walls[3].m_transform.GetTranslation().y = box.m_min.y + -10.0f;
        walls[3].m_type = 1;
        walls[4].m_geometry = wall;
        walls[4].m_transform.RotateX(-kQuarterPi);
        walls[4].m_transform.GetTranslation().z = 20.0f + size;
        walls[4].m_transform.GetTranslation().y = -10.0f + box.m_min.y;
        walls[4].m_type = 1;
        walls[5].m_geometry = wall;
        walls[5].m_transform.GetTranslation().y = size * 2.5f;
        walls[5].m_type = 1;
        PhysicsSceneryElement* arena = physics->CreateSceneryElement(walls, 6, PhysicsMassProperties());
        if (arena) {
            m_sceneryElements.PushBack(arena);
            control->AddSceneryElement(arena, Matrix3D(), 1, BasicArray<BrickConnectSceneControl::PlacedMesh>());
        }
        wall->Release();
        AnimatePhysicsSolver* solver =
            control->GetScene()->GetPhysicsScene()->CreateAnimatingSolver(0, 0, 0);
        // NOTE: both builds release the result unchecked (Vortex always makes one); the
        // headless stand-in returns none.
        if (solver)
            solver->Release();
        m_lastUpdate = GetMilliseconds();
    }

    // A ball for every shooter.
    const BasicArray<RefPointer<Brick> >& bricks = control->GetScene()->GetBricks();
    for (UInt32 i = 0; i < bricks.GetSize(); ++i) {
        Brick* brick = bricks[i];
        if (brick->GetDesignID() == kShooterDesignID) {
            Part* part = brick->GetParts()[0];
            Matrix3D transform;
            part->GetElement(0)->GetTransform(transform);
            transform.Translate(Rotate(Vector3(0.4f, 0.94f, 1.4f), transform));
            AddBall(ball, transform, part->GetMaterial(0));
        }
        if (brick->GetDesignID() == kFastShooterDesignID) {
            Part* part = brick->GetParts()[0];
            Matrix3D transform;
            part->GetElement(0)->GetTransform(transform);
            transform.Translate(Rotate(Vector3(2.0f, 1.54f, -0.4f), transform));
            PhysicsSceneryElement* element = AddBall(ball, transform, part->GetMaterial(0));
            if (element)
                element->SetLinearVelocity(Rotate(Vector3(50.0f, 0.0f, 0.0f), transform));
        }
    }
}

// Mac 0x1d0d10, Win 0x6235fe
PhysicsSceneryElement* ViewMode::AddBall(const Primitive* primitive, const Matrix3D& transform,
                                         UInt32 material)
{
    BrickConnectSceneControl* control = GetSceneControl();
    BrickConnectSceneManager* sceneManager = GetSceneManager();
    BrickPhysics* physics = sceneManager->GetBrickConnect()->GetPhysics();
    RefPointer<PrimitiveElement> element =
        control->GetScene()->GetBrickConnect()->CreatePrimitiveElement(primitive, 0, transform);
    if (!element)
        return 0;

    // The ball's collision geometry, as a scenery element.
    PhysicsPrimitiveElement* physicsElement = element->GetPhysicsElement();
    BasicArray<PhysicsPlacedConfiguredGeometry> geometries;
    UInt32 count = physicsElement->GetGeometryCount();
    for (UInt32 i = 0; i < count; ++i) {
        PhysicsPlacedConfiguredGeometry geometry;
        if (physicsElement->GetGeometry(i, geometry))
            geometries.PushBack(geometry);
    }
    // The bone's <PhysicsAttributes> (inertia +0xa8, centre of mass +0xe8, mass +0xf4, friction
    // +0xf8) have the layout of PhysicsMassProperties (both builds pass Bone + 0xa8).
    const PhysicsMassProperties& properties =
        reinterpret_cast<const PhysicsMassProperties&>(element->GetBone().m_inertiaTensor);
    PhysicsSceneryElement* scenery =
        physics->CreateSceneryElement(geometries.IsEmpty() ? 0 : &geometries[0], geometries.GetSize(),
                                      properties);
    for (UInt32 i = 0; i < geometries.GetSize(); ++i)
        geometries[i].m_geometry->Release();
    if (!scenery)
        return 0;
    m_sceneryElements.PushBack(scenery);
    scenery->SetDynamic(true);

    // One mesh per material slot of the primitive (at least one).
    BasicArray<BrickConnectSceneControl::PlacedMesh> meshes;
    UInt32 face = 0;
    do {
        RefPointer<Mesh> mesh = sceneManager->CreateMesh(primitive, face, material, 0);
        if (mesh)
            meshes.PushBack(BrickConnectSceneControl::PlacedMesh(mesh, Vector3(), Matrix3D(), 0, 1.0f));
        ++face;
    } while (face < (primitive->GetDecorationFaceCount() == 0 ? 1 : primitive->GetDecorationFaceCount()));
    control->AddSceneryElement(scenery, transform, 1, meshes);
    return scenery;
}

} // namespace LEGO
