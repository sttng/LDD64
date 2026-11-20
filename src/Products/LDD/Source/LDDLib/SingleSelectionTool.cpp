// LEGO Digital Designer — restored source
// LEGO::SingleSelectionTool — see SingleSelectionTool.h.
// TU 110 "SingleSelectionTool.cpp": Mac 0x1b9c70-0x1ba5eb, Win 0x684360-0x68468d. Not code: the
// thunks, Mac 0x1ba570 = <iostream> + a header static.
#include "SingleSelectionTool.h"
#include "BrickConnectSceneControl.h"
#include "ControlManager.h"
#include "PrimitiveElement.h"
#include "Part.h"

namespace LEGO {

// Mac 0x1b9c70, Win 0x68457f
SingleSelectionTool::SingleSelectionTool(ClickStickTool* clickStickTool)
    : SelectionTool(clickStickTool, true, true), m_shift(false)
{
    SetCursor(1);
}

// Mac 0x1ba2c0 / 0x1ba320, Win 0x68461a / 0x68468d
SingleSelectionTool::~SingleSelectionTool()
{
}

// Mac 0x1b9d30, Win 0x684360
void SingleSelectionTool::OnToolEvent24(const BasicArray<RefPointer<Part> >& parts)
{
    if (m_shift) {
        SelectionTool::OnToolEvent24(parts);
        return;
    }
    GetSceneControl()->SetSelection(parts);
}

// Mac 0x1b9d70, Win 0x684381
void SingleSelectionTool::OnPickPressed(bool clear)
{
    if (m_shift)
        return;
    SelectionTool::OnPickPressed(clear);
}

// Mac 0x1b9d90, Win 0x684458
void SingleSelectionTool::OnPickClicked(bool flag)
{
    if (m_shift) {
        SelectionTool::OnPickClicked(flag);
        return;
    }
    if (flag)
        return;
    GetSceneControl()->ClearSelection();
    const BasicArray<RefPointer<Part> >& selection = GetSceneControl()->GetSelection();
    if (!m_filters.IsEmpty()) {
        // NOTE: the selection was just cleared, so the part is never found (both builds).
        const Part* part = m_picked->GetPart();
        bool found = false;
        for (UInt32 i = 0; i < selection.GetSize() && !found; ++i)
            found = selection[i].Get() == part;
        if (!found) {
            BasicArray<RefPointer<Part> > parts = selection;
            for (UInt32 i = 0; i < m_filters.GetSize(); ++i)
                m_filters[i]->Apply(m_picked, parts);
            GetSceneControl()->SetSelection(parts);
            return;
        }
    }
    GetSceneControl()->ToggleSelection(m_picked->GetPart());
}

// Mac 0x1ba0b0, Win 0x5c8021 (ICF)
void SingleSelectionTool::OnPickReleased()
{
}

// Mac 0x1ba180 (thunk 0x1ba1b0), Win 0x6843c8
void SingleSelectionTool::OnKeyboardFocusGained(Control* control, bool flag, Control* previous,
                                                UInt32 modifiers)
{
    m_shift = (modifiers & 1) != 0;
    if (m_shift)
        ApplyCursor(0x66);
    BrickConnectSceneControlTool::OnKeyboardFocusGained(control, flag, previous, modifiers);
}

// Mac 0x1ba200 (thunk 0x1ba230), Win 0x6843fd
void SingleSelectionTool::OnKeyboardFocusLost(Control* control, Control* next)
{
    m_shift = false;
    ApplyCursor(1);
    BrickConnectSceneControlTool::OnKeyboardFocusLost(control, next);
}

// Mac 0x1ba0c0 (thunk 0x1ba0f0), Win 0x684392
void SingleSelectionTool::OnKeyPressed(Control*, UInt32, UInt32 modifiers, bool)
{
    m_shift = (modifiers & 1) != 0;
    if (m_shift)
        ApplyCursor(0x66);
}

// Mac 0x1ba120 (thunk 0x1ba150), Win 0x6843ad
void SingleSelectionTool::OnKeyReleased(Control*, UInt32, UInt32 modifiers)
{
    m_shift = (modifiers & 1) != 0;
    if (!m_shift)
        ApplyCursor(1);
}

// Mac 0x1ba270, Win 0x684424
void SingleSelectionTool::OnAttached(BrickConnectSceneControl* control)
{
    SelectionTool::OnAttached(control);
    m_shift = (control->GetManager()->GetModifiers() & 1) != 0;
    if (m_shift)
        control->SetToolCursor(0x66, m_cursorFlag);
}

} // namespace LEGO
