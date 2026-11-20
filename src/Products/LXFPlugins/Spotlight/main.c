// LEGO Digital Designer — restored source
// LXF.mdimporter: the CFPlug-in glue of Xcode's Spotlight importer template ("main.c" of the
// template; names as exported by the binary). Spotlight 0x17e0-0x19ef.
//   factory  D21F19F8-D5AC-44C3-8A21-57442858C951 (Info.plist CFPlugInFactories)
//   type     8B08C4BF-415B-11D8-B3F9-0003936726FC (kMDImporterTypeID)
// The importer is GetMetadataForFile.mm.
#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFPlugInCOM.h>
#include <CoreServices/CoreServices.h>

#define PLUGIN_ID "D21F19F8-D5AC-44C3-8A21-57442858C951"

Boolean GetMetadataForFile(void* thisInterface, CFMutableDictionaryRef attributes, CFStringRef contentTypeUTI,
                           CFStringRef pathToFile);

// 12 bytes: the (shared) interface table, the factory, the reference count.
typedef struct __MetadataImporterPluginType {
    MDImporterInterfaceStruct* conduitInterface;
    CFUUIDRef                  factoryID;
    UInt32                     refCount;
} MetadataImporterPluginType;

HRESULT MetadataImporterQueryInterface(void* thisInstance, REFIID iid, LPVOID* ppv);
ULONG   MetadataImporterPluginAddRef(void* thisInstance);
ULONG   MetadataImporterPluginRelease(void* thisInstance);
void*   MetadataImporterPluginFactory(CFAllocatorRef allocator, CFUUIDRef typeID);

// Spotlight 0x6db80
static MDImporterInterfaceStruct testInterfaceFtbl = {
    NULL,
    MetadataImporterQueryInterface,
    MetadataImporterPluginAddRef,
    MetadataImporterPluginRelease,
    GetMetadataForFile
};

// Spotlight 0x17e0
HRESULT MetadataImporterQueryInterface(void* thisInstance, REFIID iid, LPVOID* ppv)
{
    CFUUIDRef interfaceID = CFUUIDCreateFromUUIDBytes(kCFAllocatorDefault, iid);
    if (CFEqual(interfaceID, kMDImporterInterfaceID) || CFEqual(interfaceID, IUnknownUUID)) {
        ((MetadataImporterPluginType*)thisInstance)->conduitInterface->AddRef(thisInstance);
        *ppv = thisInstance;
        CFRelease(interfaceID);
        return S_OK;
    }
    *ppv = NULL;
    CFRelease(interfaceID);
    return E_NOINTERFACE;
}

// Spotlight 0x1920
ULONG MetadataImporterPluginAddRef(void* thisInstance)
{
    ((MetadataImporterPluginType*)thisInstance)->refCount += 1;
    return ((MetadataImporterPluginType*)thisInstance)->refCount;
}

// Spotlight 0x1930
ULONG MetadataImporterPluginRelease(void* thisInstance)
{
    MetadataImporterPluginType* instance = (MetadataImporterPluginType*)thisInstance;
    instance->refCount -= 1;
    if (instance->refCount == 0) {
        CFUUIDRef factoryID = instance->factoryID;
        free(instance);
        if (factoryID) {
            CFPlugInRemoveInstanceForFactory(factoryID);
            CFRelease(factoryID);
        }
        return 0;
    }
    return instance->refCount;
}

// Spotlight 0x1970
void* MetadataImporterPluginFactory(CFAllocatorRef allocator, CFUUIDRef typeID)
{
    (void)allocator;
    if (!CFEqual(typeID, kMDImporterTypeID))
        return NULL;
    CFUUIDRef factoryID = CFUUIDCreateFromString(kCFAllocatorDefault, CFSTR(PLUGIN_ID));
    MetadataImporterPluginType* instance =
        (MetadataImporterPluginType*)malloc(sizeof(MetadataImporterPluginType));
    instance->conduitInterface = &testInterfaceFtbl;
    instance->factoryID = CFRetain(factoryID);
    CFPlugInAddInstanceForFactory(factoryID);
    instance->refCount = 1;
    CFRelease(factoryID);
    return instance;
}
