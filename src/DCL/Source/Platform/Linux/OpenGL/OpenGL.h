// LDD64 Linux platform layer (not restored code): the CGL subset of <OpenGL/OpenGL.h> the Mac
// branch uses (OpenGLRender.cpp), on the GL views of LinuxShell.cpp. A CGLContextObj is a GL view
// (its GtkGLArea context and offscreen framebuffer); CGLFlushDrawable presents the frame drawn into
// the framebuffer. Enum values are Apple's.
#pragma once
#include "gl.h"
#include <stddef.h>

typedef struct _CGLContextObject*     CGLContextObj;
typedef struct _CGLPixelFormatObject* CGLPixelFormatObj;
typedef struct _CGLPBufferObject*     CGLPBufferObj;

typedef enum _CGLError {
    kCGLNoError            = 0,
    kCGLBadAttribute       = 10000,
    kCGLBadProperty        = 10001,
    kCGLBadPixelFormat     = 10002,
    kCGLBadRendererInfo    = 10003,
    kCGLBadContext         = 10004,
    kCGLBadDrawable        = 10005,
    kCGLBadDisplay         = 10006,
    kCGLBadState           = 10007,
    kCGLBadValue           = 10008,
    kCGLBadMatch           = 10009,
    kCGLBadEnumeration     = 10010,
    kCGLBadOffScreen       = 10011,
    kCGLBadFullScreen      = 10012,
    kCGLBadWindow          = 10013,
    kCGLBadAddress         = 10014,
    kCGLBadCodeModule      = 10015,
    kCGLBadAlloc           = 10016,
    kCGLBadConnection      = 10017
} CGLError;

typedef enum _CGLContextParameter {
    kCGLCPSwapRectangle          = 200,
    kCGLCPSwapInterval           = 222,
    kCGLCPDispatchTableSize      = 224,
    kCGLCPClientStorage          = 226,
    kCGLCPSurfaceTexture         = 228,
    kCGLCPSurfaceOrder           = 235,
    kCGLCPSurfaceOpacity         = 236,
    kCGLCPSurfaceBackingSize     = 304,
    kCGLCPSurfaceSurfaceVolatile = 306,
    kCGLCPReclaimResources       = 308,
    kCGLCPCurrentRendererID      = 309,
    kCGLCPGPUVertexProcessing    = 310,
    kCGLCPGPUFragmentProcessing  = 311,
    kCGLCPHasDrawable            = 314,
    kCGLCPMPSwapsInFlight        = 315
} CGLContextParameter;

// The current GL view (0: none).
CGLContextObj CGLGetCurrentContext();
// Makes the view's context current with its framebuffer as "the window" (0: none current).
CGLError CGLSetCurrentContext(CGLContextObj context);
// Presents the frame the view's framebuffer holds.
CGLError CGLFlushDrawable(CGLContextObj context);
// The view keeps its drawable until it is destroyed (LinuxShell::DestroyGLView).
inline CGLError CGLClearDrawable(CGLContextObj context) { return context ? kCGLNoError : kCGLBadContext; }
inline CGLContextObj CGLRetainContext(CGLContextObj context) { return context; }
inline void CGLReleaseContext(CGLContextObj) {}
// Renderer IDs of <OpenGL/CGLRenderers.h>: Apple's software renderers and the matching bits.
enum {
    kCGLRendererGenericFloatID = 0x00020400,
    kCGLRendererAppleSWID      = 0x00020600
};
#define kCGLRendererIDMatchingMask 0x00FE7F00

// The renderer (OpenGLRender ctor, Mac 0xf2900) is never Apple's software renderer: 0 (hardware
// or llvmpipe, both with shaders).
inline CGLError CGLGetParameter(CGLContextObj context, CGLContextParameter parameter, GLint* value)
{
    if (!context)
        return kCGLBadContext;
    if (!value)
        return kCGLBadAddress;
    switch (parameter) {
    case kCGLCPCurrentRendererID:
        *value = 0;
        return kCGLNoError;
    case kCGLCPGPUVertexProcessing:
    case kCGLCPGPUFragmentProcessing:
    case kCGLCPHasDrawable:
        *value = 1;
        return kCGLNoError;
    case kCGLCPSwapInterval:
        *value = 1;
        return kCGLNoError;
    default:
        return kCGLBadEnumeration;
    }
}
inline CGLError CGLSetParameter(CGLContextObj context, CGLContextParameter, const GLint*)
{
    return context ? kCGLNoError : kCGLBadContext;
}
