// LEGO Digital Designer — restored source
// TU 065 "RenderControl.cpp" (Mac 0x1523d0-0x15811b, Win 0x50ceae-0x50fa32): ControlDrawArgs
// helpers, BufferedAbleRenderControl, RenderControl, RenderControlManager (+ RenderRoot), the
// Mac-only MacWindowResizeRootControl and the ProxyFactory<T> instances; also RenderControl's
// weak dtor / OnFinalRelease / observer overriders (TU 028 on Mac, 0xec2b0-0xec730) and the
// BufferedAbleRenderControl inlines of TU 021 (ctor 0x2fa20, OnBufferingDisabled 0x51020, slot
// 37 0x51060). Layouts and slots: the headers.
#include "RenderControl.h"
#include "RenderControlManager.h"
#include "ControlManager.h"
#include "Window.h"
#include "Render.h"
#include "Texture.h"
#include "VertexBuffer.h"
#include "Shader.h"
#include "TextLayout.h"
#include "Font.h"
#include "ImageLoader.h"
#include "ProxyFactory.h"
#include "MathUtil.h"

#include <math.h>

namespace LEGO {

namespace {

// Nearest integer value, ties up (Win 0x4c9630 LEGO::RoundToNearest; inlined on the Mac).
inline Real32 RoundToNearest(Real32 value)
{
    Real32 lower = (Real32)floor((double)value);
    Real32 upper = (Real32)ceil((double)value);
    return value - lower < upper - value ? lower : upper;
}

inline Real32 Floor(Real32 value) { return (Real32)floor((double)value); }

// The smallest power of two >= `value` (1 for values <= 1). (inlined everywhere)
inline Int32 PowerOfTwo(Int32 value)
{
    Int32 result = 1;
    while (result < value)
        result *= 2;
    return result;
}
inline UInt32 PowerOfTwo(UInt32 value)
{
    UInt32 result = 1;
    while (result < value)
        result *= 2;
    return result;
}

// Float to integer as the originals convert (truncation through a 64-bit integer).
inline Int32 Truncate(Real32 value) { return (Int32)(Int64)value; }

// The 9-slice index list of the frame geometry (4 x 4 vertices, row-major; the centre quad is
// last, so 0x30 indices leave it out). Mac 0x633bd0, Win 0xa280b8.
const UInt32 kFrameIndices[0x36] = {
    0, 1, 4,   4, 1, 5,   1, 2, 5,   5, 2, 6,   2, 3, 6,   6, 3, 7,
    4, 5, 8,   8, 5, 9,   6, 7, 10,  10, 7, 11, 8, 9, 12,  12, 9, 13,
    9, 10, 13, 13, 10, 14, 10, 11, 14, 14, 11, 15, 5, 6, 9,  9, 6, 10
};

// Primitive types of VertexBuffer::SetPrimitiveType used here (GL mode table Mac 0x763d80).
enum { kPrimitiveLines = 1, kPrimitiveTriangles = 3, kPrimitiveTriangleStrip = 4 };   // (names inferred)

// Alpha above which a control is hit-testable (Mac 0x633c00 area, Win 0x37800080 = 1/65535).
const Real32 kVisibleAlpha = 1.5259022e-05f;

// Render::Clear bits: colour | depth | stencil.
enum { kClearAll = 7 };

// Render::SetBlendFunc factor indices (OpenGLRender kBlendFactors).
enum { kBlendOne = 1, kBlendSourceAlpha = 2, kBlendOneMinusSourceAlpha = 3 };

// The four corners of `rect` (floored), z = 0, in the order the quad buffer draws them.
void QuadPositions(Vector3* positions, const Rect& rect)
{
    Real32 x0 = Floor(rect.m_min.x);
    Real32 y0 = Floor(rect.m_min.y);
    Real32 x1 = Floor(rect.m_max.x);
    Real32 y1 = Floor(rect.m_max.y);
    positions[0] = Vector3(x0, y0, 0.0f);
    positions[1] = Vector3(x1, y0, 0.0f);
    positions[2] = Vector3(x0, y1, 0.0f);
    positions[3] = Vector3(x1, y1, 0.0f);
}

} // namespace

// ===========================================================================================
// ControlDrawArgs
// ===========================================================================================

// Mac 0x1523d0, Win 0x50d259
void ControlDrawArgs::Setup(bool viewport3D)
{
    if (!viewport3D) {
        if (m_state != kState2D) {
            m_state = kState2D;
            Texture* target = m_render->GetRenderTarget();
            Vector2 size = target ? target->GetSize() : Vector2(m_render->GetSize());
            Rect viewport(size);
            m_render->SetViewport(viewport);
            Matrix3D matrix;
            m_render->SetViewMatrix(matrix);
            matrix.SetOrthographic(viewport, -1.0f, 1.0f);
            m_render->SetProjectionMatrix(matrix);
            m_render->SetMaterial(0);
            m_render->SetTexture(0, 0);
            m_render->EnableDepthTest(false);
            m_render->EnableLighting(false);
            m_render->SetCullMode(0);
#if defined(_WIN32)
            m_render->SetBlendFunc(kBlendSourceAlpha, kBlendOneMinusSourceAlpha);
#else
            // NOTE: the Mac build draws its 2D layer with premultiplied alpha (GL_ONE).
            m_render->SetBlendFunc(kBlendOne, kBlendOneMinusSourceAlpha);
#endif
            m_render->EnableBlend(true);
        }
        Matrix3D world;
        Real32* m = world;
        m[0] = m_scale.x * m_targetScale.x;
        m[5] = m_scale.y * m_targetScale.y;
        m[12] = Floor(m_targetScale.x * m_offset.x);
        m[13] = Floor(m_targetScale.y * m_offset.y);
        m_render->SetWorldMatrix(world);
    } else {
        Vector2 position(m_offset.x * m_targetScale.x, m_offset.y * m_targetScale.y);
        Vector2 size(m_size.x * m_scale.x * m_targetScale.x, m_size.y * m_scale.y * m_targetScale.y);
        m_render->SetViewport(Rect(position, size));
        if (m_state != kState3D) {
            m_state = kState3D;
            m_render->SetMaterial(0);
            m_render->SetTexture(0, 0);
            m_render->EnableDepthTest(true);
            m_render->EnableLighting(true);
            m_render->SetCullMode(1);
            m_render->SetBlendFunc(kBlendSourceAlpha, kBlendOneMinusSourceAlpha);
            m_render->EnableBlend(true);
        }
    }
    Rect scissor(m_clip);
    scissor.Scale(m_targetScale);
    m_render->SetScissor(scissor);
}

// Mac 0x152740, Win 0x50d48f
void ControlDrawArgs::MultiplyWorldMatrix(const Matrix3D& m)
{
    Matrix3D world(m_render->GetWorldMatrix());
    world.Multiply(m);
    m_render->SetWorldMatrix(world);
}

// Mac 0x1527a0, Win 0x50d4cd
void ControlDrawArgs::DrawTexture(const Rect& rect, Texture* texture, const Rect& textureRect,
                                  const Color& color, bool blend)
{
    Color colors[4] = { color, color, color, color };
    m_quad->SetColors(colors, 0, 4, 0);
    Vector3 positions[4];
    QuadPositions(positions, rect);
    m_quad->SetPositions(positions, 0, 4, 0);
    Real32 texCoords[8] = {
        textureRect.m_min.x, textureRect.m_min.y, textureRect.m_max.x, textureRect.m_min.y,
        textureRect.m_min.x, textureRect.m_max.y, textureRect.m_max.x, textureRect.m_max.y
    };
    m_quad->SetTexCoords(texCoords, 0);
    m_render->SetVertexBuffer(m_quad);
    m_render->SetTexture(texture, 0);
    m_render->EnableBlend(blend);
    m_render->DrawVertexBuffer();
}

// Mac 0x1529a0, Win 0x50d562
void ControlDrawArgs::DrawRect(const Rect& rect, const Color& color, bool blend)
{
    Color colors[4] = { color, color, color, color };
    m_quad->SetColors(colors, 0, 4, 0);
    Vector3 positions[4];
    QuadPositions(positions, rect);
    m_quad->SetPositions(positions, 0, 4, 0);
    m_render->SetVertexBuffer(m_quad);
    m_render->SetTexture(0, 0);
    m_render->EnableBlend(blend);
    m_render->DrawVertexBuffer();
}

// Mac 0x152b50, Win 0x50d5a5
void ControlDrawArgs::DrawLine(const Vector2& a, const Vector2& b, const Color& color, bool blend)
{
    Color colors[2] = { color, color };
    m_lines->SetColors(colors, 0, 2, 0);
    Vector3 positions[2] = {
        Vector3(Floor(a.x), Floor(a.y), 0.0f),
        Vector3(Floor(b.x), Floor(b.y), 0.0f)
    };
    m_lines->SetPositions(positions, 0, 2, 0);
    m_render->SetVertexBuffer(m_lines);
    m_render->SetTexture(0, 0);
    m_render->EnableBlend(blend);
    m_render->DrawVertexBuffer();
}

// Win 0x50cbd8 (inlined on the Mac, e.g. 0x156360)
Rect ControlDrawArgs::GetLocalClip() const
{
    return Rect((Real32)(((EvalReal)m_clip.m_min.x - m_offset.x) / m_scale.x),
                (Real32)(((EvalReal)m_clip.m_min.y - m_offset.y) / m_scale.y),
                (Real32)(((EvalReal)m_clip.m_max.x - m_offset.x) / m_scale.x),
                (Real32)(((EvalReal)m_clip.m_max.y - m_offset.y) / m_scale.y));
}

// Win 0x50d219 (ESI = this); inlined on the Mac (0x153770, 0x154060)
void ControlDrawArgs::BeginTarget(Texture* target, const Color& clearColor, const Vector2& scale)
{
    m_targetScale = scale;
    m_state = kStateNone;
    m_render->SetRenderTarget(target);
    m_render->DisableScissor();
    m_render->SetClearColor(clearColor);
    m_render->Clear(kClearAll);
}

// ===========================================================================================
// BufferedAbleRenderControl
// ===========================================================================================

// Mac 0x2fa20 (C2 with VTT); Win inlined into RenderControl's constructor 0x50e3bc
BufferedAbleRenderControl::BufferedAbleRenderControl(ControlManager* manager)
    : Control(manager),
      m_color(1.0f, 1.0f, 1.0f, 1.0f),
      m_clearColor(0.0f, 0.0f, 0.0f, 1.0f),
      m_bufferSize(),
      m_bufferScale(1.0f, 1.0f),
      m_alphaMode(0),
      m_wrapMode(1)
{
    SetHitTestable(true);
}

// Mac 0x50f30/0x50fe0
BufferedAbleRenderControl::~BufferedAbleRenderControl()
{
}

// Mac 0x153c40, Win 0x50e0b5
void BufferedAbleRenderControl::OnRectangleChanged(const Vector2& oldPosition, const Vector2& oldSize)
{
    if (m_inputFlags & 4) {
        IntVector2 size(Truncate(Floor(m_size.x)), Truncate(Floor(m_size.y)));
        if (size.x != 0 && size.y != 0 &&
            (m_bufferSize.x < size.x || size.x < m_bufferSize.x >> 1 ||
             m_bufferSize.y < size.y || size.y < m_bufferSize.y >> 1)) {
            m_bufferSize.x = PowerOfTwo(size.x);
            m_bufferSize.y = PowerOfTwo(size.y);
            m_buffer = 0;
            m_buffer = CreateBuffer(m_bufferSize);
            if (!m_buffer)
                SetInputFlags(m_inputFlags & ~4u);
        }
    }
    Control::OnRectangleChanged(oldPosition, oldSize);
}

// Mac 0x153dd0, Win 0x50e181
bool BufferedAbleRenderControl::OnBufferingEnabled()
{
    m_bufferSize.x = Truncate(Floor(m_size.x));
    m_bufferSize.y = Truncate(Floor(m_size.y));
    if (m_bufferSize.y != 0 && m_bufferSize.x != 0) {
        m_bufferSize.x = PowerOfTwo(m_bufferSize.x);
        m_bufferSize.y = PowerOfTwo(m_bufferSize.y);
        m_buffer = CreateBuffer(m_bufferSize);
        if (!m_buffer)
            return false;
    }
    return true;
}

// Mac 0x51020, Win 0x456070
void BufferedAbleRenderControl::OnBufferingDisabled()
{
    m_buffer = 0;
}

// Mac 0x153ed0, Win 0x50e20c
void BufferedAbleRenderControl::EndDraw(ControlDrawArgs& args)
{
    args.Setup(false);
    Vector2 imageSize = m_buffer->GetImageSize();
    // The render target's rows are bottom-up: flipped texture coordinates of the used part.
    Rect textureRect(0.0f, imageSize.y, (Real32)((EvalReal)m_bufferScale.x * m_size.x),
                     (Real32)((EvalReal)m_buffer->GetImageSize().y -
                              (EvalReal)m_bufferScale.y * m_size.y));
    textureRect.Divide(m_buffer->GetImageSize());
#if !defined(_WIN32)
    args.m_render->SetBlendFunc(kBlendSourceAlpha, kBlendOneMinusSourceAlpha);
#endif
    Rect rect(m_size);
    bool blend = true;
    if (m_color.a == 1.0f)
        blend = m_buffer->GetFormat() != 0;
    args.DrawTexture(rect, m_buffer.Get(), textureRect, m_color, blend);
#if !defined(_WIN32)
    args.m_render->SetBlendFunc(kBlendOne, kBlendOneMinusSourceAlpha);
#endif
}

// Mac 0x154060, Win 0x50e2df
void BufferedAbleRenderControl::BeginDraw(ControlDrawArgs& args)
{
    args.BeginTarget(m_buffer.Get(), m_clearColor, m_bufferScale);
}

// Mac 0x1540e0, Win 0x50e301
Texture* BufferedAbleRenderControl::CreateBuffer(const IntVector2& size)
{
    Texture::RenderTargetCreateArgs2D args;
    args.format = 0x40000420;
    args.depthBuffer = true;
    args.wrapMode = m_wrapMode;
    args.alphaMode = m_alphaMode;
    Real32 width = (Real32)size.x * m_bufferScale.x;
    args.width = PowerOfTwo((UInt32)(Int64)RoundToNearest(width));
    Real32 height = (Real32)size.y * m_bufferScale.y;
    args.height = PowerOfTwo((UInt32)(Int64)RoundToNearest(height));
    Object* object = m_manager->GetObjectFactory()->CreateObject(typeid(Texture), args);
    return object ? dynamic_cast<Texture*>(object) : 0;
}

// Mac 0x51060, Win 0x459790 (empty)
void BufferedAbleRenderControl::OnColorChanged()
{
}

// Mac 0x1539a0, Win 0x50df13
void BufferedAbleRenderControl::SetColor(const Color& color)
{
    Color clamped(color);
#if !defined(_WIN32)
    clamped.PremultiplyAlpha();   // the Mac build keeps premultiplied colours
#endif
    clamped.Clamp();
    if (m_color == clamped)
        return;
    m_color = clamped;
    SetHitTestable(m_color.a > kVisibleAlpha);
    OnColorChanged();
    Redraw();
}

// Mac 0x153900, Win 0x50de7e
void BufferedAbleRenderControl::SetOpacity(Real32 opacity)
{
    Real32 value = opacity <= 1.0f ? opacity : 1.0f;
    value = 0.0f <= value ? value : 0.0f;
    if (m_color.a == value)
        return;
    m_color.a = value;
#if !defined(_WIN32)
    // NOTE: Mac quirk — the already premultiplied colour is multiplied by the new alpha again.
    m_color.PremultiplyAlpha();
#endif
    SetHitTestable(m_color.a > kVisibleAlpha);
    OnColorChanged();
    Redraw();
}

// Mac 0x153a60, Win 0x50df9f
void BufferedAbleRenderControl::SetClearColor(const Color& color)
{
    Color clamped(color);
#if !defined(_WIN32)
    clamped.PremultiplyAlpha();
#endif
    clamped.Clamp();
    if (m_clearColor == clamped)
        return;
    m_clearColor = clamped;
    if (m_buffered)
        Redraw();
}

// Mac 0x153af0, Win 0x50dfee
void BufferedAbleRenderControl::SetBufferOptions(UInt32 alphaMode, UInt32 wrapMode, const Vector2& scale)
{
    if (m_alphaMode == alphaMode && m_wrapMode == wrapMode && m_bufferScale == scale)
        return;
    m_bufferScale = scale;
    m_alphaMode = alphaMode;
    m_wrapMode = wrapMode;
    if ((m_inputFlags & 4) && m_bufferSize.x != 0 && m_bufferSize.y != 0) {
        m_buffer = 0;
        m_buffer = CreateBuffer(m_bufferSize);
        if (!m_buffer)
            SetInputFlags(m_inputFlags & ~4u);
    }
    if (m_buffered)
        Redraw();
}

// (inlined everywhere: Mac `if (!manager->+0xd0) ControlManager::fn_db0d0(this, 0)`, Win 0x44b400)
void BufferedAbleRenderControl::Redraw()
{
    if (!m_manager->IsUpdateLocked())
        Invalidate(false);
}

// ===========================================================================================
// RenderControl
// ===========================================================================================

// Mac 0x154530 (C2 with VTT), Win 0x50e3bc
RenderControl::RenderControl(ControlManager* manager)
    : BufferedAbleRenderControl(manager),
      m_transparent(false),
      m_drawAsFrame(false),
      m_drawFrameCenter(true),
      m_tileImage(false),
      m_imageRect(0.0f, 0.0f, 0.0f, 0.0f),
      m_textOffset(0.0f, 0.0f),
      m_frameHorizontal(),
      m_frameVertical()
{
}

// Mac 0xec2b0/0xec310 (body 0xeb2f0)
RenderControl::~RenderControl()
{
}

// Mac 0xec380
void RenderControl::OnFinalRelease()
{
    Control::OnFinalRelease();
    TextLayoutObserver::OnFinalRelease();
    TextureObserver::OnFinalRelease();
}

// Mac 0xec430 (thunk 0xec5c0), Win 0x50d0f9
void RenderControl::OnTextLayoutChanged(UInt32 /*arg*/)
{
    Redraw();
}

// Mac 0xec460 (thunk 0xec730), Win 0x50d0ee
void RenderControl::OnTextureChanged(UInt32 /*face*/)
{
    Redraw();
}

// Mac 0x154790, Win 0x50e5b5
bool RenderControl::Initialize()
{
    SetInputFlags(m_inputFlags | kInputInitialized);
    return true;
}

// Win 0x50d0b4
Color RenderControl::GetDrawColor() const
{
    if (m_buffered)
        return Color(1.0f, 1.0f, 1.0f, 1.0f);
    return m_color;
}

// Win 0x44b660
void RenderControl::SetTransparent(bool transparent)
{
    m_transparent = transparent;
    if (!m_image)
        Redraw();
}

// Mac 0x1547c0
void RenderControl::GetImage(RefPointer<Texture>& image, Rect& imageRect) const
{
    image = m_image;
    imageRect.Set(m_imageRect);
}

// Mac 0x154830, Win 0x50e5cb
void RenderControl::SetImage(Texture* image, const Rect* imageRect)
{
    Rect rect(imageRect ? *imageRect
                        : Rect(Vector2(), image ? image->GetSize() : Vector2()));
    if (m_image.Get() == image) {
        if (m_imageRect == rect)
            return;
    } else {
        if (image)
            TextureObserver::Attach(image);
        else
            TextureObserver::Detach();
        m_image = image;
    }
    m_imageRect.Set(rect);
    Redraw();
    if (m_image) {
        if (m_frameBuffer)
            UpdateFrame(true, false, false);
        if (!m_frameBufferInUse && !m_tileImage && (m_inputFlags & kInputInitialized))
            SetSize(rect.GetAbsoluteSize());
    }
}

// Mac 0x155520, Win 0x50e705
void RenderControl::SetImage(const String& path)
{
    Rect rect(0.0f, 0.0f, 0.0f, 0.0f);
    RefPointer<Texture> image = ImageLoader::GetInstance()->GetImage(path, rect);
    SetImage(image.Get(), &rect);
}

// Mac 0x1555f0, Win 0x50e796
bool RenderControl::SetImageRect(const Rect* imageRect)
{
    Rect rect(imageRect ? *imageRect
                        : Rect(Vector2(), m_image ? m_image->GetSize() : Vector2()));
    if (rect == m_imageRect)
        return true;
    m_imageRect.Set(rect);
    Redraw();
    if (m_image) {
        if (m_frameBuffer)
            UpdateFrame(true, false, false);
        if (!m_frameBufferInUse && !m_tileImage && (m_inputFlags & kInputInitialized))
            SetSize(rect.GetAbsoluteSize());
    }
    return true;
}

// Mac 0x155710, Win 0x50e875
void RenderControl::SetFrame(const Vector3& horizontal, const Vector3& vertical, bool drawCenter)
{
    if (m_frameHorizontal == horizontal && m_frameVertical == vertical &&
        m_drawFrameCenter == drawCenter)
        return;
    m_frameHorizontal = horizontal;
    m_frameVertical = vertical;
    m_drawFrameCenter = drawCenter;
    Vector2 minimum(m_minimumSize);
    Real32 width = m_frameHorizontal.x + m_frameHorizontal.y + m_frameHorizontal.z;
    if (minimum.x < width)
        minimum.x = width;
    Real32 height = m_frameVertical.x + m_frameVertical.y + m_frameVertical.z;
    if (minimum.y < height)
        minimum.y = height;
    SetMinimumSize(minimum);
    if (m_frameBuffer) {
        UpdateFrame(true, true, false);
        if (m_drawAsFrame)
            Redraw();
    }
}

// Mac 0x155830, Win 0x50e96f
void RenderControl::SetDrawAsFrame(bool frame)
{
    if (m_drawAsFrame == frame)
        return;
    m_drawAsFrame = frame;
    if (frame) {
        UpdateFrame(false, false, false);
        m_frameBufferInUse = m_frameBuffer;
    } else {
        m_frameBufferInUse = 0;
    }
    Redraw();
}

// Mac 0x155930, Win 0x50efde
void RenderControl::SetTileImage(bool tile)
{
    m_tileImage = tile;
    Redraw();
}

// Mac 0x154c70, Win 0x50e9ac
void RenderControl::UpdateFrame(bool texCoords, bool positions, bool colors)
{
    if (!m_frameBuffer) {
        VertexBuffer::CreateArgs args;
        args.vertexCount = 0x10;
        args.indexCount = 0x36;
        args.frameCount = 0;
        args.dynamic = true;
        args.hasNormals = false;
        args.colorSetCount = 1;
        args.texCoordDimensions.PushBack(2);
        Object* object = m_manager->GetObjectFactory()->CreateObject(typeid(VertexBuffer), args);
        m_frameBuffer = object ? dynamic_cast<VertexBuffer*>(object) : 0;
        if (!m_frameBuffer)
            return;
        m_frameBuffer->SetIndices(kFrameIndices, 0, 0x36, 0);
        m_frameBuffer->SetPrimitiveType(kPrimitiveTriangles);
        texCoords = positions = colors = true;
    }
    if (texCoords && m_image) {
        Vector2 size = m_imageRect.GetSize();
        Vector2 imageSize = m_image->GetImageSize();
        Real32 left = m_imageRect.m_min.x;
        Real32 top = m_imageRect.m_min.y;
        Real32 right = (Real32)((EvalReal)size.x + left);
        Real32 bottom = (Real32)((EvalReal)top + size.y);
        Real32 u[4] = {
            left / imageSize.x,
            (Real32)(((EvalReal)m_frameHorizontal.x + left) / imageSize.x),
            (Real32)(((EvalReal)right - m_frameHorizontal.z) / imageSize.x),
            right / imageSize.x
        };
        Real32 v[4] = {
            top / imageSize.y,
            (Real32)(((EvalReal)m_frameVertical.x + top) / imageSize.y),
            (Real32)(((EvalReal)bottom - m_frameVertical.z) / imageSize.y),
            bottom / imageSize.y
        };
        Real32 coords[32];
        for (UInt32 row = 0; row < 4; ++row) {
            for (UInt32 column = 0; column < 4; ++column) {
                coords[(row * 4 + column) * 2] = u[column];
                coords[(row * 4 + column) * 2 + 1] = v[row];
            }
        }
        m_frameBuffer->SetTexCoords(coords, 0);
    }
    if (positions) {
        Vector2 size(m_size);
        Real32 x[4] = { 0.0f, m_frameHorizontal.x, size.x - m_frameHorizontal.z, size.x };
        Real32 y[4] = { 0.0f, m_frameVertical.x, size.y - m_frameVertical.z, size.y };
        Vector3 points[16];
        for (UInt32 row = 0; row < 4; ++row)
            for (UInt32 column = 0; column < 4; ++column)
                points[row * 4 + column] = Vector3(x[column], y[row], 0.0f);
        m_frameBuffer->SetPositions(points, 0, 0x10, 0);
    }
    if (colors) {
        Color color = GetDrawColor();
        Color data[16];
        for (UInt32 i = 0; i < 16; ++i)
            data[i] = color;
        m_frameBuffer->SetColors(data, 0, 0x10, 0);
    }
    m_frameBuffer->SetIndexCount(m_drawFrameCenter ? 0x36 : 0x30);
}

// Mac 0x155900, Win 0x50efc6
void RenderControl::OnColorChanged()
{
    if (m_frameBuffer)
        UpdateFrame(false, false, true);
}

// Mac 0x155970, Win 0x50eff0
void RenderControl::GetTextLayout(RefPointer<TextLayoutRender>& layout, bool create)
{
    if (!m_textLayout && create)
        CreateTextLayout();
    layout = m_textLayout;
}

// Mac 0x1559e0, Win 0x50f32e
void RenderControl::CreateTextLayout()
{
    if (m_textLayout)
        return;
    TextLayout::CreateArgs args;
    args.text = String();
    Object* object = m_manager->GetObjectFactory()->CreateObject(typeid(TextLayoutRender), args);
    if (!object)
        return;
    TextLayoutRender* layout = dynamic_cast<TextLayoutRender*>(object);
    if (!layout)
        return;
    RefPointer<TextLayoutRender> keep(layout);
    SetTextLayout(layout);
}

// Mac 0x155b30, Win 0x50f017
bool RenderControl::SetTextLayout(TextLayoutRender* layout)
{
    if (m_textLayout.Get() == layout)
        return true;
    if (!layout) {
        m_textLayout = 0;
        TextLayoutObserver::Detach();
    } else {
        TextLayoutObserver::Attach(layout);
        m_textLayout = layout;
        if (m_inputFlags & kInputWrapText)
            m_textLayout->SetMaxWidth((UInt32)Truncate(m_size.x));
    }
    Redraw();
    return true;
}

// Mac 0x155eb0, Win 0x50f0ad
void RenderControl::SetTextOffset(Real32 x, Real32 y)
{
    if (m_textOffset.x == x && m_textOffset.y == y)
        return;
    m_textOffset.x = x;
    m_textOffset.y = y;
    Redraw();
}

// Mac 0x155f20, Win 0x50f0ef
void RenderControl::SetHorizontalTextOffset(Real32 x)
{
    if (m_textOffset.x == x)
        return;
    m_textOffset.x = x;
    Redraw();
}

// Mac 0x155f70, Win 0x50f112
void RenderControl::SetVerticalTextOffset(Real32 y)
{
    if (m_textOffset.y == y)
        return;
    m_textOffset.y = y;
    Redraw();
}

// Mac 0x155fc0, Win 0x50f135
bool RenderControl::SetText(const String& text)
{
    CreateTextLayout();
    return m_textLayout ? m_textLayout->SetText(text) : false;
}

// Mac 0x156000 (Win inlined)
void RenderControl::GetFont(RefPointer<Font>& font) const
{
    if (!m_textLayout)
        font = 0;
    else
        m_textLayout->GetFont(font);
}

// Mac 0x156050, Win 0x50f152
bool RenderControl::SetFont(Font* font)
{
    CreateTextLayout();
    return m_textLayout ? m_textLayout->SetFont(font) : false;
}

// Mac 0x156090, Win 0x50f16f
bool RenderControl::SetTextAlignment(UInt32 horizontal, UInt32 vertical)
{
    CreateTextLayout();
    return m_textLayout ? m_textLayout->SetAlignment(horizontal, vertical) : false;
}

// Mac 0x1560d0, Win 0x50f18c
bool RenderControl::SetHorizontalAlignment(UInt32 alignment)
{
    CreateTextLayout();
    return m_textLayout ? m_textLayout->SetHorizontalAlignment(alignment) : false;
}

// Mac 0x156110, Win 0x50f1aa
bool RenderControl::SetVerticalAlignment(UInt32 alignment)
{
    CreateTextLayout();
    return m_textLayout ? m_textLayout->SetVerticalAlignment(alignment) : false;
}

// Mac 0x156150, Win 0x50f1c8
bool RenderControl::ResizeHeightToFitText(UInt32 width)
{
    Vector2 size;
    if (m_textLayout) {
        m_textLayout->SetMaxWidth(width);
        size = m_textLayout->GetSize();
        // NOTE: original quirk — with a plainly drawn image the size is limited to the image
        // rect (min, not max).
        if (m_image && !m_frameBufferInUse && !m_tileImage) {
            if (m_imageRect.GetAbsoluteWidth() < size.x)
                size.x = m_imageRect.GetAbsoluteWidth();
            if (m_imageRect.GetAbsoluteHeight() < size.y)
                size.y = m_imageRect.GetAbsoluteHeight();
        }
    }
    return SetSize(size);
}

// Mac 0x156230, Win 0x50f287
void RenderControl::ResizeToFitText()
{
    Vector2 size;
    if (m_textLayout) {
        m_textLayout->SetMaxWidth((UInt32)Truncate(m_size.x));
        size = m_textLayout->GetSize();
        size.x = m_size.x;
        if (m_image && !m_frameBufferInUse && !m_tileImage) {
            if (m_imageRect.GetAbsoluteHeight() < size.y)
                size.y = m_imageRect.GetAbsoluteHeight();
        }
    }
    SetSize(size);
}

// Mac 0x156300, Win 0x50f3ee
void RenderControl::OnRectangleChanged(const Vector2& oldPosition, const Vector2& oldSize)
{
    if (m_textLayout && (m_inputFlags & kInputWrapText))
        m_textLayout->SetMaxWidth((UInt32)Truncate(m_size.x));
    if (m_frameBuffer)
        UpdateFrame(false, true, false);
    BufferedAbleRenderControl::OnRectangleChanged(oldPosition, oldSize);
}

// Mac 0x156360, Win 0x50f441
void RenderControl::Draw(ControlDrawArgs& args)
{
    args.Setup(false);
    Color color = GetDrawColor();
#if defined(_WIN32)
    if ((double)color.a > 0.0001) {          // Win: double constant 0xa2d830
#else
    if (color.a > 0.0001f) {
#endif
        if (!m_image) {
            if (!m_transparent)
                args.DrawRect(Rect(m_size), color, color.a != 1.0f);
        } else {
            args.m_render->SetShader(m_shader.Get());
            bool blend = true;
            if (color.a == 1.0f)
                blend = m_image->GetFormat() != 0;
            if (m_frameBufferInUse) {
                args.m_render->SetVertexBuffer(m_frameBufferInUse.Get());
                args.m_render->SetTexture(m_image.Get(), 0);
                args.m_render->EnableBlend(blend);
                args.m_render->DrawVertexBuffer();
            } else if (!m_tileImage) {
                Rect textureRect(m_imageRect);
                textureRect.Divide(m_image->GetImageSize());
                args.DrawTexture(Rect(m_size), m_image.Get(), textureRect, color, blend);
            } else {
                Vector2 repeat(m_size);
                repeat.x = repeat.x / m_image->GetImageSize().x;
                repeat.y = repeat.y / m_image->GetImageSize().y;
                args.DrawTexture(Rect(m_size), m_image.Get(), Rect(repeat), color, blend);
            }
            args.m_render->SetShader(0);
        }
        if (m_textLayout) {
            Vector2 position(m_textOffset);
            m_textLayout->AlignInArea(m_size, position);
            Rect rect(position, m_textLayout->GetSize());
            rect &= args.GetLocalClip();
            if (rect.IsValid()) {
                if (Texture* texture = m_textLayout->GetTexture()) {
                    RefPointer<Font> font;
                    m_textLayout->GetFont(font);
                    // NOTE: a layout without a font crashes both originals here.
                    const Color& fontColor = font->GetColor();
                    color.r *= fontColor.r;
                    color.g *= fontColor.g;
                    color.b *= fontColor.b;
                    color.a *= fontColor.a;
                    Rect target(position, m_textLayout->GetSize());
                    args.DrawTexture(target, texture, m_textLayout->GetTexCoords(), color, true);
                }
            }
        }
    }
    m_status.m_invalid = 0;
}

// ===========================================================================================
// RenderControlManager
// ===========================================================================================

// Layer 0 of a RenderControlManager: a buffered root whose "buffer" is the screen (clear to
// opaque black, scale 1). RTTI RenderControlManager::RenderRoot (si : Control); Mac vtable
// 0x70e67c (BeginDraw 0x153770), dtors 0x1570f0/0x157160; Win ctor 0x50cf9a (0x178 bytes), dtor
// 0x50d00e (sdd 0x50d084), BeginDraw 0x50de01.
class RenderControlManager::RenderRoot : public Control {
public:
    RenderRoot(ControlManager* manager, UInt32 layer) : Control(manager, layer) {}
    virtual ~RenderRoot() {}

    // Mac 0x153770, Win 0x50de01
    virtual void BeginDraw(ControlDrawArgs& args)
    {
        args.BeginTarget(0, Color(0.0f, 0.0f, 0.0f, 1.0f), Vector2(1.0f, 1.0f));
    }
};

#if !defined(_WIN32)
// Mac only: the roots of layers 1 and 2 draw the resize grip of a resizable window (three
// diagonal lines in the bottom-right corner). RTTI si : Control; vtable 0x767c04 (Draw
// 0x1573e0), dtors 0x1572f0/0x157360. (name from RTTI)
class MacWindowResizeRootControl : public Control {
public:
    MacWindowResizeRootControl(ControlManager* manager, UInt32 layer) : Control(manager, layer) {}
    virtual ~MacWindowResizeRootControl() {}

    // Mac 0x1573e0
    virtual void Draw(ControlDrawArgs& args)
    {
        if (m_manager->GetWindow()->GetStyle().flag2a) {
            args.Setup(false);
            static const Real32 kOffsets[3] = { 3.0f, 7.0f, 11.0f };
            for (UInt32 i = 0; i < 3; ++i) {
                Vector2 a(m_size.x - kOffsets[i], m_size.y + -1.0f);
                Vector2 b(m_size.x + -1.0f, m_size.y - kOffsets[i]);
                args.DrawLine(a, b, Color::DarkGray(), false);
            }
        }
        m_status.m_invalid = 0;
    }
};
#endif

// Mac 0x152ca0 (C2 with VTT), Win 0x50d798
RenderControlManager::RenderControlManager(Window* window, Render* render)
    : ControlManager(window), m_render(render)
{
    window->SetModalUpdateReciever(this);
}

// Mac 0x156a50/0x156ab0 (body 0x1579f0), Win 0x50cec4 (sdd 0x50d05e)
RenderControlManager::~RenderControlManager()
{
}

// Mac 0x156b40, Win 0x51cc2a (folded with OpenGLWindow::GetLevel)
Render* RenderControlManager::GetRender() const
{
    return m_render.Get();
}

Control* RenderControlManager::GetRootControl() const
{
    return GetLayer(0);
}

bool RenderControlManager::IsUpdateLocked() const
{
    return ControlManager::IsUpdateLocked();
}

// Mac 0x152db0, Win 0x50d895
bool RenderControlManager::Initialize()
{
    if (!ControlManager::Initialize())
        return false;
    ObjectFactory* factory = m_render->GetObjectFactory();
    m_objectFactory->RegisterFactory(typeid(Texture), RefPointer<SpecificObjectFactory>(new ProxyFactory<Texture>(factory)).Get());
    m_objectFactory->RegisterFactory(typeid(VertexBuffer), RefPointer<SpecificObjectFactory>(new ProxyFactory<VertexBuffer>(factory)).Get());
    m_objectFactory->RegisterFactory(typeid(Font), RefPointer<SpecificObjectFactory>(new ProxyFactory<Font>(factory)).Get());
    m_objectFactory->RegisterFactory(typeid(TextLayoutRender), RefPointer<SpecificObjectFactory>(new ProxyFactory<TextLayoutRender>(factory)).Get());

    VertexBuffer::CreateArgs quadArgs;
    quadArgs.vertexCount = 4;
    quadArgs.indexCount = 4;
    quadArgs.frameCount = 0;
    quadArgs.dynamic = true;
    quadArgs.hasNormals = false;
    quadArgs.colorSetCount = 1;
    quadArgs.texCoordDimensions.PushBack(2);
    Object* object = m_render->GetObjectFactory()->CreateObject(typeid(VertexBuffer), quadArgs);
    m_quad = object ? dynamic_cast<VertexBuffer*>(object) : 0;
    if (!m_quad)
        return false;
    static const UInt32 kQuadIndices[4] = { 0, 1, 2, 3 };
    m_quad->SetIndices(kQuadIndices, 0, 4, 0);
    m_quad->SetPrimitiveType(kPrimitiveTriangleStrip);

    VertexBuffer::CreateArgs lineArgs;
    lineArgs.vertexCount = 2;
    lineArgs.indexCount = 2;
    lineArgs.frameCount = 0;
    lineArgs.dynamic = true;
    lineArgs.hasNormals = false;
    lineArgs.colorSetCount = 1;
    object = m_render->GetObjectFactory()->CreateObject(typeid(VertexBuffer), lineArgs);
    m_lines = object ? dynamic_cast<VertexBuffer*>(object) : 0;
    if (!m_lines)
        return false;
    static const UInt32 kLineIndices[2] = { 0, 1 };
    m_lines->SetIndices(kLineIndices, 0, 2, 0);
    m_lines->SetPrimitiveType(kPrimitiveLines);
    return true;
}

// Win 0x50cf68 (EAX = this; inlined on the Mac)
bool RenderControlManager::NeedsPaint() const
{
    Control* root = GetLayer(0);
    return root->GetStatus().m_invalid && Rect(root->GetSize()).IsValid();
}

// Mac 0x153480, Win 0x50dcb5
void RenderControlManager::Update()
{
    ControlManager::Update();
    if (NeedsPaint())
        m_window->Invalidate(0);
}

// Mac 0x1535a0 (thunk 0x1535f0), Win 0x50dcde
void RenderControlManager::ModalUpdate()
{
    if (NeedsPaint())
        m_window->Invalidate(0);
}

// Mac 0x153640, Win 0x50dd05
void RenderControlManager::Paint()
{
    if (!NeedsPaint())
        return;
    ControlDrawArgs args;
    args.m_offset = Vector2(0.0f, 0.0f);
    args.m_scale = Vector2(1.0f, 1.0f);
    args.m_clip = Rect(0.0f, 0.0f, 0.0f, 0.0f);
    args.m_size = Vector2(0.0f, 0.0f);
    args.m_render = m_render.Get();
    args.m_quad = m_quad.Get();
    args.m_lines = m_lines.Get();
    args.m_targetScale = Vector2(1.0f, 1.0f);
    args.m_state = ControlDrawArgs::kStateNone;
    m_render->BeginFrame();
    Draw(args);
    m_render->EndFrame();
    m_render->Present();
}

// Mac 0x156b20, Win 0x50ceae
void RenderControlManager::OnPaint(UInt32 /*arg*/)
{
    GetLayer(0)->Invalidate(false);
    Paint();
}

// Mac 0x153810, Win 0x50de35
void RenderControlManager::CreateLayer(UInt32 layer)
{
    if (layer == 0) {
        new RenderRoot(this, 0);
        return;
    }
#if defined(_WIN32)
    ControlManager::CreateLayer(layer);
#else
    new MacWindowResizeRootControl(this, layer);
#endif
}

} // namespace LEGO
