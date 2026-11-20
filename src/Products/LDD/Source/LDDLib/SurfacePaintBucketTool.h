// LEGO Digital Designer — restored source
// LEGO::SurfacePaintBucketTool — "paintBucketTool" (the free-build material tool of LDD
//   Extended): the hovered surface previews the material of the edit mode's material item control
//   (set on the part without undo, restored when the hover leaves); a click paints it
//   (BrickConnectSceneControl::SetPartMaterial, with undo); a group tree click paints the parts.
//
// TU 187 "SurfacePaintBucketTool.cpp" (Mac 0x240720-0x241cbb; Win 0x67e15e-0x67e74f).
// RTTI: SurfacePaintBucketTool : SurfacePickTool, MaterialItemControlObserver (+0x60). Mac vtable
//   0x7308fc (38 entries: SurfacePickTool's 37 + the MaterialItemControlObserver overrider 37),
//   sizeof 0x80 (Object +0x78); Win 0x9db774, MaterialItemControlObserver-in 0x9db6d4, sizeof
//   0x94. Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "SurfacePickTool.h"
#include "MaterialItemControl.h"

namespace LEGO {

class SurfacePaintBucketTool : public SurfacePickTool, public MaterialItemControlObserver {
public:
    // SurfacePickTool(true) picking material slots; cursor 0x6d. Mac 0x240720, Win 0x67e4e1
    SurfacePaintBucketTool();
    virtual ~SurfacePaintBucketTool();                                      // Mac 0x241190/0x2411f0, Win 0x67e5c5?/0x67e72d

    // NOTE: an inline RefPointer assignment in the original (Mac +0x68; EditMode Mac 0x1866ce).
    void SetMaterialItemControl(MaterialItemControl* control) { m_materialItemControl = control; }
    MaterialItemControl* GetMaterialItemControl() const { return m_materialItemControl; }

    // The parts get the control's material (SetPartsMaterial). Mac 0x240880, Win 0x67e2fd
    virtual void OnToolEvent24(const BasicArray<RefPointer<Part> >& parts); // 24/21
    // A press on empty space clears the selection. Mac 0x2408c0, Win 0x67e15e
    virtual void OnPickPressed(bool clear);                                 // 27/22
    // The preview is undone, the clicked surface painted with undo (SetPartMaterial) and becomes
    // the preview without an old material. Mac 0x2408f0, Win 0x67e31f
    virtual void OnPickClicked(bool flag);                                  // 28/23
    // Replaces the highlight: the old preview is undone, the hovered surface previews the
    // control's material (unless it already has it). Mac 0x2409d0, Win 0x67e3cc
    virtual void UpdateHighlight();                                         // 36/26
    // MaterialItemControlObserver's (empty). Mac 0x240e20 (entry 37, thunk 0x240e40), Win 0x67e173
    virtual void OnMaterialChanged(UInt32 material);
    // fixed (not in the original): a save or export follows: the hover preview is undone
    // (as in OnDetached); the next hover change previews again.
    virtual void OnClearPreview();

protected:
    // Leaves the material item control, then SurfacePickTool's. Mac 0x241260 (thunks …)
    virtual void OnFinalRelease();
    // SurfacePickTool's; observes the material item control. Mac 0x240ac0, Win 0x67e74f
    virtual void OnAttached(BrickConnectSceneControl* control);
    // The preview is undone; leaves the material item control; SurfacePickTool's. Mac 0x240d60,
    // Win 0x67e674
    virtual void OnDetached(BrickConnectSceneControl* control);
    using SurfacePickTool::OnAttached;
    using SurfacePickTool::OnDetached;
    using MaterialItemControlObserver::OnAttached;
    using MaterialItemControlObserver::OnDetached;
    // Puts the preview's old material back (when there is one). (inline in the original)
    void RestorePreview();

    // ---- data (Mac offsets; MaterialItemControlObserver +0x60, its subject +0x64) ----
    RefPointer<MaterialItemControl> m_materialItemControl;   // +0x68
    PrimitiveElement*               m_previewElement;        // +0x6c
    UInt32                          m_previewSurface;        // +0x70
    UInt32                          m_previewMaterial;       // +0x74 the surface's own material (-1: none)
                                                             // Object +0x78
};

} // namespace LEGO
