// LEGO Digital Designer — restored source
// TU 357 "BIEvent.cpp" (Mac 0x33ed70-0x33f02f): the building-guide events. See BIEvent.h.
#include "BIEvent.h"
#include "BGScene.h"

namespace LEGO {

// ---- BIEventBrick ---------------------------------------------------------------------------

// Mac 0x33ee10 (D0 0x33ee20), Win 0x553240 (sdd 0x5532a0)
BIEventBrick::~BIEventBrick()
{
}

// Mac 0x33ee60, Win 0x5d5710
BIBrick* BIEventBrick::GetBrick() const
{
    return m_brick;
}

// Mac 0x33ed70, Win 0x577aa0
bool BIEventBrick::Apply(BGScene* scene, bool forward)
{
    if (forward)
        return scene->AddBrick(m_brick, true);
    return scene->RemoveBrick(m_brick, true);
}

// Mac 0x33eda0, Win 0x577af0 (shared by every event class)
bool BIEventBrick::GetFlag() const
{
    return false;
}

// ---- BIEventStep ----------------------------------------------------------------------------

// Mac 0x33eed0 (D0 0x33ef30), Win 0x55a480 (sdd 0x55a4f0)
BIEventStep::~BIEventStep()
{
}

// Mac 0x33edd0, Win 0x577b00
bool BIEventStep::Apply(BGScene* /*scene*/, bool /*forward*/)
{
    return true;
}

// Mac 0x33ede0, Win 0x577af0
bool BIEventStep::GetFlag() const
{
    return false;
}

// ---- BIEventUnknown -------------------------------------------------------------------------

// Mac 0x33ee70 (D0 0x33ee80), Win sdd 0x560060
BIEventUnknown::~BIEventUnknown()
{
}

// Mac 0x33edb0, Win 0x57baa0
bool BIEventUnknown::Apply(BGScene* /*scene*/, bool /*forward*/)
{
    return false;
}

// Mac 0x33edc0, Win 0x577af0
bool BIEventUnknown::GetFlag() const
{
    return false;
}

// ---- BIEventJumper --------------------------------------------------------------------------

// Mac 0x33efa0 (D0 0x33efb0), Win 0x5661d0 (sdd 0x566230)
BIEventJumper::~BIEventJumper()
{
}

// Mac 0x33edf0, Win 0x577b00
bool BIEventJumper::Apply(BGScene* /*scene*/, bool /*forward*/)
{
    return true;
}

// Mac 0x33ee00, Win 0x577af0
bool BIEventJumper::GetFlag() const
{
    return false;
}

} // namespace LEGO
