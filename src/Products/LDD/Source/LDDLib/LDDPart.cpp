// LEGO Digital Designer — restored source
// LDDPart, LDDBrick, LDDBrickConnect (Mac TU 351, Win 0x653eef-0x654430). See LDDPart.h and
// LDDBrickConnect.h.
// Mac TU 351 also holds the BasicArray<LDDPart::FlexSurface> destructors (0x333340 / 0x3333a0,
// Win sdd 0x654271: template code) and the TU initialiser 0x3334e0 (iostream + six zeroed
// globals of a header, as in every TU that includes it).
#include "LDDPart.h"
#include "LDDBrickConnect.h"
#include "BrickConnectSceneManager.h"
#include "Material.h"

namespace LEGO {

// =============================================================================================
// LDDPart
// =============================================================================================

// Mac inlined into 0x332a20, Win 0x654293
LDDPart::LDDPart(const Primitive* primitive, BrickConnectSceneManager* sceneManager)
    : Part(primitive), m_sceneManager(sceneManager)
{
}

// Mac 0x332c40 / 0x332ca0, Win 0x654338 / 0x6543f5: member destructors only
LDDPart::~LDDPart()
{
}

// Mac 0x332680, Win 0x653f16
void LDDPart::OnMaterialsChanged(const BasicArray<PointGroup>& groups)
{
    Part::OnMaterialsChanged(groups);
    for (UInt32 i = 0; i < m_flexSurfaces.GetSize(); ++i) {
        FlexSurface& surface = m_flexSurfaces[i];
        if (surface.m_vertexBuffer.IsNull())
            continue;
        RefPointer<Material> material = m_sceneManager->GetMaterial(GetMaterial(i), 0);
        if (!material.IsNull())
            surface.m_mesh->SetMaterial(material);   // NOTE: no check for a missing mesh (both builds)
    }
}

// Mac 0x3327a0, Win 0x653fae
void LDDPart::OnDecorationsChanged(const BasicArray<UInt32>& faces)
{
    if (m_flexSurfaces.IsEmpty())
        return;
    for (BasicArray<UInt32>::ConstIterator it = faces.Begin(); it != faces.End(); ++it) {
        UInt32 face = *it;
        FlexSurface& surface = m_flexSurfaces[face];   // NOTE: no range check (both builds)
        if (GetDecoration(face) == 0) {
            surface.m_decalMesh = 0;
            continue;
        }
        RefPointer<Material> decal = m_sceneManager->GetDecalMaterial(GetDecoration(face));
        if (decal.IsNull())
            surface.m_decalMesh = 0;
        else
            surface.m_decalMesh = new SingleMesh(decal, surface.m_vertexBuffer);
    }
}

// =============================================================================================
// LDDBrick
// =============================================================================================

// Mac inlined into 0x332980, Win 0x65407a
LDDBrick::LDDBrick(UInt32 designID)
    : Brick(designID)
{
}

// Mac 0x332b60, Win 0x6540f6 (the design ID through the virtual GetDesignID)
LDDBrick::LDDBrick(const LDDBrick& source)
    : Object(), Brick(source.GetDesignID()), m_paletteBrick(source.m_paletteBrick)
{
}

// Mac 0x333190 / 0x333240, Win 0x65417a / 0x654247
LDDBrick::~LDDBrick()
{
}

// Mac 0x332620, Win 0x653ef4
void LDDBrick::SetPaletteBrick(const BrickPalette::Brick* brick)
{
    m_paletteBrick = brick;
    m_name = brick->GetName();
}

// =============================================================================================
// LDDBrickConnect
// =============================================================================================

// Mac 0x332980 (named ~LDDBrickConnect_deleting by the symbol spreading), Win 0x6541d7
Brick* LDDBrickConnect::CreateBrick(UInt32 designID)
{
    return new LDDBrick(designID);
}

// Mac 0x332a20, Win 0x6543b2
Part* LDDBrickConnect::CreatePart(const Primitive* primitive)
{
    return new LDDPart(primitive, m_sceneManager);
}

// Mac 0x332b10, Win 0x65420f
Brick* LDDBrickConnect::CloneBrick(const AbstractBrick* brick)
{
    return new LDDBrick(*static_cast<const LDDBrick*>(brick));
}

// Mac 0x332c30, Win 0x653eef
PrimitiveElement* LDDBrickConnect::ClonePrimitiveElement(const PrimitiveElement* element)
{
    return BrickConnect::ClonePrimitiveElement(element);
}

} // namespace LEGO
