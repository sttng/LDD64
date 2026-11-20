// LEGO Digital Designer — restored source
// TU 439 "SavePalettesCommand.cpp": Mac 0x3b5e10-0x3b64fb, Win 0x66bad6 / 0x62c251.
#include "SavePalettesCommand.h"
#include "BrickPalette.h"
#include "BrickPaletteBagLoaderPAXML.h"
#include "Application.h"
#include "Directory.h"
#include "Path.h"

namespace LEGO {

// Mac 0x3b61d0 (D1) / 0x3b6230 (D0), Win sdd 0x62c251
SavePalettesCommand::~SavePalettesCommand()
{
}

// Mac 0x3b5e10, Win 0x66bad6
void SavePalettesCommand::Do()
{
    BrickPaletteBagLoaderPAXML loader;
    RefPointer<Directory> directory = Application::GetInstance()->GetUserDataDirectory();
    BrickPalette* palette = m_palette->GetBrickPalette();
    for (BrickPalette::BagArray::ConstIterator it = palette->GetBagsBegin(); it != palette->GetBagsEnd(); ++it) {
        RefPointer<const BrickPalette::Bag> bag = *it;
        String name("SavedPalettes/");
        name << bag->m_path.GetName() << ".paxml";
        loader.Save(Path(name, directory), *bag);
    }
}

// Mac 0x3b62a0, Win 0x630465
void SavePalettesCommand::UpdateEnable()
{
    SetEnable(true);
}

} // namespace LEGO
