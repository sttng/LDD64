// LEGO Digital Designer — restored source
// LEGO::BricksControl — a ModelControl that shows one or more bricks (PrimitiveElementBrick
//   snapshots): the items of the brick palette, the material / decoration pickers and the
//   building-guide / HTML / BOM previews. A flexible single brick gets a small "flex" badge.
// LEGO::BrickGroupControl — a ModelControl that shows a scene BrickGroup (its parts and those of
//   its sub-groups): the items of the group tree (GroupTreeViewControl::MyBrickGroupControl).
// The two item factories of LDDUtilities (TU 063) that make palette items are declared here too.
//
// Home TU 075 "BricksControl.cpp" (Mac 0x169800-0x16b6ab; Win 0x685cd0-0x686144 + 0x63957f /
// 0x63959b / 0x6790d1). Both constructors are inline (Mac: in LDDUtilities 0x14a980 / 0x14abe0,
// BGMode 0x34faf0, HTMLBIGenerator 0x1c53f0, ExportBOM 0x3b75c0, GroupTreeView 0x27dd10).
//
// BricksControl: RTTI si {ModelControl}. Mac sizeof 0x258: ModelControl +0x000..+0x238,
//   m_bricks +0x238, m_flexBadge +0x248, m_materialSet +0x24c, Object +0x250. Win sizeof 0x278
//   (m_bricks +0x254, badge +0x264, set +0x268, Object +0x270).
//   Vtables Mac 0x711dec (48: OnFinalRelease 0x16ac10, StartLoading 0x169fa0, OnLoaded
//   0x16a160), Win 0x9d3cd4 (StartLoading 0x685f79) + callback 0x9d3ca0 {0x63923b, OnLoaded
//   0x6860c7, OnNotHandled 0x685ca8}.
// BrickGroupControl: RTTI si {ModelControl}. Mac sizeof 0x244 (m_group +0x238, Object +0x23c);
//   Win m_group +0x254, Object +0x25c. Vtables Mac 0x7125c8 (StartLoading 0x16a420, OnLoaded
//   0x16a5f0), Win 0x9da0cc (StartLoading 0x686048) + callback 0x9da098 {…, OnLoaded 0x686144}.
// Method and member names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "ObjectRefPointer.h"
#include "ModelControl.h"
#include "BrickPalette.h"
#include "PrimitiveElementBrick.h"

namespace LEGO {

class BrickGroup;
class Material;
class ControlObserver;

class BricksControl : public ModelControl {
public:
    typedef BasicArray<ObjectRefPointer<PrimitiveElementBrick> > BrickArray;

    // Material set 1 = the palette's (LDD controls), 2 = the scene's (building guide previews).
    // (inline: Mac 0x14a980 ff.; Win 0x63915d)
    BricksControl(ControlManager* manager, BrickConnectSceneManager* sceneManager, UInt32 materialSet)
        : ModelControl(manager, sceneManager), m_bricks(), m_flexBadge(), m_materialSet(materialSet)
    {
    }
    virtual ~BricksControl();                                               // Mac 0x16ab40/0x16aba0 (body 0x16b4d0), Win sdd 0x63959b

    // ---- set-up (true when the control shows something) ----
    // The palette brick at the database's default orientation and camera; then
    // EnableModelInput. False for a design unknown to the database. Mac 0x169800, Win 0x686301
    bool Create(const BrickPalette::Brick* brick);
    // A copy of `brick`, seen from `camera`; then EnableModelInput. Mac 0x1699c0
    bool Create(const PrimitiveElementBrick& brick, const Vector2& camera);
    // A copy of `brick` from the database's default camera of its design (false for an unknown
    // design); then EnableModelInput. Mac 0x169ae0
    bool Create(const PrimitiveElementBrick& brick);
    // The bricks (shared), seen from `camera`; then EnableModelInput. Mac 0x169950
    bool Create(const BrickArray& bricks, const Vector2& camera);

    // Adds the palette brick (default orientation, camera = its default camera). Mac 0x169840
    bool AddBrick(const BrickPalette::Brick* brick);
    // Adds a copy of `brick`; camera = `camera`. Mac 0x169a00
    bool AddBrick(const PrimitiveElementBrick& brick, const Vector2& camera);

    // The first brick / its palette brick (NOTE: no check for an empty control).
    // Mac 0x169b50 / 0x169b60, Win 0x685cd0 / 0x685cd9
    PrimitiveElementBrick*     GetBrick() const;
    const BrickPalette::Brick* GetPaletteBrick() const;
    const BrickArray&          GetBricks() const { return m_bricks; }
    UInt32                     GetMaterialSet() const { return m_materialSet; }

    // Every brick and every mesh get the material. Mac 0x169b80, Win 0x685d92
    void SetMaterial(Material* material, UInt32 materialID);

    // ---- ModelControl ----
    // LazyLoad of every brick's design, materials and decorations. Mac 0x169fa0, Win 0x685f79
    virtual void StartLoading();
    // Builds the meshes of every part of every brick; false when one fails. Mac 0x16a160
    // (thunk 0x16a2d0), Win 0x6860c7
    virtual bool OnLoaded(const LazyLoadData& data);

protected:
    // Mac 0x16ac10 (= RenderControl's; Win ICF)
    virtual void OnFinalRelease();

    // The flex badge, then: cancel, no meshes, redraw (the meshes are made again on the next
    // draw); true. Mac 0x169be0
    bool Rebuild();
    // A single flexible brick gets the badge (a 19 x 19 child at the top right showing
    // "Graphics/LDDgraphics01.png" (183, 294, 14, 14)), any other content loses it.
    // Mac 0x169cc0
    void UpdateFlexBadge();

    // ---- data (Mac offsets / Win offsets) ----
    BrickArray                m_bricks;        // +0x238 / +0x254 (vtable Mac 0x760dd8)
    RefPointer<RenderControl> m_flexBadge;     // +0x248 / +0x264
    UInt32                    m_materialSet;   // +0x24c / +0x268
                                               // Object +0x250 / +0x270
};

class BrickGroupControl : public ModelControl {
public:
    // (inline: Mac 0x27dd10, Win 0x678e..)
    BrickGroupControl(ControlManager* manager, BrickConnectSceneManager* sceneManager)
        : ModelControl(manager, sceneManager), m_group(0)
    {
    }
    virtual ~BrickGroupControl();                                           // Mac 0x16b040/0x16b0a0, Win sdd 0x6790d1

    // Shows `group` (not referenced) from `camera`; the meshes are made again; then
    // EnableModelInput; true. Mac 0x16a2f0
    bool Create(BrickGroup* group, const Vector2& camera);
    BrickGroup* GetGroup() const { return m_group; }

    // ---- ModelControl ----
    // LazyLoad of the group's parts (recursively). Mac 0x16a420, Win 0x686048
    virtual void StartLoading();
    // The meshes of every part of the group (recursively); true. Mac 0x16a5f0 (thunk 0x16a760),
    // Win 0x686144
    virtual bool OnLoaded(const LazyLoadData& data);

protected:
    // Design, materials and decorations of the parts of `group` and its sub-groups.
    // Mac 0x16a550 (recursive)
    void CollectLoadData(const BrickGroup* group, LazyLoadData& data);
    // BrickDraw::AddPart(part, 1) for every part of `group` and its sub-groups. Mac 0x16a6f0
    void AddGroup(const BrickGroup* group);

    // ---- data ----
    BrickGroup* m_group;   // +0x238 / +0x254
                           // Object +0x23c / +0x25c
};

// ---- palette item factories (LDDUtilities, TU 063; names inferred) ----
// NOTE: on the Mac both take an unused first argument (dropped by /LTCG on Windows); it is
// left out here.
// A BricksControl (material set 1) of the palette brick below `parent` (cursor 100), observed by
// `observer`, with a tooltip (the localized brick name, or the full brick description when
// `fullDescription`) and a BrickCountAugmentation; null when it cannot be made.
// Mac 0x14abe0, Win 0x639445
BricksControl* CreateBricksControl(const BrickPalette::Brick* brick, ControlManager* manager,
                                   BrickConnectSceneManager* sceneManager, ControlObserver* observer,
                                   Control* parent, bool fullDescription);
// The same for a copy of `brick` seen from `camera` (tooltip: the localized name of its design).
// Mac 0x14a980 (callers: DecorationControl; Win partner not identified)
BricksControl* CreateBricksControl(const PrimitiveElementBrick& brick, const Vector2& camera,
                                   ControlManager* manager, BrickConnectSceneManager* sceneManager,
                                   ControlObserver* observer, Control* parent);

} // namespace LEGO
