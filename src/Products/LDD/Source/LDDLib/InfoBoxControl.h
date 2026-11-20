// LEGO Digital Designer — restored source
// LEGO::InfoBoxControl — the status text under the scene ("infoControl" in the Lua layout):
//   the total brick count of the scene ("TOTAL BRICKS INFO", e.g. "0 bricks"), or the selection
//   ("SELECTED BRICKS INFO"; for one brick its id, localized name and materials). Follows the
//   scene control's selection (BrickConnectSceneControlObserver).
//
// TU 303 "InfoBoxControl.cpp": Mac 0x2aff40-0x2b149b, Win 0x6769c6-0x677034. Mac 0x2b1420 =
// <iostream> + a header static (not code).
// RTTI: vmi {RenderControl +0, BrickConnectSceneControlObserver (Mac +0x1ec, Win +0x208)},
//   virtual base Object. Mac vtable 0x74452c (48 entries: RenderControl's 45, Create 45, then
//   the Itanium primary entries of the observer overriders OnEvent15 46 / OnEvent19 47),
//   observer-in 0x744640 {OnEvent15 thunk 0x2b13a0, OnEvent19 thunk 0x2b13f0}; Win 0x9d9854
//   (41 entries: Create 40 = 0x676ad5), observer-in 0x9d97dc {12: 0x677011, 16: 0x67701c}.
// Layout (Mac / Win): RenderControl +0x000..+0x1ec / +0x208, BrickConnectSceneControlObserver
//   +0x1ec / +0x208 (subject +0x1f0 / +0x20c), database +0x1f4 / +0x214, flag +0x1f8 / +0x218
//   (false; not read in the TU), Object +0x1fc / +0x220 (Mac sizeof 0x204, Win 0x228).
// Method and member names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "BasicArray.h"
#include "RenderControl.h"
#include "BrickConnectSceneControlObserver.h"

namespace LEGO {

class BrickDatabase;
class BrickConnectSceneControl;
class Part;

class InfoBoxControl : public RenderControl, public BrickConnectSceneControlObserver {
public:
    InfoBoxControl(ControlManager* manager, BrickDatabase* database);       // Mac 0x2aff40, Win 0x6769c6
    virtual ~InfoBoxControl();                                              // Mac 0x2b03c0/0x2b05e0, Win sdd 0x676ab3

    // Observes `sceneControl`; transparent background. Always true.
    // Mac slot 45 (+0xb4), Win slot 40 (+0xa0).
    virtual bool Create(BrickConnectSceneControl* sceneControl);           // Mac 0x2b0040, Win 0x676ad5

    // ---- BrickConnectSceneControlObserver (Mac observer slot, primary slot) ----
    // The selection changed: the text for `selection`.
    virtual void OnEvent15(const BasicArray<RefPointer<Part> >& selection); // 15, 46 Mac 0x2b0a90 (thunk 0x2b13a0), Win 0x677011
    // The text for the scene control's current selection.
    virtual void OnEvent19();                                               // 19, 47 Mac 0x2b13c0 (thunk 0x2b13f0), Win 0x67701c

    // The text: no part selected: "TOTAL BRICKS INFO" with the scene's brick count; one brick
    // (of the selected parts): "SELECTED BRICK ID INFO", "SELECTED BRICK NAME INFO" and
    // "SELECTED BRICK COLOR INFO" ("<id> - <Material<id>>, …") joined by spaces; more:
    // "SELECTED BRICKS INFO" with the count ("%s" in each localized text is replaced). Then the
    // height follows the text (ResizeHeightToFitText(0)). Mac 0x2b0aa0, Win 0x676b18
    void UpdateInfo(const BasicArray<RefPointer<Part> >& selection);

protected:
    // The observer part (leaves the scene control), then RenderControl's. Mac 0x2b0850
    virtual void OnFinalRelease();

    RefPointer<BrickDatabase> m_database;   // Mac +0x1f4, Win +0x214
    bool                      m_flag;       // Mac +0x1f8, Win +0x218 (meaning unknown)
};

} // namespace LEGO
