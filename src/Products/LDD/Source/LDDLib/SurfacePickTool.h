// LEGO Digital Designer — restored source
// LEGO::SurfacePickTool — base of the tools that pick one surface (decoration face / material
//   slot) of a part: the surface under the mouse is highlighted with a pulsing overlay in its own
//   colour (PickerMaterial, animated while the control manager updates), a press / release on the
//   same pickable surface is a click (OnPickClicked). Bases of SurfaceColorPickerTool,
//   SurfacePaintBucketTool and DecorationTool.
// LEGO::SurfacePickTool::PickerMaterial — the highlight material: lit, pulled towards the viewer
//   (polygon offset -1), drawn with the colours computed from the surface colour; with shaders in
//   the scene it wraps a shader material.
//
// TU 188 "SurfacePickTool.cpp" (Mac 0x241cc0-0x243c4b; Win 0x67d812, 0x67e76a-0x67f122). The
// constructor is inline (in the subclasses'); the destructor and the empty virtuals are emitted
// in SurfaceColorPickerTool.cpp (TU 186: D1 0x240010, D0 0x240070, Win sdd 0x67d9d5).
// RTTI: SurfacePickTool : BrickConnectSceneControlTool, UpdateObserver (+0x2c). Mac vtable
//   0x73037c (37 entries: BCSCTool's 27, new 27-30, the ControlObserver overriders 31-34, the
//   UpdateObserver overrider 35, new 36), sizeof 0x68 (Object +0x60); Win primary 0x9db254 (27:
//   new 22-26), UpdateObserver-in 0x9db1c0, ControlObserver-in 0x9db1d4.
//   PickerMaterial : Material (Mac vtable 0x73114c, sizeof 0x6c: base colour +0x04, start time
//   +0x14, colours +0x1c, shader material +0x60, Object +0x64; Win 0x9db810).
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "Geometry.h"
#include "Color.h"
#include "Render.h"
#include "Material.h"
#include "Mesh.h"
#include "Updater.h"
#include "BrickConnectSceneControlTool.h"

namespace LEGO {

class Render;
class RenderQueue;
class BrickConnectSceneManager;
class PrimitiveElement;

class SurfacePickTool : public BrickConnectSceneControlTool, public UpdateObserver {
public:
    class PickerMaterial : public Material {
    public:
        // Colours: diffuse (0, 0, 0, 0.6), ambient (0.05, 0.05, 0.05, 0), specular and emissive
        // clear, shininess 85; `shaderMaterial` (may be null) is referenced. Mac 0x241cc0
        explicit PickerMaterial(Material* shaderMaterial);
        virtual ~PickerMaterial() {}                                        // Mac 0x2439d0/0x243a40, Win sdd 0x67ea9e

        virtual bool IsOpaque() const { return false; }                    // Mac 0x241e20, Win 0x67e76a
        virtual bool IsDecal() const { return false; }                     // Mac 0x241e30, Win 0x67e76a
        // Lighting, the colours, polygon offset (0, -1) on; the shader material's Begin.
        // Mac 0x241e40, Win 0x67eabd
        virtual void Begin(Render* render);
        // Mac 0x241eb0, Win 0x67eb0f
        virtual void End(Render* render);

        Color              m_color;       // +0x04 the surface colour (brightened when dark)
        Int64              m_startTime;   // +0x14 GetMilliseconds of the highlight start
        MaterialColors     m_colors;      // +0x1c diffuse = m_color * pulse (Update)
        RefPointer<Material> m_shaderMaterial; // +0x60
    };

    // (inline) `cursorFlag`: BrickConnectSceneControlTool's.
    explicit SurfacePickTool(bool cursorFlag = false);
    virtual ~SurfacePickTool();                                             // Mac 0x240010/0x240070, Win sdd 0x67d9d5

    // Active; the highlight material (a shader material of the scene manager inside when the
    // application uses shaders in the scene; false when it cannot be made). `render` is not used.
    // Mac 0x241ef0 (Win 0x67eb3b?)
    bool Create(Render* render, BrickConnectSceneManager* sceneManager);

    // ---- new virtuals (Mac slot / Win slot) ----
    virtual void OnPickPressed(bool clear);                                 // 27/22 Mac 0x2400e0 (empty)
    virtual void OnPickClicked(bool flag);                                  // 28/23 Mac 0x23ffd0 (empty)
    virtual void OnPickReleased();                                          // 29/24 Mac 0x2400f0 (empty)
    // Whether the tool may pick `surface` of `element` (true). Mac 0x240100, Win 0x67d812
    virtual bool IsPickable(PrimitiveElement* element, UInt32 surface);     // 30/25

    // ---- ControlObserver (Mac-only primary entries 31-34) ----
    // The highlight goes. Mac 0x2423b0 (thunk 0x242430), Win 0x67eddd
    virtual void OnMouseLeave(Control* control, const Vector2& position);
    // UpdateHover. Mac 0x2424b0 (thunk 0x2424d0), Win 0x67f05d
    virtual void OnMouseMove(Control* control, const Vector2& position);
    // Button 0: the pressed surface, OnPickPressed(false). Mac 0x2424f0 (thunk 0x2426f0),
    // Win 0x67ee09
    virtual void OnMousePressed(Control* control, const Vector2& position, UInt32 button);
    // Button 0: OnPickClicked on the pressed, pickable surface; OnPickReleased. Mac 0x242710
    // (thunk 0x242960), Win 0x67ee9f
    virtual void OnMouseReleased(Control* control, const Vector2& position, UInt32 button);

    // ---- UpdateObserver (entry 35) ----
    // The highlight pulses: diffuse = colour * ((cos(0.005 * ms) + 1) * 0.4 + 0.2); redraw.
    // Mac 0x242ae0 (thunk 0x242bc0), Win 0x67ec3f
    virtual void Update();

    // ---- new virtual ----
    // The highlight meshes of the hovered surface (rigid parts: the geometry vertex buffer of each
    // face that redirects to the surface, at the element transform; flexible parts: the skinned
    // flex surface), the material colour and start time; the control manager's updater is
    // observed while something is hovered. Mac 0x242cb0, Win 0x67f122
    virtual void UpdateHighlight();                                         // 36/26

    // The surface under `position` (PickElement, no point, world, m_materialSlot): when it
    // changed the hover follows (IsPickable) and UpdateHighlight. Mac 0x242110, Win 0x67ef73
    void UpdateHover(const Vector2& position);

protected:
    // Leaves the updater, then BrickConnectSceneControlTool's. Mac 0x243860 (thunks 0x2438b0 …)
    virtual void OnFinalRelease();
    // BCSCTool's; the hover follows the mouse when it is inside. Mac 0x2420c0, Win 0x67f027
    virtual void OnAttached(BrickConnectSceneControl* control);
    // No press (OnPickReleased while pressed), no hover; BCSCTool's. Mac 0x242340, Win 0x67e7ee
    virtual void OnDetached(BrickConnectSceneControl* control);
    using BrickConnectSceneControlTool::OnAttached;
    using BrickConnectSceneControlTool::OnDetached;
    using UpdateObserver::OnAttached;
    using UpdateObserver::OnDetached;
    // The highlight meshes into the overlay queue. Mac 0x242980, Win 0x67ecfd
    virtual void OnEvent05(const Matrix3D& transform, Render* render, RenderQueue* overlay,
                           RenderQueue* scene);
    // The hover follows the mouse when it is inside. Mac 0x2429f0, Win 0x67f06c
    virtual void OnEvent18();
    // The press is forgotten, then as OnEvent18. Mac 0x242a60, Win 0x67f0b6
    virtual void OnEvent19();

    typedef BasicArray<Pair<RefPointer<Mesh>, Matrix3D> > MeshArray;

    // ---- data (Mac offsets; UpdateObserver +0x2c, its subject +0x30) ----
    bool                       m_pressed;          // +0x34
    bool                       m_clickFlag;        // +0x35 (always false; OnPickClicked's argument)
    PrimitiveElement*          m_hover;            // +0x38
    UInt32                     m_hoverSurface;     // +0x3c (-1)
    MeshArray                  m_highlights;       // +0x40
    RefPointer<PickerMaterial> m_material;         // +0x50
    PrimitiveElement*          m_pressedElement;   // +0x54
    UInt32                     m_pressedSurface;   // +0x58 (-1)
    bool                       m_materialSlot;     // +0x5c PickElement's materialSlot
                                                   // Object +0x60
};

} // namespace LEGO
