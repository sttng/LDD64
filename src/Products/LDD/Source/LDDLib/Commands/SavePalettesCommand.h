// LEGO Digital Designer — restored source
// LEGO::SavePalettesCommand — developer command ("savePalettes" in LDD.commands.editMode):
//   writes every bag of the palette control's brick palette as
//   "<user data>/SavedPalettes/<bag path name>.paxml" (BrickPaletteBagLoaderPAXML::Save).
//
// TU 439 "SavePalettesCommand.cpp": Mac 0x3b5e10-0x3b64fb, Win 0x66bad6 / 0x62c251. The
// constructor is inline (Mac 0x18e130, emitted in EditMode.cpp; Win 0x62af2b). The TU also emits
// the BasicArray<CommandObserver*> base-destructor helper Mac 0x3b63a0 (Notifier.h); Mac 0x3b6480
// = <iostream> + a header static (not code).
// RTTI: SavePalettesCommand : Command. Mac vtable 0x758edc {D1 0x3b61d0, D0 0x3b6230,
//   OnFinalRelease (NotifierT's), Do 0x3b5e10, SetEnable, GetEnable, UpdateEnable 0x3b62a0};
//   Win 0x9d197c {Do 0x66bad6, …, UpdateEnable 0x630465}, Object-in 0x9d1968 {sdd 0x62c251}.
// Layout (Mac): Command +0x00..+0x28, palette control +0x28 (referenced), Object +0x2c.
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"
#include "BrickPaletteControl.h"

namespace LEGO {

class SavePalettesCommand : public Command {
public:
    // Enabled. Mac 0x18e130 (inline, emitted in EditMode.cpp), Win 0x62af2b
    explicit SavePalettesCommand(BrickPaletteControl* palette) : Command(true), m_palette(palette) {}
    virtual ~SavePalettesCommand();                                         // Mac 0x3b61d0/0x3b6230, Win sdd 0x62c251

    // See the class comment.
    virtual void Do();                                                      // 3/0 Mac 0x3b5e10, Win 0x66bad6
    // Enabled.
    virtual void UpdateEnable();                                            // 6/3 Mac 0x3b62a0, Win 0x630465

private:
    RefPointer<BrickPaletteControl> m_palette;   // Mac +0x28
};

} // namespace LEGO
