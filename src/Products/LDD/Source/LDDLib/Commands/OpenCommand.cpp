// LEGO Digital Designer — restored source
// TU 245 "OpenCommand.cpp": Mac 0x2729f0-0x272d2f, Win 0x66a239 / 0x62c156 / 0x62c712.
#include "OpenCommand.h"
#include "Application.h"

namespace LEGO {

// Mac 0x272a10 (D1) / 0x272a80 (D0), Win sdd 0x62c156
OpenCommand::~OpenCommand()
{
}

// Mac 0x272b00, Win 0x62c712
void OpenCommand::OnFinalRelease()
{
    Command::OnFinalRelease();
}

// Mac 0x2729f0, Win 0x66a239
void OpenCommand::Do()
{
    // NOTE: Application's slot 11 is named NewDocument; LDD's override asks for a file to open.
    Application::GetInstance()->NewDocument();
}

// Mac 0x272b50, Win 0x630465
void OpenCommand::UpdateEnable()
{
    SetEnable(true);
}

} // namespace LEGO
