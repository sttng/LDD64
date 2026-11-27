// LEGO Digital Designer — restored source
// LEGO::OpenGLRender — OpenGL implementation of Render, observing its Window (resize, close);
// OpenGLVertexBufferFactory, PlatformTextLayoutFactory. TU 034 (Win __FILE__ ...\OpenGLRender.cpp):
// Mac 0xf2870-0xf87af, Win 0x4eb6f0-0x4f1ec0 (ctor 0x4ec4b0, dtor 0x4ef400).
// Vtables: Mac 0x70826c (88 slots: Render 0-81, D1/D0 82/83, Initialize 84, OnFinalRelease 85,
// OnResize 86, OnClose 87 — Itanium gives the overriders of WindowObserver's virtuals primary
// entries), WindowObserver-in 0x7083d8 (-8), Observer-in 0x70844c (-0xc), Object-in 0x708478;
// Win 0xa25db4 (83 slots: Render 0-81, Initialize 82), WindowObserver 0xa25f04 (+0x0c),
// Observer 0xa25f64 (+0x14), Object 0xa25f70.
// Layout: Mac sizeof 0x2d8 (Object at +0x2d0); Win 0x2f8 (Object at +0x2f0). Offsets per member.
// The member functions wrap their GL calls in try/catch(...) -> ReportOpenGLCrash(__FILE__,
// __LINE__) (LDD_GL_TRY / LDD_GL_CATCH, OpenGLCrashReport.h), in both builds (Win /EHa also
// catches access violations, Mac only C++ exceptions).
// Member names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "ObjectFactory.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "Geometry.h"
#include "Color.h"
#include "Render.h"
#include "Window.h"
#include "OpenGLExtensions.h"

#if !defined(_WIN32)
#include <OpenGL/OpenGL.h>
#endif

namespace LEGO {

class OpenGLTexture;
class OpenGLVertexBuffer;
class Bitmap;

class OpenGLRender : public Render, private WindowObserver {
public:
    // `window` is kept (reference) and observed; the render adopts the context current at
    // construction (OpenGLWindow::CreateContext made it current) and reads the window size.
    OpenGLRender(UInt32 level, Window* window);                       // Mac 0xf2900, Win 0x4ec4b0 (thiscall(level, window, mostDerived))
    virtual ~OpenGLRender();                                          // Mac 0xf6eb0/0xf7060 (thunks 0xf6f10 …), Win 0x4ef400 (sdd 0x4ec810)

    // ---- Render ------------------------------------------------------------------------------
    virtual ObjectFactory* GetObjectFactory() const;                  // 0  Mac 0xf7990, Win 0x4ebe20
    virtual const Vector2& GetSize() const;                           // 1  Mac 0xf33c0, Win 0x4ecb50
    virtual void SetViewport(const Rect& viewport);                   // 2  Mac 0xf33d0, Win 0x4ecb60
    virtual const Rect& GetViewport() const;                          // 3  Mac 0xf3580, Win 0x4ecca0
    virtual void SetScissor(const Rect& rect);                        // 4  Mac 0xf3590, Win 0x4eccb0
    virtual const Rect& GetScissor() const;                           // 5  Mac 0xf3750, Win 0x4ecdf0
    virtual void DisableScissor();                                    // 6  Mac 0xf3760, Win 0x4ece00
    virtual void SetClearColor(const Color& color);                   // 7  Mac 0xf37d0, Win 0x4ece80
    virtual void Clear(UInt32 buffers);                               // 8  Mac 0xf3860, Win 0x4ecf20
    virtual bool BeginFrame();                                        // 9  Mac 0xf38e0, Win 0x4ecfc0
    virtual void EndFrame();                                          // 10 Mac 0xf3910, Win 0x4ecfe0
    virtual void Present();                                           // 11 Mac 0xf3aa0, Win 0x4ecff0
    virtual void SetCullMode(UInt32 mode);                            // 12 Mac 0xf3b30, Win 0x4ed070
    virtual void SetPolygonMode(UInt32 mode);                         // 13 Mac 0xf3bd0, Win 0x4ed110
    virtual void SetFrontFace(UInt32 winding);                        // 14 Mac 0xf3c50, Win 0x4ed190
    virtual void SetLineWidth(UInt32 width);                          // 15 Mac 0xf3cc0, Win 0x4ed210
    virtual void SetDepthFunc(UInt32 func);                           // 16 Mac 0xf3d40, Win 0x4ed2a0
    virtual void SetPolygonOffset(Real32 factor, Real32 units);       // 17 Mac 0xf3db0, Win 0x4ed320
    virtual void SetAlphaFunc(UInt32 func, Real32 reference);         // 18 Mac 0xf3e30, Win 0x4ed3b0
    virtual void SetBlendFunc(UInt32 source, UInt32 destination);     // 19 Mac 0xf3eb0, Win 0x4ed440
    virtual UInt32 GetMaxClipPlanes() const;                          // 20 Mac 0xf3f30, Win 0x4ed4d0
    virtual void SetClipPlane(UInt32 index, const Plane& plane);      // 21 Mac 0xf3fb0, Win 0x4ed550
    virtual void EnableBlend(bool enable);                            // 22 Mac 0xf40b0, Win 0x4ed660
    virtual void EnableLighting(bool enable);                         // 23 Mac 0xf4130, Win 0x4ed6e0
    virtual void SetLocalViewer(bool enable);                         // 24 Mac 0xf41c0, Win 0x4ed760
    virtual void EnableColorWrite(bool enable);                       // 25 Mac 0xf4230, Win 0x4ed7e0
    virtual void EnableAlphaWrite(bool enable);                       // 26 Mac 0xf42c0, Win 0x4ed870
    virtual void EnableAlphaTest(bool enable);                        // 27 Mac 0xf4350, Win 0x4ed900
    virtual void EnableDepthTest(bool enable);                        // 28 Mac 0xf43d0, Win 0x4ed980
    virtual void EnableDepthWrite(bool enable);                       // 29 Mac 0xf4450, Win 0x4eda00
    virtual void EnablePolygonOffset(bool enable);                    // 30 Mac 0xf44c0, Win 0x4eda80
    virtual void EnableClipPlane(UInt32 index, bool enable);          // 31 Mac 0xf4540, Win 0x4edb00
    virtual void EnableFog(bool enable);                              // 32 Mac 0xf45c0, Win 0x4edb80
    virtual RenderQueue* GetRenderQueue(UInt32 index);                // 33 Mac 0xf4640, Win 0x4edc00
    virtual const char* GetName() const;                              // 34 Mac 0xf49f0, Win 0x4edd00
    virtual Texture* GetRenderTarget() const;                         // 35 Mac 0xf4a20, Win 0x4edd20
    virtual Texture* GetRenderTargetDepthBuffer(Texture* target) const; // 36 Mac 0xf4a30, Win 0x4edd30
    virtual void SetRenderTarget(Texture* target);                    // 37 Mac 0xf4b10, Win 0x4eddb0
    virtual void SetRenderTargetDepthBuffer(Texture* target, Texture* depthBuffer); // 38 Mac 0xf4e20, Win 0x4ee040
    virtual void SetProjectionMatrix(const Matrix3D& projection);     // 39 Mac 0xf4eb0, Win 0x4ee080
    virtual void SetViewMatrix(const Matrix3D& view);                 // 40 Mac 0xf4f90, Win 0x4ee110
    virtual void SetWorldMatrix(const Matrix3D& world);               // 41 Mac 0xf5090, Win 0x4ee1b0
    virtual const Matrix3D& GetProjectionMatrix() const;              // 42 Mac 0xf5190, Win 0x4ee260
    virtual const Matrix3D& GetViewMatrix() const;                    // 43 Mac 0xf51a0, Win 0x4ee270
    virtual const Matrix3D& GetWorldMatrix() const;                   // 44 Mac 0xf51b0, Win 0x5aa5d0 (ICF)
    virtual void SetLight(const Light* light, UInt32 index);          // 45 Mac 0xf51c0, Win 0x4ee280
    virtual void SetMaterialColors(const MaterialColors& colors);     // 46 Mac 0xf5470, Win 0x4ee490
    virtual const MaterialColors& GetMaterialColors() const;          // 47 Mac 0xf5490, Win 0x4ee4b0
    virtual void SetOpacity(Real32 opacity);                          // 48 Mac 0xf54a0, Win 0x4ee4c0
    virtual Real32 GetOpacity() const;                                // 49 Mac 0xf4a10, Win 0x4edd10
    virtual void SetTextureAttachment(Texture* texture, UInt32 attachment, UInt32 unit); // 50 Mac 0xf54d0, Win 0x4ee520
    virtual void SetTexture(Texture* texture, UInt32 unit);           // 51 Mac 0xf56c0, Win 0x4ee6a0
    virtual Texture* GetTexture(UInt32 unit) const;                   // 52 Mac 0xf5870, Win 0x4ee800
    virtual void SetShader(Shader* shader);                           // 53 Mac 0xf5890, Win 0x4ee820
    virtual void SetVertexBuffer(VertexBuffer* buffer);               // 54 Mac 0xf5930, Win 0x4ee8c0
    virtual void BeginVertexBuffer(VertexBuffer* buffer);             // 55 Mac 0xf59b0, Win 0x4ee950
    virtual void EndVertexBuffer(VertexBuffer* buffer);               // 56 Mac 0xf5a20, Win 0x4ee9d0
    virtual void SetMaterial(Material* material);                     // 57 Mac 0xf5a90, Win 0x4eea50
    virtual void SetDefaultMaterial(Material* material);              // 58 Mac 0xf5b30, Win 0x4eeaf0
    virtual void UseDefaultMaterial();                                // 59 Mac 0xf5b50, Win 0x4eeb00
    virtual void ReadPixels(UInt8* buffer, UInt32 width, UInt32 height, UInt32 arg4, UInt32 arg5,
                            bool backBuffer, bool alpha);             // 60 Mac 0xf5b70, Win 0x4eeb20
    virtual void DrawSphere(const Vector3& center, Real32 radius, bool wireframe); // 61 Mac 0xf5fa0, Win 0x4eed70
    virtual void DrawBox(const Vector3& minimum, const Vector3& maximum, bool wireframe); // 62 Mac 0xf6200, Win 0x4eee60
    virtual void DrawVertexBuffer();                                  // 63 Mac 0xf5f10, Win 0x4eece0
    virtual void SetColor(const Color& color);                        // 64 Mac 0xf5f80, Win 0x4eed60
    virtual bool IsOcclusionQuerySupported() const;                   // 65 Mac 0xf7310, Win 0x4ef760
    virtual Texture* BeginOcclusionQueries(Texture* target);          // 66 Mac 0xf73b0, Win 0x4ef830
    virtual UInt32 GetVisibleSampleCount(Mesh* mesh, const Matrix3D& transform); // 67 Mac 0xf7570, Win 0x4ef9b0
    virtual void SwitchRenderTarget(Texture* target);                 // 68 Mac 0xf7780, Win 0x4efd10
    virtual bool SupportsShaders() const;                             // 69 Mac 0xf77b0, Win 0x4efd40
    virtual void SetShaderMaterialColors(bool enable);                // 70 Mac 0xf77e0, Win 0x4efd70
    virtual bool GetShaderMaterialColors() const;                     // 71 Mac 0xf7800, Win 0x4efd80
    virtual bool SupportsFloatTextures() const;                       // 72 Mac 0xf7810, Win 0x4efd90
    virtual bool SupportsMultipleRenderTargets() const;               // 73 Mac 0xf7820, Win 0x4efda0
    virtual UInt32 GetMaxColorAttachments() const;                    // 74 Mac 0xf7830, Win 0x4efdb0
    virtual UInt32 GetMaxTextureSize() const;                         // 75 Mac 0xf7840, Win 0x4efdc0
    virtual UInt32 GetMaxVertexTextureImageUnits() const;             // 76 Mac 0xf7850, Win 0x4efdd0
    virtual UInt32 GetMaxTextureImageUnits() const;                   // 77 Mac 0xf7860, Win 0x4efde0
    virtual UInt32 GetMaxCombinedTextureImageUnits() const;           // 78 Mac 0xf7870, Win 0x4efdf0
    virtual UInt32 GetMaxVertexUniformComponents() const;             // 79 Mac 0xf7880, Win 0x4efe00
    virtual UInt32 GetMaxFragmentUniformComponents() const;           // 80 Mac 0xf7890, Win 0x4efe10
    virtual UInt32 GetMaxVaryingFloats() const;                       // 81 Mac 0xf78a0, Win 0x4efe20

    // ---- OpenGLRender ------------------------------------------------------------------------
    // Creates the render's ObjectFactory and registers the vertex buffer, texture, font, shader
    // and text layout factories (OpenGLFactories.h), sets the default GL state and reads the
    // implementation limits. False when GL_MAX_TEXTURE_SIZE is 0; otherwise GL version >= 1.1.
    virtual bool Initialize(const Render::CreateArgs& args);          // Mac 84 0xf2f00, Win 82 0x4ec850

    // Makes the render's context current (Win: wglMakeCurrent when another one is; Mac: only
    // CGLGetCurrentContext — NSOpenGLContext keeps it current).
    void MakeCurrent();                                               // Mac 0xf3900, Win 0x4ef310 (ESI = this)
    // Fixed-function material for immediate drawing / vertex buffers: glColor4f(diffuse, alpha *
    // opacity) + glMaterial* while lighting is on, white without lighting; nothing when a
    // shader takes the colours (SetShaderMaterialColors).
    void ApplyMaterial();                                             // Mac 0xf60c0, Win 0x4ef640 (ESI = this)
    // Re-uploads a CPU bitmap into a texture image (mutable textures), keeps unit 0's binding.
    void UploadTextureImage(UInt32 bindTarget, UInt32 imageTarget, UInt32 textureName, UInt32 level,
                            UInt32 format, UInt32 type, const Bitmap& bitmap); // Mac 0xf66e0, Win 0x4ef230
    // Textures alive on this render (OnClose releases their GL objects).
    void RegisterTexture(OpenGLTexture* texture);                     // Mac 0xf6780, Win 0x4ef340
    void UnregisterTexture(OpenGLTexture* texture);                   // Mac 0xf69c0, Win 0x4ef360
    const OpenGLExtensions& GetExtensions() const { return *m_extensions; } // (inlined field read)
    Window* GetWindow() const { return m_window; }
#if defined(_WIN32)
    HGLRC GetGLContext() const { return m_context; }
    HDC   GetGLDC() const { return m_dc; }
#else
    CGLContextObj GetGLContext() const { return m_context; }
#endif

protected:
    // Clears the queues, stops observing the window, then the base OnFinalRelease bodies.
    virtual void OnFinalRelease();                                    // Mac 85 0xf6bf0 (thunks 0xf6c90 …), Win 0x4ef3a0
    // WindowObserver: keeps the client size (viewport height reference).
    virtual void OnResize(const Vector2& size, const Vector2& clientSize); // Mac 86 0xf7250 (thunk 0xf7270), Win 0x4ef590
    // WindowObserver: the window (and its context) goes away: release every texture's GL object.
    virtual void OnClose();                                           // Mac 87 0xf7290 (thunk 0xf72d0), Win 0x4ef5b0

private:
    // One texture unit: bound texture, its GL target, its type (Texture::Type; -1 = none bound
    // yet), attachment of a multiple-render-target texture.
    struct TextureUnit {
        Texture* texture;      // +0x00
        UInt32   target;       // +0x04
        Int32    type;         // +0x08
        UInt32   attachment;   // +0x0c (not initialised by the constructor)
        UInt32   glName;       // (added) GL name the last BindTexture bound on the unit
        UInt32   stamp;        // (added) GL-state epoch of that bind (see NoteGLStateChanged)
    };
    typedef BasicMap<UInt32, RefPointer<RenderQueue>, Less<UInt32> > QueueMap;

    // Unbinds everything; Render::EndFrame and render target switches. (name inferred)
    void ResetState();                                                // Mac 0xf3920, Win 0x4ef5f0 (ESI = this)
    void BindTexture(Texture* texture, UInt32 attachment, UInt32 unit, bool setEnv);
    // ---- performance: state batching in SetMaterial (not in the originals) -------------------
    // SetMaterial runs the old material's End and the new one's Begin as one batch. Inside it the
    // GL writes that End undoes and Begin redoes are dropped and the rest coalesced, so the GL
    // state after the batch is the one the original call sequence leaves; see SetMaterialBatched.
    enum BatchState {
        kBatchLighting, kBatchLocalViewer, kBatchPolygonOffset, kBatchPolygonMode,
        kBatchCullMode, kBatchFrontFace, kBatchDepthWrite, kBatchLineWidth, kBatchStateCount
    };
    void SetMaterialBatched(Material* material);
    void FlushBatch();
    void DeferState(UInt32 state, UInt32 value);
    void BatchSetTexture(Texture* texture, UInt32 attachment, UInt32 unit, bool setEnv,
                         bool attachmentCall);
    void BatchSetShader(Shader* shader);
    void NoteBind(UInt32 unit, UInt32 name);
    void NoteShaderBind();
    void NoteGLStateOwner();

    OpenGLExtensions*         m_extensions;          // Mac +0x14, Win +0x20 (owned)
    RefPointer<Window>        m_window;              // Mac +0x18, Win +0x24
    QueueMap                  m_queues;              // Mac +0x1c, Win +0x28
#if defined(_WIN32)
    HWND                      m_hwnd;                // Win +0x4c (only used with m_ownsContext)
    HGLRC                     m_context;             // Win +0x50 wglGetCurrentContext at construction
    HDC                       m_dc;                  // Win +0x54 wglGetCurrentDC
#else
    CGLContextObj             m_context;             // Mac +0x38 CGLGetCurrentContext at construction
#endif
    Rect                      m_scissor;             // Mac +0x3c, Win +0x58
    Rect                      m_viewport;            // Mac +0x4c, Win +0x68
    Matrix3D                  m_projection;          // Mac +0x5c, Win +0x78
    Matrix3D                  m_view;                // Mac +0x9c, Win +0xb8
    Matrix3D                  m_world;               // Mac +0xdc, Win +0xf8
    Vector2                   m_size;                // Mac +0x11c, Win +0x138 window client size
    MaterialColors            m_materialColors;      // Mac +0x124, Win +0x140
    Real32                    m_opacity;             // Mac +0x168, Win +0x184
    Shader*                   m_shader;              // Mac +0x16c, Win +0x188
    Material*                 m_material;            // Mac +0x170, Win +0x18c
    Material*                 m_defaultMaterial;     // Mac +0x174, Win +0x190
    VertexBuffer*             m_vertexBuffer;        // Mac +0x178, Win +0x194
    Texture*                  m_renderTarget;        // Mac +0x17c, Win +0x198 (null = window)
    bool                      m_ownsContext;         // Mac +0x180, Win +0x19c (NOTE: never set)
    bool                      m_frameActive;         // Mac +0x181, Win +0x19d
    bool                      m_colorWrite;          // Mac +0x182, Win +0x19e
    bool                      m_alphaWrite;          // Mac +0x183, Win +0x19f
    RefPointer<ObjectFactory> m_objectFactory;       // Mac +0x184, Win +0x1a0
    TextureUnit               m_textureUnits[16];    // Mac +0x188, Win +0x1a4
    BasicArray<OpenGLTexture*>      m_textures;      // Mac +0x288, Win +0x2a4
    BasicArray<OpenGLVertexBuffer*> m_vertexBuffers; // Mac +0x298, Win +0x2b4 (never filled here)
    bool                      m_gpuProcessing;       // Mac +0x2a8 (not CGL's software renderer), Win +0x2c4 (true)
    UInt32                    m_maxTextureSize;      // Mac +0x2ac, Win +0x2c8 (power of two)
    UInt32                    m_maxColorAttachments; // Mac +0x2b0, Win +0x2cc
    UInt32                    m_maxVertexTextureImageUnits;   // Mac +0x2b4, Win +0x2d0
    UInt32                    m_maxTextureImageUnits;         // Mac +0x2b8, Win +0x2d4
    UInt32                    m_maxCombinedTextureImageUnits; // Mac +0x2bc, Win +0x2d8
    UInt32                    m_maxVertexUniformComponents;   // Mac +0x2c0, Win +0x2dc
    UInt32                    m_maxFragmentUniformComponents; // Mac +0x2c4, Win +0x2e0
    UInt32                    m_maxVaryingFloats;             // Mac +0x2c8, Win +0x2e4
    bool                      m_lighting;            // Mac +0x2cc, Win +0x2e8
    bool                      m_shaderMaterialColors;// Mac +0x2cd, Win +0x2e9
    bool                      m_batchActive;         // (added) inside SetMaterialBatched
    UInt32                    m_batchStates;         // (added) deferred states (BatchState bits)
    UInt32                    m_batchValues[kBatchStateCount]; // (added) their last values
    UInt32                    m_batchUnbinds;        // (added) units with a deferred unbind
    Texture*                  m_batchUnbound[16];    // (added) the texture each one unbound
    Int32                     m_batchLastUnit;       // (added) last unit the original would select
    Int32                     m_batchLastBound;      // (added) unit of the last BindTexture run
    Shader*                   m_batchUnboundShader;  // (added) shader with a deferred Unbind
    UInt32                    m_shaderStamp;         // (added) GL-state epoch of m_shader's Bind

    OpenGLRender(const OpenGLRender&);
    OpenGLRender& operator=(const OpenGLRender&);
};

// (added, performance) Call after GL bindings or the context change behind OpenGLRender's back
// (texture creation, deletion, uploads, context switches); see OpenGLRender::BatchSetTexture.
void NoteGLStateChanged();

// ===========================================================================================
// OpenGLVertexBufferFactory — Mac vtable 0x763978 {D1 0xf79a0, D0 0xf7a00, Object 2-4,
// CreateObject 0xf7a70, Initialize, Shutdown}, Win sdd 0x4ec0f0, CreateObject 0x4ebe30,
// ctor 0x4ec040. 0x14 bytes: SpecificObjectFactory + OpenGLRender* +0x10.
// ===========================================================================================
class OpenGLVertexBufferFactory : public SpecificObjectFactory {
public:
    explicit OpenGLVertexBufferFactory(OpenGLRender* render) : m_render(render) {}
    virtual ~OpenGLVertexBufferFactory() {}
    // CreateArgsRoundEdge -> OpenGLVertexBufferVBORoundEdge (needs SupportsShaders); CreateArgs ->
    // OpenGLVertexBufferVBO for static data with VBOs, else (or when that fails) the client-array
    // OpenGLVertexBuffer; null for more than one texture-coordinate set without multitexture.
    virtual Object* CreateObject(const ObjectFactoryArgs& args);

private:
    OpenGLRender* m_render;   // +0x10
};

// ===========================================================================================
// PlatformTextLayoutFactory — Mac vtable 0x7639c8 (CreateObject 0xf7f50), Win sdd 0x4ec490,
// CreateObject 0x4ec310, ctor 0x4ec3e0. Registered for PlatformTextLayout and TextLayoutRender.
// ===========================================================================================
class PlatformTextLayoutFactory : public SpecificObjectFactory {
public:
    explicit PlatformTextLayoutFactory(OpenGLRender* render) : m_render(render) {}
    virtual ~PlatformTextLayoutFactory() {}
    virtual Object* CreateObject(const ObjectFactoryArgs& args);

private:
    OpenGLRender* m_render;   // +0x10
};

} // namespace LEGO
