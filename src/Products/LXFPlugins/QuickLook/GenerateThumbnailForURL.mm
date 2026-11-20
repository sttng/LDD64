// LEGO Digital Designer — restored source
// LXFQuickLook.qlgenerator: the thumbnail of an .lxf file = its IMAGE100.PNG (QLThumbnailRequest-
// SetImageWithData, type hint public.png); other files (.lxfml) get none. QuickLook
// 0x1cf0-0x239f. File name inferred (Xcode template).
// Mac only; the Objective-C parts sit behind __OBJC__ (tests/test_lxfplugins.cpp, HOST-ONLY
// stand-ins as in GeneratePreviewForURL.mm).
#if !defined(_WIN32)
#include "LXFPluginStorage.h"
#include "Platform.h"
#include "StringConversion.h"
#include "Path.h"
#include "File.h"

#if defined(__OBJC__)
#import <Foundation/Foundation.h>
#endif
#include <ApplicationServices/ApplicationServices.h>
#include <QuickLook/QuickLook.h>

using namespace LEGO;

extern "C" OSStatus GenerateThumbnailForURL(void* thisInterface, QLThumbnailRequestRef thumbnail, CFURLRef url,
                                            CFStringRef contentTypeUTI, CFDictionaryRef options, CGSize maxSize);
extern "C" void CancelThumbnailGeneration(void* thisInterface, QLThumbnailRequestRef thumbnail);

// QuickLook 0x1cf0
OSStatus GenerateThumbnailForURL(void* /*thisInterface*/, QLThumbnailRequestRef thumbnail, CFURLRef url,
                                 CFStringRef /*contentTypeUTI*/, CFDictionaryRef /*options*/, CGSize /*maxSize*/)
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
        if (!archive.IsNull())
            image.Assign(Path(String("IMAGE100.PNG"), archive));
        // (without an archive `image` has no directory: OpenFile fails)
        RefPointer<File> file = image.OpenFile(OpenMode(true, false, true, false), false);
        if (!file.IsNull()) {
            // NOTE: new[] buffer handed to NSData without copy (freed with free()), leaked when
            // the read fails, and the declared size is not checked (as the preview); fixed as
            // there (H26).
            const UInt64 size = file->GetSize();
            char* buffer = size != 0 && size <= kLXFGeneratorMaxSize
                               ? static_cast<char*>(malloc((size_t)size)) : 0;
            if (!buffer) {
                // nothing to show
            } else if (!file->Read(size, buffer, 0)) {
                free(buffer);
            } else {
#if defined(__OBJC__)
                NSData* data = [NSData dataWithBytesNoCopy:buffer length:(NSUInteger)file->GetSize()];
                if (data) {
                    NSDictionary* properties =
                        [NSDictionary dictionaryWithObject:@"public.png"
                                                    forKey:(NSString*)kCGImageSourceTypeIdentifierHint];
                    QLThumbnailRequestSetImageWithData(thumbnail, (CFDataRef)data, (CFDictionaryRef)properties);
                }
#else
                // HOST-ONLY: no properties dictionary
                CFDataRef data = LXFHostDataWithBytesNoCopy(buffer, file->GetSize());
                if (data)
                    QLThumbnailRequestSetImageWithData(thumbnail, data, 0);
#endif
            }
        }
    }
    ReleaseLXFStorage();
#if defined(__OBJC__)
    [pool release];
#endif
    return noErr;
}

// QuickLook 0x23a0
void CancelThumbnailGeneration(void* /*thisInterface*/, QLThumbnailRequestRef /*thumbnail*/)
{
}
#endif // !_WIN32
