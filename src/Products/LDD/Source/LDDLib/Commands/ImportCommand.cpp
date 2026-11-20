// LEGO Digital Designer — restored source
// TU 241 "ImportCommand.cpp": Mac 0x271640-0x2719eb, Win 0x66a245 / 0x62c175.
#include "ImportCommand.h"
#include "BrickConnectSceneManager.h"
#include "ClickStickTool.h"
#include "LDDApplication.h"

namespace LEGO {

// Mac 0x271690 (D1) / 0x271700 (D0), Win sdd 0x62c175
ImportCommand::~ImportCommand()
{
}

// Mac 0x271780
void ImportCommand::OnFinalRelease()
{
    Command::OnFinalRelease();
}

// Mac 0x271640, Win 0x66a245
void ImportCommand::Do()
{
    m_sceneManager->SetActiveTool(m_clickStickTool);
    if (!LDDApplication::GetInstance()->ImportWithPanel(m_clickStickTool))
        m_sceneManager->SetActiveTool(0);
}

// Mac 0x2717d0, Win 0x630465
void ImportCommand::UpdateEnable()
{
    SetEnable(true);
}

} // namespace LEGO
