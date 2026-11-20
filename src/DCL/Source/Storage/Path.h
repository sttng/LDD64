// LEGO Digital Designer — restored source
// LEGO::Path — a VFS element as (directory, name). Mac vtable 0x761268 {D1 0x2d40, D0 0x34f0},
// Win 0x9c96b4 {sdd 0x40ba80}; weak typeinfo Mac 0x761270. Header-only except the members in TU
// 013 "Directory.cpp" (Mac 0x17290-0x17dcf, Win 0x4294f0-0x429b2f); no Path.cpp (TUs 055, 057, 141
// only hold weak copies).
// Inline ctors: Path(name, directory) e.g. Mac 0x11b6c0; copy = default + Assign (Mac 0x388830).
// Mac sizeof 0x18, Win 0x0c.
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "BasicArray.h"
#include "Directory.h"

namespace LEGO {

class Path {
public:
    Path() {}
    Path(const String& name, Directory* directory) : m_name(name), m_directory(directory) {}
    Path(const Path& other) : m_name(other.m_name), m_directory(other.m_directory) {}
    virtual ~Path() {}                                                // Mac 0x2d40/0x34f0, Win 0x40ba80

    Path& operator=(const Path& other) { Assign(other); return *this; }

    // Same directory object and case-insensitively equal names. Inlined, e.g. std::find over
    // BasicArray<Path> Mac 0x2d8000 (TU 324): `a.dir == b.dir && a.name.EqualsNoCase(b.name)`.
    bool operator==(const Path& other) const
    {
        return m_directory.Get() == other.m_directory.Get() && m_name.EqualsNoCase(other.m_name);
    }
    bool operator!=(const Path& other) const { return !(*this == other); }
    // Orders by directory object address, then case-insensitively by name. Inlined as
    // Less<Path> in every BasicMap<Path, …> (e.g. OpenGLShaderFactory Mac 0x388330-0x388740:
    // `a.dir == b.dir ? a.name.LessNoCase(b.name) : a.dir < b.dir`).
    bool operator<(const Path& other) const
    {
        if (m_directory.Get() == other.m_directory.Get())
            return m_name.LessNoCase(other.m_name);
        return m_directory.Get() < other.m_directory.Get();
    }

    const String& GetName() const { return m_name; }                  // (accessor, not located)
    Directory*    GetParentDirectory() const { return m_directory; }  // (accessor, not located)
    // Mac: LiffStorageFactory::CreateObject (0x19160) assigns the name directly. (name inferred)
    void          SetName(const String& name) { m_name = name; }

    // Operations on the named element (all fail / return null without a directory).
    RefPointer<File>      OpenFile(const OpenMode& mode, bool create) const;   // Mac 0x17290, Win 0x4294f0
    RefPointer<Directory> GetDirectory(bool create) const;                     // Mac 0x172f0, Win 0x429590
    bool                  Delete(bool recursive) const;                        // Mac 0x17340, Win 0x429620
    bool                  FileExists() const;                                  // Mac 0x17370, Win 0x429640
    bool                  Exists() const;                                      // Mac 0x173a0, Win 0x429660

    // Moves the path by a relative '/' path (".." allowed); a path without a directory is
    // anchored at the root directory. Returns false when the target directory does not exist.
    bool Append(const String& relativePath);                                   // Mac 0x173d0, Win 0x429680
    bool Append(const BasicArray<String>& components);                         // Mac 0x174c0, Win 0x429710
    // Replaces the path by its parent directory. (name inferred)
    bool ToParent();                                                           // Mac 0x17b40, Win inlined
    // Makes the path name `element`. (name inferred)
    void SetElement(const DirectoryElement* element);                          // Mac 0x17c70, Win inlined
    // Returns nothing (Mac 0x17d80 does not return this). (name inferred)
    void Assign(const Path& other);                                            // Mac 0x17d80, Win inlined

    // `full` = false: the name only; true: the directory's full path, '/', the name (no '/'
    // when the directory path is empty). Weak copy Mac 0xd850 (emitted in TU 010), Win 0x40b890.
    // (name inferred, after DirectoryElement::GetPath)
    String GetPath(bool full) const
    {
        if (!full)
            return String(m_name);
        String path;
        if (m_directory) {
            path = m_directory->GetPath(true);
            if (!path.IsEmpty())
                path << "/";
        }
        path << m_name;
        return path;
    }

private:
    String                m_name;        // Mac +0x04, Win +0x04
    RefPointer<Directory> m_directory;   // Mac +0x14, Win +0x08
};

} // namespace LEGO
