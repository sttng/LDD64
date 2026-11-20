// LEGO Digital Designer — restored source
// The render layer's Material implementations (fixed-function, GLSL "Shaded*" / SceneNormals*,
// helpers Silhouette / Outline / CoordinateSystem) and three vertex-buffer builders. File name
// inferred. Mac TU 043 0x106320-0x10af6f + inlines in TU 022b (0x62f20..,
// 0x62ff0-0x634a0, ShadedMaterial ctor 0x63cb0 / dtor 0x63900, ShadedTextured ctor 0x63550,
// ShadedDecal 0x63ae0..). Win 0x5b9010-0x5bb630 + 0x5bd7d0-0x5bda60 (+ ICF'd one-liners
// 0x57c9a0, 0x57ba30, 0x5fbd39, 0x67e76a, 0x4aede0, 0x4f5b70, 0x6123e8, 0x60a87d).
//
// Render slots used: 12 SetCullMode, 13 SetPolygonMode (1 lines, 2 fill), 14 SetFrontFace,
// 15 SetLineWidth, 17 SetPolygonOffset, 23 EnableLighting, 24 SetLocalViewer, 29
// EnableDepthWrite, 30 EnablePolygonOffset, 46 SetMaterialColors, 47 GetMaterialColors, 49
// GetOpacity, 51 SetTexture, 52 GetTexture, 53 SetShader, 64 SetColor.
// Shader uniforms: DiffuseColor (opacity-scaled diffuse), Texture0-2 / VertexTexture /
// PackedAverageNormalTexture / PackedRoundEdgeDataTexture sampler units, vertexNormalTexelOffset
// and packedRedTexelOffset (1 / width of the round-edge textures on units 4 and 6).
//
// Layouts (Mac; Win adds a vbptr after the vptr, so every member moves by +4):
//   ColoredMaterial / FilledMaterial / WireframeMaterial  {vptr, MaterialColors +0x04} Object +0x48
//   TexturedMaterial (+Decal/ReverseWinding/Skybox)       + RefPointer<Texture> +0x48, Object +0x4c
//   ShadedMaterial (+ColorChange, RE Wireframe)          {vptr, RefPointer<Shader> +0x04,
//                                                         MaterialColors +0x08, Real32 opacity +0x4c} Object +0x50
//   ShadedTexturedMaterial / ShadedDecalMaterial          + RefPointer<Texture> +0x50, Object +0x54
//   ShadedRoundEdgeDecalMaterial                          + RefPointer<Texture> +0x50, Object +0x54
//   ShadedRoundEdgeMaterial (+ColorChange)                + 3 RefPointer<Texture> +0x50..+0x58, Object +0x5c
//   SceneNormalsMaterial (+RoundEdge)                     {vptr, RefPointer<Shader> +0x04} Object +0x08
//   Silhouette / Outline / CoordinateSystem               {vptr} Object +0x04
// Class names from RTTI; member and method names inferred. Constructors of the fixed-function
// materials are inline in their users (none in this TU); their signatures are inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Color.h"
#include "Render.h"
#include "Material.h"
#include "Shader.h"
#include "Texture.h"
#include "VertexBuffer.h"
#include "Mesh.h"

namespace LEGO {

class Directory;

// ===========================================================================================
// Fixed-function materials
// ===========================================================================================

// Lit material colour. Mac vtable 0x709e5c-family, D1 0x109b60, D0 0x109b80.
class ColoredMaterial : public Material {
public:
    explicit ColoredMaterial(const MaterialColors& colors) : m_colors(colors) {}

    virtual bool IsOpaque() const;                                    // Mac 0x109b20, Win 0x57c9a0
    virtual bool IsDecal() const;                                     // Mac 0x109b50, Win 0x57ba30
    virtual void Begin(Render* render);                               // Mac 0x106c40, Win 0x5ba120
    virtual void End(Render* render);                                 // Mac 0x106c80, Win 0x5ba160

    const MaterialColors& GetColors() const { return m_colors; }

protected:
    MaterialColors m_colors;   // Mac +0x04, Win +0x08
};

// Unlit, no face culling. Mac D1 0x109c70, D0 0x109c90.
class FilledMaterial : public Material {
public:
    explicit FilledMaterial(const MaterialColors& colors) : m_colors(colors) {}

    virtual bool IsOpaque() const;                                    // Mac 0x109c30, Win 0x6123e8
    virtual bool IsDecal() const;                                     // Mac 0x109c60, Win 0x67e76a
    virtual void Begin(Render* render);                               // Mac 0x106cb0, Win 0x5ba180
    virtual void End(Render* render);                                 // Mac 0x106cf0, Win 0x5ba1c0

protected:
    MaterialColors m_colors;   // Mac +0x04, Win +0x08
};

// Lit lines, pulled towards the viewer by a polygon offset. Mac D1 0x109d40, D0 0x109d60.
class WireframeMaterial : public ColoredMaterial {
public:
    explicit WireframeMaterial(const MaterialColors& colors) : ColoredMaterial(colors) {}

    virtual void Begin(Render* render);                               // Mac 0x106d10, Win 0x5ba1e0
    virtual void End(Render* render);                                 // Mac 0x106d80, Win 0x5ba230
};

// Lit, one texture on unit 0. Mac D1 0x109e50, D0 0x109ec0.
class TexturedMaterial : public ColoredMaterial {
public:
    TexturedMaterial(const MaterialColors& colors, Texture* texture)
        : ColoredMaterial(colors), m_texture(texture) {}

    // Opaque when the texture has no alpha (GetFormat() == 0) and the diffuse alpha is 1.
    virtual bool IsOpaque() const;                                    // Mac 0x109e10, Win 0x5bd890
    virtual void Begin(Render* render);                               // Mac 0x106dd0, Win 0x5ba260
    virtual void End(Render* render);                                 // Mac 0x106e30, Win 0x5ba290

    Texture* GetTexture() const { return m_texture; }

protected:
    RefPointer<Texture> m_texture;   // Mac +0x48, Win +0x4c
};

// TexturedMaterial with clockwise front faces. Mac D1 0x10a050, D0 0x10a0c0.
class ReverseWindingTexturedMaterial : public TexturedMaterial {
public:
    ReverseWindingTexturedMaterial(const MaterialColors& colors, Texture* texture)
        : TexturedMaterial(colors, texture) {}

    virtual void Begin(Render* render);                               // Mac 0x106e70, Win 0x5ba2c0
    virtual void End(Render* render);                                 // Mac 0x106ed0, Win 0x5ba2e0
};

// TexturedMaterial drawn after the opaque geometry with a polygon offset. Mac D1 0x10a260,
// D0 0x10a2d0 (Win sdd 0x68732e).
class DecalMaterial : public TexturedMaterial {
public:
    DecalMaterial(const MaterialColors& colors, Texture* texture)
        : TexturedMaterial(colors, texture) {}

    virtual bool IsDecal() const;                                     // Mac 0x10a250, Win 0x5fbd39
    virtual void Begin(Render* render);                               // Mac 0x106f20, Win 0x5ba300
    virtual void End(Render* render);                                 // Mac 0x106fa0, Win 0x5ba340
};

// Skybox faces: no depth writes. Mac D1 0x10a460, D0 0x10a4d0.
class TexturedSkyboxMaterial : public TexturedMaterial {
public:
    TexturedSkyboxMaterial(const MaterialColors& colors, Texture* texture)
        : TexturedMaterial(colors, texture) {}

    virtual void Begin(Render* render);                               // Mac 0x106ff0, Win 0x5ba360
    virtual void End(Render* render);                                 // Mac 0x107060, Win 0x5ba390
};

// ===========================================================================================
// Shader materials
// ===========================================================================================

// A shader with its material colours (uploaded once by the constructor; the diffuse alpha is
// re-sent scaled by the render's opacity whenever that changed). Mac D1 0x63900, D0 0x63980
// (TU 022b); Win ctor 0x5b9060, dtor 0x5b9150, sdd 0x5b91d0.
class ShadedMaterial : public Material {
public:
    // Mac 0x63cb0 (TU 022b), Win 0x5b9060
    ShadedMaterial(Render* render, Shader* shader, const MaterialColors& colors);

    virtual bool IsOpaque() const;                                    // Mac 0x638e0, Win 0x5b9200 (shader's)
    virtual bool IsDecal() const;                                     // Mac 0x63720, Win 0x4aede0
    virtual void Begin(Render* render);                               // Mac 0x1070c0, Win 0x5ba3c0
    virtual void End(Render* render);                                 // Mac 0x107250, Win 0x5bb470

    Shader* GetShader() const { return m_shader; }
    const MaterialColors& GetColors() const { return m_colors; }

protected:
    RefPointer<Shader> m_shader;    // Mac +0x04, Win +0x08
    MaterialColors     m_colors;    // Mac +0x08, Win +0x0c
    Real32             m_opacity;   // Mac +0x4c, Win +0x50 (render opacity of the last upload)
};

// ShadedMaterial with a texture on unit 0 ("Texture0"). Mac D1 0x63730, D0 0x63790 (022b).
class ShadedTexturedMaterial : public ShadedMaterial {
public:
    // Mac 0x63550 (TU 022b)
    ShadedTexturedMaterial(Render* render, Shader* shader, const MaterialColors& colors,
                           Texture* texture);

    virtual bool IsOpaque() const;                                    // Mac 0x63700, Win 0x60a87d
    virtual void Begin(Render* render);                               // Mac 0x107530, Win 0x5ba640
    virtual void End(Render* render);                                 // Mac 0x107650, Win 0x5ba6e0

protected:
    RefPointer<Texture> m_texture;   // Mac +0x50, Win +0x54
};

// Shaded decal (queued after the opaque geometry). Mac D1 0x63b00, D0 0x63b60 (022b).
class ShadedDecalMaterial : public ShadedTexturedMaterial {
public:
    ShadedDecalMaterial(Render* render, Shader* shader, const MaterialColors& colors,
                        Texture* texture)
        : ShadedTexturedMaterial(render, shader, colors, texture) {}

    virtual bool IsOpaque() const;                                    // Mac 0x63ae0, Win 0x67e76a
    virtual bool IsDecal() const;                                     // Mac 0x63af0, Win 0x5fbd39
};

// Takes the colours from the render (Render::GetMaterialColors) at every Begin.
// Mac D1 0x107330, D0 0x107420; Win ctor 0x5ba4c0, dtor 0x5ba5e0, sdd 0x5ba570.
class ShadedColorChangeMaterial : public ShadedMaterial {
public:
    // Mac 0x107270, Win 0x5ba4c0
    ShadedColorChangeMaterial(Render* render, Shader* shader, const MaterialColors& colors)
        : ShadedMaterial(render, shader, colors) {}

    virtual void Begin(Render* render);                               // Mac 0x1072f0, Win 0x5ba5a0
};

// Round-edge brick shader: three lookup textures (blue.png, red.png, magenta.png from the
// shader directory) on units 1-3 and the round-edge data textures of the vertex buffer on
// units 4-6 (bound by OpenGLVertexBufferVBORoundEdge::Begin).
// Mac D1 0x10a680, D0 0x10a6e0 (D2 0x10ae10); Win dtor 0x5b9210, sdd 0x5b92d0.
class ShadedRoundEdgeMaterial : public ShadedMaterial {
public:
    // Mac 0x107f30 (C1) / 0x107680 (C2), Win 0x5ba710
    ShadedRoundEdgeMaterial(Render* render, Shader* shader, const MaterialColors& colors,
                            Directory* shaderDirectory);

    virtual bool IsOpaque() const;                                    // Mac 0x10a660, Win 0x5b9200
    // With textures on units 4 and 6: lookup textures on 1-3 and the two texel offsets.
    virtual void Begin(Render* render);                               // Mac 0x108600, Win 0x5baa40
    virtual void End(Render* render);                                 // Mac 0x1088d0, Win 0x5babc0

protected:
    RefPointer<Texture> m_texture1;   // Mac +0x50, Win +0x54 blue.png
    RefPointer<Texture> m_texture2;   // Mac +0x54, Win +0x58 red.png
    RefPointer<Texture> m_texture3;   // Mac +0x58, Win +0x5c magenta.png
};

// Mac D1 0x10a830, D0 0x10a890; Win ctor 0x5bad10, dtor 0x5b9300, sdd 0x5b9360.
class ShadedRoundEdgeColorChangeMaterial : public ShadedRoundEdgeMaterial {
public:
    // Mac 0x108930, Win 0x5bad10
    ShadedRoundEdgeColorChangeMaterial(Render* render, Shader* shader,
                                       const MaterialColors& colors, Directory* shaderDirectory)
        : ShadedRoundEdgeMaterial(render, shader, colors, shaderDirectory) {}

    virtual void Begin(Render* render);                               // Mac 0x1089c0, Win 0x5badc0
};

// Decal on a round-edge brick (RE decal shader). Mac D1 0x109060, D0 0x109120 (D2 0x108fb0);
// Win dtor 0x5bb0b0, sdd 0x5baf90.
class ShadedRoundEdgeDecalMaterial : public ShadedMaterial {
public:
    // Mac 0x108a00, Win 0x5bae00
    ShadedRoundEdgeDecalMaterial(Render* render, Shader* shader, const MaterialColors& colors,
                                 Texture* decal);

    virtual bool IsOpaque() const;                                    // Mac 0x108dd0, Win 0x5bafc0 (decal has no alpha)
    virtual bool IsDecal() const;                                     // Mac 0x108df0, Win 0x4f5b70
    virtual void Begin(Render* render);                               // Mac 0x108e00, Win 0x5bafd0
    virtual void End(Render* render);                                 // Mac 0x108f80, Win 0x5ba6e0

protected:
    RefPointer<Texture> m_decal;   // Mac +0x50, Win +0x54
};

// Round-edge geometry drawn as lines. Mac D1 0x109540, D0 0x109630; Win ctor 0x5bb130,
// dtor 0x5bb340, sdd 0x5bb220.
class ShadedRoundEdgeWireframeMaterial : public ShadedMaterial {
public:
    // Mac 0x109210, Win 0x5bb130
    ShadedRoundEdgeWireframeMaterial(Render* render, Shader* shader, const MaterialColors& colors);

    virtual void Begin(Render* render);                               // Mac 0x1093a0, Win 0x5bb250
    virtual void End(Render* render);                                 // Mac 0x109510, Win 0x5bb320
};

// Shader only (no colours): the scene-normals pass of the post effects.
// Mac D1 0x10aa10, D0 0x10aa80; Win ctor 0x5bb3a0, dtor 0x5b9390, sdd 0x5b9410.
class SceneNormalsMaterial : public Material {
public:
    // Mac 0x109740, Win 0x5bb3a0 (the render is not used)
    SceneNormalsMaterial(Render* render, Shader* shader);

    virtual bool IsOpaque() const;                                    // Mac 0x10a9e0, Win 0x5b9200
    virtual bool IsDecal() const;                                     // Mac 0x10aa00, Win 0x4aede0
    virtual void Begin(Render* render);                               // Mac 0x1097c0, Win 0x5bb450
    virtual void End(Render* render);                                 // Mac 0x1097e0, Win 0x5bb470

protected:
    RefPointer<Shader> m_shader;   // Mac +0x04, Win +0x08
};

// Mac D1 0x10ac10, D0 0x10ac80; Win dtor 0x5b9440, sdd 0x5b94a0.
class SceneNormalsRoundEdgeMaterial : public SceneNormalsMaterial {
public:
    // Mac 0x109800, Win 0x5bb490
    SceneNormalsRoundEdgeMaterial(Render* render, Shader* shader);

    virtual void Begin(Render* render);                               // Mac 0x109990, Win 0x5bb560
    virtual void End(Render* render);                                 // Mac 0x109b00, Win 0x5bb630
};

// ===========================================================================================
// Helper materials (inline in TU 022b)
// ===========================================================================================

// Black 2-pixel outline of the back faces' edges (selection silhouette).
// Mac D1 0x630c0, D0 0x630e0.
class SilhouetteMaterial : public Material {
public:
    virtual bool IsOpaque() const;                                    // Mac 0x62ff0, Win 0x5fbd39
    virtual bool IsDecal() const;                                     // Mac 0x63000, Win 0x67e76a
    virtual void Begin(Render* render);                               // Mac 0x63010, Win 0x609782
    virtual void End(Render* render);                                 // Mac 0x63080, Win 0x6097d0
};

// Unlit black. Mac D1 0x63220, D0 0x63240.
class OutlineMaterial : public Material {
public:
    virtual bool IsOpaque() const;                                    // Mac 0x63190, Win 0x5fbd39
    virtual bool IsDecal() const;                                     // Mac 0x631a0, Win 0x67e76a
    virtual void Begin(Render* render);                               // Mac 0x631b0, Win 0x60999a
    virtual void End(Render* render);                                 // Mac 0x63200, Win 0x6099d6
};

// Unlit 2-pixel lines (vertex colours). Mac D1 0x63480, D0 0x634a0.
class CoordinateSystemMaterial : public Material {
public:
    virtual bool IsOpaque() const;                                    // Mac 0x63400, Win 0x5fbd39
    virtual bool IsDecal() const;                                     // Mac 0x63410, Win 0x67e76a
    virtual void Begin(Render* render);                               // Mac 0x63420, Win 0x6098ac
    virtual void End(Render* render);                                 // Mac 0x63450, Win 0x6098c7
};

// ===========================================================================================
// Vertex-buffer builders (TU 043; names inferred). The buffers come from the render's
// ObjectFactory (null when it cannot create one) and are not referenced (refcount 0).
// ===========================================================================================

// 12 edges of the unit cube [0,1]^3 as lines (8 vertices, 24 indices), one colour.
VertexBuffer* CreateBoxVertexBuffer(Render* render, const Color& color, bool dynamic);   // Mac 0x106320, Win 0x5b94d0
SingleMesh*   CreateBoxMesh(Render* render, const Color& color, Material* material,
                            bool dynamic);                                               // Mac 0x1065a0, Win 0x5b9920
// One triangle (1,0,0), (-1/2,0,√3/2), (-1/2,0,-√3/2) as a primitive of type 4, one colour.
VertexBuffer* CreateTriangleVertexBuffer(Render* render, const Color& color);           // Mac 0x106610, Win 0x5b99c0
SingleMesh*   CreateTriangleMesh(Render* render, const Color& color, Material* material); // Mac 0x1068a0, Win 0x5b9cb0
// The three unit axes as lines from the origin, coloured x, y, z.
VertexBuffer* CreateAxesVertexBuffer(Render* render, const Color& x, const Color& y,
                                     const Color& z, bool dynamic);                     // Mac 0x106910, Win 0x5b9d40
SingleMesh*   CreateAxesMesh(Render* render, const Color& x, const Color& y, const Color& z,
                             Material* material, bool dynamic);                          // Mac 0x106bc0, Win 0x5ba070

} // namespace LEGO
