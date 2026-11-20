// LEGO Digital Designer — restored source
// The storage set-up shared by the two plug-ins of the Mac bundle (Contents/Library):
// LXFQuickLook.qlgenerator and LXF.mdimporter. Every entry point (GeneratePreviewForURL,
// GenerateThumbnailForURL, GetMetadataForFile) inlines it: the native Mac file system and the zip
// archives (an .lxf file is a zip with IMAGE100.PNG and IMAGE100.LXFML) as VFS directories; the
// ObjectRegistry is emptied again at the end of every request (ReleaseLXFStorage).
// QuickLook 0x12f8-0x13e3, Spotlight 0x1419b-0x1427e (both binaries: i386, Xcode 9.1, 10.13 SDK).
// File and function names inferred.
#pragma once
#include "LEGO/Types.h"
#include "ObjectFactory.h"
#include "ObjectRegistry.h"
#include "Directory.h"
#include "MacOSXDirectory.h"
#include "ZipStorageDirectory.h"

#include <typeinfo>

namespace LEGO {

// = RegisterNativeStorageFactory (LiffStorageFactory.cpp, Mac app) + the zip factory; no LIFF or
// memory storage.
inline bool RegisterLXFStorageFactories()
{
    RefPointer<SpecificObjectFactory> nativeFactory = new MacOSXStorageFactory;
    if (!ObjectFactory::GetInstance()->RegisterFactory(typeid(MacOSXDirectory), nativeFactory, typeid(Directory)))
        return false;
    RefPointer<SpecificObjectFactory> zipFactory = new ZipStorageFactory;
    return ObjectFactory::GetInstance()->RegisterFactory(typeid(ZipStorageDirectory), zipFactory, typeid(Directory));
}

// (not in the original) The largest file the generator plug-ins read whole.
const UInt64 kLXFGeneratorMaxSize = 64 * 1024 * 1024;

// Drops every registered singleton (factories, root directory, ...) after a request.
inline void ReleaseLXFStorage()
{
    ObjectRegistry::GetInstance()->UnregisterAll();
}

} // namespace LEGO

#if !defined(__OBJC__)
// HOST-ONLY (the restoration's tests): what the Objective-C parts of the plug-ins do through
// Cocoa, for the Linux-hosted Mac branch: NSData dataWithBytesNoCopy:length: (takes the new[]
// buffer), the sorted brick list and count of GetMetadataForFile.
#include <CoreServices/CoreServices.h>
extern "C" CFDataRef LXFHostDataWithBytesNoCopy(void* bytes, LEGO::UInt64 length);
extern "C" void LXFHostSetBrickAttributes(CFMutableDictionaryRef attributes, LEGO::UInt32 count,
                                          CFMutableArrayRef designIDs);
#endif
