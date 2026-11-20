// LEGO Digital Designer — restored source
// LEGO::SimpleSound — one sound file played by the platform (skybox background sounds, button
// clicks, brick sounds; created by CreateLDDSound, LDDUtilities.h).
// TU 296 "SimpleSound.mm" (file name inferred): Mac 0x2a59e0-0x2a602f — NSSound with the objc
// delegate LEGOSimpleSoundDelegate (looping = replay when finished); Win 0x53abd0-0x53b4bf
// — a DirectShow filter graph (IGraphBuilder, IMediaControl, IMediaSeeking, IBasicAudio,
// IMediaEventEx) whose events arrive as message 0x8001 at the message-only window (and class)
// "SimpleSoundMessageOnlyWindow".
// HOST-ONLY: the Mac branch built as plain C++ (tests, Linux host app) is silent: Load only reads
// the file, Play/Stop succeed without output.

// RTTI: vmi, virtual base Object. Vtables: Mac 0x74402c {D1 0x2a5aa0, D0 0x2a5b90, OnFinalRelease
// 0x2a5ca0}, Object-in-SimpleSound 0x744050 {thunks 0x2a5b10 0x2a5c10 0x2a5ce0, AddRef, Release};
// Win 0xa2b140 {HandleMessage 0x53b2d0}, 0xa2b148 {sdd thunk 0x53b420 -> 0x53ac90, OnFinalRelease
// thunk 0x53b430 -> 0x53b010, AddRef, Release}.
// Sizes: Mac 0x18 (Object +0x10), Win 0x38 (Object +0x30). Method and member names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"

#if defined(_WIN32)
#include <windows.h>
struct IGraphBuilder;
struct IMediaControl;
struct IMediaSeeking;
struct IBasicAudio;
struct IMediaEventEx;
#endif

namespace LEGO {

class Path;

class SimpleSound : public virtual Object {
public:
    // Win: CoInitialize(0); state "no file".
    SimpleSound();                                                   // Mac 0x2a5a10  Win 0x53abd0
    // Mac: releases the delegate. Win: nothing (the COM objects go in OnFinalRelease).
    virtual ~SimpleSound();                                          // Mac 0x2a5aa0/0x2a5b90 (thunks 0x2a5b10/0x2a5c10)  Win 0x53ac90 (thunk 0x53b420)

    // Mac: nothing to do (true). Win: registers the window class and creates the message-only
    // window that receives the graph events (true when it exists). Call before Load.
    bool Initialize();                                               // Mac 0x2a5d30  Win 0x53ace0
    // Loads (replaces, Win) the sound; volume 25 of 255. Mac: the file is read into memory
    // (NSSound initWithData:); Win: the native path is rendered by DirectShow.
    bool Load(const Path& path);                                     // Mac 0x2a5d40  Win 0x53adf0
    // Starts or resumes playing; true when it plays.
    bool Play();                                                     // Mac 0x2a5f10  Win 0x53af60
    bool Stop();                                                     // Mac 0x2a5f80  Win 0x53af90
    // 0 (silent) .. 255 (full). Mac: NSSound volume v / 255; Win: IBasicAudio::put_Volume
    // (v * 1000 - 255000) / 255 (hundredths of a dB, -1000 .. 0).
    bool SetVolume(UInt8 volume);                                    // Mac 0x2a5ec0  Win 0x53afc0
    // Replay from the start when the end is reached.
    bool SetLooping(bool loop);                                      // Mac 0x2a5fb0  Win 0x53b000

#if defined(_WIN32)
    // Window procedure of the message-only window (0x53b1b0) forwards here: 0x8001 = graph
    // event (OnGraphEvent), everything else DefWindowProc.
    virtual LRESULT HandleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam); // Win 0x53b2d0
#endif

protected:
    // Mac: releases the NSSound. Win: releases the graph, destroys the window, CoUninitialize.
    virtual void OnFinalRelease();                                   // Mac 0x2a5ca0 (thunk 0x2a5ce0)  Win 0x53b010 (thunk 0x53b430)

private:
    SimpleSound(const SimpleSound&);
    SimpleSound& operator=(const SimpleSound&);

#if defined(_WIN32)
    enum State { kRunning = 0, kStopped = 2, kNoFile = 3 };          // (values from the code, names inferred)

    bool CreateGraph();                                              // Win 0x53b060 (EAX = this)
    void ReleaseGraph();                                             // Win 0x53b130 (ESI = this)
    void OnGraphEvent();                                             // Win 0x53b320 (EAX = this)
    static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam); // Win 0x53b1b0

    // Win layout: vfptr +0, vbptr +4
    bool           m_loop;          // +0x08
    UInt32         m_state;         // +0x0c State
    IGraphBuilder* m_graph;         // +0x10
    IMediaControl* m_control;       // +0x14
    IMediaSeeking* m_seeking;       // +0x18
    IBasicAudio*   m_audio;         // +0x1c
    IMediaEventEx* m_events;        // +0x20
    HWND           m_window;        // +0x24
    DWORD          m_seekCaps;      // +0x28 IMediaSeeking::GetCapabilities
#else
    bool  m_paused;                 // Mac +0x04 (only read: resume instead of play)
    void* m_sound;                  // Mac +0x08 NSSound*
    void* m_delegate;               // Mac +0x0c LEGOSimpleSoundDelegate*
#endif
};

} // namespace LEGO
