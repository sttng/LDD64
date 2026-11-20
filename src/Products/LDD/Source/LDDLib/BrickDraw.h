// LEGO Digital Designer — restored source
// LEGO::BrickDraw (name from the RTTI of BasicArray<BrickDraw::SubPart>) — a list of parts to
// draw outside the scene graph (the bricks a tool moves or places, the palette / clipboard /
// storage previews): per part the meshes the BrickConnectSceneManager makes for its primitive,
// materials and decorations, and the part's transform; plus the union of the parts' boxes.
// The static BuildMeshes / BuildFlexMeshes make the meshes of one primitive (also used by
// BrickConnectSceneControl 0x48940, FallingBricks 0x1bbe70 and 0x34f0e0).
//
// Restored in BrickDraw.cpp (Mac TU 173, 0x231290-0x232c9b, file name inferred from the class;
// Win 0x65736c-0x657ef6, found through the BCSM lookups 0x6112da / 0x61140d / 0x610c93 /
// 0x6104c1 and the SubPart array: CreateMesh / CreateDecorationMesh callers).
// Directory: LDDLib (depends on BrickConnectSceneManager, PrimitiveElementBrick).
//
// No vtable; the constructor and destructor are inline in the users (ClickStickTool, MoveTool,
// BricksControl, StorageSceneControl, ModelControl 0x208460). Layout (both builds, 0x2c bytes):
//   +0x00 BrickConnectSceneManager* m_sceneManager
//   +0x04 BasicArray<SubPart>       m_subParts (Mac vtable 0x769918 {0x147460, 0x147510},
//                                   Win 0x9d0488 {sdd 0x62cff6})
//   +0x14 BoundingBox               m_box
// Method and member names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "Geometry.h"
#include "Mesh.h"
#include "Material.h"
#include "StorageScene2.h"

namespace LEGO {

class BrickConnectSceneManager;
class Primitive;
class Part;
class RenderQueue;
class PrimitiveElementBrick;

class BrickDraw {
public:
    // One drawn part. sizeof 0x50 (both builds); copy constructor Mac 0x232b10 (Win inlined).
    struct SubPart {
        BasicArray<RefPointer<Mesh> > m_meshes;     // +0x00
        Matrix3D                      m_transform;  // +0x10 (identity when constructed)
    };

    // (inline in the users; the box starts reset, as Clear leaves it)
    explicit BrickDraw(BrickConnectSceneManager* sceneManager)
        : m_sceneManager(sceneManager), m_box(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f)
    {
        m_box.Reset();
    }

    // Removes every part and resets the box (inlined in the users, e.g. BricksControl Mac
    // 0x16a189).
    void Clear()
    {
        m_subParts.Clear();
        m_box.Reset();
    }

    // ---- building -----------------------------------------------------------------------------

    // A rigid part of design `designID` at `transform` (BuildMeshes); its box (the primitive's,
    // transformed) is added to m_box. Nothing is added when the primitive is unknown or a mesh
    // could not be made. Mac 0x231e40, Win 0x657b5c
    bool AddPart(UInt32 designID, const Matrix3D& transform, const BasicArray<UInt32>& materials,
                 const BasicArray<UInt32>& decorations, UInt32 materialSet);
    // A flexible part (BuildFlexMeshes, bone transforms in world space; identity part
    // transform; the box is added untransformed). Mac 0x2322e0, Win 0x657c27
    bool AddFlexPart(UInt32 designID, const BasicArray<Matrix3D>& boneTransforms,
                     const BasicArray<UInt32>& materials, const BasicArray<UInt32>& decorations,
                     UInt32 materialSet);
    // A scene part: one element -> AddPart at the element's world transform, more -> AddFlexPart
    // with the elements' transforms by bone index. Mac 0x232590, Win 0x657cca
    bool AddPart(const Part* part, UInt32 materialSet);
    // A stored part (one bone -> AddPart, more -> AddFlexPart; NOTE: a part without bones
    // reads bone 0 in both builds). Mac 0x232700, Win 0x657db7
    bool AddPart(const StorageScene2::Part& part, UInt32 materialSet);
    // Part `part` of `brick` (its own or the palette's bone transforms / materials /
    // decorations; rigid when the palette part has fewer than two bones). Mac 0x232830,
    // Win 0x657e76
    bool AddPart(const PrimitiveElementBrick& brick, UInt32 part, UInt32 materialSet);

    // Every mesh gets `material`. Mac 0x231db0, Win 0x65736c
    void SetMaterial(Material* material);

    // ---- drawing (RenderQueue::Add / AddWithDepthPrepass, layer 0) ---------------------------

    // Every mesh at its part transform, depth = squared distance of the translations of the
    // part transform and `camera`, opacity 1. Mac 0x2328d0, Win 0x6573a2
    void Draw(RenderQueue* queue, const Matrix3D& camera) const;
    // The same with the part transforms moved by `world` (world * part). Mac 0x232980,
    // Win 0x657401
    void Draw(RenderQueue* queue, const Matrix3D& camera, const Matrix3D& world) const;
    // As above through AddWithDepthPrepass, opacity 0.5 (the "ghost" of a moved brick).
    // Mac 0x232a40, Win 0x65746c
    void DrawWithDepthPrepass(RenderQueue* queue, const Matrix3D& camera,
                              const Matrix3D& world) const;

    // ---- mesh making (static) -----------------------------------------------------------------

    // Meshes of a rigid primitive: one per material slot (max(1, decoration faces); slot i uses
    // materials[i], or materials[0] when that is 0 or missing; NOTE: `materials` must not be
    // empty) from BCSM::CreateMesh, then one decal mesh per non-zero decoration 1.. from
    // BCSM::CreateDecorationMesh; `box` = the primitive's box. Fails (false) when a material
    // mesh is missing (meshes made so far stay in `meshes`). A flexible primitive goes to
    // BuildFlexMeshes with no bone transforms. Mac 0x231290, Win 0x65795e
    static bool BuildMeshes(const Primitive* primitive, const BasicArray<UInt32>& materials,
                            const BasicArray<UInt32>& decorations,
                            BrickConnectSceneManager* sceneManager,
                            BasicArray<RefPointer<Mesh> >& meshes, BoundingBox& box,
                            UInt32 materialSet);
    // Meshes of a flexible primitive: per decoration face a skinned vertex buffer of the face's
    // flex geometry (CreateFlexVertexBuffer with the bone transforms: none -> the primitive's
    // bones, one -> that transform * the primitive's bones, else exactly one per bone) with a
    // SingleMesh of the face material and, for a decorated face > 0, one of the decal material.
    // `box` = union of bone 0's box at each given bone transform (NOTE: reset/empty when none
    // are given). A rigid primitive goes to BuildMeshes. Mac 0x231500, Win 0x65751c (+ 0x65756e,
    // 0x657669)
    static bool BuildFlexMeshes(const Primitive* primitive, const BasicArray<Matrix3D>& boneTransforms,
                                const BasicArray<UInt32>& materials,
                                const BasicArray<UInt32>& decorations,
                                BrickConnectSceneManager* sceneManager,
                                BasicArray<RefPointer<Mesh> >& meshes, BoundingBox& box,
                                UInt32 materialSet);
    // The same by design ID (the scene manager's database; false for an unknown design).
    // Mac 0x231d10 / 0x231d60, Win 0x657ab4 / 0x657891
    static bool BuildMeshes(UInt32 designID, const BasicArray<UInt32>& materials,
                            const BasicArray<UInt32>& decorations,
                            BrickConnectSceneManager* sceneManager,
                            BasicArray<RefPointer<Mesh> >& meshes, BoundingBox& box,
                            UInt32 materialSet);
    static bool BuildFlexMeshes(UInt32 designID, const BasicArray<Matrix3D>& boneTransforms,
                                const BasicArray<UInt32>& materials,
                                const BasicArray<UInt32>& decorations,
                                BrickConnectSceneManager* sceneManager,
                                BasicArray<RefPointer<Mesh> >& meshes, BoundingBox& box,
                                UInt32 materialSet);

    const BasicArray<SubPart>& GetSubParts() const { return m_subParts; }
    const BoundingBox&         GetBoundingBox() const { return m_box; }

    // ---- data --------------------------------------------------------------------------------
    BrickConnectSceneManager* m_sceneManager;   // +0x00
    BasicArray<SubPart>       m_subParts;       // +0x04
    BoundingBox               m_box;            // +0x14
};

} // namespace LEGO
