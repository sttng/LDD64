// LEGO Digital Designer — restored source
// ControlManager, the Mac-only NativeControlHitTester / ControlManagerNativeControlKeyboardFocus-
// Observer, and the Control members of TU 027's ControlManager part (sub-TUs 027a Mac
// 0xd7130-0xde100, 027c Mac 0xe76b0-0xeb1dc): layer-root ctor, Invalidate, InvalidateBuffered,
// FindControlAt, Add/RemoveAugmentation. 027c's other Control helpers (Mac 0xe7880 DrawChildren,
// 0xe7ac0 / 0xe7ff0 anchor dependency lists, 0xe81f0 / 0xe8600 / 0xe86f0 anchor layout, 0xe9510
// ManagerToLocal) are in Control.cpp with their callers; 027c's template instances come from
// the headers.
#include "ControlManager.h"
#include "Control.h"
#include "Cursor.h"

namespace LEGO {

typedef NotifierT<ControlManager, ControlManagerObserver>::Iterator ControlManagerObserverIterator;

// Calls `call` on every ControlManagerObserver registered when the pass starts.
#define LEGO_CM_NOTIFY(call)                                                                    \
    {                                                                                           \
        ControlManagerObserverIterator it(*this);                                               \
        while (ControlManagerObserver* o =                                                      \
                   it.Next(NotifierT<ControlManager, ControlManagerObserver>::m_observers))     \
            o->call;                                                                            \
    }

#if !defined(_WIN32)
// ===========================================================================================
// NativeControlHitTester (Mac only) — Mac vtable 0x706bec {HitTest 0xd7130, Activate 0xd71a0,
// D1 0xd71f0, D0 0xd7230}, Object-in 0x706c14. Layout vptr, ControlManager* +0x04, Vector2 +0x08
// (last tested position), Object +0x10 (operator new 0x18 in Initialize). (names inferred)
// ===========================================================================================
class NativeControlHitTester : public HitTester {
public:
    explicit NativeControlHitTester(ControlManager* manager) : m_manager(manager) {}

    // True when the topmost control at `position` is one of LDD's own (status bit 11 clear).
    virtual bool HitTest(const Vector2& position)                           // Mac 0xd7130
    {
        m_position = position;
        Control* control = m_manager->GetLayer(1)->FindControlAt(position);
        if (!control)
            return false;
        return !control->GetStatus().m_bit11;
    }
    // A native control took the click: the control there gets the keyboard focus.
    virtual bool Activate()                                                 // Mac 0xd71a0
    {
        return m_manager->SetKeyboardFocus(m_manager->GetLayer(1)->FindControlAt(m_position), false, false);
    }
    virtual ~NativeControlHitTester() {}                                    // Mac 0xd71f0/0xd7230

private:
    ControlManager* m_manager;    // +0x04
    Vector2         m_position;   // +0x08
};

// ===========================================================================================
// ControlManagerNativeControlKeyboardFocusObserver (Mac only) — Mac vtables 0x706c34
// {D1 0xd7660, D0 0xd77c0, OnManagerKeyboardFocusChanged 0xd75f0, OnFinalRelease 0xd7950},
// 0x706c50 (ControlManagerObserver-in, slot 13 = thunk 0xd7620), Object-in 0x706cbc.
// Layout: NativeControlKeyboardFocusObserver (vptr, Window* +0x04), ControlManagerObserver
// +0x08 (subject +0x0c), Object +0x10 (operator new 0x18).
// When the keyboard focus moves from a native control (status bit 11) to one of LDD's own, the
// window's drawing view becomes first responder again.
// ===========================================================================================
class ControlManagerNativeControlKeyboardFocusObserver : public NativeControlKeyboardFocusObserver,
                                                         public ControlManagerObserver {
public:
    explicit ControlManagerNativeControlKeyboardFocusObserver(ControlManager* manager) // Mac 0xd7530
    {
        Observer<ControlManager, ControlManagerObserver>::Attach(manager);
    }
    virtual ~ControlManagerNativeControlKeyboardFocusObserver() {}         // Mac 0xd7660/0xd77c0

    virtual void OnManagerKeyboardFocusChanged(bool /*flag*/, Control* previous, Control* current,
                                               UInt32 /*modifiers*/)       // Mac 0xd75f0 / 0xd7620
    {
        if (previous && previous->GetStatus().m_bit11 && (!current || !current->GetStatus().m_bit11))
            ToggleFirstResponder();
    }

protected:
    virtual void OnFinalRelease()                                           // Mac 0xd7950
    {
        ControlManagerObserver::OnFinalRelease();
    }
};
#endif

// ===========================================================================================
// ControlManager
// ===========================================================================================

// Mac 0xd7ad0, Win 0x506446
ControlManager::ControlManager(Window* window)
    : m_window(window),
      m_cursor(0),
      m_mouseControl(0),
      m_keyboardFocus(0),
      m_pendingFocus(0),
      m_savedFocus(0),
      m_inFocusChange(false),
      m_mouseCaptured(false),
      m_cursorVisible(false),
      m_inMouseOverUpdate(false),
      m_mouseOverRequested(false),
      m_mouseOverDirty(false),
      m_inMenuLoop(false),
      m_active(true),
      m_mouseButtons(0),
      m_modifiers(0),
      m_modalControl(0),
      m_updateLocked(false),
      m_valueA8(0)
{
    // NOTE: +0xa8 (m_valueA8) is left uninitialised by both originals; fixed: 0 (low).
    m_clickedControls.Reserve(4);
    UpdateObserver::Attach(static_cast<Updater*>(window));
    Observer<Window, WindowObserver>::Attach(window);
}

// Mac 0xdbc00 (D1) / 0xdbe90 (D0), Win 0x506d6f / sdd 0x506613: members and bases only.
ControlManager::~ControlManager()
{
}

// ---- HumanInput ---------------------------------------------------------------------------

// Mac 0xd95a0 (→ 0xd95b0), Win 0x5068a6 (→ 0x507585)
void ControlManager::OnMouseMove(const Vector2& position)
{
    HandleMouseMove(position);
}

// Mac 0xd98d0, Win 0x5068ab
void ControlManager::OnMouseDown(const Vector2& position, UInt32 button)
{
    HandleMouseButton(position, button, true, false);
    LEGO_CM_NOTIFY(OnManagerMouseDown(position, button))
}

// Mac 0xd9ec0, Win 0x50690e
void ControlManager::OnMouseUp(const Vector2& position, UInt32 button)
{
    HandleMouseButton(position, button, false, false);
    LEGO_CM_NOTIFY(OnManagerMouseUp(position, button))
}

// Mac 0xda180, Win 0x506971 (no observer notification)
void ControlManager::OnMouseDoubleClick(const Vector2& position, UInt32 button)
{
    HandleMouseButton(position, button, true, true);
}

// Mac 0xda1a0, Win 0x506989
void ControlManager::OnMouseWheel(UInt32 axis, Real32 delta)
{
    if (m_mouseControl)
        m_mouseControl->OnMouseWheel(axis, delta);
    LEGO_CM_NOTIFY(OnManagerMouseWheel(axis, delta))
}

// Mac 0xda480, Win 0x5069fd
void ControlManager::OnKeyDown(UInt32 key, UInt32 modifiers, bool repeat)
{
    m_modifiers = modifiers;
    if (m_keyboardFocus)
        m_keyboardFocus->OnKeyPressed(key, modifiers, repeat);
    LEGO_CM_NOTIFY(OnManagerKeyPressed(key, modifiers, repeat))
}

// Mac 0xda760, Win 0x506a6f
void ControlManager::OnKeyUp(UInt32 key, UInt32 modifiers)
{
    m_modifiers = modifiers;
    if (m_keyboardFocus)
        m_keyboardFocus->OnKeyReleased(key, modifiers);
    LEGO_CM_NOTIFY(OnManagerKeyReleased(key, modifiers))
}

// Mac 0xdaa20, Win 0x506adb
void ControlManager::OnCharacter(UniChar16 c, UInt32 modifiers, bool repeat)
{
    m_modifiers = modifiers;
    if (m_keyboardFocus)
        m_keyboardFocus->OnChar(c, modifiers, repeat);
    // NOTE: the observers get the stored modifiers (re-read after the control's handler).
    LEGO_CM_NOTIFY(OnManagerCharacter(c, m_modifiers, repeat))
}

// ---- WindowInput --------------------------------------------------------------------------

// Mac 0xe9450, Win 0x50628d
void ControlManager::OnResize(const Vector2& /*size*/, const Vector2& clientSize)
{
    Resize(clientSize);
}

// Mac 0xdad00, Win 0x506b52
void ControlManager::OnActivate(bool active)
{
    if (m_active == active)
        return;
    m_active = active;
    if (m_mouseControl) {
        m_mouseControl->m_value124 = 0;
        if (m_mouseCaptured)
            CancelMouseCapture();
    }
    UpdateMouseOver();
}

// Mac 0xdad90, Win 0x506b93
void ControlManager::OnInput13(bool flag, UInt32 modifiers, UInt32 arg)
{
    m_modifiers = modifiers;
    if (!flag) {
        if (arg != 0)
            return;
        if (m_keyboardFocus) {
            m_savedFocus = m_keyboardFocus;
            SetKeyboardFocus(0, false, false);
        }
    } else {
        if (m_keyboardFocus)
            return;
        SetKeyboardFocus(m_savedFocus, false, false);
        m_savedFocus = 0;
    }
    LEGO_CM_NOTIFY(OnManagerWindowKeyboardFocusChanged(flag, m_modifiers))
}

// Mac 0xe9470, Win 0x506281
bool ControlManager::OnCloseRequest()
{
    return m_modalControl == 0;
}

// Mac 0xe9490, Win 0x50629d
bool ControlManager::OnInput17()
{
    Vector2 size = m_window->GetSize();
    Resize(size);
    return true;
}

// Mac 0xdb0a0, Win 0x506c2b
void ControlManager::OnInput19()
{
    m_layers[0]->Invalidate(true);
    m_layers[0]->InvalidateBuffered();
}

// Mac 0xdb220, Win 0x506c41
void ControlManager::OnInput20(UInt32 arg)
{
    if ((Int16)arg == 1) {
        m_layers[0]->Invalidate(true);
        m_layers[0]->InvalidateBuffered();
    }
}

// Mac 0xe94d0, Win 0x5062bd: the window hides (arg) / shows the cursor.
void ControlManager::OnInput21(UInt32 arg)
{
    SetCursorVisible(((UInt8)arg ^ 1) != 0);
}

// Mac 0xdb260, Win 0x506c62
void ControlManager::OnInput22()
{
    m_inMenuLoop = true;
    if (m_keyboardFocus) {
        m_savedFocus = m_keyboardFocus;
        SetKeyboardFocus(0, false, false);
    }
    if (m_mouseControl && !m_mouseCaptured)
        UpdateMouseOver();
    LEGO_CM_NOTIFY(OnManagerWindowEnterMenuLoop())
}

// Mac 0xdb520, Win 0x506ce1
void ControlManager::OnInput23()
{
    m_inMenuLoop = false;
    if (m_savedFocus) {
        SetKeyboardFocus(m_savedFocus, false, false);
        m_savedFocus = 0;
    }
    UpdateMouseOver();
    LEGO_CM_NOTIFY(OnManagerWindowExitMenuLoop())
}

// ---- ControlManager virtuals --------------------------------------------------------------

// Mac 0xd8640, Win 0x50664b
bool ControlManager::Initialize()
{
    m_objectFactory = new ObjectFactory();
    CreateLayer(0);
    CreateLayer(1);
    CreateLayer(2);
    Vector2 size = m_window->GetSize();
    Resize(size);
#if !defined(_WIN32)
    RefPointer<HitTester> tester = new NativeControlHitTester(this);
    m_window->SetHitTester(tester);
    RefPointer<NativeControlKeyboardFocusObserver> observer =
        new ControlManagerNativeControlKeyboardFocusObserver(this);
    m_window->SetKeyboardFocusObserver(observer);
#endif
    return true;
}

// Mac 0xdc180, Win 0x506e5e
void ControlManager::OnFinalRelease()
{
    SetKeyboardFocus(0, false, false);
    m_mouseControl = 0;
    while (!m_augmentations.IsEmpty()) {
        ControlManagerAugmentation* augmentation = m_augmentations.Begin()->second;
        ControlManager* manager = augmentation->m_manager;
        // NOTE: an augmentation in the map without a manager makes both originals spin here.
        if (!manager)
            continue;
        RefPointer<ControlManagerAugmentation> keep(augmentation);
        augmentation->m_manager = 0;
        manager->RemoveAugmentation(augmentation);
        augmentation->OnManagerDetached(manager);
    }
    if (m_cursor)
        m_cursor->Detach();
    Notifier<ControlManagerObserver>::OnFinalRelease();
    m_layers[2] = 0;
    m_layers[1] = 0;
    m_layers[0] = 0;
    m_objectFactory = 0;
    Updater::OnFinalRelease();
    UpdateObserver::OnFinalRelease();
    WindowObserver::OnFinalRelease();
}

// Mac 0xdb7d0 (thunks 0xdb810, 0xdb850), Win 0x506d4c
void ControlManager::Update()
{
    Updater::Update();
    if (m_mouseOverDirty) {
        m_mouseOverDirty = false;
        UpdateMouseOver();
    }
}

// Mac 0xdc580, Win 0x506f03
void ControlManager::CreateLayer(UInt32 layer)
{
    new Control(this, layer);
}

// Mac 0xdcb30, Win 0x506f3f
void ControlManager::NotifyResized()
{
    LEGO_CM_NOTIFY(OnManagerResized())
}

// Mac 0xdcda0, Win 0x506f89
void ControlManager::NotifyMouseEnter(Control* control, const Vector2& position)
{
    LEGO_CM_NOTIFY(OnManagerMouseEnter(control, position))
}

// Mac 0xdd020, Win 0x506fdb
void ControlManager::NotifyMouseLeave(Control* control, const Vector2& position)
{
    LEGO_CM_NOTIFY(OnManagerMouseLeave(control, position))
}

// Mac 0xdd2a0, Win 0x50702d
void ControlManager::NotifyMouseOverChanged(Control* previous, Control* current)
{
    LEGO_CM_NOTIFY(OnManagerMouseOverChanged(previous, current))
}

// Mac 0xdd520, Win 0x50707f
void ControlManager::NotifyKeyboardFocusChanged(bool flag, Control* previous, Control* current)
{
    LEGO_CM_NOTIFY(OnManagerKeyboardFocusChanged(flag, previous, current, m_modifiers))
}

// Mac 0xdd7b0, Win 0x5070da
bool ControlManager::SetCursor(Cursor* cursor)
{
    if (m_cursor == cursor)
        return true;
    if (Control* control = dynamic_cast<Control*>(cursor)) {
        if (!control->SetParent(m_layers[2], 0))
            return false;
    }
    if (m_cursor)
        m_cursor->Detach();
    m_cursor = cursor;
    cursor->SetCursorPosition(m_mousePosition);
    m_cursor->SetCursorVisible(m_cursorVisible);
    if (!m_mouseControl)
        ApplyCursor(1, true);
    else
        ApplyCursor(m_mouseControl->m_cursor, true);
    return true;
}

// Mac 0xddee0, Win 0x507160
void ControlManager::RemoveCursor(Cursor* cursor)
{
    if (m_cursor != cursor)
        return;
    if (Control* control = dynamic_cast<Control*>(cursor))
        control->SetParent(0, 0);
    m_cursor = 0;
}

// Mac 0xddf30, Win 0x50718a. Only the map entry: the augmentation sets its manager itself.
bool ControlManager::AddAugmentation(ControlManagerAugmentation* augmentation)
{
    if (m_augmentations.Find(augmentation->m_name) != m_augmentations.End())
        return false;
    m_augmentations[augmentation->m_name] = augmentation;
    return true;
}

// Mac 0xde010, Win 0x5071fd
void ControlManager::RemoveAugmentation(ControlManagerAugmentation* augmentation)
{
    BasicMap<String, RefPointer<ControlManagerAugmentation> >::Iterator it =
        m_augmentations.Find(augmentation->m_name);
    // NOTE: both originals erase find()'s result unchecked (end() for an unknown name corrupts the
    // tree); fixed: an unknown name is ignored.
    if (it != m_augmentations.End())
        m_augmentations.Erase(it);
}

// ---- non-virtual --------------------------------------------------------------------------

// Mac 0xd89c0, Win 0x5066c9
Vector2 ControlManager::GetMousePosition(Control* control) const
{
    Vector2 position(m_mousePosition);
    control->ManagerToLocal(position);
    return position;
}

// Mac 0xd7180, Win 0x5066c1 (→ 0x509d52)
Control* ControlManager::GetControlAt(const Vector2& position) const
{
    return m_layers[1]->FindControlAt(position);
}

Control* ControlManager::FindControlAt(Control* root, const Vector2& position) const
{
    return root->FindControlAt(position);
}

// Mac 0xd71d0, Win 0x5066e7
bool ControlManager::SetKeyboardFocus(Control* control)
{
    return SetKeyboardFocus(control, false, false);
}

// Mac 0xd8a00, Win 0x50788e
bool ControlManager::SetKeyboardFocus(Control* control, bool walkUp, bool flag)
{
    for (;;) {
        Control* previous = m_keyboardFocus;
        if (previous == control)
            return true;
        if (m_inFocusChange) {
            m_pendingFocus = control;
            return false;
        }
        Control* target;
        if (!walkUp && !flag) {
            target = control;
            if (control) {
                if (!(control->m_inputFlags & 2))
                    return false;
                if (!control->m_status.m_enabledInTree || !control->m_status.m_visible)
                    return false;
                if (!IsInsideModal(control))
                    return false;
            }
        } else {
            // NOTE: a null `control` is dereferenced here in both originals.
            while (!(control->m_inputFlags & 2) || !control->m_status.m_enabledInTree) {
                control = control->m_parent;
                if (!control)
                    return false;
            }
            target = control;
            if (!IsInsideModal(control))
                return false;
        }
        m_inFocusChange = true;
        m_pendingFocus = target;
        if (previous) {
            m_keyboardFocus = 0;
            previous->OnKeyboardFocusLost(target);
        }
        m_keyboardFocus = target;
        if (target)
            target->OnKeyboardFocusGained(flag, previous, m_modifiers);
        NotifyKeyboardFocusChanged(flag, previous, m_keyboardFocus);
        m_inFocusChange = false;
        control = m_pendingFocus;
        walkUp = false;
        flag = false;
        if (control == target)
            return true;
    }
}

// Mac 0xd8b60, Win 0x5066fb
bool ControlManager::SetModalControl(bool modal, Control* control)
{
    if (!modal) {
        if (m_modalControl != control)
            return false;
        m_window->SetModalState(false);
        m_modalControl = 0;
        LEGO_CM_NOTIFY(OnManagerModalControl(false, control))
        UpdateMouseOver();
        return true;
    }
    m_modalControl = control;
    if (m_mouseControl && !IsInsideModal(m_mouseControl)) {
        if (m_mouseCaptured) {
            m_mouseCaptured = false;
            m_inMouseOverUpdate = true;
            m_mouseControl->OnMouseCaptureLost();
            m_inMouseOverUpdate = false;
            m_mouseOverRequested = false;
        }
        UpdateMouseOver();
    }
    if (m_keyboardFocus && !IsInsideModal(m_keyboardFocus))
        SetKeyboardFocus(0, false, false);
    m_window->SetModalState(true);
    LEGO_CM_NOTIFY(OnManagerModalControl(true, control))
    return true;
}

// Mac 0xd9120, Win 0x507635
void ControlManager::UpdateMouseOver()
{
    for (;;) {
        if (m_inMouseOverUpdate) {
            m_mouseOverRequested = true;
            return;
        }
        m_inMouseOverUpdate = true;
        m_mouseOverRequested = false;

        Control* previous = m_mouseControl;
        Control* hit = 0;
        if (!m_inMenuLoop && m_active) {
            hit = m_layers[1]->FindControlAt(m_mousePosition);
            while (hit && !(hit->m_inputFlags & 1))
                hit = hit->m_parent;
            if (!IsInsideModal(hit))
                hit = 0;
        }

        if (hit && hit == previous) {
            if (!previous->m_status.m_mouseInside) {
                previous->m_status.m_mouseInside = 1;
                Vector2 position = GetMousePosition(m_mouseControl);
                NotifyMouseEnter(m_mouseControl, position);
                m_mouseControl->OnMouseEnter(position);
            }
        } else {
            bool changed = false;
            bool done = false;
            if (previous) {
                if (previous->m_status.m_mouseInside) {
                    previous->m_status.m_mouseInside = 0;
                    Vector2 position = GetMousePosition(previous);
                    NotifyMouseLeave(previous, position);
                    previous->OnMouseLeave(position);
                }
                if (previous != m_mouseControl)
                    done = true;
                else if (!m_mouseCaptured && previous->m_value124 == 0) {
                    m_mouseControl = 0;
                    changed = true;
                } else {
                    hit = 0;
                }
            }
            if (!done && hit) {
                m_mouseControl = hit;
                hit->m_status.m_mouseInside = 1;
                changed = true;
                Vector2 position = GetMousePosition(m_mouseControl);
                NotifyMouseEnter(m_mouseControl, position);
                m_mouseControl->OnMouseEnter(position);
                if (hit != m_mouseControl)
                    done = true;
            }
            if (!done && changed) {
                if (!m_mouseControl) {
                    if (m_cursor)
                        m_cursor->SetShapeEx(1, 0);
                } else {
                    ApplyCursor(m_mouseControl->m_cursor, m_mouseControl->m_flag12c);
                }
                NotifyMouseOverChanged(previous, m_mouseControl);
            }
        }

        m_inMouseOverUpdate = false;
        if (!m_mouseOverRequested)
            return;
        m_mouseOverRequested = false;
    }
}

// Mac 0xd93d0, Win 0x506860
bool ControlManager::ChangeCursor(Cursor* cursor)
{
    if (!cursor) {
        if (m_cursor)
            m_cursor->Detach();
        return true;
    }
    return cursor->Attach();
}

// Mac 0xd9400, Win 0x506886
void ControlManager::SetCursorVisible(bool visible)
{
    if (m_cursorVisible == visible)
        return;
    m_cursorVisible = visible;
    if (m_cursor)
        m_cursor->SetCursorVisible(visible);
}

// Win 0x507864 (inlined on Mac: 0xd9120, 0xdd7b0, Control::SetCursor 0xe2330)
void ControlManager::ApplyCursor(UInt32 shape, bool flag)
{
    if (!m_cursor)
        return;
    if (flag)
        m_cursor->SetShape(shape);
    else
        m_cursor->SetShapeEx(shape, 0);
}

// Mac 0xd9440
void ControlManager::Draw(ControlDrawArgs& args)
{
    m_updateLocked = true;
    DrawControl(m_layers[0], args);
    m_updateLocked = false;
}

// Mac 0xd9470
bool ControlManager::DrawControl(Control* control, ControlDrawArgs& args)
{
    if (!control->m_status.m_connected || !control->m_status.m_visible || !control->m_buffered)
        return false;
    args.m_offset = Vector2(0.0f, 0.0f);
    args.m_scale = Vector2(1.0f, 1.0f);
    args.m_clip.Set(Rect(control->m_size));
    control->DrawTree(args);
    return true;
}

// Mac 0xd9510, Win 0x51387d
ControlManagerAugmentation* ControlManager::GetAugmentation(const String& name) const
{
    BasicMap<String, RefPointer<ControlManagerAugmentation> >::ConstIterator it = m_augmentations.Find(name);
    if (it == m_augmentations.End())
        return 0;
    return it->second;
}

// Mac 0xd95b0, Win 0x507585
void ControlManager::HandleMouseMove(const Vector2& position)
{
    if (!(m_mousePosition != position))
        return;
    m_mousePosition = position;
    LEGO_CM_NOTIFY(OnManagerMouseMove(position))
    if (m_mouseControl) {
        Vector2 local = GetMousePosition(m_mouseControl);
        m_mouseControl->OnMouseMove(local);
    }
    m_mouseOverDirty = false;
    UpdateMouseOver();
    if (m_cursor)
        m_cursor->SetCursorPosition(m_mousePosition);
}

// Mac 0xd9b90, Win 0x5072e9
void ControlManager::HandleMouseButton(const Vector2& position, UInt32 button, bool down, bool doubleClick)
{
    HandleMouseMove(position);
    if (m_mouseOverDirty) {
        m_mouseOverDirty = false;
        UpdateMouseOver();
    }
    if (m_clickedControls.GetSize() <= button)
        m_clickedControls.Insert(m_clickedControls.End(), button + 1 - m_clickedControls.GetSize(), (Control*)0);

    UInt32 bit = 1u << (button & 0x1f);
    if ((m_mouseButtons & bit) || down) {
        if (!down) {
            m_mouseButtons &= ~bit;
            Control* control = m_mouseControl;
            if (!control || !control->m_status.m_enabledInTree || !(control->m_value124 & bit)) {
                m_clickedControls[button] = 0;
            } else {
                control->m_value124 &= ~bit;
                bool inside = control->m_status.m_mouseInside != 0;
                RefPointer<Control> keep(control);
                Vector2 local = GetMousePosition(m_mouseControl);
                m_mouseControl->OnMouseReleased(local, button);
                if (!inside) {
                    m_clickedControls[button] = 0;
                } else {
                    m_clickedControls[button] = control;
                    control->OnMouseClicked(local, button);
                }
                if (control != m_mouseControl)
                    return;
            }
            m_mouseOverDirty = false;
            UpdateMouseOver();
            return;
        }
        m_mouseButtons |= bit;
        Control* control = m_mouseControl;
        if (control && control->m_status.m_enabledInTree) {
            RefPointer<Control> keep(control);
            if (!(m_mouseControl->m_value124 & bit)) {
                m_mouseControl->m_value124 |= bit;
                Vector2 local = GetMousePosition(m_mouseControl);
                m_mouseControl->OnMousePressed(local, button);
                if (doubleClick && control == m_clickedControls[button])
                    control->OnMouseDoubleClicked(local, button);
            }
            if (control != m_mouseControl) {
                m_clickedControls[button] = 0;
                return;
            }
            SetKeyboardFocus(control, true, true);
        }
    }
    m_clickedControls[button] = 0;
}

// Mac 0xd8820, Win 0x507238
void ControlManager::Resize(const Vector2& size)
{
    Vector2 previous(m_layers[0]->m_size);
    if (size == previous)
        return;
    m_layers[0]->m_size = size;
    m_layers[1]->m_size = size;
    m_layers[2]->m_size = size;
    m_layers[0]->Invalidate(true);
    m_layers[1]->Invalidate(true);
    m_layers[2]->Invalidate(true);
    // NOTE: all three get layer 0's position as the old position.
    m_layers[0]->OnRectangleChanged(m_layers[0]->m_position, previous);
    m_layers[1]->OnRectangleChanged(m_layers[0]->m_position, previous);
    m_layers[2]->OnRectangleChanged(m_layers[0]->m_position, previous);
    NotifyResized();
}

// Mac 0xde100 (Win inlined into Control 0x5079bc)
bool ControlManager::SetMouseCapture(Control* control)
{
    if (!control->m_status.m_connected || !control->m_status.m_visible || !(control->m_inputFlags & 1))
        return false;
    if (!IsInsideModal(control))
        return false;
    Control* previous = m_mouseControl;
    if (!m_mouseCaptured) {
        m_mouseControl = 0;
    } else {
        if (previous == control)
            return true;
        m_mouseControl = 0;
        m_mouseCaptured = false;
        m_inMouseOverUpdate = true;
        previous->OnMouseCaptureLost();
        m_inMouseOverUpdate = false;
    }
    if (previous && previous != control && previous->m_status.m_mouseInside) {
        previous->m_status.m_mouseInside = 0;
        m_inMouseOverUpdate = true;
        Vector2 position = GetMousePosition(previous);
        NotifyMouseLeave(previous, position);
        previous->OnMouseLeave(position);
        m_inMouseOverUpdate = false;
    }
    m_mouseControl = control;
    m_mouseCaptured = true;
    m_inMouseOverUpdate = true;
    control->OnMouseCaptured();
    m_inMouseOverUpdate = false;
    if (m_mouseControl != previous) {
        m_inMouseOverUpdate = true;
        NotifyMouseOverChanged(previous, m_mouseControl);
        m_inMouseOverUpdate = false;
    }
    if (m_mouseOverRequested) {
        m_mouseOverRequested = false;
        UpdateMouseOver();
    }
    return true;
}

// Win 0x507b06 (Mac inlined in OnActivate 0xdad00)
bool ControlManager::CancelMouseCapture()
{
    Control* control = m_mouseControl;
    bool inside = control->m_status.m_mouseInside != 0;
    m_mouseCaptured = false;
    m_inMouseOverUpdate = true;
    control->OnMouseCaptureLost();
    m_inMouseOverUpdate = false;
    if (!inside || m_mouseOverRequested) {
        m_mouseOverRequested = false;
        UpdateMouseOver();
    }
    return true;
}

// Win 0x50799d (Mac inlined)
bool ControlManager::IsInsideModal(Control* control) const
{
    if (m_modalControl && control) {
        while (m_modalControl != control) {
            control = control->m_parent;
            if (!control)
                return false;
        }
    }
    return true;
}

// ===========================================================================================
// Control members in this part of the TU
// ===========================================================================================

// One of the manager's layer roots; layers 1 and 2 are children of layer 0 (2 behind 1).
// Mac 0xdc5d0 (operator new 0x160 in CreateLayer), Win 0x507cfb (0x178 bytes; `this` on the
// stack, flag 1 = most derived)
Control::Control(ControlManager* manager, UInt32 layer)
    : m_manager(manager),
      m_parent(0),
      m_previousSibling(0),
      m_nextSibling(0),
      m_firstChild(0),
      m_lastChild(0),
      m_position(),
      m_size(),
      m_scale(1.0f, 1.0f),
      m_minimumSize(0.0f, 0.0f),
      m_maximumSize(32768.0f, 32768.0f),
      m_value68(0),
      m_inputFlags(0),
      m_value124(0),
      m_cursor(1),
      m_flag12c(false),
      m_buffered(false)
{
    *reinterpret_cast<UInt16*>(&m_status) &= 0xf000;
    manager->m_layers[layer] = this;
    // status bits 0-7: fixed scale, connected, visible, shown, enabled (+ in tree), active (+ in tree)
    m_status.m_fixedScale = 1;
    m_status.m_connected = 1;
    m_status.m_visible = 1;
    m_status.m_shown = 1;
    m_status.m_enabledInTree = 1;
    m_status.m_enabled = 1;
    m_status.m_activeInTree = 1;
    m_status.m_active = 1;
    if (layer == 0) {
        m_inputFlags |= 4;
        m_buffered = true;
        return;
    }
    if (layer != 1 && layer != 2)
        return;
    // Link below layer 0 behind `after` (null: last; the parent itself: first): SetParent's link,
    // inlined.
    Control* parent = manager->m_layers[0];
    Control* after = layer == 2 ? (Control*)manager->m_layers[1] : 0;
    m_parent = parent;
    Control* next;
    if (!after) {
        m_previousSibling = parent->m_lastChild;
        m_nextSibling = 0;
        next = 0;
        if (m_previousSibling)
            m_previousSibling->m_nextSibling = this;
        else
            parent->m_firstChild = this;
    } else if (after == parent) {
        next = parent->m_firstChild;
        m_previousSibling = 0;
        m_nextSibling = next;
        parent->m_firstChild = this;
    } else {
        next = after->m_nextSibling;
        m_previousSibling = after;
        m_nextSibling = next;
        after->m_nextSibling = this;
    }
    if (next)
        next->m_previousSibling = this;
    else
        parent->m_lastChild = this;
}

// Mac 0xdb0d0, Win 0x50a29c (inlined variant 0x44b400)
void Control::Invalidate(bool force)
{
    if (m_status.m_invalid && (!m_parent || m_parent->m_status.m_invalid))
        return;
    if (m_parent && !force) {
        Rect parentRect(m_parent->m_size);
        Rect rect(m_position, Vector2(m_size.x * m_scale.x, m_size.y * m_scale.y));
        rect &= parentRect;
        if (!rect.IsValid())
            return;
    }
    bool visible = m_status.m_visible != 0;
    m_status.m_invalid = 1;
    if (m_parent && (visible || !m_parent->m_status.m_visible))
        m_parent->Invalidate(false);
}

// Mac 0xdb1c0, Win 0x50a2e2
void Control::InvalidateBuffered()
{
    if (m_buffered && !m_manager->m_updateLocked)
        Invalidate(false);
    for (Control* child = m_firstChild; child; child = child->m_nextSibling)
        child->InvalidateBuffered();
}

// Mac 0xd8910, Win 0x509d52
Control* Control::FindControlAt(const Vector2& position)
{
    Vector2 local((position.x - m_position.x) / m_scale.x, (position.y - m_position.y) / m_scale.y);
    Rect rect(m_size);
    if (!rect.Contains(local))
        return 0;
    for (Control* child = m_lastChild; child; child = child->m_previousSibling) {
        if (child->m_status.m_visible) {
            if (Control* found = child->FindControlAt(local))
                return found;
        }
    }
    if (IsInside(local))
        return this;
    return 0;
}

// Mac 0xe76b0, Win 0x509b30. A taken name succeeds only for the same augmentation.
bool Control::AddAugmentation(const String& name, ControlAugmentation* augmentation)
{
    BasicMap<String, RefPointer<ControlAugmentation> >::Iterator it = m_augmentations.Find(name);
    if (it != m_augmentations.End())
        return it->second == augmentation;
    m_augmentations[name] = augmentation;
    return true;
}

// Mac 0xe7790, Win 0x509bb1
void Control::RemoveAugmentation(const String& name)
{
    BasicMap<String, RefPointer<ControlAugmentation> >::Iterator it = m_augmentations.Find(name);
    // NOTE: unchecked erase in the originals (see ControlManager::RemoveAugmentation); fixed.
    if (it != m_augmentations.End())
        m_augmentations.Erase(it);
}

} // namespace LEGO
