// LDD64 Linux platform layer (not restored code): the text engine standing in for CoreText in the
// Linux branches of FontPlatformFactory.cpp, TextLayout.cpp and PlatformTextLayout.cpp: Pango /
// cairo with the desktop's UI font, word wrap or truncation, alignment, 8-bit coverage.
#pragma once
#if defined(LDD_LINUX)
#include "LEGO/Types.h"
#include "LEGO/String.h"

namespace LEGO {
namespace LinuxText {

struct Font;
struct Frame;

// The Mac UI font types map to sizes like CTFontCreateUIFontForLanguage(type, 0) would (13 px
// system fonts, 11 px small / tool tips); `size` 0 picks `defaultSize`.
Font*  CreateFont(UInt32 size, bool bold, bool italic, UInt32 defaultSize);
void   DestroyFont(Font* font);
UInt32 GetFontSize(const Font* font);
// The font's PangoFontDescription (native controls use the font too).
const void* GetFontDescription(const Font* font);

// Lays `text` out ("\r\n" already folded by the caller). maxWidth 0: no limit. With a limit,
// words wrap unless `truncate` is set, then each line ends in "…" when too long.
// horizontalAlignment: 0 left, 1 right, 2 center (inside the frame width).
Frame* CreateFrame(const Font* font, const String& text, Real32 maxWidth, bool truncate,
                   UInt32 horizontalAlignment);
void   DestroyFrame(Frame* frame);
// Width of the widest line and the sum of the line heights (not rounded).
void   GetFrameSize(const Frame* frame, Real32& width, Real32& height);
// Sets the frame width used for alignment (the final frame of the Mac code has the size of
// the text, so alignment acts inside that width).
void   SetFrameWidth(Frame* frame, Real32 width);

// A drawing target: the CGBitmapContext stand-in of PlatformTextLayout.
struct Target {
    UInt8* pixels;          // top-down rows
    UInt32 width;
    UInt32 height;
    Int32  pitch;
    UInt32 bytesPerPixel;
    UInt32 channel;         // byte of each pixel that receives the coverage
    Int32  x;               // origin of the text (top-left)
    Int32  y;
};

// Adds the coverage (0-255) of the text, its top-left corner at (x, y), to the byte at
// `channel` of each `bytesPerPixel`-byte pixel of a top-down image.
void   DrawFrame(const Frame* frame, UInt8* pixels, UInt32 width, UInt32 height, Int32 pitch,
                 UInt32 bytesPerPixel, UInt32 channel, Int32 x, Int32 y);

} // namespace LinuxText
} // namespace LEGO
#endif
