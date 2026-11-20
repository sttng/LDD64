// LEGO Digital Designer — restored source
// TU 407 "BGScreenShotCommand.cpp": Mac 0x38d370-0x38d71b, Win 0x65ebb5-0x65ec37.
#include "BGScreenShotCommand.h"

#include <iostream>

#include "RenderQueue.h"
#include "RenderControlManager.h"
#include "BGMode.h"

namespace LEGO {

// Mac 0x38d370, Win 0x65eaff
BGScreenShotCommand::BGScreenShotCommand(BrickConnectSceneControl* control, BGMode* mode)
    : ScreenShotCommand(control), m_queue(), m_mode(mode),
      m_render(static_cast<RenderControlManager*>(control->GetManager())->GetRender())
{
}

// Mac 0x38d4f0 (D1) / 0x38d5e0 (D0) (thunks 0x38d5a0 … 0x38d660), Win sdd 0x65ebb5
BGScreenShotCommand::~BGScreenShotCommand()
{
}

// Mac 0x38d440, Win 0x65ebd4
void BGScreenShotCommand::Do()
{
    if (!m_queue)
        m_queue = new RenderQueue(m_render, 2000);
    m_mode->QueueParts(m_queue);
    TakeScreenShot(m_queue);
}

} // namespace LEGO
