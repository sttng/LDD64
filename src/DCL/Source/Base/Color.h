// LEGO Digital Designer — restored source
// LEGO::Color — RGBA float colour (struct in the Win RTTI: BasicArray<struct LEGO::Color>).
//   Mac TU 274 (0x28ded0-0x28e5ef), Win 0x41f730-0x41fd4f. Member names inferred.
#pragma once
#include "LEGO/Types.h"

namespace LEGO {

class String;

struct Color {
    Color();                                                // Mac 0x28ded0, Win 0x41f730 (white)
    Color(Real32 r, Real32 g, Real32 b, Real32 a);          // Mac 0x28def0

    // r, g, b within 0.001 (alpha ignored). (name inferred)
    bool EqualsRGB(const Color& c) const;                   // Mac 0x28df20, Win 0x41f740
    void PremultiplyAlpha();                                // Mac 0x28df80 (name inferred)
    void Clamp();                                           // Mac 0x28dfc0, Win 0x41f7d0 (each into [0, 1])
    bool operator==(const Color& c) const;                  // Mac 0x28dff0, Win 0x41f8e0 (0.001)
    bool operator!=(const Color& c) const;                  // Mac 0x28e070, Win 0x41f930
    // NOTE: compares r, then b, then g, then alpha (0.001 tolerance), in that order.
    bool operator<(const Color& c) const;                   // Mac 0x28e0f0, Win 0x41f990

    // Components / 255 (one divps on Mac). Not a constructor: it would make Color(0, 1, 1, 1)
    // ambiguous. Win 0x41fa60 is an /LTCG clone with alpha = 255 folded in. (name inferred)
    void Set(UInt8 r, UInt8 g, UInt8 b, UInt8 a);           // Mac 0x28e190, Win 0x41fa60
    // Set() on a new colour, returned by value. (name inferred)
    static Color FromBytes(UInt8 r, UInt8 g, UInt8 b, UInt8 a); // Mac 0x28e1e0, Win 0x41faa0

    // The Cocoa NSColor palette (same values, same order). (names inferred)
    static Color Black();                                   // Mac 0x28e230, Win 0x41fb00
    static Color Blue();                                    // Mac 0x28e250, Win 0x41fb20
    static Color Brown();                                   // Mac 0x28e270, Win 0x41fb40 (0.6 0.4 0.2)
    static Color Clear();                                   // Mac 0x28e290, Win 0x41fb70 (alpha 0)
    static Color Cyan();                                    // Mac 0x28e2c0, Win 0x41fb90
    static Color DarkGray();                                // Mac 0x28e2e0, Win 0x41fbb0 (1/3)
    static Color Gray();                                    // Mac 0x28e300, Win 0x41fbd0 (1/2)
    static Color LightGray();                               // Mac 0x28e320, Win 0x41fbf0 (2/3)
    static Color Green();                                   // Mac 0x28e340, Win 0x41fc10
    static Color Magenta();                                 // Mac 0x28e360, Win 0x41fc30
    static Color Orange();                                  // Mac 0x28e380, Win 0x41fc50 (1 0.5 0)
    static Color Purple();                                  // Mac 0x28e3a0, Win 0x41fc70 (0.5 0 0.5)
    static Color Red();                                     // Mac 0x28e3c0, Win 0x41fc90
    static Color White();                                   // Mac 0x28e3e0, Win 0x41fcb0
    static Color Yellow();                                  // Mac 0x28e400, Win 0x41fcd0
    // rand() / RAND_MAX per component, alpha 1.
    static Color Random();                                  // Mac 0x28e420, Win 0x41fcf0

    Real32 r;   // +0x00
    Real32 g;   // +0x04
    Real32 b;   // +0x08
    Real32 a;   // +0x0c
};

// "r<sep>g<sep>b<sep>a" (any one separator character between the numbers); position is
// advanced. The Optional form accepts "#" for "keep this component". See StringStream.h.
bool FromString(Color& value, const String& text, UInt32& position);         // Mac 0x28e4b0
bool FromStringOptional(Color& value, const String& text, UInt32& position); // Mac 0x28e530

} // namespace LEGO
