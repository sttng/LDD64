// LEGO Digital Designer — restored source
// LEGO::Mesh (material + draw callbacks, queued with a transform) and SingleMesh (one
// VertexBuffer). Header-only; inlines emitted in TU 021a/021d:
//   Mesh       Mac vtable 0x76051c {0x529f0, 0x52a10, 0x52a40, 0x52a80, 0x52a90, D1 0x52aa0, D0 0x52b60}
//              Win 0x9cead0 {0x57c3c0, 0x57c410, 0x57c460, 0x559e90, 0x559e90}
//   SingleMesh Mac vtable 0x767acc {.., 0x52c00, 0x52c50, 0x52c70, D1 0x52c90, D0 0x52cf0}
//              Win 0x9ceafc {0x57c3c0, 0x57c410, 0x57c710, 0x57c760, 0x57c7a0}
// SelectableMesh : SingleMesh is in LDDLib/SelectableMesh.h (TU 063). Same slot order in both builds.
// Layout: Mesh Mac {vptr, RefPointer<Material> +0x04, Object +0x08} (Win: vptr, vbptr,
// material +0x08, Object +0x0c); SingleMesh adds RefPointer<VertexBuffer> (Mac +0x08, Win +0x0c).
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Render.h"
#include "Material.h"
#include "VertexBuffer.h"

namespace LEGO {

class Mesh : public virtual Object {
public:
    Mesh() {}
    explicit Mesh(Material* material) : m_material(material) {}

    // Opaque unless the material says otherwise.                    Mac 0x529f0, Win 0x57c3c0
    virtual bool IsOpaque() const { return m_material.IsNull() ? true : m_material->IsOpaque(); }
    //                                                               Mac 0x52a10, Win 0x57c410
    virtual bool IsDecal() const { return m_material.IsNull() ? false : m_material->IsDecal(); }
    // Prepares the draw: the mesh's material (or the render's default material).
    // `depth` is the queue entry's sort key (RenderEntry::depth; the Win queue passes it with
    // fld/fstp, Win 0x4f3a84); not read by Mesh/SingleMesh.        Mac 0x52a40, Win 0x57c460
    virtual void Bind(Render* render, Real32 depth, bool useMaterial)
    {
        (void)depth;
        if (useMaterial)
            render->SetMaterial(m_material);
        else
            render->UseDefaultMaterial();
    }
    // Around the draw call (empty for Mesh).                        Mac 0x52a80 / 0x52a90, Win 0x559e90 (ICF)
    virtual void BeginDraw(Render* render) { (void)render; }
    virtual void EndDraw(Render* render) { (void)render; }

    virtual ~Mesh() {}                                                // Mac 0x52aa0/0x52b60

    Material* GetMaterial() const { return m_material; }
    void SetMaterial(Material* material) { m_material = material; }

protected:
    RefPointer<Material> m_material;   // Mac +0x04, Win +0x08
};

class SingleMesh : public Mesh {
public:
    SingleMesh() {}
    SingleMesh(Material* material, VertexBuffer* buffer) : Mesh(material), m_vertexBuffer(buffer) {}

    //                                                               Mac 0x52c00, Win 0x57c710
    virtual void Bind(Render* render, Real32 depth, bool useMaterial)
    {
        render->SetVertexBuffer(m_vertexBuffer);
        Mesh::Bind(render, depth, useMaterial);
    }
    virtual void BeginDraw(Render* render) { render->BeginVertexBuffer(m_vertexBuffer); } // Mac 0x52c50, Win 0x57c760
    virtual void EndDraw(Render* render) { render->EndVertexBuffer(m_vertexBuffer); }     // Mac 0x52c70, Win 0x57c7a0

    virtual ~SingleMesh() {}                                          // Mac 0x52c90/0x52cf0

    VertexBuffer* GetVertexBuffer() const { return m_vertexBuffer; }

protected:
    RefPointer<VertexBuffer> m_vertexBuffer;   // Mac +0x08, Win +0x0c
};

} // namespace LEGO
