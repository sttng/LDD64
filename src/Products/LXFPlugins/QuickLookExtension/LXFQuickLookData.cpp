// LEGO Digital Designer — NOT restored code: an addition for the 64-bit Mac project (see
// LXFQuickLookData.h). Mac only (the Windows branch compiles nothing here).
#if !defined(_WIN32)
#include "LXFQuickLookData.h"
#include "LXFPluginStorage.h"
#include "Platform.h"
#include "StringConversion.h"
#include "Path.h"
#include "File.h"

#include <pthread.h>
#include <string.h>

namespace LEGO {

namespace {
pthread_mutex_t s_lock = PTHREAD_MUTEX_INITIALIZER;

// The whole file into a new[] buffer (the generator's SetPreviewData without its leak, and with a
// size limit: the size is the archive's / file system's declaration).
bool ReadWhole(const Path& path, UInt8*& data, UInt64& size)
{
    RefPointer<File> file = path.OpenFile(OpenMode(true, false, true, false), false);
    if (file.IsNull())
        return false;
    UInt64 length = file->GetSize();
    if (length == 0 || length > kLXFPreviewMaxSize)
        return false;
    UInt8* buffer = new UInt8[(size_t)length];
    if (!file->Read(length, buffer, 0)) {
        delete[] buffer;
        return false;
    }
    data = buffer;
    size = length;
    return true;
}
} // namespace

LXFPreviewKind ReadLXFPreview(const char* nativePath, bool thumbnail, UInt8*& data, UInt64& size)
{
    data = 0;
    size = 0;
    if (nativePath == 0 || *nativePath == 0)
        return kLXFPreviewNone;
    LXFPreviewKind kind = kLXFPreviewNone;
    pthread_mutex_lock(&s_lock);
    if (RegisterLXFStorageFactories()) {
        Path path;
        String native = DecodeString(kTextEncodingUTF8, nativePath, (UInt32)strlen(nativePath));
        path.Append(Platform::GetInstance()->NativeToVFSPath(native));
        RefPointer<Directory> archive = path.GetDirectory(false);
        if (!archive.IsNull()) {
            if (ReadWhole(Path(String("IMAGE100.PNG"), archive), data, size))
                kind = kLXFPreviewPNG;
        } else if (!thumbnail) {
            if (ReadWhole(path, data, size))
                kind = kLXFPreviewText;
        }
    }
    ReleaseLXFStorage();
    pthread_mutex_unlock(&s_lock);
    return kind;
}

} // namespace LEGO
#endif // !_WIN32
