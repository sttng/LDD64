// LEGO Digital Designer — restored source
// LDDCursor, Mac part — TU 137 (Mac 0x1f02f0 cursor table, 0x1f2cd0 / 0x1f2d40 NSCursor
// release). Objective-C behind __OBJC__; LDD64 Linux (LDD_LINUX) has the Windows build's cursors;
// the plain C++ branch is otherwise HOST-ONLY (the tests: every shape maps to the default cursor).
#if !defined(_WIN32)
#include "LDDCursor.h"

#if defined(__OBJC__)
#import <AppKit/AppKit.h>
#elif defined(LDD_LINUX)
#include "LinuxShell.h"
#endif

namespace LEGO {

#if defined(__OBJC__)
// The map node destructor (Mac 0x1f2cd0) and operator[]'s temporary (0x1f2d40).
LDDCursor::State::~State()
{
    if (owned)
        [(NSCursor*)cursor release];
}

namespace {

void SetSystemCursor(LDDCursor::StateMap& states, UInt32 shape, NSCursor* cursor)
{
    LDDCursor::State& state = states[shape];
    state.cursor = cursor;
    state.owned = false;
}

void SetImageCursor(LDDCursor::StateMap& states, UInt32 shape, NSString* image, CGFloat hotSpot)
{
    NSCursor* cursor = [[NSCursor alloc] initWithImage:[NSImage imageNamed:image]
                                               hotSpot:NSMakePoint(hotSpot, hotSpot)];
    LDDCursor::State& state = states[shape];
    state.cursor = cursor;
    state.owned = true;
}

} // namespace

// Mac 0x1f02f0 (always true)
bool LDDCursor::Initialize()
{
    SetSystemCursor(m_states, 1, [NSCursor arrowCursor]);
    SetSystemCursor(m_states, 2, [NSCursor pointingHandCursor]);
    ShareState(2, 3);
    SetSystemCursor(m_states, 4, [NSCursor openHandCursor]);
    SetSystemCursor(m_states, 5, [NSCursor closedHandCursor]);
    ShareState(4, 6);
    ShareState(4, 7);
    ShareState(1, 8);
    ShareState(1, 9);
    SetSystemCursor(m_states, 10, [NSCursor resizeUpDownCursor]);
    SetSystemCursor(m_states, 11, [NSCursor resizeUpDownCursor]);
    SetSystemCursor(m_states, 12, [NSCursor resizeLeftRightCursor]);
    SetSystemCursor(m_states, 13, [NSCursor resizeLeftRightCursor]);
    ShareState(1, 16);
    ShareState(1, 17);
    SetSystemCursor(m_states, 100, [NSCursor openHandCursor]);
    SetSystemCursor(m_states, 101, [NSCursor closedHandCursor]);
    SetImageCursor(m_states, 102, @"CursorMAC_MultipleSlct.png", 0);
    SetImageCursor(m_states, 103, @"CursorMAC_ConnectSlct.png", 0);
    SetImageCursor(m_states, 104, @"CursorMAC_ColorSlct.png", 0);
    SetImageCursor(m_states, 105, @"CursorMAC_ShapeSlct.png", 0);
    SetImageCursor(m_states, 106, @"CursorMAC_ColorShapeSlct.png", 0);
    SetImageCursor(m_states, 107, @"CursorMAC_Rotate.png", 0);
    SetImageCursor(m_states, 108, @"CursorMAC_Copy.png", 0);
    SetImageCursor(m_states, 109, @"CursorMAC_Paint.png", 0);
    SetImageCursor(m_states, 110, @"CursorMAC_Delete.png", 0);
    SetImageCursor(m_states, 14, @"CursorPC_SizeL.png", 12);
    SetImageCursor(m_states, 15, @"CursorPC_SizeR.png", 12);
    SetImageCursor(m_states, 111, @"CursorMAC_ColorPicker.png", 0);
    SetImageCursor(m_states, 112, @"CursorMAC_Deco.png", 0);
    SetImageCursor(m_states, 113, @"CursorMAC_H_Align1.png", 0);
    SetImageCursor(m_states, 114, @"CursorMAC_H_Align2.png", 0);
    SetImageCursor(m_states, 115, @"CursorMAC_Hide.png", 0);
    return true;
}

#elif defined(LDD_LINUX)
// LDD64 Linux (not restored code): the shapes of the Windows build (LDDCursor.cpp, Win 0x64a67c):
// the theme's cursors for the standard shapes, the Windows cursor resources (Cursors/cursor_<id>.cur
// next to the executable) for the LDD shapes, the arrow when a file is missing.
LDDCursor::State::~State()
{
    if (owned)
        LinuxShell::ReleaseCursor(cursor);
}

namespace {
void AddSystem(LDDCursor::StateMap& states, UInt32 shape, const char* name)
{
    LDDCursor::State& state = states[shape];
    state.cursor = LinuxShell::LoadSystemCursor(name);
    state.owned = state.cursor != 0;
}

void AddResource(LDDCursor::StateMap& states, UInt16 id, UInt32 shape)
{
    void* cursor = LinuxShell::LoadResourceCursor(id);
    if (!cursor) {
        AddSystem(states, shape, "default");
        return;
    }
    LDDCursor::State& state = states[shape];
    state.cursor = cursor;
    state.owned = true;
}
} // namespace

bool LDDCursor::Initialize()
{
    AddSystem(m_states, 1, "default");
    ShareState(1, 2);
    ShareState(1, 3);
    AddSystem(m_states, 4, "pointer");
    ShareState(4, 5);
    ShareState(4, 6);
    ShareState(4, 7);
    ShareState(1, 8);
    ShareState(1, 9);
    AddSystem(m_states, 10, "ns-resize");
    ShareState(10, 11);
    AddSystem(m_states, 12, "ew-resize");
    ShareState(12, 13);
    ShareState(1, 16);
    AddSystem(m_states, 17, "wait");
    static const UInt16 kResources[][2] = {
        {107, 102}, {106, 103}, {102, 104}, {108, 105}, {105, 106}, {110, 107}, {115, 108},
        {112, 109}, {109, 100}, {116, 101}, {127, 111}, {128, 112}, {131, 113}, {133, 114},
        {134, 115}};
    for (UInt32 i = 0; i < sizeof(kResources) / sizeof(kResources[0]); ++i)
        AddResource(m_states, kResources[i][0], kResources[i][1]);
    return true;
}

#else
// HOST-ONLY: no native cursors are owned.
LDDCursor::State::~State()
{
}

// HOST-ONLY stand-in for Mac 0x1f02f0: the same shape ids, all with the default cursor (0).
bool LDDCursor::Initialize()
{
    static const UInt32 kShapes[] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
        100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115
    };
    for (UInt32 i = 0; i < sizeof(kShapes) / sizeof(kShapes[0]); ++i) {
        State& state = m_states[kShapes[i]];
        state.cursor = 0;
        state.owned = false;
    }
    return true;
}
#endif

} // namespace LEGO
#endif // !_WIN32
