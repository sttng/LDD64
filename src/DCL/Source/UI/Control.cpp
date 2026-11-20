// LEGO Digital Designer — restored source
// LEGO::Control body of TU 027 "Control.cpp": Mac sub-TU 027b (0xde100-0xe76b0) + SetParent
// 0xdd890 (027a), the anchor / layout / draw helpers 0xe7880-0xe86f0 and ManagerToLocal 0xe9510
// (027c); Win 0x5079bc-0x50a9f7 (most helpers split out by /LTCG). The layer ctor (Mac
// 0xdc5d0), Invalidate (0xdb0d0), InvalidateBuffered (0xdb1c0), FindControlAt (0xd8910),
// AddAugmentation / RemoveAugmentation (0xe76b0 / 0xe7790) and ControlManager::SetMouseCapture
// (0xde100) are in ControlManager.cpp. All names inferred.
// Arithmetic: Mac computes every operation in SSE single precision; Win (MSVC x87, 53-bit
// precision) keeps an expression's intermediates at double and rounds to float on store.
// `Intermediate` reproduces both: multi-step expressions use it, every Real32 assignment is a
// store; single float operations agree either way.
#include "Control.h"
#include "ControlManager.h"
#include <math.h>

namespace LEGO {

namespace {

#if defined(_WIN32)
typedef double Intermediate;   // x87 (MSVC): expression intermediates at 53-bit precision
#else
typedef Real32 Intermediate;   // SSE (clang): every operation rounded to float
#endif
inline Intermediate X(Real32 value) { return (Intermediate)value; }

// Nearest integer value, ties up (Win 0x4c9630 LEGO::RoundToNearest; inlined on the Mac).
inline Real32 RoundToNearest(Real32 value)
{
    Real32 lower = (Real32)floor((double)value);
    Real32 upper = (Real32)ceil((double)value);
    return value - lower < upper - value ? lower : upper;
}

// Component `axis` of a vector (the original indexes through operator Real32*).
inline Real32& At(Vector2& v, UInt32 axis) { return (&v.x)[axis]; }
inline Real32 At(const Vector2& v, UInt32 axis) { return (&v.x)[axis]; }

// Removes the first `value` (the last element takes its place). Mac 0xe7ff0 (BasicArray
// instance: std::find, *it = back, erase(end - 1)).
void RemoveValue(BasicArray<Control*>& array, Control* value)
{
    BasicArray<Control*>::Iterator it = array.Begin();
    while (it != array.End() && *it != value)
        ++it;
    if (it == array.End())
        return;
    BasicArray<Control*>::Iterator last = array.End() - 1;
    if (it != last)
        *it = *last;
    array.PopBack();
}

// From the coordinates of `ancestor` into those of `control` (parent chain below `ancestor`,
// outermost first). Mac 0xe86f0 (custom convention: EDX value, ECX axis), Win 0x50a62a
// NOTE: no null check (both builds): when `ancestor` is not above `control` (see UpdateAnchors)
// the walk dereferences null; fixed: it stops at the root (low).
void AncestorToLocal(const Control* control, const Control* ancestor, UInt32 axis, Real32& value)
{
    if (control == ancestor || control == 0)
        return;
    AncestorToLocal(control->GetParent(), ancestor, axis, value);
    value = (Real32)((X(value) - At(control->GetPosition(), axis)) / At(control->GetScale(), axis));
}

} // namespace

// ===========================================================================================
// InternalAnchorData
// ===========================================================================================
InternalAnchorData::InternalAnchorData()
    : m_reference(0), m_flag(false), m_commonAncestor(0)
{
    for (UInt32 i = 0; i < 5; ++i)
        m_values[i] = 0.0f;
}

// Mac 0xe1c70
InternalAnchorData::InternalAnchorData(Real32 v0, Real32 v1, Real32 v2, Real32 v3, Real32 v4,
                                       Control* reference, bool referenceIsParent)
    : m_reference(reference), m_flag(referenceIsParent), m_commonAncestor(0)
{
    m_values[0] = v0;
    m_values[1] = v1;
    m_values[2] = v2;
    m_values[3] = v3;
    m_values[4] = v4;
}

namespace {

// Win 0x50a86d: the common body (reference already resolved by the callers' flag).
void AssignAnchor(InternalAnchorData& anchor, Control* owner, Real32 v0, Real32 v1, Real32 v2,
                  Real32 v3, Real32 v4, Control* reference, bool referenceIsParent,
                  bool& referenceChanged, bool& changed)
{
    if (referenceIsParent)
        reference = owner->GetParent();
    if (anchor.m_reference != reference) {
        anchor.m_reference = reference;
        referenceChanged = true;
    }
    anchor.m_flag = referenceIsParent;
    if (referenceChanged || changed || anchor.m_values[0] != v0 || anchor.m_values[1] != v1 ||
        anchor.m_values[2] != v2 || anchor.m_values[3] != v3 || anchor.m_values[4] != v4) {
        anchor.m_values[0] = v0;
        anchor.m_values[1] = v1;
        anchor.m_values[2] = v2;
        anchor.m_values[3] = v3;
        anchor.m_values[4] = v4;
        changed = true;
    }
}

} // namespace

// Win 0x50a831 (→ 0x50a86d)
void InternalAnchorData::Set(Control* owner, Real32 v0, Real32 v1, Real32 v2, Real32 v3, Real32 v4,
                             Control* reference, bool& referenceChanged, bool& changed)
{
    AssignAnchor(*this, owner, v0, v1, v2, v3, v4, reference, reference == 0, referenceChanged,
                 changed);
}

// Win 0x50a905
void InternalAnchorData::Set(Control* owner, const InternalAnchorData* anchor,
                             bool& referenceChanged, bool& changed)
{
    if (!anchor)
        return;
    AssignAnchor(*this, owner, anchor->m_values[0], anchor->m_values[1], anchor->m_values[2],
                 anchor->m_values[3], anchor->m_values[4], anchor->m_reference, anchor->m_flag,
                 referenceChanged, changed);
}

// Win 0x50a994
void InternalAnchorData::SetAligned(Control* owner, UInt32 alignment, UInt32 referenceAlignment,
                                    Real32 offset, Control* reference, bool& referenceChanged,
                                    bool& changed)
{
    Set(owner, Control::AlignmentToFraction(alignment), 0.0f,
        Control::AlignmentToFraction(referenceAlignment), 0.0f, offset, reference,
        referenceChanged, changed);
}

// Win 0x50a943 tail, 0x50a9e0
void InternalAnchorData::Clear(bool& referenceChanged)
{
    if (m_reference || m_flag) {
        m_reference = 0;
        m_flag = false;
        referenceChanged = true;
    }
}

// ===========================================================================================
// Construction / destruction
// ===========================================================================================

// Mac 0xde780 (C1) / 0xde290 (C2), Win 0x507b51
Control::Control(ControlManager* manager)
    : m_manager(manager),
      m_parent(0),
      m_previousSibling(0),
      m_nextSibling(0),
      m_firstChild(0),
      m_lastChild(0),
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
    *(UInt16*)&m_status &= 0xf000;
    // A child control keeps its manager alive (the layer roots do not: bit 0).
    m_manager->AddRef();
}

// Mac 0xe7230 (D1) / 0xe7300 (D0) / 0xe6fc0 (D2), Win 0x507ccb (scalar deleting)
Control::~Control()
{
    if (!m_status.m_fixedScale)
        m_manager->Release();
}

// Mac 0xe73f0 (thunk 0xe7690), Win 0x5099d0
void Control::OnFinalRelease()
{
    // Each augmentation leaves through ControlAugmentation::Detach (→ RemoveAugmentation).
    while (!m_augmentations.IsEmpty())
        m_augmentations.Begin()->second->Detach();
    // Observers leave (last first), then Object::OnFinalRelease.
    Notifier<ControlObserver>::OnFinalRelease();

    while (m_lastChild)
        m_lastChild->SetParent(0, 0);

    // Controls anchored to this one lose the reference.
    while (!m_controlsFC.IsEmpty()) {
        Control* dependent = m_controlsFC[0];
        for (UInt32 axis = 0; axis < 2; ++axis)
            for (UInt32 i = 0; i < 2; ++i)
                if (dependent->m_anchors[axis].m_anchors[i].m_reference == this)
                    dependent->m_anchors[axis].m_anchors[i].m_reference = 0;
        dependent->UpdateAnchors(false);
    }
    for (UInt32 axis = 0; axis < 2; ++axis) {
        for (UInt32 i = 0; i < 2; ++i) {
            InternalAnchorData& anchor = m_anchors[axis].m_anchors[i];
            anchor.m_reference = 0;
            anchor.m_commonAncestor = 0;
            anchor.m_flag = false;
        }
    }
    while (!m_controlsEC.IsEmpty()) {
        Control* source = m_controlsEC[0];
        RemoveValue(source->m_controlsFC, this);
        RemoveValue(m_controlsEC, source);
    }

    if (!m_status.m_fixedScale) {
        SetParent(0, 0);
    } else {
        // A layer root leaves its parent (layers 1 and 2 are children of layer 0).
        Unlink();
        m_parent = 0;
        UpdateVisible();
        UpdateEnabled();
        UpdateActive();
        Disconnect();
        OnDestroyed();
    }

    // The manager forgets this control as the last clicked one of any button.
    ControlManager* manager = m_manager;
    for (BasicArray<Control*>::Iterator it = manager->m_clickedControls.Begin();
         it != manager->m_clickedControls.End(); ++it) {
        if (*it == this)
            *it = 0;
    }
}

// ===========================================================================================
// Coordinates
// ===========================================================================================

const Vector2& Control::GetSize() const
{
    return m_size;
}

// Mac 0xe9510
void Control::ManagerToLocal(Vector2& point) const
{
    if (m_parent)
        m_parent->ManagerToLocal(point);
    point -= m_position;
    point.x = (Real32)((X(1.0f) / m_scale.x) * point.x);
    point.y = (Real32)((X(1.0f) / m_scale.y) * point.y);
}

// Mac 0xe0780, Win 0x5085a9
Vector2 Control::LocalToControl(const Control* target, const Vector2& point) const
{
    Vector2 result(point);
    const Control* control = this;
    do {
        result.x = control->m_scale.x * result.x;
        result.y = control->m_scale.y * result.y;
        result += control->m_position;
        control = control->m_parent;
    } while (control);
    if (target) {
        Vector2 local(result);
        target->ManagerToLocal(local);
        result = local;
    }
    return result;
}

// Mac 0xe2390, Win 0x508f60
Vector2 Control::GetAbsolutePosition() const
{
    if (!m_parent)
        return Vector2(m_position);
    Vector2 parentPosition = m_parent->GetAbsolutePosition();
    Vector2 offset(m_position.x * m_parent->m_scale.x, m_position.y * m_parent->m_scale.y);
    return parentPosition + offset;
}

// Mac 0xe02a0, Win 0x45a2c0
bool Control::IsDescendantOf(const Control* ancestor) const
{
    const Control* control = this;
    do {
        control = control->m_parent;
        if (!control)
            return false;
    } while (control != ancestor);
    return true;
}

// ===========================================================================================
// Hit test, selection mask, input flags, cursor, buffering, mouse capture
// ===========================================================================================

// Mac 0xe1ff0, Win 0x508c49
bool Control::IsInside(const Vector2& point) const
{
    if (m_selectionMask.Get())
        return m_selectionMask->Contains(point);
    return m_status.m_bit10;
}

// Mac 0xe2030, Win 0x508ca2
void Control::SetSelectionMask(const SelectionMask* mask)
{
    if (m_selectionMask.Get() == mask)
        return;
    m_selectionMask = mask;
    if (m_status.m_connected)
        m_manager->m_mouseOverDirty = true;
}

// Mac 0xdf530, Win 0x508c65
void Control::SetHitTestable(bool hitTestable)
{
    if (m_status.m_bit10 == (UInt16)hitTestable)
        return;
    m_status.m_bit10 = hitTestable;
    if (m_status.m_connected)
        m_manager->m_mouseOverDirty = true;
}

// The manager stops pointing at this control as the one under the mouse. (inlined in
// UpdateVisible Mac 0xdfca0, SetInputFlags 0xe2090, ControlManager::SetMouseCapture 0xde100)
void Control::LoseMouse()
{
    ControlManager* manager = m_manager;
    if (manager->m_mouseControl != this)
        return;
    manager->m_mouseControl = 0;
    m_value124 = 0;
    if (m_status.m_mouseInside) {
        m_status.m_mouseInside = 0;
        Vector2 position(manager->m_mousePosition);
        ManagerToLocal(position);
        manager->NotifyMouseLeave(this, position);
        OnMouseLeave(position);
    }
    manager->NotifyMouseOverChanged(this, 0);
}

// Mac 0xe22a0 (Win inlined)
bool Control::ReleaseMouseCapture()
{
    ControlManager* manager = m_manager;
    if (manager->m_mouseControl == this && manager->m_mouseCaptured) {
        bool mouseInside = m_status.m_mouseInside;
        manager->m_mouseCaptured = false;
        manager->m_inMouseOverUpdate = true;
        OnMouseCaptureLost();
        manager->m_inMouseOverUpdate = false;
        if (!mouseInside || manager->m_mouseOverRequested) {
            manager->m_mouseOverRequested = false;
            manager->UpdateMouseOver();
        }
    }
    return true;
}

// Mac 0xe2310, Win 0x5079bc (ControlManager::SetMouseCapture inlined)
bool Control::CaptureMouse()
{
    return m_manager->SetMouseCapture(this);
}

// Mac 0xe2090, Win 0x508ded
bool Control::SetInputFlags(UInt32 flags)
{
    UInt32 changed = m_inputFlags ^ flags;
    m_inputFlags = flags;
    if ((changed & 1) && m_status.m_connected) {
        ControlManager* manager = m_manager;
        if (!(flags & 1)) {
            // No more mouse input: give up the capture and the mouse.
            ReleaseMouseCapture();
            if (m_manager->m_mouseControl == this) {
                LoseMouse();
                m_manager->m_mouseOverDirty = true;
            }
        } else if (!manager->m_mouseCaptured) {
            manager->m_mouseOverDirty = true;
        }
    }
    if ((changed & 2) && m_status.m_connected && !(m_inputFlags & 2) &&
        m_manager->m_keyboardFocus == this)
        m_manager->SetKeyboardFocus(m_parent, m_parent != 0, false);
    bool result = true;
    if (changed & 4) {
        Invalidate(false);
        if (!(m_inputFlags & 4)) {
            m_buffered = false;
            OnBufferingDisabled();
        } else if (!OnBufferingEnabled()) {
            m_inputFlags &= ~4u;
            m_buffered = false;
            OnBufferingDisabled();
            result = false;
        } else {
            m_buffered = true;
        }
    }
    return result;
}

// Mac 0xe2330, Win 0x508f2c (→ ControlManager::ApplyCursor 0x507864)
void Control::SetCursor(UInt32 cursor, bool flag)
{
    m_cursor = cursor;
    m_flag12c = flag;
    if (m_status.m_connected && m_manager->m_mouseControl == this)
        m_manager->ApplyCursor(cursor, flag);
}

// Mac 0xe2700, Win 0x508fb5
bool Control::SetBuffered(bool buffered)
{
    if (m_buffered != buffered) {
        m_buffered = buffered;
        if (buffered && !(m_inputFlags & 4) && !SetInputFlags(m_inputFlags | 4))
            return false;
        if (!m_manager->m_updateLocked)
            Invalidate(false);
    }
    return true;
}

// Mac 0xdf580, Win 0x508daa
bool Control::SetKeyboardFocus(bool focus)
{
    if (!m_status.m_connected)
        return !focus;
    if ((m_manager->m_keyboardFocus == this) != focus)
        return m_manager->SetKeyboardFocus(focus ? this : 0, false, false);
    return true;
}

// Mac 0xdf5d0, Win 0x507f67
bool Control::GetAugmentation(const String& name, RefPointer<ControlAugmentation>& augmentation) const
{
    BasicMap<String, RefPointer<ControlAugmentation> >::ConstIterator it = m_augmentations.Find(name);
    if (it == m_augmentations.End())
        return false;
    if (augmentation.Get() != it->second.Get())
        augmentation = it->second;
    return true;
}

// ===========================================================================================
// Status propagation
// ===========================================================================================

// Mac 0xdfa70, Win 0x509ce0
void Control::Connect()
{
    m_status.m_connected = 1;
    if (m_buffered)
        m_status.m_invalid = 1;
    OnConnected();
    for (Control* child = m_firstChild; child; child = child->m_nextSibling)
        child->Connect();
}

// Mac 0xdfb80, Win 0x509d14
void Control::Disconnect()
{
    m_status.m_connected = 0;
    if (m_manager->m_keyboardFocus == this)
        m_manager->SetKeyboardFocus(0, false, false);
    OnDisconnected();
    for (Control* child = m_firstChild; child; child = child->m_nextSibling)
        child->Disconnect();
}

// Mac 0xdfad0, Win 0x509f36
bool Control::UpdateActive()
{
    bool active = m_parent && m_parent->m_status.m_activeInTree ? m_status.m_active != 0 : false;
    if (m_status.m_activeInTree == (UInt16)active)
        return true;
    m_status.m_activeInTree = active;
    if (!active) {
        OnDeactivated();
    } else if (!OnActivated()) {
        m_status.m_activeInTree = 0;
        return false;
    }
    for (Control* child = m_firstChild; child; child = child->m_nextSibling)
        if (!child->UpdateActive())
            return false;
    return true;
}

// Mac 0xdfbe0, Win 0x509ebf
void Control::UpdateEnabled()
{
    Control* parent = m_parent;
    bool enabled = parent && parent->m_status.m_enabledInTree ? m_status.m_enabled != 0 : false;
    if (m_status.m_enabledInTree == (UInt16)enabled)
        return;
    m_status.m_enabledInTree = enabled;
    if (!enabled) {
        m_value124 = 0;
        if (m_manager->m_keyboardFocus == this)
            m_manager->SetKeyboardFocus(parent, parent != 0, false);
        OnDisabled();
    } else {
        OnEnabled();
    }
    for (Control* child = m_firstChild; child; child = child->m_nextSibling)
        child->UpdateEnabled();
}

// Mac 0xdfca0, Win 0x509dda
void Control::UpdateVisible()
{
    bool visible = m_parent && m_parent->m_status.m_visible ? m_status.m_shown != 0 : false;
    if (m_status.m_visible == (UInt16)visible)
        return;
    m_status.m_visible = visible;
    if (!visible) {
        ReleaseMouseCapture();
        LoseMouse();
        if (m_manager->m_keyboardFocus == this)
            m_manager->SetKeyboardFocus(m_parent, m_parent != 0, false);
        OnHidden();
    } else {
        OnShown();
    }
    for (Control* child = m_firstChild; child; child = child->m_nextSibling)
        child->UpdateVisible();
}

namespace {

// The control's (scaled) rectangle overlaps its parent's. (inlined everywhere; Win 0x506366)
bool IntersectsParent(const Vector2& parentSize, const Vector2& position, const Vector2& size,
                      const Vector2& scale)
{
    Rect parentRect(parentSize);
    Vector2 scaledSize(size.x * scale.x, size.y * scale.y);
    Rect rect(position, scaledSize);
    rect &= parentRect;
    return rect.IsValid();
}

} // namespace

// Mac 0xdf320, Win 0x508cce
bool Control::SetShown(bool shown)
{
    StatusBitPattern old = m_status;
    if (old.m_shown == (UInt16)shown)
        return true;
    m_status.m_shown = shown;
    if (old.m_connected) {
        UpdateVisible();
        m_manager->m_mouseOverDirty = true;
    }
    if (old.m_visible != m_status.m_visible && m_parent &&
        IntersectsParent(m_parent->m_size, m_position, m_size, m_scale))
        m_parent->Invalidate(false);
    return true;
}

// Mac 0xdf420, Win 0x508d3f
bool Control::SetEnabled(bool enabled)
{
    StatusBitPattern old = m_status;
    if (old.m_enabled != (UInt16)enabled) {
        m_status.m_enabled = enabled;
        if (old.m_connected)
            UpdateEnabled();
    }
    return true;
}

// Mac 0xdf470, Win 0x508d74
bool Control::SetActive(bool active)
{
    StatusBitPattern old = m_status;
    if (old.m_active != (UInt16)active) {
        m_status.m_active = active;
        if (old.m_connected)
            return UpdateActive();
    }
    return true;
}

// ===========================================================================================
// Tree
// ===========================================================================================

void Control::Unlink()
{
    Control* previous = m_previousSibling;
    if (previous)
        previous->m_nextSibling = m_nextSibling;
    else if (m_parent)
        m_parent->m_firstChild = m_nextSibling;
    Control* next = m_nextSibling;
    if (next)
        next->m_previousSibling = previous;
    else if (m_parent)
        m_parent->m_lastChild = previous;
    m_previousSibling = 0;
    m_nextSibling = 0;
}

void Control::Link(Control* after)
{
    Control* parent = m_parent;
    Control* previous;
    Control* next;
    if (!after) {
        previous = parent->m_lastChild;
        next = 0;
    } else if (after == parent) {
        previous = 0;
        next = parent->m_firstChild;
    } else {
        previous = after;
        next = after->m_nextSibling;
    }
    m_previousSibling = previous;
    m_nextSibling = next;
    if (previous)
        previous->m_nextSibling = this;
    else
        parent->m_firstChild = this;
    if (next)
        next->m_previousSibling = this;
    else
        parent->m_lastChild = this;
}

// Mac 0xdd890, Win 0x507fcc
bool Control::SetParent(Control* parent, Control* after)
{
    if (m_status.m_fixedScale)
        return false;
    if (!parent && after)
        parent = after->m_parent;
    Control* oldParent = m_parent;
    if (oldParent == parent) {
        if (!parent)
            return true;
        return DoSetZOrder(after);
    }
    for (Control* control = parent; control; control = control->m_parent)
        if (control == this)
            return false;

    bool wasConnected = false;
    if (oldParent) {
        wasConnected = oldParent->m_status.m_connected != 0;
        if (IntersectsParent(oldParent->m_size, m_position, m_size, m_scale) && m_parent &&
            (m_status.m_visible || !m_parent->m_status.m_visible))
            m_parent->Invalidate(false);
        Unlink();
        m_parent = 0;
    }
    if (parent) {
        m_parent = parent;
        Link(after);
    }

    // The dependents re-resolve their anchors (copy: UpdateAnchors changes the array).
    BasicArray<Control*> dependents(m_controlsFC);
    for (BasicArray<Control*>::Iterator it = dependents.Begin(); it != dependents.End(); ++it)
        (*it)->UpdateAnchors(false);
    for (UInt32 axis = 0; axis < 2; ++axis)
        for (UInt32 i = 0; i < 2; ++i)
            if (m_anchors[axis].m_anchors[i].m_flag)
                m_anchors[axis].m_anchors[i].m_reference = m_parent;
    UpdateAnchors(false);

    if (!m_parent || !m_parent->m_status.m_connected) {
        if (wasConnected) {
            if (m_manager->m_modalControl == this)
                m_manager->SetModalControl(false, this);
            UpdateVisible();
            UpdateEnabled();
            UpdateActive();
            if (m_status.m_connected)
                m_manager->m_mouseOverDirty = true;
            Disconnect();
        }
    } else {
        if (!wasConnected)
            Connect();
        if (!UpdateActive()) {
            Unlink();
            m_parent = 0;
            Disconnect();
            return false;
        }
        UpdateEnabled();
        UpdateVisible();
    }
    if (m_parent && IntersectsParent(m_parent->m_size, m_position, m_size, m_scale) && m_parent &&
        (m_status.m_visible || !m_parent->m_status.m_visible))
        m_parent->Invalidate(false);
    OnDestroyed();
    if (m_status.m_connected)
        m_manager->m_mouseOverDirty = true;
    SetRectangle(m_position, m_size, true);
    return true;
}

// Mac 0xdf6a0, Win 0x508254
bool Control::SetZOrder(Control* after)
{
    if (!m_status.m_fixedScale && m_parent)
        return DoSetZOrder(after);
    return false;
}

// Mac 0xe00d0, Win 0x509bea
bool Control::DoSetZOrder(Control* after)
{
    if (m_parent && IntersectsParent(m_parent->m_size, m_position, m_size, m_scale) &&
        (m_status.m_visible || !m_parent->m_status.m_visible))
        m_parent->Invalidate(false);
    Control* parent = m_parent;
    if (!after) {
        // NOTE: original: no null check of the parent here (SetZOrder / SetParent ensure one).
        after = parent->m_lastChild;
    } else if (after == parent) {
        if (!m_previousSibling)
            return true;
    } else {
        if (after->m_parent != parent)
            return false;
        if (m_previousSibling && m_previousSibling == after)
            return true;
    }
    if (after == this)
        return true;
    Unlink();
    m_parent = parent;
    Link(after);
    if (m_status.m_connected)
        m_manager->m_mouseOverDirty = true;
    OnParentChanged();
    return true;
}

// Mac 0xdf150, Win 0x507eaf
bool Control::Setup(Control* parent, Control* after, UInt32 flags, const Rect& rectangle,
                    UInt32 inputFlags, bool hitTestable)
{
    if (!SetInputFlags(((flags & 8) >> 2) | inputFlags | m_inputFlags))
        return false;
    if (!SetShown((flags & 1) != 0))
        return false;
    if (!SetEnabled(((flags >> 1) & 1) != 0))
        return false;
    if (!SetActive(((flags >> 2) & 1) != 0))
        return false;
    if (!SetRectangle(rectangle))
        return false;
    SetHitTestable(hitTestable);
    if (!SetParent(parent, after))
        return false;
    return SetKeyboardFocus(((flags >> 3) & 1) != 0);
}

// ===========================================================================================
// Rectangle and layout
// ===========================================================================================

// Mac 0xe8600, Win 0x50a65f
Real32 Control::GetAnchorPosition(const InternalAnchorData& anchor, UInt32 axis) const
{
    const Control* reference = anchor.m_reference;
    Real32 value = (Real32)((X(At(reference->m_size, axis)) * anchor.m_values[2] + anchor.m_values[3]) *
                                At(reference->m_scale, axis) +
                            At(reference->m_position, axis));
    for (const Control* control = reference->m_parent; control != anchor.m_commonAncestor;
         control = control->m_parent)
        value = (Real32)(X(value) * At(control->m_scale, axis) + At(control->m_position, axis));
    AncestorToLocal(m_parent, anchor.m_commonAncestor, axis, value);
    return value;
}

// Mac 0xe81f0, Win 0x50a6a9
void Control::LayoutAxis(UInt32 axis, Real32 position, Real32 size)
{
    const InternalAnchorData& nearAnchor = m_anchors[axis].m_anchors[0];
    const InternalAnchorData& farAnchor = m_anchors[axis].m_anchors[1];
    Real32 scale = At(m_scale, axis);
    if (!nearAnchor.m_commonAncestor) {
        if (farAnchor.m_commonAncestor) {
            Real32 local = (Real32)((X(farAnchor.m_values[0]) * size + farAnchor.m_values[1]) * scale);
            Real32 point = GetAnchorPosition(farAnchor, axis);
            position = (Real32)((X(point) - local) - farAnchor.m_values[4]);
        }
    } else if (!farAnchor.m_commonAncestor) {
        Real32 local = (Real32)((X(nearAnchor.m_values[0]) * size + nearAnchor.m_values[1]) * scale);
        Real32 point = GetAnchorPosition(nearAnchor, axis);
        position = (Real32)((X(point) - local) - nearAnchor.m_values[4]);
    } else {
        // Both edges anchored: position and size from the two equations.
        Real32 nearOffset = nearAnchor.m_values[1] * scale;
        Real32 farOffset = farAnchor.m_values[1] * scale;
        Real32 nearPoint = GetAnchorPosition(nearAnchor, axis);
        Real32 farPoint = GetAnchorPosition(farAnchor, axis);
        nearPoint = (Real32)((X(nearPoint) - nearOffset) - nearAnchor.m_values[4]);
        farPoint = (Real32)((X(farPoint) - farOffset) - farAnchor.m_values[4]);
        Real32 scaledSize;
        if (farAnchor.m_values[0] <= nearAnchor.m_values[0]) {
            Real32 denominator = nearAnchor.m_values[0] - farAnchor.m_values[0];
            scaledSize = (Real32)((X(nearPoint) - farPoint) / denominator);
            position = (Real32)(X(farPoint) - X(farAnchor.m_values[0]) * scaledSize);
        } else {
            Real32 denominator = farAnchor.m_values[0] - nearAnchor.m_values[0];
            scaledSize = (Real32)((X(farPoint) - nearPoint) / denominator);
            position = (Real32)(X(nearPoint) - X(nearAnchor.m_values[0]) * scaledSize);
        }
        size = scaledSize / scale;
    }
    At(m_position, axis) = position;
    At(m_size, axis) = size;
    At(m_position, axis) = RoundToNearest(At(m_position, axis));
    Real32 value = At(m_size, axis);
    Real32 minimum = At(m_minimumSize, axis);
    Real32 clamped;
    if (value < minimum) {
        clamped = minimum;
    } else {
        Real32 maximum = At(m_maximumSize, axis);
#if defined(_WIN32)
        // NOTE: MSVC's x87 comparison treats an unordered (NaN) size as "not greater": it stays.
        clamped = value > maximum ? maximum : value;
#else
        clamped = value <= maximum ? value : maximum;
#endif
    }
    At(m_size, axis) = RoundToNearest(clamped);
}

// Mac 0xdfe40, Win 0x50a527
void Control::SetRectangle(const Vector2& position, const Vector2& size, bool relayoutDependents)
{
    Vector2 oldPosition(m_position);
    Vector2 oldSize(m_size);
    bool wasVisible = m_parent ? IntersectsParent(m_parent->m_size, m_position, m_size, m_scale) : false;
    LayoutAxis(0, position.x, size.x);
    LayoutAxis(1, position.y, size.y);
    if (m_status.m_connected)
        m_manager->m_mouseOverDirty = true;
    if (oldPosition != m_position) {
        if (wasVisible ||
            (m_parent && IntersectsParent(m_parent->m_size, m_position, m_size, m_scale))) {
            Control* parent = m_parent;
            if (parent && (m_status.m_visible || !parent->m_status.m_visible))
                parent->Invalidate(false);
        }
    }
    if (oldSize != m_size)
        Invalidate(wasVisible);
    if (oldPosition != m_position || oldSize != m_size) {
        OnRectangleChanged(oldPosition, oldSize);
    } else if (relayoutDependents) {
        for (UInt32 i = 0; i < m_controlsFC.GetSize(); ++i) {
            Control* dependent = m_controlsFC[i];
            dependent->SetRectangle(dependent->m_position, dependent->m_size, false);
        }
    }
}

// Mac 0xdf4c0, Win 0x50831a
bool Control::SetRectangle(const Rect& rectangle)
{
    if (m_status.m_fixedScale)
        return false;
    Rect current(m_position, m_size);
    if (!(rectangle == current))
        SetRectangle(rectangle.GetMin(), rectangle.GetSize(), false);
    return true;
}

// Mac 0xe02d0, Win 0x508292
bool Control::SetPosition(const Vector2& position)
{
    if (m_status.m_fixedScale)
        return false;
    if (!(position == m_position))
        SetRectangle(position, m_size, false);
    return true;
}

// Mac 0xe0320, Win 0x5082d6
bool Control::SetSize(const Vector2& size)
{
    if (m_status.m_fixedScale)
        return false;
    if (!(size == m_size))
        SetRectangle(m_position, size, false);
    return true;
}

// Mac 0xe0370, Win 0x50837f
bool Control::SetScale(const Vector2& scale)
{
    if (m_status.m_fixedScale || !(1e-05f < scale.x) || !(1e-05f < scale.y))
        return false;
    if (scale == m_scale)
        return true;
    Vector2 oldScale(m_scale);
    bool wasVisible = m_parent ? IntersectsParent(m_parent->m_size, m_position, m_size, m_scale) : false;
    m_scale = scale;
    Invalidate(wasVisible);
    if (m_status.m_connected)
        m_manager->m_mouseOverDirty = true;
    OnScaleChanged(oldScale);
    return true;
}

// Mac 0xe04c0, Win 0x508415
bool Control::SetMinimumSize(const Vector2& size)
{
    if (m_status.m_fixedScale || size.x < 0.0f || size.y < 0.0f || size.x > m_maximumSize.x ||
        size.y > m_maximumSize.y)
        return false;
    if (size == m_minimumSize)
        return true;
    m_minimumSize.x = RoundToNearest(size.x);
    m_minimumSize.y = RoundToNearest(size.y);
#if defined(_WIN32)
    // NOTE: MSVC's x87 compare: a NaN size never counts as below the minimum.
    if (!(m_minimumSize.x > m_size.x) && !(m_minimumSize.y > m_size.y))
        return true;
#else
    if (m_minimumSize.x <= m_size.x && m_minimumSize.y <= m_size.y)
        return true;
#endif
    SetRectangle(m_position, m_size, false);
    return true;
}

// Mac 0xe0610, Win 0x5084dd
bool Control::SetMaximumSize(const Vector2& size)
{
    if (m_status.m_fixedScale || size.x < m_minimumSize.x || size.y > 33554432.0f ||
        size.x > 33554432.0f || size.y < m_minimumSize.y)
        return false;
    if (size == m_maximumSize)
        return true;
    m_maximumSize.x = RoundToNearest(size.x);
    m_maximumSize.y = RoundToNearest(size.y);
#if defined(_WIN32)
    // NOTE: MSVC's x87 compare: a NaN size never counts as above the maximum.
    if (!(m_maximumSize.x < m_size.x) && !(m_maximumSize.y < m_size.y))
        return true;
#else
    if (m_size.x <= m_maximumSize.x && m_size.y <= m_maximumSize.y)
        return true;
#endif
    SetRectangle(m_position, m_size, false);
    return true;
}

// Mac 0xe0820, Win 0x5085eb
bool Control::MoveInsideParent()
{
    if (!m_parent)
        return true;
    Vector2 parentSize(m_parent->m_size);
    Vector2 size(m_size);
    if (Rect(m_position, m_size).m_max.x > parentSize.x) {
        Vector2 position(parentSize.x - size.x, m_position.y);
        if (!SetPosition(position))
            return false;
    }
    if (parentSize.y < Rect(m_position, m_size).m_max.y) {
        Vector2 position(m_position.x, parentSize.y - size.y);
        if (!SetPosition(position))
            return false;
    }
    if (Rect(m_position, m_size).m_min.x < 0.0f) {
        Vector2 position(0.0f, m_position.y);
        if (!SetPosition(position))
            return false;
    }
    if (Rect(m_position, m_size).m_min.y < 0.0f) {
        Vector2 position(m_position.x, 0.0f);
        if (!SetPosition(position))
            return false;
    }
    return true;
}

// Win 0x50aa21 (Mac inlined)
bool Control::GetChildrenRect(Rect& rectangle) const
{
    const Control* child = m_firstChild;
    if (!child)
        return false;
    rectangle.Set(Rect(child->m_position, child->m_size));
    for (child = child->m_nextSibling; child; child = child->m_nextSibling)
        rectangle.Union(Rect(child->m_position, child->m_size));
    return true;
}

// Mac 0xe0b10, Win 0x50aa6b
void Control::FitToRect(Rect& rectangle, const Real32* top, const Real32* left,
                        const Real32* bottom, const Real32* right)
{
    rectangle.m_min.x = left ? rectangle.m_min.x - *left : 0.0f;
    rectangle.m_min.y = top ? rectangle.m_min.y - *top : 0.0f;
    rectangle.m_max.x = right ? *right + rectangle.m_max.x : m_size.x;
    rectangle.m_max.y = bottom ? *bottom + rectangle.m_max.y : m_size.y;
    rectangle.m_min.x = RoundToNearest(rectangle.m_min.x);
    rectangle.m_min.y = RoundToNearest(rectangle.m_min.y);
    rectangle.m_max.x = RoundToNearest(rectangle.m_max.x);
    rectangle.m_max.y = RoundToNearest(rectangle.m_max.y);
    if (rectangle.m_min.IsZero()) {
        Vector2 size(rectangle.GetWidth(), rectangle.GetHeight());
        if (!m_status.m_fixedScale && !(size == m_size))
            SetRectangle(m_position, size, false);
        return;
    }
    Vector2 position = m_position + rectangle.m_min;
    Rect target(position, Vector2(rectangle.GetWidth(), rectangle.GetHeight()));
    if (!m_status.m_fixedScale && !(target == Rect(m_position, m_size)))
        SetRectangle(target.GetMin(), target.GetSize(), false);
    // The children keep their place on screen.
    for (Control* child = m_firstChild; child; child = child->m_nextSibling) {
        Vector2 childPosition = child->m_position - rectangle.m_min;
        if (!child->m_status.m_fixedScale && !(childPosition == child->m_position))
            child->SetRectangle(childPosition, child->m_size, false);
    }
}

// Mac 0xe0a40, Win 0x508718
void Control::FitToChildren(bool top, bool left, bool bottom, bool right)
{
    Rect rectangle(0.0f, 0.0f, 0.0f, 0.0f);
    if (!GetChildrenRect(rectangle))
        return;
    Real32 zero = 0.0f;
    FitToRect(rectangle, top ? &zero : 0, left ? &zero : 0, bottom ? &zero : 0, right ? &zero : 0);
}

// Mac 0xe0ed0, Win 0x508796
void Control::GrowToChildren(bool top, bool left, bool bottom, bool right)
{
    Rect rectangle(0.0f, 0.0f, 0.0f, 0.0f);
    if (!GetChildrenRect(rectangle))
        return;
    Real32 zero = 0.0f;
    const Real32* topMargin = top && rectangle.m_min.y < 0.0f ? &zero : 0;
    const Real32* leftMargin = left && rectangle.m_min.x < 0.0f ? &zero : 0;
    const Real32* bottomMargin = bottom && rectangle.m_max.y > m_size.y ? &zero : 0;
    const Real32* rightMargin = right && rectangle.m_max.x > m_size.x ? &zero : 0;
    FitToRect(rectangle, topMargin, leftMargin, bottomMargin, rightMargin);
}

// Mac 0xe0fd0
void Control::FitToChildren(const Real32* top, const Real32* left, const Real32* bottom,
                            const Real32* right)
{
    Rect rectangle(0.0f, 0.0f, 0.0f, 0.0f);
    if (!GetChildrenRect(rectangle))
        return;
    FitToRect(rectangle, top, left, bottom, right);
}

// ===========================================================================================
// Anchors
// ===========================================================================================

// Win 0x5089f3 (Mac: float table at 0x631a58, indices 1-3)
Real32 Control::AlignmentToFraction(UInt32 alignment)
{
    switch (alignment) {
    case 1: return 0.0f;
    case 2: return 0.5f;
    case 3: return 1.0f;
    default: return 0.0f;
    }
}

// Mac 0xe1cc0
InternalAnchorData* Control::GetAnchor(UInt32 axis, bool nearEdge)
{
    return &m_anchors[axis].m_anchors[nearEdge ? 0 : 1];
}

// Win 0x50a9f7 (Mac inlined)
void Control::ApplyAnchorChanges(bool referenceChanged, bool changed)
{
    if (referenceChanged)
        UpdateAnchors(true);
    else if (changed)
        SetRectangle(m_position, m_size, false);
}

// Mac 0xe7ac0 (Win inlined)
void Control::AddDependent(Control* dependent)
{
    for (BasicArray<Control*>::Iterator it = m_controlsFC.Begin(); it != m_controlsFC.End(); ++it)
        if (*it == dependent)
            return;
    m_controlsFC.PushBack(dependent);
    dependent->m_controlsEC.PushBack(this);
}

// Mac 0xdf6c0, Win 0x50a390
void Control::UpdateAnchors(bool layout)
{
    while (!m_controlsEC.IsEmpty()) {
        Control* source = m_controlsEC[0];
        RemoveValue(source->m_controlsFC, this);
        RemoveValue(m_controlsEC, source);
    }
    InternalAnchorData* anchors[4] = {
        &m_anchors[0].m_anchors[0], &m_anchors[0].m_anchors[1],
        &m_anchors[1].m_anchors[0], &m_anchors[1].m_anchors[1],
    };
    for (UInt32 i = 0; i < 4; ++i) {
        InternalAnchorData& anchor = *anchors[i];
        Control* reference = anchor.m_reference;
        if (!reference) {
            anchor.m_commonAncestor = 0;
            continue;
        }
        // The same reference as an earlier anchor: same ancestor, already registered.
        UInt32 j = 0;
        while (j < i && anchors[j]->m_reference != reference)
            ++j;
        if (j < i) {
            anchor.m_commonAncestor = anchors[j]->m_commonAncestor;
            continue;
        }
        // First common ancestor of the reference's parent and this control's parent.
        // NOTE: the mark counter (ControlManager +0xa8) is not initialised and may wrap; a mark of
        // 0 matches every control never marked (m_value68 starts at 0), so a control outside this
        // one's parent chain is taken as the ancestor and the second walk below runs into null
        // (both builds); fixed: 0 is skipped and both walks stop at the root (low).
        UInt32 mark = ++m_manager->m_valueA8;
        if (mark == 0)
            mark = ++m_manager->m_valueA8;
        for (Control* control = m_parent; control; control = control->m_parent)
            control->m_value68 = mark;
        Control* ancestor = reference;
        do {
            ancestor = ancestor->m_parent;
            anchor.m_commonAncestor = ancestor;
        } while (ancestor && ancestor->m_value68 != mark);
        // Every control between both and the ancestor moves this one when it changes.
        for (Control* control = reference; control && control != ancestor; control = control->m_parent)
            control->AddDependent(this);
        for (Control* control = m_parent; control && control != ancestor; control = control->m_parent)
            control->AddDependent(this);
    }
    if (layout)
        SetRectangle(m_position, m_size, false);
}

// Mac 0xe1070 (Win 0x508856)
void Control::SetAxisAnchors(UInt32 axis, Real32 v0, Real32 v1, Real32 v2, Real32 v3, Real32 v4,
                             Control* reference0, Real32 w0, Real32 w1, Real32 w2, Real32 w3,
                             Real32 w4, Control* reference1)
{
    bool referenceChanged = false;
    bool changed = false;
    m_anchors[axis].m_anchors[0].Set(this, v0, v1, v2, v3, v4, reference0, referenceChanged, changed);
    m_anchors[axis].m_anchors[1].Set(this, w0, w1, w2, w3, w4, reference1, referenceChanged, changed);
    ApplyAnchorChanges(referenceChanged, changed);
}

// Mac 0xe1240, Win 0x5088f4
void Control::SetAnchors(Real32 a0, Real32 a1, Real32 a2, Real32 a3, Real32 a4, Control* reference0,
                         Real32 b0, Real32 b1, Real32 b2, Real32 b3, Real32 b4, Control* reference1,
                         Real32 c0, Real32 c1, Real32 c2, Real32 c3, Real32 c4, Control* reference2,
                         Real32 d0, Real32 d1, Real32 d2, Real32 d3, Real32 d4, Control* reference3)
{
    bool referenceChanged = false;
    bool changed = false;
    m_anchors[0].m_anchors[0].Set(this, a0, a1, a2, a3, a4, reference0, referenceChanged, changed);
    m_anchors[0].m_anchors[1].Set(this, b0, b1, b2, b3, b4, reference1, referenceChanged, changed);
    m_anchors[1].m_anchors[0].Set(this, c0, c1, c2, c3, c4, reference2, referenceChanged, changed);
    m_anchors[1].m_anchors[1].Set(this, d0, d1, d2, d3, d4, reference3, referenceChanged, changed);
    ApplyAnchorChanges(referenceChanged, changed);
}

// Mac 0xe1cf0, Win 0x508bde
void Control::SetAnchors(const InternalAnchorData* horizontalNear,
                         const InternalAnchorData* horizontalFar,
                         const InternalAnchorData* verticalNear,
                         const InternalAnchorData* verticalFar)
{
    bool referenceChanged = false;
    bool changed = false;
    m_anchors[0].m_anchors[0].Set(this, horizontalNear, referenceChanged, changed);
    m_anchors[0].m_anchors[1].Set(this, horizontalFar, referenceChanged, changed);
    m_anchors[1].m_anchors[0].Set(this, verticalNear, referenceChanged, changed);
    m_anchors[1].m_anchors[1].Set(this, verticalFar, referenceChanged, changed);
    ApplyAnchorChanges(referenceChanged, changed);
}

// Mac 0xe1540, Win 0x508a09
void Control::SetAxisAlignment(UInt32 axis, UInt32 alignment, UInt32 referenceAlignment,
                               Real32 offset, Control* reference)
{
    bool referenceChanged = false;
    bool changed = false;
    m_anchors[axis].m_anchors[0].SetAligned(this, alignment & 3, referenceAlignment & 3, offset,
                                            reference, referenceChanged, changed);
    m_anchors[axis].m_anchors[1].Clear(referenceChanged);
    ApplyAnchorChanges(referenceChanged, changed);
}

// Mac 0xe16a0, Win 0x508a4e
void Control::SetAxisAlignments(UInt32 axis, UInt32 alignment0, UInt32 referenceAlignment0,
                                Real32 offset0, Control* reference0, UInt32 alignment1,
                                UInt32 referenceAlignment1, Real32 offset1, Control* reference1)
{
    bool referenceChanged = false;
    bool changed = false;
    m_anchors[axis].m_anchors[0].SetAligned(this, alignment0 & 3, referenceAlignment0 & 3, offset0,
                                            reference0, referenceChanged, changed);
    m_anchors[axis].m_anchors[1].SetAligned(this, alignment1 & 3, referenceAlignment1 & 3, offset1,
                                            reference1, referenceChanged, changed);
    ApplyAnchorChanges(referenceChanged, changed);
}

// Mac 0xe18d0, Win 0x508ac3
void Control::SetAlignment(UInt32 alignment, UInt32 referenceAlignment, Real32 offsetX,
                           Real32 offsetY, Control* reference)
{
    bool referenceChanged = false;
    bool changed = false;
    m_anchors[0].m_anchors[0].SetAligned(this, alignment & 3, referenceAlignment & 3, offsetX,
                                         reference, referenceChanged, changed);
    m_anchors[0].m_anchors[1].Clear(referenceChanged);
    m_anchors[1].m_anchors[0].SetAligned(this, (alignment >> 4) & 3, (referenceAlignment >> 4) & 3,
                                         offsetY, reference, referenceChanged, changed);
    m_anchors[1].m_anchors[1].Clear(referenceChanged);
    ApplyAnchorChanges(referenceChanged, changed);
}

// Mac 0xe1b30, Win 0x508b28
void Control::SetMargins(Real32 left, Real32 right, Real32 top, Real32 bottom, Control* reference)
{
    SetAnchors(0.0f, 0.0f, 0.0f, 0.0f, left, reference,
               1.0f, 0.0f, 1.0f, 0.0f, right, reference,
               0.0f, 0.0f, 0.0f, 0.0f, top, reference,
               1.0f, 0.0f, 1.0f, 0.0f, bottom, reference);
}

// Mac 0xe1c10, Win 0x508bb3
void Control::SetAxisMargins(UInt32 axis, Real32 nearMargin, Real32 farMargin, Control* reference)
{
    SetAxisAlignments(axis, 1, 1, nearMargin, reference, 3, 3, farMargin, reference);
}

// ===========================================================================================
// Drawing
// ===========================================================================================

namespace {

// The part of `args.clip` covered by `child` at `offset` / `scale`. (inlined in both walks)
Rect ChildClip(const ControlDrawArgs& args, const Vector2& size, const Vector2& offset,
               const Vector2& scale)
{
    Rect clip(args.m_clip);
    Vector2 scaledSize(size.x * scale.x, size.y * scale.y);
    clip &= Rect(offset, scaledSize);
    return clip;
}

} // namespace

// Mac 0xe2420
void Control::DrawTree(ControlDrawArgs& args)
{
    if (m_buffered && !m_status.m_invalid)
        return;
    for (Control* child = m_firstChild; child; child = child->m_nextSibling) {
        if (!child->m_status.m_visible)
            continue;
        Vector2 offset(child->m_position.x * args.m_scale.x + args.m_offset.x,
                       child->m_position.y * args.m_scale.y + args.m_offset.y);
        Vector2 scale(child->m_scale.x * args.m_scale.x, child->m_scale.y * args.m_scale.y);
        Rect clip = ChildClip(args, child->m_size, offset, scale);
        if (!clip.IsValid())
            continue;
        Vector2 savedOffset(args.m_offset);
        Vector2 savedScale(args.m_scale);
        Rect savedClip(args.m_clip);
        args.m_offset = offset;
        args.m_scale = scale;
        args.m_clip.Set(clip);
        child->DrawTree(args);
        args.m_offset = savedOffset;
        args.m_scale = savedScale;
        args.m_clip.Set(savedClip);
    }
    // A buffered control redraws itself and its unbuffered descendants into its buffer.
    if (m_buffered && 1.0f <= m_size.x && 1.0f <= m_size.y) {
        args.m_offset = Vector2(0.0f, 0.0f);
        args.m_scale = Vector2(1.0f, 1.0f);
        args.m_clip.Set(Rect(m_size));
        args.m_size = m_size;
        BeginDraw(args);
        Draw(args);
        DrawChildren(args);
    }
}

// Mac 0xe7880
void Control::DrawChildren(ControlDrawArgs& args)
{
    for (Control* child = m_firstChild; child; child = child->m_nextSibling) {
        if (!child->m_status.m_visible)
            continue;
        Vector2 offset(child->m_position.x * args.m_scale.x + args.m_offset.x,
                       child->m_position.y * args.m_scale.y + args.m_offset.y);
        Vector2 scale(child->m_scale.x * args.m_scale.x, child->m_scale.y * args.m_scale.y);
        Rect clip = ChildClip(args, child->m_size, offset, scale);
        if (!clip.IsValid()) {
            // Out of sight: nothing to redraw.
            if (!child->m_buffered)
                child->m_status.m_invalid = 0;
            continue;
        }
        Vector2 savedOffset(args.m_offset);
        Vector2 savedScale(args.m_scale);
        Rect savedClip(args.m_clip);
        args.m_offset = offset;
        args.m_scale = scale;
        args.m_clip.Set(clip);
        // The child's size becomes the target size before it draws (Mac
        // 0xe7a16: `args+0x20 = &child->m_size`; not restored afterwards) — the 3D viewport
        // of ControlDrawArgs::Setup(true) spans it (BrickConnectSceneControl).
        args.m_size = child->m_size;
        if (!child->m_buffered) {
            child->Draw(args);
            child->DrawChildren(args);
        } else {
            child->EndDraw(args);
        }
        args.m_offset = savedOffset;
        args.m_scale = savedScale;
        args.m_clip.Set(savedClip);
    }
}

// Mac 0x4aec0, Win 0x455e00 (inline in the original header; emitted elsewhere)
void Control::Draw(ControlDrawArgs& /*args*/)
{
    m_status.m_invalid = 0;
}

// Mac 0x510d0, Win 0x50fca7 (empty; ICF)
void Control::EndDraw(ControlDrawArgs& /*args*/)
{
}

// Mac 0x510e0, Win 0x50fca7 (empty; ICF)
void Control::BeginDraw(ControlDrawArgs& /*args*/)
{
}

// ===========================================================================================
// Notifications: each default tells the observers (NotifierT iteration; observers added during
// the pass are not called, removed ones are skipped). Mac 0xe2770-0xe6d50, Win 0x508ff2-0x5098c0
// ===========================================================================================

#define LEGO_CONTROL_NOTIFY(call)                                                   \
    NotifierT<Control, ControlObserver>::Iterator it(*this);                        \
    while (ControlObserver* observer = it.Next(m_observers))                        \
        observer->call

void Control::OnConnected() { LEGO_CONTROL_NOTIFY(OnConnected(this)); }                       // Mac 0xe2770, Win 0x508ff2
void Control::OnDisconnected() { LEGO_CONTROL_NOTIFY(OnDisconnected(this)); }                 // Mac 0xe29e0, Win 0x50903d
void Control::OnDestroyed() { LEGO_CONTROL_NOTIFY(OnDestroyed(this)); }                       // Mac 0xe2c50, Win 0x509088
void Control::OnParentChanged() { LEGO_CONTROL_NOTIFY(OnParentChanged(this)); }               // Mac 0xe2ec0, Win 0x5090d3

// Mac 0xe3130, Win 0x50911e: then the dependents follow (Win 0x50a500).
void Control::OnRectangleChanged(const Vector2& oldPosition, const Vector2& oldSize)
{
    {
        LEGO_CONTROL_NOTIFY(OnRectangleChanged(this, oldPosition, oldSize));
    }
    for (UInt32 i = 0; i < m_controlsFC.GetSize(); ++i) {
        Control* dependent = m_controlsFC[i];
        dependent->SetRectangle(dependent->m_position, dependent->m_size, false);
    }
}

// Mac 0xe33f0, Win 0x509178: then a layout pass that also moves the dependents.
void Control::OnScaleChanged(const Vector2& oldScale)
{
    {
        LEGO_CONTROL_NOTIFY(OnScaleChanged(this, oldScale));
    }
    SetRectangle(m_position, m_size, true);
}

void Control::OnShown() { LEGO_CONTROL_NOTIFY(OnShown(this)); }                               // Mac 0xe3690, Win 0x5091d9
void Control::OnHidden() { LEGO_CONTROL_NOTIFY(OnHidden(this)); }                             // Mac 0xe3900, Win 0x509224
void Control::OnEnabled() { LEGO_CONTROL_NOTIFY(OnEnabled(this)); }                           // Mac 0xe3b70, Win 0x50926f
void Control::OnDisabled() { LEGO_CONTROL_NOTIFY(OnDisabled(this)); }                         // Mac 0xe3de0, Win 0x5092ba

// Mac 0xe4050, Win 0x509305: the first observer that refuses ends the pass.
bool Control::OnActivated()
{
    NotifierT<Control, ControlObserver>::Iterator it(*this);
    bool result = true;
    while (result) {
        ControlObserver* observer = it.Next(m_observers);
        if (!observer)
            break;
        result = observer->OnActivated(this);
    }
    return result;
}

void Control::OnDeactivated() { LEGO_CONTROL_NOTIFY(OnDeactivated(this)); }                   // Mac 0xe42c0, Win 0x509356
void Control::OnMouseEnter(const Vector2& position) { LEGO_CONTROL_NOTIFY(OnMouseEnter(this, position)); } // Mac 0xe4530, Win 0x5093a1
void Control::OnMouseLeave(const Vector2& position) { LEGO_CONTROL_NOTIFY(OnMouseLeave(this, position)); } // Mac 0xe47b0, Win 0x5093f1
void Control::OnMouseMove(const Vector2& position) { LEGO_CONTROL_NOTIFY(OnMouseMove(this, position)); }   // Mac 0xe4a30, Win 0x509441

// Mac 0xe4cb0, Win 0x509491
void Control::OnMousePressed(const Vector2& position, UInt32 button)
{
    LEGO_CONTROL_NOTIFY(OnMousePressed(this, position, button));
}

// Mac 0xe4f30, Win 0x5094e4
void Control::OnMouseReleased(const Vector2& position, UInt32 button)
{
    LEGO_CONTROL_NOTIFY(OnMouseReleased(this, position, button));
}

// Mac 0xe51b0, Win 0x509537
void Control::OnMouseClicked(const Vector2& position, UInt32 button)
{
    LEGO_CONTROL_NOTIFY(OnMouseClicked(this, position, button));
}

// Mac 0xe5430, Win 0x50958a
void Control::OnMouseDoubleClicked(const Vector2& position, UInt32 button)
{
    LEGO_CONTROL_NOTIFY(OnMouseDoubleClicked(this, position, button));
}

// Mac 0xe56b0, Win 0x5095dd
void Control::OnMouseWheel(UInt32 axis, Real32 delta)
{
    LEGO_CONTROL_NOTIFY(OnMouseWheel(this, axis, delta));
}

void Control::OnMouseCaptured() { LEGO_CONTROL_NOTIFY(OnMouseCaptured(this)); }               // Mac 0xe5940, Win 0x509634
void Control::OnMouseCaptureLost() { LEGO_CONTROL_NOTIFY(OnMouseCaptureLost(this)); }         // Mac 0xe5bb0, Win 0x50967f

// Mac 0xe5e20, Win 0x5096ca
void Control::OnKeyboardFocusGained(bool flag, Control* previous, UInt32 modifiers)
{
    LEGO_CONTROL_NOTIFY(OnKeyboardFocusGained(this, flag, previous, modifiers));
}

// Mac 0xe60b0, Win 0x509720
void Control::OnKeyboardFocusLost(Control* next)
{
    LEGO_CONTROL_NOTIFY(OnKeyboardFocusLost(this, next));
}

// Mac 0xe6330, Win 0x509770
void Control::OnKeyPressed(UInt32 key, UInt32 modifiers, bool repeat)
{
    LEGO_CONTROL_NOTIFY(OnKeyPressed(this, key, modifiers, repeat));
}

// Mac 0xe65c0, Win 0x5097c6
void Control::OnKeyReleased(UInt32 key, UInt32 modifiers)
{
    LEGO_CONTROL_NOTIFY(OnKeyReleased(this, key, modifiers));
}

// Mac 0xe6840, Win 0x509819
void Control::OnChar(UniChar16 c, UInt32 modifiers, bool repeated)
{
    LEGO_CONTROL_NOTIFY(OnChar(this, c, modifiers, repeated));
}

// Mac 0xe6ad0, Win 0x50986f: the first observer that refuses ends the pass.
bool Control::OnBufferingEnabled()
{
    NotifierT<Control, ControlObserver>::Iterator it(*this);
    bool result = true;
    while (result) {
        ControlObserver* observer = it.Next(m_observers);
        if (!observer)
            break;
        result = observer->OnBufferingEnabled(this);
    }
    return result;
}

void Control::OnBufferingDisabled() { LEGO_CONTROL_NOTIFY(OnBufferingDisabled(this)); }       // Mac 0xe6d50, Win 0x5098c0

#undef LEGO_CONTROL_NOTIFY

} // namespace LEGO
