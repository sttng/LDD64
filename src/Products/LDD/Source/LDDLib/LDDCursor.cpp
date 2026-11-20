// LEGO Digital Designer — restored source
// LDDCursor — TU 137 "LDDCursor.cpp" (Mac 0x1ef4d0-0x1f318b), Win 0x649310-0x64aa65.
// The Mac cursor table (NSCursor) and the State destructor of the Mac build are in
// LDDCursorMac.mm; the Win32 versions are here. See LDDCursor.h.
#include "LDDCursor.h"
#include "ControlManager.h"

namespace LEGO {

// Mac 0x1f01a0, Win 0x64a45a
LDDCursor::LDDCursor(ControlManager* manager)
    : RenderCursor(manager),
      m_visible(true),
      m_shape(1),
      m_states()
{
}

// Mac 0x1f18e0 / 0x1f1b70 (body 0x1f1750: the map, the notifier, RenderCursor), Win 0x64a570
LDDCursor::~LDDCursor()
{
}

// Mac 0x1f2350
void LDDCursor::OnFinalRelease()
{
    Notifier<LDDCursorObserver>::OnFinalRelease();
    RenderCursor::OnFinalRelease();
}

Window* LDDCursor::GetWindow() const
{
    return GetCursorManager()->GetWindow();   // Mac +0x5c, Win +0x6c of the manager
}

void LDDCursor::ShareState(UInt32 from, UInt32 to)
{
    State& source = m_states[from];
    State& target = m_states[to];
    target.cursor = source.cursor;
    target.owned = source.owned;
#if defined(_WIN32)
    source.owned = false;   // Win 0x64a67c clears the source's flag; the Mac copy keeps it
#endif
}

// Win 0x649dab (inline in Mac 0x1f12c0 / 0x1f13b0)
bool LDDCursor::ApplyShape(UInt32 shape)
{
    StateMap::Iterator it = m_states.Find(shape);
    if (it == m_states.End())
        return false;
    GetWindow()->SetCursor(it->second.cursor);
    return true;
}

// Mac 0x1f1200, Win 0x649310
void LDDCursor::SetCursorVisible(bool visible)
{
    m_visible = visible;
    GetWindow()->ShowCursor(m_shape != 0 && visible);
}

// Mac 0x1f12c0, Win 0x64a00b
bool LDDCursor::Attach()
{
    if (!Cursor::Attach())
        return false;
    ApplyShape(m_shape);
    GetWindow()->ShowCursor(m_visible);
    return true;
}

// Mac 0x1f2400, Win 0x6611fb (ICF)
UInt32 LDDCursor::GetShape() const
{
    return m_shape;
}

// Mac 0x1f13b0, Win 0x64a045
bool LDDCursor::SetShape(UInt32 shape)
{
    if (m_shape == shape)
        return true;
    if (shape == 0) {
        GetWindow()->ShowCursor(false);
    } else {
        if (!ApplyShape(shape))
            return false;
        GetWindow()->ShowCursor(m_visible);
    }
    m_shape = shape;
    typedef NotifierT<LDDCursor, LDDCursorObserver> CursorNotifier;
    CursorNotifier::Iterator it(*this);
    while (LDDCursorObserver* observer = it.Next(CursorNotifier::m_observers))
        observer->OnCursorShapeChanged(m_shape);
    return true;
}

// Mac 0x1f2410, Win 0x64a540
bool LDDCursor::SetShapeEx(UInt32 shape, UInt32 /*arg*/)
{
    return SetShape(shape);
}

// Mac 0x1f2430, Win 0x6c9dea (ICF)
bool LDDCursor::CursorSlot8()
{
    return false;
}

// Mac 0x1ef4d0, Win 0x6496b5
void LDDCursor::RegisterCursorNames()
{
    RegisterCursorShapeName(100, AString("openHand"));
    RegisterCursorShapeName(101, AString("closedHand"));
    RegisterCursorShapeName(102, AString("selectMultiple"));
    RegisterCursorShapeName(103, AString("selectConnected"));
    RegisterCursorShapeName(104, AString("selectSameColor"));
    RegisterCursorShapeName(105, AString("selectSameShape"));
    RegisterCursorShapeName(106, AString("selectSameColorAndShape"));
    RegisterCursorShapeName(107, AString("hinge"));
    RegisterCursorShapeName(108, AString("clone"));
    RegisterCursorShapeName(109, AString("paint"));
    RegisterCursorShapeName(110, AString("deletePointer"));
    RegisterCursorShapeName(111, AString("colorPicker"));
    RegisterCursorShapeName(112, AString("decoration"));
    RegisterCursorShapeName(113, AString("hingeAlign"));
    RegisterCursorShapeName(114, AString("hingeAlign2"));
    RegisterCursorShapeName(115, AString("hide"));
}

#if defined(_WIN32)
// Win 0x648fee
LDDCursor::State::~State()
{
    if (owned)
        ::DestroyCursor(cursor);
}

// Win 0x64a388
bool LDDCursor::AddSystemCursor(LPCSTR id, UInt32 shape)
{
    HCURSOR cursor = ::LoadCursorA(0, id);
    if (cursor) {
        State& state = m_states[shape];
        state.cursor = cursor;
        state.owned = false;
    }
    return cursor != 0;
}

// Win 0x64a5fb
bool LDDCursor::AddResourceCursor(UInt16 id, UInt32 shape)
{
    HCURSOR cursor = ::LoadCursorA(::GetModuleHandleA(0), MAKEINTRESOURCEA(id));
    if (!cursor)
        return AddSystemCursor(IDC_ARROW, shape);
    State& state = m_states[shape];
    state.cursor = cursor;
    state.owned = true;
    return true;
}

// Win 0x64a67c: system cursors for the standard shapes, the exe's cursor resources (102-134,
// LDD.rc) for the LDD shapes. Stops at the first cursor that cannot be loaded (the copies
// between the loads are made regardless).
bool LDDCursor::Initialize()
{
    bool ok = AddSystemCursor(IDC_ARROW, 1);
    ShareState(1, 2);
    ShareState(1, 3);
    ok = ok && AddSystemCursor(IDC_HAND, 4);
    ShareState(4, 5);
    ShareState(4, 6);
    ShareState(4, 7);
    ShareState(1, 8);
    ShareState(1, 9);
    ok = ok && AddSystemCursor(IDC_SIZENS, 10);
    ShareState(10, 11);
    ok = ok && AddSystemCursor(IDC_SIZEWE, 12);
    ShareState(12, 13);
    ShareState(1, 16);
    return ok
        && AddSystemCursor(IDC_WAIT, 17)          // 0x7f02
        && AddResourceCursor(107, 102)
        && AddResourceCursor(106, 103)
        && AddResourceCursor(102, 104)
        && AddResourceCursor(108, 105)
        && AddResourceCursor(105, 106)
        && AddResourceCursor(110, 107)
        && AddResourceCursor(115, 108)
        && AddResourceCursor(112, 109)
        && AddResourceCursor(109, 100)
        && AddResourceCursor(116, 101)
        && AddResourceCursor(127, 111)
        && AddResourceCursor(128, 112)
        && AddResourceCursor(131, 113)
        && AddResourceCursor(133, 114)
        && AddResourceCursor(134, 115)
        && AddResourceCursor(104, 110)
        && AddSystemCursor(IDC_SIZENESW, 14)      // 0x7f83
        && AddSystemCursor(IDC_SIZENWSE, 15);     // 0x7f82
}
#endif

} // namespace LEGO
