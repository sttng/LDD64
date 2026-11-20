// LEGO Digital Designer — restored source
// LEGO::HingeAlignTool. See HingeAlignTool.h.
//
// Original object file: HingeAlignTool.cpp (Mac TU 066 0x158120-0x15b6ab; Win 0x665301-0x66753f).
#include "HingeAlignTool.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneManager.h"
#include "ConnectionField.h"
#include "ConnectionFieldPlanar.h"
#include "ControlManager.h"
#include "Materials.h"
#include "ObjectFactory.h"
#include "Path.h"
#include "Platform.h"
#include "PrimitiveElement.h"
#include "PrimitiveRigidElement.h"
#include "Render.h"
#include "RigidElement.h"
#include "Scene.h"
#include "LEGO/String.h"
#include "SystemTime.h"
#include "Texture.h"
#include "UndoManager.h"
#include "VertexBuffer.h"

#include <typeinfo>

namespace LEGO {

namespace {

// Invalidates the scene control unless the manager's updates are locked.
void Redraw(BrickConnectSceneControl* control)
{
    if (!control->GetManager()->IsUpdateLocked())
        control->Invalidate(false);
}

// std::find with the feature equality (element, field, point, offset). Mac 0x15ae70,
// Win 0x665ee6
bool Contains(const BasicArray<Scene::Feature>& features, const Scene::Feature& feature)
{
    for (UInt32 i = 0; i < features.GetSize(); ++i) {
        const Scene::Feature& f = features[i];
        if (f.m_element == feature.m_element && f.m_field == feature.m_field &&
            f.m_index == feature.m_index && f.m_offset == feature.m_offset)
            return true;
    }
    return false;
}

// Every feature of `a` is in `b` (and they have the same size). (inlined in Mac 0x1596f0)
bool SameFeatures(const BasicArray<Scene::Feature>& a, const BasicArray<Scene::Feature>& b)
{
    if (a.GetSize() != b.GetSize())
        return false;
    for (UInt32 i = 0; i < a.GetSize(); ++i) {
        if (!Contains(b, a[i]))
            return false;
    }
    return true;
}

// A texture of the arrow quads. (inlined three times in Mac 0x158120)
Texture* LoadArrowTexture(Render* render, const Texture::LoadArgs2D& args)
{
    Object* object = render->GetObjectFactory()->CreateObject(typeid(Texture), args);
    return object ? dynamic_cast<Texture*>(object) : 0;
}

} // namespace

// Mac 0x158120, Win 0x665a32
bool HingeAlignTool::Create(Render* render, BrickConnectSceneManager* /*sceneManager*/)
{
    VertexBuffer::CreateArgs args;
    args.vertexCount = 4;
    args.indexCount = 4;
    args.frameCount = 0;
    args.dynamic = false;
    args.hasNormals = false;
    args.colorSetCount = 0;
    args.texCoordDimensions.PushBack(2);
    Object* object = render->GetObjectFactory()->CreateObject(typeid(VertexBuffer), args);
    if (object == 0)
        return false;
    RefPointer<VertexBuffer> buffer = dynamic_cast<VertexBuffer*>(object);
    if (buffer.IsNull())
        return false;

    const Vector3 positions[4] = {
        Vector3(-2.0f, 0.0f, -2.0f), Vector3(2.0f, 0.0f, -2.0f),
        Vector3(-2.0f, 0.0f, 2.0f), Vector3(2.0f, 0.0f, 2.0f)
    };
    const Vector2 texCoords[4] = {
        Vector2(0.0f, 0.0f), Vector2(1.0f, 0.0f), Vector2(0.0f, 1.0f), Vector2(1.0f, 1.0f)
    };
    const UInt32 indices[4] = {0, 1, 2, 3};
    buffer->SetPrimitiveType(4);
    buffer->SetPositions(positions, 0, 4, 0);
    buffer->SetIndices(indices, 0, 4, 0);
    buffer->SetTexCoords((const Real32*)&texCoords[0], 0);

    Texture::LoadArgs2D load;
    load.path.SetElement(Platform::GetInstance()->GetSystemDirectory(9, true).Get());
    load.path.Append(String("Graphics/EditScreen/H-A_arrowCircle1.png"));
    MaterialColors colors;
    colors.shininess = 0.0f;

    RefPointer<Texture> texture = LoadArrowTexture(render, load);
    if (texture.IsNull())
        return false;
    m_meshes[0] = new SingleMesh(new TexturedMaterial(colors, texture), buffer);

    load.path.SetName(String("H-A_arrowCircle2.png"));
    texture = LoadArrowTexture(render, load);
    if (texture.IsNull())
        return false;
    m_meshes[1] = new SingleMesh(new TexturedMaterial(colors, texture), buffer);

    load.path.SetName(String("H-A_arrowCircle2B.png"));
    texture = LoadArrowTexture(render, load);
    if (texture.IsNull())
        return false;
    m_meshes[2] = new SingleMesh(new TexturedMaterial(colors, texture), buffer);
    return true;
}

// Mac 0x15a690
void HingeAlignTool::OnFinalRelease()
{
    UpdateObserver::OnFinalRelease();
    BrickConnectSceneControlTool::OnFinalRelease();
}

// Mac 0x158e40, Win 0x665301
void HingeAlignTool::OnAttached(BrickConnectSceneControl* control)
{
    BrickConnectSceneControlTool::OnAttached(control);
    GetSceneControl()->ClearSelection();
    SetCursor(0x71);
}

// Mac 0x158e80, Win 0x6664f0
void HingeAlignTool::OnDetached(BrickConnectSceneControl* control)
{
    if (!m_first.IsEmpty() || !m_second.IsEmpty() || !m_hover.IsEmpty())
        Redraw(GetSceneControl());
    if (UpdateObserver::GetSubject() != 0) {
        GetSceneControl()->GetScene()->PhysicsCall6(false);
        m_transforms.Clear();
        UpdateObserver::Detach();
        BrickConnectSceneControl* sceneControl = GetSceneControl();
        sceneControl->GetManager()->SetModalControl(false, sceneControl);
    }
    m_first.Clear();
    m_second.Clear();
    m_hover.Clear();
    SetCursor(0x71);
    BrickConnectSceneControlTool::OnDetached(control);
}

// Mac 0x158f90, Win 0x666e53
void HingeAlignTool::UpdateHover(const Vector2& position)
{
    BasicArray<Scene::Feature> previous(m_hover);   // NOTE: not used (both builds)
    (void)previous;
    BasicArray<Scene::Feature> picked;
    GetSceneControl()->PickFeatures(position, picked);
    // only planar and axle fields (std::remove_if)
    BasicArray<Scene::Feature> kept;
    for (UInt32 i = 0; i < picked.GetSize(); ++i) {
        if ((picked[i].m_field->GetType() | 2) == 2)
            kept.PushBack(picked[i]);
    }
    if (kept.GetSize() == m_hover.GetSize()) {
        bool changed = false;
        for (UInt32 i = 0; i < kept.GetSize() && !changed; ++i)
            changed = !Contains(m_hover, kept[i]);
        if (!changed)
            return;
    }
    if (!kept.IsEmpty() && !m_first.IsEmpty() && !FindFeaturePair(m_first, kept, 0))
        kept.Clear();
    m_hover = kept;
    Redraw(GetSceneControl());
}

// Mac 0x158f70 (thunk 0x159210)
void HingeAlignTool::OnMouseMove(Control* /*control*/, const Vector2& position)
{
    UpdateHover(position);
}

// Mac 0x159230, Win 0x666e62
void HingeAlignTool::OnEvent18()
{
    if (GetSceneControl()->GetStatus().m_mouseInside) {
        BrickConnectSceneControl* control = GetSceneControl();
        UpdateHover(control->GetManager()->GetMousePosition(GetSceneControl()));
    }
}

// Mac 0x159290, Win 0x666ea1
void HingeAlignTool::OnEvent19()
{
    m_first.Clear();
    m_second.Clear();
    m_hover.Clear();
    SetCursor(0x71);
    if (GetSceneControl()->GetStatus().m_mouseInside) {
        BrickConnectSceneControl* control = GetSceneControl();
        UpdateHover(control->GetManager()->GetMousePosition(GetSceneControl()));
    }
}

// Mac 0x159310 (thunk 0x1596d0), Win 0x666efb
void HingeAlignTool::Update()
{
    UInt64 now = (UInt64)GetMilliseconds();
    UInt64 elapsed = now - m_lastTime + m_accumulated;
    m_accumulated = elapsed % m_stepInterval;   // NOTE: no guard against 0 here (both builds)
    m_lastTime = now;
    UInt64 limit = now + m_maxTime;
    UInt32 steps = (UInt32)(elapsed / (m_stepInterval > 1 ? m_stepInterval : 1));
    for (; steps != 0; --steps) {
        UInt32 state = GetSceneControl()->GetScene()->PhysicsCall5(0x3f800000);   // 1.0f
        if (state == 1) {
            UpdateObserver::Detach();
            BrickConnectSceneControl* control = GetSceneControl();
            control->GetManager()->SetModalControl(false, control);
            FinishMove();
            return;
        }
        if ((state | 1) == 3) {
            FailMove();
            return;
        }
        if ((UInt64)GetMilliseconds() > m_deadline) {
            GetSceneControl()->GetScene()->PhysicsCall6(false);
            FailMove();
            return;
        }
        if ((UInt64)GetMilliseconds() > limit)
            return;
    }
}

// (inlined twice in Mac 0x159310)
void HingeAlignTool::FailMove()
{
    UpdateObserver::Detach();
    BrickConnectSceneControl* control = GetSceneControl();
    control->GetManager()->SetModalControl(false, control);
    m_failed = true;
    UpdateHover(GetSceneControl()->GetManager()->GetMousePosition(GetSceneControl()));
    m_transforms.Clear();
    Redraw(GetSceneControl());
}

// Mac 0x159560
void HingeAlignTool::FinishMove()
{
    m_first.Clear();
    m_second.Clear();
    SetCursor(0x71);
    UpdateHover(GetSceneControl()->GetManager()->GetMousePosition(GetSceneControl()));
    BasicArray<Pair<PrimitiveElement*, Matrix3D> > changed;
    GetSceneControl()->GetChangedTransforms(m_transforms, changed);
    m_transforms.Clear();
    if (!changed.IsEmpty()) {
        UndoManager* undo = GetSceneControl()->GetUndoManager();
        undo->AddAction(new UndoMoveSnapshot(GetSceneControl(), changed, false));
    }
    Redraw(GetSceneControl());
}

// Mac 0x15a4f0, Win 0x665400
void HingeAlignTool::GetFeatureTransform(const Scene::Feature& feature, Matrix3D& transform)
{
    transform.Multiply(feature.m_element->GetRigidElement()->GetLocalTransform(),
                       feature.m_element->GetLocalTransform());
    transform.Multiply(feature.m_field->GetTransform());
    transform.Multiply(feature.m_offset);
    if (feature.m_field->GetType() == ConnectionField::TYPE_PLANAR &&
        (static_cast<const ConnectionFieldPlanar*>(feature.m_field)->GetPoint(feature.m_index).m_flags &
         ConnectionPoint::FLAG_FIXED) != 0)
        transform.Translate(Rotate(Vector3(0.0f, 0.176f, 0.0f), transform));
}

// Mac 0x159920, Win 0x665745
void HingeAlignTool::DrawFeatures(const BasicArray<Scene::Feature>& features, SingleMesh* mesh,
                                  Render* render)
{
    if (features.IsEmpty())
        return;
    const Scene::Feature& feature = features[0];
    Matrix3D world;
    feature.m_element->GetRigidElement()->GetRigidElement()->GetTransform(world);
    Matrix3D local;
    GetFeatureTransform(feature, local);
    world.Multiply(local);
    render->SetWorldMatrix(world);
    render->EnableBlend(true);
    render->EnableLighting(false);
    render->EnableDepthWrite(false);
    render->SetCullMode(0);
    mesh->Bind(render, 0.0f, true);
    render->DrawVertexBuffer();
    render->EnableBlend(false);
    render->EnableLighting(true);
    render->EnableDepthWrite(true);
    render->SetCullMode(1);
}

// Mac 0x1596f0, Win 0x665f69
void HingeAlignTool::OnEvent06(Render* render)
{
    if (m_first.IsEmpty()) {
        render->SetOpacity(0.5f);
        DrawFeatures(m_hover, m_meshes[0], render);
    } else {
        render->SetOpacity(1.0f);
        DrawFeatures(m_first, m_meshes[0], render);
        if (m_second.IsEmpty()) {
            if (!SameFeatures(m_first, m_hover)) {
                render->SetOpacity(0.5f);
                DrawFeatures(m_hover, m_meshes[1], render);
            }
        } else if (!m_failed) {
            render->SetOpacity(1.0f);
            DrawFeatures(m_second, m_meshes[1], render);
        } else if (SameFeatures(m_second, m_hover)) {
            DrawFeatures(m_second, m_meshes[2], render);
        } else {
            render->SetOpacity(0.5f);
            DrawFeatures(m_second, m_meshes[2], render);
            DrawFeatures(m_hover, m_meshes[1], render);
        }
    }
    render->SetOpacity(1.0f);
}

// Mac 0x159a50
bool HingeAlignTool::FindFeaturePair(const BasicArray<Scene::Feature>& first,
                                     const BasicArray<Scene::Feature>& second, FeaturePair* pair)
{
    BasicMap<float, FeaturePair, Less<float> > ranked;
    for (UInt32 i = 0; i < first.GetSize(); ++i) {
        const Scene::Feature& a = first[i];
        for (UInt32 j = 0; j < second.GetSize(); ++j) {
            const Scene::Feature& b = second[j];
            RigidElement* bodyA = a.m_element->GetRigidElement()->GetRigidElement();
            RigidElement* bodyB = b.m_element->GetRigidElement()->GetRigidElement();
            if (bodyA == bodyB || bodyA->GetHingedElement() != bodyB->GetHingedElement())
                continue;
            if (!a.m_field->IsCompatible(a.m_index, *b.m_field, b.m_index))
                continue;
            UInt32 type = a.m_field->GetType();
            if (type >= 5)
                continue;
            if (type == ConnectionField::TYPE_AXLE) {
                float rank = ((a.m_field->GetKind() ^ b.m_field->GetKind()) & 1) == 0 ? 0.25f : 0.75f;
                ranked[rank] = FeaturePair(a, b);
                continue;
            }
            if (pair)
                *pair = FeaturePair(a, b);
            return true;
        }
    }
    if (ranked.IsEmpty())
        return false;
    if (pair) {
        BasicMap<float, FeaturePair, Less<float> >::Iterator last = ranked.End();
        --last;
        *pair = last->second;
    }
    return true;
}

// Mac 0x159fa0 (thunk 0x15a4d0), Win 0x66701d
void HingeAlignTool::OnMousePressed(Control* /*control*/, const Vector2& /*position*/, UInt32 button)
{
    if (button != 0 || UpdateObserver::GetSubject() != 0)
        return;
    if (m_hover.IsEmpty()) {
        if (m_first.IsEmpty())
            return;
        m_first.Clear();
        m_second.Clear();
        Redraw(GetSceneControl());
        SetCursor(0x71);
        return;
    }
    if (m_first.IsEmpty()) {
        m_first = m_hover;
        m_failed = false;
        Redraw(GetSceneControl());
        SetCursor(0x72);
        return;
    }
    if (!m_second.IsEmpty() && !m_failed)
        return;
    m_second = m_hover;
    FeaturePair pair;
    if (!FindFeaturePair(m_first, m_second, &pair)) {
        m_failed = true;
    } else {
        GetSceneControl()->GetTransforms(m_transforms);
        PhysicsSolver* solver = GetSceneControl()->GetScene()->MoveTogether(pair.first, pair.second);
        if (solver == 0) {
            m_failed = true;
            UpdateHover(GetSceneControl()->GetManager()->GetMousePosition(GetSceneControl()));
            m_transforms.Clear();
            Redraw(GetSceneControl());   // NOTE: invalidated twice (both builds)
        } else {
            solver->Release();
            BrickConnectSceneControl* control = GetSceneControl();
            control->GetManager()->SetModalControl(true, control);
            UpdateObserver::Attach(GetSceneControl()->GetManager());
            UInt64 now = (UInt64)GetMilliseconds();
            m_lastTime = now;
            m_accumulated = 0;
            m_deadline = now + 25000;
            m_failed = false;
        }
    }
    Redraw(GetSceneControl());
}

} // namespace LEGO
