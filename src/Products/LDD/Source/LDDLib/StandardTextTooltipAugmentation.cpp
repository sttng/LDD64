// LEGO Digital Designer — restored source
// StandardTextTooltipAugmentation — TU 145 (Mac 0x207980-0x20808b), Win 0x513a74-0x513f70.
// See StandardTextTooltipAugmentation.h.
#include "StandardTextTooltipAugmentation.h"
#include "FramedTextControl.h"
#include "ControlManager.h"
#include "ObjectRegistry.h"
#include "ObjectFactory.h"
#include "Font.h"

#include <typeinfo>

namespace LEGO {

// Mac 0x207a10 / 0x207980
StandardTextTooltipAugmentation::StandardTextTooltipAugmentation(Control* control,
                                                                 const String& text)
    : TooltipAugmentation(), m_text(text)
{
    if (control)
        AttachTo(control);
}

// Mac 0x207ad0
void StandardTextTooltipAugmentation::SetText(const String& text)
{
    m_text = text;
    Refresh();
}

// Mac 0x207de0, Win 0x513ed9
bool StandardTextTooltipAugmentation::CreateTooltip(RefPointer<Control>& tooltip,
                                                    ControlRefArray& /*extra*/)
{
    RefPointer<Object> registered = ObjectRegistry::GetInstance()->Find(kTooltipControlID);
    RefPointer<FramedTextControl> control =
        registered ? dynamic_cast<FramedTextControl*>(registered.Get()) : 0;
    control->SetText(m_text);
    tooltip = control.Get();
    return true;
}

// Mac 0x207fe0, Win 0x513f5a
bool StandardTextTooltipAugmentation::UpdateTooltip(Control* tooltip, ControlRefArray& /*extra*/)
{
    static_cast<RenderControl*>(tooltip)->SetText(m_text);   // slot 39 (Win 36) of the control
    return true;
}

// Mac 0x207b00
bool StandardTextTooltipAugmentation::InstallTooltipControl(ControlManager* manager,
                                                            Texture* image, const Rect* imageRect,
                                                            const Vector3& frameHorizontal,
                                                            const Vector3& frameVertical,
                                                            bool drawCenter)
{
    // A previously registered control stays referenced until the end (the registration below
    // then fails; as the original).
    RefPointer<Object> previous = ObjectRegistry::GetInstance()->Find(kTooltipControlID);
    Font::SystemTypeFontArgs args;
    args.color = Color::Black();
    args.type = Font::kToolTipsFont;
    Object* object = manager->GetObjectFactory()->CreateObject(typeid(Font), args);
    RefPointer<Font> font = object ? dynamic_cast<Font*>(object) : 0;
    if (!font)
        return false;
    RefPointer<FramedTextControl> control = new FramedTextControl(manager);
    control->SetFont(font);
    control->SetInputFlags(control->GetInputFlags() & ~(UInt32)RenderControl::kInputInitialized);
    ObjectRegistry::GetInstance()->Register(kTooltipControlID, control.Get());
    control->SetImage(image, imageRect);
    control->SetFrame(frameHorizontal, frameVertical, drawCenter);
    control->SetDrawAsFrame(true);
    return true;
}

} // namespace LEGO
