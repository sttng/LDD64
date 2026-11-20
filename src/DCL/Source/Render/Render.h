// LEGO Digital Designer — restored source
// LEGO::Render — abstract rendering API (only OpenGLRender); Light, MaterialColors, RenderEntry,
// Render::CreateArgs. Header-only: D1/D0 in TU 034 (Mac 0xf78c0/0xf78e0), the one non-pure
// virtual (slot 65) Mac 0xf78b0 / Win 0x4aede0 (ICF). Vtables Mac 0x7084dc (84 slots), Win
// 0xa25c54 (82; dtor in the Object vtable); Render slots equal in both builds
// Slot names inferred from OpenGLRender's GL calls (Mac
// addresses below are OpenGLRender's) and the callers.
// Layout: Mac {vptr, m_level +0x04, Object +0x08} sizeof 0x10; Win {vptr, vbptr, m_level +0x08,
// Object} (Win ctor 0x4ec4b0 stores +8). OpenGLRender's layout: OpenGLRender.h.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "ObjectFactory.h"
#include "Geometry.h"
#include "Color.h"

namespace LEGO {

class RenderQueue;
class Texture;
class Shader;
class VertexBuffer;
class Material;
class Mesh;

// ===========================================================================================
// Light — 0x64 bytes, passed to Render::SetLight (Mac 0xf51c0 reads every field).
// Default lights: BrickConnectSceneControl (Mac 021b 0x32b90). (struct and field names inferred)
// ===========================================================================================
struct Light {
    enum Type { kDirectional = 0, kPoint = 1, kSpot = 2 };

    Light()
        : type(kDirectional), constantAttenuation(1.0f), linearAttenuation(0.0f),
          quadraticAttenuation(0.0f), spotCutoff(180.0f), spotExponent(0.0f), reserved60(0) {}

    UInt32  type;                    // +0x00
    Color   diffuse;                 // +0x04 GL_DIFFUSE
    Color   specular;                // +0x14 GL_SPECULAR
    Color   ambient;                 // +0x24 GL_AMBIENT
    Vector3 position;                // +0x34 GL_POSITION (w = 1) for point/spot lights
    Vector3 direction;               // +0x40 GL_POSITION (w = 0) for directional, GL_SPOT_DIRECTION for spot
    Real32  constantAttenuation;     // +0x4c
    Real32  linearAttenuation;       // +0x50
    Real32  quadraticAttenuation;    // +0x54
    Real32  spotCutoff;              // +0x58 (point lights always get 180)
    Real32  spotExponent;            // +0x5c
    UInt32  reserved60;              // +0x60 (not read by SetLight)
};

// ===========================================================================================
// MaterialColors — 0x44 bytes: fixed-function material + shader material uniforms.
// Render::SetMaterialColors copies it (Mac 0xf5470: rep movsd 0x11 dwords); OpenGLRender's
// ApplyMaterial (Mac 0xf60c0 / Win 0x4ef640) feeds glColor4f(diffuse, a * opacity) and
// glMaterialfv(GL_DIFFUSE/AMBIENT/SPECULAR/EMISSION, GL_SHININESS); Shader::SetMaterialColors
// (Mac 0x235f90) the uniforms. ColoredMaterial keeps one at +0x04. (names inferred)
// ===========================================================================================
struct MaterialColors {
    Color  diffuse;      // +0x00
    Color  ambient;      // +0x10
    Color  specular;     // +0x20
    Color  emissive;     // +0x30
    Real32 shininess;    // +0x40
};

// ===========================================================================================
// RenderEntry — 0x50 bytes, one queued draw (BasicArray<RenderEntry> in RenderQueue, Mac 0xf4730;
// RenderQueue::Add Mac 0x1032e0 fills it). (field names inferred; RenderQueue.h)
// ===========================================================================================
struct RenderEntry {
    RefPointer<Mesh> mesh;        // +0x00
    Matrix3D         transform;   // +0x04
    Real32           depth;       // +0x44 sort key (Win Add 0x4eb900 loads it with fld; the queue
                                  //       predicates Mac 0x1025b0/0x102600 compare it as float);
                                  //       passed to Mesh::Bind
    UInt32           layer;       // +0x48 primary sort key (compared signed)
    Real32           opacity;     // +0x4c (1.0 for the occlusion query draw)
    // A sphere around the mesh at `transform` (world space) for RenderQueue's view culling;
    // cullRadius < 0: never culled.
    Vector3          cullCenter;
    Real32           cullRadius;
};

// ===========================================================================================
// Render
// ===========================================================================================
class Render : public virtual Object {
public:
    // Factory arguments of a render (start-up Mac 0x13a456: {vptr, 2000, 0}); handed to
    // OpenGLRender::Initialize (Mac slot 84). Mac vtable 0x766118 {D1 0x13ad40, D0 0x13ff10},
    // Win sdd 0x5c807d. (field names inferred)
    struct CreateArgs : public ObjectFactoryArgs {
        CreateArgs() : maxRenderEntries(2000), flags(0) {}
        virtual ~CreateArgs() {}
        UInt32 maxRenderEntries;   // +0x04 (2000 at start-up; RenderQueue capacity)
        UInt32 flags;              // +0x08 (0 at start-up)
    };

    // Clear() buffer bits (Mac 0xf3860 maps them to GL_*_BUFFER_BIT).
    enum ClearFlags { kClearColor = 1, kClearDepth = 2, kClearStencil = 4 };
    // SetCullMode (Mac 0xf3b30): 0 = off, 1 = GL_BACK, anything else = GL_FRONT.
    enum CullMode { kCullNone = 0, kCullBack = 1, kCullFront = 2 };

    explicit Render(UInt32 level) : m_level(level) {}      // inlined (Win base ctor part of 0x4ec4b0)

    // ---- slots 0-11: frame, viewport ----------------------------------------------------
    // The render's own ObjectFactory (created by OpenGLRender::Initialize; texture, vertex
    // buffer, shader and text-layout factories are registered there).
    virtual ObjectFactory* GetObjectFactory() const = 0;              // 0  Mac 0xf7990 (+0x184)
    virtual const Vector2& GetSize() const = 0;                       // 1  Mac 0xf33c0 window client size
    virtual void SetViewport(const Rect& viewport) = 0;               // 2  Mac 0xf33d0 glViewport
    virtual const Rect& GetViewport() const = 0;                      // 3  Mac 0xf3580
    virtual void SetScissor(const Rect& rect) = 0;                    // 4  Mac 0xf3590 glScissor + enable
    virtual const Rect& GetScissor() const = 0;                       // 5  Mac 0xf3750
    virtual void DisableScissor() = 0;                                // 6  Mac 0xf3760
    virtual void SetClearColor(const Color& color) = 0;               // 7  Mac 0xf37d0
    virtual void Clear(UInt32 buffers) = 0;                           // 8  Mac 0xf3860 (ClearFlags)
    virtual bool BeginFrame() = 0;                                    // 9  Mac 0xf38e0
    // Unbinds shader, material, all 16 texture units and the vertex buffer.
    virtual void EndFrame() = 0;                                      // 10 Mac 0xf3910 (-> 0xf3920)
    virtual void Present() = 0;                                       // 11 Mac 0xf3aa0 flushBuffer / SwapBuffers

    // ---- slots 12-32: fixed-function state ----------------------------------------------
    virtual void SetCullMode(UInt32 mode) = 0;                        // 12 Mac 0xf3b30 (CullMode)
    virtual void SetPolygonMode(UInt32 mode) = 0;                     // 13 Mac 0xf3bd0 (table 0x7638f4)
    virtual void SetFrontFace(UInt32 winding) = 0;                    // 14 Mac 0xf3c50 (table 0x763900)
    virtual void SetLineWidth(UInt32 width) = 0;                      // 15 Mac 0xf3cc0 (UInt32 → float), Win 0x4ed210
    virtual void SetDepthFunc(UInt32 func) = 0;                       // 16 Mac 0xf3d40
    virtual void SetPolygonOffset(Real32 factor, Real32 units) = 0;   // 17 Mac 0xf3db0
    virtual void SetAlphaFunc(UInt32 func, Real32 reference) = 0;     // 18 Mac 0xf3e30
    virtual void SetBlendFunc(UInt32 source, UInt32 destination) = 0; // 19 Mac 0xf3eb0
    virtual UInt32 GetMaxClipPlanes() const = 0;                      // 20 Mac 0xf3f30 GL_MAX_CLIP_PLANES
    virtual void SetClipPlane(UInt32 index, const Plane& plane) = 0;  // 21 Mac 0xf3fb0 (in view space)
    virtual void EnableBlend(bool enable) = 0;                        // 22 Mac 0xf40b0 GL_BLEND
    virtual void EnableLighting(bool enable) = 0;                     // 23 Mac 0xf4130 GL_LIGHTING
    virtual void SetLocalViewer(bool enable) = 0;                     // 24 Mac 0xf41c0 GL_LIGHT_MODEL_LOCAL_VIEWER
    virtual void EnableColorWrite(bool enable) = 0;                   // 25 Mac 0xf4230 glColorMask rgb
    virtual void EnableAlphaWrite(bool enable) = 0;                   // 26 Mac 0xf42c0 glColorMask a
    virtual void EnableAlphaTest(bool enable) = 0;                    // 27 Mac 0xf4350 GL_ALPHA_TEST
    virtual void EnableDepthTest(bool enable) = 0;                    // 28 Mac 0xf43d0 GL_DEPTH_TEST
    virtual void EnableDepthWrite(bool enable) = 0;                   // 29 Mac 0xf4450 glDepthMask
    virtual void EnablePolygonOffset(bool enable) = 0;                // 30 Mac 0xf44c0 GL_POLYGON_OFFSET_FILL
    virtual void EnableClipPlane(UInt32 index, bool enable) = 0;      // 31 Mac 0xf4540 GL_CLIP_PLANE0 + i
    virtual void EnableFog(bool enable) = 0;                          // 32 Mac 0xf45c0 GL_FOG

    // ---- slots 33-38: queues, render targets --------------------------------------------
    // Queue `index`, created on first use (capacity 2000 entries).
    virtual RenderQueue* GetRenderQueue(UInt32 index) = 0;            // 33 Mac 0xf4640
    virtual const char* GetName() const = 0;                          // 34 Mac 0xf49f0 "OpenGL"
    virtual Texture* GetRenderTarget() const = 0;                     // 35 Mac 0xf4a20 (+0x17c)
    // Depth buffer texture attached to the FBO render target `target` (or to the current
    // target when `target` is null / no FBO target); null otherwise. (name inferred; R2.3: the
    // body returns the RT's +0x64 / MRT's +0x74 depth texture, not the framebuffer name)
    virtual Texture* GetRenderTargetDepthBuffer(Texture* target) const = 0; // 36 Mac 0xf4a30, Win 0x4edd30
    // Null = the window. Switches FBO (or CGL/WGL pbuffer context without FBOs).
    virtual void SetRenderTarget(Texture* target) = 0;                // 37 Mac 0xf4b10
    // Attaches a depth texture to a 2D FBO render target (current one when null).
    virtual void SetRenderTargetDepthBuffer(Texture* target, Texture* depthBuffer) = 0; // 38 Mac 0xf4e20

    // ---- slots 39-49: transforms, lights, material colours ------------------------------
    virtual void SetProjectionMatrix(const Matrix3D& projection) = 0; // 39 Mac 0xf4eb0 GL_PROJECTION
    // Sets the view and resets the world matrix to identity (GL_MODELVIEW = view).
    virtual void SetViewMatrix(const Matrix3D& view) = 0;             // 40 Mac 0xf4f90
    // GL_MODELVIEW = view * world.
    virtual void SetWorldMatrix(const Matrix3D& world) = 0;           // 41 Mac 0xf5090
    virtual const Matrix3D& GetProjectionMatrix() const = 0;          // 42 Mac 0xf5190
    virtual const Matrix3D& GetViewMatrix() const = 0;                // 43 Mac 0xf51a0
    virtual const Matrix3D& GetWorldMatrix() const = 0;               // 44 Mac 0xf51b0
    // Null disables GL_LIGHT0 + index.
    virtual void SetLight(const Light* light, UInt32 index) = 0;      // 45 Mac 0xf51c0
    virtual void SetMaterialColors(const MaterialColors& colors) = 0; // 46 Mac 0xf5470
    virtual const MaterialColors& GetMaterialColors() const = 0;      // 47 Mac 0xf5490
    // Clamped to [0, 1]; multiplies the diffuse alpha.
    virtual void SetOpacity(Real32 opacity) = 0;                      // 48 Mac 0xf54a0
    virtual Real32 GetOpacity() const = 0;                            // 49 Mac 0xf4a10

    // ---- slots 50-59: textures, shader, vertex buffer, material -------------------------
    // Binds texture attachment `attachment` of `texture` (GL_MODULATE) on texture unit `unit`.
    virtual void SetTextureAttachment(Texture* texture, UInt32 attachment, UInt32 unit) = 0; // 50 Mac 0xf54d0
    virtual void SetTexture(Texture* texture, UInt32 unit) = 0;       // 51 Mac 0xf56c0
    virtual Texture* GetTexture(UInt32 unit) const = 0;               // 52 Mac 0xf5870
    // Unbinds the previous shader (Shader::Unbind) and binds the new one (Shader::Bind).
    virtual void SetShader(Shader* shader) = 0;                       // 53 Mac 0xf5890
    // Binds the arrays (OpenGLVertexBuffer slot 35) when the buffer changes.
    virtual void SetVertexBuffer(VertexBuffer* buffer) = 0;           // 54 Mac 0xf5930
    virtual void BeginVertexBuffer(VertexBuffer* buffer) = 0;         // 55 Mac 0xf59b0 (OpenGL VB slot 36)
    virtual void EndVertexBuffer(VertexBuffer* buffer) = 0;           // 56 Mac 0xf5a20 (OpenGL VB slot 37)
    // Ends the previous material (Material::End) and begins the new one (Material::Begin).
    virtual void SetMaterial(Material* material) = 0;                 // 57 Mac 0xf5a90
    virtual void SetDefaultMaterial(Material* material) = 0;          // 58 Mac 0xf5b30
    virtual void UseDefaultMaterial() = 0;                            // 59 Mac 0xf5b50

    // ---- slots 60-67: read-back, debug drawing, occlusion -------------------------------
    // Copies the colour buffer (current target, or front/back buffer of the window) into
    // `buffer` (RGB or RGBA rows, top-down). (signature inferred: 7 arguments, Win ret 0x1c;
    // the two UInt32 in the middle are not read in the Mac body)
    virtual void ReadPixels(UInt8* buffer, UInt32 width, UInt32 height, UInt32 arg4, UInt32 arg5,
                            bool backBuffer, bool alpha) = 0;         // 60 Mac 0xf5b70 (Win 0x4eeb20: GL_BACK when set)
    virtual void DrawSphere(const Vector3& center, Real32 radius, bool wireframe) = 0; // 61 Mac 0xf5fa0
    virtual void DrawBox(const Vector3& minimum, const Vector3& maximum, bool wireframe) = 0; // 62 Mac 0xf6200
    // Draws the current vertex buffer (OpenGL VB slot 38).
    virtual void DrawVertexBuffer() = 0;                              // 63 Mac 0xf5f10
    virtual void SetColor(const Color& color) = 0;                    // 64 Mac 0xf5f80 glColor4fv
    virtual bool IsOcclusionQuerySupported() const { return false; }  // 65 Mac 0xf78b0 (OpenGL 0xf7310)
    // Prepares an occlusion pass into `target`; returns the previous render target.
    virtual Texture* BeginOcclusionQueries(Texture* target) = 0;      // 66 Mac 0xf73b0
    // Samples of `mesh` that pass the depth test (ARB_occlusion_query through queue 0).
    virtual UInt32 GetVisibleSampleCount(Mesh* mesh, const Matrix3D& transform) = 0; // 67 Mac 0xf7570

    // ---- slots 68-81: switches, capabilities --------------------------------------------
    // SetRenderTarget + EndFrame. (name inferred)
    virtual void SwitchRenderTarget(Texture* target) = 0;             // 68 Mac 0xf7780
    // GPU vertex+fragment processing and GL 2.0 or ARB_shader_objects.
    virtual bool SupportsShaders() const = 0;                         // 69 Mac 0xf77b0
    // With a shader bound, skip the fixed-function material colours. (name inferred)
    virtual void SetShaderMaterialColors(bool enable) = 0;            // 70 Mac 0xf77e0 (+0x2cd)
    virtual bool GetShaderMaterialColors() const = 0;                 // 71 Mac 0xf7800
    virtual bool SupportsFloatTextures() const = 0;                   // 72 Mac 0xf7810 (caps +6)
    virtual bool SupportsMultipleRenderTargets() const = 0;           // 73 Mac 0xf7820 (caps +0xd)
    virtual UInt32 GetMaxColorAttachments() const = 0;                // 74 Mac 0xf7830 GL_MAX_COLOR_ATTACHMENTS_EXT
    virtual UInt32 GetMaxTextureSize() const = 0;                     // 75 Mac 0xf7840 GL_MAX_TEXTURE_SIZE (power of 2)
    virtual UInt32 GetMaxVertexTextureImageUnits() const = 0;         // 76 Mac 0xf7850
    virtual UInt32 GetMaxTextureImageUnits() const = 0;               // 77 Mac 0xf7860
    virtual UInt32 GetMaxCombinedTextureImageUnits() const = 0;       // 78 Mac 0xf7870
    virtual UInt32 GetMaxVertexUniformComponents() const = 0;         // 79 Mac 0xf7880
    virtual UInt32 GetMaxFragmentUniformComponents() const = 0;       // 80 Mac 0xf7890
    virtual UInt32 GetMaxVaryingFloats() const = 0;                   // 81 Mac 0xf78a0

    // Declared last (Mac D1/D0 = slots 82/83; Win: in the Object vtable).
    virtual ~Render() {}                                              // Mac 0xf78c0/0xf78e0

    UInt32 GetLevel() const { return m_level; }                       // (name inferred)

protected:
    UInt32 m_level;   // Mac +0x04, Win +0x08: compatibility level given to the caps init (name inferred)

private:
    Render(const Render&);
    Render& operator=(const Render&);
};

} // namespace LEGO
