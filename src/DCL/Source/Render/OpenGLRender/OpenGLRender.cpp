// LEGO Digital Designer — restored source
// OpenGLRender.cpp (TU 034): LEGO::OpenGLRender, OpenGLVertexBufferFactory,
// PlatformTextLayoutFactory; Render's D1/D0 are header inlines emitted here (Mac 0xf78c0/0xf78e0).
// Mac 0xf2870-0xf87af, Win 0x4eb6f0-0x4f1ec0. See OpenGLRender.h.
#include "OpenGLRender.h"
#include "OpenGLFactories.h"
#include "OpenGLWindow.h"
#include "OpenGLTexture.h"
#include "OpenGLVertexBuffer.h"
#include "OpenGLShader.h"
#include "RenderQueue.h"
#include "Material.h"
#include "Shader.h"
#include "Mesh.h"
#include "Platform.h"
#include "Bitmap.h"

#include <math.h>
#include <string.h>

#if defined(_WIN32)
#include <GL/glu.h>
#else
#include <OpenGL/glu.h>
#endif

namespace LEGO {

namespace {

// GL enums indexed by the Render API constants (Mac 0x763890 / Win 0xae1128 …).
const GLenum kCompareFuncs[8] = {GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL,
                                 GL_GEQUAL, GL_ALWAYS};                       // Mac 0x763890, Win 0xae1128
const GLenum kDefaultDepthFunc = GL_LEQUAL;                                   // Mac 0x76389c, Win 0xae1134
const GLenum kBlendFactors[11] = {GL_ZERO, GL_ONE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                                  GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_DST_COLOR,
                                  GL_ONE_MINUS_DST_COLOR, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR,
                                  GL_CONSTANT_COLOR};                         // Mac 0x7638b0, Win 0xae1148
const GLenum kDefaultBlendSource = GL_SRC_ALPHA;                              // Mac 0x7638b8, Win 0xae1150
const GLenum kDefaultBlendDestination = GL_ONE_MINUS_SRC_ALPHA;               // Mac 0x7638bc, Win 0xae1154
const GLenum kPolygonModes[3] = {GL_POINT, GL_LINE, GL_FILL};                 // Mac 0x7638f4
const GLenum kFrontFaces[2] = {GL_CW, GL_CCW};                                // Mac 0x763900

// Nearest integer value, ties up (Win 0x4c9630 LEGO::RoundToNearest; inlined on the Mac).
inline Real32 RoundToNearest(Real32 value)
{
    Real32 lower = floorf(value);
    Real32 upper = ceilf(value);
    return (value - lower < upper - value) ? lower : upper;
}

// Win 0x4ed640 (EAX = capability, stack: enable). (name inferred)
inline void SetCapability(GLenum capability, bool enable)
{
    if (enable)
        glEnable(capability);
    else
        glDisable(capability);
}

// Win 0x4ef2c0: default state set by Initialize (inlined on the Mac). (name inferred)
void SetDefaultState()
{
    SetCapability(GL_LIGHTING, false);
    SetCapability(GL_DEPTH_TEST, true);
    glDepthMask(GL_TRUE);
    glDepthFunc(kDefaultDepthFunc);
    glBlendFunc(kDefaultBlendSource, kDefaultBlendDestination);
}

// GL_MAX_TEXTURE_SIZE rounded down to a power of two (Win 0x4f19d0 + shift; inlined on Mac).
inline UInt32 FloorPowerOfTwo(UInt32 value)
{
    UInt32 power = 1;
    UInt32 previous;
    do {
        previous = power;
        power *= 2;
    } while (previous < value);
    return previous >> 1;
}

} // namespace

// =============================================================================================
// OpenGLRender
// =============================================================================================

// Mac 0xf2900, Win 0x4ec4b0
OpenGLRender::OpenGLRender(UInt32 level, Window* window)
    : Render(level),
      m_extensions(new OpenGLExtensions(level)),
      m_window(window),
#if defined(_WIN32)
      m_hwnd(0),
      m_context(wglGetCurrentContext()),
      m_dc(wglGetCurrentDC()),
#else
      m_context(CGLGetCurrentContext()),
#endif
      m_scissor(0.0f, 0.0f, 0.0f, 0.0f),
      m_viewport(0.0f, 0.0f, 0.0f, 0.0f),
      m_opacity(1.0f),
      m_shader(0),
      m_material(0),
      m_defaultMaterial(0),
      m_vertexBuffer(0),
      m_renderTarget(0),
      m_ownsContext(false),
      m_frameActive(false),
      m_colorWrite(true),
      m_alphaWrite(true),
      m_objectFactory(0),
      m_gpuProcessing(false),
      m_maxTextureSize(0),
      m_maxColorAttachments(0),
      m_maxVertexTextureImageUnits(0),
      m_maxTextureImageUnits(0),
      m_maxCombinedTextureImageUnits(0),
      m_maxVertexUniformComponents(0),
      m_maxFragmentUniformComponents(0),
      m_maxVaryingFloats(0),
      m_lighting(false),
      m_shaderMaterialColors(false)
{
    // m_projection / m_view / m_world: Matrix3D() is the identity (Mac 0x1df350; Win copies
    // Matrix3D::Identity()). m_materialColors: white colours (Color()), shininess 0.
    m_materialColors.shininess = 0.0f;
    m_batchActive = false;
    m_batchStates = 0;
    m_batchUnbinds = 0;
    m_batchLastUnit = -1;
    m_batchLastBound = -1;
    m_batchUnboundShader = 0;
    m_shaderStamp = 0;
    for (UInt32 i = 0; i < 16; ++i) {
        m_textureUnits[i].texture = 0;
        m_textureUnits[i].target = 0;
        m_textureUnits[i].type = -1;
        m_textureUnits[i].attachment = 0;   // not initialised by the original; fixed (low)
        m_textureUnits[i].glName = 0;
        m_textureUnits[i].stamp = 0;
    }
#if defined(_WIN32)
    m_gpuProcessing = true;
#else
    if (Platform::GetInstance()->GetOSVersion().m_minor > 3) {
        GLint value;
        if (CGLGetParameter(m_context, kCGLCPGPUVertexProcessing, &value) == kCGLNoError && value == 1 &&
            CGLGetParameter(m_context, kCGLCPGPUFragmentProcessing, &value) == kCGLNoError && value == 1)
            m_gpuProcessing = true;
    }
#endif
    WindowObserver::Attach(window);
    m_size = m_window->GetSize();
}

// Mac 0xf6eb0 (via 0xf6cf0) / 0xf7060, Win 0x4ef400
OpenGLRender::~OpenGLRender()
{
    if (m_ownsContext) {
#if defined(_WIN32)
        OpenGLWindow::DestroyGLContext(m_hwnd, m_context, m_dc);
        DestroyWindow(m_hwnd);
#else
        CGLClearDrawable(m_context);
        CGLReleaseContext(m_context);
#endif
    }
    delete m_extensions;
}

// Mac 0xf2f00, Win 0x4ec850
bool OpenGLRender::Initialize(const Render::CreateArgs& /*args*/)
{
    bool result = false;
    LDD_GL_TRY
    m_objectFactory = new ObjectFactory;
    m_objectFactory->RegisterFactory(typeid(OpenGLVertexBuffer), new OpenGLVertexBufferFactory(this),
                                     typeid(VertexBuffer));
    m_objectFactory->RegisterFactory(typeid(OpenGLTexture), CreateOpenGLTextureFactory(this),
                                     typeid(Texture));
    m_objectFactory->RegisterFactory(TypeOfFontPlatform(), CreateFontPlatformFactory(), TypeOfFont());
    m_objectFactory->RegisterFactory(typeid(OpenGLShader), CreateOpenGLShaderFactory(this),
                                     typeid(Shader));
    RefPointer<SpecificObjectFactory> textLayoutFactory = new PlatformTextLayoutFactory(this);
    m_objectFactory->RegisterFactory(TypeOfPlatformTextLayout(), textLayoutFactory, TypeOfTextLayout());
    m_objectFactory->RegisterFactory(TypeOfTextLayoutRender(), textLayoutFactory, TypeOfTextLayout());

    SetDefaultState();
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint*)&m_maxTextureSize);
    if (m_maxTextureSize == 0)
        return false;
    if (m_maxTextureSize & (m_maxTextureSize - 1))
        m_maxTextureSize = FloorPowerOfTwo(m_maxTextureSize);
    if (m_extensions->m_framebufferObject)
        glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS_EXT, (GLint*)&m_maxColorAttachments);
    // NOTE: gated on float-texture support, not on GLSL.
    if (m_extensions->m_textureFloat)
        glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, (GLint*)&m_maxVertexTextureImageUnits);
    if (m_extensions->m_glsl120) {
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, (GLint*)&m_maxTextureImageUnits);
        glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, (GLint*)&m_maxCombinedTextureImageUnits);
        glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, (GLint*)&m_maxFragmentUniformComponents);
        glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, (GLint*)&m_maxVertexUniformComponents);
        glGetIntegerv(GL_MAX_VARYING_FLOATS, (GLint*)&m_maxVaryingFloats);
    }
    result = m_extensions->m_glVersion >= Version(1, 1, 0, 0);
    LDD_GL_CATCH
    return result;
}

// Mac 0xf6bf0, Win 0x4ef3a0
void OpenGLRender::OnFinalRelease()
{
    m_queues.Clear();
    WindowObserver::OnFinalRelease();   // leaves the window, Object::OnFinalRelease
}

// Mac 0xf7250, Win 0x4ef590
void OpenGLRender::OnResize(const Vector2& /*size*/, const Vector2& clientSize)
{
    m_size = clientSize;
}

// Mac 0xf7290, Win 0x4ef5b0
void OpenGLRender::OnClose()
{
    for (OpenGLTexture** it = m_textures.Begin(); it != m_textures.End(); ++it)
        (*it)->ReleaseGLObject();
}

// Mac 0xf3900, Win 0x4ef310
void OpenGLRender::MakeCurrent()
{
#if defined(_WIN32)
    if (wglGetCurrentContext() != m_context || wglGetCurrentDC() != m_dc) {
        wglMakeCurrent(m_dc, m_context);
        NoteGLStateChanged();
    }
#else
    CGLGetCurrentContext();
#endif
}

// Mac 0xf6780, Win 0x4ef340
void OpenGLRender::RegisterTexture(OpenGLTexture* texture)
{
    m_textures.PushBack(texture);
}

// Mac 0xf69c0 (0xf69f0: find + erase), Win 0x4ef360
void OpenGLRender::UnregisterTexture(OpenGLTexture* texture)
{
    for (UInt32 i = 0; i < m_textures.GetSize(); ++i) {
        if (m_textures[i] == texture) {
            m_textures.Erase(i);
            return;
        }
    }
}

// Mac 0xf3920, Win 0x4ef5f0
void OpenGLRender::ResetState()
{
    SetShader(0);
    SetMaterial(0);
    for (UInt32 unit = 0; unit < 16; ++unit)
        SetTexture(0, unit);
    m_vertexBuffer = 0;
    m_frameActive = false;
}

// Mac 0xf60c0, Win 0x4ef640
void OpenGLRender::ApplyMaterial()
{
    if (m_shader && m_shaderMaterialColors)
        return;
    if (!m_lighting) {
        glColor4ub(0xff, 0xff, 0xff, 0xff);
        return;
    }
    Real32 diffuse[4] = {m_materialColors.diffuse.r, m_materialColors.diffuse.g,
                         m_materialColors.diffuse.b, m_opacity * m_materialColors.diffuse.a};
    glColor4f(diffuse[0], diffuse[1], diffuse[2], diffuse[3]);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, &m_materialColors.ambient.r);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, &m_materialColors.specular.r);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, &m_materialColors.emissive.r);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, m_materialColors.shininess);
}

// Mac 0xf66e0, Win 0x4ef230 (EAX = this, EDI = bitmap)
void OpenGLRender::UploadTextureImage(UInt32 bindTarget, UInt32 imageTarget, UInt32 textureName,
                                      UInt32 level, UInt32 format, UInt32 type, const Bitmap& bitmap)
{
    MakeCurrent();
    if (m_extensions->m_multitexture)
        glActiveTextureARB(GL_TEXTURE0_ARB);
    glBindTexture(bindTarget, textureName);
    NoteGLStateChanged();
    glTexSubImage2D(imageTarget, level, 0, 0, (bitmap.m_width + 3) & ~3u, bitmap.m_height, format,
                    type, bitmap.m_data);
    Texture* bound = m_textureUnits[0].texture;
    if (bound) {
        OpenGLTexture* texture = static_cast<OpenGLTexture*>(bound);
        glBindTexture(texture->GetTarget(), texture->GetTextureName(0));
    } else {
        glBindTexture(bindTarget, 0);
    }
}

// ---- slots 0-11 ----------------------------------------------------------------------------

// Mac 0xf7990, Win 0x4ebe20
ObjectFactory* OpenGLRender::GetObjectFactory() const { return m_objectFactory; }

// Mac 0xf33c0, Win 0x4ecb50
const Vector2& OpenGLRender::GetSize() const { return m_size; }

// Mac 0xf33d0, Win 0x4ecb60: GL's origin is bottom left, the API's top left.
void OpenGLRender::SetViewport(const Rect& viewport)
{
    LDD_GL_TRY
    GLint x = (GLint)floorf(viewport.m_min.x);
    GLint y = (GLint)floorf(viewport.m_min.y);
    GLint width = (GLint)ceilf(viewport.GetWidth());
    GLint height = (GLint)ceilf(viewport.GetHeight());
    Real32 targetHeight = m_renderTarget ? m_renderTarget->GetImageSize().y : m_size.y;
    glViewport(x, (GLint)RoundToNearest(targetHeight) - height - y, width, height);
    m_viewport.Set(Rect(viewport));
    LDD_GL_CATCH
}

// Mac 0xf3580, Win 0x4ecca0
const Rect& OpenGLRender::GetViewport() const { return m_viewport; }

// Mac 0xf3590, Win 0x4eccb0
void OpenGLRender::SetScissor(const Rect& rect)
{
    LDD_GL_TRY
    GLint x = (GLint)floorf(rect.m_min.x);
    GLint y = (GLint)floorf(rect.m_min.y);
    GLint width = (GLint)ceilf(rect.GetWidth());
    GLint height = (GLint)ceilf(rect.GetHeight());
    Real32 targetHeight = m_renderTarget ? m_renderTarget->GetImageSize().y : m_size.y;
    glScissor(x, (GLint)RoundToNearest(targetHeight) - height - y, width, height);
    glEnable(GL_SCISSOR_TEST);
    m_scissor.Set(Rect(rect));
    LDD_GL_CATCH
}

// Mac 0xf3750, Win 0x4ecdf0
const Rect& OpenGLRender::GetScissor() const { return m_scissor; }

// Mac 0xf3760, Win 0x4ece00
void OpenGLRender::DisableScissor()
{
    LDD_GL_TRY
    glDisable(GL_SCISSOR_TEST);
    LDD_GL_CATCH
}

// Mac 0xf37d0, Win 0x4ece80
void OpenGLRender::SetClearColor(const Color& color)
{
    LDD_GL_TRY
    glClearColor(color.r, color.g, color.b, color.a);
    LDD_GL_CATCH
}

// Mac 0xf3860, Win 0x4ecf20
void OpenGLRender::Clear(UInt32 buffers)
{
    LDD_GL_TRY
    GLbitfield mask = 0;
    if (buffers & kClearColor)
        mask |= GL_COLOR_BUFFER_BIT;
    if (buffers & kClearDepth)
        mask |= GL_DEPTH_BUFFER_BIT;
    if (buffers & kClearStencil)
        mask |= GL_STENCIL_BUFFER_BIT;
    if (mask)
        glClear(mask);
    LDD_GL_CATCH
}

// Mac 0xf38e0, Win 0x4ecfc0
bool OpenGLRender::BeginFrame()
{
    MakeCurrent();
    m_frameActive = true;
    return true;
}

// Mac 0xf3910, Win 0x4ecfe0
void OpenGLRender::EndFrame()
{
    ResetState();
}

// Mac 0xf3aa0 ([[NSOpenGLContext currentContext] flushBuffer]), Win 0x4ecff0 (SwapBuffers)
void OpenGLRender::Present()
{
    MakeCurrent();
    LDD_GL_TRY
#if defined(_WIN32)
    SwapBuffers(m_dc);
#elif defined(__OBJC__)
    [[NSOpenGLContext currentContext] flushBuffer];
#else
    CGLFlushDrawable(CGLGetCurrentContext());   // HOST-ONLY stand-in for flushBuffer
#endif
    LDD_GL_CATCH
}

// ---- slots 12-32 ---------------------------------------------------------------------------

// Mac 0xf3b30, Win 0x4ed070
void OpenGLRender::SetCullMode(UInt32 mode)
{
    LDD_GL_TRY
    if (m_batchActive) {
        DeferState(kBatchCullMode, mode);
        return;
    }
    if (mode == kCullNone) {
        glDisable(GL_CULL_FACE);
    } else {
        glEnable(GL_CULL_FACE);
        glCullFace(mode == kCullBack ? GL_BACK : GL_FRONT);
    }
    LDD_GL_CATCH
}

// Mac 0xf3bd0, Win 0x4ed110
void OpenGLRender::SetPolygonMode(UInt32 mode)
{
    LDD_GL_TRY
    if (m_batchActive) {
        DeferState(kBatchPolygonMode, mode);
        return;
    }
    glPolygonMode(GL_FRONT_AND_BACK, kPolygonModes[mode]);
    LDD_GL_CATCH
}

// Mac 0xf3c50, Win 0x4ed190
void OpenGLRender::SetFrontFace(UInt32 winding)
{
    LDD_GL_TRY
    if (m_batchActive) {
        DeferState(kBatchFrontFace, winding);
        return;
    }
    glFrontFace(kFrontFaces[winding]);
    LDD_GL_CATCH
}

// Mac 0xf3cc0, Win 0x4ed210
void OpenGLRender::SetLineWidth(UInt32 width)
{
    LDD_GL_TRY
    if (m_batchActive) {
        DeferState(kBatchLineWidth, width);
        return;
    }
    glLineWidth((GLfloat)width);
    LDD_GL_CATCH
}

// Mac 0xf3d40, Win 0x4ed2a0
void OpenGLRender::SetDepthFunc(UInt32 func)
{
    LDD_GL_TRY
    glDepthFunc(kCompareFuncs[func]);
    LDD_GL_CATCH
}

// Mac 0xf3db0, Win 0x4ed320
void OpenGLRender::SetPolygonOffset(Real32 factor, Real32 units)
{
    LDD_GL_TRY
    glPolygonOffset(factor, units);
    LDD_GL_CATCH
}

// Mac 0xf3e30, Win 0x4ed3b0: the reference is scaled by the opacity.
void OpenGLRender::SetAlphaFunc(UInt32 func, Real32 reference)
{
    LDD_GL_TRY
    glAlphaFunc(kCompareFuncs[func], m_opacity * reference);
    LDD_GL_CATCH
}

// Mac 0xf3eb0, Win 0x4ed440
void OpenGLRender::SetBlendFunc(UInt32 source, UInt32 destination)
{
    LDD_GL_TRY
    glBlendFunc(kBlendFactors[source], kBlendFactors[destination]);
    LDD_GL_CATCH
}

// Mac 0xf3f30, Win 0x4ed4d0
UInt32 OpenGLRender::GetMaxClipPlanes() const
{
    GLint planes = 0;
    LDD_GL_TRY
    glGetIntegerv(GL_MAX_CLIP_PLANES, &planes);
    LDD_GL_CATCH
    return (UInt32)planes;
}

// Mac 0xf3fb0, Win 0x4ed550: the plane is given in view space (loaded with the view matrix).
void OpenGLRender::SetClipPlane(UInt32 index, const Plane& plane)
{
    LDD_GL_TRY
    GLint mode;
    glGetIntegerv(GL_MATRIX_MODE, &mode);
    if (mode != GL_MODELVIEW)
        glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixf(m_view);
    GLdouble equation[4] = {plane.m_normal.x, plane.m_normal.y, plane.m_normal.z, plane.m_distance};
    glClipPlane(GL_CLIP_PLANE0 + index, equation);
    glPopMatrix();
    if (mode != GL_MODELVIEW)
        glMatrixMode(mode);
    LDD_GL_CATCH
}

// Mac 0xf40b0, Win 0x4ed660
void OpenGLRender::EnableBlend(bool enable)
{
    LDD_GL_TRY
    SetCapability(GL_BLEND, enable);
    LDD_GL_CATCH
}

// Mac 0xf4130, Win 0x4ed6e0
void OpenGLRender::EnableLighting(bool enable)
{
    LDD_GL_TRY
    m_lighting = enable;
    if (m_batchActive) {
        DeferState(kBatchLighting, enable);
        return;
    }
    SetCapability(GL_LIGHTING, enable);
    LDD_GL_CATCH
}

// Mac 0xf41c0, Win 0x4ed760
void OpenGLRender::SetLocalViewer(bool enable)
{
    LDD_GL_TRY
    if (m_batchActive) {
        DeferState(kBatchLocalViewer, enable);
        return;
    }
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, enable);
    LDD_GL_CATCH
}

// Mac 0xf4230, Win 0x4ed7e0
void OpenGLRender::EnableColorWrite(bool enable)
{
    LDD_GL_TRY
    m_colorWrite = enable;
    glColorMask(enable, enable, enable, m_alphaWrite);
    LDD_GL_CATCH
}

// Mac 0xf42c0, Win 0x4ed870
void OpenGLRender::EnableAlphaWrite(bool enable)
{
    LDD_GL_TRY
    m_alphaWrite = enable;
    glColorMask(m_colorWrite, m_colorWrite, m_colorWrite, enable);
    LDD_GL_CATCH
}

// Mac 0xf4350, Win 0x4ed900
void OpenGLRender::EnableAlphaTest(bool enable)
{
    LDD_GL_TRY
    SetCapability(GL_ALPHA_TEST, enable);
    LDD_GL_CATCH
}

// Mac 0xf43d0, Win 0x4ed980
void OpenGLRender::EnableDepthTest(bool enable)
{
    LDD_GL_TRY
    SetCapability(GL_DEPTH_TEST, enable);
    LDD_GL_CATCH
}

// Mac 0xf4450, Win 0x4eda00
void OpenGLRender::EnableDepthWrite(bool enable)
{
    LDD_GL_TRY
    if (m_batchActive) {
        DeferState(kBatchDepthWrite, enable);
        return;
    }
    glDepthMask(enable);
    LDD_GL_CATCH
}

// Mac 0xf44c0, Win 0x4eda80
void OpenGLRender::EnablePolygonOffset(bool enable)
{
    LDD_GL_TRY
    if (m_batchActive) {
        DeferState(kBatchPolygonOffset, enable);
        return;
    }
    SetCapability(GL_POLYGON_OFFSET_FILL, enable);
    LDD_GL_CATCH
}

// Mac 0xf4540, Win 0x4edb00
void OpenGLRender::EnableClipPlane(UInt32 index, bool enable)
{
    LDD_GL_TRY
    SetCapability(GL_CLIP_PLANE0 + index, enable);
    LDD_GL_CATCH
}

// Mac 0xf45c0, Win 0x4edb80
void OpenGLRender::EnableFog(bool enable)
{
    LDD_GL_TRY
    SetCapability(GL_FOG, enable);
    LDD_GL_CATCH
}

// ---- slots 33-38 ---------------------------------------------------------------------------

// Mac 0xf4640, Win 0x4edc00: created on first use with room for 2000 entries
RenderQueue* OpenGLRender::GetRenderQueue(UInt32 index)
{
    QueueMap::Iterator it = m_queues.Find(index);
    if (it != m_queues.End())
        return it->second;
    RefPointer<RenderQueue>& queue = m_queues[index];
    queue = CreateRenderQueue(this, 2000);
    return queue;
}

// Mac 0xf49f0, Win 0x4edd00
const char* OpenGLRender::GetName() const { return "OpenGL"; }

// Mac 0xf4a20, Win 0x4edd20
Texture* OpenGLRender::GetRenderTarget() const { return m_renderTarget; }

// Mac 0xf4a30, Win 0x4edd30
Texture* OpenGLRender::GetRenderTargetDepthBuffer(Texture* target) const
{
    if (target) {
        if (OpenGLTexture2DRenderTargetFBO* rt = dynamic_cast<OpenGLTexture2DRenderTargetFBO*>(target))
            return rt->GetDepthBuffer();
        if (OpenGLTexture2DMultipleRenderTargetFBO* mrt = dynamic_cast<OpenGLTexture2DMultipleRenderTargetFBO*>(target))
            return mrt->GetDepthBuffer();
    }
    Texture* current = m_renderTarget;
    if (current) {
        if (OpenGLTexture2DRenderTargetFBO* rt = dynamic_cast<OpenGLTexture2DRenderTargetFBO*>(current))
            return rt->GetDepthBuffer();
        if (OpenGLTexture2DMultipleRenderTargetFBO* mrt = dynamic_cast<OpenGLTexture2DMultipleRenderTargetFBO*>(current))
            return mrt->GetDepthBuffer();
    }
    return 0;
}

// Mac 0xf4b10, Win 0x4eddb0
void OpenGLRender::SetRenderTarget(Texture* target)
{
    LDD_GL_TRY
    if (target == m_renderTarget)
        return;
    bool frameActive = m_frameActive;
    if (frameActive)
        ResetState();
    OpenGLTexture* previous = static_cast<OpenGLTexture*>(m_renderTarget);
    NoteGLStateChanged();
    if (m_extensions->m_framebufferObject) {
        OpenGLTexture2DMultipleRenderTargetFBO* mrt =
            target ? dynamic_cast<OpenGLTexture2DMultipleRenderTargetFBO*>(target) : 0;
        OpenGLTexture2DRenderTargetFBO* rt =
            (target && !mrt) ? dynamic_cast<OpenGLTexture2DRenderTargetFBO*>(target) : 0;
        if (mrt) {
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mrt->GetFramebuffer());
            m_renderTarget = target;
            GLenum buffers[4] = {GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT,
                                 GL_COLOR_ATTACHMENT2_EXT, GL_COLOR_ATTACHMENT3_EXT};
            glDrawBuffers(4, buffers);
            if (previous) {
                // NOTE: the previous target is read as a multiple-render-target texture whatever
                // its class (Mac +0x74, Win +0x6c).
                previous->NotifyChanged(0);
                static_cast<OpenGLTexture2DMultipleRenderTargetFBO*>(previous)->GetDepthBuffer()->NotifyChanged(0);
            }
        } else if (rt) {
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, rt->GetFramebuffer());
            m_renderTarget = target;
            if (previous) {
                // NOTE: likewise read as a single render target (Mac +0x64, Win +0x5c).
                previous->NotifyChanged(0);
                static_cast<OpenGLTexture2DRenderTargetFBO*>(previous)->GetDepthBuffer()->NotifyChanged(0);
            }
        } else {
            // the window, or a texture that is no render target
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
            m_renderTarget = target;
        }
    } else {
        // No FBOs: a render target is a pbuffer with its own context.
        // NOTE: no texture class creates pbuffers (OpenGLTexture.h); dead in practice.
        OpenGLTexture2DRenderTargetFBO* newTarget = static_cast<OpenGLTexture2DRenderTargetFBO*>(target);
#if defined(_WIN32)
        if (previous) {
            if (wglMakeContextCurrentARB)
                wglMakeContextCurrentARB(m_dc, m_dc, m_context);
            else
                wglMakeCurrent(m_dc, m_context);
            OpenGLTexture2DRenderTargetFBO* old = static_cast<OpenGLTexture2DRenderTargetFBO*>(previous);
            glBindTexture(old->GetTarget(), old->GetTextureName(0));
            wglBindTexImageARB((HPBUFFERARB)old->GetPbuffer(), WGL_FRONT_LEFT_ARB);
            glBindTexture(old->GetTarget(), 0);
        }
        m_renderTarget = target;
        if (newTarget) {
            wglReleaseTexImageARB((HPBUFFERARB)newTarget->GetPbuffer(), WGL_FRONT_LEFT_ARB);
            HDC dc = (HDC)newTarget->GetPbufferDC();
            HGLRC context = (HGLRC)newTarget->GetPbufferContext();
            if (wglMakeContextCurrentARB)
                wglMakeContextCurrentARB(dc, dc, context);
            else
                wglMakeCurrent(dc, context);
        }
#else
        if (previous)
            glFlush();
        CGLSetCurrentContext(newTarget ? (CGLContextObj)newTarget->GetPbufferContext() : m_context);
        m_renderTarget = target;
#endif
        if (previous)
            previous->NotifyChanged(0);
    }
    if (frameActive)
        m_frameActive = true;
    LDD_GL_CATCH
}

// Mac 0xf4e20, Win 0x4ee040
void OpenGLRender::SetRenderTargetDepthBuffer(Texture* target, Texture* depthBuffer)
{
    OpenGLTexture2DRenderTargetFBO* rt = target ? dynamic_cast<OpenGLTexture2DRenderTargetFBO*>(target) : 0;
    if (!rt) {
        if (!m_renderTarget)
            return;
        rt = dynamic_cast<OpenGLTexture2DRenderTargetFBO*>(m_renderTarget);
        if (!rt)
            return;
    }
    rt->SetDepthBuffer(depthBuffer);
}

// ---- slots 39-49 ---------------------------------------------------------------------------

// Mac 0xf4eb0, Win 0x4ee080
void OpenGLRender::SetProjectionMatrix(const Matrix3D& projection)
{
    LDD_GL_TRY
    m_projection = projection;
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(m_projection);
    LDD_GL_CATCH
}

// Mac 0xf4f90, Win 0x4ee110
void OpenGLRender::SetViewMatrix(const Matrix3D& view)
{
    LDD_GL_TRY
    m_view = view;
    m_world.SetIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(m_view);
    LDD_GL_CATCH
}

// Mac 0xf5090, Win 0x4ee1b0
void OpenGLRender::SetWorldMatrix(const Matrix3D& world)
{
    LDD_GL_TRY
    m_world = world;
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(m_view);
    glMultMatrixf(m_world);
    LDD_GL_CATCH
}

// Mac 0xf5190 / 0xf51a0 / 0xf51b0, Win 0x4ee260 / 0x4ee270 / 0x5aa5d0
const Matrix3D& OpenGLRender::GetProjectionMatrix() const { return m_projection; }
const Matrix3D& OpenGLRender::GetViewMatrix() const { return m_view; }
const Matrix3D& OpenGLRender::GetWorldMatrix() const { return m_world; }

// Mac 0xf51c0, Win 0x4ee280
void OpenGLRender::SetLight(const Light* light, UInt32 index)
{
    LDD_GL_TRY
    GLenum id = GL_LIGHT0 + index;
    if (!light) {
        glDisable(id);
    } else {
        glLightfv(id, GL_DIFFUSE, &light->diffuse.r);
        glLightfv(id, GL_AMBIENT, &light->ambient.r);
        glLightfv(id, GL_SPECULAR, &light->specular.r);
        if (light->type == Light::kDirectional) {
            GLfloat position[4] = {light->direction.x, light->direction.y, light->direction.z, 0.0f};
            glLightfv(id, GL_POSITION, position);
        } else if (light->type == Light::kPoint) {
            glLightf(id, GL_CONSTANT_ATTENUATION, light->constantAttenuation);
            glLightf(id, GL_LINEAR_ATTENUATION, light->linearAttenuation);
            glLightf(id, GL_QUADRATIC_ATTENUATION, light->quadraticAttenuation);
            glLighti(id, GL_SPOT_CUTOFF, 180);
            GLfloat position[4] = {light->position.x, light->position.y, light->position.z, 1.0f};
            glLightfv(id, GL_POSITION, position);
        } else if (light->type == Light::kSpot) {
            glLightf(id, GL_CONSTANT_ATTENUATION, light->constantAttenuation);
            glLightf(id, GL_LINEAR_ATTENUATION, light->linearAttenuation);
            glLightf(id, GL_QUADRATIC_ATTENUATION, light->quadraticAttenuation);
            glLightf(id, GL_SPOT_CUTOFF, light->spotCutoff);
            glLightf(id, GL_SPOT_EXPONENT, light->spotExponent);
            GLfloat direction[3] = {light->direction.x, light->direction.y, light->direction.z};
            glLightfv(id, GL_SPOT_DIRECTION, direction);
            GLfloat position[4] = {light->position.x, light->position.y, light->position.z, 1.0f};
            glLightfv(id, GL_POSITION, position);
        }
        glEnable(id);
    }
    LDD_GL_CATCH
}

// Mac 0xf5470, Win 0x4ee490
void OpenGLRender::SetMaterialColors(const MaterialColors& colors) { m_materialColors = colors; }

// Mac 0xf5490, Win 0x4ee4b0
const MaterialColors& OpenGLRender::GetMaterialColors() const { return m_materialColors; }

// Mac 0xf54a0, Win 0x4ee4c0
void OpenGLRender::SetOpacity(Real32 opacity)
{
    if (opacity >= 1.0f)
        opacity = 1.0f;
    if (opacity <= 0.0f)
        opacity = 0.0f;
    m_opacity = opacity;
}

// Mac 0xf4a10, Win 0x4edd10
Real32 OpenGLRender::GetOpacity() const { return m_opacity; }

// ---- slots 50-59 ---------------------------------------------------------------------------

// Common body of SetTextureAttachment / SetTexture (both inline it). (helper)
void OpenGLRender::BindTexture(Texture* texture, UInt32 attachment, UInt32 unit, bool setEnv)
{
    TextureUnit& u = m_textureUnits[unit];
    if (m_extensions->m_multitexture)
        glActiveTextureARB(GL_TEXTURE0_ARB + unit);
    Int32 type = texture ? (Int32)texture->GetType() : -1;
    if (u.type != type) {
        if (u.target)
            glDisable(u.target);
        if (type >= 0) {
            if (u.type == Texture::kTypeCube) {
                glDisable(GL_TEXTURE_GEN_S);
                glDisable(GL_TEXTURE_GEN_T);
                glDisable(GL_TEXTURE_GEN_R);
            }
            if (type == Texture::kTypeCube) {
                glEnable(GL_TEXTURE_GEN_S);
                glEnable(GL_TEXTURE_GEN_T);
                glEnable(GL_TEXTURE_GEN_R);
            }
            u.type = type;
        }
    }
    if (!texture) {
        glBindTexture(u.target, 0);
        NoteBind(unit, 0);
        u.target = 0;
    } else {
        OpenGLTexture* glTexture = static_cast<OpenGLTexture*>(texture);
        u.target = glTexture->GetTarget();
        if (setEnv)
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glEnable(u.target);
        glBindTexture(u.target, glTexture->GetTextureName(attachment));
        NoteBind(unit, glTexture->GetTextureName(attachment));
    }
}

// Mac 0xf54d0, Win 0x4ee520
void OpenGLRender::SetTextureAttachment(Texture* texture, UInt32 attachment, UInt32 unit)
{
    LDD_GL_TRY
    if (m_batchActive) {
        BatchSetTexture(texture, attachment, unit, true, true);
        return;
    }
    TextureUnit& u = m_textureUnits[unit];
    if (texture != u.texture || u.attachment != attachment) {
        u.texture = texture;
        u.attachment = attachment;
        BindTexture(texture, attachment, unit, true);
    }
    LDD_GL_CATCH
}

// Mac 0xf56c0, Win 0x4ee6a0
void OpenGLRender::SetTexture(Texture* texture, UInt32 unit)
{
    LDD_GL_TRY
    if (m_batchActive) {
        BatchSetTexture(texture, 0, unit, false, false);
        return;
    }
    TextureUnit& u = m_textureUnits[unit];
    if (texture != u.texture) {
        u.texture = texture;
        BindTexture(texture, 0, unit, false);
    }
    LDD_GL_CATCH
}

// Mac 0xf5870, Win 0x4ee800
Texture* OpenGLRender::GetTexture(UInt32 unit) const { return m_textureUnits[unit].texture; }

// Mac 0xf5890, Win 0x4ee820
void OpenGLRender::SetShader(Shader* shader)
{
    LDD_GL_TRY
    if (m_batchActive) {
        BatchSetShader(shader);
        return;
    }
    if (m_shader != shader) {
        if (m_shader)
            m_shader->Unbind(this);
        NoteGLStateOwner();
        m_shader = shader;
        if (shader)
            shader->Bind(this);
        NoteShaderBind();
    }
    LDD_GL_CATCH
}

// Mac 0xf5930, Win 0x4ee8c0
// NOTE: both originals dereference a null buffer that differs from the current one (BindArrays
// on null). Win catches the access violation (/EHa) and reports a driver crash (ShowGraphicFailure
// Win 0x5c8a3b; seen in tests/diff/diff_scene_furniture.cpp); Mac dies with SIGSEGV. Deliberate
// deviation: checked for null and reported as on Win (ReportOpenGLCrash), in both builds.
void OpenGLRender::SetVertexBuffer(VertexBuffer* buffer)
{
    LDD_GL_TRY
    if (buffer != m_vertexBuffer) {
        m_vertexBuffer = buffer;
        if (!buffer) {
            ReportOpenGLCrash(__FILE__, __LINE__);
            return;
        }
        static_cast<OpenGLVertexBuffer*>(buffer)->BindArrays();
    }
    LDD_GL_CATCH
}

// Mac 0xf59b0, Win 0x4ee950
void OpenGLRender::BeginVertexBuffer(VertexBuffer* buffer)
{
    LDD_GL_TRY
    static_cast<OpenGLVertexBuffer*>(buffer)->Begin();
    LDD_GL_CATCH
}

// Mac 0xf5a20, Win 0x4ee9d0
void OpenGLRender::EndVertexBuffer(VertexBuffer* buffer)
{
    LDD_GL_TRY
    static_cast<OpenGLVertexBuffer*>(buffer)->End();
    LDD_GL_CATCH
}

// Mac 0xf5a90, Win 0x4eea50 (ends the previous material even when it is the same one)
void OpenGLRender::SetMaterial(Material* material)
{
    if (m_material && material) {
        SetMaterialBatched(material);
        return;
    }
    LDD_GL_TRY
    if (m_material)
        m_material->End(this);
    m_material = material;
    if (material)
        material->Begin(this);
    LDD_GL_CATCH
}

// Mac 0xf5b30, Win 0x4eeaf0
void OpenGLRender::SetDefaultMaterial(Material* material) { m_defaultMaterial = material; }

// Mac 0xf5b50, Win 0x4eeb00
void OpenGLRender::UseDefaultMaterial() { SetMaterial(m_defaultMaterial); }

// ---- slots 60-67 ---------------------------------------------------------------------------

// Mac 0xf5b70, Win 0x4eeb20: `width` x `height` pixels from the bottom left of the viewport
// reference, rows flipped to top-down. Rows are read with an even width (GL_PACK_ALIGNMENT 1).
// Mac reads GL_RGB / GL_BGRA, Win GL_BGR / GL_BGRA.
void OpenGLRender::ReadPixels(UInt8* buffer, UInt32 width, UInt32 height, UInt32 /*arg4*/,
                              UInt32 /*arg5*/, bool backBuffer, bool alpha)
{
    MakeCurrent();
    LDD_GL_TRY
    glFlush();
    glFinish();
    UInt32 bytesPerPixel = alpha ? 4 : 3;
    UInt32 paddedWidth = width + (width & 1);
    UInt32 stride = bytesPerPixel * paddedWidth;
    UInt32 rowBytes = bytesPerPixel * width;
    UInt8* pixels = new UInt8[stride * height];
    if (m_renderTarget && m_extensions->m_framebufferObject)
        glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
    else
        glReadBuffer(backBuffer ? GL_BACK : GL_FRONT);
    Real32 targetHeight = m_renderTarget ? m_renderTarget->GetImageSize().y : m_size.y;
    GLint top = (GLint)RoundToNearest(targetHeight);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
#if defined(_WIN32)
    GLenum format = alpha ? GL_BGRA_EXT : GL_BGR_EXT;
#else
    GLenum format = alpha ? GL_BGRA : GL_RGB;
#endif
    glReadPixels(0, top - (GLint)height, paddedWidth, height, format, GL_UNSIGNED_BYTE, pixels);
    const UInt8* row = pixels + stride * (height - 1);
    for (UInt32 y = 0; y < height; ++y) {
        memcpy(buffer, row, rowBytes);
        buffer += rowBytes;
        row -= stride;
    }
    delete[] pixels;
    LDD_GL_CATCH
}

// Mac 0xf5fa0, Win 0x4eed70
void OpenGLRender::DrawSphere(const Vector3& center, Real32 radius, bool wireframe)
{
    LDD_GL_TRY
    GLUquadric* quadric = gluNewQuadric();
    if (quadric) {
        glPushMatrix();
        gluQuadricDrawStyle(quadric, wireframe ? GLU_LINE : GLU_FILL);
        ApplyMaterial();
        glMatrixMode(GL_MODELVIEW);
        Matrix3D translation;
        translation.SetTranslationMatrix(center);
        glLoadMatrixf(m_view);
        glMultMatrixf(translation);
        gluSphere(quadric, radius, 32, 32);
        gluDeleteQuadric(quadric);
        glPopMatrix();
    }
    LDD_GL_CATCH
}

// Mac 0xf6200, Win 0x4eee60: six faces (immediate mode, current model-view matrix)
void OpenGLRender::DrawBox(const Vector3& minimum, const Vector3& maximum, bool wireframe)
{
    LDD_GL_TRY
    ApplyMaterial();
    GLenum mode = wireframe ? GL_LINE_LOOP : GL_QUADS;
    const Vector3& a = minimum;
    const Vector3& b = maximum;
    glBegin(mode);
    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(a.x, a.y, a.z);
    glVertex3f(a.x, b.y, a.z);
    glVertex3f(b.x, b.y, a.z);
    glVertex3f(b.x, a.y, a.z);
    glEnd();
    glBegin(mode);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(b.x, b.y, b.z);
    glVertex3f(a.x, b.y, b.z);
    glVertex3f(a.x, a.y, b.z);
    glVertex3f(b.x, a.y, b.z);
    glEnd();
    glBegin(mode);
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(b.x, b.y, a.z);
    glVertex3f(b.x, b.y, b.z);
    glVertex3f(b.x, a.y, b.z);
    glVertex3f(b.x, a.y, a.z);
    glEnd();
    glBegin(mode);
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(a.x, b.y, a.z);
    glVertex3f(a.x, a.y, a.z);
    glVertex3f(a.x, a.y, b.z);
    glVertex3f(a.x, b.y, b.z);
    glEnd();
    glBegin(mode);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(b.x, b.y, b.z);
    glVertex3f(b.x, b.y, a.z);
    glVertex3f(a.x, b.y, a.z);
    glVertex3f(a.x, b.y, b.z);
    glEnd();
    glBegin(mode);
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(b.x, a.y, b.z);
    glVertex3f(a.x, a.y, b.z);
    glVertex3f(a.x, a.y, a.z);
    glVertex3f(b.x, a.y, a.z);
    glEnd();
    LDD_GL_CATCH
}

// Mac 0xf5f10, Win 0x4eece0
void OpenGLRender::DrawVertexBuffer()
{
    LDD_GL_TRY
    static_cast<OpenGLVertexBuffer*>(m_vertexBuffer)->Draw();
    LDD_GL_CATCH
}

// Mac 0xf5f80, Win 0x4eed60
void OpenGLRender::SetColor(const Color& color)
{
    glColor4fv(&color.r);
}

// Mac 0xf7310, Win 0x4ef760
bool OpenGLRender::IsOcclusionQuerySupported() const
{
    if (m_extensions->m_occlusionQuery) {
        const_cast<OpenGLRender*>(this)->MakeCurrent();
        GLint bits = 0;
        LDD_GL_TRY
        glGetQueryiv(GL_SAMPLES_PASSED_ARB, GL_QUERY_COUNTER_BITS_ARB, &bits);
        LDD_GL_CATCH
        return bits > 0;
    }
    return m_extensions->m_occlusionQueryNV || m_extensions->m_occlusionTestHP;
}

// Mac 0xf73b0, Win 0x4ef830
Texture* OpenGLRender::BeginOcclusionQueries(Texture* target)
{
    Texture* previous = 0;
    if (IsOcclusionQuerySupported()) {
        BeginFrame();
        previous = GetRenderTarget();
        LDD_GL_TRY   // (after a reported crash: `previous`, as the original)
        SetRenderTarget(target);
        Real32 width = target->GetSize().x;
        Real32 height = target->GetSize().y;
        SetViewport(Rect(0.0f, 0.0f, width, height));
        DisableScissor();
        SetClearColor(Color::Black());
        SetMaterial(0);
        SetTexture(0, 0);
        EnableDepthWrite(true);
        EnableDepthTest(true);
        EnableLighting(false);
        SetCullMode(kCullBack);
        LDD_GL_CATCH
    }
    return previous;
}

// Mac 0xf7570 (ARB only), Win 0x4ef9b0 (ARB, NV, HP): draws `mesh` through queue 0 with
// colour and depth writes off.
UInt32 OpenGLRender::GetVisibleSampleCount(Mesh* mesh, const Matrix3D& transform)
{
    UInt32 samples = 0;
    LDD_GL_TRY
    if (m_extensions->m_occlusionQuery) {
        GLuint query;
        glGenQueriesARB(1, &query);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glDepthMask(GL_FALSE);
        glBeginQueryARB(GL_SAMPLES_PASSED_ARB, query);
        GetRenderQueue(0)->Add(mesh, transform, 0.0f, 0, 1.0f);
        GetRenderQueue(0)->Flush(0.1f, true, true);
        GetRenderQueue(0)->Clear();
        glEndQueryARB(GL_SAMPLES_PASSED_ARB);
        GLint available = 0;
        do {
            glGetQueryObjectivARB(query, GL_QUERY_RESULT_AVAILABLE_ARB, &available);
        } while (available == 0);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_TRUE);
        GLuint result = 0;
        glGetQueryObjectuivARB(query, GL_QUERY_RESULT_ARB, &result);
        glDeleteQueriesARB(1, &query);
        samples = result;
    }
#if defined(_WIN32)
    else if (m_extensions->m_occlusionQueryNV) {
        GLuint query;
        glGenOcclusionQueriesNV(1, &query);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glDepthMask(GL_FALSE);
        glBeginOcclusionQueryNV(query);
        GetRenderQueue(0)->Add(mesh, transform, 0.0f, 0, 1.0f);
        GetRenderQueue(0)->Flush(0.1f, true, true);
        GetRenderQueue(0)->Clear();
        glEndOcclusionQueryNV();
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_TRUE);
        GLuint result = 0;
        glGetOcclusionQueryuivNV(query, GL_PIXEL_COUNT_NV, &result);
        glDeleteOcclusionQueriesNV(1, &query);
        samples = result;
    } else if (m_extensions->m_occlusionTestHP) {
        glDepthMask(GL_FALSE);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glEnable(GL_OCCLUSION_TEST_HP);
        GetRenderQueue(0)->Add(mesh, transform, 0.0f, 0, 1.0f);
        GetRenderQueue(0)->Flush(0.1f, true, true);
        GetRenderQueue(0)->Clear();
        glDisable(GL_OCCLUSION_TEST_HP);
        glDepthMask(GL_TRUE);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        GLboolean visible = GL_FALSE;
        glGetBooleanv(GL_OCCLUSION_TEST_RESULT_HP, &visible);
        samples = visible ? 10 : 0;   // NOTE: a yes/no test, reported as 10 samples
    }
#endif
    LDD_GL_CATCH
    return samples;
}

// ---- slots 68-81 ---------------------------------------------------------------------------

// Mac 0xf7780, Win 0x4efd10
void OpenGLRender::SwitchRenderTarget(Texture* target)
{
    SetRenderTarget(target);
    EndFrame();
}

// Mac 0xf77b0, Win 0x4efd40
bool OpenGLRender::SupportsShaders() const
{
    return m_gpuProcessing && (m_extensions->m_shaders20 || m_extensions->m_shaderObjectsARB);
}

// Mac 0xf77e0 / 0xf7800, Win 0x4efd70 / 0x4efd80
void OpenGLRender::SetShaderMaterialColors(bool enable) { m_shaderMaterialColors = enable; }
bool OpenGLRender::GetShaderMaterialColors() const { return m_shaderMaterialColors; }

// Mac 0xf7810-0xf78a0, Win 0x4efd90-0x4efe20
bool OpenGLRender::SupportsFloatTextures() const { return m_extensions->m_textureFloat; }
bool OpenGLRender::SupportsMultipleRenderTargets() const { return m_extensions->m_drawBuffers; }
UInt32 OpenGLRender::GetMaxColorAttachments() const { return m_maxColorAttachments; }
UInt32 OpenGLRender::GetMaxTextureSize() const { return m_maxTextureSize; }
UInt32 OpenGLRender::GetMaxVertexTextureImageUnits() const { return m_maxVertexTextureImageUnits; }
UInt32 OpenGLRender::GetMaxTextureImageUnits() const { return m_maxTextureImageUnits; }
UInt32 OpenGLRender::GetMaxCombinedTextureImageUnits() const { return m_maxCombinedTextureImageUnits; }
UInt32 OpenGLRender::GetMaxVertexUniformComponents() const { return m_maxVertexUniformComponents; }
UInt32 OpenGLRender::GetMaxFragmentUniformComponents() const { return m_maxFragmentUniformComponents; }
UInt32 OpenGLRender::GetMaxVaryingFloats() const { return m_maxVaryingFloats; }

// =============================================================================================
// OpenGLVertexBufferFactory — Mac 0xf7a70, Win 0x4ebe30
// =============================================================================================
Object* OpenGLVertexBufferFactory::CreateObject(const ObjectFactoryArgs& args)
{
    if (const VertexBuffer::CreateArgsRoundEdge* roundEdge =
            dynamic_cast<const VertexBuffer::CreateArgsRoundEdge*>(&args)) {
        if (!m_render->SupportsShaders())
            return 0;
        m_render->MakeCurrent();
        VertexBuffer* buffer = CreateOpenGLVertexBufferVBORoundEdge(m_render, *roundEdge);
        return buffer;
    }
    const VertexBuffer::CreateArgs* create = dynamic_cast<const VertexBuffer::CreateArgs*>(&args);
    if (!create)
        return 0;
    if (create->texCoordDimensions.GetSize() > 1 && !m_render->GetExtensions().m_multitexture)
        return 0;
    m_render->MakeCurrent();
    if (!create->dynamic && m_render->GetExtensions().m_vertexBufferObject) {
        if (VertexBuffer* buffer = CreateOpenGLVertexBufferVBO(m_render, *create))
            return buffer;
    }
    VertexBuffer* buffer = CreateOpenGLVertexBuffer(m_render, *create);
    return buffer;
}

// =============================================================================================
// PlatformTextLayoutFactory — Mac 0xf7f50, Win 0x4ec310
// =============================================================================================
Object* PlatformTextLayoutFactory::CreateObject(const ObjectFactoryArgs& args)
{
    return CreatePlatformTextLayout(m_render, args);
}

// =============================================================================================
// Performance: state batching in SetMaterial (not in the originals).
//
// The originals end and begin the material on every draw, even when it is the same one; End undoes
// what Begin redoes (about 20 of the 68 GL calls per draw of a shaded round-edge mesh).
// SetMaterialBatched runs End and Begin as one batch:
//   - fixed-function states are written once, with their last value;
//   - a texture unbind or shader Unbind is held back and cancelled when the same one is bound again
//     and GL is known to still hold it (see NoteGLStateChanged); a rebound shader only uploads the
//     variables recorded since;
//   - the active texture unit is left where the original sequence leaves it.
// Held-back operations run at the end. The GL state after the batch is the one the original
// sequence produces; only calls that would be undone in the same batch are left out.
// =============================================================================================

namespace {

// Bumped whenever the GL state may have changed outside what a render recorded: texture
// creation, deletion and uploads, context switches, or another render writing GL state.
UInt32 s_glStateEpoch = 1;
OpenGLRender* s_glStateOwner = 0;

} // namespace

void NoteGLStateChanged()
{
    ++s_glStateEpoch;
    s_glStateOwner = 0;
}

// Another render may share the context: its writes invalidate this render's records.
void OpenGLRender::NoteGLStateOwner()
{
    if (s_glStateOwner != this) {
        ++s_glStateEpoch;
        s_glStateOwner = this;
    }
}

// BindTexture bound `name` (0: none) on `unit`.
void OpenGLRender::NoteBind(UInt32 unit, UInt32 name)
{
    NoteGLStateOwner();
    m_textureUnits[unit].glName = name;
    m_textureUnits[unit].stamp = s_glStateEpoch;
    m_batchLastBound = (Int32)unit;
}

// m_shader->Bind ran: its program is current.
void OpenGLRender::NoteShaderBind()
{
    NoteGLStateOwner();
    m_shaderStamp = s_glStateEpoch;
}

void OpenGLRender::DeferState(UInt32 state, UInt32 value)
{
    m_batchStates |= 1u << state;
    m_batchValues[state] = value;
}

// SetTexture / SetTextureAttachment inside a batch: the same cache check, then the unit
// deferral described above.
void OpenGLRender::BatchSetTexture(Texture* texture, UInt32 attachment, UInt32 unit, bool setEnv,
                                   bool attachmentCall)
{
    LDD_GL_TRY
    TextureUnit& u = m_textureUnits[unit];
    if (texture == u.texture && (!attachmentCall || u.attachment == attachment))
        return;
    m_batchLastUnit = (Int32)unit;
    const UInt32 bit = 1u << unit;
    if (!texture) {
        if (!(m_batchUnbinds & bit) && u.stamp == s_glStateEpoch && u.glName != 0) {
            m_batchUnbinds |= bit;
            m_batchUnbound[unit] = u.texture;
            u.texture = 0;
            if (attachmentCall)
                u.attachment = attachment;
            return;
        }
    } else if (m_batchUnbinds & bit) {
        m_batchUnbinds &= ~bit;
        OpenGLTexture* glTexture = static_cast<OpenGLTexture*>(texture);
        if (texture == m_batchUnbound[unit] && u.stamp == s_glStateEpoch &&
            glTexture->GetTarget() == u.target &&
            glTexture->GetTextureName(attachment) == u.glName) {
            u.texture = texture;   // GL still has it bound: the unbind and the bind cancel out
            if (attachmentCall)
                u.attachment = attachment;
            return;
        }
        BindTexture(0, 0, unit, false);   // the held-back unbind, as the original made it
    }
    u.texture = texture;
    if (attachmentCall)
        u.attachment = attachment;
    BindTexture(texture, attachment, unit, setEnv);
    LDD_GL_CATCH
}

// SetShader inside a batch.
void OpenGLRender::BatchSetShader(Shader* shader)
{
    LDD_GL_TRY
    if (m_shader == shader)
        return;
    if (!shader) {
        if (!m_batchUnboundShader && m_shaderStamp == s_glStateEpoch) {
            m_batchUnboundShader = m_shader;
            m_shader = 0;
            return;
        }
        m_shader->Unbind(this);
        NoteGLStateOwner();
        m_shader = 0;
        return;
    }
    if (m_shader) {
        m_shader->Unbind(this);
        NoteGLStateOwner();
    }
    Shader* unbound = m_batchUnboundShader;
    m_batchUnboundShader = 0;
    m_shader = shader;
    if (unbound == shader && m_shaderStamp == s_glStateEpoch) {
        // The program is still current: only the variables recorded since need uploading, and
        // Bind uploads them (its glUseProgram of the current program changes nothing).
        OpenGLShader* glShader = dynamic_cast<OpenGLShader*>(shader);
        if (glShader && glShader->GetPendingUniforms().IsEmpty() &&
            glShader->GetPendingAttributes().IsEmpty())
            return;
    } else if (unbound) {
        unbound->Unbind(this);   // the held-back Unbind, as the original made it
        NoteGLStateOwner();
    }
    shader->Bind(this);
    NoteShaderBind();
    LDD_GL_CATCH
}

// SetMaterial with a material on both sides (see the comment block above).
void OpenGLRender::SetMaterialBatched(Material* material)
{
    m_batchActive = true;
    m_batchStates = 0;
    m_batchUnbinds = 0;
    m_batchLastUnit = -1;
    m_batchLastBound = -1;
    m_batchUnboundShader = 0;
    LDD_GL_TRY
    m_material->End(this);
    m_material = material;
    material->Begin(this);
    LDD_GL_CATCH
    FlushBatch();
}

// Ends the batch: the coalesced states, then the held-back unbinds, then the active unit.
void OpenGLRender::FlushBatch()
{
    m_batchActive = false;
    LDD_GL_TRY
    const UInt32 states = m_batchStates;
    m_batchStates = 0;
    if (states & (1u << kBatchLighting))      EnableLighting(m_batchValues[kBatchLighting] != 0);
    if (states & (1u << kBatchLocalViewer))   SetLocalViewer(m_batchValues[kBatchLocalViewer] != 0);
    if (states & (1u << kBatchPolygonOffset)) EnablePolygonOffset(m_batchValues[kBatchPolygonOffset] != 0);
    if (states & (1u << kBatchPolygonMode))   SetPolygonMode(m_batchValues[kBatchPolygonMode]);
    if (states & (1u << kBatchCullMode))      SetCullMode(m_batchValues[kBatchCullMode]);
    if (states & (1u << kBatchFrontFace))     SetFrontFace(m_batchValues[kBatchFrontFace]);
    if (states & (1u << kBatchDepthWrite))    EnableDepthWrite(m_batchValues[kBatchDepthWrite] != 0);
    if (states & (1u << kBatchLineWidth))     SetLineWidth(m_batchValues[kBatchLineWidth]);
    if (Shader* unbound = m_batchUnboundShader) {
        m_batchUnboundShader = 0;
        unbound->Unbind(this);
        NoteGLStateOwner();
    }
    const UInt32 unbinds = m_batchUnbinds;
    m_batchUnbinds = 0;
    for (UInt32 unit = 0; unit < 16; ++unit) {
        if (unbinds & (1u << unit))
            BindTexture(0, 0, unit, false);
    }
    // BindTexture selects its unit first, so the original sequence leaves the unit of its last
    // texture change active.
    if (m_batchLastUnit >= 0 && m_batchLastBound != m_batchLastUnit && m_extensions->m_multitexture)
        glActiveTextureARB(GL_TEXTURE0_ARB + (UInt32)m_batchLastUnit);
    m_batchLastUnit = -1;
    LDD_GL_CATCH
}

} // namespace LEGO
