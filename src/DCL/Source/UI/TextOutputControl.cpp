// LEGO Digital Designer — restored source
// TU 326 "TextOutputControl.cpp" (Mac 0x2daae0-0x2db09b, Win 0x514025-0x514297).
#include "TextOutputControl.h"
#include "RenderControl.h"
#include "ControlManager.h"
#include "LEGO/String.h"

namespace LEGO {

// Mac 0x2daae0 (C1), Win 0x514055
TextOutputControl::TextOutputControl(ControlManager* manager)
    : ArrayLayoutControl(manager), m_lineManager(manager), m_lineWidth(10000), m_maxHeight(2000)
{
}

// Mac 0x2dae60 (D1, thunks 0x2daf60 / 0x2dafc0), 0x2daf20 (D0, thunks 0x2daf80 / 0x2dafe0);
// Win 0x513fbe, sdd 0x514025
TextOutputControl::~TextOutputControl()
{
}

// Mac 0x2dab90, Win 0x514108
void TextOutputControl::Initialize()
{
    SetHorizontal(false);
    SetResizeMode(ResizeSingle);
    SetReversed(false);
}

// Mac 0x2dabc0, Win 0x514130
void TextOutputControl::SetFont(Font* font)
{
    m_font = font;
    for (ControlIterator it = BeginControls(); it != EndControls(); ++it) {
        RenderControl* line = static_cast<RenderControl*>(it->Get());
        line->SetFont(m_font);
        line->ResizeHeightToFitText(m_lineWidth);
    }
}

// Mac 0x2dac70, Win 0x514177
void TextOutputControl::AddLine(const String& text)
{
    if (text.IsEmpty())
        return;
    RefPointer<RenderControl> line = new RenderControl(m_lineManager);
    line->Initialize();
    line->Setup(0, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    line->SetTransparent(true);
    line->SetFont(m_font);
    line->SetText(text);
    line->ResizeHeightToFitText(m_lineWidth);
    AddControl(line);
#if defined(_WIN32)
    // x87: the unsigned height loaded exactly (fild + 2^32 fix-up) and compared as is
    while (GetControlCount() > 1 && (double)m_maxHeight < (double)m_size.y)
#else
    // SSE: the unsigned height converted to float first
    while (GetControlCount() > 1 && (Real32)(double)m_maxHeight < m_size.y)
#endif
        RemoveControl(BeginControls()->Get());
}

} // namespace LEGO
