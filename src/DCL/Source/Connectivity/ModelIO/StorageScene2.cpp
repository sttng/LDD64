// LEGO Digital Designer — restored source
// StorageScene2 has no out-of-line code of its own: every member is header-inline (see
// StorageScene2.h for the Mac weak copies and the Win /LTCG out-of-line copies). This unit only
// makes sure the header compiles on its own.
//
// Mac TU 440 (0x3b6500-0x3b73cb) is NOT StorageScene2 code: it is
// the palette-from-model helper of BrickPaletteBagLoaderModel (caller Mac 0x17d170, Win 0x5fcece):
//   Mac 0x3b6500 / Win 0x64f402  load a model file into a StorageScene2 (Mac 0x14a680), then build
//                                one BrickPalette::Brick per StorageScene2::Brick (design and
//                                assembly checks against BrickDatabase, materials/decorations
//                                validated per surface, traces "Cannot create brick: …",
//                                "Part ID error in brick: …", "Material error on surface: …",
//                                "Decoration error on surface: …", "Cannot find brick: …")
//   Mac 0x3b7280 / Win 0x64f26e  set up the bag from the scene: name = scene name or
//                                "User Palette", clear the rest
//   Win 0x64f303                 per-assembly part list helper (inlined on Mac)
// The TU is restored in LDDLib/BrickPaletteBagLoaderModel.cpp (it needs BrickPalette, TU 141,
// and BrickDatabase, 024b-d).
#include "StorageScene2.h"
