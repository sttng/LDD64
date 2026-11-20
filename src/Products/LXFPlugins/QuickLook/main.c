// LEGO Digital Designer — restored source
// LXFQuickLook.qlgenerator: the CFPlug-in glue of Xcode's Quick Look generator template ("main.c"
// of the template; names as exported by the binary). QuickLook 0x1050-0x12ab.
//   factory  D995E624-C593-4C3E-BD01-4EFF0A8779B5 (Info.plist CFPlugInFactories)
//   type     5E2D9680-5022-40FA-B806-43349622E5B9 (kQLGeneratorTypeID)
// The callbacks are GeneratePreviewForURL.mm / GenerateThumbnailForURL.mm.
#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFPlugInCOM.h>
#include <CoreServices/CoreServices.h>
#include <QuickLook/QuickLook.h>

#define PLUGIN_ID "D995E624-C593-4C3E-BD01-4EFF0A8779B5"

OSStatus GenerateThumbnailForURL(void* thisInterface, QLThumbnailRequestRef thumbnail, CFURLRef url,
                                 CFStringRef contentTypeUTI, CFDictionaryRef options, CGSize maxSize);
void CancelThumbnailGeneration(void* thisInterface, QLThumbnailRequestRef thumbnail);
OSStatus GeneratePreviewForURL(void* thisInterface, QLPreviewRequestRef preview, CFURLRef url,
                               CFStringRef contentTypeUTI, CFDictionaryRef options);
void CancelPreviewGeneration(void* thisInterface, QLPreviewRequestRef preview);

// 12 bytes: the interface (a copy of the table below), the factory, the reference count.
typedef struct __QuickLookGeneratorPluginType {
    void*     conduitInterface;
    CFUUIDRef factoryID;
    UInt32    refCount;
} QuickLookGeneratorPluginType;

HRESULT QuickLookGeneratorQueryInterface(void* thisInstance, REFIID iid, LPVOID* ppv);
ULONG   QuickLookGeneratorPluginAddRef(void* thisInstance);
ULONG   QuickLookGeneratorPluginRelease(void* thisInstance);
void*   QuickLookGeneratorPluginFactory(CFAllocatorRef allocator, CFUUIDRef typeID);

// The callbacks are filled in by QueryInterface (QuickLook 0x1e670, 0x20 bytes).
static QLGeneratorInterfaceStruct myInterfaceFtbl = {
    NULL,
    QuickLookGeneratorQueryInterface,
    QuickLookGeneratorPluginAddRef,
    QuickLookGeneratorPluginRelease,
    NULL,
    NULL,
    NULL,
    NULL
};

// QuickLook 0x1050
HRESULT QuickLookGeneratorQueryInterface(void* thisInstance, REFIID iid, LPVOID* ppv)
{
    CFUUIDRef interfaceID = CFUUIDCreateFromUUIDBytes(kCFAllocatorDefault, iid);
    if (CFEqual(interfaceID, kQLGeneratorCallbacksInterfaceID)) {
        QLGeneratorInterfaceStruct* table =
            (QLGeneratorInterfaceStruct*)((QuickLookGeneratorPluginType*)thisInstance)->conduitInterface;
        table->GenerateThumbnailForURL = GenerateThumbnailForURL;
        table->CancelThumbnailGeneration = CancelThumbnailGeneration;
        table->GeneratePreviewForURL = GeneratePreviewForURL;
        table->CancelPreviewGeneration = CancelPreviewGeneration;
        table->AddRef(thisInstance);
        *ppv = thisInstance;
        CFRelease(interfaceID);
        return S_OK;
    }
    // NOTE: IUnknown is not answered (as the template)
    *ppv = NULL;
    CFRelease(interfaceID);
    return E_NOINTERFACE;
}

// QuickLook 0x1140
ULONG QuickLookGeneratorPluginAddRef(void* thisInstance)
{
    ((QuickLookGeneratorPluginType*)thisInstance)->refCount += 1;
    return ((QuickLookGeneratorPluginType*)thisInstance)->refCount;
}

// QuickLook 0x1150
ULONG QuickLookGeneratorPluginRelease(void* thisInstance)
{
    QuickLookGeneratorPluginType* instance = (QuickLookGeneratorPluginType*)thisInstance;
    instance->refCount -= 1;
    if (instance->refCount == 0) {
        CFUUIDRef factoryID = instance->factoryID;
        free(instance->conduitInterface);
        free(instance);
        if (factoryID) {
            CFPlugInRemoveInstanceForFactory(factoryID);
            CFRelease(factoryID);
        }
        return 0;
    }
    return instance->refCount;
}

// QuickLook 0x11a0
void* QuickLookGeneratorPluginFactory(CFAllocatorRef allocator, CFUUIDRef typeID)
{
    (void)allocator;
    if (!CFEqual(typeID, kQLGeneratorTypeID))
        return NULL;
    CFUUIDRef factoryID = CFUUIDCreateFromString(kCFAllocatorDefault, CFSTR(PLUGIN_ID));
    QuickLookGeneratorPluginType* instance =
        (QuickLookGeneratorPluginType*)malloc(sizeof(QuickLookGeneratorPluginType));
    instance->conduitInterface = malloc(sizeof(QLGeneratorInterfaceStruct));
    memcpy(instance->conduitInterface, &myInterfaceFtbl, sizeof(QLGeneratorInterfaceStruct));
    instance->factoryID = CFRetain(factoryID);
    CFPlugInAddInstanceForFactory(factoryID);
    instance->refCount = 1;
    CFRelease(factoryID);
    return instance;
}
