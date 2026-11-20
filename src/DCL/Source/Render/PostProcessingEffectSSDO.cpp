// LEGO Digital Designer — restored source
// TU 371 "PostProcessingEffectSSDO.cpp" (Mac 0x35eb80-0x360d5f; Win 0x5c23e0-0x5c369f).
// See PostProcessingEffectSSDO.h.
#include "PostProcessingEffectSSDO.h"
#include "PostProcessingScreenPass.h"

#include "Render.h"
#include "RenderQueue.h"
#include "Texture.h"
#include "VertexBuffer.h"
#include "Shader.h"

#include <math.h>

namespace LEGO {

// Mac 0x35eb80, Win 0x5c23e0
PostProcessingEffectSSDO::PostProcessingEffectSSDO(Render* render, Directory* shaders,
                                                   bool roundEdges, Directory* roundEdgeShaders)
    : PostProcessingEffect(render, shaders, roundEdges, roundEdgeShaders)
{
}

// Mac 0x360360 (body 0x3601a0) / 0x360370, Win 0x5c30f0 / sdd 0x5c2480
PostProcessingEffectSSDO::~PostProcessingEffectSSDO()
{
}

// Mac 0x35ebf0, Win 0x5c24a0
bool PostProcessingEffectSSDO::Initialize()
{
    if (!PostProcessingEffect::Initialize() || !LoadShaders())
        return false;
    m_randomTexture = PostProcessing::LoadShaderTexture(m_render, m_shaderDirectory,
                                                       String("SSAOVectorTexture.png"));
    if (m_randomTexture.IsNull())
        return false;
    PostProcessing::SetUnitTexCoords(m_quad);
    return true;
}

// Mac 0x35ee50, Win 0x5c2610
void PostProcessingEffectSSDO::Resize(const Vector2& size, const Vector2& imageSize)
{
    PostProcessingEffect::Resize(size, imageSize);
    m_ssdoTarget = PostProcessing::CreateTarget(m_render, 0x40000420, imageSize);
    m_normalsTarget = PostProcessing::CreateTarget(m_render, 0x40000440, imageSize);
    // NOTE: no null check in either build (a target the GPU could not create crashes); fixed
    if (!m_ssdoTarget.IsNull())
        FitQuad(size, m_ssdoTarget);
}

// Mac 0x35f0d0, Win 0x5c2860
void PostProcessingEffectSSDO::Deactivate()
{
    PostProcessingEffect::Deactivate();
    m_normalsTarget = 0;
    m_ssdoTarget = 0;
}

// Mac 0x35f130, Win 0x5c28b0
void PostProcessingEffectSSDO::Apply(const Vector2& size, RenderQueue* queue, Texture* target,
                                     const Light* light0, const Light* light1, const Light* light2,
                                     const Matrix3D& projection, Real32 fieldOfView, Real32 aspect,
                                     Real32 nearPlane, Real32 farPlane)
{
    (void)aspect;
    Render* render = m_render;
    PostProcessing::DrawNormals(render, queue, m_normalsTarget, m_target, m_normalsMaterial);
    render->SetRenderTargetDepthBuffer(target, render->GetRenderTargetDepthBuffer(m_target));
    render->SetRenderTarget(m_ssdoTarget);
    PostProcessing::ScreenPass pass(render, m_ssdoTarget->GetSize());

    m_ssdoShader->SetVariable(AString("screen_size"), target->GetSize().x, target->GetSize().y, false);
    m_ssdoShader->SetVariable(AString("near"), nearPlane, false);
    m_ssdoShader->SetVariable(AString("far"), farPlane, false);
    m_ssdoShader->SetVariable(AString("light_vec0"), light0->direction.x, light0->direction.y,
                              light0->direction.z, false);
    m_ssdoShader->SetVariable(AString("light_vec1"), light1->direction.x, light1->direction.y,
                              light1->direction.z, false);
    m_ssdoShader->SetVariable(AString("light_vec2"), light2->direction.x, light2->direction.y,
                              light2->direction.z, false);
    const Real32 focal = tanf(fieldOfView * 0.5f);
    m_ssdoShader->SetVariable(AString("invFocalLen"), focal, size.y / size.x * focal, false);
    m_ssdoShader->SetVariable(AString("proj_matrix"), projection, false, false);
    m_blurShader->SetVariable(AString("screen_size"), target->GetSize().x, target->GetSize().y, false);
    m_blurShader->SetVariable(AString("near"), nearPlane, false);
    m_blurShader->SetVariable(AString("far"), farPlane, false);

    // the occlusion term
    render->SetShader(m_ssdoShader);
    render->SetVertexBuffer(m_quad);
    render->SetTexture(m_target, 0);
    render->SetTexture(m_randomTexture, 1);
    render->SetTexture(render->GetRenderTargetDepthBuffer(m_target), 2);
    render->SetTexture(m_normalsTarget, 3);
    render->EnableBlend(false);
    render->DrawVertexBuffer();
    render->SetShader(0);
    render->SetTexture(0, 3);
    render->SetTexture(0, 2);
    render->SetTexture(0, 1);
    render->SetTexture(0, 0);

    // blurred into the caller's target
    render->SetRenderTarget(target);
    render->SetShader(m_blurShader);
    render->SetVertexBuffer(m_quad);
    render->SetTexture(m_target, 0);
    render->SetTexture(m_ssdoTarget, 1);
    render->SetTexture(render->GetRenderTargetDepthBuffer(m_target), 2);
    render->EnableBlend(false);
    render->DrawVertexBuffer();
    render->SetShader(0);
    render->SetTexture(0, 2);
    render->SetTexture(0, 1);
    render->SetTexture(0, 0);
    pass.End();
}

// Mac 0x3603b0, Win 0x5c3210
bool PostProcessingEffectSSDO::LoadShaders()
{
    if (!LoadNormalsMaterial(m_normalsShader, m_normalsMaterial))
        return false;
    m_ssdoShader = LoadShader(String("ssdo.sh"), m_ssdoShader, false);
    if (m_ssdoShader.IsNull())
        return false;
    m_blurShader = LoadShader(String("blur.sh"), m_blurShader, false);
    if (m_blurShader.IsNull()) {
        m_ssdoShader = 0;
        return false;
    }
    m_ssdoShader->SetVariable(AString("ScreenMap"), (Int32)0, false);
    m_ssdoShader->SetVariable(AString("RandomMap"), (Int32)1, false);
    m_ssdoShader->SetVariable(AString("DepthMap"), (Int32)2, false);
    m_ssdoShader->SetVariable(AString("NormalMap"), (Int32)3, false);
    m_blurShader->SetVariable(AString("ScreenMap"), (Int32)0, false);
    m_blurShader->SetVariable(AString("SsaoMap"), (Int32)1, false);
    m_blurShader->SetVariable(AString("DepthMap"), (Int32)2, false);
    return true;
}

// Mac 0x360cf0, Win 0x5c3640
String PostProcessingEffectSSDO::GetName() const
{
    return String("SSDO");
}

} // namespace LEGO
