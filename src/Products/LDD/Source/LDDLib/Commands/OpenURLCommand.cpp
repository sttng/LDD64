// LEGO Digital Designer — restored source
// TU 248 "OpenURLCommand.cpp": Mac 0x2730a0-0x27337f, Win 0x684c89 / 0x630716. The TU also emits
// the BasicArray<CommandObserver*> base-destructor helper Mac 0x273270 (Notifier.h); Mac 0x273340
// = <iostream> + a header static (not code).
#include "OpenURLCommand.h"
#include "Platform.h"

namespace LEGO {

// Mac 0x2730c0 (D1) / 0x273120 (D0), Win sdd 0x630716
OpenURLCommand::~OpenURLCommand()
{
}

// Mac 0x2730a0, Win 0x684c89
void OpenURLCommand::Do()
{
    Platform::GetInstance()->OpenURL(m_url);
}

} // namespace LEGO
