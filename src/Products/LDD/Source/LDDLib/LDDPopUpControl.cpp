// LEGO Digital Designer — restored source
// TU 192 "LDDPopUpControl.cpp": Mac 0x2469f0-0x246ecb, Win 0x6603d3-0x660741.
// Not code: Mac 0x246e50 = <iostream> + a header static (ErrorCode constants).
#include "LDDPopUpControl.h"
#include "RenderControlManager.h"
#include "LDDUtilities.h"
#include "Texture.h"

namespace LEGO {

// Mac 0x2469f0, Win 0x6603fc
bool LDDPopUpControl::Create(Control* parent, Control* after)
{
    RefPointer<Texture> image = LoadApplicationTexture(String("Graphics/LDDgraphics01.png"),
        static_cast<RenderControlManager*>(m_manager)->GetRender());
    if (!image)
        return false;
    // The 9-slice of the pop-up frame in LDDgraphics01.png (corners 22 x 22 / 17 x 22 / 22 x 17 /
    // 17 x 17, edges 5 wide).
    const Rect topLeft    (Vector2(169.0f, 341.0f), Vector2(22.0f, 22.0f));
    const Rect top        (Vector2(191.0f, 341.0f), Vector2( 5.0f, 22.0f));
    const Rect topRight   (Vector2(196.0f, 341.0f), Vector2(17.0f, 22.0f));
    const Rect left       (Vector2(169.0f, 363.0f), Vector2(22.0f,  5.0f));
    const Rect center     (Vector2(191.0f, 363.0f), Vector2( 5.0f,  5.0f));
    const Rect right      (Vector2(196.0f, 363.0f), Vector2(17.0f,  5.0f));
    const Rect bottomLeft (Vector2(169.0f, 368.0f), Vector2(22.0f, 17.0f));
    const Rect bottom     (Vector2(191.0f, 368.0f), Vector2( 5.0f, 17.0f));
    const Rect bottomRight(Vector2(196.0f, 368.0f), Vector2(17.0f, 17.0f));
    if (!StretchableControl::Create(image, topLeft, top, topRight, left, center, right,
                                    bottomLeft, bottom, bottomRight))
        return false;
    return Setup(parent, after, 6, Rect(0.0f, 0.0f, 0.0f, 0.0f), 1, true);
}

// Mac 0x246e10, Win 0x6603d3
void LDDPopUpControl::Show(const Vector2& position)
{
    SetZOrder(0);
    SetShown(true);
    SetPosition(position);
    MoveInsideParent();
}

} // namespace LEGO
