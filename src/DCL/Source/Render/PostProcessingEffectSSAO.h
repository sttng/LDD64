// LEGO Digital Designer — restored source
// TU 370 effects (Mac 0x35b370-0x35eb7f; Win 0x5bfe10-0x5c23df): SSAO, Blur, SceneNormals,
// EdgeDetection. Blur ("DownSamplingShader.sh") and EdgeDetection ("EdgeDetection.sh") never
// initialise: the shipped Assets.lif lacks their descriptors (as in the original).
// Vtables Mac / Win: SSAO 0x74fd40 / 0xa2d268, Blur 0x74fd78 / 0xa2d298, SceneNormals
// 0x74fdb0 / 0xa2d2c8, EdgeDetection 0x74fde8 / 0xa2d2f8. Member names inferred.
#pragma once
#include "PostProcessingEffect.h"
#include "Material.h"

namespace LEGO {

// SSAO: Apply renders the occlusion term from the depth buffer into an own target (ssao.sh with
// the 4x4 random vector texture), then blur.sh composes colour * blurred occlusion into the
// caller's target. sizeof 0x50.
class PostProcessingEffectSSAO : public PostProcessingEffect {
public:
    // Mac 0x35b370, Win 0x5bfe10
    PostProcessingEffectSSAO(Render* render, Directory* shaders, bool roundEdges,
                             Directory* roundEdgeShaders);
    virtual ~PostProcessingEffectSSAO();                                    // Mac 0x35c390 (0x35c280) / 0x35c3a0, Win 0x5c07f0 / sdd 0x5bfe90

    // Base Initialize, LoadShaders, "SSAOVectorTexture.png" (alpha mode 2) and the second
    // texture-coordinate set of the quad (0 0, 1 0, 0 1, 1 1).
    virtual bool Initialize();                                              // Mac 0x35b3d0, Win 0x5bfeb0
    // Base Resize + the occlusion target (same format and size); the quad fits `size`.
    virtual void Resize(const Vector2& size, const Vector2& imageSize);     // Mac 0x35b630, Win 0x5c0020
    virtual void Deactivate();                                              // Mac 0x35b820, Win 0x5c0210
    virtual void Apply(const Vector2& size, RenderQueue* queue, Texture* target,
                       const Light* light0, const Light* light1, const Light* light2,
                       const Matrix3D& projection, Real32 fieldOfView, Real32 aspect,
                       Real32 nearPlane, Real32 farPlane);                  // Mac 0x35b860, Win 0x5c0240
    // "ssao.sh" (ScreenMap 0, RandomMap 1, DepthMap 2) and "blur.sh" (ScreenMap 0, SsaoMap 1,
    // DepthMap 2); without blur.sh the SSAO shader is dropped as well.
    virtual bool LoadShaders();                                             // Mac 0x35c3e0, Win 0x5c08c0
    virtual String GetName() const;                                         // Mac 0x35ca90, Win 0x5c0b60 "SSAO"

protected:
    RefPointer<Shader>  m_blurShader;      // +0x40
    RefPointer<Shader>  m_ssaoShader;      // +0x44
    RefPointer<Texture> m_randomTexture;   // +0x48 SSAOVectorTexture.png
    RefPointer<Texture> m_ssaoTarget;      // +0x4c
};

// Blur (down-sampling): one quad with "DownSamplingShader.sh" (missing from the assets).
// sizeof 0x44.
class PostProcessingEffectBlur : public PostProcessingEffect {
public:
    // Mac 0x35cac0, Win 0x5c0bc0
    PostProcessingEffectBlur(Render* render, Directory* shaders, bool roundEdges,
                             Directory* roundEdgeShaders);
    virtual ~PostProcessingEffectBlur();                                    // Mac 0x35cff0 / 0x35d050, Win 0x5c1040 / sdd 0x5c0c30

    // Base Initialize + LoadShaders. Mac 0x35cb00, Win 0x5c0c50 (one body for Blur,
    // SceneNormals and EdgeDetection)
    virtual bool Initialize();
    virtual void Resize(const Vector2& size, const Vector2& imageSize);     // Mac 0x35cb30, Win 0x5c0c70
    virtual void Apply(const Vector2& size, RenderQueue* queue, Texture* target,
                       const Light* light0, const Light* light1, const Light* light2,
                       const Matrix3D& projection, Real32 fieldOfView, Real32 aspect,
                       Real32 nearPlane, Real32 farPlane);                  // Mac 0x35cc20, Win 0x5c0d30
    virtual bool LoadShaders();                                             // Mac 0x35d0d0, Win 0x5c10b0
    virtual String GetName() const;                                         // Mac 0x35d260, Win 0x5c11b0 "Blur"

protected:
    RefPointer<Shader> m_shader;   // +0x40
};

// Scene normals (debug view): the scene queue is drawn again with the normals material into
// an own target, which is then shown with "RenderScreenSizedTexture.sh". sizeof 0x50.
class PostProcessingEffectSceneNormals : public PostProcessingEffect {
public:
    // Mac 0x35d290, Win 0x5c1210
    PostProcessingEffectSceneNormals(Render* render, Directory* shaders, bool roundEdges,
                                     Directory* roundEdgeShaders);
    virtual ~PostProcessingEffectSceneNormals();                            // Mac 0x35d970 (0x35d860) / 0x35d980, Win 0x5c15c0 / sdd 0x5c1290

    virtual bool Initialize();                                              // Mac 0x35d2f0, Win 0x5c0c50
    // Base Resize, quad, the normals target (format 0x40000440).
    virtual void Resize(const Vector2& size, const Vector2& imageSize);     // Mac 0x35d320, Win 0x5c1a00
    virtual void Deactivate();                                              // Mac 0x35d500, Win 0x5c12b0
    virtual void Apply(const Vector2& size, RenderQueue* queue, Texture* target,
                       const Light* light0, const Light* light1, const Light* light2,
                       const Matrix3D& projection, Real32 fieldOfView, Real32 aspect,
                       Real32 nearPlane, Real32 farPlane);                  // Mac 0x35d540, Win 0x5c12e0
    // "Normals.sh" + SceneNormalsMaterial, or with round edges "RoundEdge/RENormals.sh" +
    // SceneNormalsRoundEdgeMaterial; "RenderScreenSizedTexture.sh" (TexMap 0).
    virtual bool LoadShaders();                                             // Mac 0x35d9c0, Win 0x5c1690
    virtual String GetName() const;                                         // Mac 0x35dcf0, Win 0x5c18f0 "Scene normals"

protected:
    RefPointer<Shader>   m_normalsShader;     // +0x40
    RefPointer<Shader>   m_textureShader;     // +0x44
    RefPointer<Texture>  m_normalsTarget;     // +0x48
    RefPointer<Material> m_normalsMaterial;   // +0x4c
};

// Edge detection: like SceneNormals, then "EdgeDetection.sh" (missing from the assets) over the
// normals and the depth buffer. sizeof 0x54.
class PostProcessingEffectEdgeDetection : public PostProcessingEffect {
public:
    // Mac 0x35dd20, Win 0x5c1950
    PostProcessingEffectEdgeDetection(Render* render, Directory* shaders, bool roundEdges,
                                      Directory* roundEdgeShaders);
    virtual ~PostProcessingEffectEdgeDetection();                           // Mac 0x35e6b0 (0x35e570) / 0x35e6c0, Win 0x5c2000 / sdd 0x5c19e0

    virtual bool Initialize();                                              // Mac 0x35dd80, Win 0x5c0c50
    virtual void Resize(const Vector2& size, const Vector2& imageSize);     // Mac 0x35ddb0, Win 0x5c1a00
    virtual void Deactivate();                                              // Mac 0x35df90, Win 0x5c12b0
    virtual void Apply(const Vector2& size, RenderQueue* queue, Texture* target,
                       const Light* light0, const Light* light1, const Light* light2,
                       const Matrix3D& projection, Real32 fieldOfView, Real32 aspect,
                       Real32 nearPlane, Real32 farPlane);                  // Mac 0x35dfd0, Win 0x5c1c00
    // The normals shader + material as SceneNormals; "EdgeDetection.sh" (NormalMap 0,
    // DepthMap 1).
    virtual bool LoadShaders();                                             // Mac 0x35e700, Win 0x5c20f0
    virtual String GetName() const;                                         // Mac 0x35eb10, Win 0x5c2380 "Edge detection"

protected:
    RefPointer<Shader>   m_normalsShader;     // +0x40
    RefPointer<Shader>   m_shader44;          // +0x44 (never set; released by the destructor)
    RefPointer<Texture>  m_normalsTarget;     // +0x48
    RefPointer<Material> m_normalsMaterial;   // +0x4c
    RefPointer<Shader>   m_edgeShader;        // +0x50
};

} // namespace LEGO
