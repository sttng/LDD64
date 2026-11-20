// LEGO Digital Designer — restored source
// LEGO::DecorationTool — "decorationTool": a click on a decorable surface pops up the
//   DecorationToolControl with that surface's decorations at the press position - (10, 10).
//
// TU 195 "DecorationTool.cpp" (Mac 0x2476b0-0x247d0b; Win 0x67d554-0x67da07).
// RTTI: DecorationTool : SurfacePickTool. Mac vtable 0x732b5c (37 entries), sizeof 0x74
//   (m_control +0x60, m_pressPosition +0x64, Object +0x6c); Win 0x9db36c, sizeof 0x84.
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "SurfacePickTool.h"
#include "DecorationToolControl.h"

namespace LEGO {

class ControlManager;
class BrickConnectSceneControl;

class DecorationTool : public SurfacePickTool {
public:
    // SurfacePickTool(false) picking decoration faces; cursor 0x70. Mac 0x2476b0, Win 0x67d8ba
    DecorationTool();
    virtual ~DecorationTool();                                              // Mac 0x2478c0/0x2479e0, Win sdd 0x67da07

    // SurfacePickTool::Create with the manager's render, the pop-up (manager, scene manager,
    // 10 x 8) below `sceneControl`. Mac 0x2477e0, Win 0x67d5b6
    bool Create(BrickConnectSceneControl* sceneControl, ControlManager* manager,
                BrickConnectSceneManager* sceneManager);

    // The pop-up for the clicked surface. Mac 0x247ae0, Win 0x67d636
    virtual void OnPickClicked(bool flag);                                  // 28/23
    // The pop-up has decorations for the surface. Mac 0x247b70, Win 0x67d69e
    virtual bool IsPickable(PrimitiveElement* element, UInt32 surface);     // 30/25
    // Remembers the press position, SurfacePickTool's. Mac 0x247bf0 (thunk 0x247c20), Win 0x67d572
    virtual void OnMousePressed(Control* control, const Vector2& position, UInt32 button);

protected:
    // SurfacePickTool's; the selection is cleared. Mac 0x247ba0, Win 0x67d554
    virtual void OnAttached(BrickConnectSceneControl* control);
    // The pop-up is hidden; SurfacePickTool's. Mac 0x247bd0, Win 0x67d6a9
    virtual void OnDetached(BrickConnectSceneControl* control);
    using SurfacePickTool::OnAttached;
    using SurfacePickTool::OnDetached;
    // SurfacePickTool's; the pop-up is hidden. Mac 0x247c60, Win 0x67d6c5
    virtual void OnEvent19();

    RefPointer<DecorationToolControl> m_control;         // +0x60
    Vector2                           m_pressPosition;   // +0x64
};

} // namespace LEGO
