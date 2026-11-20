// LEGO Digital Designer — restored source
// LEGO::OpenGLWindow — Window with an OpenGL context (Win: WGL on the HDC; Mac: LEGOOpenGLView).
// TU 052: Mac 0x118cb0-0x11991f (LEGOOpenGLView 0x1191f0-0x119330), Win 0x51c5f5-0x51cd0f.
// Vtables: Mac 0x70b45c (61 slots: Window's 60 + CreateContext), Win 0x9c9394 (54 slots:
// Window's 53 + CreateContext); overrides OnFinalRelease (Mac 36), NotifyInput17 (Mac 50 / Win
// 45 — sent by Window::Create right after the native window exists, so the context is created
// there), ChangeDisplayMode (Mac 56 / Win 51).
// Layout: Mac Window (0xf4) + UInt32 level +0xf4, LEGOOpenGLView* +0xf8, Object +0xfc
//         (sizeof 0x104); Win Window (0xf0) + level +0xf0, HGLRC +0xf4, HDC +0xf8, Object +0x100.
// Created at start-up (Mac 0x13a1c0 / Win 0x5cb675) with the CompatibilityModeLevel preference,
// then Window::Create(style). Member names inferred. Host builds: HOST-ONLY GLX fallback (.mm).
#pragma once
#include "LEGO/Types.h"
#include "Window.h"
#include "OpenGLCrashReport.h"

#if defined(_WIN32)
#include <windows.h>
#endif

namespace LEGO {

class OpenGLWindow : public Window {
public:
    // `level`: compatibility mode level (0 = everything allowed; pixel-format choice on Win).
    OpenGLWindow(bool flag95, UInt32 level);                          // Mac 0x118cb0, Win 0x51cb74
    virtual ~OpenGLWindow();                                          // Mac 0x119360/0x1193c0, Win sdd 0x51c64b

    UInt32 GetLevel() const { return m_level; }                       // Mac 0x118d20, Win 0x51cc2a (name inferred)

protected:
    // Releases the GL view / context, then Window::OnFinalRelease.
    virtual void OnFinalRelease();                                    // Mac 36 0x118d30, Win 0x51cc31

public:
    // (Re)creates the context; notifies the observers only when that worked.
    virtual bool NotifyInput17();                                     // Mac 50 0x118f40, Win 45 0x51cc48
    // Window::ChangeDisplayMode; a switch to/from full screen or of the full-screen depth
    // destroys the native window (and with it the context) so that Create builds a new one.
    virtual bool ChangeDisplayMode(const WindowStyle& style, const WindowStyle& previous); // Mac 56 0x118fc0, Win 51 0x51cc6a
    // Creates the context for the native window and makes it current. (name inferred)
    virtual bool CreateContext();                                     // Mac 60 0x119020, Win 53 0x51cca3

#if defined(_WIN32)
    HGLRC GetGLContext() const { return m_context; }                  // (name inferred; inlined)
    HDC   GetGLDC() const { return m_dc; }                            // (name inferred; inlined)

    // ---- WGL helpers (static, /LTCG custom conventions noted) ----
    // Score of a pixel format for `colorBits` (0 = unusable): exact colour bits, RGBA, window +
    // OpenGL support, depth >= 16; + double buffering/swap method, ICD > MCD > generic (generic
    // software only with level != 0, nothing with level >= 100 except generic). (name inferred)
    static Real32 ScorePixelFormat(const PIXELFORMATDESCRIPTOR& pfd, UInt32 colorBits, UInt32 level); // Win 0x51c67b (EAX = pfd)
    // Best pixel format of `dc` (1-based, 0 = none) + its descriptor; traces the choice.
    static int ChooseBestPixelFormat(HDC dc, UInt32 colorBits, UInt32 level, PIXELFORMATDESCRIPTOR* pfd); // Win 0x51c807
    // GetDC, ChoosePixelFormat, SetPixelFormat, wglCreateContext, wglMakeCurrent.
    static bool CreateGLContext(HWND hwnd, HDC* dc, HGLRC* context, UInt32 level, UInt32 colorBits); // Win 0x51ca7d (EBX/ESI/EDI)
    // wglMakeCurrent(0, 0) (FormatMessage on failure), wglDeleteContext, ReleaseDC.
    static void DestroyGLContext(HWND hwnd, HGLRC context, HDC dc);   // Win 0x51caf4
#endif

protected:
    void DestroyContext();                                            // Win 0x51cccf (EAX = this), Mac inline

    UInt32 m_level;          // Mac +0xf4, Win +0xf0
#if defined(_WIN32)
    HGLRC  m_context;        // Win +0xf4
    HDC    m_dc;             // Win +0xf8
#else
    void*  m_view;           // Mac +0xf8 LEGOOpenGLView* (HOST-ONLY: GLX window + context)
#endif
};

} // namespace LEGO
