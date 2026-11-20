// LEGO Digital Designer — restored source
// LEGO::DirectoryElement — base of every VFS node (File, Directory, storage subclasses);
// Attributes, Times. TU "Directory.cpp" (Mac 0x12530-0x18bbf, Win 0x424d30-0x42b8ff). The inline
// dtor (inlined in File dtor Mac 0x11f40) needs the complete Directory: defined in Directory.h,
// included at the end. Public accessors take the global storage lock (static recursive mutex Mac
// 0x77d070 / Win 0xb42e70) and call a "Do..." slot; backends call slots unlocked. Names inferred.
// Vtables: DirectoryElement Mac 0x703e30 / Win 0xa1f808, File Mac 0x75f918 / Win 0xa20114,
// Directory Mac 0x703e60 / Win 0xa1f78c. Slots (Win = Mac - 1, one sdd slot):
//   Mac  Win  member                         DirectoryElement  File      Directory
//    0    0   ~DirectoryElement (D1 / sdd)   0x17dd0           0x11f40   0x17ed0
//    1    -   ~DirectoryElement (D0)         0x17e40           0x11fc0   0x17ee0
//    2    1   OnFinalRelease                 Object            Object    0x16b80
//    3    2   AddRef                         Object            Object    0x16be0
//    4    3   Release                        Object            Object    0x16bf0 (locked)
//    5    4   DoSetAttributes(attr)          0x12050 false
//    6    5   DoGetAttributes()              pure              0x12060   0x17f20
//    7    6   DoSetTimes(times)              0x12070 false
//    8    7   DoGetTimes()                   0x12080 zero
//    9    8   DoSetInfo(attr, times)         0x12940
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"

namespace LEGO {

class Directory;

// Access/kind flags of an element (name inferred). One byte of bit-fields: Win builds it with
// read-modify-write masks (Directory::DoGetAttributes 0x424ed0: `b = b & 0xc3 | 3`), Mac
// returns it in AL. Bits 6-7 are unused. Mapping from MacOSX (0x329d30): bit1 = S_IRUSR,
// bits 2+3 = S_IWUSR, bit5 = S_IXUSR.
struct Attributes {
    Attributes(bool isDirectory = false, bool readable = false, bool writable = false,
               bool removable = false, bool hidden = false, bool executable = false) // Win 0x424bc0
        : m_isDirectory(isDirectory), m_readable(readable), m_writable(writable),
          m_removable(removable), m_hidden(hidden), m_executable(executable)
    {
    }

    bool m_isDirectory : 1;   // 0x01
    bool m_readable    : 1;   // 0x02
    bool m_writable    : 1;   // 0x04
    bool m_removable   : 1;   // 0x08 (set before a recursive delete, MacOSX 0x32a1c0)
    bool m_hidden      : 1;   // 0x10 (name inferred; never set by the restored backends)
    bool m_executable  : 1;   // 0x20
};

// Time stamps of an element in 100 ns ticks since 1601-01-01 (FILETIME epoch; MacOSX 0x329790
// converts NSDate with factor 1e7 against 1601-01-01). sizeof 0x18. (name inferred)
struct Times {
    Times() : m_created(0), m_modified(0), m_accessed(0) {}
    Times(UInt64 created, UInt64 modified, UInt64 accessed)
        : m_created(created), m_modified(modified), m_accessed(accessed)
    {
    }

    UInt64 m_created;    // +0x00
    UInt64 m_modified;   // +0x08
    UInt64 m_accessed;   // +0x10 (MacOSX fills it with the modification date)
};

// Mac sizeof 0x1c, Win 0x10.
class DirectoryElement : public Object {
public:
    DirectoryElement(const String& name, Directory* parent);          // Mac 0x128a0, Win 0x4257a0
    virtual ~DirectoryElement();                                      // inline, see Directory.h

    // --- virtual interface (unlocked) ---------------------------------------------------
    virtual bool       DoSetAttributes(const Attributes&) { return false; }   // Mac 0x12050, Win 0x424d30
    virtual Attributes DoGetAttributes() const = 0;                           // Mac +0x18, Win +0x14
    virtual bool       DoSetTimes(const Times&) { return false; }             // Mac 0x12070, Win 0x424d30
    virtual Times      DoGetTimes() const { return Times(); }                 // Mac 0x12080, Win 0x424d40
    // Sets times, then attributes (so read-only attributes do not block the time update).
    virtual bool       DoSetInfo(const Attributes& attributes, const Times& times); // Mac 0x12940, Win 0x425830

    // --- locked public interface ----------------------------------------------------------
    Attributes GetAttributes() const;                                  // Mac 0x126c0, Win 0x425470
    Times      GetTimes() const;                                       // Mac inlined only, Win 0x4254f0
    bool       SetInfo(const Attributes& attributes, const Times& times); // Mac inlined only, Win 0x425560
    // `full` = false: the element name; true: names of all ancestors below the root joined
    // with '/' (the root's own name is not part of it).
    String     GetPath(bool full) const;                               // Mac 0x12720, Win 0x4255e0
    RefPointer<Directory> GetParent() const;                           // Mac 0x12830, Win 0x425700

protected:
    String                m_name;     // Mac +0x08, Win +0x08
    RefPointer<Directory> m_parent;   // Mac +0x18, Win +0x0c

    friend class Directory;

private:
    DirectoryElement(const DirectoryElement&);
    DirectoryElement& operator=(const DirectoryElement&);
};

} // namespace LEGO

// Completes DirectoryElement (inline destructor needs the complete Directory).
#include "Directory.h"
