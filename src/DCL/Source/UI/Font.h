// LEGO Digital Designer — restored source
// LEGO::Font — abstract font: a colour plus a platform font object (FontPlatform, TU 118).
// Font::SystemTypeFontArgs / SystemFontArgs: the two ways to create one via
// Render::GetObjectFactory()->CreateObject(typeid(Font), args) (FontPlatformFactory).
// Header only: no out-of-line code (the implicit dtor follows GetColor in FontPlatform's Mac
// vtable 0x72169c {GetColor 0x1d5f80, D1 0x1d5b40, D0 0x1d5c20}). Win: Font vtable 0xa2b09c
// {_purecall} + Object-in 0xa2b0a4. The argument blocks' dtors are weak copies in their users
// (Mac SystemTypeFontArgs 0xefad0/0xf0e30, SystemFontArgs 0x2ce4e0/0x2cefc0; Win sdd 0x455e80 /
// 0x46c410).
// System font types = the "typeFontName" values of Scripts/Fonts.xml (parser TU 322, Win
// 0x46d0f0): boldSystemFont 0, controlContentFont 1, labelFont 2, menuFont 3, menuBarFont 4,
// messageFont 5, paletteFont 6, systemFont 7, titleBarFont 8, toolTipsFont 9 (enumerator and
// member names inferred).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "ObjectFactory.h"
#include "Color.h"
#include "LEGO/String.h"

namespace LEGO {

class Font : public virtual Object {
public:
    enum SystemType {
        kBoldSystemFont     = 0,
        kControlContentFont = 1,
        kLabelFont          = 2,
        kMenuFont           = 3,
        kMenuBarFont        = 4,
        kMessageFont        = 5,
        kPaletteFont        = 6,
        kSystemFont         = 7,
        kTitleBarFont       = 8,
        kToolTipsFont       = 9
    };

    // A font of one of the system font types. Mac vtable 0x7630e8, RTTI 0x76f920; Win vtable
    // 0x9cd50c. Mac/Win sizeof 0x1c. Inline ctor (Mac TU 145 0x207b00, Win 0x455e10): {kSystemFont,
    // 0, white}.
    struct SystemTypeFontArgs : public ObjectFactoryArgs {
        SystemTypeFontArgs() : type(kSystemFont), size(0), color(Color::White()) {}
        virtual ~SystemTypeFontArgs() {}                              // Mac 0xefad0/0xf0e30, Win 0x455e80
        UInt32 type;    // +0x04 SystemType
        UInt32 size;    // +0x08 pixels; 0 = the system's size for the type
        Color  color;   // +0x0c
    };

    // A font by face name ("Fonts.xml" fontName). Mac vtable 0x775ae8, RTTI 0x775af0; Win vtable
    // 0xa23788. Mac sizeof 0x2c, Win 0x20. Inline ctor Win 0x46c390.
    struct SystemFontArgs : public ObjectFactoryArgs {
        SystemFontArgs() : size(0), color(Color::White()), bold(false), italic(false) {}
        virtual ~SystemFontArgs() {}                                  // Mac 0x2ce4e0/0x2cefc0, Win 0x46c410
        String name;    // +0x04
        UInt32 size;    // Mac +0x14, Win +0x08 pixels
        Color  color;   // Mac +0x18, Win +0x0c
        bool   bold;    // Mac +0x28, Win +0x1c
        bool   italic;  // Mac +0x29, Win +0x1d
    };

    // The text colour (FontPlatform +0x04 Mac / +0x08 Win). (name inferred)
    virtual const Color& GetColor() const = 0;                       // Mac slot 0, Win slot 0
};

} // namespace LEGO
