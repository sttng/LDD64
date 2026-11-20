// LEGO Digital Designer — restored source
// LEGO::StartupDelayedCommand — the end of the start-up (StartMode hands over):
//   on the first main-loop pass it exits the start mode, ends the window's modal state, enters
//   the edit mode, closes (= resets) the document, then opens the document given at start-up,
//   or the most recent model ("LoadMostRecentModel"), or shows the start help ("ShowStartHelp").
//
// TU 259 "StartupDelayedCommand.cpp" (Mac 0x277580-0x277d4b; Win 0x6090aa, 0x5ca0b4/0x5ca0c7).
// The constructor is header-inline (Win 0x5c9fee).
// Mac layout: DelayedCommand +0x00..+0x30, edit mode +0x30, start mode +0x34 (referenced),
// Object +0x38 (sizeof 0x40). Win: edit mode +0x38, start mode +0x3c, Object +0x44.
// Vtables: Mac 0x73aadc {D1 0x277930, D0 0x277990, OnFinalRelease 0x26eb50, Do 0x277580, …,
// UpdateEnable 0x277a00, Update 0x26e9b0}; Win 0x9ca2a0 {Do 0x6090aa, …, 0x630465}.
#pragma once
#include "LEGO/Types.h"
#include "DelayedCommand.h"

namespace LEGO {

class Mode;
class EditMode;

class StartupDelayedCommand : public DelayedCommand {
public:
    // Win 0x5c9fee (argument names inferred)
    StartupDelayedCommand(bool enabled, EditMode* editMode, Mode* startMode)
        : DelayedCommand(enabled), m_editMode(editMode), m_startMode(startMode) {}
    virtual ~StartupDelayedCommand();                                       // Mac 0x277930/0x277990, Win 0x5ca0b4 (sdd 0x5ca0c7)

    virtual void Do();                                                      // Mac 0x277580, Win 0x6090aa
    virtual void UpdateEnable();                                            // Mac 0x277a00 (SetEnable(true)), Win 0x630465

protected:
    EditMode*        m_editMode;    // Mac +0x30, Win +0x38 (not referenced)
    RefPointer<Mode> m_startMode;   // Mac +0x34, Win +0x3c
};

} // namespace LEGO
