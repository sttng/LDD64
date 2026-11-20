// LEGO Digital Designer — restored source
// LEGO::FontPlatform — the platform Font: a colour and a native font (Mac CTFontRef, Win HFONT;
// HOST-ONLY: HostText::Font*). LEGO::FontPlatformFactory — SpecificObjectFactory for
// Font::SystemFontArgs / SystemTypeFontArgs (registered by OpenGLRender::Initialize).
// TU 118 "FontPlatformFactory.cpp" (Mac 0x1d5730-0x1d5fcf, Win 0x53a630-0x53aacf + 0x4ec2f0).
// Vtables: FontPlatform Mac 0x72169c {GetColor 0x1d5f80, D1 0x1d5b40, D0 0x1d5c20} + Object-in
//   0x7216c0; Win 0xa2b0b8 {GetColor 0x53a630} + Object-in 0xa2b0c0 {sdd 0x53a6f0, …}.
//   FontPlatformFactory Mac 0x7216dc {D1 0x1d5eb0, D0 0x1d5f10, Object ×3, CreateObject
//   0x1d5d30, Initialize, Shutdown}; Win 0xa26004 {sdd 0x4ec2f0, …, CreateObject 0x53a9c0}.
// Layout: FontPlatform Mac {vptr, Color +0x04, CTFontRef +0x14, Object +0x18} (sizeof 0x20);
//   Win {vfptr, vbptr +4, Color +0x08, HFONT +0x18, Object +0x1c} (sizeof 0x24).
//   FontPlatformFactory: SpecificObjectFactory only (Mac sizeof 0x10). Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "ObjectFactory.h"
#include "Color.h"
#include "Font.h"

#if defined(_WIN32)
#include <windows.h>
#endif

namespace LEGO {

class FontPlatform : public Font {
public:
#if defined(_WIN32)
    typedef HFONT NativeFont;
#else
    typedef void* NativeFont;   // CTFontRef (HOST-ONLY: HostText::Font*)
#endif

    FontPlatform() : m_native(0) {}                                    // inline (Mac 0x1d5d30, Win 0x53a640)
    virtual ~FontPlatform();                                           // Mac 0x1d5b40/0x1d5c20, Win 0x53a930/0x53a6f0

    virtual const Color& GetColor() const;                             // Mac 0x1d5f80, Win 0x53a630

    NativeFont GetNativeFont() const { return m_native; }

    // Creates the native font. Mac: CTFontDescriptor {name, symbolic traits} → CTFont (always
    // true); Win: LOGFONT → CreateFontIndirect (true when a font was created).
    bool Create(const SystemFontArgs& args);                           // Mac 0x1d5730, Win 0x53a720
    // Mac: CTFontCreateUIFontForLanguage; Win: Tahoma / Trebuchet MS.
    bool Create(const SystemTypeFontArgs& args);                       // Mac 0x1d58d0, Win 0x53a800

protected:
    Color      m_color;    // Mac +0x04, Win +0x08 (white until created)
    NativeFont m_native;   // Mac +0x14, Win +0x18
};

class FontPlatformFactory : public SpecificObjectFactory {
public:
    FontPlatformFactory() {}
    virtual ~FontPlatformFactory() {}                                  // Mac 0x1d5eb0/0x1d5f10, Win sdd 0x4ec2f0

    // SystemFontArgs or SystemTypeFontArgs → new FontPlatform (not referenced); null for other
    // arguments and for a failed SystemTypeFontArgs font (Win: for any failed font).
    virtual Object* CreateObject(const ObjectFactoryArgs& args);       // Mac 0x1d5d30, Win 0x53a9c0
};

} // namespace LEGO
