// LEGO Digital Designer — NOT restored code: an addition for the 64-bit Mac project
// (LDD64.xcodeproj).
// LXFThumbnail.appex: the Quick Look thumbnail extension of .lxf files (macOS 10.15 and later;
// macOS 15 no longer loads LXFQuickLook.qlgenerator): the .lxf's IMAGE100.PNG, fitted into the
// requested size; a bare .lxfml gets none (as with the generator, LXFQuickLookData.cpp).
// 64-bit Objective-C only; other compiles (C++ checks) see nothing here.
#if defined(__OBJC__) && defined(__LP64__)
#import <Cocoa/Cocoa.h>
#import <QuickLookThumbnailing/QuickLookThumbnailing.h>
#include "LXFQuickLookData.h"

@interface LXFThumbnailProvider : QLThumbnailProvider
@end

@implementation LXFThumbnailProvider

- (void)provideThumbnailForFileRequest:(QLFileThumbnailRequest*)request
                     completionHandler:(void (^)(QLThumbnailReply* reply, NSError* error))handler
{
    LEGO::UInt8* bytes = 0;
    LEGO::UInt64 size = 0;
    LEGO::LXFPreviewKind kind =
        LEGO::ReadLXFPreview([[[request fileURL] path] fileSystemRepresentation], true, bytes, size);
    NSImage* image = nil;
    if (kind == LEGO::kLXFPreviewPNG) {
        NSData* data = [NSData dataWithBytes:bytes length:(NSUInteger)size];
        image = [[[NSImage alloc] initWithData:data] autorelease];
    }
    delete[] bytes;
    NSSize imageSize = image ? [image size] : NSZeroSize;
    if (image == nil || imageSize.width <= 0 || imageSize.height <= 0) {
        handler(nil, [NSError errorWithDomain:NSCocoaErrorDomain code:NSFileReadCorruptFileError userInfo:nil]);
        return;
    }
    // the image fitted into the maximum size, aspect ratio kept
    CGSize maximum = [request maximumSize];
    CGFloat scale = maximum.width / imageSize.width;
    if (maximum.height / imageSize.height < scale)
        scale = maximum.height / imageSize.height;
    CGSize contextSize = CGSizeMake(imageSize.width * scale, imageSize.height * scale);
    QLThumbnailReply* reply = [QLThumbnailReply replyWithContextSize:contextSize
                                          currentContextDrawingBlock:^BOOL {
                                              [image drawInRect:NSMakeRect(0, 0, contextSize.width, contextSize.height)];
                                              return YES;
                                          }];
    handler(reply, nil);
}

@end
#endif
