// LEGO Digital Designer — restored source
// TU 247 "OpenPathAsURLCommand.cpp": Mac 0x272d70-0x27309f, Win 0x684c78 / 0x6306f7. The TU also
// emits the BasicArray<CommandObserver*> base-destructor helper Mac 0x272f60 (Notifier.h);
// Mac 0x273060 = <iostream> + a header static (not code).
#include "OpenPathAsURLCommand.h"
#include "Platform.h"

namespace LEGO {

// Mac 0x272d90 (D1) / 0x272df0 (D0), Win sdd 0x6306f7
OpenPathAsURLCommand::~OpenPathAsURLCommand()
{
}

// Mac 0x272d70, Win 0x684c78
void OpenPathAsURLCommand::Do()
{
    Platform::GetInstance()->OpenDocument(m_path);
}

// Mac 0x272e60, Win 0x630465
void OpenPathAsURLCommand::UpdateEnable()
{
    SetEnable(true);
}

} // namespace LEGO
