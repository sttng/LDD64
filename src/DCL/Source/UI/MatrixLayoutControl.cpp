// LEGO Digital Designer — restored source
// TU 276 "MatrixLayoutControl.cpp" (Mac 0x28ec20-0x28fb7b, Win 0x5179e5-0x5180ef).
// Arithmetic: `Intermediate` as in ArrayLayoutControl.cpp.
#include "MatrixLayoutControl.h"
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

// Win 0x4c9630 LEGO::RoundToNearest (inlined on the Mac).
inline Real32 RoundToNearest(Real32 value)
{
    Real32 lower = (Real32)floor((double)value);
    Real32 upper = (Real32)ceil((double)value);
    return value - lower < upper - value ? lower : upper;
}

// (Real32) of an unsigned count (Mac: 0x43300000 bias; Win: fild + 2^32 when negative).
inline Real32 FromUnsigned(UInt32 value) { return (Real32)(double)value; }

} // namespace

// Mac 0x28ecf0 (C1), 0x28ec20 (C2)
MatrixLayoutControl::MatrixLayoutControl(ControlManager* manager)
    : Control(manager), m_fitMode(FitHeight), m_inLayout(false), m_leftToRight(true),
      m_topToBottom(true), m_spacing(0.0f)
{
}

// Mac 0x28f610 (D1), 0x28f540 (D2 body), 0x28f6e0 (D0); Win 0x517c20, sdd 0x5179e5
MatrixLayoutControl::~MatrixLayoutControl()
{
}

// Mac 0x28edf0, Win 0x517a1c
void MatrixLayoutControl::SetCellSize(const Vector2& size)
{
    if (m_cellSize != size) {
        m_cellSize = size;
        Layout();
    }
}

// Mac 0x28f410 (no change test)
void MatrixLayoutControl::SetFitMode(FitMode mode)
{
    m_fitMode = mode;
    Layout();
}

// Mac 0x28f430
void MatrixLayoutControl::SetLeftToRight(bool leftToRight)
{
    m_leftToRight = leftToRight;
    Layout();
}

// Mac 0x28f450
void MatrixLayoutControl::SetTopToBottom(bool topToBottom)
{
    m_topToBottom = topToBottom;
    Layout();
}

// Mac 0x28f480, Win 0x517bba
void MatrixLayoutControl::SetSpacing(Real32 spacing)
{
    Real32 rounded = RoundToNearest(spacing);
    if (m_spacing == rounded)
        return;
    m_spacing = rounded;
    Layout();
}

// Mac 0x28f120, Win 0x517a4c
void MatrixLayoutControl::AddControl(Control* control)
{
    control->SetParent(this, 0);
    m_controls.PushBack(RefPointer<Control>(control));
    Layout();
}

// Mac 0x28f1d0, Win 0x517acb
void MatrixLayoutControl::RemoveControl(Control* control)
{
    UInt32 index = 0;
    while (index < m_controls.GetSize() && m_controls[index] != control)
        ++index;
    if (index == m_controls.GetSize())
        return;
    if (control->GetParent() == this)
        control->SetParent(0, 0);
    m_controls.Erase(index);
    Layout();
}

// Mac 0x28f360, Win 0x517b75
void MatrixLayoutControl::RemoveAllControls()
{
    for (UInt32 i = 0; i < m_controls.GetSize(); ++i) {
        Control* control = m_controls[i];
        if (control->GetParent() == this)
            control->SetParent(0, 0);
    }
    m_controls.Clear();
    Layout();
}

// Mac 0x28ee30, Win 0x517c87
void MatrixLayoutControl::Layout()
{
    m_inLayout = true;
    UInt32 count = m_controls.GetSize();
    Real32 width;
    if (m_fitMode == FitColumn)
        width = (Real32)(X(FromUnsigned(count - 1)) * X(m_spacing) + X(FromUnsigned(count)) * X(m_cellSize.x));
    else if (m_fitMode == FitRow)
        width = (Real32)(X(FromUnsigned(count - 1)) * X(m_spacing) + X(m_cellSize.x));
    else
        width = m_size.x;
    Vector2 step = m_cellSize + m_spacing;
    Vector2 position(0.0f, 0.0f);
    Vector2 size(0.0f, 0.0f);
    Int32 columns = (Int32)((X(width) + X(m_spacing)) / (X(m_cellSize.x) + X(m_spacing)));
    if (columns <= 0)
        columns = 1;
    UInt32 rows = (count - 1 + (UInt32)columns) / (UInt32)columns;
    size.x = width;
    size.y = (Real32)(X((Real32)(Int32)(rows - 1)) * X(m_spacing) +
                      X((Real32)(Int32)rows) * X(m_cellSize.y));
    if (m_fitMode != FitNone)
        SetSize(size);
    size = m_size;
    Real32 startX = 0.0f;
    if (!m_leftToRight) {
        position.x = size.x - m_cellSize.x;
        step.x = -step.x;
        startX = position.x;
    }
    if (!m_topToBottom) {
        position.y = size.y - m_cellSize.y;
        step.y = -step.y;
    }
    Int32 column = 0;
    for (UInt32 i = 0; i < m_controls.GetSize(); ++i) {
        m_controls[i]->SetRectangle(Rect(position, m_cellSize));
        position.x = step.x + position.x;
        ++column;
        if (columns <= column) {
            position.x = startX;
            position.y = step.y + position.y;
            column = 0;
        }
    }
    m_inLayout = false;
}

// Mac 0x28f7d0, Win 0x517eae
void MatrixLayoutControl::OnRectangleChanged(const Vector2& oldPosition, const Vector2& oldSize)
{
    if (!m_inLayout)
        Layout();
    Control::OnRectangleChanged(oldPosition, oldSize);
}

// Mac 0x28f800, Win 0x517ed2
bool FromString(MatrixLayoutControl::FitMode& value, const String& text, UInt32& position)
{
    static const struct {
        const char* keyword;
        MatrixLayoutControl::FitMode mode;
    } kKeywords[] = {
        {"height", MatrixLayoutControl::FitHeight}, {"column", MatrixLayoutControl::FitColumn},
        {"row", MatrixLayoutControl::FitRow},       {"none", MatrixLayoutControl::FitNone},
        {"0", MatrixLayoutControl::FitHeight},      {"1", MatrixLayoutControl::FitColumn},
        {"2", MatrixLayoutControl::FitRow},         {"3", MatrixLayoutControl::FitNone},
    };
    for (UInt32 i = 0; i < sizeof(kKeywords) / sizeof(kKeywords[0]); ++i) {
        if (ParseKeyword(String(kKeywords[i].keyword), text, position, ',')) {
            value = kKeywords[i].mode;
            return true;
        }
    }
    value = MatrixLayoutControl::FitHeight;
    return false;
}

} // namespace LEGO
