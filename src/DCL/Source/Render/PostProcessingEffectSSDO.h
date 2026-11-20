// LEGO Digital Designer — restored source
// LEGO::PostProcessingEffectSSDO — screen-space directional occlusion: scene normals + depth feed
//   "ssdo.sh" (random vectors, lights, projection, inverse focal length), "blur.sh" composes.
//   Default effect with shaders ("SelectedPostProcessingEffect" defaults to the count: the last).
// TU 371 (Mac 0x35eb80-0x360d5f; Win 0x5c23e0-0x5c369f). Vtables Mac 0x74fe60, Win 0xa2d374.
// sizeof 0x5c. Member names inferred.
#pragma once
#include "PostProcessingEffect.h"
#include "Material.h"

namespace LEGO {

class PostProcessingEffectSSDO : public PostProcessingEffect {
public:
    // Mac 0x35eb80, Win 0x5c23e0
    PostProcessingEffectSSDO(Render* render, Directory* shaders, bool roundEdges,
                             Directory* roundEdgeShaders);
    virtual ~PostProcessingEffectSSDO();                                    // Mac 0x360360 (0x3601a0) / 0x360370, Win 0x5c30f0 / sdd 0x5c2480

    // Base Initialize, LoadShaders, "SSAOVectorTexture.png", the quad's second coordinate set.
    virtual bool Initialize();                                              // Mac 0x35ebf0, Win 0x5c24a0
    // Base Resize + the occlusion target (0x40000420) and the normals target (0x40000440).
    virtual void Resize(const Vector2& size, const Vector2& imageSize);     // Mac 0x35ee50, Win 0x5c2610
    virtual void Deactivate();                                              // Mac 0x35f0d0, Win 0x5c2860
    virtual void Apply(const Vector2& size, RenderQueue* queue, Texture* target,
                       const Light* light0, const Light* light1, const Light* light2,
                       const Matrix3D& projection, Real32 fieldOfView, Real32 aspect,
                       Real32 nearPlane, Real32 farPlane);                  // Mac 0x35f130, Win 0x5c28b0
    // Normals shader + material; "ssdo.sh" (ScreenMap 0, RandomMap 1, DepthMap 2, NormalMap 3)
    // and "blur.sh" (ScreenMap 0, SsaoMap 1, DepthMap 2).
    virtual bool LoadShaders();                                             // Mac 0x3603b0, Win 0x5c3210
    virtual String GetName() const;                                         // Mac 0x360cf0, Win 0x5c3640 "SSDO"

protected:
    RefPointer<Shader>   m_normalsShader;     // +0x40
    RefPointer<Shader>   m_blurShader;        // +0x44
    RefPointer<Shader>   m_ssdoShader;        // +0x48
    RefPointer<Material> m_normalsMaterial;   // +0x4c
    RefPointer<Texture>  m_randomTexture;     // +0x50
    RefPointer<Texture>  m_ssdoTarget;        // +0x54
    RefPointer<Texture>  m_normalsTarget;     // +0x58
};

} // namespace LEGO
