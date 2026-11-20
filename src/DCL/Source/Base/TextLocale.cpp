// LEGO Digital Designer — restored source
// TU "TextLocale.cpp"  Mac 0x1a950-0x1cdbf (34 functions), Win 0x40c3a0, 0x419e20, 0x41a070,
// 0x41fed0-0x4210ff. See TextLocale.h (classes, .loc format).
// Templates emitted here (not written out): Mac 0x1b820 / 0x1c0d0 SectionMap D1/D0 (Win 0x421000
//   / 0x421070, ctor 0x420f70), 0x1bfd0 / 0x1c000 StringMap D1/D0 (Win 0x419e20 / 0x41a070);
//   _M_erase 0x1c050 (StringMap) / 0x1c120 (SectionMap); SectionMap operator[] 0x1c1b0,
//   _M_insert_unique 0x1c3c0 / 0x1c6b0, _M_create_node 0x1c790; StringMap _M_copy 0x1c8a0,
//   _M_create_node 0x1c9b0, _M_insert_unique 0x1ca40; AString Reserve 0x1cb20; static init 0x1cd40.
#include "TextLocale.h"
#include "StringConversion.h"
#include "Directory.h"
#include "File.h"
#include "LEGO/StdLib.h"
#if !defined(LDD_TEXTLOCALE_SEAMS)
#include "Preferences.h"
#include "Platform.h"
#endif

#include <typeinfo>
#include <iostream>

namespace LEGO {

// ---------------------------------------------------------------------------------------------
// Services of other TUs, inlined at the original call sites (names inferred):
//   preference "language": PreferencesInstantiator / Preferences::GetInstance()->GetValue /
//     SetValue (TU 055: Mac 0x11adc0 + 0x11bb10 / 0x11bd50, Win 0x418870 + 0x418c50 / 0x418b60);
//   the assets directory: Platform::GetInstance()->GetSystemDirectory(9, true) (TU 010: Mac
//     0xd7f0 + 0xb990, Win 0x415020 + 0x412870; 9 = "<resources>/Assets", the mounted Assets.lif).
// LDD_TEXTLOCALE_SEAMS (test switch, not original): the tests define these three services.
//
// ---------------------------------------------------------------------------------------------
#if defined(LDD_TEXTLOCALE_SEAMS)
bool GetPreferenceString(const String& key, String& value);
void SetPreferenceString(const String& key, const String& value);
RefPointer<Directory> GetStandardDirectory(UInt32 which, bool create);
#else
namespace {
inline bool GetPreferenceString(const String& key, String& value)
{
    return Preferences::GetInstance()->GetValue(key, value);
}
inline void SetPreferenceString(const String& key, const String& value)
{
    Preferences::GetInstance()->SetValue(key, value);
}
inline RefPointer<Directory> GetStandardDirectory(UInt32 which, bool create)
{
    return Platform::GetInstance()->GetSystemDirectory(which, create);
}
} // namespace
#endif

namespace {

enum { kAssetsDirectory = 9 };                                        // (name inferred)

// Suffix of platform-specific string keys. Win 0x41fdd0 (member of the platform singleton,
// returns "WIN"); inlined on the Mac as the literal "MAC" right after the singleton accessor.
inline String GetPlatformName()
{
#if defined(_WIN32)
    return String("WIN");
#else
    return String("MAC");
#endif
}

} // namespace

// =============================================================================================
// TextLocale
// =============================================================================================

// Mac 0x1b850, Win 0x420760
TextLocale::TextLocale()
    : m_language(), m_path(String(), 0)
{
    // NOTE: m_language is always empty here; the original tests it anyway.
    if (m_language.IsEmpty() && !GetPreferenceString(String("language"), m_language))
        return;
    SetLanguage(m_language);
}

// Mac 0x1bab0 (D2) / 0x1bbd0 / 0x1bc90, Win 0x420930 / 0x420900
TextLocale::~TextLocale()
{
}

// Mac 0x1a950, Win 0x41ffe0
RefPointer<TextLocale> TextLocale::GetInstance()
{
    TextLocaleInstantiator instantiator;
    RefPointer<Object> object = ObjectRegistry::GetInstance()->Get(kRegistryId, instantiator);
    return dynamic_cast<TextLocale*>(object.Get());
}

// Mac 0x1b4c0, Win 0x420520
void TextLocale::SetLanguage(const String& language)
{
    m_language = language;
    SetPreferenceString(String("language"), m_language);
    m_path = Path(String(), 0);

    RefPointer<Directory> directory = GetStandardDirectory(kAssetsDirectory, true);
    if (directory) {
        directory = directory->GetDirectory(language, false);
        if (directory)
            m_path = Path(String("localizedStrings.loc"), directory);
    }
}

// Mac 0x1b340, Win 0x4203b0
bool TextLocale::AddStrings(Directory* directory)
{
    if (!HasStringFile())
        return false;
    String name(m_language);
    name << "/localizedStrings.loc";
    Path file(name, directory);
    return Load(file, m_path.GetPath(true));
}

// Mac inlined in 0x1bdb0, Win 0x4204b0
bool TextLocale::Localize(const String& key, String& out)
{
    if (HasStringFile() && Lookup(m_path, key, out) && !out.IsEmpty())
        return true;
    out = "??";
    out << key << "??";
    return false;
}

// Mac 0x1aa40, Win 0x4200d0
bool TextLocale::Lookup(const Path& file, const String& key, String& out)
{
    ScopedLock lock(m_lock);
    String section = file.GetPath(true);
    out.Clear();

    SectionMap::Iterator it = m_sections.Find(section);
    if (it == m_sections.End()) {
        if (!Load(file, file.GetPath(true)))
            return false;
        it = m_sections.Find(section);
        // NOTE: the original dereferences the result unchecked; an empty file (Load succeeds
        // without creating the section) makes it read the map's header node. Fixed: false.
        if (it == m_sections.End())
            return false;
    }

    StringMap& strings = it->second;
    StringMap::Iterator entry = strings.Find(key);
    if (entry == strings.End()) {
        String platformKey;
        platformKey << key << "_" << GetPlatformName();
        entry = strings.Find(platformKey);
        if (entry == strings.End())
            return false;
    }
    out = entry->second;
    return true;
}

// Mac 0x1adb0, Win 0x4209c0
bool TextLocale::Load(const Path& file, const String& section)
{
    RefPointer<File> stream = file.OpenFile(OpenMode(true, false, true, false), false);
    if (!stream)
        return false;
    UInt64 size = stream->GetSize();
    if (size == 0)
        return true;

    // PORT64: a 64-bit build refuses a file of 4 GB and more (allocated with the low 32 bits,
    // read with all 64); compiled out on 32-bit
    if (sizeof(void*) > 4 && size >= 0xFFFFFFFEu)
        return false;
    UInt32 length = (UInt32)size;
    char* buffer = new char[length + 2];
    if (!stream->ReadAt(0, size, buffer, 0)) {
        delete[] buffer;                // NOTE: the Mac build frees with scalar operator delete
        return false;
    }

    StringMap& strings = m_sections[section];       // created even for unknown versions
    buffer[length] = 0;
    buffer[length + 1] = 0;

    UInt32 version = StrToUInt32(buffer, 0, 10);
    UInt64 position = StrLen(buffer) + 1;
    bool result = true;
    if (version == 1) {
        // ASCII keys and values
        String key;
        String value;
        while (position + 3 < size) {
            key = buffer + position;
            UInt32 keyLength = key.GetLength();
            value = buffer + position + keyLength + 1;
            UInt32 valueLength = value.GetLength();
            strings.Insert(key, value);
            position += keyLength + 1 + valueLength + 1;
        }
    } else if (version == 2) {
        // ASCII keys, UTF-8 values
        String key;
        AString utf8;
        while (position + 3 < size) {
            key = buffer + position;
            UInt32 keyLength = key.GetLength();
            const char* text = buffer + position + keyLength + 1;
            UInt32 textLength = StrLen(text);
            utf8.Assign(text, textLength);
            String value = DecodeString(kTextEncodingUTF8, utf8.c_str(), utf8.GetLength());
            strings.Insert(key, value);
            position += keyLength + 1 + textLength + 1;
        }
    } else {
        result = false;
    }
    delete[] buffer;
    return result;
}

// =============================================================================================
// TextLocaleInstantiator
// =============================================================================================

// Mac 0x1bf70, Win 0x41fed0
RefPointer<Object> TextLocaleInstantiator::CreateObject() const
{
    return RefPointer<Object>(new TextLocale);
}

// =============================================================================================
// LocalizedString
// =============================================================================================

// Mac 0x1bd80, Win 0x420e10
LocalizedString::LocalizedString()
    : String("????")
{
}

// Mac 0x1bea0, Win 0x420e70
LocalizedString::LocalizedString(const String& key)
    : String()
{
    Load(key);
}

// Mac 0x1bf00 / 0x1bf20, Win 0x40c3a0
LocalizedString::~LocalizedString()
{
}

// Mac 0x1bdb0, Win 0x420ee0
bool LocalizedString::Load(const String& key)
{
    RefPointer<TextLocale> locale = TextLocale::GetInstance();
    return locale->Localize(key, *this);            // NOTE: no null check in either build
}

} // namespace LEGO
