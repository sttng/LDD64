// LEGO Digital Designer — restored source
// LEGO::CollapseMaingroupsCommand — collapses every main group of the brick palette
//   ("collapseMaingroupsCommand" in LDD.commands.editMode); enabled while the main groups are not
//   all collapsed (it observes the palette control).
//
// TU 391 "CollapseMaingroupsCommand.cpp": Mac 0x37c550-0x37d07b, Win 0x66dde7-0x66e132. The TU also
// emits Command / Observer template code; the last function of the TU is <iostream> + a header
// static (not code).
// RTTI: vmi {Command +0, BrickPaletteControlObserver (Mac +0x28, Win +0x2c)}, virtual base Object.
//   Mac vtable 0x753b8c {D1 0x37cbc0, D0 0x37cc30, OnFinalRelease 0x37ccc0, Do 0x37c900, SetEnable,
//   GetEnable, UpdateEnable 0x37c920, then the Itanium primary entries of the observer overriders
//   OnAttached 0x37cd50 (thunk 0x37ce90), OnMainGroupsExpanded 0x37c960 (thunk 0x37c990)}; Win {Do
//   0x66dfe3, …, UpdateEnable 0x66dff0}, observer-in {OnAttached 0x631d53 (ICF),
//   OnMainGroupsExpanded 0x66e013}.
// Layout (Mac): Command +0x00..+0x28, observer +0x28 (subject = the palette control +0x2c),
//   Object +0x30.
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"
#include "BrickPaletteControl.h"

namespace LEGO {

class CollapseMaingroupsCommand : public Command, private BrickPaletteControlObserver {
public:
    // Disabled; observes `palette` (attaching runs UpdateEnable). Mac 0x37c7e0, Win 0x66e02c
    explicit CollapseMaingroupsCommand(BrickPaletteControl* palette);
    virtual ~CollapseMaingroupsCommand();                                                  // Mac 0x37cbc0/0x37cc30, Win sdd 0x66e132

    // BrickPaletteControl::CollapseMainGroups (nothing without a palette).
    virtual void Do();                                                      // 3/0 Mac 0x37c900, Win 0x66dfe3
    // Enabled while there is a palette whose main groups are not all collapsed.
    virtual void UpdateEnable();                                            // 6/3 Mac 0x37c920, Win 0x66dff0

protected:
    // ---- BrickPaletteControlObserver ----
    virtual void OnAttached(BrickPaletteControl* palette);                  // 7 Mac 0x37cd50 (thunk 0x37ce90), Win 0x631d53
    // Expanded: enabled; collapsed: UpdateEnable.
    virtual void OnMainGroupsExpanded(bool expanded);                       // 8 Mac 0x37c960 (thunk 0x37c990), Win 0x66e013
    // Observer<BrickPaletteControl> part, then NotifierT<Command>'s. Mac 0x37ccc0
    virtual void OnFinalRelease();
};

} // namespace LEGO
