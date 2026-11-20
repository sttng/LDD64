// LEGO Digital Designer — restored source
// PlatformTextLayout — TU 040 "PlatformTextLayout.cpp" (Mac 0x100fc0-0x1025af), Win
// 0x500500-0x5013b0; plus the OpenGLRender factory hooks of the text classes
// (OpenGLFactories.h: CreatePlatformTextLayout = PlatformTextLayoutFactory::CreateObject Mac
// 0xf7f50 / Win 0x4ec310, TypeOfTextLayout*). Platform parts: Win GDI DIB section; Mac
// CoreGraphics (`__APPLE__`); otherwise Linux (Platform/Linux/LinuxText.h; the tests' HOST-ONLY builds: compat/host_app/HostText.h).
#include "PlatformTextLayout.h"
#include "FontPlatformFactory.h"
#include "OpenGLFactories.h"
#include "OpenGLRender.h"
#include "Bitmap.h"

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <ApplicationServices/ApplicationServices.h>
#include <math.h>
#else
#if defined(LDD_LINUX)
#include "LinuxText.h"
namespace LEGO { namespace PlatformText = LinuxText; }
#else
#include "host_app/HostText.h"   // HOST-ONLY (the Linux-hosted Mac branch of the tests)
namespace LEGO { namespace PlatformText = HostText; }
#endif
#include <math.h>
#include <string.h>
#endif

#include <typeinfo>

namespace LEGO {

// ---------------------------------------------------------------------------------------------
// OpenGLRender registration hooks (OpenGLFactories.h)
// ---------------------------------------------------------------------------------------------
const std::type_info& TypeOfTextLayout() { return typeid(TextLayout); }
const std::type_info& TypeOfPlatformTextLayout() { return typeid(PlatformTextLayout); }
const std::type_info& TypeOfTextLayoutRender() { return typeid(TextLayoutRender); }

// PlatformTextLayoutFactory::CreateObject, Mac 0xf7f50 / Win 0x4ec310: TextLayout::CreateArgs
// only; a font must be a FontPlatform (no font is fine).
Object* CreatePlatformTextLayout(OpenGLRender* render, const ObjectFactoryArgs& args)
{
    const TextLayout::CreateArgs* createArgs = dynamic_cast<const TextLayout::CreateArgs*>(&args);
    if (!createArgs)
        return 0;
    FontPlatform* font = 0;
    if (createArgs->font) {
        font = dynamic_cast<FontPlatform*>(createArgs->font);
        if (!font)
            return 0;
    }
    return new PlatformTextLayout(static_cast<Render*>(render), createArgs->text, font,
                                  createArgs->horizontalAlignment, createArgs->verticalAlignment,
                                  createArgs->maxWidth);
}

// ---------------------------------------------------------------------------------------------

// Mac 0x100fc0, Win 0x500860
PlatformTextLayout::PlatformTextLayout(Render* render, const String& text, FontPlatform* font,
                                       UInt32 horizontalAlignment, UInt32 verticalAlignment,
                                       UInt32 maxWidth)
    : TextLayoutRender(text, font, horizontalAlignment, verticalAlignment, maxWidth),
      m_textureDirty(true),
      m_texture(),
      m_render(render)
{
}

// Mac 0x101890 / 0x101950 (body 0x1017e0: render, texture, TextLayout), Win 0x500ef0
PlatformTextLayout::~PlatformTextLayout()
{
}

#if defined(_WIN32)
// Win 0x500aa0
HDC PlatformTextLayout::GetMeasureDC()
{
    HDC dc = ::GetDC(0);
    ::SetMapMode(dc, MM_TEXT);
    return dc;
}
#endif

// Mac 0x102360, Win 0x500850
void PlatformTextLayout::NotifyChanged()
{
    m_textureDirty = true;
    TextLayout::NotifyChanged();
}

// Mac 0x1010a0, Win 0x500970
Rect PlatformTextLayout::GetTexCoords()
{
    UpdateLayout();
    Vector2 textureSize = GetTextureSize();
    if (fabsf(textureSize.x) < 1e-05f || fabsf(textureSize.y) < 1e-05f)
        return Rect(0.0f, 0.0f, 0.0f, 0.0f);
    return Rect(1.0f / textureSize.x, 1.0f / textureSize.y,
                (m_size.x + 1.0f) / textureSize.x, (m_size.y + 1.0f) / textureSize.y);
}

// Mac 0x101170, Win 0x500a40
Vector2 PlatformTextLayout::GetTextureSize()
{
    UpdateTexture();
    if (!m_texture)
        return Vector2();
    return m_texture->GetImageSize();
}

// Mac 0x1013b0, Win 0x500a90
Texture* PlatformTextLayout::GetTexture()
{
    UpdateTexture();
    return m_texture;
}

// Mac 0x1011c0, Win 0x500d20
void PlatformTextLayout::UpdateTexture()
{
    UpdateLayout();
    if (!m_textureDirty)
        return;
    m_textureDirty = false;
    m_texture = 0;   // cleared, then released (as the original)
    if (!(1.0f <= m_size.x && 1.0f <= m_size.y))
        return;
    Texture::MutableCreateArgs2D args;
    args.priority = 0.0f;
    args.wrapMode = Texture::kWrapRepeat;
    args.alphaMode = Texture::kAlphaImage;   // 2
    args.width = (UInt32)(Int32)ceilf(m_size.x) + 2;
    args.height = (UInt32)(Int32)ceilf(m_size.y) + 2;
    args.format = kBitmapFormatRGBA32;
    Object* object = m_render->GetObjectFactory()->CreateObject(typeid(Texture), args);
    m_texture = object ? dynamic_cast<Texture*>(object) : 0;
    if (m_texture) {
        RefPointer<BitmapObject> bitmap = m_texture->GetBitmap(0);
        RenderText(bitmap->GetBitmap());
        bitmap->Update();
    }
}

namespace {

// RGB of the font colour as R | G << 8 | B << 16 (bytes R, G, B, A in memory): truncating
// float → int conversions (Mac 0x1013d0 inline, Win 0x501260).
UInt32 PackColor(const Color& color)
{
    return ((UInt32)(Int32)(color.b * 255.0f) & 0xff) << 16 |
           ((UInt32)(Int32)(color.g * 255.0f) & 0xff) << 8 |
           ((UInt32)(Int32)(color.r * 255.0f) & 0xff);
}

// The common tail of Mac 0x1013d0 / Win 0x500ac0: every pixel gets the colour; inner pixels get
// the coverage byte as alpha, the 1-pixel border 0. Inner pixel (x, y)'s coverage is
// coverage[(y - offset) * coveragePitch + (x - offset) * 4 + channel].
void FillColorAndAlpha(Bitmap& bitmap, UInt32 color, const UInt8* coverage, Int32 coveragePitch,
                       UInt32 offset, UInt32 channel)
{
    UInt32* row = (UInt32*)bitmap.m_data;
    for (UInt32 x = 0; x < bitmap.m_width; ++x)
        row[x] = color;
    row = (UInt32*)((UInt8*)row + bitmap.m_pitch);
    for (UInt32 y = 1; y < bitmap.m_height - 1; ++y) {
        const UInt8* source = coverage + (Int32)(y - offset) * coveragePitch;
        row[0] = color;
        UInt32 x = 1;
        for (; x < bitmap.m_width - 1; ++x)
            row[x] = (UInt32)source[(x - offset) * 4 + channel] << 24 | color;
        row[x] = color;
        row = (UInt32*)((UInt8*)row + bitmap.m_pitch);
    }
    for (UInt32 x = 0; x < bitmap.m_width; ++x)
        row[x] = color;
}

} // namespace

#if defined(_WIN32)
// Win 0x500ac0: GDI draws white text on black into a top-down 32-bit DIB of the text size; its
// blue channel is the coverage.
void PlatformTextLayout::RenderText(Bitmap& bitmap)
{
    HFONT font = static_cast<FontPlatform*>(m_font.Get())->GetNativeFont();
    // NOTE: the font is also selected into the screen DC for the duration (as the original).
    HDC screen = ::GetDC(0);
    HGDIOBJ oldScreenFont = ::SelectObject(screen, font);
    BITMAPINFO info;
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = (LONG)bitmap.m_width - 2;
    info.bmiHeader.biHeight = 2 - (LONG)bitmap.m_height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    info.bmiHeader.biSizeImage = 0;
    info.bmiHeader.biXPelsPerMeter = 0;
    info.bmiHeader.biYPelsPerMeter = 0;
    info.bmiHeader.biClrUsed = 0;
    info.bmiHeader.biClrImportant = 0;
    HDC dc = ::CreateCompatibleDC(0);
    void* bits = 0;
    HBITMAP dib = ::CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, 0, 0);
    HGDIOBJ oldBitmap = ::SelectObject(dc, dib);
    ::SetMapMode(dc, MM_TEXT);
    ::SetBkMode(dc, OPAQUE);
    ::SetTextColor(dc, m_textColor);
    ::SetBkColor(dc, 0);
    HGDIOBJ oldFont = ::SelectObject(dc, font);
    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = (LONG)m_size.x;
    rect.bottom = (LONG)m_size.y;
    DrawTextInto(dc, &rect);
    if (oldFont)
        ::SelectObject(dc, oldFont);
    ::SelectObject(screen, oldScreenFont);
    ::ReleaseDC(0, screen);
    FillColorAndAlpha(bitmap, PackColor(m_font->GetColor()), (const UInt8*)bits,
                      ((Int32)bitmap.m_width - 2) * 4, 1, 0);
    ::SelectObject(dc, oldBitmap);
    ::DeleteObject(dib);
    ::DeleteDC(dc);
}

#elif defined(__APPLE__)
// Mac 0x1013d0: CoreGraphics draws the CTFrame straight into the texture bitmap (premultiplied
// RGBA, font smoothing off) one pixel in from the bottom-left; its alpha is the coverage.
void PlatformTextLayout::RenderText(Bitmap& bitmap)
{
    CGColorSpaceRef space = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
    CGContextRef context = CGBitmapContextCreate(bitmap.m_data, bitmap.m_width, bitmap.m_height,
                                                 8, bitmap.m_pitch, space,
                                                 kCGImageAlphaPremultipliedLast);
    CGContextSetShouldSmoothFonts(context, false);
    CGContextClearRect(context, CGRectMake(0, 0, (CGFloat)bitmap.m_width, (CGFloat)bitmap.m_height));
    CGContextSetTextMatrix(context, CGAffineTransformIdentity);
    CGContextTranslateCTM(context, 1.0f, 1.0f);
    DrawFrame(context);
    CGContextRelease(context);
    CGColorSpaceRelease(space);
    FillColorAndAlpha(bitmap, PackColor(m_font->GetColor()), (const UInt8*)bitmap.m_data,
                      bitmap.m_pitch, 0, 3);
}

#else
// Linux / HOST-ONLY stand-in for Mac 0x1013d0: the host engine writes coverage into the alpha bytes of
// the cleared bitmap, one pixel in from the top-left.
void PlatformTextLayout::RenderText(Bitmap& bitmap)
{
    for (UInt32 y = 0; y < bitmap.m_height; ++y)
        memset((UInt8*)bitmap.m_data + (Int32)y * bitmap.m_pitch, 0, bitmap.m_width * 4);
    PlatformText::Target target;
    target.pixels = (UInt8*)bitmap.m_data;
    target.width = bitmap.m_width;
    target.height = bitmap.m_height;
    target.pitch = bitmap.m_pitch;
    target.bytesPerPixel = 4;
    target.channel = 3;
    target.x = 1;
    target.y = 1;
    DrawFrame(&target);
    FillColorAndAlpha(bitmap, PackColor(m_font->GetColor()), (const UInt8*)bitmap.m_data,
                      bitmap.m_pitch, 0, 3);
}
#endif

} // namespace LEGO
