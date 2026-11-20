// LEGO Digital Designer — restored source
// LEGO::Shader — abstract GPU program; Shader::LoadArgs. Header-only: D1/D0 and SetMaterialColors
// (Mac 0x235f90, Win 0x4fa5a0) are emitted in TU 180. Created via the render's ObjectFactory with
// LoadArgs (OpenGLShaderFactory, Mac 0x3843f0 / Win 0x4fae90, parses the `.sh` descriptor).
//
// Vtables: Mac 0x72f27c (15 slots: 13 + D1/D0 last), Win 0xa27824 (13 slots). The nine
// SetVariable overloads are reversed by MSVC: Mac 2..10 = Win 10..2; slots 0, 1, 11, 12 keep
// their numbers. OpenGLShader adds Mac 15/16 = Win 13/14 (uniform / attribute location).
// Layout: Mac {vptr, bool m_opaque +0x04, virtual base Object +0x08};
//         Win {vptr, vbptr, bool +0x08, Object +0x0c}.
// SetVariable: OpenGLShader queues {type, values, location} records (Mac 0x2352a0) for
// uniforms (attribute == false, location via slot 15) or vertex attributes (slot 16; -1 =
// unknown attribute, ignored). Type codes 0-3 float1-4, 4-7 int1-4, 8 matrix. Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "ObjectFactory.h"
#include "BasicString.h"
#include "Geometry.h"
#include "Path.h"
#include "Render.h"

namespace LEGO {

class Shader : public virtual Object {
public:
    // Mac vtable 0x7692a0 {D1 0x61c20, D0 0x64150}, Win sdd 0x5c3720 (ICF). Mac sizeof 0x20.
    // Users: BrickConnectSceneManager (022a), PostProcessingEffect (Mac 369: field1C = 1).
    struct LoadArgs : public ObjectFactoryArgs {
        LoadArgs() : field1C(0) {}
        virtual ~LoadArgs() {}                                        // Mac 0x61c20/0x64150
        Path   path;      // +0x04 the `.sh` descriptor (e.g. Shaders/ShinyPlastic.sh)
        UInt32 field1C;   // +0x1c (1 in every user seen; meaning unknown)
    };

    // NOTE: neither build initialises m_opaque (OpenGLShader ctor Win 0x4fab20, inline on Mac in
    // 0x3873f0 / 0x387910, skips Win +0x08 / Mac +0x04): heap garbage until the first
    // SetMaterialColors, so IsOpaque() is unpredictable for materials that never pass colours
    // (SceneNormalsMaterial, post-processing). Deliberate deviation: starts true (the same as
    // any non-zero garbage byte).
    Shader() : m_opaque(true) {}

    virtual bool IsOpaque() const = 0;                                // 0 (OpenGLShader 0x236570 returns m_opaque)
    // Uploads AmbientColor/DiffuseColor/SpecularColor/EmissiveColor/Shininess (uniforms) and
    // records whether the diffuse alpha is 1.
    // NOTE: written inline here; the original body is emitted once in TU 180 (OpenGLShader.cpp).
    virtual void SetMaterialColors(const MaterialColors& colors)     // 1 Mac 0x235f90, Win 0x4fa5a0
    {
        m_opaque = colors.diffuse.a == 1.0f;
        SetVariable(AString("AmbientColor"), colors.ambient.r, colors.ambient.g, colors.ambient.b, colors.ambient.a, false);
        SetVariable(AString("DiffuseColor"), colors.diffuse.r, colors.diffuse.g, colors.diffuse.b, colors.diffuse.a, false);
        SetVariable(AString("SpecularColor"), colors.specular.r, colors.specular.g, colors.specular.b, colors.specular.a, false);
        SetVariable(AString("EmissiveColor"), colors.emissive.r, colors.emissive.g, colors.emissive.b, colors.emissive.a, false);
        SetVariable(AString("Shininess"), colors.shininess, false);
    }
    virtual void SetVariable(const AString& name, Real32 x, bool attribute) = 0;                       // 2  (Win 10) OpenGL 0x2350a0
    virtual void SetVariable(const AString& name, Real32 x, Real32 y, bool attribute) = 0;             // 3  (Win 9)  0x2353f0
    virtual void SetVariable(const AString& name, Real32 x, Real32 y, Real32 z, bool attribute) = 0;   // 4  (Win 8)  0x2355f0
    virtual void SetVariable(const AString& name, Real32 x, Real32 y, Real32 z, Real32 w,
                             bool attribute) = 0;                                                      // 5  (Win 7)  0x235800
    virtual void SetVariable(const AString& name, Int32 x, bool attribute) = 0;                        // 6  (Win 6)  0x235a10
    virtual void SetVariable(const AString& name, Int32 x, Int32 y, bool attribute) = 0;               // 7  (Win 5)  0x235b30
    virtual void SetVariable(const AString& name, Int32 x, Int32 y, Int32 z, bool attribute) = 0;      // 8  (Win 4)  0x235c50
    virtual void SetVariable(const AString& name, Int32 x, Int32 y, Int32 z, Int32 w,
                             bool attribute) = 0;                                                      // 9  (Win 3)  0x235d70
    virtual void SetVariable(const AString& name, const Matrix3D& m, bool transpose,
                             bool attribute) = 0;                                                      // 10 (Win 2)  0x235e90
    // Called by Render::SetShader (Mac 0xf5890) on the new / previous shader.
    virtual void Bind(Render* render) = 0;                            // 11 (OpenGLShader20 0x24faa0)
    virtual void Unbind(Render* render) = 0;                          // 12 (OpenGLShader20 0x24fdb0)

    virtual ~Shader() {}                                              // Mac 13/14 0x2364a0/0x2364c0

protected:
    bool m_opaque;   // Mac +0x04, Win +0x08 (diffuse alpha == 1 in the last SetMaterialColors)
};

} // namespace LEGO
