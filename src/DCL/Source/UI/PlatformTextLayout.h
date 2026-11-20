// LEGO Digital Designer — restored source
// LEGO::PlatformTextLayout — the platform TextLayoutRender: renders the laid-out text into a
// CPU-writable texture (MutableCreateArgs2D, RGBA32, 1-pixel transparent border): font colour,
// alpha = text coverage. Created by PlatformTextLayoutFactory (OpenGLRender.cpp).
// TU 040 "PlatformTextLayout.cpp" (Mac 0x100fc0-0x1025af; Win 0x500500-0x5013b0).
// Vtable: Mac 0x7097ec (23 entries: TextLayout's with NotifyChanged 0x102360 and the three
//   TextLayoutRender slots 0x1010a0 / 0x101170 / 0x1013b0) + Object-in 0x709860;
//   Win 0xa27c04 (22 entries: GetMeasureDC 0x500aa0, NotifyChanged 0x500850, 0x500970,
//   0x500a40, 0x500a90) + Object-in 0xa27c60 {sdd 0x5013b0 / 0x500940, …}.
// Layout: Mac TextLayout part +0x00..+0x54, texture dirty +0x54, Texture* +0x58, Render* +0x5c,
//   Object +0x60 (sizeof 0x68); Win TextLayout part to +0x50, dirty +0x50, Texture* +0x54,
//   Render* +0x58, vtordisp +0x5c, Object +0x60. Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "TextLayout.h"
#include "Texture.h"
#include "Render.h"

namespace LEGO {

class Bitmap;
class FontPlatform;

class PlatformTextLayout : public TextLayoutRender {
public:
    // Mac 0x100fc0 (C1), Win 0x500860. The render is referenced (Mac: unconditionally).
    PlatformTextLayout(Render* render, const String& text, FontPlatform* font,
                       UInt32 horizontalAlignment, UInt32 verticalAlignment, UInt32 maxWidth);
    virtual ~PlatformTextLayout();                                         // Mac 0x101890/0x101950 (body 0x1017e0), Win 0x500ef0/0x500940

#if defined(_WIN32)
    // The screen DC in MM_TEXT mode.
    virtual HDC GetMeasureDC();                                            // Win 0x500aa0
#endif
    // Also marks the texture for re-rendering.
    virtual void NotifyChanged();                                          // Mac 0x102360, Win 0x500850

    virtual Rect GetTexCoords();                                           // Mac 0x1010a0, Win 0x500970
    virtual Vector2 GetTextureSize();                                      // Mac 0x101170, Win 0x500a40
    virtual Texture* GetTexture();                                         // Mac 0x1013b0, Win 0x500a90

protected:
    // Lays out (if needed) and re-creates + renders the texture when it is dirty. Texture of
    // ceil(size) + 2 pixels; none for a size below one pixel.
    void UpdateTexture();                                                  // Mac 0x1011c0, Win 0x500d20
    // Draws the text into the texture's bitmap (Mac CGBitmapContext + CTFrameDraw, Win GDI
    // DIB section + DrawText) and fills in colour + coverage alpha.
    void RenderText(Bitmap& bitmap);                                       // Mac 0x1013d0, Win 0x500ac0

    bool                m_textureDirty;   // Mac +0x54, Win +0x50 (true initially)
    RefPointer<Texture> m_texture;        // Mac +0x58, Win +0x54
    RefPointer<Render>  m_render;         // Mac +0x5c, Win +0x58
};

} // namespace LEGO
