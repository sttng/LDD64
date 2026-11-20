// LEGO Digital Designer — restored source
// LEGO::HingeTool, HingeButton, SetAngleButton, UndoHingeTool — see HingeTool.h.
// TU 056 "HingeTool.cpp": Mac 0x11d900-0x1298bb, Win 0x67f8ed-0x68435f (+ 0x4562c0-0x45773f:
// TextInputControlObserver template instances). Not restored as code: the template instances
// of the TU (BasicArray<CylinderPair> 0x11d900 / 0x128fb0 and PushBack 0x120010,
// BasicArray<DialControl*> / <TextInputControl*> destructors, ObserverN<DialControl> /
// <TextInputControl> Attach 0x1240c0 / 0x1203d0 and Detach 0x123c40, NotifierT RemoveObserver
// 0x129060 / 0x129250 / 0x129450 / 0x129640, the flattening of the manipulated bodies 0x126330,
// the destructor bodies 0x126b70 and the construction-vtable thunks).
#include "HingeTool.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneManager.h"
#include "ConnectionManipulator.h"
#include "RigidElement.h"
#include "RigidConnection.h"
#include "PrimitiveConnection.h"
#include "PrimitiveElement.h"
#include "PrimitiveRigidElement.h"
#include "BrickPhysics.h"
#include "Scene.h"
#include "Part.h"
#include "ControlManager.h"
#include "RenderControl.h"
#include "RenderControlManager.h"
#include "RenderQueue.h"
#include "Render.h"
#include "Materials.h"
#include "Texture.h"
#include "BitmapSelectionMask.h"
#include "LDDApplication.h"
#include "LDDUtilities.h"
#include "SystemTime.h"
#include "MathUtil.h"
#include "LEGO/String.h"

namespace LEGO {

namespace {

// `value` below `limit` + `tolerance` (the band hit test). Win 0x67f8ed (cdecl; Mac inlined)
bool IsWithin(Real32 value, Real32 limit, Real32 tolerance)
{
    return value < (EvalReal)limit + tolerance;
}

// Degrees -> radians as both builds compute it (Mac: the folded float PI / 180; Win: the
// float PI global / 180.0 in double precision).
Real32 DegToRad(Real32 degrees)
{
#if defined(_WIN32)
    return (Real32)((double)PI / 180.0 * (double)degrees);
#else
    return degrees * (PI / 180.0f);
#endif
}

// Radians -> degrees (Mac 57.29578f; Win 180.0 / PI in double precision).
Real32 RadToDeg(Real32 radians)
{
#if defined(_WIN32)
    return (Real32)(180.0 / (double)PI * (double)radians);
#else
    return radians * (180.0f / PI);
#endif
}

// The steps of the keys (±18°) and of the minus / plus buttons (±1°).
Real32 KeyStep()
{
#if defined(_WIN32)
    return (Real32)((double)PI / 10.0);       // Win 0x683f37: -PI / 10.0
#else
    return PI / 10.0f;                        // Mac 0x3ea0d97c
#endif
}
Real32 ButtonStep()
{
#if defined(_WIN32)
    return (Real32)((double)PI / 180.0);
#else
    return PI / 180.0f;                       // Mac 0x3c8efa35
#endif
}

// The joint of the manipulator. (inlined everywhere)
PhysicsConnection* GetJoint(ConnectionManipulator* manipulator)
{
    return manipulator->GetConnection()->m_joint;
}

// The bodies a manipulation moves: every active element and its sub-elements (Mac 0x126330).
void CollectBodies(ConnectionManipulator* manipulator, BasicArray<RigidElement*>& bodies)
{
    const BasicArray<Scene::ActiveRigidElement>& elements = manipulator->GetActiveElements();
    for (UInt32 i = 0; i < elements.GetSize(); ++i) {
        bodies.PushBack(elements[i].m_element);
        for (UInt32 j = 0; j < elements[i].m_elements.GetSize(); ++j)
            bodies.PushBack(elements[i].m_elements[j]);
    }
}

bool Contains(const BasicArray<RigidElement*>& bodies, RigidElement* body)
{
    for (UInt32 i = 0; i < bodies.GetSize(); ++i)
        if (bodies[i] == body)
            return true;
    return false;
}

RigidElement* BodyOf(PrimitiveElement* element)
{
    return element->GetRigidElement()->GetRigidElement();
}

} // namespace

// ===========================================================================================
// SetAngleButton
// ===========================================================================================

// (inlined in Mac 0x120b70 / Win 0x68135f)
SetAngleButton::SetAngleButton(ControlManager* manager, DialControl* dial, Real32 angle)
    : HingeButton(manager), m_dial(dial), m_angle(DegToRad(angle))
{
}

// Mac 0x120d30 (Win inlined into 0x68135f)
bool SetAngleButton::Create(Texture* image, const Rect& normal, const Rect& over, const Rect& pressed,
                            Control* parent, const Vector2& position)
{
    if (!Initialize())
        return false;
    Vector2 size;
    if (normal.IsFlipped())
        size = Vector2(normal.GetFlipped().GetWidth(), normal.GetFlipped().GetHeight());
    else
        size = Vector2(normal.GetWidth(), normal.GetHeight());
    Setup(parent, 0, 7, Rect(position, size), 0, false);
    StateDescription description;
    description.m_image = image;
    description.m_transparent = false;
    description.m_imageRect.Set(normal);
    SetStateDescription(0, description);
    description.m_imageRect.Set(over);
    SetStateDescription(1, description);
    description.m_imageRect.Set(pressed);
    SetStateDescription(2, description);
    return true;
}

// Mac 0x128cb0, Win 0x680dc5
void SetAngleButton::OnMouseReleased(const Vector2& /*position*/, UInt32 /*button*/)
{
    m_dial->SetAngle(m_angle);
}

// ===========================================================================================
// UndoHingeTool
// ===========================================================================================

// Mac 0x125c50
UndoHingeTool::UndoHingeTool(BrickConnectSceneControl* control, HingeTool* tool)
    : m_control(control), m_tool(tool), m_snapshot(0)
{
    m_control->GetTransforms(m_transforms);
    m_control->CreatePhysicsSnapshot(m_snapshot);
}

// Mac 0x127df0 (D0 0x127ec0, Win sdd 0x680add)
UndoHingeTool::~UndoHingeTool()
{
    if (m_tool)
        m_tool->m_undo = 0;
    if (m_snapshot) {
        m_snapshot->Release();
        m_snapshot = 0;
    }
}

// Mac 0x127f40, Win 0x68296f
void UndoHingeTool::Execute()
{
    if (!m_snapshot) {
        if (!m_transforms.IsEmpty())
            m_control->SetTransforms(m_transforms, true);
    } else {
        m_control->RestorePhysicsSnapshot(m_snapshot, true);
    }
    if (m_tool) {
        m_tool->m_undo = 0;
        m_tool->UpdateControlValues();
        m_tool = 0;
    }
}

// Mac 0x1269c0
void UndoHingeTool::Finish()
{
    m_tool = 0;
    ReleaseSnapshot();
    TransformArray changed;
    m_control->GetChangedTransforms(m_transforms, changed);
    if (changed.IsEmpty()) {
        m_transforms.Clear();
        m_control->GetUndoManager()->Undo();
    } else {
        m_transforms.Swap(changed);
    }
}

// (inlined, Mac 0x123412 / 0x125780)
void UndoHingeTool::Cancel()
{
    m_tool = 0;
    ReleaseSnapshot();
    m_transforms.Clear();
    m_control->GetUndoManager()->Undo();
}

// (inlined, Mac 0x1233cb)
void UndoHingeTool::ReleaseSnapshot()
{
    if (m_snapshot) {
        m_snapshot->Release();
        m_snapshot = 0;
    }
}

// ===========================================================================================
// HingeTool: construction
// ===========================================================================================

// Mac 0x11d9a0, Win 0x680e62
HingeTool::HingeTool()
    : m_rigidElement(0), m_element(0), m_undo(0), m_flag6c(true), m_axisIndex(0),
      m_dragging(false), m_radius(2.0f), m_repeatInterval(50), m_repeatDelay(750),
      m_repeatFirst(true), m_repeatStart(0)
{
    m_startAngle = 0.0f;   // not initialised by the original (HingeTool.h); fixed (low)
    SetCursor(0x6b);
}

// Mac 0x127630 (D1, body 0x126b70), 0x1278c0 (D0), Win sdd 0x6829d8
HingeTool::~HingeTool()
{
}

// Mac 0x11e1b0, Win 0x681f65
bool HingeTool::Create(ControlManager* manager, BrickConnectSceneManager* sceneManager)
{
    Render* render = static_cast<RenderControlManager*>(manager)->GetRender();
    RefPointer<Texture> arrow = LoadApplicationTexture(String("Graphics/EditScreen/Rotation_Arrow.png"), render);
    if (arrow.IsNull())
        return false;
    RefPointer<Texture> selected =
        LoadApplicationTexture(String("Graphics/EditScreen/Rotation_Arrow_Selected.png"), render);
    if (selected.IsNull())
        return false;

    CreateDialControls(manager);   // NOTE: the result is ignored (both builds)

    MaterialColors translucent;
    translucent.diffuse = translucent.ambient = translucent.specular = translucent.emissive =
        Color(1.0f, 1.0f, 1.0f, 0.35f);
    translucent.shininess = 0.0f;
    MaterialColors opaque;
    opaque.ambient = Color::White();
    opaque.diffuse = Color::White();
    opaque.specular = Color::White();
    opaque.emissive = Color::White();
    opaque.shininess = 0.0f;

    m_materials[0] = new TexturedMaterial(translucent, arrow);
    m_materials[1] = new ReverseWindingTexturedMaterial(translucent, arrow);
    m_materials[2] = new TexturedMaterial(opaque, arrow);
    m_materials[3] = new ReverseWindingTexturedMaterial(opaque, arrow);
    m_materials[4] = new TexturedMaterial(translucent, selected);
    m_materials[5] = new ReverseWindingTexturedMaterial(translucent, selected);
    m_materials[6] = new TexturedMaterial(opaque, selected);
    m_materials[7] = new ReverseWindingTexturedMaterial(opaque, selected);

    m_hingeFront = CreateSelectableMesh(render, Color::DarkGray(), m_materials[4], true, 0.5f, 1.0f, 64);
    m_hingeBack = CreateSelectableMesh(render, Color::DarkGray(), m_materials[5], false, 0.5f, 1.0f, 64);
    for (UInt32 i = 0; i < 3; ++i) {
        CylinderPair pair;
        pair.m_first = CreateSelectableMesh(render, Color::DarkGray(), m_materials[0], true, 0.5f, 1.0f, 64);
        pair.m_second = CreateSelectableMesh(render, Color::DarkGray(), m_materials[1], false, 0.5f, 1.0f, 64);
        m_rings.PushBack(pair);
    }
    m_boxMesh = CreateBoxMesh(render, Color::Red(), sceneManager->GetSilhouetteMaterial(), false);
    m_connectMesh = CreateBoxMesh(render, Color::Green(), sceneManager->GetSilhouetteMaterial(), false);

    // NOTE: the back half of the third ball band is not checked (both builds).
    return m_hingeFront.Get() && m_hingeBack.Get() && m_rings[0].m_first.Get() &&
           m_rings[0].m_second.Get() && m_rings[1].m_first.Get() && m_rings[1].m_second.Get() &&
           m_rings[2].m_first.Get() && !m_boxMesh.IsNull() && !m_connectMesh.IsNull();
}

// Mac 0x11f440, Win 0x68143f
bool HingeTool::CreateDialControls(ControlManager* manager)
{
    Render* render = static_cast<RenderControlManager*>(manager)->GetRender();
    RefPointer<Texture> icons = LoadApplicationTexture(String("Graphics/General/Icon_BI2.png"), render);

    m_background = new RenderControl(manager);
    if (!m_background->Initialize())
        return false;
    if (!m_background->Setup(0, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false))
        return false;
    Rect backgroundRect(Vector2(313.0f, 70.0f), Vector2(145.0f, 145.0f));
    m_background->SetImage(icons, &backgroundRect);

    m_dial = new DialControl(manager);
    if (!m_dial->Initialize(Vector2(30.0f, 80.0f), Vector2(15.5f, 72.0f)))
        return false;
    DialControl::StateDescription state;
    state.m_image = icons;
    state.m_offset = Vector2(413.0f, 221.0f);
    m_dial->SetStateDescription(0, state);
    state.m_offset = Vector2(445.0f, 221.0f);
    m_dial->SetStateDescription(1, state);
    m_dial->SetStateDescription(2, state);
    state.m_offset = Vector2(477.0f, 221.0f);
    m_dial->SetStateDescription(3, state);

    // the presets: horizontal / vertical / diagonal arrows (normal, over, pressed)
    const Vector2 wide(33.0f, 16.0f);
    const Rect wideNormal(Vector2(400.0f, 51.0f), wide);
    const Rect wideOver(Vector2(470.0f, 51.0f), wide);
    const Rect widePressed(Vector2(435.0f, 51.0f), wide);
    const Vector2 tall(16.0f, 33.0f);
    const Rect tallNormal(Vector2(451.0f, 16.0f), tall);
    const Rect tallOver(Vector2(487.0f, 16.0f), tall);
    const Rect tallPressed(Vector2(469.0f, 16.0f), tall);
    AddSetAngleButton(manager, 0.0f, Vector2(56.0f, 1.0f), icons, wideNormal, wideOver, widePressed);
    AddSetAngleButton(manager, 90.0f, Vector2(130.0f, 57.0f), icons, tallNormal, tallOver, tallPressed);
    AddSetAngleButton(manager, 180.0f, Vector2(56.0f, 130.0f), icons, wideNormal, wideOver, widePressed);
    AddSetAngleButton(manager, 270.0f, Vector2(1.0f, 56.0f), icons, tallNormal, tallOver, tallPressed);
    const Vector2 small(16.0f, 16.0f);
    const Rect smallNormal(Vector2(410.0f, 17.0f), small);
    const Rect smallOver(Vector2(427.0f, 34.0f), small);
    const Rect smallPressed(Vector2(427.0f, 17.0f), small);
    AddSetAngleButton(manager, 45.0f, Vector2(112.0f, 20.0f), icons, smallNormal, smallOver, smallPressed);
    AddSetAngleButton(manager, 135.0f, Vector2(112.0f, 112.0f), icons, smallNormal, smallOver, smallPressed);
    AddSetAngleButton(manager, 225.0f, Vector2(18.0f, 112.0f), icons, smallNormal, smallOver, smallPressed);
    AddSetAngleButton(manager, 315.0f, Vector2(18.0f, 20.0f), icons, smallNormal, smallOver, smallPressed);

    // the dial reacts inside the circle of the application mask only
    Rect maskRect(Vector2(0.0f, 105.0f), Vector2(147.0f, 147.0f));
    Vector2 pickPoint(70.0f, 70.0f);
    RefPointer<BitmapSelectionMask> mask = new BitmapSelectionMask;
    mask->SetMask(LDDApplication::GetInstance()->GetMask(), &maskRect);
    mask->Pick(pickPoint, true);
    m_dial->SetSelectionMask(mask);
    m_dial->SetParent(m_background, 0);
    m_background->SetPosition(Vector2(15.0f, 10.0f));
    m_background->SetSize(Vector2(146.0f, 146.0f));
    m_background->SetTransparent(true);
    return true;
}

// Mac 0x120b70, Win 0x68135f
bool HingeTool::AddSetAngleButton(ControlManager* manager, Real32 angle, const Vector2& position,
                                  Texture* image, const Rect& normal, const Rect& over,
                                  const Rect& pressed)
{
    RefPointer<SetAngleButton> button = new SetAngleButton(manager, m_dial, angle);
    if (!button->Create(image, normal, over, pressed, m_background, Vector2(position)))
        return false;
    m_presets.PushBack(RefPointer<Control>(button.Get()));
    return true;
}

// ---- control setters --------------------------------------------------------------------------

// Mac 0x120280, Win 0x6811eb
void HingeTool::SetMinusButton(ButtonControl* button)
{
    m_minus1 = button;
    ControlObserver::Attach(button);
}

// Mac 0x1202e0, Win 0x68120d
void HingeTool::SetPlusButton(ButtonControl* button)
{
    m_plus1 = button;
    ControlObserver::Attach(button);
}

// Mac 0x120340, Win 0x68122f
void HingeTool::SetTextInputControl(TextInputControl* control)
{
    m_text1 = control;
    m_text1->SetNumberType(TextInputControl::RealNumber);
    m_text1->SetFractionDigits(2);
    TextInputControlObserver::Attach(control);
}

// Mac 0x1208d0, Win 0x681267
void HingeTool::SetMinusButton2(ButtonControl* button)
{
    m_minus2 = button;
    ControlObserver::Attach(button);
}

// Mac 0x120930, Win 0x681289
void HingeTool::SetPlusButton2(ButtonControl* button)
{
    m_plus2 = button;
    ControlObserver::Attach(button);
}

// Mac 0x120990, Win 0x6812ab
void HingeTool::SetTextInputControl2(TextInputControl* control)
{
    m_text2 = control;
    m_text2->SetNumberType(TextInputControl::RealNumber);
    m_text2->SetFractionDigits(2);
    TextInputControlObserver::Attach(control);
}

// Mac 0x120a20, Win 0x6812e3
void HingeTool::SetMinusButton3(ButtonControl* button)
{
    m_minus3 = button;
    ControlObserver::Attach(button);
}

// Mac 0x120a80, Win 0x681305
void HingeTool::SetPlusButton3(ButtonControl* button)
{
    m_plus3 = button;
    ControlObserver::Attach(button);
}

// Mac 0x120ae0, Win 0x681327
void HingeTool::SetTextInputControl3(TextInputControl* control)
{
    m_text3 = control;
    m_text3->SetNumberType(TextInputControl::RealNumber);
    m_text3->SetFractionDigits(2);
    TextInputControlObserver::Attach(control);
}

// ===========================================================================================
// HingeTool: helpers
// ===========================================================================================

void HingeTool::RedrawSceneControl()
{
    BrickConnectSceneControl* control = GetSceneControl();
    if (!control->GetManager()->IsUpdateLocked())
        control->Invalidate(false);
}

RigidElement* HingeTool::GetBody() const
{
    return m_rigidElement ? m_rigidElement : BodyOf(m_element);
}

// Win 0x680207 (Mac inlined)
bool HingeTool::IsFirstBody() const
{
    RigidElement* body = GetBody();
    return GetJoint(m_manipulator)->GetRigidElement(true) == body->GetBody();
}

// Win 0x67fe36 (Mac inlined)
bool HingeTool::IsStepButton(Control* control) const
{
    return m_plus1 == control || m_plus2 == control || m_plus3 == control ||
           m_minus1 == control || m_minus2 == control || m_minus3 == control;
}

// Mac 0x121260, Win 0x6826da
void HingeTool::ResetControls()
{
    if (!m_background.IsNull())
        m_background->SetShown(false);
    DialControlObserver::Detach(m_dial);
    if (!m_minus1.IsNull())
        m_minus1->SetEnabled(false);
    if (!m_plus1.IsNull())
        m_plus1->SetEnabled(false);
    if (!m_text1.IsNull()) {
        m_text1->SetEnabled(false);
        m_text1->SetText(String());
    }
    if (!m_minus2.IsNull())
        m_minus2->SetEnabled(false);
    if (!m_plus2.IsNull())
        m_plus2->SetEnabled(false);
    if (!m_text2.IsNull()) {
        m_text2->SetEnabled(false);
        m_text2->SetText(String());
    }
    if (!m_minus3.IsNull())
        m_minus3->SetEnabled(false);
    if (!m_plus3.IsNull())
        m_plus3->SetEnabled(false);
    if (!m_text3.IsNull()) {
        m_text3->SetEnabled(false);
        m_text3->SetText(String());
    }
}

// Mac 0x123e20, Win 0x681bfa
void HingeTool::EnableHingeControls()
{
    m_background->SetShown(true);
    if (!m_minus1.IsNull())
        m_minus1->SetEnabled(true);
    if (!m_plus1.IsNull())
        m_plus1->SetEnabled(true);
    if (!m_text1.IsNull()) {
        m_text1->SetEnabled(true);
        m_text1->SetFloat(RadToDeg(m_manipulator->GetAngle()));
    }
    m_dial->SetAngle(m_manipulator->GetAngle());
    DialControlObserver::Attach(m_dial);
}

// Mac 0x123f00, Win 0x68259a
void HingeTool::EnableBallControls()
{
    Real32 angles[3];
    m_manipulator->SlideUnknown(&angles[0], &angles[1], &angles[2]);
    if (!m_background.IsNull())
        m_background->SetShown(false);
    if (!m_minus1.IsNull())
        m_minus1->SetEnabled(true);
    if (!m_plus1.IsNull())
        m_plus1->SetEnabled(true);
    if (!m_text1.IsNull()) {
        m_text1->SetEnabled(true);
        m_text1->SetFloat(RadToDeg(angles[0]));
    }
    if (!m_minus2.IsNull())
        m_minus2->SetEnabled(true);
    if (!m_plus2.IsNull())
        m_plus2->SetEnabled(true);
    if (!m_text2.IsNull()) {
        m_text2->SetEnabled(true);
        m_text2->SetFloat(RadToDeg(angles[1]));
    }
    if (!m_minus3.IsNull())
        m_minus3->SetEnabled(true);
    if (!m_plus3.IsNull())
        m_plus3->SetEnabled(true);
    if (!m_text3.IsNull()) {
        m_text3->SetEnabled(true);
        m_text3->SetFloat(RadToDeg(angles[2]));
    }
    DialControlObserver::Detach(m_dial);
}

// Mac 0x125990, Win 0x68280f
void HingeTool::UpdateControlValues()
{
    if (m_manipulator.IsNull())
        return;
    if (GetJoint(m_manipulator)->GetType() == PrimitiveConnection::JointHinge) {
        DialControlObserver::Detach(m_dial);
        m_dial->SetAngle(m_manipulator->GetAngle());
        DialControlObserver::Attach(m_dial);
        if (!m_text1.IsNull())
            m_text1->SetFloat(RadToDeg(m_manipulator->GetAngle()));
    }
    if (!m_manipulator.IsNull() && GetJoint(m_manipulator)->GetType() == PrimitiveConnection::JointBall) {
        Real32 angles[3];
        m_manipulator->SlideUnknown(&angles[0], &angles[1], &angles[2]);
        if (!m_text1.IsNull())
            m_text1->SetFloat(RadToDeg(angles[0]));
        if (!m_text2.IsNull())
            m_text2->SetFloat(RadToDeg(angles[1]));
        if (!m_text3.IsNull())
            m_text3->SetFloat(RadToDeg(angles[2]));
    }
}

// Mac 0x121540 (Win 0x68040d, unverified)
void HingeTool::UpdateRingAxes()
{
    if (m_manipulator.IsNull() || !m_manipulator->GetConnection())
        return;
    if (GetJoint(m_manipulator)->GetType() != PrimitiveConnection::JointBall)
        return;
    Vector3 axes[3];
    bool first = IsFirstBody();
    GetJoint(m_manipulator)->GetLocalFrame(first, &axes[1], &axes[2], 0);
    axes[0] = axes[2] ^ axes[1];
    axes[0].Normalize();
    m_axis = axes[m_axisIndex];
    RedrawSceneControl();
}

// Mac 0x121010, Win 0x68371c
void HingeTool::RotateBy(Real32 delta)
{
    if (!m_manipulator->GetConnection())
        return;
    UInt32 type = GetJoint(m_manipulator)->GetType();
    if (type == PrimitiveConnection::JointBall)
        Apply(IsFirstBody(), delta, &m_axis);
    else if (type == PrimitiveConnection::JointHinge)
        Apply(IsFirstBody(), delta, 0);
}

// Mac 0x1210f0, Win 0x683663
void HingeTool::Apply(bool first, Real32 delta, const Vector3* axis)
{
    if (!m_undo) {
        m_undo = new UndoHingeTool(GetSceneControl(), this);
        GetSceneControl()->GetUndoManager()->AddAction(m_undo);
    }
    bool moved;
    if (GetJoint(m_manipulator)->GetType() == PrimitiveConnection::JointHinge)
        moved = m_manipulator->Rotate(first, delta);
    else
        moved = m_manipulator->Slide(first, delta, axis);
    if (moved) {
        UpdateConnectParts();
        RedrawSceneControl();
    }
    UpdateControlValues();
}

// Mac 0x125d20, Win 0x68345b
bool HingeTool::UpdateConnectParts()
{
    m_connectParts.Clear();
    BasicArray<RigidElement*> bodies;
    CollectBodies(m_manipulator, bodies);
    Scene::ConnectResult result;
    if (!GetSceneControl()->CanConnect(bodies, result))
        return false;
    for (UInt32 i = 0; i < result.m_connections.GetSize(); ++i) {
        const PrimitiveConnectionInfo& info = result.m_connections[i];
        if (!Contains(bodies, BodyOf(info.m_element))) {
            m_connectParts.PushBack(RefPointer<Part>(info.m_element->GetPart()));
        } else if (!Contains(bodies, BodyOf(info.m_otherElement))) {
            m_connectParts.PushBack(RefPointer<Part>(info.m_otherElement->GetPart()));
        } else {
            m_connectParts.PushBack(RefPointer<Part>(info.m_element->GetPart()));
            m_connectParts.PushBack(RefPointer<Part>(info.m_otherElement->GetPart()));
        }
    }
    RedrawSceneControl();
    return true;
}

// Mac 0x1239b0 (Win 0x680064, unverified)
void HingeTool::SetActiveRing(const RefPointer<SelectableMesh>& first,
                              const RefPointer<SelectableMesh>& second)
{
    if (m_activeFront == first)
        return;
    RefPointer<SelectableMesh> oldFirst = m_activeFront;
    RefPointer<SelectableMesh> oldSecond = m_activeBack;
    m_activeFront = first;
    m_activeBack = second;
    oldFirst->SetMaterial(m_materials[0]);
    oldSecond->SetMaterial(m_materials[1]);
    first->SetMaterial(m_materials[6]);
    second->SetMaterial(m_materials[7]);
    RedrawSceneControl();
}

void HingeTool::ActivateRing()
{
    RefPointer<SelectableMesh> first = m_rings[m_axisIndex].m_first.Get();
    RefPointer<SelectableMesh> second = m_rings[m_axisIndex].m_second.Get();
    SetActiveRing(first, second);
}

// Mac 0x124740, Win 0x6804c6
void HingeTool::PreviousAxis()
{
    if (m_axisIndex == 0)
        m_axisIndex = 3;
    --m_axisIndex;
    m_dragging = false;
    ActivateRing();
    UpdateRingAxes();
}

// Mac 0x124880, Win 0x68054f
void HingeTool::NextAxis()
{
    m_axisIndex = (m_axisIndex + 1) % 3;
    m_dragging = false;
    ActivateRing();
    UpdateRingAxes();
}

// Mac 0x124b50, Win 0x683f87
void HingeTool::BallMinus(UInt32 index)
{
    m_axisIndex = index % 3;
    UpdateRingAxes();
    Apply(IsFirstBody(), IsFirstBody() ? ButtonStep() : -ButtonStep(), &m_axis);
    ActivateRing();
}

// Mac 0x124d20, Win 0x684042
void HingeTool::BallPlus(UInt32 index)
{
    m_axisIndex = index % 3;
    UpdateRingAxes();
    Apply(IsFirstBody(), IsFirstBody() ? -ButtonStep() : ButtonStep(), &m_axis);
    ActivateRing();
}

// Mac 0x123680, Win 0x682ae6
void HingeTool::StartRepeat(Control* control)
{
    m_repeatControl = control;
    m_repeatFirst = true;
    m_repeatStart = 0;
    RefPointer<Updater> updater = MainUpdater::GetInstance();
    UpdateObserver::Attach(updater);
}

void HingeTool::StopRepeat()
{
    UpdateObserver::Detach();
    m_repeatControl = 0;
}

// ===========================================================================================
// HingeTool: observers
// ===========================================================================================

// Mac 0x121210, Win 0x6829fa
// NOTE: the original keeps a repeat that has not ended when the tool is detached (no release /
// leave of the button seen): the main updater keeps calling Update and the joint picked after
// the next activation turns on its own; fixed: the repeat ends on activation (M17).
void HingeTool::OnAttached(BrickConnectSceneControl* control)
{
    StopRepeat();
    BrickConnectSceneControlTool::OnAttached(control);
    GetSceneControl()->ClearSelection();
    m_background->SetParent(control, 0);
    ResetControls();
}

// Mac 0x121440, Win 0x682a30
void HingeTool::OnDetached(BrickConnectSceneControl* control)
{
    if (!m_manipulator.IsNull()) {
        EndManipulation();
        m_manipulator = 0;
        m_rigidElement = 0;
        m_element = 0;
        ResetControls();
        if (!control->GetManager()->IsUpdateLocked())
            control->Invalidate(false);
    }
    m_dragging = false;
    m_background->SetParent(0, 0);
    BrickConnectSceneControlTool::OnDetached(control);
}

// Mac 0x125050, Win 0x67fe74
void HingeTool::OnEvent05(const Matrix3D& transform, Render* /*render*/, RenderQueue* overlayQueue,
                          RenderQueue* /*sceneQueue*/)
{
    Vector3 up(0.0f, 1.0f, 0.0f);   // (not used, both builds)
    (void)up;
    if (m_manipulator.IsNull())
        return;
    if (m_manipulator->GetConnection()) {
        Matrix3D frame;
        BoundingBox box(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        RigidElement* body = GetBody();
        body->GetTransform(frame);
        body->GetBoundingBox(box, true);
        Matrix3D step;
        step.SetTranslationMatrix(box.m_min);
        frame.Multiply(step);
        Real32 width = box.GetWidth();
        Real32 height = box.GetHeight();
        step.SetScaling(width, height, box.GetDepth());
        frame.Multiply(step);
        step.Multiply(transform, frame);
        overlayQueue->Add(m_boxMesh, step, 0.0f, 1, 1.0f);
    }
    for (BasicArray<RefPointer<Part> >::ConstIterator part = m_connectParts.Begin();
         part != m_connectParts.End(); ++part) {
        Matrix3D box;
        GetPartBoxTransform(*part, box, false);
        overlayQueue->Add(m_connectMesh, box, 0.0f, 0, 1.0f);
    }
}

// Mac 0x125240, Win 0x6805cd
void HingeTool::OnEvent06(Render* render)
{
    Vector3 up(0.0f, 1.0f, 0.0f);   // (not used, both builds)
    (void)up;
    if (m_manipulator.IsNull() || !m_manipulator->GetConnection())
        return;
    Matrix3D camera;
    camera.SetInverse(render->GetViewMatrix());
    Vector3 toCamera = camera.GetTranslation() - m_center;
    toCamera.Normalize();
    Plane facing(toCamera, m_center);   // (not used, both builds)
    (void)facing;
    Vector3 a;
    Vector3 b;
    Vector3 unused;
    Vector3 c;
    bool first = IsFirstBody();
    GetJoint(m_manipulator)->GetLocalFrame(first, &a, &b, 0);
    Matrix3D frame;
    GetBody()->GetTransform(frame);
    a = Rotate(a, frame);
    b = Rotate(b, frame);
    c = a ^ b;
    UInt32 type = GetJoint(m_manipulator)->GetType();
    if (type == PrimitiveConnection::JointBall) {
        DrawRing(a, m_center, m_rings[0].m_first, render, true);
        DrawRing(a, m_center, m_rings[0].m_second, render, false);
        DrawRing(b, m_center, m_rings[1].m_first, render, true);
        DrawRing(b, m_center, m_rings[1].m_second, render, false);
        DrawRing(c, m_center, m_rings[2].m_first, render, true);
        DrawRing(c, m_center, m_rings[2].m_second, render, false);
    } else if (type == PrimitiveConnection::JointHinge) {
        DrawRing(a, m_center, m_hingeFront, render, true);
        DrawRing(a, m_center, m_hingeBack, render, false);
    }
}

// Mac 0x125500, Win 0x67f909
void HingeTool::DrawRing(const Vector3& axis, const Vector3& center, SelectableMesh* mesh,
                         Render* render, bool front)
{
    Vector3 up(0.0f, 1.0f, 0.0f);
    Matrix3D world;
    Matrix3D scaling;
    scaling.SetScaling(m_radius, m_radius, m_radius);
    if (!up.IsEqual(axis, EPSILON) && !up.IsEqual(-axis, EPSILON)) {
        Vector3 rotationAxis = up ^ axis;
        world.SetRotation(rotationAxis, ACos(up * axis));
        world.Multiply(scaling);
    } else {
        world = scaling;
    }
    world.SetTranslation(center);
    render->SetWorldMatrix(world);
    render->EnableBlend(true);
    render->EnableDepthTest(false);
    if (!front) {
        render->SetOpacity(0.25f);
        mesh->Bind(render, 0.0f, true);
        render->DrawVertexBuffer();
        render->SetOpacity(1.0f);
    } else {
        mesh->Bind(render, 0.0f, true);
        render->DrawVertexBuffer();
    }
    render->EnableDepthTest(true);
    render->EnableBlend(false);
}

// Mac 0x125780, Win 0x682ab9
void HingeTool::OnEvent19()
{
    if (m_undo) {
        m_undo->Cancel();
        m_undo = 0;
    }
    m_rigidElement = 0;
    m_element = 0;
    m_manipulator = 0;
    ResetControls();
}

// Mac 0x1214e0 (thunk 0x121510), Win 0x683764
void HingeTool::OnAngleChanged(DialControl* /*dial*/, Real32 angle, Real32 oldAngle)
{
    RotateBy(angle - oldAngle);
}

// Mac 0x121690, Win 0x683783
void HingeTool::OnMouseMove(Control* control, const Vector2& position)
{
    if (control != GetSceneControl())
        return;
    if (m_dragging) {
        if (m_manipulator.IsNull())
            return;
        bool first = IsFirstBody();
        Vector3 direction;
        Vector3 origin;
        Vector3 hit;
        Real32 length;
        GetSceneControl()->GetPickRay(position, origin, direction, length);
        UInt32 type = GetJoint(m_manipulator)->GetType();
        if (type == PrimitiveConnection::JointBall) {
            // seen from behind the sphere: through its surface, along the band normal
            if (RaySphereIntersect(origin, direction, m_center, m_radius, &hit)) {
                origin = hit;
                if (m_plane.GetDistance(origin) >= 0.0f)
                    direction = -m_plane.m_normal;
                else
                    direction = Vector3(m_plane.m_normal);
            }
            if (!RayPlaneIntersect(origin, direction, m_plane, hit))
                return;
            hit -= m_center;
            hit.Normalize();
            Real32 angle = ACos(hit * m_reference);
            Vector3 cross = m_reference ^ hit;
            if (cross * m_plane.m_normal < 0.0f)
                angle = -angle;
            m_reference = hit;
            Apply(first, angle, &m_axis);
        } else if (type == PrimitiveConnection::JointHinge) {
            if (RayCylinderIntersect(origin, direction, m_center, m_plane.m_normal, m_radius, m_radius, &hit)) {
                origin = hit;
                if (m_plane.GetDistance(origin) >= 0.0f)
                    direction = -m_plane.m_normal;
                else
                    direction = Vector3(m_plane.m_normal);
            }
            if (!RayPlaneIntersect(origin, direction, m_plane, hit))
                return;
            hit -= m_center;
            hit.Normalize();
            Vector3 cross;
            Real32 dot;
            if (first) {
                dot = hit * m_reference;
                cross = hit ^ m_reference;
            } else {
                dot = m_reference * hit;
                cross = m_reference ^ hit;
            }
            Real32 angle = ACos(dot);
            if (cross * m_plane.m_normal < 0.0f)
                angle = -angle;
            Real32 delta = (Real32)((EvalReal)m_startAngle + angle - m_manipulator->GetAngle());
            Apply(first, delta, 0);
        }
        return;
    }

    if (m_manipulator.IsNull())
        return;
    Vector2 point(position);
    Vector3 direction;
    Vector3 origin;
    Vector3 hit;
    Real32 length;
    GetSceneControl()->GetPickRay(point, origin, direction, length);
    Plane view(direction, m_center);
    RayPlaneIntersect(origin, direction, view, hit);
    Real32 distance = (hit - m_center).GetLength();
    if (distance <= m_radius) {
        // inside the bands: highlight the one under the mouse
        if (!m_manipulator->GetConnection())
            return;
        UInt32 type = GetJoint(m_manipulator)->GetType();
        if (type == PrimitiveConnection::JointBall) {
            Vector3 axes[3];
            bool first = IsFirstBody();
            GetJoint(m_manipulator)->GetLocalFrame(first, &axes[0], &axes[1], 0);
            axes[2] = axes[0] ^ axes[1];
            axes[2].Normalize();
            Matrix3D frame;
            GetBody()->GetTransform(frame);
            Vector3 rayOrigin;
            Vector3 rayDirection;
            Vector3 bandHit;
            Vector3 edgeHit;
            Real32 rayLength;
            GetSceneControl()->GetPickRay(position, rayOrigin, rayDirection, rayLength);
            Vector3 toEye = rayOrigin - m_center;
            toEye.Normalize();
            Plane facing(toEye, m_center);
            for (UInt32 i = 0; i < 3; ++i) {
                Vector3 axis = Rotate(axes[i], frame);
                Plane band(axis, m_center);
                if (!RayPlaneIntersect(rayOrigin, rayDirection, band, bandHit))
                    continue;
                Vector3 radial = bandHit - m_center;
                radial.SetLength(m_radius);
                Vector3 rim = m_center + radial;
                radial.Normalize();
                Plane tangent(radial, rim);
                if (!RayPlaneIntersect(rayOrigin, rayDirection, tangent, edgeHit))
                    continue;
                Real32 offset = (edgeHit - bandHit).GetLength();
                m_axisIndex = i;
                Material* front;
                Material* back;
                if (!IsWithin(offset, m_radius * 0.5f, 0.2f)) {
                    bool active = m_rings[i].m_first.Get() == m_activeFront;
                    front = active ? m_materials[4] : m_materials[0];
                    back = active ? m_materials[5] : m_materials[1];
                } else {
                    if (facing.GetDistance(bandHit) < 0.0f)
                        continue;
                    bool active = m_rings[i].m_first.Get() == m_activeFront;
                    front = active ? m_materials[6] : m_materials[2];
                    back = active ? m_materials[7] : m_materials[3];
                }
                m_rings[m_axisIndex].m_first->SetMaterial(front);
                m_rings[m_axisIndex].m_second->SetMaterial(back);
                RedrawSceneControl();
            }
            return;
        }
        if (type != PrimitiveConnection::JointHinge)
            return;
        Vector3 axis;
        GetJoint(m_manipulator)->GetWorldFrame(&axis, 0, 0);
        Vector3 rayDirection;
        Vector3 rayOrigin;
        Vector3 bandHit;
        Vector3 edgeHit;
        Real32 rayLength;
        GetSceneControl()->GetPickRay(position, rayOrigin, rayDirection, rayLength);
        Plane band(axis, m_center);
        if (!RayPlaneIntersect(rayOrigin, rayDirection, band, bandHit))
            return;
        Vector3 radial = bandHit - m_center;
        radial.SetLength(m_radius);
        Vector3 rim = m_center + radial;
        radial.Normalize();
        Plane tangent(radial, rim);
        if (!RayPlaneIntersect(rayOrigin, rayDirection, tangent, edgeHit))
            return;
        Real32 offset = (bandHit - edgeHit).GetLength();
        if (!IsWithin(offset, m_radius * 0.5f, 0.2f)) {
            m_hingeFront->SetMaterial(m_materials[4]);
            m_hingeBack->SetMaterial(m_materials[5]);
        } else {
            m_hingeFront->SetMaterial(m_materials[6]);
            m_hingeBack->SetMaterial(m_materials[7]);
        }
        RedrawSceneControl();
        return;
    }

    // outside the bands: no highlight
    if (!m_manipulator->GetConnection())
        return;
    UInt32 type = GetJoint(m_manipulator)->GetType();
    if (type == PrimitiveConnection::JointHinge) {
        m_hingeFront->SetMaterial(m_materials[4]);
        m_hingeBack->SetMaterial(m_materials[5]);
        RedrawSceneControl();
    } else if (type == PrimitiveConnection::JointBall) {
        // NOTE: original quirk: the loop leaves m_axisIndex at 2.
        for (UInt32 i = 0; i < 3; ++i) {
            m_axisIndex = i;
            if (m_rings[i].m_first.Get() == m_activeFront) {
                m_rings[m_axisIndex].m_first->SetMaterial(m_materials[4]);
                m_rings[m_axisIndex].m_second->SetMaterial(m_materials[5]);
            } else {
                m_rings[m_axisIndex].m_first->SetMaterial(m_materials[0]);
                m_rings[m_axisIndex].m_second->SetMaterial(m_materials[1]);
            }
            RedrawSceneControl();
        }
    }
}

// Mac 0x122690, Win 0x682b7b
void HingeTool::OnMousePressed(Control* control, const Vector2& position, UInt32 button)
{
    if (button == 0 && IsStepButton(control))
        StartRepeat(control);
    if (button != 0 || control != GetSceneControl())
        return;

    if (!m_manipulator.IsNull()) {
        // a press on a band starts dragging it
        Vector2 point(position);
        Vector3 direction;
        Vector3 origin;
        Vector3 hit;
        Real32 length;
        GetSceneControl()->GetPickRay(point, origin, direction, length);
        Plane view(direction, m_center);
        RayPlaneIntersect(origin, direction, view, hit);
        Real32 distance = (hit - m_center).GetLength();
        if (!(m_radius < distance) && m_manipulator->GetConnection()) {
            UInt32 type = GetJoint(m_manipulator)->GetType();
            if (type == PrimitiveConnection::JointBall) {
                Vector3 axes[3];
                bool first = IsFirstBody();
                GetJoint(m_manipulator)->GetLocalFrame(first, &axes[0], &axes[1], 0);
                axes[2] = axes[0] ^ axes[1];
                axes[2].Normalize();
                Matrix3D frame;
                GetBody()->GetTransform(frame);
                Vector3 rayDirection;
                Vector3 rayOrigin;
                Vector3 bandHit;
                Vector3 edgeHit;
                Real32 rayLength;
                GetSceneControl()->GetPickRay(position, rayOrigin, rayDirection, rayLength);
                Vector3 toEye = rayOrigin - m_center;
                toEye.Normalize();
                Plane facing(toEye, m_center);
                for (UInt32 i = 0; i < 3; ++i) {
                    Vector3 axis = Rotate(axes[i], frame);
                    m_plane = Plane(axis, m_center);
                    if (!RayPlaneIntersect(rayOrigin, rayDirection, m_plane, bandHit))
                        continue;
                    if (facing.GetDistance(bandHit) < 0.0f)
                        continue;
                    Vector3 radial = bandHit - m_center;
                    radial.SetLength(m_radius);
                    Vector3 rim = m_center + radial;
                    radial.Normalize();
                    Plane tangent(radial, rim);
                    m_reference = bandHit - m_center;
                    if (!RayPlaneIntersect(rayOrigin, rayDirection, tangent, edgeHit))
                        continue;
                    if (IsWithin((edgeHit - bandHit).GetLength(), m_radius * 0.5f, 0.2f)) {
                        m_dragging = true;
                        m_reference.Normalize();
                        m_axisIndex = i;
                        m_axis = axes[i];
                        RefPointer<SelectableMesh> ringFirst = m_rings[i].m_first.Get();
                        RefPointer<SelectableMesh> ringSecond = m_rings[i].m_second.Get();
                        SetActiveRing(ringFirst, ringSecond);
                        return;
                    }
                }
            } else if (type == PrimitiveConnection::JointHinge) {
                Vector3 axis;
                GetJoint(m_manipulator)->GetWorldFrame(&axis, 0, 0);
                Vector3 rayDirection;
                Vector3 rayOrigin;
                Vector3 bandHit;
                Vector3 edgeHit;
                Real32 rayLength;
                GetSceneControl()->GetPickRay(position, rayOrigin, rayDirection, rayLength);
                Vector3 toEye = rayOrigin - m_center;
                toEye.Normalize();
                Plane facing(toEye, m_center);   // (not used, both builds)
                (void)facing;
                m_plane = Plane(axis, m_center);
                if (RayPlaneIntersect(rayOrigin, rayDirection, m_plane, bandHit)) {
                    Vector3 radial = bandHit - m_center;
                    radial.SetLength(m_radius);
                    Vector3 rim = m_center + radial;
                    radial.Normalize();
                    Plane tangent(radial, rim);
                    m_reference = bandHit - m_center;
                    if (RayPlaneIntersect(rayOrigin, rayDirection, tangent, edgeHit) &&
                        IsWithin((edgeHit - bandHit).GetLength(), m_radius * 0.5f, 0.2f)) {
                        m_dragging = true;
                        m_startAngle = m_manipulator->GetAngle();
                        m_reference.Normalize();
                        RedrawSceneControl();
                        return;
                    }
                }
            }
        }
    }

    // a click on a part: its joints
    BasicArray<RefPointer<Part> > filterParts;
    BasicArray<RefPointer<Part> > extraParts;
    PrimitiveElement* picked = GetSceneControl()->PickElement(position, 0, true, 0, true, filterParts,
                                                               extraParts);
    RigidElement* body = picked ? BodyOf(picked) : 0;
    bool samePart = m_element ? picked == m_element : body == m_rigidElement;
    if (!samePart) {
        EndManipulation();
        m_element = picked;
        m_rigidElement = 0;
        m_manipulator = 0;
        ResetControls();
        if (m_element) {
            m_manipulator = m_element->CreateManipulator();
            body = BodyOf(picked);
        }
        if (body && m_manipulator.IsNull()) {
            m_element = 0;
            m_rigidElement = body;
            m_manipulator = body->CreateManipulator();
        }
        if (!m_manipulator.IsNull()) {
            GetJoint(m_manipulator)->GetWorldFrame(0, 0, &m_center);
            m_axisIndex = 0;
            m_activeFront = m_rings[0].m_first.Get();
            m_activeBack = m_rings[0].m_second.Get();
            UpdateRingAxes();
            DialControlObserver::Detach(m_dial);
            UInt32 type = GetJoint(m_manipulator)->GetType();
            if (type == PrimitiveConnection::JointHinge)
                EnableHingeControls();
            else if (GetJoint(m_manipulator)->GetType() == PrimitiveConnection::JointBall)
                EnableBallControls();
            DialControlObserver::Attach(m_dial);
            m_dragging = false;
        } else {
            m_element = 0;
            m_rigidElement = 0;
            m_dragging = false;
        }
    } else {
        if (!m_rigidElement && !m_element)
            return;
        bool wrapped;
        if (!m_manipulator->SelectNext(&wrapped)) {
            EndManipulation();
            m_rigidElement = 0;
            m_element = 0;
            m_manipulator = 0;
            m_dragging = false;
            ResetControls();
        } else if (!wrapped) {
            if (m_undo)
                m_undo->ReleaseSnapshot();
        } else {
            EndManipulation();
            if (!m_manipulator.IsNull()) {
                if (!m_manipulator->GetConnection()) {
                    if (m_undo) {
                        m_undo->Cancel();
                        m_undo = 0;
                    }
                    m_rigidElement = 0;
                    m_element = 0;
                    m_manipulator = 0;
                    m_dragging = false;
                    ResetControls();
                } else {
                    GetJoint(m_manipulator)->GetWorldFrame(0, 0, &m_center);
                    m_axisIndex = 0;
                    m_activeFront = m_rings[0].m_first.Get();
                    m_activeBack = m_rings[0].m_second.Get();
                    UpdateRingAxes();
                    DialControlObserver::Detach(m_dial);
                    if (GetJoint(m_manipulator)->GetType() == PrimitiveConnection::JointHinge)
                        EnableHingeControls();
                    else if (GetJoint(m_manipulator)->GetType() == PrimitiveConnection::JointBall)
                        EnableBallControls();
                    DialControlObserver::Attach(m_dial);
                }
            }
        }
    }
    ResetRingMaterials();
    RedrawSceneControl();
}

// Mac 0x1245e0 (thunk 0x124650), Win 0x682a85
void HingeTool::OnMouseReleased(Control* control, const Vector2& /*position*/, UInt32 /*button*/)
{
    StopRepeat();
    if (control == GetSceneControl())
        m_dragging = false;
}

// Mac 0x1246d0 (thunk 0x1249c0), Win 0x68424d
void HingeTool::OnKeyPressed(Control* control, UInt32 key, UInt32 modifiers, bool /*repeat*/)
{
    if (control != GetSceneControl())
        return;
    if (key == 0xcd) {
        if (!m_manipulator.IsNull())
            RotateBy(-KeyStep());
    } else if (key == 0xcb) {
        if (!m_manipulator.IsNull())
            RotateBy(KeyStep());
    } else if (key == 0xf) {
        if (modifiers & 2)
            PreviousAxis();
        else
            NextAxis();
    }
}

// Mac 0x124a40 (thunk 0x124ef0), Win 0x684297
void HingeTool::OnMouseClicked(Control* control, const Vector2& /*position*/, UInt32 /*button*/)
{
    if (m_manipulator.IsNull())
        return;
    if (m_minus1 == control) {
        if (GetJoint(m_manipulator)->GetType() == PrimitiveConnection::JointHinge)
            RotateBy(-ButtonStep());
        else
            BallMinus(0);
    } else if (m_plus1 == control) {
        if (GetJoint(m_manipulator)->GetType() == PrimitiveConnection::JointHinge)
            RotateBy(ButtonStep());
        else
            BallPlus(0);
    } else if (m_minus2 == control) {
        BallMinus(1);
    } else if (m_plus2 == control) {
        BallPlus(1);
    } else if (m_minus3 == control) {
        BallMinus(2);
    } else if (m_plus3 == control) {
        BallPlus(2);
    }
}

// Mac 0x124f10 (thunk 0x124fb0), Win 0x682aa1
void HingeTool::OnMouseLeave(Control* control, const Vector2& /*position*/)
{
    if (m_plus1 == control || m_plus2 == control || m_plus3 == control ||
        m_minus1 == control || m_minus2 == control || m_minus3 == control)
        StopRepeat();
}

// Mac 0x125800 (thunk 0x125b00), Win 0x6840fd
void HingeTool::OnTextEntered(TextInputControl* control)
{
    if (m_manipulator.IsNull())
        return;
    if (GetJoint(m_manipulator)->GetType() == PrimitiveConnection::JointHinge) {
        Real32 target = DegToRad(control->GetFloat());
        RotateBy((Real32)((EvalReal)target - m_manipulator->GetAngle()));
    }
    if (GetJoint(m_manipulator)->GetType() == PrimitiveConnection::JointBall) {
        Real32 angles[3];
        m_manipulator->SlideUnknown(&angles[0], &angles[1], &angles[2]);
        // NOTE: the original leaves the delta undefined (a stale register) when the control is
        // none of the three; 0 here.
        Real32 delta = 0.0f;
        if (m_text1 == control) {
            m_axisIndex = 0;
            delta = (Real32)((EvalReal)angles[0] - DegToRad(control->GetFloat()));
        }
        if (m_text2 == control) {
            m_axisIndex = 1;
            delta = (Real32)((EvalReal)angles[1] - DegToRad(control->GetFloat()));
        }
        if (m_text3 == control) {
            m_axisIndex = 2;
            delta = (Real32)((EvalReal)angles[2] - DegToRad(control->GetFloat()));
        }
        UpdateRingAxes();
        RotateBy(delta);
        UpdateControlValues();
    }
}

// Mac 0x125b20, Win 0x6807e7
void HingeTool::ResetRingMaterials()
{
    m_rings[0].m_first->SetMaterial(m_materials[4]);
    m_rings[0].m_second->SetMaterial(m_materials[5]);
    for (UInt32 i = 1; i < m_rings.GetSize(); ++i) {
        m_rings[i].m_first->SetMaterial(m_materials[0]);
        m_rings[i].m_second->SetMaterial(m_materials[1]);
    }
}

// Mac 0x1267e0, Win 0x6833b5
void HingeTool::EndManipulation()
{
    if (!m_undo)
        return;
    m_undo->Finish();
    m_undo = 0;
    BasicArray<RigidElement*> bodies;
    CollectBodies(m_manipulator, bodies);
    Scene::ConnectResult result;
    if (GetSceneControl()->CanConnect(bodies, result))
        GetSceneControl()->Reconnect(result);
    m_connectParts.Clear();
}

// Mac 0x126aa0 (thunk 0x126b50), Win 0x680157
// NOTE: the original never resets the start time of the repeat, so after the 750 ms delay every
// pass repeats (not every 50 ms); fixed: each repeat starts the interval again (M17).
void HingeTool::Update()
{
    if (m_repeatControl.IsNull())
        return;
    UInt64 now = (UInt64)GetMilliseconds();
    if (m_repeatStart == 0) {
        m_repeatStart = now;
        return;
    }
    if (m_repeatFirst) {
        if (m_repeatStart + m_repeatDelay >= now)
            return;
        m_repeatFirst = false;
    } else {
        if (m_repeatStart + m_repeatInterval >= now)
            return;
    }
    m_repeatStart = now;
    OnMouseClicked(m_repeatControl, Vector2(0.0f, 0.0f), 0);
}

// Mac 0x127bb0 (thunks 0x127d50-0x127dd0), Win 0x682b73
void HingeTool::OnFinalRelease()
{
    TextInputControlObserver::OnFinalRelease();
    DialControlObserver::OnFinalRelease();
    BrickConnectSceneControlTool::OnFinalRelease();
    UpdateObserver::OnFinalRelease();
}

} // namespace LEGO
