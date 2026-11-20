// LEGO Digital Designer — restored source
// Base/Color.cpp — Mac TU 274 (0x28ded0-0x28e5ef, 29 functions), Win 0x41f730-0x41fd4f.
// See Color.h. Not restored as code: the static initialiser 0x28e5b0 (`#include <iostream>`).
#include "Color.h"
#include "MathUtil.h"
#include "StringStream.h"
#include <stdlib.h>

namespace LEGO {

static const Real32 kColorEpsilon = 0.001f;   // (name inferred)

Color::Color() : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {}              // Mac 0x28ded0, Win 0x41f730
Color::Color(Real32 r_, Real32 g_, Real32 b_, Real32 a_) : r(r_), g(g_), b(b_), a(a_) {} // Mac 0x28def0

void Color::Set(UInt8 r_, UInt8 g_, UInt8 b_, UInt8 a_)            // Mac 0x28e190, Win 0x41fa60
{
    r = (Real32)r_ / 255.0f;
    g = (Real32)g_ / 255.0f;
    b = (Real32)b_ / 255.0f;
    a = (Real32)a_ / 255.0f;
}

bool Color::EqualsRGB(const Color& c) const                         // Mac 0x28df20, Win 0x41f740
{
    return IsEqual(c.r, r, kColorEpsilon) && IsEqual(c.g, g, kColorEpsilon) &&
           IsEqual(c.b, b, kColorEpsilon);
}

void Color::PremultiplyAlpha()                                      // Mac 0x28df80
{
    r *= a;
    g *= a;
    b = a * b;
}

// Mac: maxps with 0 then minps with 1 (a NaN component becomes 0); Win compares with <= 0 and
// >= 1 (a NaN component stays NaN).
void Color::Clamp()                                                 // Mac 0x28dfc0, Win 0x41f7d0
{
#if defined(_WIN32)
    r = r <= 0.0f ? 0.0f : (r >= 1.0f ? 1.0f : r);
    g = g <= 0.0f ? 0.0f : (g >= 1.0f ? 1.0f : g);
    b = b <= 0.0f ? 0.0f : (b >= 1.0f ? 1.0f : b);
    a = a <= 0.0f ? 0.0f : (a < 1.0f ? a : 1.0f);
#else
    r = Min(Max(r, 0.0f), 1.0f);
    g = Min(Max(g, 0.0f), 1.0f);
    b = Min(Max(b, 0.0f), 1.0f);
    a = Min(Max(a, 0.0f), 1.0f);
#endif
}

bool Color::operator==(const Color& c) const                        // Mac 0x28dff0, Win 0x41f8e0
{
    return EqualsRGB(c) && IsEqual(c.a, a, kColorEpsilon);
}

bool Color::operator!=(const Color& c) const                        // Mac 0x28e070, Win 0x41f930
{
    return !(EqualsRGB(c) && IsEqual(c.a, a, kColorEpsilon));
}

bool Color::operator<(const Color& c) const                         // Mac 0x28e0f0, Win 0x41f990
{
    if (!IsEqual(r, c.r, kColorEpsilon))
        return r < c.r;
    if (!IsEqual(b, c.b, kColorEpsilon))
        return b < c.b;
    if (!IsEqual(g, c.g, kColorEpsilon))
        return g < c.g;
    return IsLess(a, c.a, kColorEpsilon);
}

Color Color::FromBytes(UInt8 r_, UInt8 g_, UInt8 b_, UInt8 a_)     // Mac 0x28e1e0, Win 0x41faa0
{
    Color c;
    c.Set(r_, g_, b_, a_);
    return c;
}

Color Color::Black()     { return Color(0.0f, 0.0f, 0.0f, 1.0f); }  // Mac 0x28e230, Win 0x41fb00
Color Color::Blue()      { return Color(0.0f, 0.0f, 1.0f, 1.0f); }  // Mac 0x28e250, Win 0x41fb20
Color Color::Brown()     { return Color(0.6f, 0.4f, 0.2f, 1.0f); }  // Mac 0x28e270, Win 0x41fb40
Color Color::Clear()     { return Color(0.0f, 0.0f, 0.0f, 0.0f); }  // Mac 0x28e290, Win 0x41fb70
Color Color::Cyan()      { return Color(0.0f, 1.0f, 1.0f, 1.0f); }  // Mac 0x28e2c0, Win 0x41fb90
Color Color::DarkGray()  { return Color(1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f, 1.0f); } // Mac 0x28e2e0, Win 0x41fbb0
Color Color::Gray()      { return Color(0.5f, 0.5f, 0.5f, 1.0f); }  // Mac 0x28e300, Win 0x41fbd0
Color Color::LightGray() { return Color(2.0f / 3.0f, 2.0f / 3.0f, 2.0f / 3.0f, 1.0f); } // Mac 0x28e320, Win 0x41fbf0
Color Color::Green()     { return Color(0.0f, 1.0f, 0.0f, 1.0f); }  // Mac 0x28e340, Win 0x41fc10
Color Color::Magenta()   { return Color(1.0f, 0.0f, 1.0f, 1.0f); }  // Mac 0x28e360, Win 0x41fc30
Color Color::Orange()    { return Color(1.0f, 0.5f, 0.0f, 1.0f); }  // Mac 0x28e380, Win 0x41fc50
Color Color::Purple()    { return Color(0.5f, 0.0f, 0.5f, 1.0f); }  // Mac 0x28e3a0, Win 0x41fc70
Color Color::Red()       { return Color(1.0f, 0.0f, 0.0f, 1.0f); }  // Mac 0x28e3c0, Win 0x41fc90
Color Color::White()     { return Color(1.0f, 1.0f, 1.0f, 1.0f); }  // Mac 0x28e3e0, Win 0x41fcb0
Color Color::Yellow()    { return Color(1.0f, 1.0f, 0.0f, 1.0f); }  // Mac 0x28e400, Win 0x41fcd0

Color Color::Random()                                               // Mac 0x28e420, Win 0x41fcf0
{
    int red = rand();
    int green = rand();
    int blue = rand();
    return Color((Real32)red / (Real32)RAND_MAX, (Real32)green / (Real32)RAND_MAX,
                 (Real32)blue / (Real32)RAND_MAX, 1.0f);
}

bool FromString(Color& value, const String& text, UInt32& position) // Mac 0x28e4b0
{
    if (!FromString(value.r, text, position))
        return false;
    ++position;
    if (!FromString(value.g, text, position))
        return false;
    ++position;
    if (!FromString(value.b, text, position))
        return false;
    ++position;
    return FromString(value.a, text, position);
}

bool FromStringOptional(Color& value, const String& text, UInt32& position) // Mac 0x28e530
{
    if (!FromStringOptional(value.r, text, position))
        return false;
    ++position;
    if (!FromStringOptional(value.g, text, position))
        return false;
    ++position;
    if (!FromStringOptional(value.b, text, position))
        return false;
    ++position;
    return FromStringOptional(value.a, text, position);
}

} // namespace LEGO
