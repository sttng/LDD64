// LEGO Digital Designer — restored source
// LEGO::Material — abstract render state applied around a draw (implementations: Materials.cpp
// TU 043, BrickConnectSceneManager 022b, RenderGrid 117, tools). Header-only, D1/D0 in TU 022b
// (Mac 0x62f20 / 0x62f40). Vtables Mac 0x709e5c {4 pure, D1, D0}, Win 0x9ce900 {4 pure}, same
// order. Layout: Mac {vptr, Object +0x04}; Win {vptr, vbptr, Object +0x08}. Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"

namespace LEGO {

class Render;

class Material : public virtual Object {
public:
    // Opaque materials go to the opaque render queue list (RenderQueue::Add Mac 0x1032e0):
    // ColoredMaterial: diffuse alpha == 1 (Mac 0x109b20); ShadedMaterial: its shader's.
    virtual bool IsOpaque() const = 0;                                // 0
    // Decals are queued after the opaque geometry: true for DecalMaterial (Mac 0x10a250),
    // ShadedDecalMaterial (0x63af0), ShadedRoundEdgeDecalMaterial,
    // GridTexturedSemitransparentMaterial.
    virtual bool IsDecal() const = 0;                                 // 1
    // Called by Render::SetMaterial on the new material (ColoredMaterial Mac 0x106c40:
    // EnableLighting(true), SetLocalViewer(true), SetMaterialColors(m_colors)).
    virtual void Begin(Render* render) = 0;                           // 2
    // Called on the previous material (ColoredMaterial Mac 0x106c80 undoes Begin).
    virtual void End(Render* render) = 0;                             // 3

    virtual ~Material() {}                                            // Mac 4/5 0x62f20/0x62f40
};

} // namespace LEGO
