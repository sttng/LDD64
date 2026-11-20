// LEGO Digital Designer — restored source
// LEGO::ClickStickTool / LEGO::UndoSetContent — see ClickStickTool.h.
// TU 062 "ClickStickTool.cpp": Mac 0x142b80-0x147b2b, Win 0x667dc3-0x66a238 (+ 0x62cff6-0x62d337).
// Not code: the thunks, the BasicMap / BasicArray instances (0x146120, 0x147020-0x147ab0).
#include "ClickStickTool.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneManager.h"
#include "BrickConnect.h"
#include "BrickDatabase.h"
#include "ControlManager.h"
#include "Window.h"
#include "Key.h"
#include "Camera.h"
#include "Directory.h"
#include "Application.h"
#include "LDDApplication.h"
#include "LDDUtilities.h"
#include "LDDPart.h"
#include "PrimitiveElementBrick.h"
#include "PrimitiveElement.h"
#include "ModelLoaderProgressBarThread.h"
#include "Materials.h"
#include "RenderQueue.h"
#include "Mesh.h"
#include "SimpleSound.h"
#include "Trace.h"
#include "Part.h"
#include "Brick.h"

namespace LEGO {

namespace {

// The tool cursor while bricks are carried. (BrickConnectSceneControl cursor ids)
const UInt32 kCarryCursor = 0x65;

bool IsDeveloperMode()
{
    bool developer = false;
    Preferences::GetInstance()->GetValue(String("DeveloperMode"), developer);
    return developer;
}

} // namespace

// =============================================================================================
// UndoSetContent
// =============================================================================================

// Mac 0x146d40, Win 0x667eb8
void UndoSetContent::Execute()
{
    m_sceneManager->SetActiveTool(0);
}

// =============================================================================================
// ClickStickTool
// =============================================================================================

// Mac 0x18b720 (inline, emitted in EditMode.cpp), Win 0x62d132
ClickStickTool::ClickStickTool(BrickConnectSceneManager* sceneManager)
    : m_brickConnect(sceneManager->GetBrickConnect()), m_mouseMoved(false), m_lastDesignID(0),
      m_repeat(false), m_returnTool(0), m_loading(false), m_storage(0), m_sourceControl(0),
      m_draw(sceneManager), m_pressed(false)
{
}

// Mac 0x1466d0 (D1) / 0x1469d0 (D0), body 0x146150; Win sdd 0x62d337, body 0x66938a
ClickStickTool::~ClickStickTool()
{
    // the load thread (ModelLoaderHandler's) goes with the tool
    if (m_thread != 0) {
        delete m_thread;
        m_thread = 0;
    }
}

// Mac 0x146d60 (thunks 0x146e10, 0x146e30, 0x146e50, 0x146e70)
void ClickStickTool::OnFinalRelease()
{
    ModelLoaderHandler::OnFinalRelease();
    PreferencesObserver::OnFinalRelease();
    UpdateObserver::OnFinalRelease();
    BrickConnectSceneControlTool::OnFinalRelease();
}

// Mac 0x142b80, Win 0x668b69
bool ClickStickTool::Create(Render* render, BrickConnectSceneManager* sceneManager)
{
    m_render = render;
    SetActive(true);
    m_dragScene = m_brickConnect->NewScene(0);
    if (m_dragScene == 0)
        return false;
    m_boxMesh = CreateBoxMesh(m_render, Color::Green(), sceneManager->GetSilhouetteMaterial(), false);
    m_dropSound = LDDUtilities::GetInstance()->CreateSound(String("IA_drop.mp3"));
    PreferencesObserver::Attach(Preferences::GetInstance().Get());
    return true;
}

// Mac 0x146e00, Win 0x62d306
BrickConnectSceneControlTool* ClickStickTool::GetTool()
{
    return m_returnTool;
}

// Mac 0x1433e0, Win 0x668e9c
void ClickStickTool::RebuildDraw(PrimitiveElement* first, bool single)
{
    m_draw.Clear();
    if (GetSceneControl() == 0)
        return;
    BasicArray<RefPointer<Part> > parts;
    m_dragScene->GetParts(parts);
    for (BasicArray<RefPointer<Part> >::ConstIterator it = parts.Begin(); it != parts.End(); ++it)
        m_draw.AddPart(*it, 0);
    Begin(GetSceneControl()->GetScene(), m_dragScene, first, single);
    GetSceneControl()->Redraw();
}

// Mac 0x1437b0, Win 0x669851
void ClickStickTool::LoadDragScene(const BasicArray<ObjectRefPointer<PrimitiveElementBrick> >& bricks,
                                   bool keepPivot)
{
    BrickDatabase* database = GetSceneControl()->GetSceneManager()->GetDatabase();
    m_sourceControl = 0;
    m_dragScene->Clear();
    BasicMap<BehaviorElement*, const BasicArray<StorageScene2::Behavior>*> behaviors;
    BasicArray<RefPointer<Brick> > newBricks;
    BasicArray<RefPointer<Brick> > failed;
    for (BasicArray<ObjectRefPointer<PrimitiveElementBrick> >::ConstIterator it = bricks.Begin();
         it != bricks.End(); ++it) {
        const PrimitiveElementBrick* elementBrick = it->Get();
        const BrickPalette::Brick* paletteBrick = elementBrick->GetBrick();
        LDDBrick* brick = static_cast<LDDBrick*>(m_brickConnect->CreateBrick(paletteBrick->m_designID));
        brick->SetPaletteBrick(paletteBrick);
        newBricks.PushBack(RefPointer<Brick>(brick));
        behaviors[static_cast<BehaviorElement*>(brick)] = &elementBrick->GetBrickBehaviors();
        for (UInt32 p = 0; p < paletteBrick->m_parts.GetSize(); ++p) {
            const BrickPalette::Part* palettePart = elementBrick->GetPalettePart(p);
            const Primitive* primitive = database->GetPrimitive(palettePart->m_designID);
            Part* part = m_brickConnect->CreatePart(primitive);
            part->SetBrick(brick);
            behaviors[static_cast<BehaviorElement*>(part)] = &elementBrick->GetPartBehaviors(p);
            for (UInt32 bone = 0; bone < palettePart->m_boneTransforms.GetSize(); ++bone) {
                PrimitiveElement* element = m_brickConnect->CreatePrimitiveElement(
                    primitive, bone, elementBrick->GetBoneTransforms(p)[bone]);
                element->SetPart(part);
                behaviors[static_cast<BehaviorElement*>(element)] = &elementBrick->GetBoneBehaviors(p, bone);
            }
            part->SetMaterials(elementBrick->GetMaterials(p));
            part->SetDecorations(elementBrick->GetDecorations(p));
            part->UpdateState();
        }
    }
    m_dragScene->BatchInsert(newBricks, failed, 0, false, 0);
    for (BasicMap<BehaviorElement*, const BasicArray<StorageScene2::Behavior>*>::Iterator it =
             behaviors.Begin(); it != behaviors.End(); ++it)
        it->first->AddBehaviors(*it->second);
    RebuildDraw(0, false);
    // the grip at the bottom centre, except for the single brick of the design picked last
    if (!keepPivot || bricks.GetSize() != 1) {
        m_pivot.SetIdentity();
        BoundingBox box(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        m_dragScene->GetBoundingBox(box, true);
        Vector3 center = box.GetCenter();
        Vector3 centerZ = box.GetCenter();
        m_grip.SetTranslation(Vector3(-center.x, -box.m_min.y, -centerZ.z));
        m_heightOffset = 0.0f;
        m_lastDesignID = 0;
    } else {
        const UInt32 designID = bricks[0]->GetBrick()->m_designID;
        if (designID != m_lastDesignID) {
            m_pivot.SetIdentity();
            BoundingBox box(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
            m_dragScene->GetBoundingBox(box, true);
            Vector3 center = box.GetCenter();
            Vector3 centerZ = box.GetCenter();
            m_grip.SetTranslation(Vector3(-center.x, -box.m_min.y, -centerZ.z));
            m_heightOffset = 0.0f;
            m_lastDesignID = designID;
        }
    }
}

// Mac 0x143630, Win 0x669e7b
void ClickStickTool::PickUp(const BasicArray<ObjectRefPointer<PrimitiveElementBrick> >& bricks,
                            bool repeat, BrickConnectSceneControlTool* returnTool, bool aligned,
                            bool keepPivot)
{
    Cancel(GetSceneControl());
    if (GetSceneControl()->SetCarriedBricks(bricks)) {
        m_aligned = aligned;
        m_repeat = repeat;
        m_returnTool = returnTool != this ? returnTool : 0;
        LoadDragScene(bricks, keepPivot);
        if (!m_dragScene->IsEmpty()) {
            m_undo = new UndoSetContent(GetSceneControl()->GetSceneManager());
            GetSceneControl()->GetUndoManager()->AddAction(m_undo);
            ApplyCursor(kCarryCursor);
            return;
        }
    }
    GetSceneControl()->GetSceneManager()->SetActiveTool(0);
}

// Mac 0x143050, Win 0x669cde
void ClickStickTool::PickUpParts(const BasicArray<RefPointer<Part> >& parts, PrimitiveElement* picked,
                                 const Vector3& point, bool single, bool copy,
                                 BrickConnectSceneControlTool* returnTool)
{
    Cancel(GetSceneControl());
    m_repeat = false;
    m_returnTool = returnTool != this ? returnTool : 0;
    m_pivot.SetIdentity();
    m_lastDesignID = 0;
    if (copy) {
        m_aligned = false;
        m_sourceControl = 0;
        m_dragScene->Clear();
        Scene::CopyMap map;
        GetSceneControl()->CopyParts(parts, m_dragScene, map, false, 0x1c);
        picked = map.m_elements[picked];
    } else {
        m_aligned = true;
        m_sourceControl = GetSceneControl();
        m_sourceControl->MoveParts(parts, m_dragScene);
    }
    m_grip.SetTranslation(-point);
    RebuildDraw(picked, single);
    UpdateHeightOffset();
    m_undo = new UndoSetContent(GetSceneControl()->GetSceneManager());
    GetSceneControl()->GetUndoManager()->AddAction(m_undo);
    ApplyCursor(kCarryCursor);
    if (m_dragScene->IsEmpty())
        GetSceneControl()->GetSceneManager()->SetActiveTool(0);
}

// Mac 0x1446f0, Win 0x668d2c
void ClickStickTool::Cancel(BrickConnectSceneControl* control)
{
    if (m_sourceControl == 0) {
        End(true);
        control->ResetCarriedBricks();
    } else {
        // the moved parts go back where they were (the identity placement)
        Scene::ConnectResult result;
        Matrix3D identity;
        const bool connects = m_scene->CanConnectDrag(identity, result);
        End(true);
        // NOTE: parts that no longer connect there stay in the drag scene and are cleared
        // below (both builds).
        if (connects)
            m_sourceControl->MergeScene(m_dragScene, result, true);
        m_grip.SetIdentity();
        control->ClearCarriedBricks();
        m_sourceControl = 0;
    }
    m_dragScene->Clear();
    m_draw.Clear();
    if (m_undo != 0) {
        control->GetUndoManager()->RemoveAction(m_undo);
        m_undo = 0;
    }
}

// Mac 0x144580, Win 0x66838e
void ClickStickTool::UpdateCast(const Vector2& position)
{
    Vector3 direction;
    Vector3 origin;
    Real32 length;
    GetSceneControl()->GetPickRay(position, origin, direction, length);
    m_planeHeight = -GetSceneControl()->GetGridHeight();
    Cast(origin, direction, length);
    GetConnectedParts(m_connectedParts);
    BoundingBox box(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    m_dragScene->GetBoundingBox(box, true);
    BoundingBox placed = box * m_transform;
    placed.m_min.y = 0.0f;
    placed.m_max.y = 0.0f;
    GetSceneControl()->UpdateGrid(&placed);
    GetSceneControl()->Redraw();
}

// Mac 0x144150 (thunk 0x1442b0), Win 0x669f71
void ClickStickTool::OnLoadStarted(StorageScene2* storage)
{
    Cancel(GetSceneControl());
    m_repeat = false;
    m_returnTool = 0;
    if (m_thread != 0) {
        delete m_thread;
        m_thread = 0;
    }
    m_loading = true;
    m_storage = storage;
    m_dragScene->BeginUpdate();
    m_loadedParts.Clear();
    m_pivot.SetIdentity();
    m_grip.SetIdentity();
    m_thread = new ModelLoaderProgressBarThread();
    m_thread->Start(GetSceneControl()->GetSceneManager(), m_dragScene, m_storage,
                    LDDApplication::GetInstance()->GetBrickPalette(), &m_loadedParts, 0);
}

// Mac 0x1442d0 (thunk 0x1446d0), Win 0x668fa3
void ClickStickTool::OnLoadFinished()
{
    m_loading = false;
    m_dragScene->EndUpdate();
    if (!IsCancelled() && !m_dragScene->IsEmpty()) {
        if (Succeeded()) {
            BrickConnectSceneControl* control = BrickConnectSceneControlObserver::GetSubject();
            control->GetSceneManager()->Load(m_thread->GetLazyLoadData());
            // the grip at the bottom centre of the model
            BoundingBox box(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
            m_dragScene->GetBoundingBox(box, true);
            Vector3 center = box.GetCenter();
            Vector3 centerZ = box.GetCenter();
            m_grip.SetTranslation(Vector3(-center.x, -box.m_min.y, -centerZ.z));
            m_heightOffset = 0.0f;
            RebuildDraw(0, false);
            Vector2 mouse = GetSceneControl()->GetManager()->GetMousePosition(GetSceneControl());
            UpdateCast(mouse);
            m_undo = new UndoSetContent(GetSceneControl()->GetSceneManager());
            GetSceneControl()->GetUndoManager()->AddAction(m_undo);
            ApplyCursor(kCarryCursor);
            m_aligned = true;
            return;
        }
        Application::GetInstance()->CloseDocument();
        return;
    }
    GetSceneControl()->GetSceneManager()->SetActiveTool(0);
}

// Mac 0x1449b0, Win 0x669153
void ClickStickTool::OnAttached(BrickConnectSceneControl* control)
{
    BrickConnectSceneControlTool::OnAttached(control);
    UpdateObserver::Attach(MainUpdater::GetInstance().Get());
    m_mouseMoved = false;
    BrickConnectSceneControlObserver::GetSubject()->ClearSelection();
    RebuildDraw(0, false);
    if (!IsDeveloperMode())
        return;
    // developer mode: Right held records the casts, Left held replays them
    Window* window = BrickConnectSceneControlObserver::GetSubject()->GetManager()->GetWindow();
    if (window->IsKeyPressed(kKeyRightArrow)) {
        RefPointer<File> file = Application::GetInstance()->GetUserDataDirectory()->OpenFile(
            String("cstRecording.dat"), OpenMode(false, true), true, 0);
        if (file == 0 || !StartRecording(file))
            Trace(String("ClickStickTool record failed"), true);
        else
            Trace(String("ClickStickTool record started"), true);
    } else if (window->IsKeyPressed(kKeyLeftArrow)) {
        RefPointer<File> file = Application::GetInstance()->GetUserDataDirectory()->OpenFile(
            String("cstRecording.dat"), OpenMode(true, false, true, false), false, 0);
        if (file == 0 || !StartPlayback(file))
            Trace(String("ClickStickTool playback failed"), true);
        else
            Trace(String("ClickStickTool playback started"), true);
    }
}

// Mac 0x145060, Win 0x668e16
void ClickStickTool::OnDetached(BrickConnectSceneControl* control)
{
    m_simple = false;
    Cancel(control);
    control->UpdateGrid(0);
    control->Redraw();
    UpdateObserver::Detach();
    m_dragScene->Clear();
    m_draw.Clear();
    m_storage = 0;
    m_returnTool = 0;
    if (m_recording || m_playing)
        Application::GetInstance()->StopRunLoop(true);
    StopRecording(true);
    BrickConnectSceneControlTool::OnDetached(control);
}

// Mac 0x145200 (thunk 0x145240), Win 0x667dc3
void ClickStickTool::OnMouseMove(Control* /*control*/, const Vector2& position)
{
    if (!m_loading) {
        m_mousePosition = position;
        m_mouseMoved = true;
    }
}

// Mac 0x145280 (thunk 0x1452c0), Win 0x667ee2
void ClickStickTool::OnMousePressed(Control* /*control*/, const Vector2& position, UInt32 /*button*/)
{
    if (!m_loading && m_file == 0) {
        m_pressed = true;
        m_pressPosition = position;
    }
}

// Mac 0x145300 (thunk 0x1456d0), Win 0x66a057
void ClickStickTool::OnMouseReleased(Control* /*control*/, const Vector2& position, UInt32 button)
{
    if (m_loading || m_file != 0)
        return;
    if (m_pressed) {
        const Real32 moved = DistanceSquared(m_pressPosition, position);
        if (button == 0 && moved < 2500.0f && m_connected) {
            End(false);
            if (m_undo != 0) {
                GetSceneControl()->GetUndoManager()->RemoveAction(m_undo);
                m_undo = 0;
            }
            if (m_dropSound != 0)
                m_dropSound->Play();
            GetSceneControl()->MergeScene(m_dragScene, m_result, m_sourceControl != 0);
            if (m_aligned)
                m_grip.SetIdentity();
            ClearResult();
            BasicArray<ObjectRefPointer<PrimitiveElementBrick> > carried(*GetSceneControl()->GetCarriedBricks());
            GetSceneControl()->ClearCarriedBricks();
            m_sourceControl = 0;
            if (m_repeat && !carried.IsEmpty()) {
                if (Preferences::GetInstance()->GetBool(String("REPEATPASTE"), false))
                    PickUp(carried, true, m_returnTool, m_aligned, true);
                else
                    GetSceneControl()->GetSceneManager()->SetActiveTool(m_returnTool);
            } else {
                GetSceneControl()->GetSceneManager()->SetActiveTool(m_returnTool);
            }
        }
    }
    m_pressed = false;
}

// Mac 0x145700 (thunk 0x145a60), Win 0x667f0d
void ClickStickTool::OnKeyPressed(Control* /*control*/, UInt32 key, UInt32 modifiers, bool /*repeat*/)
{
    if (modifiers != 0)
        return;
    if (key == kKeyI) {
        if (!IsDeveloperMode())
            return;
        m_simple = true;
        Reset();
        GetSceneControl()->Redraw();
    } else {
        if (m_simple)
            return;
        Vector3 view = GetSceneControl()->GetCameraPosition() - GetSceneControl()->GetCameraTarget();
        view.Normalize();
        Vector3 up = GetSceneControl()->GetCamera()->GetUpDirection();
        up.Normalize();
        UInt32 direction;
        switch (key) {
        case kKeyW:
        case kKeyUpArrow:
            direction = 2;
            break;
        case kKeyA:
        case kKeyLeftArrow:
            direction = 4;
            break;
        case kKeyS:
        case kKeyDownArrow:
            direction = 1;
            break;
        case kKeyD:
        case kKeyRightArrow:
            direction = 3;
            break;
        default:
            return;
        }
        Rotate(direction, up, view);
        GetSceneControl()->Redraw();
    }
    m_mousePosition = GetSceneControl()->GetManager()->GetMousePosition(GetSceneControl());
    m_mouseMoved = true;
}

// Mac 0x145a90 (thunk 0x145c00), Win 0x6680b1
void ClickStickTool::OnKeyReleased(Control* /*control*/, UInt32 key, UInt32 modifiers)
{
    if (key != kKeyI || modifiers != 0)
        return;
    if (!IsDeveloperMode())
        return;
    m_simple = false;
    GetSceneControl()->Redraw();
    m_mousePosition = GetSceneControl()->GetManager()->GetMousePosition(GetSceneControl());
    m_mouseMoved = true;
}

// Mac 0x145c30, Win 0x66817c
void ClickStickTool::OnEvent05(const Matrix3D& transform, Render* /*render*/, RenderQueue* overlayQueue,
                               RenderQueue* sceneQueue)
{
    const Matrix3D& camera = GetSceneControl()->GetCamera()->GetViewMatrix();
    Matrix3D world;
    world.Multiply(transform, m_transform);
    if (!m_connected) {
        m_draw.DrawWithDepthPrepass(sceneQueue, camera, world);
    } else {
        m_draw.Draw(sceneQueue, camera, world);
        for (BasicArray<RefPointer<Part> >::ConstIterator it = m_connectedParts.Begin();
             it != m_connectedParts.End(); ++it) {
            Matrix3D box;
            GetPartBoxTransform(*it, box, false);
            overlayQueue->Add(m_boxMesh, box, 0.0f, 0, 1.0f);
        }
    }
}

// Mac 0x145d30, Win 0x667de9 (ICF)
void ClickStickTool::OnEvent18()
{
    m_mousePosition = GetSceneControl()->GetManager()->GetMousePosition(GetSceneControl());
    m_mouseMoved = true;
}

// Mac 0x145d90, Win 0x667de9 (ICF)
void ClickStickTool::OnEvent19()
{
    m_mousePosition = GetSceneControl()->GetManager()->GetMousePosition(GetSceneControl());
    m_mouseMoved = true;
}

// Mac 0x145df0 (thunk 0x145e40), Win 0x668442
void ClickStickTool::Update()
{
    if (!m_loading && (m_mouseMoved || m_playing)) {
        m_mouseMoved = false;
        UpdateCast(m_mousePosition);
    }
}

// Mac 0x145e90 (thunk 0x145fc0), Win 0x668233
void ClickStickTool::OnPreferenceChanged(const String& key)
{
    if (key.EqualsNoCase(String("PLAYSOUND")))
        m_dropSound = LDDUtilities::GetInstance()->CreateSound(String("IA_drop.mp3"));
}

// Mac 0x145fe0 (thunk 0x146000), Win 0x667e23
void ClickStickTool::OnPlaybackFinished(bool /*success*/)
{
    Application::GetInstance()->StopRunLoop(true);
}

// Mac 0x146020 (thunk 0x1460e0), Win 0x667e34
void ClickStickTool::OnPlaybackMismatch(UInt32 frame, UInt32 recordedFlags, UInt32 flags,
                                        const Matrix3D& recordedTransform, const Matrix3D& transform)
{
    String text("ClickStickTool playback inconsistency on frame ");
    text << frame;
    if (flags != recordedFlags)
        text << ", path was different";
    if (!recordedTransform.IsEqual(transform, 1e-5f))
        text << ", transformation was different";
    Trace(text, true);
}

} // namespace LEGO
