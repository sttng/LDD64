// LEGO Digital Designer — restored source
// TU 175 "Brand.cpp" (file name inferred) — Mac 0x233130-0x23377f, Win 0x6046b4-0x6048f0.
// See Brand.h.
#include "Brand.h"
#include "StringStream.h"

#include <iostream>

namespace LEGO {

namespace {
const UInt32 kBrickSetOfBrand[3] = { 0, 2, 1 };   // Mac 0x638480
} // namespace

// Mac 0x233130, Win 0x6046b4
void ToString(Brand value, String& out)
{
    const char* name;
    if (value == kBrandLDD)
        name = "LDD";
    else if (value == kBrandMindstorms)
        name = "Mindstorms";
    else if (value == kBrandLDDExtended)
        name = "LDDExtended";
    else
        return;
    out = name;
}

// Mac 0x233180, Win 0x6046e0
bool FromString(Brand& value, const String& text, UInt32& position)
{
    if (ParseKeyword(String("LDD"), text, position, ',')) {
        value = kBrandLDD;
        return true;
    }
    if (ParseKeyword(String("Factory"), text, position, ',')) {
        value = kBrandLDD;
        return true;
    }
    if (ParseKeyword(String("Mindstorms"), text, position, ',')) {
        value = kBrandMindstorms;
        return true;
    }
    if (ParseKeyword(String("LEGOUniverse"), text, position, ',')) {
        value = kBrandLDDExtended;
        return true;
    }
    if (ParseKeyword(String("MakeAndCreate"), text, position, ',')) {
        value = kBrandLDD;
        return true;
    }
    if (ParseKeyword(String("LDDExtended"), text, position, ',')
        || ParseKeyword(String("FreeBuild"), text, position, ',')) {
        value = kBrandLDDExtended;
        return true;
    }
    return false;
}

// Mac 0x233430, Win 0x6048dc
bool IsBrandLDDExtended(Brand brand)
{
    if ((UInt32)brand < 3)
        return brand == kBrandLDDExtended;
    return false;
}

// Mac 0x233450
bool IsBrandMindstorms(Brand brand)
{
    return brand == kBrandMindstorms;
}

// Mac 0x233460
UInt32 GetBrandBrickSet(Brand brand)
{
    if ((UInt32)brand < 3)
        return kBrickSetOfBrand[brand];
    return 2;
}

// Mac 0x233490
void GetBrandBrickSets(Brand brand, BasicArray<UInt32>& sets)
{
    sets.Clear();
    const UInt32 set = GetBrandBrickSet(brand);
    sets.PushBack(set);
    if (set == 0)
        sets.PushBack(2);
}

} // namespace LEGO
