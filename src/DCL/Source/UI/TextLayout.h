// LEGO Digital Designer — restored source
// LEGO::TextLayout — a laid-out text (string, font, alignment, maximum width, truncation); the
// size is computed lazily by the platform engine (Mac CoreText CTFramesetter, Win GDI
// DrawText(DT_CALCRECT)); observers hear every change. TextLayoutRender adds texture + texture
// coordinates; TextLayout::CreateArgs is the block for CreateObject(typeid(TextLayoutRender), …)
// (PlatformTextLayoutFactory, OpenGLRender.cpp). LEGO::TextLayoutObserver: one handler.
// TU 121 "TextLayout.cpp" (Mac 0x1d80c0-0x1d8fff, Win 0x5386c0-0x538d5f, 0x500610-0x50064f); the
// inline virtuals and dtors are emitted in TU 040 (PlatformTextLayout.cpp, Mac 0x101b10-0x101ec0,
// Win 0x500570-0x501390).
//
// Vtables (Mac primary 0x70995c, 20 entries; Win 0xa27af4, 19 entries): Mac 0/1 D1/D0,
//   2 NotifierT::OnFinalRelease (Win: in the Object-in vtable 0xa27b44), then the new virtuals
//   below in declaration order (Mac slot = Win slot + 3 up to SetTruncate). Win has two more
//   virtuals, the measuring DC (Win slots 16/17), before NotifyChanged (Mac 19, Win 18).
//   TextLayoutRender (Mac 0x7098bc, Win 0xa27b94) adds three pure virtuals (Mac 20-22, Win 19-21).
//
// Layout (Mac; ctor 0x1d80c0 with VTT): Notifier part +0x00..+0x24 (vptr, observers +0x04,
//   iterators +0x14), String +0x24, Font* +0x34, alignment +0x38/+0x3c, dirty +0x40,
//   truncate +0x41, max width +0x44, size +0x48, CTFrameRef +0x50, Object +0x54 (sizeof 0x5c).
// Layout (Win; ctor 0x538590): vfptr, vbptr +4, observers +0x08, iterators +0x18,
//   String +0x28, Font* +0x2c, alignment +0x30/+0x34, dirty +0x38, truncate +0x39, max width
//   +0x3c, size +0x40, text colour +0x48, DrawText alignment flags +0x4c, vtordisp +0x50,
//   Object +0x54.
// Method and member names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Notifier.h"
#include "ObjectFactory.h"
#include "Geometry.h"
#include "LEGO/String.h"
#include "Font.h"

#if defined(_WIN32)
#include <windows.h>
#endif

namespace LEGO {

class TextLayout;
class Texture;

// Mac vtable 0x707b4c {D1 0xec880, D0 0xec8b0, OnFinalRelease 0xec900, OnAttached 0xec5a0,
// OnDetached 0xec5b0, OnTextLayoutChanged = pure}; Win 0x9cee3c (Object-in) + {OnAttached,
// OnDetached, handler}.
class TextLayoutObserver : public Observer<TextLayout, TextLayoutObserver> {
public:
    virtual ~TextLayoutObserver() {}                                        // Mac 0xec880/0xec8b0
    // The layout's text / font / alignment / size changed (TextLayout::NotifyChanged).
    // NOTE: both originals call the handler without an argument (Mac 0x1d8c37 pushes only the
    // observer, Win 0x5389d0 nothing); the parameter is a restoration addition and
    // receives 0. (name inferred)
    virtual void OnTextLayoutChanged(UInt32 arg) = 0;                       // Mac slot 5, Win slot 2
};

class TextLayout : public Notifier<TextLayoutObserver> {
public:
    // Horizontal / vertical alignment values (strings "Left"/"Right"/"Center",
    // "Top"/"Bottom"/"Center" in the Lua/XML layouts, ParseHorizontalAlignment below).
    enum HorizontalAlignment { kAlignLeft = 0, kAlignRight = 1, kAlignCenterHorizontally = 2 };
    enum VerticalAlignment { kAlignTop = 0, kAlignBottom = 1, kAlignCenterVertically = 2 };
    enum { kAlignCenter = 2 };

    // Mac vtable 0x7630fc {D1 0xefaa0, D0 0xf0e40}, Win 0xa28194 (sdd 0x50ccec). Mac sizeof
    // 0x24. Read by PlatformTextLayoutFactory::CreateObject (Mac 0xf7f50, Win 0x4ec310).
    struct CreateArgs : public ObjectFactoryArgs {
        CreateArgs() : font(0), horizontalAlignment(0), verticalAlignment(0), maxWidth(0) {}
        virtual ~CreateArgs() {}                                    // Mac 0xefaa0/0xf0e40
        String text;                  // +0x04
        Font*  font;                  // Mac +0x14, Win +0x08 (not referenced by the args)
        UInt32 horizontalAlignment;   // Mac +0x18
        UInt32 verticalAlignment;     // Mac +0x1c
        UInt32 maxWidth;              // Mac +0x20 (0: no limit, no word wrapping)
    };

    // Constructor of the base subobject (Mac 0x1d80c0, Win 0x538590). The font is referenced.
    TextLayout(const String& text, Font* font, UInt32 horizontalAlignment,
               UInt32 verticalAlignment, UInt32 maxWidth);
    // Mac 0x101d90/0x101df0 (body 0x102370: CFRelease of the frame, font release, ~String),
    // Win 0x500650 / sdd 0x501390.
    virtual ~TextLayout();

    // ---- Mac slots 3.. / Win slots 0.. ----
    // True without a font or without text.
    virtual bool IsEmpty() const;                                           //  3/0  Mac 0x101b60, Win 0x500570
    virtual const String& GetText() const;                                  //  4/1  Mac 0x101b90, Win 0x5b7060
    // Changes the text; observers are told only when there is a font. Always true.
    virtual bool SetText(const String& text);                               //  5/2  Mac 0x1d8230, Win 0x538680
    virtual void GetFont(RefPointer<Font>& font) const;                     //  6/3  Mac 0x101ba0, Win 0x500590
    // Changes the font; observers are told only when there is text. Always true.
    virtual bool SetFont(Font* font);                                       //  7/4  Mac 0x1d8280, Win 0x5386c0
    virtual void GetAlignment(UInt32& horizontal, UInt32& vertical) const;  //  8/5  Mac 0x101bf0, Win 0x5005d0
    virtual bool SetAlignment(UInt32 horizontal, UInt32 vertical);          //  9/6  Mac 0x1d82f0, Win 0x538700
    virtual UInt32 GetHorizontalAlignment() const;                          // 10/7  Mac 0x101c10, Win 0x5005f0
    virtual bool SetHorizontalAlignment(UInt32 alignment);                  // 11/8  Mac 0x1d8320, Win 0x538730
    virtual UInt32 GetVerticalAlignment() const;                            // 12/9  Mac 0x101c20, Win 0x500600
    virtual bool SetVerticalAlignment(UInt32 alignment);                    // 13/10 Mac 0x1d8350, Win 0x538750
    // The size of the laid-out text in pixels (laid out on demand).
    virtual Vector2 GetSize();                                              // 14/11 Mac 0x1d8380, Win 0x538770
    virtual UInt32 GetMaxWidth() const;                                     // 15/12 Mac 0x101c30, Win 0x500610
    virtual bool SetMaxWidth(UInt32 maxWidth);                              // 16/13 Mac 0x1d8450, Win 0x538790
    // With a maximum width: truncate with an ellipsis instead of wrapping words.
    virtual bool GetTruncate() const;                                       // 17/14 Mac 0x101c40, Win 0x500620
    virtual bool SetTruncate(bool truncate);                                // 18/15 Mac 0x1d8480, Win 0x5387b0
#if defined(_WIN32)
    // The DC used to measure (and, in PlatformTextLayout, to select the font into). Win only.
    virtual HDC GetMeasureDC();                                             // –/16  Win 0x500630
    virtual void ReleaseMeasureDC(HDC dc);                                  // –/17  Win 0x500640
#endif
    // Marks the layout dirty and calls every observer's OnTextLayoutChanged.
    virtual void NotifyChanged();                                           // 19/18 Mac 0x1d8ab0, Win 0x538950

    // Adds the alignment offset of the text inside `area` to `position`: (area - size) for Right /
    // Bottom, half of it for Center. Callers: RenderControl::Draw, WindowControl,
    // StretchableButtonControl.
    void AlignInArea(const Vector2& area, Vector2& position);               // Mac 0x1d84b0, Win 0x5387d0

    // "Left"/"Right"/"Center" (case-insensitive keyword at `position`, followed by ',',
    // whitespace or the end) → 0/1/2. Callers: LuaControl (TU 315).
    static bool ParseHorizontalAlignment(UInt32& alignment, const String& text, UInt32& position); // Mac 0x1d8d40, Win 0x538ac0
    // "Top"/"Bottom"/"Center" → 0/1/2.
    static bool ParseVerticalAlignment(UInt32& alignment, const String& text, UInt32& position);   // Mac 0x1d8e80, Win 0x538c10

protected:
    // Lays the text out again when it is dirty (the body of GetSize, inlined there on Mac).
    void UpdateLayout();                                                    // Mac 0x1d83f0, Win 0x538850
    // Platform text engine: computes `size` (and keeps the Mac CTFrame for drawing). Not
    // called for an empty layout.
    void Layout(Vector2& size);                                             // Mac 0x1d8570 (Win: in 0x538850)
#if defined(_WIN32)
    // DrawText with DT_CALCRECT (measure) / without (draw) and the layout's flags.
    int CalcText(HDC dc, RECT* rect);                                       // Win 0x538a00
    int DrawTextInto(HDC dc, RECT* rect);                                   // Win 0x538a60
#else
    // Draws the laid-out frame into `context` (CTFrameDraw; HOST-ONLY: the host renderer).
    void DrawFrame(void* context);                                          // Mac 0x1d8d20
#endif

    String  m_text;                  // Mac +0x24, Win +0x28
    RefPointer<Font> m_font;         // Mac +0x34, Win +0x2c
    UInt32  m_horizontalAlignment;   // Mac +0x38, Win +0x30
    UInt32  m_verticalAlignment;     // Mac +0x3c, Win +0x34
    bool    m_dirty;                 // Mac +0x40, Win +0x38 (true initially)
    bool    m_truncate;              // Mac +0x41, Win +0x39
    UInt32  m_maxWidth;              // Mac +0x44, Win +0x3c
    Vector2 m_size;                  // Mac +0x48, Win +0x40
#if defined(_WIN32)
    COLORREF m_textColor;            // Win +0x48 0xffffff (GDI draws white, PlatformTextLayout
                                     // takes the coverage as alpha)
    UINT     m_alignmentFormat;      // Win +0x4c DT_LEFT / DT_RIGHT / DT_CENTER (set by layout)
#else
    void*    m_frame;                // Mac +0x50 CTFrameRef (HOST-ONLY: HostTextFrame*)
#endif

private:
    TextLayout(const TextLayout&);
    TextLayout& operator=(const TextLayout&);
};

// Mac vtable 0x7098bc (+ 0x763e58 copy), Win 0xa27b94. No members; the constructor is inline
// (Win 0x500720), the destructors are Mac 0x101a40/0x101aa0, Win 0x5007b0/0x501380.
class TextLayoutRender : public TextLayout {
public:
    TextLayoutRender(const String& text, Font* font, UInt32 horizontalAlignment,
                     UInt32 verticalAlignment, UInt32 maxWidth)
        : TextLayout(text, font, horizontalAlignment, verticalAlignment, maxWidth) {}
    virtual ~TextLayoutRender() {}

    // Texture coordinates of the text in GetTexture(): (u0, v0, u1, v1) as a Rect.
    virtual Rect GetTexCoords() = 0;                                        // 20/19
    // The size of the texture (the GL image size).
    virtual Vector2 GetTextureSize() = 0;                                   // 21/20
    // The texture holding the rendered text (null for an empty layout).
    virtual Texture* GetTexture() = 0;                                      // 22/21
};

} // namespace LEGO
