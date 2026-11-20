// LEGO Digital Designer — restored source
// LEGO::BufferedAbleRenderControl — a Control that can draw itself into an off-screen texture
// (input flag 4 "buffering": a power-of-two render target of the control size times the buffer
// scale; BeginDraw renders into it, EndDraw draws it with m_color). Base of RenderControl.
// Home TU 065 "RenderControl.cpp" (Mac 0x1523d0-0x15811b; Win 0x50ceae-0x50fa32), bodies there;
// some Mac inline bodies sit in TU 021 (dtor 0x50f30, OnBufferingDisabled 0x51020, slot 37
// 0x51060, ctor 0x2fa20).
//
// Mac layout (ctor 0x2fa20): Control +0x000..+0x158, members below, Object +0x194.
// Win (+0x14 = Control's Win growth): m_color +0x16c, m_clearColor +0x17c, m_buffer +0x18c,
// m_bufferSize +0x190, m_bufferScale +0x198, m_alphaMode +0x1a0, m_wrapMode +0x1a4, Object
// +0x1ac (Object-in vtable 0x9c97a8 {sdd 0x457820, OnFinalRelease 0x4577f0, …}).
// Virtual slots: Mac 0x70e8e0 (38 entries; Control's 36 + 36, 37), Win 0x9c97bc (35 entries).
// Overrides: OnRectangleChanged (8/5, re-allocates the buffer), OnBufferingEnabled (31/28),
// OnBufferingDisabled (32/29), EndDraw (34/31), BeginDraw (35/32). Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Geometry.h"
#include "Color.h"
#include "Control.h"

namespace LEGO {

class Texture;

class BufferedAbleRenderControl : public Control {
public:
    // Mac 0x2fa20 (C2 with VTT): colours (1,1,1,1) / (0,0,0,1), scale (1,1), wrap mode 1, then
    // SetHitTestable(true). (Win: inlined in RenderControl's constructor 0x50e3bc)
    explicit BufferedAbleRenderControl(ControlManager* manager);
    virtual ~BufferedAbleRenderControl();                                   // Mac 0x50f30/0x50fe0

    // ---- Control overrides (Mac slot / Win slot) ----
    virtual void OnRectangleChanged(const Vector2& oldPosition, const Vector2& oldSize); // 8/5   Mac 0x153c40, Win 0x50e0b5
    // Creates the render target (power-of-two size >= the control size). Mac 0x153dd0, Win 0x50e181
    virtual bool OnBufferingEnabled();                                      // 31/28
    // Releases the render target (+0x178). Mac 0x51020, Win 0x456070
    virtual void OnBufferingDisabled();                                     // 32/29
    // Draws the buffer texture (scaled by m_bufferScale) with m_color. Mac 0x153ed0, Win 0x50e20c
    virtual void EndDraw(ControlDrawArgs& args);                            // 34/31
    // Renders into the buffer: render target, clear with m_clearColor. Mac 0x154060, Win 0x50e2df
    virtual void BeginDraw(ControlDrawArgs& args);                          // 35/32

    // ---- new virtuals ----
    // A new buffer texture for `size` (pixels; times m_bufferScale, rounded, power of two;
    // Texture::RenderTargetCreateArgs2D); null when the factory refuses. (name inferred)
    virtual Texture* CreateBuffer(const IntVector2& size);                  // 36/33 Mac 0x1540e0, Win 0x50e301
    // m_color changed (SetColor). Empty by default; RenderControl recolours its frame geometry
    // (Mac 0x155900). (name inferred)
    virtual void OnColorChanged();                                          // 37/34 Mac 0x51060, Win 0x459790

    // ---- non-virtual (names inferred) ----
    const Color& GetColor() const { return m_color; }
    // Clamped into [0, 1]; hit-testable while alpha > 1/65536; OnColorChanged; redraw.
    // Mac 0x1539a0, Win 0x50df13
    void SetColor(const Color& color);
    // Alpha of m_color (clamped to [0, 1]); as SetColor. Mac 0x153900, Win 0x50de7e
    void SetOpacity(Real32 opacity);
    // Background of the buffer. Mac 0x153a60, Win 0x50df9f
    void SetClearColor(const Color& color);
    // Buffer texture parameters (RenderTargetCreateArgs2D alphaMode / wrapMode) and buffer pixels
    // per control unit; re-creates a live buffer. Mac 0x153af0, Win 0x50dfee
    void SetBufferOptions(UInt32 alphaMode, UInt32 wrapMode, const Vector2& scale);
    Texture* GetBuffer() const { return m_buffer.Get(); }

protected:
    // Invalidate(false) unless the manager is drawing (inlined everywhere; the original reads
    // ControlManager +0xd0 / Win +0xe8 directly). (name inferred)
    void Redraw();

    // ---- data (Mac offsets) ----
    Color                m_color;          // +0x158 (1, 1, 1, 1) colour of the control / buffer quad
    Color                m_clearColor;     // +0x168 (0, 0, 0, 1) Render::SetClearColor in BeginDraw
    RefPointer<Texture>  m_buffer;         // +0x178 render target
    IntVector2           m_bufferSize;     // +0x17c power-of-two size (control pixels)
    Vector2              m_bufferScale;    // +0x184 (1, 1) buffer pixels per control unit
    UInt32               m_alphaMode;      // +0x18c 0 (RenderTargetCreateArgs2D::alphaMode)
    UInt32               m_wrapMode;       // +0x190 1 (RenderTargetCreateArgs2D::wrapMode)
                                           // Object +0x194
};

} // namespace LEGO
