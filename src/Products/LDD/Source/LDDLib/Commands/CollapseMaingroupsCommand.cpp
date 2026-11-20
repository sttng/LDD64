// LEGO Digital Designer — restored source
// TU 391 "CollapseMaingroupsCommand.cpp": Mac 0x37c550-0x37d07b, Win 0x66dde7-0x66e132.
#include "CollapseMaingroupsCommand.h"

namespace LEGO {

// Mac 0x37c7e0, Win 0x66e02c
CollapseMaingroupsCommand::CollapseMaingroupsCommand(BrickPaletteControl* palette)
    : Command(false, false)
{
    BrickPaletteControlObserver::Attach(palette);
}

// Mac 0x37cbc0 (D1) / 0x37cc30 (D0), Win sdd 0x66e132
CollapseMaingroupsCommand::~CollapseMaingroupsCommand()
{
}

// Mac 0x37ccc0
void CollapseMaingroupsCommand::OnFinalRelease()
{
    BrickPaletteControlObserver::OnFinalRelease();
    Command::OnFinalRelease();
}

// Mac 0x37c900, Win 0x66dfe3
void CollapseMaingroupsCommand::Do()
{
    if (BrickPaletteControl* palette = GetSubject())
        palette->CollapseMainGroups();
}

// Mac 0x37c920, Win 0x66dff0
void CollapseMaingroupsCommand::UpdateEnable()
{
    BrickPaletteControl* palette = GetSubject();
    SetEnable(palette ? !palette->AreMainGroupsCollapsed() : false);
}

// Mac 0x37cd50 (thunk 0x37ce90), Win 0x631d53 (ICF: UpdateEnable)
void CollapseMaingroupsCommand::OnAttached(BrickPaletteControl* /*palette*/)
{
    UpdateEnable();
}

// Mac 0x37c960 (thunk 0x37c990), Win 0x66e013
void CollapseMaingroupsCommand::OnMainGroupsExpanded(bool expanded)
{
    if (expanded)
        SetEnable(true);
    else
        UpdateEnable();
}

} // namespace LEGO
