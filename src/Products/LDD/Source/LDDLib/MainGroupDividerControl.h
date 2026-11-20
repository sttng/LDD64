// LEGO Digital Designer — restored source
// LEGO::MainGroupDividerControl — the header tile of one main group in the brick palette
//   (BrickPaletteControl): a 64 x 64 toggle button showing "MainGroupDividers/<group>.png" of
//   the brick database (fallback "Graphics/EditScreen/DefaultDivider.png"), with a small
//   expand / collapse marker (EditMode_Final.png) in its bottom-right corner that follows the
//   toggle state.
//
// TU 134 "MainGroupDividerControl.cpp": Mac 0x1ee090-0x1ef44b, Win 0x691149-0x69181e (+ sdd
// 0x6625b6). Mac 0x1ef3d0 = <iostream> + a header static (not code). The constructor is inline
// (in BrickPaletteControl, Mac 0x69d70: operator new 0x254).
// RTTI: si {ToggleControl}, virtual base Object. Mac vtable 0x72565c (56 entries: ToggleControl's
//   54 with NotifyToggled 52 = 0x1eecd0, NotifyUntoggled 53 = 0x1eed20; Create(String) 54 =
//   0x1ee090, Create(UInt32) 55 = 0x1eec50); Win: NotifyToggled 0x6917d7, NotifyUntoggled
//   0x6917fb, Create(UInt32) 49 = 0x691149, Create(String) 50 = 0x69119a (MSVC reverses the
//   overload pair).
// Layout (Mac / Win): ToggleControl +0x000..+0x240 / +0x25c, image +0x240, collapsed marker
//   +0x244, expanded marker +0x248, Object +0x24c (Mac sizeof 0x254).
// Method and member names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "Texture.h"
#include "ToggleControl.h"
#include "RenderControl.h"

namespace LEGO {

class BrickDatabase;

class MainGroupDividerControl : public ToggleControl {
public:
    // (inline in the original: ToggleControl(manager), the three members null)
    explicit MainGroupDividerControl(ControlManager* manager) : ToggleControl(manager) {}
    virtual ~MainGroupDividerControl();                                     // Mac 0x1eed70/0x1eedd0, Win sdd 0x6625b6

    // ---- ToggleControl overrides: the markers follow the state ----
    // Toggled (expanded): the expanded marker is shown, the collapsed one hidden.
    virtual void NotifyToggled();                                           // 52/47 Mac 0x1eecd0, Win 0x6917d7
    virtual void NotifyUntoggled();                                         // 53/48 Mac 0x1eed20, Win 0x6917fb

    // ---- new virtuals ----
    // ToggleControl::Initialize; Setup below `parent` (flags 7, empty rectangle, mouse input,
    // not hit-testable); input flag 0x40000000 cleared; clear colour transparent white; the
    // divider image "MainGroupDividers/<mainGroup>.png" from the database directory (fallback
    // DefaultDivider.png); the two 14 x 14 markers (bottom-right, 10 px inset); state images
    // 64 x 64 (up, over, down, disabled; the same for the toggled states). False when the
    // image, EditMode_Final.png or a marker cannot be created.
    virtual bool Create(Control* parent, const String& mainGroup, BrickDatabase* database); // 54/50 Mac 0x1ee090, Win 0x69119a
    // The same with the main group's number as the name.
    virtual bool Create(Control* parent, UInt32 mainGroup, BrickDatabase* database);        // 55/49 Mac 0x1eec50, Win 0x691149

    using ToggleControl::Create;

protected:
    RefPointer<Texture>       m_image;            // Mac +0x240
    RefPointer<RenderControl> m_collapsedMarker;  // Mac +0x244 shown while untoggled
    RefPointer<RenderControl> m_expandedMarker;   // Mac +0x248 shown while toggled
};

} // namespace LEGO
