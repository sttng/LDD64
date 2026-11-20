// LEGO Digital Designer — restored source
// LEGO::TextOutputControl — a growing column of text lines (one transparent RenderControl each,
// wrapped at a fixed width); the oldest are dropped once taller than the maximum height. The
// output pane of LuaConsole (TU 320).
// TU 326 "TextOutputControl.cpp" (Mac 0x2daae0-0x2db09b; Win 0x514025-0x514297). Names inferred.
//
// Layout (Mac): ArrayLayoutControl +0x000..+0x190, members below, Object +0x1a0 (sizeof 0x1a8).
//   Win: manager +0x1a8, font +0x1ac, width +0x1b0, max height +0x1b4, Object +0x1b8.
// Virtual slots: Mac 0x74a53c = ArrayLayoutControl's 40 + slot 40 (Initialize); Win 0xa2865c
//   = Control's 33 + slot 33.
#pragma once
#include "LEGO/Types.h"
#include "ArrayLayoutControl.h"
#include "Font.h"

namespace LEGO {

class String;

class TextOutputControl : public ArrayLayoutControl {
public:
    // Mac 0x2daae0 (C1), Win 0x514055
    explicit TextOutputControl(ControlManager* manager);
    virtual ~TextOutputControl();                                  // Mac 0x2dae60/0x2daf20, Win 0x513fbe/0x514025

    // Vertical, resized along the column, not reversed. Mac 0x2dab90, Win 0x514108 (slot 40/33)
    virtual void Initialize();

    // The font of every line (also the existing ones, which are re-wrapped). Mac 0x2dabc0,
    // Win 0x514130
    void SetFont(Font* font);
    // Appends a line (empty text: nothing); then drops the first lines while more than one
    // is left and the column is taller than the maximum height. Mac 0x2dac70, Win 0x514177
    void AddLine(const String& text);

    UInt32 GetLineWidth() const { return m_lineWidth; }
    void SetLineWidth(UInt32 width) { m_lineWidth = width; }      // (no setter in the binaries)
    UInt32 GetMaxHeight() const { return m_maxHeight; }
    void SetMaxHeight(UInt32 height) { m_maxHeight = height; }    // (no setter in the binaries)

protected:
    // ---- data (Mac / Win offsets) ----
    ControlManager*  m_lineManager;   // +0x190 / +0x1a8 the constructor's manager (for the lines)
    RefPointer<Font> m_font;          // +0x194 / +0x1ac null
    UInt32           m_lineWidth;     // +0x198 / +0x1b0 10000 (ResizeHeightToFitText)
    UInt32           m_maxHeight;     // +0x19c / +0x1b4 2000
                                      // Object +0x1a0 / +0x1b8
};

} // namespace LEGO
