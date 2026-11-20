// LEGO Digital Designer — restored source
// LEGO::MaterialManual — brick colour table (Materials.xml / built-in table, CurrentMaterials.xml,
// palette order). Part of the original BrickDatabase.cpp (Mac TU 024: sub-TU 024a 0x71fc0-0x7c6b0
// and pieces of 024b/024d/024f; Win 0x4b2970-0x4b77e0 + 0x4b77e0 FromString). See MaterialManual.h.
#include "MaterialManual.h"
#include "XMLDOMAttribute.h"
#include "XMLDOMNode.h"
#include "Directory.h"
#include "File.h"
#include "MathUtil.h"
#include "LEGO/StdLib.h"
#include "StringStream.h"

namespace LEGO {

namespace {

// The original builds the message and drops it (reporting compiled out in both builds).
inline void DebugMessage(const String&) {}

// AString == C string as the inlined comparisons do it (StrNCmp on the common length, then the
// lengths; Win calls strncmp). Same helper as in XMLDOMNode.cpp.
bool EqualsAString(const AString& a, const char* b)
{
    UInt32 bLength = StrLen(b);
    UInt32 aLength = a.GetLength();
    if (aLength == 0)
        return bLength == 0;
    UInt32 n = aLength < bLength ? aLength : bLength;
    return StrNCmp(a.GetBuffer(), b, n) == 0 && aLength == bLength;
}

// Palette order: Mac 0x62f2c0, Win 0xae7e08 (171 ids, 0-terminated).
// NOTE: Win searches up to the terminator; Mac (clang) unrolled the same search into a loop of
// exactly 171 compares. Same results (no id is 0).
const UInt32 s_materialOrder[] = {
#define LEGO_MATERIAL_ORDER(id) id,
#include "MaterialManualTable.inc"
#undef LEGO_MATERIAL_ORDER
    0
};

} // namespace

// ---------------------------------------------------------------------------
// sub-TU 024a
// ---------------------------------------------------------------------------

// Mac 0x71fc0, Win 0x4b3430
bool MaterialManual::OrderedMaterialLess::operator()(const UInt32& a, const UInt32& b) const
{
    return GetMaterialOrder(a) < GetMaterialOrder(b);
}

// Mac 0x71ff0, Win 0x4b3d60
UInt32 MaterialManual::GetMaterialOrder(UInt32 id)
{
    UInt32 i = 0;
    for (; s_materialOrder[i] != 0; ++i) {
        if (s_materialOrder[i] == id)
            return i;
    }
    String message;
    message << "Material is missing from list: " << id;
    DebugMessage(message);
    return i + id;
}

// Mac 0x72350 (168 blocks of Color::FromBytes + map operator[] + assignment), Win 0x4b3e10
// (/LTCG: colours folded to float constants, later blocks call Color::FromBytes 0x41faa0).
void MaterialManual::InitDefaultMaterials(MaterialMap& materials)
{
#define LEGO_MATERIAL(id, r, g, b, a, type) \
    materials[id] = Material(Color::FromBytes(r, g, b, a), type);
#include "MaterialManualTable.inc"
#undef LEGO_MATERIAL
}

// ---------------------------------------------------------------------------
// sub-TU 024b
// ---------------------------------------------------------------------------

// Mac 0x7c710, Win 0x4b3460
MaterialManual::MaterialManual(Directory* directory)
{
    const OpenMode readShared(true, false, true, false);
    RefPointer<File> materials = directory->OpenFile(String("Materials.xml"), readShared, false, 0);
    if (materials.IsNull()) {
        InitDefaultMaterials(m_materials);
    } else {
        RefPointer<XMLDOMAString> xml = new XMLDOMAString;
        if (xml->Parse(materials, 0)) {
            const XMLDOMNodeAString::NodeArray& children = xml->GetChildren();
            for (XMLDOMNodeAString::NodeArray::ConstIterator it = children.Begin(); it != children.End(); ++it) {
                const XMLDOMNodeAString& node = **it;
                if (!EqualsAString(node.GetName(), "Material"))
                    continue;
                UInt32 id;
                if (!GetXMLAttribute(node, AString("MatID"), id))
                    continue;
                UInt8 r = 0, g = 0, b = 0, a = 0xff;
                MaterialType type = kShinyPlastic;
                GetXMLAttribute(node, AString("Red"), r);
                GetXMLAttribute(node, AString("Green"), g);
                GetXMLAttribute(node, AString("Blue"), b);
                GetXMLAttribute(node, AString("Alpha"), a);
                GetXMLAttribute(node, AString("MaterialType"), type);
                m_materials[id] = Material(Color::FromBytes(r, g, b, a), type);
            }
        }
    }

    RefPointer<File> current = directory->OpenFile(String("CurrentMaterials.xml"), readShared, false, 0);
    if (!current.IsNull()) {
        RefPointer<XMLDOMAString> xml = new XMLDOMAString;
        if (xml->Parse(current, 0)) {
            const XMLDOMNodeAString::NodeArray& children = xml->GetChildren();
            for (XMLDOMNodeAString::NodeArray::ConstIterator it = children.Begin(); it != children.End(); ++it) {
                const XMLDOMNodeAString& node = **it;
                if (!EqualsAString(node.GetName(), "Material"))
                    continue;
                UInt32 id;
                if (GetXMLAttribute(node, AString("MatID"), id))
                    m_currentMaterials.Insert(id);
            }
        }
    }
}

// Mac 0x7d2c0, Win 0x4b3a40
bool MaterialManual::GetMaterialColor(UInt32 id, MaterialColor& out) const
{
    MaterialMap::ConstIterator it = m_materials.Find(id);
    if (it == m_materials.End())
        return false;
    const Color& c = it->second.first;
    out.diffuse = c;
    out.ambient = Color(c.r * 0.1f, c.g * 0.1f, c.b * 0.1f, 1.0f);
    out.specular = Color(1.0f, 1.0f, 1.0f, 1.0f);
    out.emission = Color(0.0f, 0.0f, 0.0f, 0.0f);
    out.shininess = 100.0f;
    return true;
}

// Mac 0x7d410, Win 0x4b77e0
bool FromString(MaterialManual::MaterialType& value, const String& text, UInt32& position)
{
    if (ParseKeyword(String("shinyPlastic"), text, position, ','))
        value = MaterialManual::kShinyPlastic;
    else if (ParseKeyword(String("mattePlastic"), text, position, ','))
        value = MaterialManual::kMattePlastic;
    else if (ParseKeyword(String("shinySteel"), text, position, ','))
        value = MaterialManual::kShinySteel;
    else if (ParseKeyword(String("brushedSteel"), text, position, ','))
        value = MaterialManual::kBrushedSteel;
    else if (ParseKeyword(String("glitter"), text, position, ','))
        value = MaterialManual::kGlitter;
    else if (ParseKeyword(String("matteSteel"), text, position, ','))
        value = MaterialManual::kMatteSteel;
    else
        return false;
    return true;
}

// ---------------------------------------------------------------------------
// sub-TU 024d (inlined into BrickDatabase on Mac)
// ---------------------------------------------------------------------------

// Mac inlined in 0x81dc0, Win 0x4b3be0
bool MaterialManual::GetMaterialType(UInt32 id, MaterialType& type) const
{
    MaterialMap::ConstIterator it = m_materials.Find(id);
    if (it == m_materials.End())
        return false;
    type = it->second.second;
    return true;
}

// Mac inlined in 0x81e30, Win 0x4b3c60
bool MaterialManual::HasMaterial(UInt32 id) const
{
    return m_materials.Find(id) != m_materials.End();
}

// Mac inlined in 0x81ea0, Win 0x4b3cb0
bool MaterialManual::IsTransparent(UInt32 id) const
{
    MaterialMap::ConstIterator it = m_materials.Find(id);
    if (it == m_materials.End())
        return false;
    Real32 d = it->second.first.a - 1.0f;
    return d < 0.0f && Abs(d) > 1e-5f;
}

// Mac inlined in 0x81f40 (Win: inlined)
bool MaterialManual::IsObsolete(UInt32 id) const
{
    if (m_currentMaterials.IsEmpty())
        return false;
    return !m_currentMaterials.Contains(id);
}

} // namespace LEGO
