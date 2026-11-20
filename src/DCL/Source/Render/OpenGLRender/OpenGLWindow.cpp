// LEGO Digital Designer — restored source
// OpenGLWindow.cpp — Windows (WGL) implementation of LEGO::OpenGLWindow (TU 052,
// Win 0x51c5f5-0x51ce23). The Mac implementation is OpenGLWindow.mm.
#if defined(_WIN32)
#include "OpenGLWindow.h"
#include "LEGO/String.h"
#include "Trace.h"

namespace LEGO {

// Win 0x51cb74 (thiscall(flag95, level, mostDerived), ret 0xc); 0x51c5f5 is the vtable part
OpenGLWindow::OpenGLWindow(bool flag95, UInt32 level)
    : Window(flag95), m_level(level), m_context(0), m_dc(0)
{
}

// Win sdd 0x51c64b (-> 0x51c667 -> 0x51c5f5: vtables, Window dtor 0x40d250)
OpenGLWindow::~OpenGLWindow()
{
}

// Win 0x51cccf (EAX = this)
void OpenGLWindow::DestroyContext()
{
    DestroyGLContext(m_hwnd, m_context, m_dc);
}

// Win 0x51cc31 (thunk 0x51ccf8)
// NOTE: m_context / m_dc keep their (dangling) values, as in the original.
void OpenGLWindow::OnFinalRelease()
{
    DestroyContext();
    Window::OnFinalRelease();
}

// Win 0x51cc48
bool OpenGLWindow::NotifyInput17()
{
    DestroyContext();
    if (!CreateContext())
        return false;
    return Window::NotifyInput17();
}

// Win 0x51cc6a
bool OpenGLWindow::ChangeDisplayMode(const WindowStyle& style, const WindowStyle& previous)
{
    if (!Window::ChangeDisplayMode(style, previous))
        return false;
    if (style.flag28 != previous.flag28 || (style.flag28 && previous.value2c != style.value2c))
        DestroyNativeWindow();
    return true;
}

// Win 0x51cca3: the full-screen depth of the style is the colour depth asked for
bool OpenGLWindow::CreateContext()
{
    return CreateGLContext(m_hwnd, &m_dc, &m_context, m_level, m_style.value2c);
}

// Win 0x51c67b. The arithmetic follows the x87 code: float accumulator, double constants
// (0xae0ec0 = 10.0, 0xa2d5d0 = 255.0, 0xa2d640 = 3.0, 0xa2d5e0 = 0.5, 0xa1ea68 = 1.0,
// 0xa2d638 = 2.0, 0xa2d630 = 2560.0, swap bonus 6/4/2, ICD 0xa2d618 = 10240, MCD 0xa2d610 = 5120).
Real32 OpenGLWindow::ScorePixelFormat(const PIXELFORMATDESCRIPTOR& pfd, UInt32 colorBits, UInt32 level)
{
    if (colorBits != pfd.cColorBits || !(pfd.dwFlags & PFD_SUPPORT_OPENGL) ||
        !(pfd.dwFlags & PFD_DRAW_TO_WINDOW) || pfd.cDepthBits < 16 || pfd.iPixelType != PFD_TYPE_RGBA)
        return 0.0f;
    const double k = 10.0;
    Real32 score = 10.0f;
    if (pfd.cAuxBuffers)
        score = (Real32)((double)(Real32)((255.0 - pfd.cAuxBuffers) / 255.0 + k) / 3.0);
    if (pfd.cAccumBits)
        score = (Real32)((double)(Real32)(score + (255.0 - pfd.cAccumBits) / 255.0) * 0.5);
    score = (Real32)(pfd.cDepthBits * k + score);
    if (pfd.cStencilBits)
        score = (Real32)(score + 1.0);
    if (pfd.cAlphaBits)
        score = (Real32)(score + 2.0);
    DWORD flags = pfd.dwFlags;
    if (flags & PFD_DOUBLEBUFFER) {
        score = (Real32)(score + 2560.0);
        DWORD swap = flags & (PFD_SWAP_EXCHANGE | PFD_SWAP_COPY);
        if (swap == PFD_SWAP_EXCHANGE)
            score = (Real32)(score + 6.0);
        else if (swap == 0)
            score = (Real32)(score + 4.0);
        else if (swap == PFD_SWAP_COPY)
            score = (Real32)(score + 2.0);
    }
    if (!(flags & PFD_GENERIC_FORMAT)) {
        if (level < 100)
            return (Real32)(score + 10240.0);     // ICD
    } else if (!(flags & PFD_GENERIC_ACCELERATED)) {
        if (level != 0)
            return score;                           // generic software renderer
    } else if (level < 100) {
        return (Real32)(score + 5120.0);           // MCD
    }
    return 0.0f;
}

// Win 0x51c807 (cdecl)
// NOTE: formats are compared with `best < score` from best = -1, so without any usable format
// the first one (score 0) is still chosen; 0 is returned only for a device without formats.
int OpenGLWindow::ChooseBestPixelFormat(HDC dc, UInt32 colorBits, UInt32 level, PIXELFORMATDESCRIPTOR* chosenPfd)
{
    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = (BYTE)colorBits;
    pfd.cDepthBits = 16;
    int count = DescribePixelFormat(dc, 1, sizeof(PIXELFORMATDESCRIPTOR), NULL);
    Real32 best = -1.0f;
    int chosen = 0;
    for (int i = 1; i <= count; ++i) {
        DescribePixelFormat(dc, i, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
        Real32 score = ScorePixelFormat(pfd, colorBits, level);
        if (best < score) {
            chosen = i;
            best = score;
            *chosenPfd = pfd;
        }
    }
    if (!chosen) {
        Trace(String("No suitable OpenGL pixel format found."), true);
        return chosen;
    }
    const char* accelerated;
    if (chosenPfd->dwFlags & PFD_GENERIC_FORMAT)
        accelerated = (chosenPfd->dwFlags & PFD_GENERIC_ACCELERATED) ? "MCD" : "No";
    else
        // NOTE: the original prints "IDC" (its misspelling); fixed: "ICD"
        accelerated = "ICD";
    String message("Selected OpenGL pixel format:\r\n  Color bits: ");
    message << (UInt16)chosenPfd->cColorBits
            << "\r\n  Red bits: " << (UInt16)chosenPfd->cRedBits
            << "\r\n  Green bits: " << (UInt16)chosenPfd->cGreenBits
            << "\r\n  Blue bits: " << (UInt16)chosenPfd->cBlueBits
            << "\r\n  Alpha bits: " << (UInt16)chosenPfd->cAlphaBits
            << "\r\n  Depth buffer bits: " << (UInt16)chosenPfd->cDepthBits
            << "\r\n  Stencil buffer bits: " << (UInt16)chosenPfd->cStencilBits
            << "\r\n  Acuum buffer bits: " << (UInt16)chosenPfd->cAccumBits
            << "\r\n  Aux buffers: " << (UInt16)chosenPfd->cAuxBuffers
            << "\r\n  Accelerated: " << accelerated
            << "\r\n  Compatibility mode level: " << (UInt32)level << "\r\n";
    TraceVerbose(message, true);
    return chosen;
}

// Win 0x51ca7d (EBX = hwnd, ESI = HDC*, EDI = HGLRC*, cdecl(level, colorBits))
bool OpenGLWindow::CreateGLContext(HWND hwnd, HDC* dc, HGLRC* context, UInt32 level, UInt32 colorBits)
{
    *dc = ::GetDC(hwnd);
    if (!*dc)
        return false;
    PIXELFORMATDESCRIPTOR pfd;
    int format = ChooseBestPixelFormat(*dc, colorBits, level, &pfd);
    if (!format || !SetPixelFormat(*dc, format, &pfd)) {
        DestroyGLContext(hwnd, *context, *dc);
        return false;
    }
    *context = wglCreateContext(*dc);
    if (!*context) {
        DestroyGLContext(hwnd, 0, *dc);
        return false;
    }
    if (!wglMakeCurrent(*dc, *context)) {
        DestroyGLContext(hwnd, *context, *dc);
        return false;
    }
    return true;
}

// Win 0x51caf4 (cdecl)
void OpenGLWindow::DestroyGLContext(HWND hwnd, HGLRC context, HDC dc)
{
    if (context) {
        if (!wglMakeCurrent(NULL, NULL)) {
            // NOTE: the system message is formatted into a String and then discarded.
            LPWSTR text = 0;
            FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                           FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(),
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&text, 0, NULL);
            String message((const UniChar16*)text);
            LocalFree(text);
        }
        wglDeleteContext(context);
    }
    if (dc)
        ReleaseDC(hwnd, dc);
}

} // namespace LEGO
#endif // _WIN32
