// LEGO Digital Designer — restored source
// TextLayout — TU 121 "TextLayout.cpp" (Mac 0x1d80c0-0x1d8fff), Win 0x5386c0-0x538d5f and
// 0x500570-0x500650, with TextLayout.h's inlines (emitted in TU 040 on the Mac,
// 0x101b10-0x101ec0). Platform parts: Win GDI; Mac CoreText (`__APPLE__`); otherwise Linux
// (Platform/Linux/LinuxText.h; the tests' HOST-ONLY builds: compat/host_app/HostText.h).
#include "TextLayout.h"
#include "FontPlatformFactory.h"
#include "StringStream.h"

#if defined(_WIN32)
#include "Win32Unicode.h"
#elif defined(__APPLE__)
#include <CoreText/CoreText.h>
#include <float.h>
#else
#if defined(LDD_LINUX)
#include "LinuxText.h"
namespace LEGO { namespace PlatformText = LinuxText; }
#else
#include "host_app/HostText.h"   // HOST-ONLY (the Linux-hosted Mac branch of the tests)
namespace LEGO { namespace PlatformText = HostText; }
#endif
#include <float.h>
#include <math.h>
#endif

namespace LEGO {

namespace {

FontPlatform::NativeFont NativeFontOf(Font* font)
{
    // The original reads the native font straight out of the FontPlatform (Mac +0x14, Win
    // +0x18): PlatformTextLayoutFactory only accepts FontPlatform fonts.
    return static_cast<FontPlatform*>(font)->GetNativeFont();
}

} // namespace

// Mac 0x1d80c0 (C2, with VTT), Win 0x538590
TextLayout::TextLayout(const String& text, Font* font, UInt32 horizontalAlignment,
                       UInt32 verticalAlignment, UInt32 maxWidth)
    : m_text(text),
      m_font(font),
      m_horizontalAlignment(horizontalAlignment),
      m_verticalAlignment(verticalAlignment),
      m_dirty(true),
      m_truncate(false),
      m_maxWidth(maxWidth),
      m_size()
#if defined(_WIN32)
      , m_textColor(0xffffff),
      m_alignmentFormat(0)
#else
      , m_frame(0)
#endif
{
}

// Mac 0x101d90 / 0x101df0 (body 0x102370), Win 0x500650 / sdd 0x501390
TextLayout::~TextLayout()
{
#if defined(__APPLE__)
    if (m_frame) {
        CFRelease((CTFrameRef)m_frame);
        m_frame = 0;
    }
#elif !defined(_WIN32)
    if (m_frame) {
        PlatformText::DestroyFrame((PlatformText::Frame*)m_frame);
        m_frame = 0;
    }
#endif
}

// Mac 0x101b60, Win 0x500570
bool TextLayout::IsEmpty() const
{
    if (!m_font)
        return true;
    return m_text.IsEmpty();
}

// Mac 0x101b90, Win 0x5b7060
const String& TextLayout::GetText() const
{
    return m_text;
}

// Mac 0x1d8230, Win 0x538680
bool TextLayout::SetText(const String& text)
{
    if (!(text == m_text)) {
        m_text = text;
        if (m_font)
            NotifyChanged();
    }
    return true;
}

// Mac 0x101ba0, Win 0x500590
void TextLayout::GetFont(RefPointer<Font>& font) const
{
    font = m_font.Get();
}

// Mac 0x1d8280, Win 0x5386c0
bool TextLayout::SetFont(Font* font)
{
    if (font != m_font.Get()) {
        m_font = font;
        if (!m_text.IsEmpty())
            NotifyChanged();
    }
    return true;
}

// Mac 0x101bf0, Win 0x5005d0
void TextLayout::GetAlignment(UInt32& horizontal, UInt32& vertical) const
{
    horizontal = m_horizontalAlignment;
    vertical = m_verticalAlignment;
}

// Mac 0x1d82f0, Win 0x538700
bool TextLayout::SetAlignment(UInt32 horizontal, UInt32 vertical)
{
    if (m_horizontalAlignment != horizontal || m_verticalAlignment != vertical) {
        m_horizontalAlignment = horizontal;
        m_verticalAlignment = vertical;
        NotifyChanged();
    }
    return true;
}

// Mac 0x101c10, Win 0x5005f0 (ICF)
UInt32 TextLayout::GetHorizontalAlignment() const
{
    return m_horizontalAlignment;
}

// Mac 0x1d8320, Win 0x538730
bool TextLayout::SetHorizontalAlignment(UInt32 alignment)
{
    if (m_horizontalAlignment != alignment) {
        m_horizontalAlignment = alignment;
        NotifyChanged();
    }
    return true;
}

// Mac 0x101c20, Win 0x500600
UInt32 TextLayout::GetVerticalAlignment() const
{
    return m_verticalAlignment;
}

// Mac 0x1d8350, Win 0x538750
bool TextLayout::SetVerticalAlignment(UInt32 alignment)
{
    if (m_verticalAlignment != alignment) {
        m_verticalAlignment = alignment;
        NotifyChanged();
    }
    return true;
}

// Mac 0x1d8380 (UpdateLayout inlined), Win 0x538770
Vector2 TextLayout::GetSize()
{
    UpdateLayout();
    return m_size;
}

// Mac 0x101c30, Win 0x500610
UInt32 TextLayout::GetMaxWidth() const
{
    return m_maxWidth;
}

// Mac 0x1d8450, Win 0x538790
bool TextLayout::SetMaxWidth(UInt32 maxWidth)
{
    if (m_maxWidth != maxWidth) {
        m_maxWidth = maxWidth;
        NotifyChanged();
    }
    return true;
}

// Mac 0x101c40, Win 0x500620
bool TextLayout::GetTruncate() const
{
    return m_truncate;
}

// Mac 0x1d8480, Win 0x5387b0
bool TextLayout::SetTruncate(bool truncate)
{
    if (m_truncate != truncate) {
        m_truncate = truncate;
        NotifyChanged();
    }
    return true;
}

#if defined(_WIN32)
// Win 0x500630
// NOTE: a memory DC that ReleaseMeasureDC hands to ReleaseDC (not DeleteDC), as the original.
HDC TextLayout::GetMeasureDC()
{
    return ::CreateCompatibleDC(0);
}

// Win 0x500640 (also used by PlatformTextLayout)
void TextLayout::ReleaseMeasureDC(HDC dc)
{
    ::ReleaseDC(0, dc);
}
#endif

// Mac 0x1d8ab0, Win 0x538950
void TextLayout::NotifyChanged()
{
    m_dirty = true;
    Iterator it(*this);
    while (TextLayoutObserver* observer = it.Next(m_observers))
        observer->OnTextLayoutChanged(0);   // NOTE: no argument in the originals (TextLayout.h)
}

// Mac 0x1d83f0 (inlined in 0x1d8380 and the PlatformTextLayout users), Win 0x538850
void TextLayout::UpdateLayout()
{
    if (!m_dirty)
        return;
    m_dirty = false;
    if (IsEmpty()) {
        m_size = Vector2();
        return;
    }
    Layout(m_size);
}

#if defined(_WIN32)
// The layout part of Win 0x538850: measure with DrawText(DT_CALCRECT).
void TextLayout::Layout(Vector2& size)
{
    switch (m_horizontalAlignment) {
    case kAlignLeft:   m_alignmentFormat = DT_LEFT; break;     // 0
    case kAlignRight:  m_alignmentFormat = DT_RIGHT; break;    // 2
    case kAlignCenter: m_alignmentFormat = DT_CENTER; break;   // 1
    default: break;
    }
    HDC dc = GetMeasureDC();
    HGDIOBJ old = ::SelectObject(dc, NativeFontOf(m_font));
    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = m_maxWidth ? (LONG)m_maxWidth : 10000000;
    rect.bottom = 0;
    CalcText(dc, &rect);
    if (old)
        ::SelectObject(dc, old);
    ReleaseMeasureDC(dc);
    size = Vector2((Real32)(rect.right - rect.left), (Real32)(rect.bottom - rect.top));
}

// Win 0x538a00: DT_CALCRECT | DT_NOPREFIX | DT_EDITCONTROL, word break with a maximum width,
// ellipses instead when truncating.
int TextLayout::CalcText(HDC dc, RECT* rect)
{
    UINT wrap = (m_maxWidth && !m_truncate) ? DT_WORDBREAK : 0;
    UINT ellipsis = (m_maxWidth && m_truncate) ? (DT_WORD_ELLIPSIS | DT_END_ELLIPSIS | DT_PATH_ELLIPSIS) : 0;
    return PlatformDrawText(dc, m_text, m_text.GetLength(), rect,
                            ellipsis | wrap | DT_EDITCONTROL | DT_NOPREFIX | DT_CALCRECT);
}

// Win 0x538a60: the same flags without DT_CALCRECT, plus the alignment.
int TextLayout::DrawTextInto(HDC dc, RECT* rect)
{
    UINT wrap = (m_maxWidth && !m_truncate) ? DT_WORDBREAK : 0;
    UINT ellipsis = (m_maxWidth && m_truncate) ? (DT_WORD_ELLIPSIS | DT_END_ELLIPSIS | DT_PATH_ELLIPSIS) : 0;
    return PlatformDrawText(dc, m_text, m_text.GetLength(), rect,
                            m_alignmentFormat | ellipsis | wrap | DT_EDITCONTROL | DT_NOPREFIX);
}

#elif defined(__APPLE__)
// Mac 0x1d8570: CoreText layout. Size = the widest line and the sum of the line heights (ascent +
// descent + 0.7), rounded up; the frame kept for drawing is laid out again in exactly that size.
void TextLayout::Layout(Vector2& size)
{
    if (m_frame) {
        CFRelease((CTFrameRef)m_frame);
        m_frame = 0;
    }
    String text(m_text);
    text.ReplaceAll(String("\r\n"), String("\n"));
    CFMutableAttributedStringRef string = CFAttributedStringCreateMutable(kCFAllocatorDefault, 0);
    CFAttributedStringReplaceString(string, CFRangeMake(0, 0), text.GetCFString());
    CFAttributedStringSetAttribute(string, CFRangeMake(0, CFAttributedStringGetLength(string)),
                                   kCTFontAttributeName, (CTFontRef)NativeFontOf(m_font));
    // NOTE: left uninitialised for other values (as the original); fixed: left (low)
    CTTextAlignment alignment = kCTLeftTextAlignment;
    if (m_horizontalAlignment == kAlignLeft)
        alignment = kCTLeftTextAlignment;
    else if (m_horizontalAlignment == kAlignRight)
        alignment = kCTRightTextAlignment;
    else if (m_horizontalAlignment == kAlignCenter)
        alignment = kCTCenterTextAlignment;
    CTLineBreakMode lineBreak = (CTLineBreakMode)(m_truncate << 2);   // word wrap / truncate tail
    CTParagraphStyleSetting settings[2] = {
        { kCTParagraphStyleSpecifierAlignment, sizeof(alignment), &alignment },
        { kCTParagraphStyleSpecifierLineBreakMode, sizeof(lineBreak), &lineBreak }
    };
    CTParagraphStyleRef paragraph = CTParagraphStyleCreate(settings, 2);
    CFAttributedStringSetAttribute(string, CFRangeMake(0, CFAttributedStringGetLength(string)),
                                   kCTParagraphStyleAttributeName, paragraph);
    CFRelease(paragraph);
    CGColorSpaceRef space = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
    const Color& c = m_font->GetColor();
    CGFloat components[4] = { c.r, c.g, c.b, c.a };
    CGColorRef color = CGColorCreate(space, components);
    CGColorSpaceRelease(space);
    CFAttributedStringSetAttribute(string, CFRangeMake(0, CFAttributedStringGetLength(string)),
                                   kCTForegroundColorAttributeName, color);
    CFRelease(color);
    CTFramesetterRef framesetter = CTFramesetterCreateWithAttributedString(string);
    CGMutablePathRef path = CGPathCreateMutable();
    CGFloat width = m_maxWidth ? (CGFloat)m_maxWidth : FLT_MAX;
    CGPathAddRect(path, 0, CGRectMake(0, 0, width, FLT_MAX));
    CTFrameRef frame = CTFramesetterCreateFrame(framesetter,
        CFRangeMake(0, CFAttributedStringGetLength(string)), path, 0);
    size = Vector2();
    CFArrayRef lines = CTFrameGetLines(frame);
    CFIndex count = CFArrayGetCount(lines);
    for (CFIndex i = 0; i < count; ++i) {
        CGFloat ascent, descent, leading;
        double lineWidth = CTLineGetTypographicBounds((CTLineRef)CFArrayGetValueAtIndex(lines, i),
                                                      &ascent, &descent, &leading);
        if (size.x <= (Real32)lineWidth)
            size.x = (Real32)lineWidth;
        size.y = size.y + ascent + descent + 0.7f;
    }
    size.x = ceilf(size.x);
    size.y = ceilf(size.y);
    CGPathRelease(path);
    CFRelease(frame);
    CGMutablePathRef exact = CGPathCreateMutable();
    CGPathAddRect(exact, 0, CGRectMake(0, 0, size.x, size.y));
    m_frame = (void*)CTFramesetterCreateFrame(framesetter,
        CFRangeMake(0, CFAttributedStringGetLength(string)), exact, 0);
    CFRelease(string);
    CGPathRelease(exact);
    CFRelease(framesetter);
}

// Mac 0x1d8d20
void TextLayout::DrawFrame(void* context)
{
    CTFrameDraw((CTFrameRef)m_frame, (CGContextRef)context);
}

#else
// Linux / HOST-ONLY stand-in for Mac 0x1d8570 (same structure: measure with the maximum width, round
// up, keep a frame of exactly that width for drawing).
void TextLayout::Layout(Vector2& size)
{
    if (m_frame) {
        PlatformText::DestroyFrame((PlatformText::Frame*)m_frame);
        m_frame = 0;
    }
    String text(m_text);
    text.ReplaceAll(String("\r\n"), String("\n"));
    PlatformText::Frame* frame = PlatformText::CreateFrame((PlatformText::Font*)NativeFontOf(m_font), text,
                                                   (Real32)m_maxWidth, m_truncate,
                                                   m_horizontalAlignment);
    Real32 width, height;
    PlatformText::GetFrameSize(frame, width, height);
    size = Vector2(ceilf(width), ceilf(height));
    PlatformText::SetFrameWidth(frame, size.x);
    m_frame = frame;
}

// Linux / HOST-ONLY: `context` is a PlatformText::Target.
void TextLayout::DrawFrame(void* context)
{
    PlatformText::Target* target = (PlatformText::Target*)context;
    if (m_frame)
        PlatformText::DrawFrame((PlatformText::Frame*)m_frame, target->pixels, target->width,
                            target->height, target->pitch, target->bytesPerPixel,
                            target->channel, target->x, target->y);
}
#endif

// Mac 0x1d84b0, Win 0x5387d0
void TextLayout::AlignInArea(const Vector2& area, Vector2& position)
{
    Vector2 size = GetSize();
    switch (GetHorizontalAlignment()) {
    case kAlignCenter: position.x = (area.x - size.x) * 0.5f + position.x; break;
    case kAlignRight:  position.x = (area.x - size.x) + position.x; break;
    default: break;
    }
    switch (GetVerticalAlignment()) {
    case kAlignCenter: position.y = (area.y - size.y) * 0.5f + position.y; break;
    case kAlignBottom: position.y = (area.y - size.y) + position.y; break;
    default: break;
    }
}

// Mac 0x1d8d40, Win 0x538ac0
bool TextLayout::ParseHorizontalAlignment(UInt32& alignment, const String& text, UInt32& position)
{
    if (ParseKeyword(String("Left"), text, position, ',')) {
        alignment = kAlignLeft;
        return true;
    }
    if (ParseKeyword(String("Right"), text, position, ',')) {
        alignment = kAlignRight;
        return true;
    }
    if (ParseKeyword(String("Center"), text, position, ',')) {
        alignment = kAlignCenter;
        return true;
    }
    return false;
}

// Mac 0x1d8e80, Win 0x538c10
bool TextLayout::ParseVerticalAlignment(UInt32& alignment, const String& text, UInt32& position)
{
    if (ParseKeyword(String("Top"), text, position, ',')) {
        alignment = kAlignTop;
        return true;
    }
    if (ParseKeyword(String("Bottom"), text, position, ',')) {
        alignment = kAlignBottom;
        return true;
    }
    if (ParseKeyword(String("Center"), text, position, ',')) {
        alignment = kAlignCenter;
        return true;
    }
    return false;
}

} // namespace LEGO
