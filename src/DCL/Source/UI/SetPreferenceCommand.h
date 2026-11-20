// LEGO Digital Designer — restored source
// LEGO::SetPreferenceCommand — sets one preference (key = value) when run.
// TU 346 "SetPreferenceCommand.cpp" (Mac 0x32bdc0-0x32c24f; Win 0x53c4d0-0x53c60f). Directory
// UI/ is a guess (DCL range on Win).
// Mac layout (ctor 0x32bdc0): Command +0x00..+0x28, key +0x28, value +0x38, Object +0x48
// (sizeof 0x50). Win: key +0x2c, value +0x30, Object +0x38.
// Vtables: Mac 0x74c28c {D1 0x32c060, D0 0x32c120, OnFinalRelease 0x1414d0, Do 0x32bee0,
// SetEnable, GetEnable, UpdateEnable 0x3730c0}; Win 0xa2b188 {Do 0x53c500, …}.
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "Command.h"

namespace LEGO {

class SetPreferenceCommand : public Command {
public:
    SetPreferenceCommand(const String& key, const String& value);           // Mac 0x32bdc0 (Win inlined)
    virtual ~SetPreferenceCommand();                                        // Mac 0x32c060/0x32c120, Win 0x53c580 (sdd 0x53c4d0)

    // Preferences::SetValue(key, value).
    virtual void Do();                                                      // Mac 0x32bee0, Win 0x53c500

    const String& GetKey() const { return m_key; }
    const String& GetValue() const { return m_value; }

protected:
    String m_key;     // Mac +0x28, Win +0x2c
    String m_value;   // Mac +0x38, Win +0x30
};

} // namespace LEGO
