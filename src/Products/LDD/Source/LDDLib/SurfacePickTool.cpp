// LEGO Digital Designer — restored source
// LEGO::SurfacePickTool, LEGO::SurfacePickTool::PickerMaterial — see SurfacePickTool.h.
// TU 188 "SurfacePickTool.cpp": Mac 0x241cc0-0x243c4b, Win 0x67d812, 0x67e76a-0x67f122. Not code:
// the thunks, the BasicArray<Pair<RefPointer<Mesh>, Matrix3D> > push_back (0x243670), Mac
// 0x243bd0 = <iostream> + a header static. The destructor and the empty virtuals are emitted in
// SurfaceColorPickerTool.cpp in the original (TU 186: 0x240010, 0x2400e0 …); defined here.
#include "SurfacePickTool.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneManager.h"
#include "ControlManager.h"
#include "PrimitiveElement.h"
#include "Primitive.h"
#include "Part.h"
#include "LDDPart.h"
#include "BrickDatabase.h"
#include "MaterialManual.h"
#include "LDDApplication.h"
#include "RenderQueue.h"
#include "SystemTime.h"
#include <math.h>

namespace LEGO {

namespace {

// The control is redrawn unless its manager is drawing (inline everywhere).
void RedrawControl(Control* control)
{
    if (!control->GetManager()->IsUpdateLocked())
        control->Invalidate(false);
}

const UInt32 kNoSurface = 0xffffffff;

} // namespace

// ===========================================================================================
// PickerMaterial
// ===========================================================================================

// Mac 0x241cc0 (Win inlined)
SurfacePickTool::PickerMaterial::PickerMaterial(Material* shaderMaterial)
    : m_startTime(0),   // NOTE: not initialised by the original (set with every highlight)
      m_shaderMaterial(shaderMaterial)
{
    m_colors.diffuse = Color(0.0f, 0.0f, 0.0f, 0.6f);
    m_colors.ambient = Color(0.05f, 0.05f, 0.05f, 0.0f);
    m_colors.specular = Color::Clear();
    m_colors.emissive = Color::Clear();
    m_colors.shininess = 85.0f;
}

// Mac 0x241e40, Win 0x67eabd
void SurfacePickTool::PickerMaterial::Begin(Render* render)
{
    render->EnableLighting(true);
    render->SetMaterialColors(m_colors);
    render->SetPolygonOffset(0.0f, -1.0f);
    render->EnablePolygonOffset(true);
    if (m_shaderMaterial)
        m_shaderMaterial->Begin(render);
}

// Mac 0x241eb0, Win 0x67eb0f
void SurfacePickTool::PickerMaterial::End(Render* render)
{
    render->EnablePolygonOffset(false);
    render->EnableLighting(false);
    if (m_shaderMaterial)
        m_shaderMaterial->End(render);
}

// ===========================================================================================
// SurfacePickTool
// ===========================================================================================

// (inline: SurfaceColorPickerTool Mac 0x23fe40, SurfacePaintBucketTool 0x240720, DecorationTool
// 0x2476b0)
SurfacePickTool::SurfacePickTool(bool cursorFlag)
    : BrickConnectSceneControlTool(cursorFlag),
      m_pressed(false),
      m_clickFlag(false),
      m_hover(0),
      m_hoverSurface(kNoSurface),
      m_material(0),
      m_pressedElement(0),
      m_pressedSurface(kNoSurface),
      m_materialSlot(false)
{
}

// Mac 0x240010 / 0x240070 (TU 186), Win sdd 0x67d9d5
SurfacePickTool::~SurfacePickTool()
{
}

// Mac 0x2400e0 / 0x23ffd0 / 0x2400f0 (TU 186; empty), Win 0x65a12a / 0x5c8021 (ICF)
void SurfacePickTool::OnPickPressed(bool) {}
void SurfacePickTool::OnPickClicked(bool) {}
void SurfacePickTool::OnPickReleased() {}

// Mac 0x240100 (TU 186), Win 0x67d812
bool SurfacePickTool::IsPickable(PrimitiveElement*, UInt32)
{
    return true;
}

// Mac 0x241ef0
bool SurfacePickTool::Create(Render*, BrickConnectSceneManager* sceneManager)
{
    SetActive(true);
    RefPointer<PickerMaterial> material;
    if (LDDApplication::GetInstance()->UseShadersInScene()) {
        MaterialColor colors;
        colors.diffuse = Color(0.0f, 0.0f, 0.0f, 0.6f);
        colors.ambient = Color(0.05f, 0.05f, 0.05f, 0.0f);
        colors.specular = Color::Clear();
        colors.emission = Color::Clear();
        colors.shininess = 85.0f;
        Material* shaderMaterial = sceneManager->CreateShaderMaterial(0, colors, 4);
        if (!shaderMaterial)
            return false;
        material = new PickerMaterial(shaderMaterial);
    } else {
        material = new PickerMaterial(0);
    }
    m_material = material;
    return true;
}

// Mac 0x2420c0, Win 0x67f027
void SurfacePickTool::OnAttached(BrickConnectSceneControl* control)
{
    BrickConnectSceneControlTool::OnAttached(control);
    if (control->GetStatus().m_mouseInside)
        UpdateHover(control->GetManager()->GetMousePosition(control));
}

// Mac 0x242110, Win 0x67ef73
void SurfacePickTool::UpdateHover(const Vector2& position)
{
    UInt32 surface = 0;
    BasicArray<RefPointer<Part> > filterParts;
    BasicArray<RefPointer<Part> > extraParts;
    PrimitiveElement* element = GetSceneControl()->PickElement(position, 0, false, &surface,
                                                               m_materialSlot, filterParts, extraParts);
    if (element == m_hover && surface == m_hoverSurface)
        return;
    // NOTE: an element that is not pickable leaves m_hover null, so every move over it repeats
    // this (both builds).
    PrimitiveElement* hover = 0;
    if (element != 0 && IsPickable(element, surface))
        hover = element;
    m_hover = hover;
    m_hoverSurface = surface;
    UpdateHighlight();
}

// Mac 0x242340, Win 0x67e7ee
void SurfacePickTool::OnDetached(BrickConnectSceneControl* control)
{
    m_pressedElement = 0;
    m_pressedSurface = kNoSurface;
    if (m_pressed) {
        OnPickReleased();
        m_pressed = false;
    }
    if (m_hover != 0) {
        // NOTE: the highlight meshes and the updater stay (both builds).
        RedrawControl(control);
        m_hover = 0;
        m_hoverSurface = kNoSurface;
    }
    BrickConnectSceneControlTool::OnDetached(control);
}

// Mac 0x2423b0 (thunk 0x242430), Win 0x67eddd
void SurfacePickTool::OnMouseLeave(Control*, const Vector2&)
{
    if (m_hover == 0)
        return;
    m_highlights.Clear();
    RedrawControl(GetSceneControl());
    m_hover = 0;
    m_hoverSurface = kNoSurface;
}

// Mac 0x2424b0 (thunk 0x2424d0), Win 0x67f05d
void SurfacePickTool::OnMouseMove(Control*, const Vector2& position)
{
    UpdateHover(position);
}

// Mac 0x2424f0 (thunk 0x2426f0), Win 0x67ee09
void SurfacePickTool::OnMousePressed(Control*, const Vector2& position, UInt32 button)
{
    if (button != 0)
        return;
    BasicArray<RefPointer<Part> > filterParts;
    BasicArray<RefPointer<Part> > extraParts;
    m_pressedElement = GetSceneControl()->PickElement(position, 0, false, &m_pressedSurface,
                                                      m_materialSlot, filterParts, extraParts);
    OnPickPressed(false);
    m_pressed = true;
    m_clickFlag = false;
}

// Mac 0x242710 (thunk 0x242960), Win 0x67ee9f
void SurfacePickTool::OnMouseReleased(Control*, const Vector2& position, UInt32 button)
{
    if (button != 0)
        return;
    if (m_pressedElement != 0) {
        UInt32 surface = 0;
        BasicArray<RefPointer<Part> > filterParts;
        BasicArray<RefPointer<Part> > extraParts;
        PrimitiveElement* element = GetSceneControl()->PickElement(position, 0, false, &surface,
                                                                   m_materialSlot, filterParts, extraParts);
        if (element == m_pressedElement && surface == m_pressedSurface && IsPickable(element, surface))
            OnPickClicked(m_clickFlag);
        m_pressedElement = 0;
        m_pressedSurface = kNoSurface;
    }
    OnPickReleased();
    m_pressed = false;
}

// Mac 0x242980, Win 0x67ecfd
void SurfacePickTool::OnEvent05(const Matrix3D& transform, Render*, RenderQueue* overlay, RenderQueue*)
{
    if (m_hover == 0)
        return;
    for (UInt32 i = 0; i < m_highlights.GetSize(); ++i) {
        const Matrix3D meshTransform(transform, m_highlights[i].second);
        overlay->Add(m_highlights[i].first, meshTransform, 0.0f, 1, 1.0f);
    }
}

// Mac 0x2429f0, Win 0x67f06c
void SurfacePickTool::OnEvent18()
{
    BrickConnectSceneControl* control = GetSceneControl();
    if (control != 0 && control->GetStatus().m_mouseInside)
        UpdateHover(control->GetManager()->GetMousePosition(control));
}

// Mac 0x242a60, Win 0x67f0b6
void SurfacePickTool::OnEvent19()
{
    if (m_pressedElement != 0) {
        m_pressedElement = 0;
        m_pressedSurface = kNoSurface;
    }
    BrickConnectSceneControl* control = GetSceneControl();
    if (control != 0 && control->GetStatus().m_mouseInside)
        UpdateHover(control->GetManager()->GetMousePosition(control));
}

// Mac 0x242ae0 (thunk 0x242bc0), Win 0x67ec3f
void SurfacePickTool::Update()
{
    if (GetSceneControl() == 0)
        return;
    PickerMaterial* material = m_material;
    // (the elapsed milliseconds as an unsigned 64-bit value)
    const UInt64 elapsed = (UInt64)(GetMilliseconds() - material->m_startTime);
    const Real32 pulse = ((Real32)cosf((Real32)elapsed * 0.005f) + 1.0f) * 0.4f + 0.2f;
    material->m_colors.diffuse.r = material->m_color.r * pulse;
    material->m_colors.diffuse.g = material->m_color.g * pulse;
    material->m_colors.diffuse.b = pulse * material->m_color.b;
    RedrawControl(GetSceneControl());
}

// Mac 0x242cb0, Win 0x67f122
void SurfacePickTool::UpdateHighlight()
{
    m_highlights.Clear();
    RedrawControl(GetSceneControl());
    if (m_hover == 0) {
        UpdateObserver::Detach();
        return;
    }
    UpdateObserver::Attach(GetSceneControl()->GetManager());

    // The surface colour; dark colours (|rgb| <= 0.5) are brightened 2.5 times, opaque.
    MaterialColor colors;
    colors.shininess = 0.0f;
    const BrickDatabase* database = GetSceneControl()->GetSceneManager()->GetDatabase();
    database->GetMaterialColor(m_hover->GetPart()->GetMaterial(m_hoverSurface), colors);
    PickerMaterial* material = m_material;
    const Color& diffuse = colors.diffuse;
    const Real32 length = sqrtf(diffuse.b * diffuse.b + (diffuse.g * diffuse.g + diffuse.r * diffuse.r));
    if (length > 0.5f)
        material->m_color = diffuse;
    else
        material->m_color = Color(diffuse.r * 2.5f, diffuse.g * 2.5f, diffuse.b * 2.5f, 1.0f);
    material->m_startTime = GetMilliseconds();

    const Primitive* primitive = m_hover->GetPrimitive();
    const BasicArray<UInt32>& redirect = primitive->GetSubMaterialRedirect();
    LDDPart* part = static_cast<LDDPart*>(m_hover->GetPart());
    if (!redirect.IsEmpty()) {
        // Every geometry face that draws with the hovered material slot.
        for (UInt32 face = 0; face < redirect.GetSize(); ++face) {
            if (redirect[face] != m_hoverSurface)
                continue;
            part = static_cast<LDDPart*>(m_hover->GetPart());
            if (part->GetFlexSurfaces().IsEmpty()) {
                VertexBuffer* buffer = m_sceneControl->GetSceneManager()->GetVertexBuffer(primitive, face);
                if (buffer == 0)
                    continue;
                Matrix3D transform;
                m_hover->GetTransform(transform);
                RefPointer<Mesh> mesh = new SingleMesh(m_material, buffer);
                m_highlights.PushBack(Pair<RefPointer<Mesh>, Matrix3D>(mesh, transform));
            } else {
                RefPointer<Mesh> mesh = new SingleMesh(m_material, part->GetFlexSurfaces()[face].m_vertexBuffer);
                Matrix3D identity;
                m_highlights.PushBack(Pair<RefPointer<Mesh>, Matrix3D>(mesh, identity));
            }
        }
        return;
    }
    if (part->GetFlexSurfaces().IsEmpty()) {
        VertexBuffer* buffer = m_sceneControl->GetSceneManager()->GetVertexBuffer(primitive, m_hoverSurface);
        if (buffer == 0)
            return;
        Matrix3D transform;
        m_hover->GetTransform(transform);
        RefPointer<Mesh> mesh = new SingleMesh(m_material, buffer);
        m_highlights.PushBack(Pair<RefPointer<Mesh>, Matrix3D>(mesh, transform));
    } else {
        RefPointer<Mesh> mesh =
            new SingleMesh(m_material, part->GetFlexSurfaces()[m_hoverSurface].m_vertexBuffer);
        Matrix3D identity;
        m_highlights.PushBack(Pair<RefPointer<Mesh>, Matrix3D>(mesh, identity));
    }
}

// Mac 0x243860 (thunks 0x2438b0, 0x243910, 0x243970)
void SurfacePickTool::OnFinalRelease()
{
    UpdateObserver::OnFinalRelease();
    BrickConnectSceneControlTool::OnFinalRelease();
}

} // namespace LEGO
