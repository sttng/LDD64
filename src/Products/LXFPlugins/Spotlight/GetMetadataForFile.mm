// LEGO Digital Designer — restored source
// LXF.mdimporter: Spotlight metadata of an .lxf / .lxfml file. Spotlight 0x14150-0x157ef.
// The model is read with the app's loaders (LXFML 5, then LXFML 4 when 5 reports a version or
// parser error) into a StorageScene2; from it:
//   kMDItemVersion       the LXFML version that read it ("5" / "4")
//   kMDItemCreator       <Meta><Application name=… versionMajor=… versionMinor=…/>: "name major.minor"
//   kMDItemKeywords      the names of the <Brand name=…/> metas
//   kMDItemAuthors       the names of the <Author name=…/> metas
//   kMDItemDescription   {lang (default "en"): text} of the <Description> metas
//   com_lego_lxf_numberofbricks  the number of <Brick>s
//   com_lego_lxf_listofbricks    their distinct design IDs, sorted (as strings)
// (schema.xml declares the two custom attributes.) File name inferred (Xcode template).
// Mac only; the Objective-C parts sit behind __OBJC__ (tests/test_lxfplugins.cpp runs the rest
// on the Linux-hosted Mac branch, with HOST-ONLY stand-ins for the pool and the brick list).
#if !defined(_WIN32)
#include "LXFPluginStorage.h"
#include "Platform.h"
#include "StringConversion.h"
#include "Path.h"
#include "Version.h"
#include "StorageScene2.h"
#include "ModelLoader.h"
#include "ModelIOLXFML4.h"
#include "ModelIOLXFML5.h"

#if defined(__OBJC__)
#import <Foundation/Foundation.h>
#endif
#include <CoreServices/CoreServices.h>

using namespace LEGO;

extern "C" Boolean GetMetadataForFile(void* thisInterface, CFMutableDictionaryRef attributes,
                                      CFStringRef contentTypeUTI, CFStringRef pathToFile);

namespace {

// Appends the value of every attribute `key` of `meta` to `values`, once each.
// NOTE: the CFStrings are not released (leaked, both arrays and the description dictionary):
// the containers are made without callbacks, so they neither retain their values nor compare
// them by content ("once each" compares pointers). Fixed: CFType callbacks, every value released
// after adding (low).
void AppendAttributeValues(const StorageScene2::Meta& meta, const char* key, CFMutableArrayRef values)
{
    for (const StorageScene2::Meta::Attribute* a = meta.m_attributes.Begin(); a != meta.m_attributes.End(); ++a) {
        if (a->m_name == String(key)) {
            CFStringRef value = CreateCFSubstring(a->m_value, 0, String::npos);
            if (!CFArrayContainsValue(values, CFRangeMake(0, CFArrayGetCount(values)), value))
                CFArrayAppendValue(values, value);
            CFRelease(value);
        }
    }
}

} // namespace

// Spotlight 0x14150
Boolean GetMetadataForFile(void* /*thisInterface*/, CFMutableDictionaryRef attributes,
                           CFStringRef /*contentTypeUTI*/, CFStringRef pathToFile)
{
#if defined(__OBJC__)
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
#endif
    bool result = RegisterLXFStorageFactories();
    RefPointer<StorageScene2> scene = new StorageScene2;
    Version version;
    if (result) {
        Path path;
        String vfsPath = Platform::GetInstance()->NativeToVFSPath(MakeString(pathToFile));
        path.Append(vfsPath);
        Path model;
        RefPointer<Directory> archive = path.GetDirectory(false);
        if (archive.IsNull())
            model.Assign(path);                                          // .lxfml
        else
            model.Assign(Path(String("IMAGE100.LXFML"), archive));      // .lxf (zip)
        RefPointer<ModelLoader> loader = new ModelLoaderLXFML5(0);
        if (loader->LoadFile(model, *scene, true)) {
            version = Version(5);
        } else if (loader->GetErrorMask() & (ModelLoader::Error::versionError | ModelLoader::Error::parserError)) {
            loader = new ModelLoaderLXFML4;
            if (loader->LoadFile(model, *scene, true))
                version = Version(4);
            else
                result = false;
        } else {
            result = false;
        }
    }
    if (result) {
        String versionText;
        ToString(version, versionText);
        CFStringRef value = CreateCFSubstring(versionText, 0, String::npos);
        CFDictionaryAddValue(attributes, kMDItemVersion, value);
        CFRelease(value);

        if (!scene->m_metas.IsEmpty()) {
            CFMutableArrayRef keywords = CFArrayCreateMutable(0, 10, &kCFTypeArrayCallBacks);
            CFMutableArrayRef authors = CFArrayCreateMutable(0, 10, &kCFTypeArrayCallBacks);
            CFMutableDictionaryRef descriptions = CFDictionaryCreateMutable(
                0, 10, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
            for (StorageScene2::Meta* meta = scene->m_metas.Begin(); meta != scene->m_metas.End(); ++meta) {
                if (meta->m_name == String("Application")) {
                    String name, major, minor;
                    for (StorageScene2::Meta::Attribute* a = meta->m_attributes.Begin();
                         a != meta->m_attributes.End(); ++a) {
                        if (a->m_name == String("name"))
                            name = a->m_value;
                        if (a->m_name == String("versionMajor"))
                            major = a->m_value;
                        if (a->m_name == String("versionMinor"))
                            minor = a->m_value;
                    }
                    if (!name.IsEmpty()) {
                        if (!major.IsEmpty()) {
                            (name << " ").Append(major);
                            if (!minor.IsEmpty())
                                (name << ".").Append(minor);
                        }
                        CFStringRef creator = CreateCFSubstring(name, 0, String::npos);
                        CFDictionaryAddValue(attributes, kMDItemCreator, creator);
                        CFRelease(creator);
                    }
                }
                if (meta->m_name == String("Brand"))
                    AppendAttributeValues(*meta, "name", keywords);
                if (meta->m_name == String("Author"))
                    AppendAttributeValues(*meta, "name", authors);
                if (meta->m_name == String("Description")) {
                    String language("en");
                    for (StorageScene2::Meta::Attribute* a = meta->m_attributes.Begin();
                         a != meta->m_attributes.End(); ++a)
                        if (a->m_name == String("lang"))
                            language = a->m_value;
                    // NOTE: key and text are not released (leaked); fixed (low, see above)
                    CFStringRef key = CreateCFSubstring(language, 0, String::npos);
                    CFStringRef text = CreateCFSubstring(meta->m_text, 0, String::npos);
                    CFDictionaryAddValue(descriptions, key, text);
                    CFRelease(key);
                    CFRelease(text);
                }
            }
            if (CFDictionaryGetCount(descriptions) != 0)
                CFDictionaryAddValue(attributes, kMDItemDescription, descriptions);
            CFRelease(descriptions);
            if (CFArrayGetCount(authors) != 0)
                CFDictionaryAddValue(attributes, kMDItemAuthors, authors);
            CFRelease(authors);
            if (CFArrayGetCount(keywords) != 0)
                CFDictionaryAddValue(attributes, kMDItemKeywords, keywords);
            CFRelease(keywords);
        }

#if defined(__OBJC__)
        NSMutableArray* bricks = [[NSMutableArray alloc] initWithCapacity:10];
        UInt32 count = 0;
        for (RefPointer<StorageScene2::Brick>* b = scene->m_bricks.Begin(); b != scene->m_bricks.End(); ++b) {
            String designID;
            designID << (*b)->m_designID;
            // NOTE: not released (leaked); fixed (low)
            NSString* text = (NSString*)CreateCFSubstring(designID, 0, String::npos);
            if (![bricks containsObject:text])
                [bricks addObject:text];
            [text release];
            ++count;
        }
        [(NSMutableDictionary*)attributes setObject:[NSNumber numberWithUnsignedInt:count]
                                             forKey:@"com_lego_lxf_numberofbricks"];
        [bricks sortUsingSelector:@selector(compare:)];
        [(NSMutableDictionary*)attributes setObject:bricks forKey:@"com_lego_lxf_listofbricks"];
        [bricks release];
#else
        // HOST-ONLY: the same list as a CFArray (tests/test_lxfplugins.cpp)
        CFMutableArrayRef bricks = CFArrayCreateMutable(0, 10, &kCFTypeArrayCallBacks);
        UInt32 count = 0;
        for (RefPointer<StorageScene2::Brick>* b = scene->m_bricks.Begin(); b != scene->m_bricks.End(); ++b) {
            String designID;
            designID << (*b)->m_designID;
            CFStringRef text = CreateCFSubstring(designID, 0, String::npos);
            if (!CFArrayContainsValue(bricks, CFRangeMake(0, CFArrayGetCount(bricks)), text))
                CFArrayAppendValue(bricks, text);
            CFRelease(text);
            ++count;
        }
        LXFHostSetBrickAttributes(attributes, count, bricks);
#endif
    }
    ReleaseLXFStorage();
#if defined(__OBJC__)
    [pool release];
#endif
    return result;
}
#endif // !_WIN32
