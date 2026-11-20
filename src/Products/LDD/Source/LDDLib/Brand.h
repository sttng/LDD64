// LEGO Digital Designer — restored source
// LEGO::Brand (enum and file name inferred) — the LDD product flavour ("brand"): plain LDD,
// LEGO MINDSTORMS or LDD Extended. Stored by LDDApplication (+0x16c), written to LXFML / the
// palettes as text, and used to pick the brick sets of the palettes.
//
// Mac: TU 175 "Brand.cpp" (Mac 0x233130-0x23377f); Win: 0x6046b4-0x6048f0 (IsBrandMindstorms
// and the brick-set helpers are inlined there). Static initialiser Mac 0x233740: <iostream>.
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "BasicArray.h"

namespace LEGO {

enum Brand {                     // (names inferred from the strings)
    kBrandLDD         = 0,       // "LDD" (also read: "Factory", "MakeAndCreate")
    kBrandMindstorms  = 1,       // "Mindstorms"
    kBrandLDDExtended = 2        // "LDDExtended" (also read: "LEGOUniverse", "FreeBuild")
};

// Sets `out` to the brand's name (unknown values leave it unchanged).
void ToString(Brand value, String& out);                                     // Mac 0x233130, Win 0x6046b4
// Parses a brand keyword (case-insensitive, ended by ',' / blank / end) at `position`.
bool FromString(Brand& value, const String& text, UInt32& position);         // Mac 0x233180, Win 0x6046e0
// (names inferred)
bool IsBrandLDDExtended(Brand brand);                                        // Mac 0x233430, Win 0x6048dc
bool IsBrandMindstorms(Brand brand);                                         // Mac 0x233450 (Win inlined)
// The brick set of the brand's palettes: LDD 0, Mindstorms 2, LDD Extended 1 (table Mac
// 0x638480); 2 for unknown brands. (name inferred)
UInt32 GetBrandBrickSet(Brand brand);                                        // Mac 0x233460 (Win inlined)
// The brick sets shown for a brand: its own one, plus set 2 for set 0 (plain LDD also offers the
// Mindstorms bricks). (name inferred)
void GetBrandBrickSets(Brand brand, BasicArray<UInt32>& sets);               // Mac 0x233490 (Win inlined)

} // namespace LEGO
