// LEGO Digital Designer — restored source
// TU 406 "ExpandMaingroupsCommand.cpp": Mac 0x38cd00-0x38d36b, Win 0x66dc49-0x66dfab.
#include "ExpandMaingroupsCommand.h"

namespace LEGO {

// Mac 0x38cd00, Win 0x66dea5
ExpandMaingroupsCommand::ExpandMaingroupsCommand(BrickPaletteControl* palette)
    : Command(false, false)
{
    BrickPaletteControlObserver::Attach(palette);
}

// Mac 0x38ceb0 (D1) / 0x38cf20 (D0), Win sdd 0x66dfab
ExpandMaingroupsCommand::~ExpandMaingroupsCommand()
{
}

// Mac 0x38cfb0
void ExpandMaingroupsCommand::OnFinalRelease()
{
    BrickPaletteControlObserver::OnFinalRelease();
    Command::OnFinalRelease();
}

// Mac 0x38ce20, Win 0x66dc49
void ExpandMaingroupsCommand::Do()
{
    if (BrickPaletteControl* palette = GetSubject())
        palette->ExpandMainGroups();
}

// Mac 0x38ce40, Win 0x66dc56
void ExpandMaingroupsCommand::UpdateEnable()
{
    BrickPaletteControl* palette = GetSubject();
    SetEnable(palette ? palette->AreMainGroupsCollapsed() : false);
}

// Mac 0x38d040 (thunk 0x38d180), Win 0x631d53 (ICF: UpdateEnable)
void ExpandMaingroupsCommand::OnAttached(BrickPaletteControl* /*palette*/)
{
    UpdateEnable();
}

// Mac 0x38ce80 (thunk 0x38ce90), Win 0x631d53 (ICF)
void ExpandMaingroupsCommand::OnMainGroupsExpanded(bool /*expanded*/)
{
    UpdateEnable();
}

} // namespace LEGO
