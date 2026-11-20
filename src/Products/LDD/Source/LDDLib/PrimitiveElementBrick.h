// LEGO Digital Designer — restored source
// LEGO::PrimitiveElementBrick — a value snapshot of one brick: the BrickPalette::Brick it is
// an instance of, plus per part its bone transforms, materials and decorations and the
// behaviors of the brick, its parts and its bones. Taken from an LDDBrick of the scene (copy &
// paste, the brick list of the clipboard / BricksControl / HTML building instructions, the
// brick palette "used bricks" bucket) or made from a palette brick at a transform; BrickDraw
// draws one (BrickDraw::AddPart(const PrimitiveElementBrick&, part, …)). Empty per-part arrays
// mean "the palette brick's defaults" (GetBoneTransforms / GetMaterials / GetDecorations).
// BrickLess orders them for the BasicMap<RefPointer<PrimitiveElementBrick>, unsigned long,
// PrimitiveElementBrick::BrickLess> of the HTML building instructions (TU 113).
//
// Restored in PrimitiveElementBrick.cpp (Mac TU 184, 0x237da0-0x23cc2f; Win 0x655a36-0x657079).
// Directory: LDDLib (depends on LDDBrick and BrickPalette, see LDDPart.h).
//
// RTTI: class LEGO::PrimitiveElementBrick : virtual Object (Win RTTI attribute 0).
//   Mac sizeof 0x70 (operator new 0x70 at BrickConnectSceneControl 0x4c6e9): vptr +0 (vtable
//       0x770fcc {D1 0x23a480, D0 0x23a4e0}), members from +0x04, Object at +0x68 (vtable
//       0x770fec {thunks 0x23a550, 0x23a5b0, …}); base-object destructor 0x23c890 (VTT 0x771000).
//   Win sizeof 0x70: vbptr +0, the same member offsets, Object at +0x68 (vftable 0x9d6744
//       {sdd 0x65695f, …}); destructor 0x656608 (body 0x656572).
// Member and method names are inferred, except the class and BrickLess (RTTI).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "Geometry.h"
#include "StorageScene2.h"
#include "BrickPalette.h"

namespace LEGO {

class LDDBrick;

class PrimitiveElementBrick : public virtual Object {
public:
    typedef StorageScene2::Behavior Behavior;

    // Order of the HTML building instructions' brick map: by the palette bricks' sort keys;
    // with equal keys by IsMaterialLess, unless neither brick has own materials (then equal).
    // Null pointers compare equal. Mac 0x239f40, Win 0x655c9f
    struct BrickLess {
        bool operator()(const PrimitiveElementBrick* a, const PrimitiveElementBrick* b) const;
    };

    // `brick` at `transform`: per part the palette part's bone transforms moved by
    // `transform`; no own materials / decorations / behaviors. Mac 0x238110, Win 0x65661b
    PrimitiveElementBrick(const BrickPalette::Brick* brick, const Matrix3D& transform);
    // Copy. NOTE: the bone transforms are copied for as many parts as the palette brick has
    // (both builds): `other` must have them (every constructor creates them). Mac 0x2383c0,
    // Win 0x656dbc (+ 0x656e69)
    PrimitiveElementBrick(const PrimitiveElementBrick& other);
    // Snapshot of a scene brick: its palette brick, per part the elements' world transforms
    // (by bone index), the part's materials and decorations, and the behaviors of the brick,
    // the parts and the elements. Mac 0x2390e0, Win 0x656ff2 (+ 0x657079)
    explicit PrimitiveElementBrick(const LDDBrick* brick);
    // Mac 0x23a480 / 0x23a4e0, Win 0x656608 / sdd 0x65695f: member destructors only
    virtual ~PrimitiveElementBrick();

    // Rebuilds the bone transforms from the palette brick: transform * bone for every bone of
    // every part. Mac 0x237e60, Win 0x656114
    void SetTransform(const Matrix3D& transform);

    // The palette brick's part (no range check). Mac 0x239970, Win 0x655a3a
    const BrickPalette::Part* GetPalettePart(UInt32 part) const;
    // Own per-part arrays, or the palette part's when this brick has none. Mac 0x239990 /
    // 0x2399c0 / 0x239cb0, Win 0x655a4a / 0x655a6d / 0x655a90
    const BasicArray<Matrix3D>& GetBoneTransforms(UInt32 part) const;
    const BasicArray<UInt32>&   GetMaterials(UInt32 part) const;
    const BasicArray<UInt32>&   GetDecorations(UInt32 part) const;

    // Every material slot of every part (as many as the palette part has) = `material`.
    // Mac 0x2399f0, Win 0x6561fc
    void SetMaterial(UInt32 material);
    // Decoration `index` of `part` (the part's array grows with zeros as needed); the
    // decoration array gets one (possibly empty) entry per palette part. Mac 0x239ce0,
    // Win 0x6562c1
    void SetDecoration(UInt32 part, UInt32 index, UInt32 decoration);
    // Decoration `index` of `part` = 0 when it exists. Mac 0x239e40, Win 0x656347
    void RemoveDecoration(UInt32 part, UInt32 index);

    // Lexicographic comparison of the materials of the parts 0 .. own part material arrays - 1
    // (own or palette defaults on either side; NOTE: `other`'s arrays are not range checked).
    // Mac 0x239fe0, Win 0x655ab3
    bool IsMaterialLess(const PrimitiveElementBrick& other) const;
    // Same palette brick and the same materials (as IsMaterialLess walks them).
    // Mac 0x23a190, Win 0x655d1b
    bool operator==(const PrimitiveElementBrick& other) const;
    // Mac 0x23a1b0, Win 0x655b3a
    bool IsMaterialEqual(const PrimitiveElementBrick& other) const;

    // Mac 0x23a300, Win 0x655a36
    const BasicArray<Behavior>& GetBrickBehaviors() const { return m_brickBehaviors; }
    // The part's behaviors, or an empty array. Mac 0x23a310, Win 0x65697e
    const BasicArray<Behavior>& GetPartBehaviors(UInt32 part) const;
    // The behaviors of bone (element) `bone` of `part`, or an empty array. NOTE: `part` is
    // checked against the part behaviors and `bone` against the number of parts (both builds).
    // Mac 0x23a3b0, Win 0x6569ec
    const BasicArray<Behavior>& GetBoneBehaviors(UInt32 part, UInt32 bone) const;
    // The palette brick's IsFlexible. Mac 0x23a460
    bool IsFlexible() const;

    const BrickPalette::Brick* GetBrick() const { return m_brick; }

    // ---- data (both builds) --------------------------------------------------------------
    RefPointer<const BrickPalette::Brick>             m_brick;           // +0x04
    BasicArray<BasicArray<Matrix3D> >                 m_boneTransforms;  // +0x08 per part, per bone
    BasicArray<BasicArray<UInt32> >                   m_materials;       // +0x18 per part (empty: palette)
    BasicArray<BasicArray<UInt32> >                   m_decorations;     // +0x28 per part (empty: palette)
    BasicArray<Behavior>                              m_brickBehaviors;  // +0x38
    BasicArray<BasicArray<Behavior> >                 m_partBehaviors;   // +0x48 per part
    BasicArray<BasicArray<BasicArray<Behavior> > >    m_boneBehaviors;   // +0x58 per part, per element

private:
    PrimitiveElementBrick& operator=(const PrimitiveElementBrick&);
};

} // namespace LEGO
