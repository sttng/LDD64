// LEGO Digital Designer — restored source
// LEGO::MaterialToolControl — the MaterialControl pop-up of the material tool ("materialTool"):
//   the palette variants of the picked brick plus the extra button; a click on a variant gives the
//   picked part's brick that variant's materials and decorations, the extra button removes the
//   brick; either way the tool is told (MaterialTool slot 39).
//
// Home TU 095 "MaterialToolControl.cpp" (Mac 0x1a9970-0x1aa15b; Win 0x67dbf2, 0x693b00).
// RTTI si {MaterialControl}; Mac sizeof 0x270 (picked element +0x260, tool +0x264, Object +0x268).
// Virtual slots: Mac 0x71a84c (59 entries) = MaterialControl's with D1 0x1a9d10 / D0 0x1a9e00,
//   OnBricksControlClicked (57) 0x1a9c30, OnMoreClicked (58) 0x1a9cb0. Win sdd 0x693b00.
// Callers: MaterialTool::Create (Mac 0x1a8170, constructor), MaterialTool slot 28 (Mac 0x1a8930,
//   SetPickedElement)
// Names inferred except the RTTI class name.
#pragma once
#include "LEGO/Types.h"
#include "MaterialControl.h"

namespace LEGO {

class MaterialTool;
class PrimitiveElement;

class MaterialToolControl : public MaterialControl {
public:
    // `filter`: the items' filter (none: MaterialControl's own). Mac 0x1a9970
    MaterialToolControl(ControlManager* manager, BrickConnectSceneManager* sceneManager,
                        MaterialTool* tool, CompositeBrickFilter* filter, UInt32 rows,
                        UInt32 columns);
    virtual ~MaterialToolControl();                                         // Mac 0x1a9d10/0x1a9e00, Win sdd 0x693b00

    // The picked element (referenced) and the variants to offer (MaterialControl::SetBricks
    // without a camera, the extra button when `withButton`). Mac 0x1a9aa0, Win 0x67dbf2
    bool SetPickedElement(PrimitiveElement* picked, BrickArray bricks, bool withButton);

    // ---- MaterialControl ----
    // The picked part's brick takes the variant's materials / decorations
    // (BrickConnectSceneControl::SetBrickDescription); the tool is told. Mac 0x1a9c30
    virtual void OnBricksControlClicked(BricksControl* item);               // 57/–
    // The picked part's brick is removed; the tool is told (no variant). Mac 0x1a9cb0
    virtual void OnMoreClicked();                                           // 58/–

protected:
    RefPointer<PrimitiveElement> m_picked;   // Mac +0x260
    MaterialTool*                m_tool;     // Mac +0x264 (not referenced)
};

} // namespace LEGO
