// LEGO Digital Designer — restored source
// TU 346 "SetPreferenceCommand.cpp": Mac 0x32bdc0-0x32c24f, Win 0x53c4d0-0x53c60f.
// Not code: Mac 0x32bf70 (destructor body), thunks, 0x32c210 = <iostream>.
#include "SetPreferenceCommand.h"
#include "Preferences.h"

namespace LEGO {

// Mac 0x32bdc0
SetPreferenceCommand::SetPreferenceCommand(const String& key, const String& value)
    : Command(true), m_key(key), m_value(value)
{
}

// Mac 0x32c060 (D1), 0x32c120 (D0), Win 0x53c580 (sdd 0x53c4d0)
SetPreferenceCommand::~SetPreferenceCommand()
{
}

// Mac 0x32bee0, Win 0x53c500
void SetPreferenceCommand::Do()
{
    RefPointer<Preferences> preferences = Preferences::GetInstance();
    preferences->SetValue(m_key, m_value);
}

} // namespace LEGO
