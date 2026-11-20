// LEGO Digital Designer — restored source
// LEGO::MaterialTool — "materialTool": a click on a part pops up the MaterialToolControl next to
//   the press position with the palette bricks that match the clicked brick (BrickMatcher over the
//   application palette's bricks that pass the filter); picking a variant repaints the brick.
//
// TU 093 "MaterialTool.cpp" (Mac 0x1a7fa0-0x1a903b; Win 0x67da26-0x67de95).
// RTTI: MaterialTool : PickTool, LDDApplicationObserver (+0xa4). Mac vtable 0x719d1c (41
//   entries: PickTool's 38 + Create 38, OnMaterialPicked 39, the LDDApplicationObserver
//   overrider 40), sizeof 0xd0 (Object +0xc8); Win primary 0x9db4b4 (31: Create 29 0x67db12,
//   OnMaterialPicked 30 0x6943f1), LDDApplicationObserver-in 0x9db410, sizeof 0xe8.
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "PickTool.h"
#include "BrickPalette.h"
#include "LDDApplication.h"
#include "BrickMatcher.h"
#include "BrickFilter.h"
#include "MaterialToolControl.h"

namespace LEGO {

class ControlManager;
class BrickConnectSceneControl;
class BrickConnectSceneManager;
class CompositeBrickFilter;
class Brick;

class MaterialTool : public PickTool, public LDDApplicationObserver {
public:
    // `withButton`: the pop-up's extra button; `filter` (referenced): the palette bricks the
    // matcher may offer. Observes the LDDApplication; cursor 0x6d. EditMode passes
    // (false, new CompositeBrickFilter); the original stores `withButton` as a byte.
    // Mac 0x1a7fa0, Win 0x67de95
    MaterialTool(bool withButton, CompositeBrickFilter* filter);
    virtual ~MaterialTool();                                                // Mac 0x1a8590/0x1a8740, Win sdd 0x67dbd0

    // Keeps the scene manager, creates the pop-up (manager, scene manager, this, `filter`, 10 x 8)
    // below `sceneControl`. EditMode passes the brick palette control's material filter
    // (GetMaterialFilter, Mac 0x186423). Mac 0x1a8170, Win 0x67db12
    virtual bool Create(BrickConnectSceneControl* sceneControl, ControlManager* manager,
                        BrickConnectSceneManager* sceneManager, CompositeBrickFilter* filter); // 38/29
    // MaterialToolControl reports the variant picked for `brick` (null: the brick was removed,
    // "more" button). Mac slot 39 (+0x9c) 0x1a8f30 (empty). (name inferred)
    virtual void OnMaterialPicked(Brick* /*brick*/, const BrickPalette::Brick* /*paletteBrick*/) {} // 39/30

    // ---- PickTool ----
    // The pop-up with the clicked brick's matches at the press position - (10, 10).
    // Mac 0x1a8930, Win 0x67dd2a
    virtual void OnPickClicked(bool flag);
    // Remembers the press position, PickTool's. Mac 0x1a8e20 (thunk 0x1a8e50), Win 0x67da26
    virtual void OnMousePressed(Control* control, const Vector2& position, UInt32 button);

protected:
    // BrickConnectSceneControlTool's, then leaves the LDDApplication. Mac 0x1a8270, Win 0x67de76
    virtual void OnFinalRelease();
    // BCSCTool's, the selection cleared; the first time the matcher over the palette's bricks
    // that pass the filter. Mac 0x1a8cb0, Win 0x67dc31
    virtual void OnAttached(BrickConnectSceneControl* control);
    // The pop-up is hidden; PickTool's. Mac 0x1a8df0, Win 0x67db8d
    virtual void OnDetached(BrickConnectSceneControl* control);
    using PickTool::OnAttached;
    using PickTool::OnDetached;
    using LDDApplicationObserver::OnAttached;
    using LDDApplicationObserver::OnDetached;
    // PickTool's; the pop-up is hidden. Mac 0x1a8e90, Win 0x67dbac
    virtual void OnEvent19();
    // LDDApplicationObserver: the brand (palette) changed: the matcher is rebuilt on the next
    // attach. Mac 0x1a8ec0 (entry 40, thunk 0x1a8f00), Win 0x67dbc3
    virtual void OnBrandChanged(Brand brand);

    // ---- data (Mac offsets; LDDApplicationObserver +0xa4, its subject +0xa8) ----
    RefPointer<MaterialToolControl>      m_control;       // +0xac
    Vector2                              m_pressPosition; // +0xb0
    RefPointer<BrickConnectSceneManager> m_sceneManager;  // +0xb8
    RefPointer<CompositeBrickFilter>     m_filter;        // +0xbc
    RefPointer<BrickMatcher>             m_matcher;       // +0xc0
    bool                                 m_withButton;    // +0xc4
                                                          // Object +0xc8
};

} // namespace LEGO
