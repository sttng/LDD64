// LEGO Digital Designer — restored source
// LEGO::RenderControl — the drawing control: a background (m_color, or an image / sub-rect,
// optionally a 9-slice frame) and an optional text (TextLayoutRender); redraws when its text
// layout or texture changes. Base of WindowControl, ButtonControl, Mode, RenderCursor, the LDD
// controls …
// Home TU 065 "RenderControl.cpp" (Mac 0x1523d0-0x15811b with RenderControlManager and
// MacWindowResizeRootControl; Win 0x50ceae-0x50fa32), bodies there; the dtor / OnFinalRelease /
// the two observer overriders are emitted weak in TU 028 on Mac (0xec2b0-0xec730).
//
// Mac layout (ctor 0x154530): BufferedAbleRenderControl +0x000..+0x194,
//   TextLayoutObserver +0x194 (subject +0x198), TextureObserver +0x19c (subject +0x1a0),
//   members below, Object +0x1ec (Object-in vtable 0x70e05c). Win: every member below at
//   Mac + 0x1c (TextLayoutObserver +0x1a8, TextureObserver +0x1b4, m_transparent +0x1c0,
//   m_image +0x1c4, m_imageRect +0x1c8, m_frameBufferInUse +0x1d8, m_textLayout +0x1e0,
//   m_textOffset +0x1e4, …), Object +0x20c (0x9cee84).
// Virtual slots: Mac 0x70df48 (45 entries), Win 0x9ceebc (40 entries = Mac 3..42):
//   overrides OnFinalRelease (2), OnRectangleChanged (8/5), Draw (33/30), OnColorChanged
//   (37/34); new 38..42 (Win 35..39); Mac-only primary entries 43 / 44 = the overriders of
//   TextLayoutObserver::OnTextLayoutChanged and TextureObserver::OnTextureChanged (Itanium
//   gives overriders of non-primary-base virtuals their own primary slot, MSVC does not).
// Names: LuaControl's layout attributes (Mac 0x2c08e0: "transparent", "drawImageAsFrame",
// "text", "hAlignment", "vAlignment", "hAlignmentOffset", "vAlignmentOffset", "resizeToFitText",
// "resizeHeightToFitText") name the setters; the rest is inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "Geometry.h"
#include "Color.h"
#include "Notifier.h"
#include "Texture.h"
#include "BufferedAbleRenderControl.h"
#include "TextLayout.h"

namespace LEGO {

class TextLayout;
class TextLayoutRender;
class Font;
class Shader;
class VertexBuffer;

// TextLayoutObserver: TextLayout.h.

class RenderControl : public BufferedAbleRenderControl,
                      public TextLayoutObserver,
                      public TextureObserver {
public:
    // Input flag bits RenderControl adds to Control's (bit 0 mouse, 1 keyboard, 2 buffering).
    enum {
        kInputInitialized = 0x40000000,   // set by Initialize (name inferred)
        kInputWrapText    = 0x80000000    // the text layout's maximum width follows the control width
    };

    explicit RenderControl(ControlManager* manager);                       // Mac 0x154530 (C2 with VTT), Win 0x50e3bc
    virtual ~RenderControl();                                               // Mac 0xec2b0/0xec310

    // ---- Control / BufferedAbleRenderControl overrides (Mac slot / Win slot) ----
    // Text width follows the control (kInputWrapText); frame geometry re-laid out.
    virtual void OnRectangleChanged(const Vector2& oldPosition, const Vector2& oldSize); // 8/5  Mac 0x156300, Win 0x50f3ee
    // Background (colour, image or frame) and text. Mac 0x156360, Win 0x50f441
    virtual void Draw(ControlDrawArgs& args);                               // 33/30
    // Recolours the frame geometry. Mac 0x155900, Win 0x50efc6
    virtual void OnColorChanged();                                          // 37/34

    // ---- new virtuals (names inferred) ----
    // Second construction step (`new X(manager)`, Initialize, then Setup); sets kInputInitialized;
    // true. Derived controls override it and call it first.
    virtual bool Initialize();                                              // 38/35 Mac 0x154790, Win 0x50e5b5
    // Text / font of the text layout (created on demand, Mac 0x1559e0); false without one.
    virtual bool SetText(const String& text);                               // 39/36 Mac 0x155fc0, Win 0x50f135
    virtual bool SetFont(Font* font);                                       // 40/37 Mac 0x156050, Win 0x50f152
    // Fit the control to its text (Lua "resizeHeightToFitText" / "resizeToFitText"): the layout's
    // maximum width = `width` (resp. the control width), height (and width) = the text size, at
    // most the image rect's size when an image is drawn plainly. Returns SetSize's result
    // (LDDUtilities Mac 0x14b910 / Win 0x6375cf test it).
    virtual bool ResizeHeightToFitText(UInt32 width);                       // 41/38 Mac 0x156150, Win 0x50f1c8
    virtual void ResizeToFitText();                                         // 42/39 Mac 0x156230, Win 0x50f287

    // ---- secondary-base overriders: redraw unless the manager is locked (+0xd0) ----
    virtual void OnTextLayoutChanged(UInt32 arg);                           // 43/– Mac 0xec430 (thunk 0xec5c0), Win 0x50d0f9
    virtual void OnTextureChanged(UInt32 face);                             // 44/– Mac 0xec460 (thunk 0xec730), Win 0x50d0ee

    // ---- non-virtual (names inferred) ----
    // Lua "transparent": nothing is drawn behind the text when there is no image.
    bool IsTransparent() const { return m_transparent; }
    void SetTransparent(bool transparent);                                  // Win 0x44b660 (inlined on Mac)
    // The image (observed) and the part of it to draw (null: the whole texture).
    // Mac 0x154830, Win 0x50e5cb
    void SetImage(Texture* image, const Rect* imageRect);
    // The image loaded through the ImageLoader (Mac 0x28fb80 / 0x290230). Mac 0x155520, Win 0x50e705
    void SetImage(const String& path);
    // The image and its rect. Mac 0x1547c0
    void GetImage(RefPointer<Texture>& image, Rect& imageRect) const;
    // A new image rect (null: the whole image); true. Mac 0x1555f0, Win 0x50e796
    bool SetImageRect(const Rect* imageRect);
    // Lua "drawImageAsFrame": 9-slice frame widths (left, centre, right / top, centre, bottom
    // in image pixels) and whether the centre is drawn; the minimum size grows to the frame.
    // Mac 0x155710, Win 0x50e875
    void SetFrame(const Vector3& horizontal, const Vector3& vertical, bool drawCenter);
    // Draw the image as that frame (true) or plainly. Mac 0x155830, Win 0x50e96f
    void SetDrawAsFrame(bool frame);
    // The whole image is tiled over the control (texture coordinates control size / image size,
    // wrap mode repeat) instead of its rect stretched over it (StartMode Mac 0x199170).
    // Mac 0x155930, Win 0x50efde (name inferred)
    void SetTileImage(bool tile);
    // The text layout; `create` makes one first. Mac 0x155970
    void GetTextLayout(RefPointer<TextLayoutRender>& layout, bool create);           // Win 0x50eff0
    // Replaces the text layout (observed; maximum width = control width with kInputWrapText).
    // Mac 0x155b30
    bool SetTextLayout(TextLayoutRender* layout);                                     // Win 0x50f017
    // Text alignment (TextLayout::HorizontalAlignment / VerticalAlignment) and offset.
    bool SetTextAlignment(UInt32 horizontal, UInt32 vertical);              // Mac 0x156090
    bool SetHorizontalAlignment(UInt32 alignment);                          // Mac 0x1560d0, Win 0x50f18c
    bool SetVerticalAlignment(UInt32 alignment);                            // Mac 0x156110, Win 0x50f1aa
    void SetTextOffset(Real32 x, Real32 y);                                 // Mac 0x155eb0, Win 0x50f0ad
    void SetHorizontalTextOffset(Real32 x);                                 // Mac 0x155f20, Win 0x50f0ef
    void SetVerticalTextOffset(Real32 y);                                   // Mac 0x155f70, Win 0x50f112
    // The layout's font (null without a layout). Mac 0x156000 (Win inlined)
    void GetFont(RefPointer<Font>& font) const;
    // Shader the image is drawn with (null: fixed function). (name inferred)
    void SetShader(Shader* shader);

protected:
    virtual void OnFinalRelease();                                          // 2/– Mac 0xec380

    // Creates the text layout when there is none (TextLayout::CreateArgs through the manager's
    // object factory). Mac 0x1559e0, Win 0x50f32e
    void CreateTextLayout();
    // (Re)builds the 16-vertex 9-slice geometry of the frame: texture coordinates, positions,
    // colours (all three when the buffer is created first). Mac 0x154c70, Win 0x50e9ac
    void UpdateFrame(bool texCoords, bool positions, bool colors);
    // White while the control draws into its own buffer (the buffer quad gets m_color), else
    // m_color. Win 0x50d0b4 (inlined on Mac)
    Color GetDrawColor() const;

    // ---- data (Mac offsets; constructor 0x154530) ----
    bool                         m_transparent;      // +0x1a4 false (Lua "transparent")
    bool                         m_drawAsFrame;      // +0x1a5 false
    bool                         m_drawFrameCenter;  // +0x1a6 true
    bool                         m_tileImage;        // +0x1a7 false
    RefPointer<Texture>          m_image;            // +0x1a8 (observed by the TextureObserver base)
    Rect                         m_imageRect;        // +0x1ac (0, 0, 0, 0) image pixels
    RefPointer<VertexBuffer>     m_frameBufferInUse; // +0x1bc = m_frameBuffer while drawn as a frame
    RefPointer<Shader>           m_shader;           // +0x1c0 0
    RefPointer<TextLayoutRender> m_textLayout;       // +0x1c4 (observed by the TextLayoutObserver base)
    Vector2                      m_textOffset;       // +0x1c8 (0, 0)
    Vector3                      m_frameHorizontal;  // +0x1d0 left, centre, right
    Vector3                      m_frameVertical;    // +0x1dc top, centre, bottom
    RefPointer<VertexBuffer>     m_frameBuffer;      // +0x1e8 9-slice geometry
                                                     // Object +0x1ec
};

} // namespace LEGO
