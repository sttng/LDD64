// LEGO Digital Designer — NOT restored code: an addition for the 64-bit Mac project
// (LDD64.xcodeproj).
// LXFPreview.appex: the data-based Quick Look preview extension of .lxf / .lxfml files (macOS 12
// and later; macOS 15 no longer loads LXFQuickLook.qlgenerator). An .lxf shows its IMAGE100.PNG,
// an .lxfml its text, as the generator does (LXFQuickLookData.cpp).
// 64-bit Objective-C only; other compiles (C++ checks) see nothing here.
#if defined(__OBJC__) && defined(__LP64__)
#import <Cocoa/Cocoa.h>
#import <QuickLookUI/QuickLookUI.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include "LXFQuickLookData.h"

@interface LXFPreviewProvider : QLPreviewProvider <QLPreviewingController>
@end

@implementation LXFPreviewProvider

- (void)providePreviewForFileRequest:(QLFilePreviewRequest*)request
                   completionHandler:(void (^)(QLPreviewReply* reply, NSError* error))handler
{
    LEGO::UInt8* bytes = 0;
    LEGO::UInt64 size = 0;
    LEGO::LXFPreviewKind kind =
        LEGO::ReadLXFPreview([[[request fileURL] path] fileSystemRepresentation], false, bytes, size);
    if (kind == LEGO::kLXFPreviewNone) {
        handler(nil, [NSError errorWithDomain:NSCocoaErrorDomain code:NSFileReadCorruptFileError userInfo:nil]);
        return;
    }
    NSData* data = [NSData dataWithBytes:bytes length:(NSUInteger)size];
    delete[] bytes;
    QLPreviewReply* reply;
    if (kind == LEGO::kLXFPreviewPNG) {
        NSImage* image = [[[NSImage alloc] initWithData:data] autorelease];
        if (image == nil) {
            handler(nil, [NSError errorWithDomain:NSCocoaErrorDomain code:NSFileReadCorruptFileError userInfo:nil]);
            return;
        }
        NSSize imageSize = [image size];
        reply = [[QLPreviewReply alloc] initWithDataOfContentType:UTTypePNG
                                                      contentSize:CGSizeMake(imageSize.width, imageSize.height)
                                                dataCreationBlock:^NSData*(QLPreviewReply* r, NSError** e) {
                                                    (void)r; (void)e;
                                                    return data;
                                                }];
    } else {
        // (the generator's preview size, QLPreviewWidth / QLPreviewHeight of its Info.plist)
        reply = [[QLPreviewReply alloc] initWithDataOfContentType:UTTypePlainText
                                                      contentSize:CGSizeMake(800, 600)
                                                dataCreationBlock:^NSData*(QLPreviewReply* r, NSError** e) {
                                                    (void)r; (void)e;
                                                    return data;
                                                }];
        [reply setStringEncoding:NSUTF8StringEncoding];
    }
    handler([reply autorelease], nil);
}

@end
#endif
