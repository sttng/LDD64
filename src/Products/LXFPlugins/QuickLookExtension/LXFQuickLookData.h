// LEGO Digital Designer — NOT restored code: an addition for the 64-bit Mac project
// (LDD64.xcodeproj).
// The data of an .lxf / .lxfml preview for the Quick Look app extensions LXFPreview.appex (macOS
// 12+) and LXFThumbnail.appex (10.15+), which replace LXFQuickLook.qlgenerator (macOS 15 no longer
// loads generators). Same logic and storage code as the generator: an .lxf (a zip archive) shows
// its IMAGE100.PNG, any other file (an .lxfml) its text.
#pragma once
#include "LEGO/Types.h"

namespace LEGO {

enum LXFPreviewKind {
    kLXFPreviewNone = 0,   // nothing to show (missing / unreadable file, no IMAGE100.PNG, too big)
    kLXFPreviewPNG = 1,    // the .lxf's IMAGE100.PNG
    kLXFPreviewText = 2    // the file itself (an .lxfml), as text
};

// The preview data of the file at `nativePath` (UTF-8 POSIX path). `thumbnail`: only an .lxf's
// image (a bare .lxfml has no thumbnail). On success `data` is a new[] buffer of `size` bytes the
// caller deletes with delete[]. Requests are serialised (the storage set-up is global); files
// larger than kLXFPreviewMaxSize are refused.
const UInt64 kLXFPreviewMaxSize = 64 * 1024 * 1024;
LXFPreviewKind ReadLXFPreview(const char* nativePath, bool thumbnail, UInt8*& data, UInt64& size);

} // namespace LEGO
