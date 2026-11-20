// LEGO Digital Designer — restored source
// Preferences, PreferencesInstantiator, ListKeyEnumerator — TU 055 (Mac 0x11adc0-0x11d8fb,
// Win 0x4181f0-0x419a8f, 0x41c700-0x41c82f). See Preferences.h.
//
// Not restored separately: the BasicMap<String, String, StringCaseInsensitiveLess> tree
// instances (Mac 0x11d1d0 operator[], 0x11d330 insert-with-hint), the BasicArray dtors
// 0x11d110-0x11d1a0 and the static initialiser 0x11d880 (`#include <iostream>` + header
// statics).
#include "Preferences.h"
#include "ObjectRegistry.h"
#include "Directory.h"
#include "Platform.h"
#include "Trace.h"
#include "Error.h"
#include "LEGO/Assert.h"

namespace LEGO {

// NOTE: both builds pass a TU-static error code that is zero in the image (Mac 0x77d570,
// Win 0xb42dc0); its symbolic name is unknown.
static const ErrorCode kPreferencesErrorCode = 0;

// String -> bool through String::Parse (0/1/no/yes/false/true). Mac 0x142710 (weak helper
// shared by several TUs). (name inferred)
static bool ParseBool(bool& value, const String& text)
{
    UInt32 position = 0;
    return text.Parse(value, position);
}

// ---------------------------------------------------------------------------
// PreferencesInstantiator — Mac vtable 0x7645cc {D1 0x11aea0 (empty), D0 0x11ce40,
// CreateObject 0x11ce50}, Win sdd 0x418800, CreateObject 0x418750.
// ---------------------------------------------------------------------------
class PreferencesInstantiator : public ObjectRegistryInstantiator {
public:
    virtual RefPointer<Object> CreateObject() const                   // Mac 0x11ce50, Win 0x418750
    {
        return RefPointer<Object>(new Preferences);
    }
};

// ---------------------------------------------------------------------------
// ListKeyEnumerator — copies every key of a Config section into a map.
// RTTI LEGO::ListKeyEnumerator; Mac vtable {D1 0x11c1a0, D0 0x11d0c0, OnKey 0x11d0d0},
// Win {sdd 0x418e30, OnKey 0x418e00}.
// ---------------------------------------------------------------------------
class ListKeyEnumerator : public Config::KeyEnumerator {
public:
    explicit ListKeyEnumerator(BasicMap<String, String>& values) : m_values(&values) {}
    virtual ~ListKeyEnumerator() {}                                    // Mac 0x11c1a0/0x11d0c0

    virtual bool OnKey(const String& name, const String& value)       // Mac 0x11d0d0, Win 0x418e00
    {
        (*m_values)[name] = value;
        return true;
    }

private:
    BasicMap<String, String>* m_values;   // +0x04
};

// ===========================================================================
// Preferences
// ===========================================================================

// Mac 0x11ceb0, Win 0x4181f0
Preferences::Preferences()
    : m_path(String(), 0)
{
}

// Mac D1 0x11cc80 / D0 0x11cce0 (body 0x11d620), Win 0x418110 / 0x418310
Preferences::~Preferences()
{
    m_user.Save(m_path, false);
}

// Mac 0x11adc0, Win 0x418870
RefPointer<Preferences> Preferences::GetInstance()
{
    PreferencesInstantiator instantiator;
    RefPointer<Object> object = ObjectRegistry::GetInstance()->Get(0x50524546 /* 'PREF' */, instantiator);
    if (object.IsNull())
        return RefPointer<Preferences>();
    return RefPointer<Preferences>(dynamic_cast<Preferences*>(object.Get()));
}

// Mac 0x11aeb0, Win 0x418960
void Preferences::Initialize(const String& applicationName, BasicArray<String>& arguments,
                             const ValueMap& defaults)
{
    if (&m_defaults != &defaults)
        m_defaults = defaults;
    ParseArguments(arguments);
    InitializeDirectories();
    LoadDefaults();
    LoadUser(applicationName);

    String traceName;
    if (GetValue(String("TraceName"), traceName))
        SetTraceName(traceName);

    bool verbose = false;
    if (GetValue(String("VerboseTrace"), verbose) && verbose)
        SetTraceFlags(GetTraceFlags() | kTraceVerbose);

    bool noAssertBox = false;
    if (GetValue(String("dontshowassertbox"), noAssertBox) && noAssertBox)
        SetAssertMode(kAssertNoBox);

    // NOTE: both builds read this one through the registered instance, not `this`.
    bool noAssertBoxContinue = false;
    RefPointer<Preferences> instance = GetInstance();
    bool found = instance->GetValue(String("dontshowassertboxcontinue"), noAssertBoxContinue);
    instance = 0;
    if (found && noAssertBoxContinue)
        SetAssertMode(kAssertNoBoxContinue);
}

// "-key value" pairs (a trailing "-key" gets an empty value); consumed entries are removed
// from `arguments`, the rest stays. Mac 0x11b270, Win 0x419260
void Preferences::ParseArguments(BasicArray<String>& arguments)
{
    UInt32 i = 0;
    while (i < arguments.GetSize()) {
        while (arguments[i].StartsWith((UniChar16)'-')) {
            String key(arguments[i], 1, arguments[i].GetLength() - 1);
            arguments.Erase(i);
            if (i == arguments.GetSize()) {
                m_arguments[key] = String();
                return;
            }
            m_arguments[key] = arguments[i];
            arguments.Erase(i);
            if (i == arguments.GetSize())
                return;
        }
        ++i;
    }
}

// Mac 0x11b420, Win 0x4198e0
void Preferences::InitializeDirectories()
{
    InitializeDirectory(String("appDirectory"), Platform::kAppDirectory);
    InitializeDirectory(String("tempDirectory"), Platform::kTempDirectory);
    InitializeDirectory(String("appDataDirectory"), Platform::kAppDataDirectory);
    InitializeDirectory(String("documentDirectory"), Platform::kDocumentDirectory);
    InitializeDirectory(String("favoriteDirectory"), Platform::kFavoriteDirectory);
    InitializeDirectory(String("pictureDirectory"), Platform::kPictureDirectory);
    InitializeDirectory(String("musicDirectory"), Platform::kMusicDirectory);
    InitializeDirectory(String("movieDirectory"), Platform::kMovieDirectory);
    InitializeDirectory(String("resourceDirectory"), Platform::kResourceDirectory);
    InitializeDirectory(String("assetsDirectory"), Platform::kAssetsDirectory);
}

// A preference holding a native path replaces one of the Platform directories.
// Mac 0x11c4c0, Win 0x419760
void Preferences::InitializeDirectory(const String& key, UInt32 kind)
{
    String value;
    if (!GetValue(key, value))
        return;
    RefPointer<Directory> directory =
        Directory::GetRoot()->GetDirectory(Platform::GetInstance()->NativeToVFSPath(value), false);
    if (directory.IsNull()) {
        String message("Preferences could not open ");
        message << key << " with path: " << value;
        ReportError(kPreferencesErrorCode, message);
    } else {
        Platform::GetInstance()->SetSystemDirectory(kind, directory, true);
    }
}

// <resources>/preferences.ini. Mac 0x11b6c0, Win 0x419440
// NOTE: the original loads it through m_path, the path Save and the destructor write the user
// values to; LoadUser replaces it only once it has the user directory, so without one (no app
// data directory, or "LEGO Company/<app>" cannot be created) the shipped file is overwritten
// with the user values. fixed: the shipped file is loaded through a local path, m_path only
// ever names the user file; without one Save fails and nothing is written.
void Preferences::LoadDefaults()
{
    RefPointer<Directory> directory =
        Platform::GetInstance()->GetSystemDirectory(Platform::kResourceDirectory, true);
    if (directory.IsNull())
        return;
    Path path(String("preferences.ini"), directory);
    m_shipped.Load(path, false);
}

// <app data>/LEGO Company/<applicationName>/preferences.ini (directories are created).
// Mac 0x11b850, Win 0x419540
bool Preferences::LoadUser(const String& applicationName)
{
    RefPointer<Directory> directory =
        Platform::GetInstance()->GetSystemDirectory(Platform::kAppDataDirectory, true);
    if (directory.IsNull())
        return false;
    directory = directory->GetDirectory(String("LEGO Company"), true);
    if (directory.IsNull())
        return false;
    directory = directory->GetDirectory(applicationName, true);
    if (directory.IsNull())
        return false;
    m_path = Path(String("preferences.ini"), directory);
    return m_user.Load(m_path, false);
}

// Mac 0x11bb10, Win 0x418c50
bool Preferences::GetValue(const String& key, String& value) const
{
    ValueMap::ConstIterator it = m_arguments.Find(key);
    if (it != m_arguments.End()) {
        value = it->second;
        return true;
    }
    if (m_user.GetValue(String(), key, value))
        return true;
    if (m_shipped.GetValue(String(), key, value))
        return true;
    it = m_defaults.Find(key);
    if (it == m_defaults.End())
        return false;
    value = it->second;
    return true;
}

// Win 0x41c700 (inlined on Mac)
bool Preferences::GetValue(const String& key, bool& value) const
{
    String text;
    if (!GetValue(key, text))
        return false;
    return ParseBool(value, text);
}

// Mac 0x11bcc0
bool Preferences::GetBool(const String& key, bool defaultValue) const
{
    bool value = false;
    if (!GetValue(key, value))
        return defaultValue;
    return value;
}

// Mac 0x11bd50, Win 0x418b60
void Preferences::SetValue(const String& key, const String& value)
{
    m_user.SetValue(String(), key, value);
    Iterator it(*this);
    while (PreferencesObserver* observer = it.Next(m_observers))
        observer->OnPreferenceChanged(key);
}

// Mac 0x11c1b0
void Preferences::RemoveValue(const String& key)
{
    m_user.RemoveKey(String(), key);
    Iterator it(*this);
    while (PreferencesObserver* observer = it.Next(m_observers))
        observer->OnPreferenceChanged(key);
}

// Mac 0x11c490
bool Preferences::Save()
{
    return m_user.Save(m_path, false);
}

// Mac 0x11c030
void Preferences::GetAll(BasicMap<String, String>& out) const
{
    out.Clear();
    for (ValueMap::ConstIterator it = m_defaults.Begin(); it != m_defaults.End(); ++it)
        out[it->first] = it->second;
    ListKeyEnumerator enumerator(out);
    m_shipped.EnumerateKeys(String(), enumerator);
    m_user.EnumerateKeys(String(), enumerator);
    for (ValueMap::ConstIterator it = m_arguments.Begin(); it != m_arguments.End(); ++it)
        out[it->first] = it->second;
}

// Mac 0x11c690
void Preferences::Dump(String& out) const
{
    BasicMap<String, String> values;
    GetAll(values);
    for (BasicMap<String, String>::ConstIterator it = values.Begin(); it != values.End(); ++it)
        out << it->first << " = " << it->second << "\n";
}

} // namespace LEGO
