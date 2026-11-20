// LEGO Digital Designer — restored source
// TU 343 "ArrayLayoutControl.cpp" (Mac 0x326990-0x32831b, Win 0x516e06-0x517601).
// Arithmetic: Mac rounds every float operation (SSE); Win (x87, 53-bit precision) keeps an
// expression's intermediates and rounds on store. `Intermediate` reproduces both (as in Control.cpp).
#include "ArrayLayoutControl.h"
#include "ControlManager.h"
#include "LEGO/String.h"
#include "StringStream.h"
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

inline Real32& At(Vector2& v, UInt32 axis) { return (&v.x)[axis]; }
inline Real32 At(const Vector2& v, UInt32 axis) { return (&v.x)[axis]; }

// The extent of `control` along `axis` in its parent's units, rounded.
inline Real32 ScaledExtent(const Control* control, UInt32 axis)
{
    Real32 product = At(control->GetSize(), axis) * At(control->GetScale(), axis);
    return RoundToNearest(product);
}

} // namespace

// Mac 0x326aa0 (C1), 0x326990 (C2); Win 0x516d1e
ArrayLayoutControl::ArrayLayoutControl(ControlManager* manager)
    : Control(manager), m_horizontal(true), m_reversed(false), m_inLayout(false),
      m_resizeMode(ResizeSingle), m_spacing(0.0f), m_leadingMargin(0.0f), m_trailingMargin(0.0f)
{
}

// Mac 0x3278a0 (D1), 0x327750 (D2 body), 0x3279e0 (D0); Win 0x517129, sdd 0x516e06
ArrayLayoutControl::~ArrayLayoutControl()
{
}

// Mac 0x326bd0, Win 0x516e36
void ArrayLayoutControl::SetHorizontal(bool horizontal)
{
    if (m_horizontal == horizontal)
        return;
    m_horizontal = horizontal;
    Layout();
}

// Mac 0x3270d0, Win 0x516e52
void ArrayLayoutControl::SetResizeMode(ResizeMode mode)
{
    if (m_resizeMode == (UInt32)mode)
        return;
    m_resizeMode = mode;
    Layout();
}

// Mac 0x327100 (Win inlined, e.g. TextOutputControl 0x514108)
void ArrayLayoutControl::SetReversed(bool reversed)
{
    if (m_reversed == reversed)
        return;
    m_reversed = reversed;
    Layout();
}

// Mac 0x327140, Win 0x516e6e
void ArrayLayoutControl::SetSpacing(Real32 spacing)
{
    Real32 rounded = RoundToNearest(spacing);
    if (m_spacing == rounded)
        return;
    m_spacing = rounded;
    Layout();
}

// Mac 0x3271e0, Win 0x516eb4
void ArrayLayoutControl::SetLeadingMargin(Real32 margin)
{
    Real32 rounded = RoundToNearest(margin);
    if (m_leadingMargin == rounded)
        return;
    m_leadingMargin = rounded;
    Layout();
}

// Mac 0x327280, Win 0x516ef9
void ArrayLayoutControl::SetTrailingMargin(Real32 margin)
{
    Real32 rounded = RoundToNearest(margin);
    if (m_trailingMargin == rounded)
        return;
    m_trailingMargin = rounded;
    if (m_resizeMode != ResizeOff)
        Layout();
}

// Mac 0x327320, Win 0x516f47
void ArrayLayoutControl::AddControl(Control* control)
{
    control->SetParent(this, 0);
    m_controls.PushBack(RefPointer<Control>(control));
    ControlObserver::Attach(control);
    Layout();
}

// Mac 0x3273e0, Win 0x516fd2
void ArrayLayoutControl::InsertControl(Control* control, UInt32 index)
{
    control->SetParent(this, 0);
    UInt32 count = m_controls.GetSize();
    if (index > count)
        index = count;
    m_controls.Insert(m_controls.Begin() + index, 1, RefPointer<Control>(control));  // Mac 0x233ac0
    ControlObserver::Attach(control);
    Layout();
}

// Mac 0x3274d0, Win 0x517052
void ArrayLayoutControl::RemoveControl(Control* control)
{
    UInt32 index = 0;
    while (index < m_controls.GetSize() && m_controls[index] != control)
        ++index;
    if (index == m_controls.GetSize())
        return;
    ControlObserver::Detach(control);
    if (control->GetParent() == this)
        control->SetParent(0, 0);
    m_controls.Erase(index);
    Layout();
}

// Mac 0x327670, Win 0x5170ce
void ArrayLayoutControl::RemoveAllControls()
{
    for (UInt32 i = 0; i < m_controls.GetSize(); ++i) {
        Control* control = m_controls[i];
        ControlObserver::Detach(control);
        if (control->GetParent() == this)
            control->SetParent(0, 0);
    }
    m_controls.Clear();
    Layout();
}

// Mac 0x326c00, Win 0x5171ba
void ArrayLayoutControl::Layout()
{
    m_inLayout = true;
    Vector2 position(0.0f, 0.0f);
    Vector2 size(m_size);
    UInt32 cross = m_horizontal ? 1 : 0;
    UInt32 main = cross ^ 1;
    if (m_resizeMode != ResizeOff) {
        At(size, main) = m_leadingMargin + m_trailingMargin;
        if (m_resizeMode == ResizeBoth)
            At(size, cross) = 0.0f;
        Int32 count = 0;
        for (UInt32 i = 0; i < m_controls.GetSize(); ++i) {
            const Control* control = m_controls[i];
            if (!control->GetStatus().m_visible)
                continue;
            At(size, main) = ScaledExtent(control, main) + At(size, main);
            if (m_resizeMode == ResizeBoth) {
                Real32 extent = ScaledExtent(control, cross);
                if (extent <= At(size, cross))
                    extent = At(size, cross);
                At(size, cross) = extent;
            }
            ++count;
        }
        At(size, main) = (Real32)(X((Real32)(count - 1)) * X(m_spacing) + X(At(size, main)));
        SetSize(size);
    }
    if (!m_reversed)
        At(position, main) = m_leadingMargin;
    else
        At(position, main) = (Real32)(X(At(size, main)) - X(m_leadingMargin) + X(m_spacing));
    for (UInt32 i = 0; i < m_controls.GetSize(); ++i) {
        Control* control = m_controls[i];
        if (!control->GetStatus().m_visible)
            continue;
        if (m_reversed)
            At(position, main) = (Real32)(X(At(position, main)) -
                                          (X(ScaledExtent(control, main)) + X(m_spacing)));
        control->SetPosition(position);
        if (!m_reversed)
            At(position, main) = (Real32)(X(ScaledExtent(control, main)) + X(m_spacing) +
                                          X(At(position, main)));
    }
    m_inLayout = false;
}

// Mac 0x327b50 (thunks 0x327bc0, 0x327c30), Win 0x5171a6
void ArrayLayoutControl::OnFinalRelease()
{
    ControlObserver::OnFinalRelease();
    Control::OnFinalRelease();
}

bool ArrayLayoutControl::Contains(Control* control) const
{
    for (ControlIterator it = m_controls.Begin(); it != m_controls.End(); ++it)
        if (*it == control)
            return true;
    return false;
}

// Mac 0x327cb0 (thunk 0x327db0), Win 0x5173c1
void ArrayLayoutControl::OnRectangleChanged(Control* control, const Vector2&, const Vector2&)
{
    if (m_inLayout)
        return;
    if (Contains(control))
        Layout();
}

// Mac 0x327de0 (thunk 0x327ee0), Win 0x517439
void ArrayLayoutControl::OnScaleChanged(Control* control, const Vector2&)
{
    if (m_inLayout)
        return;
    if (Contains(control))
        Layout();
}

// Mac 0x327f10 (thunk 0x328010), Win 0x5174b1
void ArrayLayoutControl::OnShown(Control* control)
{
    if (m_inLayout)
        return;
    if (Contains(control))
        Layout();
}

// Mac 0x328030 (thunk 0x328130), Win 0x5174b1 (folded with OnShown)
void ArrayLayoutControl::OnHidden(Control* control)
{
    if (m_inLayout)
        return;
    if (Contains(control))
        Layout();
}

// Mac 0x328150, Win 0x517529
bool FromString(ArrayLayoutControl::ResizeMode& value, const String& text, UInt32& position)
{
    if (ParseKeyword(String("off"), text, position, ',')) {
        value = ArrayLayoutControl::ResizeOff;
        return true;
    }
    if (ParseKeyword(String("single"), text, position, ',')) {
        value = ArrayLayoutControl::ResizeSingle;
        return true;
    }
    if (ParseKeyword(String("both"), text, position, ',')) {
        value = ArrayLayoutControl::ResizeBoth;
        return true;
    }
    value = ArrayLayoutControl::ResizeSingle;
    return false;
}

} // namespace LEGO
