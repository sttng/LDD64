// LEGO Digital Designer — restored source
// LEGO::LDDPart — the application's Part: keeps the render state of a flexible part (one
// FlexSurface per decoration face: skinned geometry, vertex buffer, mesh, decal mesh, and the
// bone transforms they were skinned with) and updates the meshes' materials / decal meshes
// when the part's materials or decorations change.
// LEGO::LDDBrick — the application's Brick: remembers the BrickPalette::Brick it was created
// from (its name becomes the brick's name).
// (LDDBrickConnect, the factory of both, is in LDDBrickConnect.h.)
//
// Bodies in LDDPart.cpp (Mac TU 351, 0x332610-0x33355b, file name from the TU guess;
// Win 0x653eef-0x654430). In LDDLib: the classes need the application layer
// (BrickConnectSceneManager, BrickPalette), and TU 351 sits between application TUs in both
// link orders.
//
// RTTI: class LEGO::LDDPart : Part (Win RTTI attribute 3).
//   Mac sizeof 0x84 (constructor inlined into LDDBrickConnect::CreatePart 0x332a20: Part base
//       constructor 0x95190 with VTT 0x74cbc4): vptr +0 (vtable 0x74cb3c: Part's slots with
//       D1 0x332c40, D0 0x332ca0, OnMaterialsChanged 0x332680, OnDecorationsChanged 0x3327a0),
//       BehaviorElement +4 (vtable 0x74cb78 {0x941b0, 0x942c0, 0x332d10, 0x332d70, 0x94110}),
//       own members from +0x58, Object at +0x7c (vtable 0x74cba4 {0x332de0, 0x332e40, …}).
//   Win sizeof 0x88 (constructor 0x654293: thiscall(primitive, sceneManager, mostDerived),
//       CreatePart 0x6543b2): AbstractPart vfptr +0 (0x9d65b4 {…, OnMaterialsChanged
//       0x653f16, OnDecorationsChanged 0x653fae}), vbptr +4, BehaviorElement +8 (0x9d65a4),
//       own members +8 from +0x60, Object at +0x84 (vftable 0x9d6590 {sdd 0x6543f5, …});
//       destructor 0x654338 (0x65439c).
// RTTI: class LEGO::LDDBrick : Brick (Win RTTI attribute 3).
//   Mac sizeof 0x4c (constructors inlined into LDDBrickConnect::CreateBrick 0x332980 and
//       0x332b60 (CloneBrick 0x332b10); Brick base constructor 0x333080 with VTT 0x777794):
//       vptr +0 (vtable 0x777724: Brick's slots with D1 0x333190, D0 0x333240), BehaviorElement
//       +4 (vtable 0x777750), m_paletteBrick +0x40, Object at +0x44 (vtable 0x77777c).
//   Win sizeof 0x48 (constructors 0x65407a (designID) / 0x6540f6 (copy), destructor 0x65417a
//       / 0x6541c4, sdd 0x654247): m_paletteBrick +0x3c, Object at +0x40.
// RTTI: BasicArray<LDDPart::FlexSurface> (Mac vtable 0x777948 {D1 0x333340, D0 0x3333a0},
//   Win 0x9d64dc {sdd 0x654271}).
// Every member and method name is inferred unless noted.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "Geometry.h"
#include "Brick.h"
#include "Part.h"
#include "Mesh.h"
#include "VertexBuffer.h"
#include "BrickGeometry.h"
#include "BrickPalette.h"

namespace LEGO {

class BrickConnectSceneManager;

class LDDPart : public Part {
public:
    // Render state of one decoration face of a flexible part, built by the scene control
    // (BrickConnectSceneControl Mac 0x48320 / 0x50a80). sizeof 0x10 (both builds); the
    // destructor (Mac 0x569d0, BCSC TU) releases the four references in reverse order.
    struct FlexSurface {
        RefPointer<const BrickGeometry> m_geometry;      // +0x00 BCSM::GetFlexGeometry(primitive, face)
        RefPointer<VertexBuffer>        m_vertexBuffer;  // +0x04 skinned copy (FlexVertexBuffer)
        RefPointer<Mesh>                m_mesh;          // +0x08 SingleMesh(material, m_vertexBuffer)
        RefPointer<Mesh>                m_decalMesh;     // +0x0c SingleMesh(decal material, m_vertexBuffer) or 0
    };

    // Part(primitive) + empty flex state. Mac inlined (0x332a20), Win 0x654293
    LDDPart(const Primitive* primitive, BrickConnectSceneManager* sceneManager);
    // Mac 0x332c40 / 0x332ca0 (thunks 0x332d10, 0x332d70, 0x332de0, 0x332e40; base-object
    // destructor 0x333410), Win 0x654338 / sdd 0x6543f5
    virtual ~LDDPart();

    // --- slot 10: Part's, then the mesh of every flex surface that has a vertex buffer gets
    // the scene manager's material (set 0) for the face's material. Mac 0x332680, Win 0x653f16
    virtual void OnMaterialsChanged(const BasicArray<PointGroup>& groups);
    // --- slot 11: for every changed face (no range check): a new decal mesh over the face's
    // vertex buffer when the face has a decoration whose decal material exists, else none.
    // Nothing when there are no flex surfaces. Mac 0x3327a0, Win 0x653fae
    virtual void OnDecorationsChanged(const BasicArray<UInt32>& faces);

    BrickConnectSceneManager*        GetSceneManager() const { return m_sceneManager; }
    BasicArray<FlexSurface>&         GetFlexSurfaces() { return m_flexSurfaces; }
    const BasicArray<FlexSurface>&   GetFlexSurfaces() const { return m_flexSurfaces; }
    BasicArray<Matrix3D>&            GetFlexTransforms() { return m_flexTransforms; }
    const BasicArray<Matrix3D>&      GetFlexTransforms() const { return m_flexTransforms; }

    // ---- data (Mac offsets; Win +8) -----------------------------------------------------------
    BasicArray<FlexSurface>   m_flexSurfaces;    // +0x58 one per decoration face (flexible parts)
    BasicArray<Matrix3D>      m_flexTransforms;  // +0x68 bone transforms of the last skinning
    BrickConnectSceneManager* m_sceneManager;    // +0x78 (not referenced)
};

class LDDBrick : public Brick {
public:
    // Brick(designID), no palette brick. Mac inlined (0x332980), Win 0x65407a
    explicit LDDBrick(UInt32 designID);
    // Brick(source.GetDesignID()) sharing the palette brick (the name is not copied).
    // Mac 0x332b60, Win 0x6540f6
    LDDBrick(const LDDBrick& source);
    // Mac 0x333190 / 0x333240 (thunks 0x333280, 0x3332a0, 0x3332e0, 0x333300), Win 0x65417a /
    // sdd 0x654247
    virtual ~LDDBrick();

    // Mac 0x332610 (Win inlined)
    const BrickPalette::Brick* GetPaletteBrick() const { return m_paletteBrick; }
    // References `brick` and takes its name as the brick's name (NOTE: `brick` must not be
    // null; the name is copied even when the brick does not change). Mac 0x332620, Win 0x653ef4
    void SetPaletteBrick(const BrickPalette::Brick* brick);

    RefPointer<const BrickPalette::Brick> m_paletteBrick;   // Mac +0x40 / Win +0x3c

private:
    LDDBrick& operator=(const LDDBrick&);
};

} // namespace LEGO
