// LEGO Digital Designer — restored source
// LEGO::SaveTraceStringCommand — saves the trace history (GetTraceHistory) as a UTF-16 text file
// chosen in a save panel ("Write log error!" message panels on failure).
// TU 385 "SaveTraceStringCommand.cpp" (Mac 0x372e20-0x373a8f, Win 0x423a70-0x423f1f, sdd
// 0x5c9d68); also holds the weak header-inline Command instances (Mac 0x372e20 Do, 0x372e30
// SetEnable, 0x3730b0 GetEnable, 0x3730c0 UpdateEnable, 0x3730e0 IsToggleable, 0x3730f0
// IsToggled, 0x373100 SetToggled). Directory UI/ is a guess (DCL range on Win). The ctor is
// header-inline (signature inferred).
// Mac layout: Command +0x00..+0x28, window +0x28, Object +0x2c (sizeof 0x34). Win: window
// +0x2c, Object +0x34.
// Vtables: Mac 0x751af0 {D1 0x373840, D0 0x3738b0, OnFinalRelease 0x1414d0, Do 0x373380, …};
// Win 0x9ca1e4 {Do 0x423c20, SetEnable 0x423a70, GetEnable 0x423b30, UpdateEnable 0x423b40}.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"

namespace LEGO {

class Window;

class SaveTraceStringCommand : public Command {
public:
    explicit SaveTraceStringCommand(Window* window) : Command(true), m_window(window) {}
    virtual ~SaveTraceStringCommand();                                      // Mac 0x373840/0x3738b0, Win 0x5c9a87 (sdd 0x5c9d68)

    // Save panel (no directory, no name, no types, `m_window`, empty title), then the file:
    // BOM 0xfeff + the history as UTF-16, truncated to what was written.
    virtual void Do();                                                      // Mac 0x373380, Win 0x423c20

protected:
    Window* m_window;   // Mac +0x28, Win +0x2c (not referenced)
};

} // namespace LEGO
