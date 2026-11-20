// LEGO Digital Designer — restored source
// LEGO::Preferences — settings from command-line overrides, the user's preferences.ini (saved on
// Save and destruction), the shipped preferences.ini and defaults (Config through the VFS on both
// platforms); PreferencesObserver hears SetValue/RemoveValue. ObjectRegistry key 'PREF'.
// TU 055 (Mac 0x11adc0-0x11d8fb, guessed "Path.cpp"; Win 0x4181f0-0x419a8f + 0x41c7d0..).
// Mac vtables: Preferences 0x76448c {D1 0x11cc80, D0 0x11cce0, OnFinalRelease 0x11c8a0}, Object
// part 0x7644b0; PreferencesObserver 0x704d2c {D1 0x52000, D0 0x52030, 0x45080, 0x52080, 0x52090,
// OnPreferenceChanged 0x520a0}; PreferencesInstantiator 0x7645cc {D1 0x11aea0, D0 0x11ce40,
// CreateObject 0x11ce50}; ListKeyEnumerator (.cpp) {0x11c1a0, 0x11d0c0, OnKey 0x11d0d0}.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "Notifier.h"
#include "Config.h"
#include "Path.h"

namespace LEGO {

class Preferences;

class PreferencesObserver : public Observer<Preferences, PreferencesObserver> {
public:
    virtual ~PreferencesObserver() {}                                 // Mac 0x52000/0x52030
    // `key` was set or removed. (name from LuaPreferencesObserver, which forwards it to the
    // Lua function "OnPreferenceChanged")
    virtual void OnPreferenceChanged(const String& /*key*/) {}        // Mac 0x520a0, Win slot via 0x46b610
};

// Mac sizeof 0x10c (Object virtual base at +0x104), Win sizeof 0x134 (Object at +0x12c).
class Preferences : public Notifier<PreferencesObserver> {
public:
    typedef BasicMap<String, String, StringCaseInsensitiveLess<String> > ValueMap;

    Preferences();                                                    // Mac 0x11ceb0, Win 0x4181f0
    virtual ~Preferences();                                           // Mac 0x11cc80/0x11cce0 (body 0x11d620)

    // The registered instance (created through PreferencesInstantiator on first use).
    // (name inferred)
    static RefPointer<Preferences> GetInstance();                     // Mac 0x11adc0, Win 0x418870

    // Takes the built-in `defaults`, consumes "-key [value]" pairs from `arguments`, applies
    // the "…Directory" keys to the Platform directories, loads <resources>/preferences.ini
    // and <app data>/LEGO Company/<applicationName>/preferences.ini, then applies
    // TraceName, VerboseTrace, dontshowassertbox, dontshowassertboxcontinue. (name inferred)
    void Initialize(const String& applicationName, BasicArray<String>& arguments,
                    const ValueMap& defaults);                        // Mac 0x11aeb0, Win 0x418960

    // Command line, then user file, then shipped file, then built-in defaults; keys are
    // case-insensitive. (names inferred)
    bool GetValue(const String& key, String& value) const;            // Mac 0x11bb10, Win 0x418c50
    // GetValue + String::Parse(bool). Probably a template GetValue<T> in the original
    // (Win instance 0x41c700; inlined on Mac, helper 0x142710). (name inferred)
    bool GetValue(const String& key, bool& value) const;              // Win 0x41c700
    bool GetBool(const String& key, bool defaultValue) const;         // Mac 0x11bcc0
    // Writes to the user file (in memory) and notifies the observers.
    void SetValue(const String& key, const String& value);            // Mac 0x11bd50, Win 0x418b60
    void RemoveValue(const String& key);                              // Mac 0x11c1b0
    // Writes the user file.
    bool Save();                                                      // Mac 0x11c490

    // All values (defaults, shipped, user, command line; later ones win). (name inferred)
    void GetAll(BasicMap<String, String>& out) const;                 // Mac 0x11c030
    // "key = value\n" for every value. (name inferred)
    void Dump(String& out) const;                                     // Mac 0x11c690

private:
    void ParseArguments(BasicArray<String>& arguments);               // Mac 0x11b270, Win 0x419260
    void InitializeDirectories();                                     // Mac 0x11b420, Win 0x4198e0
    void InitializeDirectory(const String& key, UInt32 kind);         // Mac 0x11c4c0, Win 0x419760
    void LoadDefaults();                                              // Mac 0x11b6c0, Win 0x419440
    bool LoadUser(const String& applicationName);                     // Mac 0x11b850, Win 0x419540

    // Layout (Mac): vptr +0x00, observers +0x04, iterators +0x14 (NotifierT)
    ValueMap m_arguments;   // +0x24  "-key value" from the command line
    Config   m_shipped;     // +0x40  <resources>/preferences.ini
    Config   m_user;        // +0x88  <app data>/LEGO Company/<app>/preferences.ini
    ValueMap m_defaults;    // +0xd0  built-in defaults (Initialize)
    Path     m_path;        // +0xec  the user file
    // Object (virtual base) +0x104
};

} // namespace LEGO
