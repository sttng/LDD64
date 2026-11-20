// LEGO Digital Designer — restored source
// LEGO::SurfacePaintBucketTool — see SurfacePaintBucketTool.h.
// TU 187 "SurfacePaintBucketTool.cpp": Mac 0x240720-0x241cbb, Win 0x67e15e-0x67e74f. Not code:
// the thunks, the Observer<MaterialItemControl, …> / MaterialItemControlObserver instances
// (0x240e60-0x241130, 0x241850 …), the destructor body (0x2417a0), Mac 0x241c40 = <iostream> + a
// header static.
#include "SurfacePaintBucketTool.h"
#include "BrickConnectSceneControl.h"
#include "PrimitiveElement.h"
#include "Part.h"
#include "Scene.h"

namespace LEGO {

// Mac 0x240720, Win 0x67e4e1
SurfacePaintBucketTool::SurfacePaintBucketTool()
    : SurfacePickTool(true),
      m_materialItemControl(0),
      m_previewElement(0),
      m_previewSurface(0),
      m_previewMaterial(0xffffffff)
{
    m_materialSlot = true;
    SetCursor(0x6d);
}

// Mac 0x241190 / 0x2411f0, Win sdd 0x67e72d
SurfacePaintBucketTool::~SurfacePaintBucketTool()
{
}

// (inline) The preview's own material goes back on the part.
void SurfacePaintBucketTool::RestorePreview()
{
    if (m_previewElement != 0 && m_previewMaterial != 0xffffffff)
        m_previewElement->GetPart()->SetMaterial(m_previewSurface, m_previewMaterial);
}

// Mac 0x240880, Win 0x67e2fd
void SurfacePaintBucketTool::OnToolEvent24(const BasicArray<RefPointer<Part> >& parts)
{
    GetSceneControl()->SetPartsMaterial(m_materialItemControl->GetMaterial(), parts);
}

// Mac 0x2408c0, Win 0x67e15e
void SurfacePaintBucketTool::OnPickPressed(bool)
{
    if (m_pressedElement == 0)
        GetSceneControl()->ClearSelection();
}

// Mac 0x2408f0, Win 0x67e31f
void SurfacePaintBucketTool::OnPickClicked(bool flag)
{
    if (flag || GetSceneControl() == 0 || !m_materialItemControl)
        return;
    Scene* scene = GetSceneControl()->GetScene();
    scene->BeginUpdate();
    RestorePreview();
    GetSceneControl()->SetPartMaterial(m_pressedElement->GetPart(), m_materialItemControl->GetMaterial(),
                                       m_pressedSurface);
    m_previewElement = m_pressedElement;
    m_previewSurface = m_pressedSurface;
    m_previewMaterial = 0xffffffff;
    GetSceneControl()->GetScene()->EndUpdate();
}

// Mac 0x2409d0, Win 0x67e3cc
void SurfacePaintBucketTool::UpdateHighlight()
{
    if (m_previewElement == m_hover && m_previewSurface == m_hoverSurface)
        return;
    Scene* scene = GetSceneControl()->GetScene();
    scene->BeginUpdate();
    RestorePreview();
    m_previewElement = m_hover;
    m_previewSurface = m_hoverSurface;
    m_previewMaterial = 0xffffffff;
    if (m_hover != 0) {
        m_previewMaterial = m_hover->GetPart()->GetMaterials()[m_hoverSurface];
        if (m_materialItemControl->GetMaterial() == m_previewMaterial)
            m_previewMaterial = 0xffffffff;
        else if (m_previewMaterial != 0xffffffff)
            m_previewElement->GetPart()->SetMaterial(m_previewSurface, m_materialItemControl->GetMaterial());
    }
    GetSceneControl()->GetScene()->EndUpdate();
}

// Mac 0x240e20 (thunk 0x240e40), Win 0x67e173
void SurfacePaintBucketTool::OnMaterialChanged(UInt32 material)
{
    MaterialItemControlObserver::OnMaterialChanged(material);
}

// NOTE: the original has no such hook: the previewed material stays on the part while a save
// or export reads the scene; fixed: BrickConnectSceneControl::ClearToolPreviews (SaveToStorage,
// ExportBOMCommand, GroupToLibraryCommand) calls this first.
void SurfacePaintBucketTool::OnClearPreview()
{
    if (m_previewElement != 0 && m_previewMaterial != 0xffffffff && GetSceneControl() != 0) {
        GetSceneControl()->GetScene()->BeginUpdate();
        RestorePreview();
        GetSceneControl()->GetScene()->EndUpdate();
    }
    m_previewElement = 0;
    m_previewSurface = 0;
    m_previewMaterial = 0xffffffff;
}

// Mac 0x241260 (thunks 0x241390, 0x2414c0, 0x2415f0)
void SurfacePaintBucketTool::OnFinalRelease()
{
    MaterialItemControlObserver::OnFinalRelease();
    SurfacePickTool::OnFinalRelease();
}

// Mac 0x240ac0, Win 0x67e74f
void SurfacePaintBucketTool::OnAttached(BrickConnectSceneControl* control)
{
    SurfacePickTool::OnAttached(control);
    // NOTE: guarded: without a material item control the original dereferences null here (the
    // inlined Attach has no check); EditMode always sets one first.
    if (m_materialItemControl)
        MaterialItemControlObserver::Attach(m_materialItemControl);
}

// Mac 0x240d60, Win 0x67e674
void SurfacePaintBucketTool::OnDetached(BrickConnectSceneControl* control)
{
    if (m_previewElement != 0 && m_previewMaterial != 0xffffffff) {
        GetSceneControl()->GetScene()->BeginUpdate();
        RestorePreview();
        GetSceneControl()->GetScene()->EndUpdate();
    }
    m_previewElement = 0;
    m_previewSurface = 0;
    m_previewMaterial = 0xffffffff;
    MaterialItemControlObserver::Detach();
    SurfacePickTool::OnDetached(control);
}

} // namespace LEGO
