// LEGO Digital Designer — restored source
// LEGO::ClickStickPickupTool — a PickTool whose drag hands the picked part (with the selection it
//   belongs to, or alone) to the click-stick tool, which then carries it (base of SelectionTool).
//
// TU 416 "ClickStickPickupTool.cpp" (second object of the class: Mac 0x394f00-0x39532b, Win
// 0x6614de). The constructor is inline (in SelectionTool's, Mac 0x160210 / 0x15ff00); the
// destructor is emitted in SelectionTool.cpp (Mac D1 0x161620, D0 0x1616d0, Win sdd 0x62834a).
// RTTI: ClickStickPickupTool : PickTool. Mac vtable 0x71049c (38 entries: PickTool's with
//   OnPickDragged 0x394f00); Win 0x9d0794 (OnPickDragged slot 23 0x6614de).
// Layout (Mac): PickTool +0x00..+0xa3, m_clickStickTool +0xa4, m_replaceSelection +0xa8,
//   Object +0xac (sizeof 0xb4). Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "PickTool.h"
#include "ClickStickTool.h"

namespace LEGO {

class ClickStickPickupTool : public PickTool {
public:
    // (inline) PickTool(true), the click-stick tool referenced.
    ClickStickPickupTool(ClickStickTool* clickStickTool, bool replaceSelection)
        : PickTool(true), m_clickStickTool(clickStickTool), m_replaceSelection(replaceSelection) {}
    virtual ~ClickStickPickupTool() {}                                      // Mac 0x161620/0x1616d0, Win sdd 0x62834a

    // The selection plus the picked part (a part outside the selection replaces it when
    // m_replaceSelection) go to the click-stick tool, which becomes the active tool:
    // ClickStickTool::PickUpParts(parts, picked, pick point, modifier bit 1, modifier bit 2
    // (copy), this). Mac 0x394f00, Win 0x6614de
    virtual void OnPickDragged();                                           // 28/23

protected:
    RefPointer<ClickStickTool> m_clickStickTool;    // +0xa4
    bool                       m_replaceSelection;  // +0xa8
};

} // namespace LEGO
