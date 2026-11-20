// LEGO Digital Designer — restored source
// TU 296 SimpleSound (Mac "SimpleSound.mm" 0x2a59e0-0x2a602f, Objective-C++: NSSound + the objc
// class LEGOSimpleSoundDelegate) and its Windows twin (Win 0x53abd0-0x53b4bf:
// DirectShow filter graph + message-only window). See SimpleSound.h. The Mac Objective-C parts
// sit behind __OBJC__; LDD64 Linux (LDD_LINUX) plays through Platform/Linux/LinuxSound.h (HOST-ONLY
// plain C++ build otherwise: see SimpleSound.h).
#include "SimpleSound.h"
#include "Path.h"
#include "File.h"
#include "Directory.h"
#include "LEGO/String.h"

#if defined(_WIN32)
#include <windows.h>
#include <dshow.h>
#include "Platform.h"
#include "Win32Unicode.h"
#elif defined(__OBJC__)
#import <Cocoa/Cocoa.h>
#elif defined(LDD_LINUX)
#include "LinuxSound.h"
#endif

#if defined(__OBJC__) && !defined(_WIN32)
// Mac 0x2a59e0: the NSSound delegate set while looping — plays the sound again when it ended.
@interface LEGOSimpleSoundDelegate : NSObject <NSSoundDelegate>   // (protocol: 10.6+ SDKs)
- (void)sound:(NSSound*)sound didFinishPlaying:(BOOL)finished;
@end

@implementation LEGOSimpleSoundDelegate
- (void)sound:(NSSound*)sound didFinishPlaying:(BOOL)finished
{
    if (finished)
        [sound play];
}
@end
#endif

namespace LEGO {

#if defined(_WIN32)
namespace {

// The GUIDs as stored in the original (Win 0x9f4050-0x9f40af); not taken from strmiids.lib.
const GUID kCLSID_FilterGraph  = { 0xe436ebb3, 0x524f, 0x11ce, { 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } };
const GUID kIID_IGraphBuilder  = { 0x56a868a9, 0x0ad4, 0x11ce, { 0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } };
const GUID kIID_IMediaControl  = { 0x56a868b1, 0x0ad4, 0x11ce, { 0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } };
const GUID kIID_IMediaEventEx  = { 0x56a868c0, 0x0ad4, 0x11ce, { 0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } };
const GUID kIID_IBasicAudio    = { 0x56a868b3, 0x0ad4, 0x11ce, { 0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 } };
const GUID kIID_IMediaSeeking  = { 0x36b73880, 0xc2c8, 0x11cf, { 0x8b, 0x46, 0x00, 0x80, 0x5f, 0x6c, 0xef, 0x60 } };

const UINT kGraphEventMessage = 0x8001;                    // WM_APP + 1 (name inferred)

// Volume 0..255 -> IBasicAudio hundredths of a dB (x87: v * 1000.0 - 255000.0, / 255.0, _ftol).
inline long VolumeToHundredthsDB(UInt8 volume)
{
    return (long)(((double)volume * 1000.0 - 255000.0) / 255.0);
}

} // namespace
#endif

// Mac 0x2a5a10 (C1; virtual base constructed here), Win 0x53abd0 (thiscall + most-derived flag)
SimpleSound::SimpleSound()
{
#if defined(_WIN32)
    m_loop = false;
    m_state = kNoFile;
    CoInitialize(0);
    m_graph = 0;
    m_control = 0;
    m_audio = 0;
    m_events = 0;
    m_seeking = 0;
    m_seekCaps = 0;
    // NOTE: the original leaves the window handle (+0x24) uninitialised until Initialize();
    // OnFinalRelease would destroy a garbage handle without it. Zeroed here.
    m_window = 0;
#else
    m_paused = false;
    m_sound = 0;
#if defined(__OBJC__)
    m_delegate = [[LEGOSimpleSoundDelegate alloc] init];
#else
    m_delegate = 0;   // HOST-ONLY
#endif
#endif
}

// Mac 0x2a5aa0 (D1), 0x2a5b90 (D0), thunks 0x2a5b10 / 0x2a5c10; Win 0x53ac90 (thunk 0x53b420)
SimpleSound::~SimpleSound()
{
#if !defined(_WIN32) && defined(__OBJC__)
    [(LEGOSimpleSoundDelegate*)m_delegate release];
#endif
}

// Mac 0x2a5ca0 (thunk 0x2a5ce0), Win 0x53b010 (thunk 0x53b430)
void SimpleSound::OnFinalRelease()
{
#if defined(_WIN32)
    ReleaseGraph();
    if (m_window != 0) {
        PlatformSetWindowLong(m_window, 0, 0);
        DestroyWindow(m_window);
        m_window = 0;
    }
    CoUninitialize();
#else
#if defined(__OBJC__)
    if (m_sound != 0)
        [(NSSound*)m_sound release];
#elif defined(LDD_LINUX)
    if (m_sound != 0)
        LinuxSound::Destroy(m_sound);
    m_sound = 0;
#endif
    Object::OnFinalRelease();
#endif
}

// Mac 0x2a5d30, Win 0x53ace0
bool SimpleSound::Initialize()
{
#if defined(_WIN32)
    WNDCLASSEXW windowClass;
    windowClass.cbSize = sizeof(WNDCLASSEXW);
    windowClass.style = 0;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = sizeof(LONG_PTR);   // PORT64: window long 0 holds the SimpleSound* (4 on Win32)
    windowClass.hInstance = GetModuleHandleA(0);
    windowClass.hIcon = 0;
    windowClass.hCursor = 0;
    windowClass.hbrBackground = 0;
    windowClass.lpszMenuName = 0;
    windowClass.lpszClassName = L"SimpleSoundMessageOnlyWindow";
    windowClass.hIconSm = 0;
    PlatformRegisterClassEx(&windowClass);      // NOTE: result ignored (already registered is fine)
    String windowName("MessageWindow");
    String className("SimpleSoundMessageOnlyWindow");
    m_window = PlatformCreateWindowEx(0, className, windowName, 0, 0, 0, 0, 0, HWND_MESSAGE, 0,
                                      GetModuleHandleA(0), this);
    return m_window != 0;
#else
    return true;
#endif
}

// Mac 0x2a5d40, Win 0x53adf0
bool SimpleSound::Load(const Path& path)
{
#if defined(_WIN32)
    // NOTE: a failing CreateGraph only turns into S_FALSE (the original tests `hr < 0`), so
    // RenderFile is still called (on a null graph when CoCreateInstance failed).
    HRESULT hr = CreateGraph() ? S_OK : S_FALSE;
    if (hr >= 0) {
        WCHAR native[256];
        {
            String nativePath = Platform::GetInstance()->VFSToNativePath(path.GetPath(true));
            AString ansi = nativePath.ToAString();
            const char* text = ansi.GetBuffer();
            if (text == 0)
                text = "";
            // NOTE: through the ANSI code page, MAX_PATH (0x104) characters into a 256 buffer
            // (both as in the original).
            MultiByteToWideChar(CP_ACP, 0, text, -1, native, MAX_PATH);
        }
        hr = m_graph->RenderFile(native, 0);
        if (hr >= 0)
            hr = m_seeking->GetCapabilities(&m_seekCaps);
    }
    bool ok = hr == S_OK;
    m_audio->put_Volume(VolumeToHundredthsDB(25));     // -901
    if (ok)
        m_state = kStopped;
    return ok;
#else
    RefPointer<File> file = path.OpenFile(OpenMode(true, false, true, false), false);
    if (file.IsNull())
        return false;
    UInt64 size64 = file->GetSize();
    UInt32 size = size64 > 0xffffffffULL ? 0xffffffffU : (UInt32)size64;
    UInt8* data = new UInt8[size];
    file->ReadAt(0, file->GetSize(), data, 0);
#if defined(__OBJC__)
    // NOTE: a previously loaded NSSound is not released (as in the original).
    NSSound* sound = [[NSSound alloc] initWithData:[NSData dataWithBytes:data length:(NSUInteger)file->GetSize()]];
    m_sound = sound;
    bool ok = sound != nil;
    delete[] data;
    [sound setVolume:25.0f / 255.0f];
    return ok;
#elif defined(LDD_LINUX)
    // as the Mac's NSSound: the data in memory, volume 25 of 255 (a sound loaded before is
    // replaced; the Mac's stays unreleased)
    if (m_sound != 0)
        LinuxSound::Destroy(m_sound);
    m_sound = LinuxSound::Create(data, size);
    delete[] data;
    return m_sound != 0;
#else
    // HOST-ONLY: no audio output; the file was read like the original does.
    delete[] data;
    return true;
#endif
#endif
}

// Mac 0x2a5f10, Win 0x53af60
bool SimpleSound::Play()
{
#if defined(_WIN32)
    if (m_state != kRunning) {
        if (m_control->Run() < 0)
            return false;
        m_state = kRunning;
    }
    return true;
#elif defined(__OBJC__)
    NSSound* sound = (NSSound*)m_sound;
    if ([sound isPlaying]) {
        if (!m_paused)
            return true;
        m_paused = false;
        return [sound resume] != NO;
    }
    if (m_paused) {
        m_paused = false;
        return [sound resume] != NO;
    }
    return [sound play] != NO;
#elif defined(LDD_LINUX)
    m_paused = false;
    return LinuxSound::Play(m_sound);
#else
    m_paused = false;   // HOST-ONLY: silent
    return true;
#endif
}

// Mac 0x2a5f80, Win 0x53af90
bool SimpleSound::Stop()
{
#if defined(_WIN32)
    if (m_state == kStopped)
        return true;
    HRESULT hr = m_control->Stop();
    if (hr >= 0)
        m_state = kStopped;
    return hr == S_OK;
#elif defined(__OBJC__)
    return [(NSSound*)m_sound stop] != NO;
#elif defined(LDD_LINUX)
    return LinuxSound::Stop(m_sound);
#else
    return true;        // HOST-ONLY
#endif
}

// Mac 0x2a5ec0, Win 0x53afc0
bool SimpleSound::SetVolume(UInt8 volume)
{
#if defined(_WIN32)
    return m_audio->put_Volume(VolumeToHundredthsDB(volume)) == S_OK;
#else
#if defined(__OBJC__)
    [(NSSound*)m_sound setVolume:(float)volume / 255.0f];
#elif defined(LDD_LINUX)
    LinuxSound::SetVolume(m_sound, (float)volume / 255.0f);
#else
    (void)volume;       // HOST-ONLY
#endif
    return true;
#endif
}

// Mac 0x2a5fb0, Win 0x53b000
bool SimpleSound::SetLooping(bool loop)
{
#if defined(_WIN32)
    m_loop = loop;
#elif defined(__OBJC__)
    [(NSSound*)m_sound setDelegate:(loop ? (LEGOSimpleSoundDelegate*)m_delegate : nil)];
#elif defined(LDD_LINUX)
    LinuxSound::SetLooping(m_sound, loop);
#else
    (void)loop;         // HOST-ONLY
#endif
    return true;
}

#if defined(_WIN32)

// Win 0x53b060 (EAX = this)
bool SimpleSound::CreateGraph()
{
    ReleaseGraph();
    HRESULT hr = CoCreateInstance(kCLSID_FilterGraph, 0, CLSCTX_INPROC_SERVER, kIID_IGraphBuilder,
                                  (void**)&m_graph);
    if (hr >= 0 && (hr = m_graph->QueryInterface(kIID_IMediaControl, (void**)&m_control)) >= 0 &&
        (hr = m_graph->QueryInterface(kIID_IMediaEventEx, (void**)&m_events)) >= 0 &&
        (hr = m_graph->QueryInterface(kIID_IMediaSeeking, (void**)&m_seeking)) >= 0 &&
        (hr = m_graph->QueryInterface(kIID_IBasicAudio, (void**)&m_audio)) >= 0 &&
        (hr = m_events->SetNotifyWindow((OAHWND)m_window, kGraphEventMessage, 0)) >= 0) {
        m_events->SetNotifyFlags(0);           // NOTE: result not checked (hr stays the previous)
    }
    return hr == S_OK;
}

// Win 0x53b130 (ESI = this)
void SimpleSound::ReleaseGraph()
{
    if (m_events != 0)
        m_events->SetNotifyWindow(0, 0, 0);
    if (m_events != 0) {
        m_events->Release();
        m_events = 0;
    }
    if (m_control != 0) {
        m_control->Release();
        m_control = 0;
    }
    if (m_graph != 0) {
        m_graph->Release();
        m_graph = 0;
    }
    if (m_seeking != 0) {
        m_seeking->Release();
        m_seeking = 0;
    }
    if (m_audio != 0) {
        m_audio->Release();
        m_audio = 0;
    }
    m_seekCaps = 0;
}

// Win 0x53b320 (EAX = this): EC_COMPLETE -> rewind; looping: run again, else stop.
void SimpleSound::OnGraphEvent()
{
    long code;
    LONG_PTR param1, param2;
    if (m_events->GetEvent(&code, &param1, &param2, 0) < 0)
        return;
    for (;;) {
        m_events->FreeEventParams(code, param1, param2);
        if (code == EC_COMPLETE)
            break;
        if (m_events->GetEvent(&code, &param1, &param2, 0) < 0)
            return;
    }
    LONGLONG position = 0;
    if (!m_loop) {
        if (m_state != kStopped && m_control->Stop() >= 0)
            m_state = kStopped;
        m_seeking->SetPositions(&position, AM_SEEKING_AbsolutePositioning, 0, AM_SEEKING_NoPositioning);
    } else {
        m_seeking->SetPositions(&position, AM_SEEKING_AbsolutePositioning, 0, AM_SEEKING_NoPositioning);
        if (m_state != kRunning && m_control->Run() >= 0)
            m_state = kRunning;
    }
}

// Win 0x53b2d0 (vftable 0xa2b140 slot 0)
LRESULT SimpleSound::HandleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == kGraphEventMessage)
        OnGraphEvent();
    if (Platform::GetInstance()->IsUnicode())
        return DefWindowProcW(window, message, wParam, lParam);
    return DefWindowProcA(window, message, wParam, lParam);
}

// Win 0x53b1b0: window long 0 = the SimpleSound (from CREATESTRUCT::lpCreateParams)
LRESULT CALLBACK SimpleSound::WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_CREATE)
        PlatformSetWindowLong(window, 0, (LONG_PTR)((CREATESTRUCTW*)lParam)->lpCreateParams);   // PORT64: no LONG cut
    SimpleSound* sound = (SimpleSound*)PlatformGetWindowLong(window, 0);
    if (sound != 0) {
        LRESULT result = sound->HandleMessage(window, message, wParam, lParam);
        if (message == WM_DESTROY)
            PlatformSetWindowLong(window, 0, 0);
        return result;
    }
    if (Platform::GetInstance()->IsUnicode())
        return DefWindowProcW(window, message, wParam, lParam);
    return DefWindowProcA(window, message, wParam, lParam);
}

#endif

} // namespace LEGO
