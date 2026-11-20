// LEGO Digital Designer — restored source
// LEGO::UndoAction, UndoManager, UndoManagerObserver — a document's undo / redo stacks
// (BrickConnectSceneControl owns one). TU 050 (file and method names inferred): Mac
// 0x114380-0x11615f, Win 0x53b910-0x53c3ff (UndoAction 0x53b910/0x53b990/0x53b9b0, Clear 0x53b9f0,
// AddAction 0x53bb40, RemoveAction 0x53be30, Undo 0x53c020, Redo 0x53c160, helpers
// 0x53c2a0-0x53c3c0). Ctor / dtor emitted in TU 060 (Win 0x5c9a9a / 0x5c9b3a, Mac D1 0x140e40 /
// D0 0x140ea0), defined in UndoManager.cpp.
// Execute() reverts a change and, while undoing, pushes its inverse (onto the redo stack; vice
// versa for redo). New actions in normal mode clear the redo stack.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "Notifier.h"

namespace LEGO {

class UndoManager;
class UndoManagerObserver;

// RTTI: vmi, virtual base Object. Vtables Mac 0x76073c {Execute (pure), D1 0x526d0, D0 0x526f0},
// Object-in 0x760760; Win 0xa2c35c-area {Execute (purecall)} + Object vftable (sdd 0x53b9b0).
// No data of its own (Mac: Object at +4; Win: vfptr, vbptr, Object at +8).
class UndoAction : public virtual Object {
public:
    UndoAction();                                                    // Win 0x53b910 (Mac inlined)
    // Reverts (undo) or re-applies (redo) the change; pushes the inverse action through
    // UndoManager::AddAction. (name inferred)
    virtual void Execute() = 0;                                      // slot 0
    virtual ~UndoAction();                                           // Mac 0x526d0/0x526f0  Win 0x53b990/0x53b9b0
};

// Observer<UndoManager, UndoManagerObserver>. Mac vtable 0x704c7c {D1 0x51d30, D0 0x51d60,
// OnFinalRelease 0x450d0, OnAttached 0x51db0, OnDetached 0x51dc0, OnUndoAvailable 0x51dd0,
// OnRedoAvailable 0x51de0}; Win own vftable 0x9cf3d0 {OnAttached, OnDetached, OnUndoAvailable,
// OnRedoAvailable} (all the empty 0x65a12a). (names inferred)
class UndoManagerObserver : public Observer<UndoManager, UndoManagerObserver> {
public:
    UndoManagerObserver() {}
    virtual ~UndoManagerObserver() {}                                // Mac 0x51d30/0x51d60
    virtual void OnUndoAvailable(bool /*available*/) {}              // Mac 0x51dd0  Win 0x65a12a (slot +8)
    virtual void OnRedoAvailable(bool /*available*/) {}              // Mac 0x51de0  Win 0x65a12a (slot +0xc)
};

// Notifier<UndoManagerObserver>. Vtables Mac 0x76620c {D1 0x140e40, D0 0x140ea0, OnFinalRelease
// 0x140a60}; Win Object-in vftable 0x9ca1f8 {sdd 0x5c9b32 -> 0x5c9f13}. Mac sizeof 0x5c (Object
// +0x54), Win 0x60 (vtordisp +0x54, Object +0x58).
class UndoManager : public Notifier<UndoManagerObserver> {
public:
    typedef BasicArray<RefPointer<UndoAction> > ActionStack;

    UndoManager();                                                   // Win 0x5c9a9a (Mac inlined, TU 060)
    virtual ~UndoManager();                                          // Mac 0x140e40/0x140ea0  Win 0x5c9b3a

    // Records `action` (normal: on the undo stack, clearing the redo stack; while undoing: on the
    // redo stack; while redoing: on the undo stack). Inside a group (m_group == action) the
    // stacks are redirected into the group instead.
    void AddAction(UndoAction* action);                              // Mac 0x1148e0  Win 0x53bb40
    // Forgets `action` on both stacks (first occurrence each).
    void RemoveAction(UndoAction* action);                           // Mac 0x1152c0  Win 0x53be30
    // Pops and executes the last undo (redo) action; no-op when the stack is empty.
    void Undo();                                                     // Mac 0x1159b0  Win 0x53c020
    void Redo();                                                     // Mac 0x115cf0  Win 0x53c160
    // Both stacks emptied, back to the manager's own stacks, observers told "nothing to undo /
    // redo".
    void Clear();                                                    // Mac 0x114380  Win 0x53b9f0

    bool CanUndo() const { return !m_undoStack->IsEmpty(); }        // (inlined accessors, names inferred)
    bool CanRedo() const { return !m_redoStack->IsEmpty(); }
    UInt32 GetUndoCount() const { return m_undoStack->GetSize(); }
    UInt32 GetRedoCount() const { return m_redoStack->GetSize(); }

    enum Mode { kNormal = 0, kUndoing = 1, kRedoing = 2 };            // (values from the code, names inferred)
    UInt32 GetMode() const { return m_mode; }

private:
    void NotifyUndoAvailable(bool available);                        // (inlined NotifierT pass, slot +0x14 / Win +8)
    void NotifyRedoAvailable(bool available);                        // (slot +0x18 / Win +0xc)

    ActionStack  m_undo;          // +0x24
    ActionStack  m_redo;          // +0x34
    ActionStack* m_undoStack;     // +0x44 current undo stack (m_undo, or a group's)
    ActionStack* m_redoStack;     // +0x48 current redo stack
    UInt32       m_mode;          // +0x4c Mode
    // NOTE: an action collecting sub-actions (UndoGroupAction): when it is added the current
    // stacks become its own. Only ever cleared (Clear, ctor) in the located code; setter and
    // class not identified. (name inferred)
    UndoAction*  m_group;         // +0x50
};

// The group action m_group points to (class not identified): an UndoAction with 8 bytes of own
// data before its undo / redo stacks (Mac +0x04, stacks +0x0c / +0x1c; Win +0x08, stacks +0x10 /
// +0x20). PORT64: a declared type instead of byte offsets. (name and leading members inferred)
class UndoGroupAction : public UndoAction {
public:
    UInt32                   m_unknown0;   // Mac +0x04  Win +0x08 (not identified)
    UInt32                   m_unknown1;   // Mac +0x08  Win +0x0c (not identified)
    UndoManager::ActionStack m_undo;       // Mac +0x0c  Win +0x10
    UndoManager::ActionStack m_redo;       // Mac +0x1c  Win +0x20
};

} // namespace LEGO
