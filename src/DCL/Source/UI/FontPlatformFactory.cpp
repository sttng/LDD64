// LEGO Digital Designer — restored source
// FontPlatform / FontPlatformFactory — TU 118 (Mac 0x1d5730-0x1d5fcf), Win 0x53a630-0x53aacf.
// Platform parts: Win GDI; Mac CoreText (`__APPLE__`); otherwise Linux
// (Platform/Linux/LinuxText.h; the tests' HOST-ONLY builds: compat/host_app/HostText.h) for the Linux-hosted Mac branch.
#include "FontPlatformFactory.h"
#include "OpenGLFactories.h"

#if defined(_WIN32)
#include "Win32Unicode.h"
#include <string.h>
#elif defined(__APPLE__)
#include <CoreText/CoreText.h>
#else
#if defined(LDD_LINUX)
#include "LinuxText.h"
namespace LEGO { namespace PlatformText = LinuxText; }
#else
#include "host_app/HostText.h"   // HOST-ONLY (the Linux-hosted Mac branch of the tests)
namespace LEGO { namespace PlatformText = HostText; }
#endif
#endif

#include <typeinfo>

namespace LEGO {

// ---------------------------------------------------------------------------------------------
// OpenGLRender registration hooks (OpenGLFactories.h; `new FontPlatformFactory` and typeid()
// in OpenGLRender::Initialize, Mac 0xf2f00).
// ---------------------------------------------------------------------------------------------
SpecificObjectFactory* CreateFontPlatformFactory() { return new FontPlatformFactory; }
const std::type_info& TypeOfFont() { return typeid(Font); }
const std::type_info& TypeOfFontPlatform() { return typeid(FontPlatform); }

// Mac 0x1d5b40 (D1) / 0x1d5c20 (D0), Win 0x53a930 / sdd 0x53a6f0
FontPlatform::~FontPlatform()
{
    if (m_native) {
#if defined(_WIN32)
        ::DeleteObject(m_native);
#elif defined(__APPLE__)
        CFRelease((CTFontRef)m_native);
#else
        PlatformText::DestroyFont((PlatformText::Font*)m_native);
#endif
    }
}

// Mac 0x1d5f80, Win 0x53a630
const Color& FontPlatform::GetColor() const
{
    return m_color;
}

// Mac 0x1d5730, Win 0x53a720
bool FontPlatform::Create(const SystemFontArgs& args)
{
    m_color = args.color;
    m_color.Clamp();
#if defined(_WIN32)
    LOGFONTW font;
    font.lfHeight = -(LONG)args.size;
    font.lfWidth = 0;
    font.lfEscapement = 0;
    font.lfOrientation = 0;
    font.lfWeight = args.bold ? FW_BOLD : FW_NORMAL;           // 700 / 400
    font.lfItalic = args.italic;
    font.lfUnderline = 0;
    font.lfStrikeOut = 0;
    // NOTE: the original reads the character set from the ANSI code page record (Win 0x433210
    // +8), which is initialised to 1 (DEFAULT_CHARSET) and never changed.
    font.lfCharSet = DEFAULT_CHARSET;
    font.lfOutPrecision = OUT_DEFAULT_PRECIS;
    font.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    font.lfQuality = ANTIALIASED_QUALITY;                      // 4
    font.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;         // 0x22
    // NOTE: unbounded copy of the face name into the 32-character field (as the original).
    const UniChar16* name = args.name.GetBuffer();
    static const UniChar16 kEmpty[1] = { 0 };
    if (!name)
        name = kEmpty;
    UInt32 i = 0;
    do {
        font.lfFaceName[i] = (WCHAR)name[i];
    } while (name[i++] != 0);
    m_native = PlatformCreateFontIndirect(&font);
    return m_native != 0;
#elif defined(__APPLE__)
    CFMutableDictionaryRef attributes = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionaryAddValue(attributes, kCTFontNameAttribute, args.name.GetCFString());
    if (args.italic || args.bold) {
        SInt32 traits = (args.bold ? kCTFontBoldTrait : 0) | (args.italic ? kCTFontItalicTrait : 0);
        CFNumberRef number = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &traits);
        CFMutableDictionaryRef traitDict = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
            &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        CFDictionaryAddValue(traitDict, kCTFontSymbolicTrait, number);
        CFDictionaryAddValue(attributes, kCTFontTraitsAttribute, traitDict);
        CFRelease(traitDict);
        CFRelease(number);
    }
    CTFontDescriptorRef descriptor = CTFontDescriptorCreateWithAttributes(attributes);
    m_native = (void*)CTFontCreateWithFontDescriptor(descriptor, (CGFloat)args.size, 0);
    CFRelease(descriptor);
    CFRelease(attributes);
    return true;
#else
    // Linux / HOST-ONLY: the face name is ignored (the desktop UI font / the one embedded face);
    // 13 px like the system font.
    m_native = PlatformText::CreateFont(args.size, args.bold, args.italic, 13);
    return true;
#endif
}

// Mac 0x1d58d0, Win 0x53a800
bool FontPlatform::Create(const SystemTypeFontArgs& args)
{
    m_color = args.color;
    m_color.Clamp();
#if defined(_WIN32)
    LOGFONTA font;
    font.lfHeight = args.size == 0 ? -10 : -(LONG)args.size;
    font.lfWidth = 0;
    font.lfEscapement = 0;
    font.lfOrientation = 0;
    font.lfWeight = FW_NORMAL;
    font.lfItalic = 0;
    font.lfUnderline = 0;
    font.lfStrikeOut = 0;
    font.lfCharSet = DEFAULT_CHARSET;                          // see Create(SystemFontArgs)
    font.lfOutPrecision = OUT_DEFAULT_PRECIS;
    font.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    font.lfQuality = ANTIALIASED_QUALITY;
    font.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
    strcpy(font.lfFaceName, "Tahoma");
    switch (args.type) {
    case kBoldSystemFont:
        font.lfWeight = FW_BOLD;
        break;
    case kTitleBarFont:
        if (args.size == 0)
            font.lfHeight = -13;
        font.lfWeight = FW_BOLD;
        strcpy(font.lfFaceName, "Trebuchet MS");
        break;
    default:
        break;
    }
    m_native = ::CreateFontIndirectA(&font);
    return m_native != 0;
#elif defined(__APPLE__)
    CTFontUIFontType type;
    switch (args.type) {
    case kBoldSystemFont:     type = kCTFontUIFontEmphasizedSystem; break;      // 3
    case kControlContentFont: type = kCTFontUIFontPushButton; break;            // 16
    case kLabelFont:          type = kCTFontUIFontLabel; break;                 // 10
    case kMenuFont:           type = kCTFontUIFontMenuItem; break;              // 12
    case kMenuBarFont:        type = kCTFontUIFontMenuTitle; break;             // 11
    case kMessageFont:
    case kSystemFont:         type = kCTFontUIFontSystem; break;                // 2
    case kPaletteFont:        type = kCTFontUIFontUtilityWindowTitle; break;    // 17
    case kTitleBarFont:       type = kCTFontUIFontWindowTitle; break;           // 15
    case kToolTipsFont:       type = kCTFontUIFontSmallSystem; break;           // 4
    default:
        return false;
    }
    m_native = (void*)CTFontCreateUIFontForLanguage(type, (CGFloat)args.size, 0);
    return m_native != 0;
#else
    // Linux / HOST-ONLY: default sizes and weights close to the Mac UI fonts of each type.
    UInt32 defaultSize;
    bool bold = false;
    switch (args.type) {
    case kBoldSystemFont:     defaultSize = 13; bold = true; break;
    case kControlContentFont: defaultSize = 13; break;
    case kLabelFont:          defaultSize = 10; break;
    case kMenuFont:
    case kMenuBarFont:        defaultSize = 14; break;
    case kMessageFont:
    case kSystemFont:         defaultSize = 13; break;
    case kPaletteFont:        defaultSize = 11; break;
    case kTitleBarFont:       defaultSize = 13; break;
    case kToolTipsFont:       defaultSize = 11; break;
    default:
        return false;
    }
    m_native = PlatformText::CreateFont(args.size, bold, false, defaultSize);
    return m_native != 0;
#endif
}

// Mac 0x1d5d30, Win 0x53a9c0
Object* FontPlatformFactory::CreateObject(const ObjectFactoryArgs& args)
{
    if (const Font::SystemFontArgs* named = dynamic_cast<const Font::SystemFontArgs*>(&args)) {
        FontPlatform* font = new FontPlatform;
#if defined(_WIN32)
        if (!font->Create(*named)) {
            font->Release();   // refcount 0 → -1: deleted (as the original)
            return 0;
        }
#else
        font->Create(*named);   // NOTE: the Mac build ignores the result (always true there)
#endif
        return font;
    }
    if (const Font::SystemTypeFontArgs* typed = dynamic_cast<const Font::SystemTypeFontArgs*>(&args)) {
        FontPlatform* font = new FontPlatform;
        if (!font->Create(*typed)) {
            font->Release();
            return 0;
        }
        return font;
    }
    return 0;
}

} // namespace LEGO
