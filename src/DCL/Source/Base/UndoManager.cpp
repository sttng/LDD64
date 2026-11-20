// LEGO Digital Designer — restored source
// TU 050 "UndoManager.cpp" (file name inferred): Mac 0x114380-0x11615f, Win 0x53b910-0x53c3ff,
// with the BasicArray<RefPointer<UndoAction>> instance (dtor Mac 0x116030 / 0x1160a0, Win
// 0x53c2a0 / 0x53c3c0; PushBack Mac 0x115180, Win 0x53c2d0; find + erase Mac 0x115860, Win
// 0x53c370). See UndoManager.h.
#include "UndoManager.h"

namespace LEGO {

namespace {

// Mac 0x115860 / Win 0x53c370: std::find + vector::erase (order kept).
void EraseFirst(UndoManager::ActionStack& stack, const UndoAction* action)
{
    for (UInt32 i = 0; i < stack.GetSize(); ++i) {
        if (stack[i].Get() == action) {
            stack.Erase(i);
            return;
        }
    }
}

// The stacks of the group action AddAction redirects to (see UndoManager::m_group and
// UndoGroupAction): index 0 = undo (Mac +0x0c, Win +0x10), 1 = redo (Mac +0x1c, Win +0x20).
// PORT64: through the declared type, not byte offsets (tests/test_services.cpp checks that the
// 32-bit layout is unchanged).
UndoManager::ActionStack* GroupStack(UndoAction* group, UInt32 index)
{
    UndoGroupAction* g = static_cast<UndoGroupAction*>(group);
    return index == 0 ? &g->m_undo : &g->m_redo;
}

} // namespace

// ===========================================================================================
// UndoAction
// ===========================================================================================

// Win 0x53b910 (thiscall + most-derived flag)
UndoAction::UndoAction()
{
}

// Mac 0x526d0 (D1), 0x526f0 (D0); Win 0x53b990, 0x53b9b0 (scalar deleting)
UndoAction::~UndoAction()
{
}

// ===========================================================================================
// UndoManager
// ===========================================================================================

// Win 0x5c9a9a
UndoManager::UndoManager()
    : m_undoStack(&m_undo),
      m_redoStack(&m_redo),
      m_mode(kNormal),
      m_group(0)
{
}

// Mac 0x140e40 (D1), 0x140ea0 (D0); Win 0x5c9b3a
UndoManager::~UndoManager()
{
}

void UndoManager::NotifyUndoAvailable(bool available)
{
    Iterator pass(*this);
    while (UndoManagerObserver* observer = pass.Next(m_observers))
        observer->OnUndoAvailable(available);
}

void UndoManager::NotifyRedoAvailable(bool available)
{
    Iterator pass(*this);
    while (UndoManagerObserver* observer = pass.Next(m_observers))
        observer->OnRedoAvailable(available);
}

// Mac 0x114380, Win 0x53b9f0
void UndoManager::Clear()
{
    m_undoStack = &m_undo;
    m_redoStack = &m_redo;
    m_redo.Clear();
    m_undo.Clear();
    m_group = 0;
    NotifyUndoAvailable(false);
    NotifyRedoAvailable(false);
}

// Mac 0x1148e0, Win 0x53bb40
void UndoManager::AddAction(UndoAction* action)
{
    switch (m_mode) {
    case kNormal:
        m_redoStack->Clear();
        m_undoStack->PushBack(RefPointer<UndoAction>(action));
        break;
    case kUndoing:
        m_redoStack->PushBack(RefPointer<UndoAction>(action));
        break;
    case kRedoing:
        m_undoStack->PushBack(RefPointer<UndoAction>(action));
        break;
    }
    if (m_group == action) {
        m_undoStack = GroupStack(m_group, 0);
        m_redoStack = GroupStack(m_group, 1);
        return;
    }
    if (m_undoStack->GetSize() == 1)
        NotifyUndoAvailable(true);
    if (m_redoStack->GetSize() == 1)
        NotifyRedoAvailable(true);
    if (m_redoStack->IsEmpty())
        NotifyRedoAvailable(false);
}

// Mac 0x1152c0, Win 0x53be30
// NOTE: the original holds `action` only during each erase: when the undo stack held the last
// reference, the first erase frees it and the redo-stack erase takes and drops a reference to
// the freed action (use after free, double free; the callers in the located code keep their
// own reference); fixed: one reference over both erases (M23).
void UndoManager::RemoveAction(UndoAction* action)
{
    {
        RefPointer<UndoAction> keep(action);
        EraseFirst(*m_undoStack, action);
        EraseFirst(*m_redoStack, action);
    }
    if (m_undoStack->IsEmpty())
        NotifyUndoAvailable(false);
    if (m_redoStack->IsEmpty())
        NotifyRedoAvailable(false);
}

// Mac 0x1159b0, Win 0x53c020
void UndoManager::Undo()
{
    if (m_undoStack->IsEmpty())
        return;
    m_mode = kUndoing;
    RefPointer<UndoAction> action = m_undoStack->Back();
    m_undoStack->PopBack();
    action->Execute();
    m_mode = kNormal;
    if (m_undoStack->IsEmpty())
        NotifyUndoAvailable(false);
}

// Mac 0x115cf0, Win 0x53c160
void UndoManager::Redo()
{
    if (m_redoStack->IsEmpty())
        return;
    m_mode = kRedoing;
    RefPointer<UndoAction> action = m_redoStack->Back();
    m_redoStack->PopBack();
    action->Execute();
    m_mode = kNormal;
    if (m_redoStack->IsEmpty())
        NotifyRedoAvailable(false);
}

} // namespace LEGO
