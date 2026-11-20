// LEGO Digital Designer — restored source
// LEGO::PostProcessingEffect — a PostProcessor screen effect: the scene goes into the effect's
//   colour target (Begin), Apply composes it into the caller's target with full-screen quads.
//   BrickConnectSceneControl keeps the selected one (+0x500). Names except RTTI ones inferred.
// TU 369 (Mac 0x35a9e0-0x35b36f; Win 0x5bfc10-0x5bfccf, 0x5c37a0-0x5c3d2f); plain Object base.
// Vtables: Mac 0x74fcf8 {D1 0x35b220, D0 0x35b230, OnFinalRelease, AddRef, Release, 5-11};
//   Win 0xa2d0b8 {sdd 0x5bfcb0, OnFinalRelease, AddRef, Release, 4-10}: Win slot = Mac slot - 1.
// Layout (both builds): Render* +0x08, shader Directory* +0x0c (not referenced), round-edge
//   directory +0x10, colour target +0x14, quad +0x18, Rect +0x1c, Rect +0x2c (zero, not read),
//   bool round edges +0x3c; sizeof 0x40. Subclasses (TU 370, 371, 405) add members from +0x40.
// Apply's arguments: from BrickConnectSceneControl::Draw Mac 0x4a4ad and RenderScene Mac 0x45e85.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Geometry.h"
#include "Color.h"
#include "LEGO/String.h"

namespace LEGO {

class Render;
class Directory;
class Texture;
class VertexBuffer;
class Shader;
class Material;
class RenderQueue;
struct Light;

class PostProcessingEffect : public Object {
public:
    // `shaders`: the "Shaders" asset directory (kept raw); `roundEdgeShaders`: its "RoundEdge"
    // subdirectory (null without round edges). Mac 0x35a9e0, Win 0x5c37a0
    PostProcessingEffect(Render* render, Directory* shaders, bool roundEdges,
                         Directory* roundEdgeShaders);
    virtual ~PostProcessingEffect();                                        // Mac 0x35b220/0x35b230, Win 0x5bfc10 / sdd 0x5bfcb0

    // The full-screen quad: 4 vertices, a triangle strip, two 2D texture-coordinate sets.
    // False when the vertex buffer cannot be created. Subclasses add LoadShaders.
    virtual bool Initialize();                                              // 5  Mac 0x35ab10, Win 0x5c3850
    // A new colour target of `imageSize` (the power-of-two buffer size; ABGR, own depth buffer,
    // clamped); `size` is the part in use (subclasses fit the quad to it).
    virtual void Resize(const Vector2& size, const Vector2& imageSize);     // 6  Mac 0x35ace0, Win 0x5c3a60
    // The effect is no longer used: releases the colour target(s).
    virtual void Deactivate();                                              // 7  Mac 0x35ade0, Win 0x5c3bb0
    // Renders into the colour target from here on: target, `clearColor`, Clear(7).
    virtual void Begin(const Color& clearColor);                            // 8  Mac 0x35ae10, Win 0x5c3bd0
    // Composes the scene rendered since Begin into `target` (the control's buffer). `size`: the
    // part of the targets in use; `queue`: the scene queue (flushed again with the effect's
    // own default material by the effects that need scene normals); the three scene lights;
    // the camera's projection, horizontal field of view, aspect (width / height), near / far.
    virtual void Apply(const Vector2& size, RenderQueue* queue, Texture* target,
                       const Light* light0, const Light* light1, const Light* light2,
                       const Matrix3D& projection, Real32 fieldOfView, Real32 aspect,
                       Real32 nearPlane, Real32 farPlane) = 0;              // 9  (Win 8)
    // (Re)loads the shaders (also BrickConnectSceneControl's debug key 'x'); false when one is
    // missing.
    virtual bool LoadShaders() = 0;                                         // 10 (Win 9)
    // The name shown by LDDPostProcessor's drop-down (debug key 't').
    virtual String GetName() const = 0;                                     // 11 (Win 10)

protected:
    // The quad's positions from `rect` (floored): (l, t) (r, t) (l, b) (r, b).
    // Mac 0x35ae60, Win 0x5c3c10
    void SetQuadPositions(const Rect& rect);
    // The quad's texture coordinates (set 0) in the same order. Mac 0x35af80, Win 0x5c3cd0
    void SetQuadTexCoords(const Rect& rect);
    // Fits the quad to `size` of the image of `target` (texture coordinates flipped
    // vertically: the used part is the top of the target image). Inlined in every Resize
    // override (Mac 0x35b630, 0x35cb30, 0x35d320, 0x35ddb0, 0x35ee50). (name inferred)
    void FitQuad(const Vector2& size, Texture* target);
    // The shader descriptor `name` of the shader (`fromRoundEdgeDirectory`: round-edge)
    // directory through the render's factory (Shader::LoadArgs, field1C 1); `current` when the
    // directory is missing (null then) or the shader cannot be loaded. Not referenced.
    // Mac 0x35aff0 (Win inlined)
    Shader* LoadShader(const String& name, Shader* current, bool fromRoundEdgeDirectory);
    // The scene-normals shader and material: "Normals.sh" + SceneNormalsMaterial, or with round
    // edges "RENormals.sh" of the round-edge directory + SceneNormalsRoundEdgeMaterial; false
    // (material unchanged) when the shader is missing. Inlined at the start of
    // SceneNormals / EdgeDetection / SSDO::LoadShaders (Mac 0x35d9c0, 0x35e700, 0x3603b0).
    // (name inferred)
    bool LoadNormalsMaterial(RefPointer<Shader>& shader, RefPointer<Material>& material);

    Render*                  m_render;                    // +0x08
    Directory*               m_shaderDirectory;           // +0x0c
    RefPointer<Directory>    m_roundEdgeShaderDirectory;  // +0x10
    RefPointer<Texture>      m_target;                    // +0x14 colour target (+ depth buffer)
    RefPointer<VertexBuffer> m_quad;                      // +0x18
    Rect                     m_rect1c;                    // +0x1c (not read)
    Rect                     m_rect2c;                    // +0x2c (not read)
    bool                     m_roundEdges;                // +0x3c

private:
    PostProcessingEffect(const PostProcessingEffect&);
    PostProcessingEffect& operator=(const PostProcessingEffect&);
};

} // namespace LEGO
