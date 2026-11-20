// LEGO Digital Designer — restored source
// LEGO::MaterialItemControl — a control showing one material: the colour swatch of
//   "Graphics/Icon_general.png" (333, 590, 34, 34) tinted with the material's diffuse colour, a
//   tooltip "<id> - <localized Material<id>>", and a frame drawn over it while the mouse is over
//   it (hoverable items) or while it is selected. Items of the MaterialGridControl and the base of
//   the tool box swatch (ToolboxMaterialItemControl).
// LEGO::MaterialItemControlObserver — Observer<MaterialItemControl, …>: the material changed
//   (SurfacePaintBucketTool / SurfaceColorPickerTool).
//
// Home TU 189 "MaterialItemControl.cpp" (Mac 0x243c50-0x24570b; Win 0x691c24-0x692488).
// Layout (Mac; constructor C1 0x243d70, C2 0x243c60 with VTT): RenderControl +0x000..+0x1ec,
//   Notifier<MaterialItemControlObserver> +0x1ec (observers +0x1f0, iterators +0x200), members
//   below, Object +0x224 (sizeof 0x22c). Win: Notifier +0x208 (vbptr), observers +0x20c,
//   iterators +0x21c, m_material +0x22c, m_icon +0x230, m_database +0x234, m_frame +0x238,
//   m_hover +0x23c, m_selected +0x23d, m_hoverable +0x23e, Object +0x244.
// Virtual slots: Mac 0x73178c (46 entries) = RenderControl's 45 with OnFinalRelease 0x244c70,
//   OnMouseEnter (16) 0x244f80, OnMouseLeave (17) 0x244fc0, Draw (33) 0x245000, Initialize (38)
//   0x243ea0, new slot 45 SetMaterial 0x244160. Win 0x9dd0e4 (41 entries: RenderControl's 40 +
//   SetMaterial 0x691f93), Object-in 0x9dd0b0 {sdd 0x692488 …}.
// MaterialItemControlObserver: Mac vtable 0x73108c {D1 0x240e60, D0 0x240ea0, OnFinalRelease,
//   OnAttached, OnDetached, OnMaterialChanged 0x243c50 (empty)}; Win 0x9db69c.
// Names inferred except the RTTI class names.
#pragma once
#include "LEGO/Types.h"
#include "Notifier.h"
#include "RenderControl.h"

namespace LEGO {

class MaterialItemControl;
class BrickDatabase;

class MaterialItemControlObserver : public Observer<MaterialItemControl, MaterialItemControlObserver> {
public:
    MaterialItemControlObserver() {}
    virtual ~MaterialItemControlObserver() {}                               // Mac 0x240e60/0x240ea0
    // The control shows another material (SetMaterial).                    // 5 Mac 0x243c50, Win 2
    virtual void OnMaterialChanged(UInt32 /*material*/) {}
};

class MaterialItemControl : public RenderControl, public Notifier<MaterialItemControlObserver> {
public:
    // Material 0 (nothing drawn until SetMaterial), not hoverable. Mac 0x243d70 (C1), 0x243c60
    // (C2), Win inlined (MaterialGridControl Win 0x66442d)
    MaterialItemControl(ControlManager* manager, const BrickDatabase* database);
    virtual ~MaterialItemControl() {}                                       // Mac 0x2447e0/0x244a00, Win sdd 0x692488

    // ---- RenderControl / Control overrides ----
    // RenderControl::Initialize, the icon texture, the frame child control (all margins 0,
    // showing the frame image (195, 590, 34, 34)), mouse input, shown, enabled, active,
    // transparent. False when the texture cannot be loaded. Mac 0x243ea0, Win 0x691cb7
    virtual bool Initialize();                                              // 38/35
    // The frame image follows the mouse on hoverable items. Mac 0x244f80, Win 0x691c69
    virtual void OnMouseEnter(const Vector2& position);                     // 16/13
    // Mac 0x244fc0, Win 0x691c90
    virtual void OnMouseLeave(const Vector2& position);                     // 17/14
    // RenderControl::Draw, then the frame (287, 590, 34, 34) while selected or (241, 590, …)
    // while hovered over the whole control. Mac 0x245000, Win 0x691e2d
    virtual void Draw(ControlDrawArgs& args);                               // 33/30

    // ---- new virtual ----
    // Shows `material` (image rect (333, 590, 34, 34), colour = its diffuse colour, tooltip) when
    // the database knows it; the observers are told either way. Same material: nothing.
    // Mac 0x244160, Win 0x691f93
    virtual void SetMaterial(UInt32 material);                              // 45/40

    // ---- non-virtual ----
    UInt32 GetMaterial() const { return m_material; }                       // Mac 0x244150, Win 0x691c24
    // The frame of the selected item. Mac 0x2445c0, Win 0x691c2b
    void SetSelected(bool selected);
    bool IsSelected() const { return m_selected; }
    // The frame follows the mouse (the hover state is taken over when the mouse is already
    // inside). Mac 0x244610, Win 0x691c43
    void SetHoverable(bool hoverable);
    // (test / inspection accessors, not in the original)
    bool IsHoverable() const { return m_hoverable; }
    bool IsHover() const { return m_hover; }

protected:
    // Detaches the observers (last first), then RenderControl's. Mac 0x244c70
    virtual void OnFinalRelease();

    // ---- data (Mac / Win offsets) ----
    UInt32                    m_material;    // +0x210 / +0x22c 0
    RefPointer<Texture>       m_icon;        // +0x214 / +0x230 Graphics/Icon_general.png
    const BrickDatabase*      m_database;    // +0x218 / +0x234 (not referenced)
    RefPointer<RenderControl> m_frame;       // +0x21c / +0x238 child shown over the swatch
    bool                      m_hover;       // +0x220 / +0x23c the mouse is over a hoverable item
    bool                      m_selected;    // +0x221 / +0x23d
    bool                      m_hoverable;   // +0x222 / +0x23e
                                             // Object +0x224 / +0x244
};

} // namespace LEGO
