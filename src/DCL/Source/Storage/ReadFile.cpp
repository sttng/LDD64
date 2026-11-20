// LEGO Digital Designer — restored source
// Whole-file read helpers (TU 181, Mac 0x2366c0-0x23698f). See ReadFile.h.
#include "ReadFile.h"
#include "Directory.h"
#include "File.h"
#include "Path.h"

namespace LEGO {

// Mac 0x2366c0, Win 0x538e70
bool ReadFile(const Path& path, AString& out)
{
    RefPointer<File> file = path.OpenFile(OpenMode(true, false, true), false);
    if (file.IsNull())
        return false;
    return ReadFile(file, out);
}

// Mac 0x236750, Win 0x539000
bool ReadFile(File* file, AString& out)
{
    AString buffer;
    // PORT64: the buffer is sized with the low 32 bits but the read takes all 64; a 64-bit build
    // refuses files of 4 GB and more instead of overflowing the buffer (the 32-bit code is the
    // original's: this check is compiled out there)
    if (sizeof(void*) > 4 && (file->GetSize() >> 32) != 0)
        return false;
    UInt32 size = (UInt32)file->GetSize();
    // NOTE: the size is trusted (both builds): 0xFFFFFFFF skips the reservation and SetLength
    // then writes the terminator far past the buffer, a size near 4 GB wraps; fixed: a file of
    // 1 GB or more is refused. (The archive storages check their entries' sizes too.)
    if (file->GetSize() >= 0x40000000u)
        return false;
    if (size != 0xFFFFFFFFu)
        buffer.Reserve(size + 1, true);
    buffer.SetLength((UInt32)file->GetSize());
    // NOTE: the buffer pointer is passed even when GetSize() failed (null buffer).
    if (!file->Read(file->GetSize(), buffer.GetWritableBuffer(), 0))
        return false;
    out = buffer;                     // Mac: BasicString<char>::Assign(buffer, 0, npos) (0x118760)
    return true;
}

// Mac 0x2368b0, Win 0x538f30
bool ReadFile(Directory* directory, const String& name, AString& out)
{
    RefPointer<File> file = directory->OpenFile(name, OpenMode(true, false, true), false, 0);
    if (file.IsNull())
        return false;
    return ReadFile(file, out);
}

} // namespace LEGO
