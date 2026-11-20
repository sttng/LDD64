// LEGO Digital Designer — restored source
// LEGO::AbstractPart — the read-only part interface (design ID, materials, decorations);
// LEGO::Part — one part of a Brick in the connectivity scene: its primitive, one
// PrimitiveElement per bone of the primitive, materials and decorations.
//
// Part.cpp = sub-TU 025d of the original Scene.cpp (Mac 0x94100-0x95c80 + 025s inline virtuals
// 0xc3670-0xc3a20; Win 0x46ef20-0x46f610, 0x4718d0-0x4722a0), with PrimitiveElement::SetHidden
// 0x95bb0 / ApplyPointGroups 0x95790 of that range; its Scene functions (0x95970
// AddChangedGroup, 0x95ad0 AddChangedRigidElement) are in Scene.cpp.
// RTTI: class LEGO::AbstractPart : virtual Object.
//   Mac vtable 0x70649c {pure ×5, D1 0xc3670, D0 0xc3690}. Win: destructor in the Object part
//   (sdd 0x46ef20).
// RTTI: class LEGO::Part : AbstractPart, BehaviorElement (Win RTTI attribute 3).
//   Mac sizeof 0x60 (constructor 0x95460, base-object variant with VTT 0x95190 for LDDPart,
//       BrickConnect::CreatePart 0xc2a90): AbstractPart vptr +0 (vtable 0x705f80
//       {GetDesignID 0xc3740, GetMaterialCount 0xc3750, GetMaterial 0x942e0, GetDecoration
//       0x94920, IsFlexible 0xc3770, D1 0xc3790, D0 0xc37f0, GetType 0x94100, GetBoundingBox
//       0x94120, GetScene 0x942a0, OnMaterialsChanged 0x95720, OnDecorationsChanged 0xc3860}),
//       BehaviorElement +0x04 (vtable 0x705fbc {thunks 0x941b0, 0x942c0, 0xc3870, 0xc38d0,
//       0x94110}), own members from +0x18, Object at +0x58 (vtable 0x705fe8). Base-object
//       destructor (VTT) 0xcbb30.
//   Win sizeof 0x68 (constructor 0x472000, BrickConnect::CreatePart 0x490af0): AbstractPart
//       vfptr +0 {0x46f440, 0x46f450, 0x471a30, 0x471e00, 0x46f4b0, OnMaterialsChanged
//       0x4721c0, OnDecorationsChanged} / vbptr +4, BehaviorElement +8 (vfptr {0x4718d0,
//       0x471a20, …}), own members +8 from +0x20 (m_brick +0x20 … m_hidden +0x5c), Object at
//       +0x60. Destructor 0x46f4e0 (sdd 0x46f5e0).
// Every member and method name is inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BehaviorElement.h"
#include "BasicArray.h"
#include "ConnectionField.h"
#include "Geometry.h"

namespace LEGO {

class Brick;
class BrickGroup;
class Primitive;
class PrimitiveElement;

class AbstractPart : public virtual Object {
public:
    AbstractPart() {}
    virtual UInt32 GetDesignID() const = 0;                    // slot 0
    virtual UInt32 GetMaterialCount() const = 0;               // slot 1
    virtual UInt32 GetMaterial(UInt32 index) const = 0;        // slot 2
    virtual UInt32 GetDecoration(UInt32 index) const = 0;      // slot 3
    virtual bool   IsFlexible() const = 0;                     // slot 4
    virtual ~AbstractPart() {}                                 // slots 5/6: Mac 0xc3670 / 0xc3690, Win sdd 0x46ef20

private:
    AbstractPart(const AbstractPart&);
    AbstractPart& operator=(const AbstractPart&);
};

class Part : public AbstractPart, public BehaviorElement {
public:
    // Per material slot: changed flag and the new ConnectionPoint::FLAG_GROUP state (set when
    // the new material is transparent). RTTI BasicArray<Pair<bool, unsigned long>>
    // (Mac 0x7628e8 / 0x947c0, Win BasicArray<Pair<bool,unsigned int>>).
    typedef ConnectionField::PointGroup PointGroup;

    // One (empty) element slot per bone, max(1, decoration faces) materials (0), no
    // decorations. Mac 0x95460 (base-object variant 0x95190), Win 0x472000
    explicit Part(const Primitive* primitive);

    // --- AbstractPart (Mac primary slots 0-6)
    virtual UInt32 GetDesignID() const;                        // Mac 0xc3740 (025s), Win 0x46f440 (primitive's)
    virtual UInt32 GetMaterialCount() const { return m_materials.GetSize(); }  // Mac 0xc3750 (025s), Win 0x46f450
    // materials[index], or materials[0] when that is 0 (no range check). Mac 0x942e0, Win 0x471a30
    virtual UInt32 GetMaterial(UInt32 index) const;
    // decorations[index] or 0. Mac 0x94920, Win 0x471e00
    virtual UInt32 GetDecoration(UInt32 index) const;
    virtual bool   IsFlexible() const;                         // Mac 0xc3770 (025s), Win 0x46f4b0 (bones > 1)
    // Only the member destructors. Mac 0xc3790 / 0xc37f0 (base 0xcbb30), Win 0x46f4e0 / sdd 0x46f5e0
    virtual ~Part();
    // --- BehaviorElement (Mac primary slots 7-9)
    virtual UInt32 GetType() const { return TypePart; }        // Mac 0x94100 (thunk 0x94110)
    // Union of the bones' boxes at the elements' world transforms (`recompute` ignored).
    // Mac 0x94120 (thunk 0x941b0), Win 0x4718d0
    virtual void   GetBoundingBox(BoundingBox& box, bool recompute) const;
    // The first element's scene (NOTE: no check for a part without elements). Mac 0x942a0
    // (thunk 0x942c0), Win 0x471a20
    virtual Scene* GetScene() const;
    // --- slot 10: after SetMaterial(s) changed a material: every element applies the point
    // groups to its fields (connection points of transparent materials); the Scene records
    // the part's group (m_flag123). LDDPart overrides it. Mac 0x95720, Win 0x4721c0
    virtual void   OnMaterialsChanged(const BasicArray<PointGroup>& groups);
    // --- slot 11: after SetDecorations changed decorations (`faces`: the changed indices).
    // Empty here. Mac 0xc3860 (025s)
    virtual void   OnDecorationsChanged(const BasicArray<UInt32>& faces) { (void)faces; }

    // ---- non-virtual API ----------------------------------------------------------------------

    // Box and its transform: a rigid part gives the primitive's box and the element's world
    // transform, a flexible part its world box (GetBoundingBox) and the identity.
    // Mac 0x94240, Win 0x471980
    void GetOrientedBoundingBox(BoundingBox& box, Matrix3D& transform) const;
    // Sets material slot `index` (slots sharing a sub-material redirect group are set
    // together); calls OnMaterialsChanged when something changed. Mac 0x946c0, Win 0x471c00
    void SetMaterial(UInt32 index, UInt32 material);
    // SetMaterial for every slot of `materials` (indices 0 .. material count - 1; NOTE: no
    // check of the size of `materials`), one OnMaterialsChanged. Mac 0x947f0, Win 0x471d00
    void SetMaterials(const BasicArray<UInt32>& materials);
    // Takes `decorations` (padded with 0 / truncated to the decoration face count); changed
    // faces: the bodies are marked changed (Scene m_flag122) and OnDecorationsChanged is
    // called. Mac 0x94950, Win 0x471e20
    void SetDecorations(const BasicArray<UInt32>& decorations);
    // Moves the part to `group` (BrickGroup::RemovePart / AddPart). Mac 0x94d60, Win 0x471fa0
    void SetGroup(BrickGroup* group);
    // Moves the part to `brick` (added to the new brick before it leaves the old one).
    // Mac 0x94f60, Win 0x471fd0
    void SetBrick(Brick* brick);
    // Part of Brick::SetHidden: the hidden flags of the part and its elements; `updateGroup`
    // records m_group in the Scene. Win 0x472250 (Mac inlined into 0x93510)
    void SetHidden(bool hidden, bool updateBodies, bool updateGroup, Scene* scene);
    // Called by the model loaders after the materials and decorations are set. Empty.
    // Mac 0x94d50 (callers 0x1437b0, 0x29ef20; Win inlined)
    void UpdateState() {}

    const Primitive*  GetPrimitive() const { return m_primitive; }
    Brick*            GetBrick() const { return m_brick; }
    BrickGroup*       GetGroup() const { return m_group; }
    bool              IsHidden() const { return m_hidden; }
    UInt32            GetElementCount() const { return m_elements.GetSize(); }
    PrimitiveElement* GetElement(UInt32 index) const { return m_elements[index]; }
    const BasicArray<UInt32>& GetMaterials() const { return m_materials; }
    const BasicArray<UInt32>& GetDecorations() const { return m_decorations; }

    // ---- data (Mac offsets; Win +8) -----------------------------------------------------------
    Brick*                                  m_brick;        // +0x18 (not referenced)
    BrickGroup*                             m_group;        // +0x1c (not referenced)
    RefPointer<const Primitive>             m_primitive;    // +0x20
    BasicArray<RefPointer<PrimitiveElement> > m_elements;   // +0x24 one per bone (PrimitiveElement::SetPart)
    BasicArray<UInt32>                      m_materials;    // +0x34
    BasicArray<UInt32>                      m_decorations;  // +0x44
    bool                                    m_hidden;       // +0x54 false

private:
    // Marks the bodies of all elements changed in `scene`. Win 0x472220 (Mac inlined into 0x94950)
    void AddChangedBodies(Scene* scene) const;
};

} // namespace LEGO
