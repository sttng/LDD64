// LEGO Digital Designer — restored source
// LEGO::Config — INI-style file of named sections of key = value pairs (virtual file system);
// names compared case-insensitively. Mac vtable 0x7039a8 {D1 0x3ae0, D0 0x3af0, Load 0x4290,
// Save 0x5000}, Win 0xa200e4 {sdd 0x4335f0, Load 0x433920, Save 0x433fa0}; KeyEnumerator Win
// vtable 0xa1f4e8 {sdd 0x4180f0, _purecall}. Nested class names from RTTI, members inferred.
// Mac sizeof(Config) 0x48, Win 0x58.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicList.h"
#include "BasicMap.h"
#include "Path.h"
#include "File.h"

namespace LEGO {

class Config {
public:
    // One key = value pair. Mac sizeof 0x20, Win 0x08. (Win ctor 0x433350, dtor 0x4333c0)
    class Key {
    public:
        Key(const String& name, const String& value = String()) : m_name(name), m_value(value) {}

        String m_name;    // Mac +0x00, Win +0x00
        String m_value;   // Mac +0x10, Win +0x04
    };

    // Orders keys by name, case-insensitively.
    class KeyLess {
    public:
        bool operator()(const Key* a, const Key* b) const { return a->m_name.LessNoCase(b->m_name); }
    };

    // Callback for EnumerateKeys(); return false to stop the enumeration.
    class KeyEnumerator {
    public:
        virtual ~KeyEnumerator() {}                                              // Win sdd 0x4180f0
        virtual bool OnKey(const String& name, const String& value) = 0;         // Mac slot 2, Win slot 1
    };

    // A named section ("" = the keys before the first [section] header).
    // Keys are kept in file order (list) and indexed by name (set). Mac sizeof 0x38, Win 0x48.
    class Section {
    public:
        explicit Section(const String& name) : m_name(name) {}                  // Mac inlined, Win 0x433420
        ~Section();                                                              // Mac 0x5510, Win 0x4334a0

        void Clear();                                                            // Mac 0x5440, Win 0x434560
        Key* FindKey(const String& name) const;                                  // Mac 0x3c90, Win 0x4348d0
        // Appends a new key (no duplicate check); null for an empty name.
        Key* AddKey(const String& name, const String& value);                    // Mac 0x3f80, Win 0x434630
        bool RemoveKey(const String& name);                                      // Mac 0x40f0, Win 0x434700
        // Always returns true.
        bool EnumerateKeys(KeyEnumerator& enumerator) const;                     // Mac inlined, Win 0x4349b0

        String                       m_name;     // Mac +0x00, Win +0x00
        BasicList<Key*>              m_keys;     // Mac +0x10, Win +0x04
        BasicSet<Key*, KeyLess>      m_keySet;   // Mac +0x1c, Win +0x24

    private:
        Section(const Section&);
        Section& operator=(const Section&);
    };

    // Orders sections by name, case-insensitively.
    class SectionLess {
    public:
        bool operator()(const Section* a, const Section* b) const { return a->m_name.LessNoCase(b->m_name); }
    };

    Config();                                                                    // Mac 0x3620 (C2) / 0x37b0 (C1), Win inlined
    virtual ~Config();                                                           // Mac 0x37c0 (D2) / 0x3ae0 / 0x3af0, Win 0x4335f0

    // Replaces the contents by the file at `path`. Fails while a file is held open. With
    // `keepOpen` the file stays open (read/write) until the next Clear(); otherwise it is
    // opened shared-read. Reads ASCII, UTF-16LE (BOM FF FE) or UTF-16BE (BOM FE FF) text.
    virtual bool Load(const Path& path, bool keepOpen);                          // Mac 0x4290, Win 0x433920
    // Writes all sections to the held file or to `path` (created; kept open with `keepOpen`).
    // Writes ASCII when possible, UTF-16LE with a BOM otherwise.
    virtual bool Save(const Path& path, bool keepOpen);                          // Mac 0x5000, Win 0x433fa0

    // Deletes all sections, closes the held file and forgets the path.
    void Clear();                                                                // Mac 0x3940, Win 0x4336a0

    Section* FindSection(const String& name) const;                              // Mac 0x3b90, Win 0x434490
    // Null when a section of that name already exists.
    Section* AddSection(const String& name);                                     // Mac 0x3ea0, Win 0x4343d0

    // `value` is cleared when the section or key does not exist.
    bool GetValue(const String& section, const String& key, String& value) const; // Mac 0x3b30, Win 0x4337f0
    // Creates the section and the key when needed.
    bool SetValue(const String& section, const String& key, const String& value); // Mac 0x3de0, Win 0x433850
    bool RemoveKey(const String& section, const String& key);                     // Mac 0x40b0, Win inlined
    bool EnumerateKeys(const String& section, KeyEnumerator& enumerator) const;   // Mac 0x4040, Win inlined

    const Path& GetPath() const { return m_path; }                               // (accessor, not located)

private:
    Config(const Config&);
    Config& operator=(const Config&);

    BasicList<Section*>                 m_sections;     // Mac +0x04, Win +0x04 (file order)
    BasicSet<Section*, SectionLess>     m_sectionSet;   // Mac +0x10, Win +0x24 (by name)
    Path                                m_path;         // Mac +0x2c, Win +0x48 (last loaded/saved)
    RefPointer<File>                    m_file;         // Mac +0x44, Win +0x54 (kept open)
};

} // namespace LEGO
