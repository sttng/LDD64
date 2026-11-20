// LEGO Digital Designer — restored source
// LEGO::ExpandMaingroupsCommand — expands every main group of the brick palette
//   ("expandMaingroupsCommand" in LDD.commands.editMode); enabled while the main groups are
//   collapsed (it observes the palette control).
//
// TU 406 "ExpandMaingroupsCommand.cpp": Mac 0x38cd00-0x38d36b, Win 0x66dc49-0x66dfab. The TU also
// emits Command / Observer template code; the last function of the TU is <iostream> + a header
// static (not code).
// RTTI: vmi {Command +0, BrickPaletteControlObserver (Mac +0x28, Win +0x2c)}, virtual base Object.
//   Mac vtable 0x7565bc {D1 0x38ceb0, D0 0x38cf20, OnFinalRelease 0x38cfb0, Do 0x38ce20, SetEnable,
//   GetEnable, UpdateEnable 0x38ce40, then the Itanium primary entries of the observer overriders
//   OnAttached 0x38d040 (thunk 0x38d180), OnMainGroupsExpanded 0x38ce80 (thunk 0x38ce90)}; Win {Do
//   0x66dc49, …, UpdateEnable 0x66dc56}, observer-in {OnAttached 0x631d53 (ICF),
//   OnMainGroupsExpanded 0x631d53 (ICF)}.
// Layout (Mac): Command +0x00..+0x28, observer +0x28 (subject = the palette control +0x2c),
//   Object +0x30.
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"
#include "BrickPaletteControl.h"

namespace LEGO {

class ExpandMaingroupsCommand : public Command, private BrickPaletteControlObserver {
public:
    // Disabled; observes `palette` (attaching runs UpdateEnable). Mac 0x38cd00, Win 0x66dea5
    explicit ExpandMaingroupsCommand(BrickPaletteControl* palette);
    virtual ~ExpandMaingroupsCommand();                                                  // Mac 0x38ceb0/0x38cf20, Win sdd 0x66dfab

    // BrickPaletteControl::ExpandMainGroups (nothing without a palette).
    virtual void Do();                                                      // 3/0 Mac 0x38ce20, Win 0x66dc49
    // Enabled while there is a palette whose main groups are collapsed.
    virtual void UpdateEnable();                                            // 6/3 Mac 0x38ce40, Win 0x66dc56

protected:
    // ---- BrickPaletteControlObserver ----
    virtual void OnAttached(BrickPaletteControl* palette);                  // 7 Mac 0x38d040 (thunk 0x38d180), Win 0x631d53
    // UpdateEnable.
    virtual void OnMainGroupsExpanded(bool expanded);                       // 8 Mac 0x38ce80 (thunk 0x38ce90), Win 0x631d53 (ICF)
    // Observer<BrickPaletteControl> part, then NotifierT<Command>'s. Mac 0x38cfb0
    virtual void OnFinalRelease();
};

} // namespace LEGO
