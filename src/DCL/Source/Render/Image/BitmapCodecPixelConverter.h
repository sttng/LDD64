// LEGO Digital Designer — restored source
// Pixel-format conversion between a file's format and a Bitmap's, for every codec:
// BitmapPixelFormat (channel layout), BitmapPixelSource (+ colour key), BitmapPixelDestination,
// BitmapPixelConverter (routine per (source bpp, destination bpp)). All names inferred.
// TU 300: Mac 0x2aac30-0x2af5df, Win 0x699160-0x6991df (out-of-line ctors) + 0x69f9b0-0x6a2cff.
// Routines are cdecl fn(width, height, source, destination); the structure layouts match the
// original (the restoration's diff tests call the Win routines with them).
#pragma once
#include "LEGO/Types.h"
#include "Color.h"
#include "Bitmap.h"

namespace LEGO {

// sizeof 0x58. Channel index order everywhere: 0 red, 1 green, 2 blue, 3 alpha.
struct BitmapPixelFormat {
    // Mac 0x2aac30 (with the mask table inlined), Win 0x69fc50 thiscall(format, palette).
    void Init(UInt32 format, const Color* palette);
    // Channel masks of the packed formats; zero for the others (incl. 0x410 and 0x218).
    // Win 0x69f9b0 cdecl(format, &r, &g, &b, &a); inlined on Mac.
    static void GetMasks(UInt32 format, UInt32& red, UInt32& green, UInt32& blue, UInt32& alpha);

    UInt32         m_byteIndex[4];   // +0x00 byte position of each channel (24/32-bit formats)
    UInt32         m_shift[4];       // +0x10 bit position of each channel
    UInt32         m_loss[4];        // +0x20 8 - channel bits (16-bit formats; 0x1f = absent)
    const UInt32*  m_expand[4];      // +0x30 n-bit value -> 8-bit value tables (16-bit formats)
    UInt32         m_mask[4];        // +0x40
    const Color*   m_palette;        // +0x50
    UInt32         m_format;         // +0x54
};

// sizeof 0x74. Win ctor 0x699160 thiscall(format, data, pitch, palette); inlined on Mac.
struct BitmapPixelSource {
    BitmapPixelSource() {}
    BitmapPixelSource(UInt32 format, const void* data, Int32 pitch, const Color* palette)
    {
        Set(format, data, pitch, palette);
    }
    void Set(UInt32 format, const void* data, Int32 pitch, const Color* palette)
    {
        m_pixelFormat.Init(format, palette);
        m_colorKey = Color();
        m_pitch = pitch;
        m_useColorKey = false;
        m_data = data;
    }

    BitmapPixelFormat m_pixelFormat;  // +0x00
    Color             m_colorKey;     // +0x58 pixels with this RGB lose their alpha
    bool              m_useColorKey;  // +0x68
    const void*       m_data;         // +0x6c
    Int32             m_pitch;        // +0x70
};

// sizeof 0x60. Win ctor 0x6991a0 thiscall(format, data, pitch, palette); inlined on Mac.
struct BitmapPixelDestination {
    BitmapPixelDestination() {}
    BitmapPixelDestination(UInt32 format, void* data, Int32 pitch, const Color* palette)
    {
        m_pixelFormat.Init(format, palette);
        m_pitch = pitch;
        m_data = data;
    }

    BitmapPixelFormat m_pixelFormat;  // +0x00
    void*             m_data;         // +0x58
    Int32             m_pitch;        // +0x5c
};

// sizeof 0xc. Mac ctor 0x2aef50 (-> 0x2ab110), Win 0x6a2b00 thiscall(source, destination).
class BitmapPixelConverter {
public:
    typedef void (*ConvertFunction)(UInt32 width, UInt32 height,
                                    const BitmapPixelSource* source,
                                    const BitmapPixelDestination* destination);

    BitmapPixelConverter(const BitmapPixelSource* source, const BitmapPixelDestination* destination);

    // NOTE: no null check in either build: an unsupported pair (e.g. a 2-bit source, or any
    // source/destination bpp outside 1/4/8/16/24/32) calls address 0.
    void Convert(UInt32 width, UInt32 height) const                   // Win 0x6991d0; inlined on Mac
    {
        m_function(width, height, m_source, m_destination);
    }
    ConvertFunction GetFunction() const { return m_function; }

    // The routines the original selects (Mac addresses / Win addresses in the .cpp).
    static ConvertFunction Select(UInt32 sourceFormat, UInt32 destinationFormat);

private:
    ConvertFunction               m_function;     // +0x00
    const BitmapPixelSource*      m_source;       // +0x04
    const BitmapPixelDestination* m_destination;  // +0x08
};

} // namespace LEGO
