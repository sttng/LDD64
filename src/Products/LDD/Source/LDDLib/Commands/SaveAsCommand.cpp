// LEGO Digital Designer — restored source
// TU 254 "SaveAsCommand.cpp": Mac 0x2752e0-0x27556f, Win 0x66a286 / 0x62c1b3.
#include "SaveAsCommand.h"
#include "Application.h"

namespace LEGO {

// Mac 0x275300 (D1) / 0x275370 (D0), Win sdd 0x62c1b3
SaveAsCommand::~SaveAsCommand()
{
}

// Mac 0x2752e0, Win 0x66a286
void SaveAsCommand::Do()
{
    Application::GetInstance()->SaveDocumentAs();
}

// Mac 0x2753f0, Win 0x630465
void SaveAsCommand::UpdateEnable()
{
    SetEnable(true);
}

} // namespace LEGO
