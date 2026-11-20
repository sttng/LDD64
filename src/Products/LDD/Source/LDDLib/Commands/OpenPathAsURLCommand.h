// LEGO Digital Designer — restored source
// LEGO::OpenPathAsURLCommand — opens a file of the installation (a Path) in the web browser
//   (BrickModeGlobalObject's "openManual": Help/<language>-manual/<language>-manual.html).
//
// Home TU 247 "OpenPathAsURLCommand.cpp" (Mac 0x272d70-0x27309f, Win 0x6306f7 / 0x684c78):
// Do, UpdateEnable and the destructor in OpenPathAsURLCommand.cpp. The constructor is
// header-inline (Mac 0x182a80 emitted in TU 084, Win 0x630229).
// Mac layout: Command +0x00..+0x28, Path +0x28 (vptr, name +0x2c, directory +0x3c), Object +0x40
// (sizeof 0x48). Win: Path +0x2c, Object +0x3c (sizeof 0x44).
// Vtables: Mac 0x73977c {D1 0x272d90, D0 0x272df0, OnFinalRelease 0x1414d0, Do 0x272d70,
// SetEnable, GetEnable, UpdateEnable 0x272e60}; Win 0x9d2a1c {Do 0x684c78, …, 0x630465},
// Object-in 0x9d2a08 {sdd 0x6306f7}.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"
#include "Path.h"

namespace LEGO {

class OpenPathAsURLCommand : public Command {
public:
    // Command(enabled); default Path, then Assign(path). (Mac 0x182a80, Win 0x630229)
    OpenPathAsURLCommand(const Path& path, bool enabled) : Command(enabled) { m_path = path; }
    virtual ~OpenPathAsURLCommand();                                         // Mac 0x272d90/0x272df0, Win sdd 0x6306f7

    // Platform::OpenDocument(path) (the system opens the file: the browser for .html).
    virtual void Do();                                                       // Mac 0x272d70, Win 0x684c78
    virtual void UpdateEnable();                                             // Mac 0x272e60, Win 0x630465 (SetEnable(true))

    const Path& GetPath() const { return m_path; }

protected:
    Path m_path;   // Mac +0x28, Win +0x2c
};

} // namespace LEGO
