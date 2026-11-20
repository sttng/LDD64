// LEGO Digital Designer — restored source
// LDDUtilities (TU 063): the drop-down item factory (Mac 0x14ba40 + the inline constructor copy
// 0x14c360; Win 0x63874e / 0x63846b). Kept in its own file so that its users (the Windows
// language row of the preferences dialog, LDDPostProcessor) do not link the rest of
// LDDUtilities. File name inferred.
#include "LDDUtilities.h"

#include "Control.h"
#include "StretchableButtonControl.h"
#include "ButtonControl.h"
#include "RenderControlManager.h"
#include "LDDApplication.h"
#include "Texture.h"
#include "Geometry.h"

namespace LEGO {

// Mac 0x14ba40, Win 0x63874e
StretchableButtonControl* CreateDropDownItem(const String& text, Control* parent,
                                             ControlManager* manager)
{
    // Held raw (no reference): the caller takes the first one.
    StretchableButtonControl* button = new StretchableButtonControl(manager);
    const Vector2 corner(3.0f, 3.0f);
    if (!button->Create(Rect(Vector2(360.0f, 205.0f), corner), Rect(Vector2(370.0f, 205.0f), corner),
                        Rect(Vector2(400.0f, 205.0f), corner), Rect(Vector2(360.0f, 210.0f), corner),
                        Rect(Vector2(370.0f, 210.0f), corner), Rect(Vector2(400.0f, 210.0f), corner),
                        Rect(Vector2(360.0f, 215.0f), corner), Rect(Vector2(370.0f, 215.0f), corner),
                        Rect(Vector2(400.0f, 215.0f), corner), 0, 0)) {
        RefPointer<StretchableButtonControl> release(button);   // (the original's Release)
        return 0;
    }
    button->Setup(parent, 0, 7, Rect(Vector2(120.0f, 20.0f)), 0, false);
    // (inlined LoadApplicationTexture: Platform directory 9)
    RefPointer<Texture> image = LoadApplicationTexture(
        String("Graphics/EditScreen/EditMode_Final.png"),
        static_cast<RenderControlManager*>(manager)->GetRender());
    if (!image.Get()) {
        // NOTE (both builds): the original releases the button here although the parent still
        // holds it (the parent's last reference: the button is deleted while it is still a
        // child); fixed: the button only stays a child of `parent`.
        return 0;
    }
    ButtonControl::StateDescription description;
    description.m_transparent = false;
    description.m_font = LDDApplication::GetInstance()->GetFont(3);
    description.m_text = text;
    description.m_image = image;
    static const Real32 kStates[4][2] = {{260.0f, 200.0f}, {360.0f, 205.0f},
                                         {360.0f, 205.0f}, {260.0f, 200.0f}};
    for (UInt32 i = 0; i < 4; ++i) {
        description.m_imageRect = Rect(Vector2(kStates[i][0], kStates[i][1]), Vector2(44.0f, 14.0f));
        button->SetStateDescription(i, description);
    }
    button->SetHorizontalTextOffset(5.0f);
    button->SetVerticalTextOffset(5.0f);
    return button;
}

} // namespace LEGO
