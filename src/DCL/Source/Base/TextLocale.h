// LEGO Digital Designer — restored source
// LEGO::TextLocale (UI language and translations), LocalizedString, TextLocaleInstantiator
// ('TLOC'). TU Mac 0x1a950-0x1cdbf, Win 0x40c3a0, 0x419e20, 0x41a070, 0x41fed0-0x4210ff. Class
// names RTTI, members inferred.
// .loc files (<Assets>/<language>/localizedStrings.loc, Assets/Languages.loc): "<version>\0"
//   (1 or 2), then "<key>\0<value>\0" pairs; values ASCII (v1) or UTF-8 (v2); keys case-insensitive,
//   first wins. Cached per VFS path ("section"). A missing key is retried as "<key>_MAC" / "_WIN".
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicMap.h"
#include "ObjectRegistry.h"
#include "Path.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace LEGO {

class Directory;

// Mac sizeof 0x7c, Win 0x58.
// Mac vtable 0x703f0c {D1 0x1bbd0, D0 0x1bc90}; Object-in-TextLocale 0x703f2c {virtual thunks
// 0x1bc30, 0x1bd00, OnFinalRelease 0x9250, AddRef 0x9260, Release 0x9280}; D2 (VTT) 0x1bab0.
// Win: no own vfptr; Object vtable 0xa1f600 at +0x50 {0x420900 (vtordisp thunk -> sdd),
// 0x459790, AddRef, Release}; dtor 0x420930.
class TextLocale : public virtual Object {
public:
    typedef BasicMap<String, String, StringCaseInsensitiveLess<String> >  StringMap;
    typedef BasicMap<String, StringMap, StringCaseInsensitiveLess<String> > SectionMap;

    enum { kRegistryId = 0x544c4f43 };                                // 'TLOC' (name inferred)

    // Takes the language from the "language" preference (if set) and selects it.
    TextLocale();                                                     // Mac 0x1b850, Win 0x420760
    virtual ~TextLocale();                                            // Mac 0x1bbd0/0x1bc90, Win 0x420930

    // The shared instance (ObjectRegistry 'TLOC', created on first use). (name inferred)
    static RefPointer<TextLocale> GetInstance();                      // Mac 0x1a950, Win 0x41ffe0

    // Stores the language in the preferences and points the string file at
    // "<Assets>/<language>/localizedStrings.loc" (no file when that directory is missing).
    void SetLanguage(const String& language);                         // Mac 0x1b4c0, Win 0x420520
    const String& GetLanguage() const { return m_language; }          // (accessor, not located)

    // True when a string file is selected (inline check in Mac 0x1b340 / 0x1bdb0). (name inferred)
    bool HasStringFile() const
    {
        return m_path.GetParentDirectory() != 0 || !m_path.GetName().IsEmpty();
    }

    // Merges "<directory>/<language>/localizedStrings.loc" into the strings of the selected
    // file (keys already present keep their value). (name inferred)
    bool AddStrings(Directory* directory);                            // Mac 0x1b340, Win 0x4203b0

    // Translation of `key` from the selected file; "??<key>??" (and false) when there is no
    // file, no such key or an empty translation. (name inferred)
    bool Localize(const String& key, String& out);                    // Mac inlined in 0x1bdb0, Win 0x4204b0

    // Looks `key` (then "<key>_<platform>") up in the string file `file`, loading and caching
    // it on first use. `out` is cleared first. (name inferred)
    bool Lookup(const Path& file, const String& key, String& out);    // Mac 0x1aa40, Win 0x4200d0

private:
    TextLocale(const TextLocale&);
    TextLocale& operator=(const TextLocale&);

    // Parses the string file `file` into m_sections[section]. True for an empty file. (name inferred)
    bool Load(const Path& file, const String& section);               // Mac 0x1adb0, Win 0x4209c0

    // Recursive mutex member (same inline shape as ObjectRegistry's). (name inferred)
    class Lock {
    public:
        Lock()
        {
#if defined(_WIN32)
            InitializeCriticalSection(&m_cs);
#else
            pthread_mutexattr_t attr;
            pthread_mutexattr_init(&attr);
            pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
            pthread_mutex_init(&m_mutex, &attr);
            pthread_mutexattr_destroy(&attr);
#endif
        }
        ~Lock()
        {
#if defined(_WIN32)
            DeleteCriticalSection(&m_cs);
#else
            pthread_mutex_destroy(&m_mutex);
#endif
        }
        void Acquire()
        {
#if defined(_WIN32)
            EnterCriticalSection(&m_cs);
#else
            pthread_mutex_lock(&m_mutex);
#endif
        }
        void Release()
        {
#if defined(_WIN32)
            LeaveCriticalSection(&m_cs);
#else
            pthread_mutex_unlock(&m_mutex);
#endif
        }
    private:
        Lock(const Lock&);
        Lock& operator=(const Lock&);
#if defined(_WIN32)
        CRITICAL_SECTION m_cs;      // 0x18 bytes
#else
        pthread_mutex_t  m_mutex;   // 0x2c bytes
#endif
    };

    class ScopedLock {                                                // (name inferred)
    public:
        explicit ScopedLock(Lock& lock) : m_lock(lock) { m_lock.Acquire(); }
        ~ScopedLock() { m_lock.Release(); }
    private:
        ScopedLock(const ScopedLock&);
        ScopedLock& operator=(const ScopedLock&);
        Lock& m_lock;
    };

    // Layout (Mac / Win):  vptr +0x00 / vbptr +0x00
    String     m_language;   // Mac +0x04, Win +0x04
    Path       m_path;       // Mac +0x14 (0x18), Win +0x08: the selected string file
    SectionMap m_sections;   // Mac +0x2c (0x1c), Win +0x14: file path -> its strings
    Lock       m_lock;       // Mac +0x48, Win +0x38
    // Mac: virtual base Object at +0x74.  Win: virtual base Object at +0x50.
};

// Mac vtable 0x75fc90 {D1 0x1aa30, D0 0x1bf60, CreateObject 0x1bf70};
// Win 0xa1f614 {sdd 0x41ff70, CreateObject 0x41fed0}.
class TextLocaleInstantiator : public ObjectRegistryInstantiator {
public:
    virtual ~TextLocaleInstantiator() {}                              // Mac 0x1aa30/0x1bf60, Win 0x41ff70
    virtual RefPointer<Object> CreateObject() const;                  // Mac 0x1bf70, Win 0x41fed0
};

// Mac sizeof 0x14 (vptr + String), vtable 0x75fc58 {D1 0x1bf00, D0 0x1bf20};
// Win sizeof 8, vtable 0x9c9350 {sdd 0x40c3a0}.
class LocalizedString : public String {
public:
    LocalizedString();                                                // Mac 0x1bd80, Win 0x420e10 ("????")
    explicit LocalizedString(const String& key);                      // Mac 0x1bea0, Win 0x420e70
    virtual ~LocalizedString();                                       // Mac 0x1bf00/0x1bf20, Win 0x40c3a0 (key function)

    // Replaces the text by the translation of `key` (see TextLocale::Localize). (name inferred)
    bool Load(const String& key);                                     // Mac 0x1bdb0, Win 0x420ee0
};

} // namespace LEGO
