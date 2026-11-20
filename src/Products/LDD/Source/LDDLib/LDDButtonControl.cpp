// LEGO Digital Designer — restored source
// TU 200 "LDDButtonControl.cpp": Mac 0x24eae0-0x24f79b, Win 0x689a0e-0x68a010.
// Not code: construction-vtable thunks 0x24f450-0x24f6a0, 0x24f720 = <iostream> + a header static.
#include "LDDButtonControl.h"
#include "LDDApplication.h"
#include "LDDUtilities.h"
#include "RenderControlManager.h"
#include "TextLayout.h"
#include "Texture.h"
#include "Font.h"

namespace LEGO {

// Mac 0x24f380/0x24f3e0, Win sdd 0x6503d6
LDDButtonControl::~LDDButtonControl()
{
}

// Mac 0x24eae0, Win 0x689a0e
bool LDDButtonControl::Create(const String& text, Control* parent, const Vector2& position)
{
    const Rect topLeft    (Vector2( 99.0f, 171.0f), Vector2(7.0f,  8.0f));
    const Rect top        (Vector2(106.0f, 171.0f), Vector2(2.0f,  8.0f));
    const Rect topRight   (Vector2(108.0f, 171.0f), Vector2(7.0f,  8.0f));
    const Rect left       (Vector2( 99.0f, 179.0f), Vector2(7.0f, 10.0f));
    const Rect center     (Vector2(106.0f, 179.0f), Vector2(2.0f, 10.0f));
    const Rect right      (Vector2(108.0f, 179.0f), Vector2(7.0f, 10.0f));
    const Rect bottomLeft (Vector2( 99.0f, 189.0f), Vector2(7.0f,  8.0f));
    const Rect bottom     (Vector2(106.0f, 189.0f), Vector2(2.0f,  8.0f));
    const Rect bottomRight(Vector2(108.0f, 189.0f), Vector2(7.0f,  8.0f));
    const Rect state0(Vector2( 99.0f, 171.0f), Vector2(16.0f, 26.0f));
    const Rect state1(Vector2(117.0f, 171.0f), Vector2(16.0f, 26.0f));
    const Rect state2(Vector2(135.0f, 171.0f), Vector2(16.0f, 26.0f));
    const Rect state3(Vector2(153.0f, 171.0f), Vector2(16.0f, 26.0f));
    if (!StretchableButtonControl::Create(topLeft, top, topRight, left, center, right,
                                          bottomLeft, bottom, bottomRight, 4, 10))
        return false;
    Setup(parent, 0, 7, Rect(position, Vector2()), 0, false);
    RefPointer<Texture> image = LoadApplicationTexture(String("Graphics/LDDgraphics01.png"),
        static_cast<RenderControlManager*>(m_manager)->GetRender());
    if (!image)
        return false;
    StateDescription state;
    state.m_text = text;
    state.m_transparent = false;
    state.m_image = image;
    state.m_font = LDDApplication::GetInstance()->GetFont(9);
    state.m_imageRect.Set(state0);
    SetStateDescription(0, state);
    state.m_imageRect.Set(state1);
    SetStateDescription(1, state);
    state.m_imageRect.Set(state2);
    SetStateDescription(2, state);
    state.m_imageRect.Set(state3);
    SetStateDescription(3, state);
    SetTextAlignment(2, 2);
    ResizeHeightToFitText(300);
    return true;
}

// Mac 0x24f290, Win 0x689f47
bool LDDButtonControl::ResizeHeightToFitText(UInt32 width)
{
    Vector2 size;
    if (m_textLayout) {
        m_textLayout->SetMaxWidth(width);
        m_textLayout->SetTruncate(true);
        size = m_textLayout->GetSize();
    }
    Real32 textWidth = (Real32)width;
    if (size.x <= textWidth)
        textWidth = size.x;
    if (textWidth <= m_minimumWidth)
        textWidth = m_minimumWidth;
    Real32 marginX = (Real32)m_textMarginX;
    size.x = marginX + marginX + textWidth;
    size.y = (Real32)(m_textMarginY * 2) + size.y;
    return SetSize(size);
}

} // namespace LEGO
