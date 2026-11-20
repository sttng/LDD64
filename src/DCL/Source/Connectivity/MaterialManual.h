// LEGO Digital Designer — restored source
// LEGO::MaterialManual — the brick colours ("materials") known to the brick database: id ->
// (RGBA colour, material type), loaded from db.lif Materials.xml, or from the built-in table
// when that file is missing; plus the set of ids currently in production
// (CurrentMaterials.xml) and the fixed display order of the colour palette.
//
// Compiled as part of BrickDatabase.cpp (Mac TU 024, Win 0x4b29e0-0x4bfde0, one object each);
// restored as its own file. Pieces:
//   sub-TU 024a  Mac 0x71fc0-0x7c6b0: OrderedMaterialLess, GetMaterialOrder, the
//                GetXMLAttribute instances (XMLDOMAttribute.h) and InitDefaultMaterials (the
//                41 KB straight-line table, generated into MaterialManualTable.inc from
//                the brick database)
//   sub-TU 024b  Mac 0x7c710 constructor, 0x7d2c0 GetMaterialColor, 0x7d410 FromString
//   sub-TU 024d  query functions, inlined into BrickDatabase on Mac (0x81dc0, 0x81e30,
//                0x81ea0, 0x81f40), out of line on Win (0x4b3be0, 0x4b3c60, 0x4b3cb0)
//   sub-TU 024f  implicit destructor (Mac 0x88860, D0 0x88920, thunks 0x88960 / 0x88980)
// Names from RTTI: MaterialManual, MaterialManual::MaterialType, MaterialManual::
// OrderedMaterialLess, BasicMap<UInt32, Pair<Color, MaterialType>>, BasicSet<UInt32,
// OrderedMaterialLess>, MaterialColor (a BasicMap key of the render layer). The MaterialType
// keywords come from FromString. Every other name is inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicMap.h"
#include "Color.h"
#include "LEGO/String.h"

namespace LEGO {

class Directory;

// Fixed-function material colours of one brick colour (RTTI name MaterialColor: key of
// BasicMap<MaterialColor, RefPointer<Material>> in the render layer; layout from
// MaterialManual::GetMaterialColor, member names inferred). sizeof 0x44.
// NOTE: a render-layer type in the original; kept here (the restored render layer does not use it).
struct MaterialColor {
    Color  diffuse;     // +0x00
    Color  ambient;     // +0x10
    Color  specular;    // +0x20
    Color  emission;    // +0x30
    Real32 shininess;   // +0x40
};

// RTTI: class LEGO::MaterialManual : virtual Object; no virtual functions of its own.
// Mac sizeof 0x44: vptr +0 (vtable 0x761c4c {D1 0x88860, D0 0x88920}), Object at +0x3c.
// Win sizeof 0x54: vbptr +0, Object at +0x4c (vftable 0xa24f44 {sdd 0x4b29e0, Object slots};
// destructor body 0x4b2970).
class MaterialManual : public virtual Object {
public:
    // Order of the keywords in FromString (Mac 0x7d410).
    enum MaterialType {
        kShinyPlastic = 0,   // "shinyPlastic"
        kMattePlastic = 1,   // "mattePlastic"
        kShinySteel   = 2,   // "shinySteel"
        kBrushedSteel = 3,   // "brushedSteel"
        kGlitter      = 4,   // "glitter"
        kMatteSteel   = 5    // "matteSteel"
    };
    typedef Pair<Color, MaterialType>  Material;
    typedef BasicMap<UInt32, Material> MaterialMap;   // Mac vtable 0x762068, Win dtor 0x4bfde0
    typedef BasicSet<UInt32>           MaterialSet;   // Mac vtable 0x7703a8

    // Sorts ids by GetMaterialOrder (palette order; unknown ids after all known ones).
    struct OrderedMaterialLess {
        bool operator()(const UInt32& a, const UInt32& b) const;     // Mac 0x71fc0, Win 0x4b3430 (thiscall, ret 8)
    };

    // Reads <directory>/Materials.xml (InitDefaultMaterials when it cannot be opened) and
    // <directory>/CurrentMaterials.xml (optional).
    explicit MaterialManual(Directory* directory);                    // Mac 0x7c710, Win 0x4b3460 (this@stack)

    // Position of `id` in the palette order (0..170); unknown ids map to 171 + id.
    static UInt32 GetMaterialOrder(UInt32 id);                        // Mac 0x71ff0, Win 0x4b3d60 (cdecl)
    // The built-in table (168 materials, MaterialManualTable.inc).
    // NOTE: Mac passes the map as the 2nd stack argument and leaves the 1st unset; Win (/LTCG)
    // takes it in ECX. Modelled as a one-argument static function.
    static void InitDefaultMaterials(MaterialMap& materials);         // Mac 0x72350, Win 0x4b3e10

    // diffuse = colour, ambient = rgb * 0.1 (alpha 1), specular white, emission (0,0,0,0),
    // shininess 100. false for an unknown id (`out` unchanged).
    bool GetMaterialColor(UInt32 id, MaterialColor& out) const;       // Mac 0x7d2c0, Win 0x4b3a40 (EAX this, ESI out, ret 4)
    bool GetMaterialType(UInt32 id, MaterialType& type) const;        // Mac inlined 0x81dc0, Win 0x4b3be0 (EAX this, ret 8)
    bool HasMaterial(UInt32 id) const;                                // Mac inlined 0x81e30, Win 0x4b3c60 (EAX this, ret 4)
    // alpha < 1 by more than 1e-5.
    bool IsTransparent(UInt32 id) const;                              // Mac inlined 0x81ea0, Win 0x4b3cb0 (EAX this, ret 4)
    // CurrentMaterials.xml was loaded (non-empty) and does not list `id`.
    bool IsObsolete(UInt32 id) const;                                 // Mac inlined 0x81f40 (Win: inlined)

    const MaterialMap& GetMaterials() const        { return m_materials; }        // (accessor)
    const MaterialSet& GetCurrentMaterials() const { return m_currentMaterials; } // (accessor)

private:
    MaterialMap m_materials;          // Mac +0x04, Win +0x04
    MaterialSet m_currentMaterials;   // Mac +0x20, Win +0x28
};

// "shinyPlastic" … "matteSteel" (case-insensitive keyword followed by ',', whitespace or the
// end, see ParseKeyword) at `position`; advances past it.
bool FromString(MaterialManual::MaterialType& value, const String& text, UInt32& position); // Mac 0x7d410, Win 0x4b77e0 (EDI text, ESI position, cdecl out)
// Inlined into GetXMLAttribute<MaterialType> (Mac 0x72260).
inline bool FromString(MaterialManual::MaterialType& value, const String& text)
{
    UInt32 position = 0;
    return FromString(value, text, position);
}

} // namespace LEGO
