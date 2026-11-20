// LEGO Digital Designer — restored source
// TU 370 "PostProcessingEffectSSAO.cpp" (Mac 0x35b370-0x35eb7f; Win 0x5bfe10-0x5c23df):
// PostProcessingEffectSSAO, PostProcessingEffectBlur, PostProcessingEffectSceneNormals,
// PostProcessingEffectEdgeDetection. See PostProcessingEffectSSAO.h.
#include "PostProcessingEffectSSAO.h"
#include "PostProcessingScreenPass.h"

#include "ObjectFactory.h"
#include "Render.h"
#include "RenderQueue.h"
#include "Texture.h"
#include "VertexBuffer.h"
#include "Shader.h"
#include "Materials.h"
#include "Directory.h"
#include "Path.h"

#include <typeinfo>

namespace LEGO {

// ===========================================================================================
// PostProcessingEffectSSAO
// ===========================================================================================

// Mac 0x35b370, Win 0x5bfe10
PostProcessingEffectSSAO::PostProcessingEffectSSAO(Render* render, Directory* shaders,
                                                   bool roundEdges, Directory* roundEdgeShaders)
    : PostProcessingEffect(render, shaders, roundEdges, roundEdgeShaders)
{
}

// Mac 0x35c390 (body 0x35c280) / 0x35c3a0, Win 0x5c07f0 / sdd 0x5bfe90
PostProcessingEffectSSAO::~PostProcessingEffectSSAO()
{
}

// Mac 0x35b3d0, Win 0x5bfeb0
bool PostProcessingEffectSSAO::Initialize()
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

// Mac 0x35b630, Win 0x5c0020
void PostProcessingEffectSSAO::Resize(const Vector2& size, const Vector2& imageSize)
{
    PostProcessingEffect::Resize(size, imageSize);
    m_ssaoTarget = PostProcessing::CreateTarget(m_render, 0x40000420, imageSize);
    // NOTE (guard): both builds go on without a null check (a crash when the render target
    // cannot be created).
    if (!m_ssaoTarget.IsNull())
        FitQuad(size, m_ssaoTarget);
}

// Mac 0x35b820, Win 0x5c0210
void PostProcessingEffectSSAO::Deactivate()
{
    m_ssaoTarget = 0;
    PostProcessingEffect::Deactivate();
}

// Mac 0x35b860, Win 0x5c0240
void PostProcessingEffectSSAO::Apply(const Vector2& size, RenderQueue* queue, Texture* target,
                                     const Light* light0, const Light* light1, const Light* light2,
                                     const Matrix3D& projection, Real32 fieldOfView, Real32 aspect,
                                     Real32 nearPlane, Real32 farPlane)
{
    (void)size; (void)queue; (void)light0; (void)light1; (void)light2; (void)projection;
    (void)fieldOfView; (void)aspect;
    Render* render = m_render;
    render->SetRenderTargetDepthBuffer(target, render->GetRenderTargetDepthBuffer(m_target));
    render->SetRenderTarget(m_ssaoTarget);
    PostProcessing::ScreenPass pass(render, m_ssaoTarget->GetSize());

    m_ssaoShader->SetVariable(AString("screen_size"), target->GetSize().x, target->GetSize().y, false);
    m_ssaoShader->SetVariable(AString("near"), nearPlane, false);
    m_ssaoShader->SetVariable(AString("far"), farPlane, false);
    m_blurShader->SetVariable(AString("screen_size"), target->GetSize().x, target->GetSize().y, false);
    m_blurShader->SetVariable(AString("near"), nearPlane, false);
    m_blurShader->SetVariable(AString("far"), farPlane, false);

    // the occlusion term
    render->SetShader(m_ssaoShader);
    render->SetVertexBuffer(m_quad);
    render->SetTexture(m_target, 0);
    render->SetTexture(m_randomTexture, 1);
    render->SetTexture(render->GetRenderTargetDepthBuffer(m_target), 2);
    render->EnableBlend(false);
    render->DrawVertexBuffer();
    render->SetShader(0);
    render->SetTexture(0, 2);
    render->SetTexture(0, 1);
    render->SetTexture(0, 0);

    // blurred into the caller's target
    render->SetRenderTarget(target);
    render->SetShader(m_blurShader);
    render->SetVertexBuffer(m_quad);
    render->SetTexture(m_target, 0);
    render->SetTexture(m_ssaoTarget, 1);
    render->SetTexture(render->GetRenderTargetDepthBuffer(m_target), 2);
    render->EnableBlend(false);
    render->DrawVertexBuffer();
    render->SetShader(0);
    render->SetTexture(0, 2);
    render->SetTexture(0, 1);
    render->SetTexture(0, 0);
    pass.End();
}

// Mac 0x35c3e0, Win 0x5c08c0
bool PostProcessingEffectSSAO::LoadShaders()
{
    m_ssaoShader = LoadShader(String("ssao.sh"), m_ssaoShader, false);
    if (m_ssaoShader.IsNull())
        return false;
    m_blurShader = LoadShader(String("blur.sh"), m_blurShader, false);
    if (m_blurShader.IsNull()) {
        m_ssaoShader = 0;
        return false;
    }
    m_ssaoShader->SetVariable(AString("ScreenMap"), (Int32)0, false);
    m_ssaoShader->SetVariable(AString("RandomMap"), (Int32)1, false);
    m_ssaoShader->SetVariable(AString("DepthMap"), (Int32)2, false);
    m_blurShader->SetVariable(AString("ScreenMap"), (Int32)0, false);
    m_blurShader->SetVariable(AString("SsaoMap"), (Int32)1, false);
    m_blurShader->SetVariable(AString("DepthMap"), (Int32)2, false);
    return true;
}

// Mac 0x35ca90, Win 0x5c0b60
String PostProcessingEffectSSAO::GetName() const
{
    return String("SSAO");
}

// ===========================================================================================
// PostProcessingEffectBlur
// ===========================================================================================

// Mac 0x35cac0, Win 0x5c0bc0
PostProcessingEffectBlur::PostProcessingEffectBlur(Render* render, Directory* shaders,
                                                   bool roundEdges, Directory* roundEdgeShaders)
    : PostProcessingEffect(render, shaders, roundEdges, roundEdgeShaders)
{
}

// Mac 0x35cff0 / 0x35d050, Win 0x5c1040 / sdd 0x5c0c30
PostProcessingEffectBlur::~PostProcessingEffectBlur()
{
}

// Mac 0x35cb00, Win 0x5c0c50
bool PostProcessingEffectBlur::Initialize()
{
    if (!PostProcessingEffect::Initialize())
        return false;
    return LoadShaders();
}

// Mac 0x35cb30, Win 0x5c0c70
void PostProcessingEffectBlur::Resize(const Vector2& size, const Vector2& imageSize)
{
    PostProcessingEffect::Resize(size, imageSize);
    // NOTE: no null check in either build (a target the GPU could not create crashes); fixed
    if (!m_target.IsNull())
        FitQuad(size, m_target);
}

// Mac 0x35cc20, Win 0x5c0d30
void PostProcessingEffectBlur::Apply(const Vector2& size, RenderQueue* queue, Texture* target,
                                     const Light* light0, const Light* light1, const Light* light2,
                                     const Matrix3D& projection, Real32 fieldOfView, Real32 aspect,
                                     Real32 nearPlane, Real32 farPlane)
{
    (void)size; (void)queue; (void)light0; (void)light1; (void)light2; (void)projection;
    (void)fieldOfView; (void)aspect; (void)nearPlane; (void)farPlane;
    Render* render = m_render;
    render->SetRenderTargetDepthBuffer(target, render->GetRenderTargetDepthBuffer(m_target));
    render->SetRenderTarget(target);
    PostProcessing::ScreenPass pass(render, target->GetSize());
    m_shader->SetVariable(AString("high_res_size"), target->GetSize().x, target->GetSize().y, false);
    render->SetShader(m_shader);
    render->SetVertexBuffer(m_quad);
    render->SetTexture(m_target, 0);
    render->EnableBlend(true);
    render->DrawVertexBuffer();
    render->SetShader(0);
    render->SetTexture(0, 0);
    pass.End();
}

// Mac 0x35d0d0, Win 0x5c10b0
bool PostProcessingEffectBlur::LoadShaders()
{
    m_shader = LoadShader(String("DownSamplingShader.sh"), m_shader, false);
    if (m_shader.IsNull())
        return false;
    m_shader->SetVariable(AString("high_res_texture"), (Int32)0, false);
    return true;
}

// Mac 0x35d260, Win 0x5c11b0
String PostProcessingEffectBlur::GetName() const
{
    return String("Blur");
}

// ===========================================================================================
// PostProcessingEffectSceneNormals
// ===========================================================================================

// Mac 0x35d290, Win 0x5c1210
PostProcessingEffectSceneNormals::PostProcessingEffectSceneNormals(Render* render,
                                                                   Directory* shaders,
                                                                   bool roundEdges,
                                                                   Directory* roundEdgeShaders)
    : PostProcessingEffect(render, shaders, roundEdges, roundEdgeShaders)
{
}

// Mac 0x35d970 (body 0x35d860) / 0x35d980, Win 0x5c15c0 / sdd 0x5c1290
PostProcessingEffectSceneNormals::~PostProcessingEffectSceneNormals()
{
}

// Mac 0x35d2f0, Win 0x5c0c50
bool PostProcessingEffectSceneNormals::Initialize()
{
    if (!PostProcessingEffect::Initialize())
        return false;
    return LoadShaders();
}

// Mac 0x35d320, Win 0x5c1a00
void PostProcessingEffectSceneNormals::Resize(const Vector2& size, const Vector2& imageSize)
{
    PostProcessingEffect::Resize(size, imageSize);
    // NOTE: no null check in either build (a target the GPU could not create crashes); fixed
    if (!m_target.IsNull())
        FitQuad(size, m_target);
    m_normalsTarget = PostProcessing::CreateTarget(m_render, 0x40000440, imageSize);
}

// Mac 0x35d500, Win 0x5c12b0
void PostProcessingEffectSceneNormals::Deactivate()
{
    m_normalsTarget = 0;
    PostProcessingEffect::Deactivate();
}

// Mac 0x35d540, Win 0x5c12e0
void PostProcessingEffectSceneNormals::Apply(const Vector2& size, RenderQueue* queue,
                                             Texture* target, const Light* light0,
                                             const Light* light1, const Light* light2,
                                             const Matrix3D& projection, Real32 fieldOfView,
                                             Real32 aspect, Real32 nearPlane, Real32 farPlane)
{
    (void)size; (void)light0; (void)light1; (void)light2; (void)projection; (void)fieldOfView;
    (void)aspect; (void)nearPlane; (void)farPlane;
    Render* render = m_render;
    PostProcessing::DrawNormals(render, queue, m_normalsTarget, m_target, m_normalsMaterial);
    render->SetRenderTarget(target);
    PostProcessing::ScreenPass pass(render, target->GetSize());
    render->SetShader(m_textureShader);
    render->SetVertexBuffer(m_quad);
    render->SetTexture(m_normalsTarget, 0);
    render->EnableBlend(true);
    render->DrawVertexBuffer();
    render->SetShader(0);
    render->SetTexture(0, 0);
    pass.End();
}

// Mac 0x35d9c0, Win 0x5c1690
bool PostProcessingEffectSceneNormals::LoadShaders()
{
    if (!LoadNormalsMaterial(m_normalsShader, m_normalsMaterial))
        return false;
    m_textureShader = LoadShader(String("RenderScreenSizedTexture.sh"), m_textureShader, false);
    if (m_textureShader.IsNull())
        return false;
    m_textureShader->SetVariable(AString("TexMap"), (Int32)0, false);
    return true;
}

// Mac 0x35dcf0, Win 0x5c18f0
String PostProcessingEffectSceneNormals::GetName() const
{
    return String("Scene normals");
}

// ===========================================================================================
// PostProcessingEffectEdgeDetection
// ===========================================================================================

// Mac 0x35dd20, Win 0x5c1950
PostProcessingEffectEdgeDetection::PostProcessingEffectEdgeDetection(Render* render,
                                                                     Directory* shaders,
                                                                     bool roundEdges,
                                                                     Directory* roundEdgeShaders)
    : PostProcessingEffect(render, shaders, roundEdges, roundEdgeShaders)
{
}

// Mac 0x35e6b0 (body 0x35e570) / 0x35e6c0, Win 0x5c2000 / sdd 0x5c19e0
PostProcessingEffectEdgeDetection::~PostProcessingEffectEdgeDetection()
{
}

// Mac 0x35dd80, Win 0x5c0c50
bool PostProcessingEffectEdgeDetection::Initialize()
{
    if (!PostProcessingEffect::Initialize())
        return false;
    return LoadShaders();
}

// Mac 0x35ddb0, Win 0x5c1a00
void PostProcessingEffectEdgeDetection::Resize(const Vector2& size, const Vector2& imageSize)
{
    PostProcessingEffect::Resize(size, imageSize);
    // NOTE: no null check in either build (a target the GPU could not create crashes); fixed
    if (!m_target.IsNull())
        FitQuad(size, m_target);
    m_normalsTarget = PostProcessing::CreateTarget(m_render, 0x40000440, imageSize);
}

// Mac 0x35df90, Win 0x5c12b0
void PostProcessingEffectEdgeDetection::Deactivate()
{
    m_normalsTarget = 0;
    PostProcessingEffect::Deactivate();
}

// Mac 0x35dfd0, Win 0x5c1c00
void PostProcessingEffectEdgeDetection::Apply(const Vector2& size, RenderQueue* queue,
                                              Texture* target, const Light* light0,
                                              const Light* light1, const Light* light2,
                                              const Matrix3D& projection, Real32 fieldOfView,
                                              Real32 aspect, Real32 nearPlane, Real32 farPlane)
{
    (void)size; (void)light0; (void)light1; (void)light2; (void)projection; (void)fieldOfView;
    (void)aspect; (void)nearPlane; (void)farPlane;
    Render* render = m_render;
    PostProcessing::DrawNormals(render, queue, m_normalsTarget, m_target, m_normalsMaterial);
    render->SetRenderTarget(target);
    PostProcessing::ScreenPass pass(render, target->GetSize());
    m_edgeShader->SetVariable(AString("pixelSizeX"), target->GetSize().x, false);
    m_edgeShader->SetVariable(AString("pixelSizeY"), target->GetSize().y, false);
    render->SetShader(m_edgeShader);
    render->SetVertexBuffer(m_quad);
    render->SetTexture(m_normalsTarget, 0);
    render->SetTexture(render->GetRenderTargetDepthBuffer(m_target), 1);
    render->EnableBlend(true);
    render->DrawVertexBuffer();
    render->SetShader(0);
    render->SetTexture(0, 1);
    render->SetTexture(0, 0);
    pass.End();
}

// Mac 0x35e700, Win 0x5c20f0
bool PostProcessingEffectEdgeDetection::LoadShaders()
{
    if (!LoadNormalsMaterial(m_normalsShader, m_normalsMaterial))
        return false;
    m_edgeShader = LoadShader(String("EdgeDetection.sh"), m_edgeShader, false);
    if (m_edgeShader.IsNull())
        return false;
    m_edgeShader->SetVariable(AString("NormalMap"), (Int32)0, false);
    m_edgeShader->SetVariable(AString("DepthMap"), (Int32)1, false);
    return true;
}

// Mac 0x35eb10, Win 0x5c2380
String PostProcessingEffectEdgeDetection::GetName() const
{
    return String("Edge detection");
}

} // namespace LEGO
