// LEGO Digital Designer — restored source
// LEGO::LDDCursor — the application's mouse cursor: a RenderCursor whose shapes are native
//   cursors (Win HCURSOR from the system / the exe's cursor resources, Mac NSCursor from the
//   system / the bundle's CursorMAC_*.png images), switched with SetShape; observers
//   (LDDCursorObserver: BrickReorientationControl …) hear about shape changes.
// LEGO::LDDCursorObserver — Observer<LDDCursor, LDDCursorObserver> with one (empty) handler.
//
// TU 137 "LDDCursor.cpp" (Mac 0x1ef4d0-0x1f318b; Win 0x649310-0x64aa65); the Mac
// cursor table and the NSCursor release are Objective-C (LDDCursorMac.mm).
//
// Shapes (ControlManager / Control +0x128 values; the ids ≥ 100 also have Lua names, see
// RegisterCursorNames): 1 arrow, 2 pointing hand (Win: arrow), 3 = 2, 4 open hand, 5 closed
// hand (Win: 4), 6 = 7 = 4, 8 = 9 = 1, 10/11 resize up-down, 12/13 resize left-right,
// 14/15 diagonal resize (Mac CursorPC_SizeL/R.png, Win IDC_SIZENESW/IDC_SIZENWSE), 16 = 1,
// 17 (Mac = 1, Win IDC_WAIT), 100 openHand … 115 hide.
//
// Vtables (Mac): primary 0x72609c (53 entries: RenderCursor's 48 with Initialize 0x1f02f0,
//   SetCursorVisible 0x1f1200, Attach 0x1f12c0, GetShape 0x1f2400, SetShape 0x1f13b0,
//   SetShapeEx 0x1f2410, CursorSlot8 0x1f2430), Cursor-in 0x7261c4, Notifier-in 0x726200,
//   Object-in 0x726224. Win: Cursor-in 0x9d4e80 {…, Attach 0x64a00b, Detach 0x648f8c,
//   GetShape 0x6611fb (ICF), SetShape 0x64a045, SetShapeEx 0x64a540, CursorSlot8 0x6c9dea},
//   Object-in 0x9d4e6c {sdd 0x64a54c, …}.
// Layout (Mac, constructor 0x1f01a0): RenderCursor +0x000..+0x1f0, Notifier<LDDCursorObserver>
//   +0x1f0 (vptr; observers +0x1f4, iterators +0x204), visible +0x214, shape +0x218,
//   BasicMap<UInt32, State> +0x21c, Object +0x238. Win (constructor 0x64a45a): visible +0x234,
//   shape +0x238, map +0x23c, Object +0x260.
// Method and member names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Notifier.h"
#include "BasicMap.h"
#include "BasicString.h"
#include "RenderCursor.h"
#include "Window.h"

namespace LEGO {

class LDDCursor;

// Mac vtable 0x72995c {D1 0x204030, D0 0x204060, OnFinalRelease 0x2040b0, OnAttached 0x204100,
// OnDetached 0x204110, OnCursorShapeChanged 0x204120 (empty)} (emitted in TU 142).
class LDDCursorObserver : public Observer<LDDCursor, LDDCursorObserver> {
public:
    virtual ~LDDCursorObserver() {}
    // The cursor shape changed to `shape`.
    virtual void OnCursorShapeChanged(UInt32 /*shape*/) {}                // Mac slot 5, Win slot 2
};

// The Lua name ↔ cursor shape table (LuaControl.cpp, TU 315, Mac 0x2c1fb0 / Win 0x44ec40:
// both directions of BasicMap<AString, UInt32, StringCaseInsensitiveLess> /
// BasicMap<UInt32, AString>). Declared here for RegisterCursorNames; defined in Lua/LuaControl.cpp.
// (name inferred)
void RegisterCursorShapeName(UInt32 shape, const AString& name);

class LDDCursor : public RenderCursor, public Notifier<LDDCursorObserver> {
public:
    typedef Window::NativeCursor NativeCursor;

    // One shape: the native cursor and whether it must be released (images / resources).
    // The destructor releases an owned cursor (Win 0x648fee DestroyCursor, Mac [cursor release]
    // in the map node destructor 0x1f2cd0 and in operator[] 0x1f2d40).
    struct State {
        State() : cursor(0), owned(false) {}
        ~State();
        NativeCursor cursor;   // +0x00
        bool         owned;    // +0x04
    };
    typedef BasicMap<UInt32, State> StateMap;

    explicit LDDCursor(ControlManager* manager);                           // Mac 0x1f01a0, Win 0x64a45a
    virtual ~LDDCursor();                                                   // Mac 0x1f18e0/0x1f1b70 (body 0x1f1750), Win 0x64a570/0x64aa65

    // Builds the shape table; false (Win) when a cursor could not be loaded.
    virtual bool Initialize();                                              // Mac slot 38 0x1f02f0, Win 0x64a67c

    // ---- Cursor ----
    // Remembers the flag; the native cursor shows when visible and the shape is not 0.
    virtual void SetCursorVisible(bool visible);                            // Mac 0x1f1200 (thunk 0x1f1260), Win 0x649310
    // Cursor::Attach, then shows the native cursor of the current shape.
    virtual bool Attach();                                                  // Mac 0x1f12c0 (thunk 0x1f1390), Win 0x64a00b
    virtual UInt32 GetShape() const;                                        // Mac 0x1f2400 (thunk 0x1f2580)
    // 0 hides the native cursor; unknown shapes fail. Observers are told the new shape.
    virtual bool SetShape(UInt32 shape);                                    // Mac 0x1f13b0 (thunk 0x1f1730), Win 0x64a045
    virtual bool SetShapeEx(UInt32 shape, UInt32 arg);                      // Mac 0x1f2410 (thunk 0x1f2590), Win 0x64a540
    virtual bool CursorSlot8();                                             // Mac 0x1f2430 (thunk 0x1f25c0), Win 0x6c9dea

    // Registers the Lua names of shapes 100-115 ("openHand" … "hide"). Called by the
    // application start-up (Mac 0x13ad50).
    static void RegisterCursorNames();                                      // Mac 0x1ef4d0, Win 0x6496b5

protected:
    // NotifierT::OnFinalRelease (observers leave), then RenderCursor's.
    virtual void OnFinalRelease();                                          // Mac 0x1f2350 (thunks 0x1f2440 …)

    // Window::SetCursor with the native cursor of `shape`; false for an unknown shape.
    bool ApplyShape(UInt32 shape);                                          // Win 0x649dab (inlined on Mac)
    // m_states[to] = m_states[from] (Win: the source then no longer owns the cursor).
    void ShareState(UInt32 from, UInt32 to);
    Window* GetWindow() const;

#if defined(_WIN32)
    // LoadCursor(NULL, id) into `shape` (not owned).
    bool AddSystemCursor(LPCSTR id, UInt32 shape);                          // Win 0x64a388
    // LoadCursor(exe, id) into `shape` (owned); the arrow if the resource is missing.
    bool AddResourceCursor(UInt16 id, UInt32 shape);                        // Win 0x64a5fb
#endif

    bool     m_visible;   // Mac +0x214, Win +0x234 (true)
    UInt32   m_shape;     // Mac +0x218, Win +0x238 (1, the arrow)
    StateMap m_states;    // Mac +0x21c, Win +0x23c
};

} // namespace LEGO
