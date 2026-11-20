// LEGO Digital Designer — restored source
// The ground grid of the building area: LEGO::Grid (abstract), LEGO::NoneGrid (no grid),
// LEGO::RenderGrid (lines + a textured plane under/over the model), LEGO::DynamicGrid (a
// RenderGrid that follows the scene box and the camera distance), and its two materials
// LEGO::GridMaterial, LEGO::GridTexturedSemitransparentMaterial.
//
// TU 117 "RenderGrid.cpp" (old guess VertexBuffer_CreateArgs.cpp):
// Mac 0x1d2bb0-0x1d572b, Win 0x654417-0x6559e9. The users are BrickConnectSceneControl
// (Mac 021b 0x32b90 creates a DynamicGrid and a NoneGrid, Initialize(render) each; the
// toggle 021f 0x4ef80 switches between them; 021e 0x49440 Draw(render, queue) per frame;
// Update(...) after camera / scene changes).
//
// Location: LDDLib, not DCL — the Win functions sit in the LDD block of LDD.exe (between LDDPart
// 0x653eef and PrimitiveElementBrick 0x655a36), far from the DCL render code (0x4e9000-0x5c3bd0).
//
// Vtables (slot order identical on both builds):
//   Grid        Mac 0x72157c {4 pure, D1 0x1d2f50, D0 0x1d2f90}, Win 0x9d6630 (+ Object 0x9d661c)
//   NoneGrid    Mac 0x721494 {0x1d55a0, 0x1d55b0, 0x1d55c0, 0x1d55d0, D1, D0}, Win 0x9d6658
//               {0x6c9dea, 0x6943f1, 0x6545fc, 0x654601} (ICF one-liners)
//   RenderGrid  Mac 0x7213cc {pure, 0x1d44d0, 0x1d46e0, pure, D1 0x1d4dd0, D0 0x1d4ea0},
//               Win 0x9d66b0 {purecall, 0x655850, 0x654ce1, purecall}
//   DynamicGrid Mac 0x72142c {0x1d5000, .., .., 0x1d51d0}, Win 0x9d66d8 {0x654596, .., .., 0x65477e}
//   GridMaterial Mac 0x72132c, Win 0x9d6604 {0x5fbd39, 0x67e76a, 0x654479, 0x6099d6}
//   GridTexturedSemitransparentMaterial Mac 0x72137c {0x109e10, 0x10a250, 0x1d2ce0, 0x1d2d10},
//               Win 0x9d6688 {0x5bd890, 0x5fbd39, 0x6544ca, 0x6544f3}
// Layouts: Mac {vptr, members, virtual base Object}; Win {vptr, vbptr, members (+4), Object}.
// Class names from RTTI; every method and member name is inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Geometry.h"
#include "Color.h"
#include "Render.h"
#include "Material.h"
#include "Materials.h"
#include "Mesh.h"
#include "VertexBuffer.h"

namespace LEGO {

// ===========================================================================================
// Grid — Mac {vptr, m_height +0x04, m_followBox +0x08, m_dirty +0x09, Object +0x0c} (0x14);
//        Win {vptr, vbptr, m_height +0x08, m_followBox +0x0c, m_dirty +0x0d, Object +0x10}.
// ===========================================================================================
class Grid : public virtual Object {
public:
    enum Type { kTypeNone = 0, kTypeDynamic = 2 };   // (1: no implementation in this build)

    // Win 0x654518 (thiscall(mostDerived)); inlined on Mac (NoneGrid 0x1d5550, RenderGrid 0x1d3020).
    Grid() : m_height(0.0f), m_followBox(true), m_dirty(true) {}
    virtual ~Grid() {}                                                // Mac 0x1d2f50/0x1d2f90, Win 0x654580 (sdd 0x654b31)

    virtual UInt32 GetType() const = 0;                               // 0
    // Queues the grid meshes into render queue `queue`.
    virtual void Draw(Render* render, UInt32 queue) = 0;              // 1
    // Creates the textures/materials; false when that failed.
    virtual bool Initialize(Render* render) = 0;                      // 2
    // `box` is the scene's bounding box (`extraBox` is merged into it, in place); `viewVector`
    // = camera position - target; `cameraPosition` and `flag` are not read by any grid.
    virtual void Update(const Vector3& cameraPosition, const Vector3& viewVector,
                        BoundingBox& box, const BoundingBox* extraBox, bool flag) = 0; // 3 (Win ret 0x14)

    // Mac 0x1d2f40, Win 0x65457c (BrickConnectSceneControl reads it for the snapping plane).
    Real32 GetHeight() const { return m_height; }

protected:
    Real32 m_height;      // Mac +0x04, Win +0x08: y of the grid plane
    bool   m_followBox;   // Mac +0x08, Win +0x0c: DynamicGrid puts the plane at the box's min y
    bool   m_dirty;       // Mac +0x09, Win +0x0d: meshes must be rebuilt
};

// No grid at all. Mac ctor 0x1d5550, Win 0x65459a; dtor Mac 0x1d55e0/0x1d5620, Win 0x654604.
class NoneGrid : public Grid {
public:
    NoneGrid() {}
    virtual UInt32 GetType() const { return kTypeNone; }              // Mac 0x1d55a0, Win 0x6c9dea (ICF)
    virtual void Draw(Render*, UInt32) {}                             // Mac 0x1d55b0, Win 0x6943f1 (ICF)
    virtual bool Initialize(Render*) { return true; }                 // Mac 0x1d55c0, Win 0x6545fc (ICF)
    virtual void Update(const Vector3&, const Vector3&, BoundingBox&, const BoundingBox*, bool) {} // Mac 0x1d55d0, Win 0x654601
};

// Unlit lines (lighting off between Begin and End). Mac sizeof 0xc, Win 0x10.
// Ctor Win 0x654417 (inlined on Mac), dtor Mac 0x1d2c10/0x1d2c50, Win 0x654487 (sdd 0x654a8d).
class GridMaterial : public Material {
public:
    GridMaterial() {}
    virtual bool IsOpaque() const { return true; }                    // Mac 0x1d2bb0, Win 0x5fbd39 (ICF)
    virtual bool IsDecal() const { return false; }                    // Mac 0x1d2bc0, Win 0x67e76a (ICF)
    virtual void Begin(Render* render);                               // Mac 0x1d2bd0, Win 0x654479
    virtual void End(Render* render);                                 // Mac 0x1d2bf0, Win 0x6099d6 (ICF)
};

// The grid texture seen from below: a decal at a quarter of the opacity.
// Mac sizeof 0x54, Win 0x58. Ctor Win 0x654ab6 (inlined on Mac), dtor Mac 0x1d2d40/0x1d2e30,
// Win 0x654637 (sdd 0x654fbe).
class GridTexturedSemitransparentMaterial : public DecalMaterial {
public:
    GridTexturedSemitransparentMaterial(const MaterialColors& colors, Texture* texture)
        : DecalMaterial(colors, texture) {}
    virtual void Begin(Render* render);                               // Mac 0x1d2ce0, Win 0x6544ca
    virtual void End(Render* render);                                 // Mac 0x1d2d10, Win 0x6544f3
};

// ===========================================================================================
// RenderGrid — Mac sizeof 0x8c (Object +0x84), Win 0x90 (Object +0x88).
// ===========================================================================================
class RenderGrid : public Grid {
public:
    RenderGrid();                                                     // Mac 0x1d3020 (C2, VTT), Win 0x654b56
    virtual ~RenderGrid() {}                                          // Mac 0x1d4c90 (D2)/0x1d4dd0/0x1d4ea0, Win 0x6546c1 (sdd 0x654fdd)

    // Rebuilds the meshes when dirty, when the level of detail changed or when the camera
    // crossed the grid plane, then queues lines and plane with AddTransparent (layer 1000 seen
    // from above, -1000 from below).
    virtual void Draw(Render* render, UInt32 queue);                  // 1 Mac 0x1d44d0, Win 0x655850
    // Loads Graphics/General/Grid.png (system directory 9) and makes the two plane materials.
    virtual bool Initialize(Render* render);                          // 2 Mac 0x1d46e0, Win 0x654ce1

protected:
    // Lines every `step` cells over `rect` (grid units; world = 1.6 * grid), colour `color`.
    VertexBuffer* CreateLineVertexBuffer(Render* render, const Color& color, const Rect& rect,
                                         UInt32 step);                // Mac 0x1d3310, Win 0x655021
    void          CreateLineMesh(Render* render, const Color& color, Material* material,
                                 const Rect& rect, UInt32 step);      // Mac 0x1d3240, Win 0x65578a
    // A textured quad over `rect` (texture repeated twice per grid unit), wound towards the
    // camera (above / below the plane at `cameraHeight`). `step` is not read.
    VertexBuffer* CreatePlaneVertexBuffer(Render* render, Real32 cameraHeight, const Rect& rect,
                                          UInt32 step);               // Mac 0x1d3be0, Win 0x65534d
    void          CreatePlaneMesh(Render* render, Real32 cameraHeight, Material* material,
                                  const Rect& rect, UInt32 step);     // Mac 0x1d3b20, Win 0x6557eb

    RefPointer<Mesh>     m_lineMesh;              // Mac +0x0c, Win +0x10
    RefPointer<Mesh>     m_planeMesh;             // Mac +0x10, Win +0x14
    RefPointer<Material> m_lineMaterial;          // Mac +0x14, Win +0x18 GridMaterial
    RefPointer<Material> m_planeMaterial;         // Mac +0x18, Win +0x1c TexturedMaterial (from above)
    RefPointer<Material> m_planeMaterialBelow;    // Mac +0x1c, Win +0x20 GridTexturedSemitransparentMaterial
    Color                m_lineColor;             // Mac +0x20, Win +0x24 (0.4, 0.4, 0.4, 1)
    Color                m_color2;                // Mac +0x30, Win +0x34 (0.6, 0.6, 0.6, 1; not read)
    Real32               m_cellSize;              // Mac +0x40, Win +0x44 0.4 (line spacing = 4 cells)
    Rect                 m_baseRect;              // Mac +0x44, Win +0x48 (-8, -8, 8, 8) grid units
    Rect                 m_rect;                  // Mac +0x54, Win +0x58 the area to draw
    Rect                 m_builtRect;             // Mac +0x64, Win +0x68 m_rect of the last change
    bool                 m_dynamic;               // Mac +0x74, Win +0x78 CreateArgs::dynamic (true)
    UInt32               m_step;                  // Mac +0x78, Win +0x7c level of detail: 1, 2 or 4
    bool                 m_stepChanged;           // Mac +0x7c, Win +0x80
    Real32               m_cameraHeight;          // Mac +0x80, Win +0x84 at the last rebuild
                                                  //   (NOTE: not initialised by the constructor;
                                                  //   only read after m_dirty forced a rebuild)
};

// Mac ctor 0x1d4f90, Win 0x654f1a; dtor Mac 0x1d5010/0x1d50e0, Win 0x654742 (sdd 0x654fff).
class DynamicGrid : public RenderGrid {
public:
    DynamicGrid() {}
    virtual UInt32 GetType() const { return kTypeDynamic; }           // 0 Mac 0x1d5000, Win 0x654596
    // Level of detail from |viewVector|^2 / 30000 (1 below, else the next power of two, at most
    // 4), plane height from the box, and the drawn area: base rect united with the box (x/z),
    // widened to whole steps and clamped to +-300000 world units / 1.6.
    virtual void Update(const Vector3& cameraPosition, const Vector3& viewVector,
                        BoundingBox& box, const BoundingBox* extraBox, bool flag); // 3 Mac 0x1d51d0, Win 0x65477e

protected:
    void UpdateStep(const Vector3& cameraPosition, const Vector3& viewVector); // inlined on Mac, Win 0x65466a
};

} // namespace LEGO
