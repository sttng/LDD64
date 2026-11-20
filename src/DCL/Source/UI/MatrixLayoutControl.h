// LEGO Digital Designer — restored source
// LEGO::MatrixLayoutControl — lays out its controls in a grid of equal cells (cell size +
// spacing), row by row, optionally mirrored; the column count follows the width (own, or
// computed from the controls: fit mode). Lua: LuaMatrixLayoutControl (TU 316).
// TU 276 "MatrixLayoutControl.cpp" (Mac 0x28ec20-0x28fb7b; Win 0x5179e5-0x5180ef + 0x664cfe).
// Names inferred, except FromString's keywords.
//
// Layout (Mac): Control +0x000..+0x158, members below, Object +0x17c (sizeof 0x184).
//   Win: cell size +0x16c, controls +0x174 (begin +0x178), fit mode +0x184, inLayout +0x188,
//   leftToRight +0x189, topToBottom +0x18a, spacing +0x18c, Object +0x190.
// Virtual slots: Mac 0x70556c (36, Control's) overriding OnRectangleChanged (8 / Win 5).
#pragma once
#include "LEGO/Types.h"
#include "Control.h"
#include "BasicArray.h"

namespace LEGO {

class String;

class MatrixLayoutControl : public Control {
public:
    // Which extent the layout computes (names from the keywords; values 0-3 accepted too).
    // FitHeight: width = the control's, height follows the rows. FitColumn: all controls in one row
    // ("column" keyword). FitRow: width = one cell. FitNone: no resize.
    enum FitMode {
        FitHeight = 0,   // "height", "0"
        FitColumn = 1,   // "column", "1"
        FitRow    = 2,   // "row", "2"
        FitNone   = 3    // "none", "3"
    };

    explicit MatrixLayoutControl(ControlManager* manager);   // Mac 0x28ecf0 (C1) / 0x28ec20 (C2), Win 0x517c20 (dtor body)
    virtual ~MatrixLayoutControl();                           // Mac 0x28f610/0x28f6e0, Win sdd 0x5179e5

    const Vector2& GetCellSize() const { return m_cellSize; }  // Mac 0x28ede0, Win 0x517a15
    void SetCellSize(const Vector2& size);                      // Mac 0x28edf0, Win 0x517a1c
    FitMode GetFitMode() const { return (FitMode)m_fitMode; }
    void SetFitMode(FitMode mode);                              // Mac 0x28f410
    bool IsLeftToRight() const { return m_leftToRight; }
    void SetLeftToRight(bool leftToRight);                      // Mac 0x28f430
    bool IsTopToBottom() const { return m_topToBottom; }
    void SetTopToBottom(bool topToBottom);                      // Mac 0x28f450
    Real32 GetSpacing() const { return m_spacing; }             // Mac 0x28f470
    void SetSpacing(Real32 spacing);                            // Mac 0x28f480, Win 0x517bba (rounded)

    void AddControl(Control* control);                          // Mac 0x28f120, Win 0x517a4c
    void RemoveControl(Control* control);                       // Mac 0x28f1d0, Win 0x517acb
    void RemoveAllControls();                                   // Mac 0x28f360, Win 0x517b75
    UInt32 GetControlCount() const { return m_controls.GetSize(); }  // Mac 0x28f3f0
    typedef BasicArray<RefPointer<Control> >::ConstIterator ControlIterator;
    ControlIterator BeginControls() const { return m_controls.Begin(); }   // Mac 0x28f520, Win 0x517c02
    ControlIterator EndControls() const { return m_controls.End(); }       // Mac 0x28f530, Win 0x517c11

    // Mac 0x28ee30, Win 0x517c87
    void Layout();

protected:
    // Lays out first (unless the layout itself moved the control). Mac 0x28f7d0, Win 0x517eae
    virtual void OnRectangleChanged(const Vector2& oldPosition, const Vector2& oldSize);

    // (additive) The laid-out controls, filled directly (no parent change, no
    // layout) by BrickPaletteControl::Refresh (Mac 0x69670 clears and appends inline).
    BasicArray<RefPointer<Control> >& GetControlArray() { return m_controls; }

private:
    // ---- data (Mac / Win offsets) ----
    Vector2                          m_cellSize;     // +0x158 / +0x16c (0, 0)
    BasicArray<RefPointer<Control> > m_controls;     // +0x160 / +0x174
    UInt32                           m_fitMode;      // +0x170 / +0x184 FitHeight
    bool                             m_inLayout;     // +0x174 / +0x188
    bool                             m_leftToRight;  // +0x175 / +0x189 true
    bool                             m_topToBottom;  // +0x176 / +0x18a true
    Real32                           m_spacing;      // +0x178 / +0x18c 0
                                                     // Object +0x17c / +0x190
};

// Keyword or digit at `position` ("height" / "column" / "row" / "none" or "0".."3");
// false (and FitHeight) otherwise. Mac 0x28f800, Win 0x517ed2 (name inferred)
bool FromString(MatrixLayoutControl::FitMode& value, const String& text, UInt32& position);

} // namespace LEGO
