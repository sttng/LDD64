// LEGO Digital Designer — restored source
// LXFQuickLook.qlgenerator: the preview of an .lxf / .lxfml file. QuickLook 0x12b0-0x1c5f.
// An .lxf (a zip archive, opened as a VFS directory) shows its IMAGE100.PNG; any other file (an
// .lxfml) is shown as plain text. File name inferred (Xcode template).
// Mac only; the Objective-C parts sit behind __OBJC__ (tests/test_lxfplugins.cpp runs the rest
// on the Linux-hosted Mac branch, with HOST-ONLY stand-ins for NSData / the pool).
#if !defined(_WIN32)
#include "LXFPluginStorage.h"
#include "Platform.h"
#include "StringConversion.h"
#include "Path.h"
#include "File.h"

#if defined(__OBJC__)
#import <Foundation/Foundation.h>
#endif
#include <CoreServices/CoreServices.h>
#include <QuickLook/QuickLook.h>

using namespace LEGO;

extern "C" OSStatus GeneratePreviewForURL(void* thisInterface, QLPreviewRequestRef preview, CFURLRef url,
                                          CFStringRef contentTypeUTI, CFDictionaryRef options);
extern "C" void CancelPreviewGeneration(void* thisInterface, QLPreviewRequestRef preview);

namespace {

// The whole file as the preview, `type` its UTI. NOTE: the buffer comes from new[] and is
// handed to NSData without copy, which frees it with free() (both plug-ins); when the read fails
// it is leaked. The size is the archive's declaration, not checked (both plug-ins, inside
// Finder's preview process); fixed: an empty file or one over kLXFGeneratorMaxSize is not shown,
// the buffer comes from malloc and a failed read frees it (H26).
void SetPreviewData(QLPreviewRequestRef preview, const Path& path, CFStringRef type)
{
    RefPointer<File> file = path.OpenFile(OpenMode(true, false, true, false), false);
    if (file.IsNull())
        return;
    if (file->GetSize() == 0 || file->GetSize() > kLXFGeneratorMaxSize)
        return;
    char* buffer = static_cast<char*>(malloc((size_t)file->GetSize()));
    if (!buffer)
        return;
    if (file->Read(file->GetSize(), buffer, 0)) {
#if defined(__OBJC__)
        NSData* data = [NSData dataWithBytesNoCopy:buffer length:(NSUInteger)file->GetSize()];
#else
        CFDataRef data = LXFHostDataWithBytesNoCopy(buffer, file->GetSize());   // HOST-ONLY
#endif
        if (data)
            QLPreviewRequestSetDataRepresentation(preview, (CFDataRef)data, type, NULL);
    }
    else {
        free(buffer);
    }
}

} // namespace

// QuickLook 0x12b0
OSStatus GeneratePreviewForURL(void* /*thisInterface*/, QLPreviewRequestRef preview, CFURLRef url,
                               CFStringRef /*contentTypeUTI*/, CFDictionaryRef /*options*/)
{
#if defined(__OBJC__)
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
#endif
    if (RegisterLXFStorageFactories()) {
        Path path;
        CFStringRef nativePath = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
        String vfsPath = Platform::GetInstance()->NativeToVFSPath(MakeString(nativePath));
        CFRelease(nativePath);
        path.Append(vfsPath);
        Path image;
        RefPointer<Directory> archive = path.GetDirectory(false);
        if (archive.IsNull()) {
            SetPreviewData(preview, path, kUTTypePlainText);
        } else {
            image.Assign(Path(String("IMAGE100.PNG"), archive));
            SetPreviewData(preview, image, kUTTypeImage);
        }
    }
    ReleaseLXFStorage();
#if defined(__OBJC__)
    [pool release];
#endif
    return noErr;
}

// QuickLook 0x1c60
void CancelPreviewGeneration(void* /*thisInterface*/, QLPreviewRequestRef /*preview*/)
{
}
#endif // !_WIN32
