// LEGO Digital Designer — restored source
// LEGO::StandardTextTooltipAugmentation — a tooltip that shows a fixed text in the shared
//   FramedTextControl registered under ObjectRegistry id 'STTC' (InstallTooltipControl, called
//   once by the application start-up). Base of ExtendedTextTooltipAugmentation (TU 063) and the
//   Lua tooltip bindings (TU 297, 312).
//
// TU 145 (Mac 0x207980-0x20808b; old guess "Font_SystemTypeFontArgs.cpp" — the TU also emits the
// inline Font::SystemTypeFontArgs code); Win 0x513a74-0x513f70.
// Vtable Mac 0x7102cc {GetControl, Attach, Detach, D1 0x14f600, D0 0x14f690, GetDelay 0x112320,
//   CreateTooltip 0x207de0, DestroyTooltip 0x112340, UpdateTooltip 0x207fe0} + Object-in
//   0x710308; Win 0x9d3658 {…, CreateTooltip 0x513ed9, DestroyTooltip 0x51317d, UpdateTooltip
//   0x513f5a} + Object-in 0x9d3644 {sdd 0x513ab7, …}.
// Layout (Mac): TooltipAugmentation +0x00..+0x20, String text +0x20, Object +0x30.
// Method and member names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "TooltipManager.h"
#include "LEGO/String.h"
#include "Geometry.h"

namespace LEGO {

class ControlManager;
class Texture;

class StandardTextTooltipAugmentation : public TooltipAugmentation {
public:
    enum { kTooltipControlID = 0x53545443 };   // 'STTC'

    // Attaches to `control` when it is not null. Mac 0x207a10 (C1) / 0x207980 (C2)
    StandardTextTooltipAugmentation(Control* control, const String& text);
    virtual ~StandardTextTooltipAugmentation() {}                           // Mac 0x14f600/0x14f690, Win 0x513a74/0x513ab7

    const String& GetText() const { return m_text; }
    // New text; the shown tooltip is refreshed. Mac 0x207ad0
    void SetText(const String& text);

    // The registered tooltip control with this text.
    // NOTE: dereferences a null control when none is registered (as the original).
    virtual bool CreateTooltip(RefPointer<Control>& tooltip, ControlRefArray& extra);   // Mac 0x207de0, Win 0x513ed9
    // Sets the text again; true.
    virtual bool UpdateTooltip(Control* tooltip, ControlRefArray& extra);               // Mac 0x207fe0, Win 0x513f5a

    // Creates the shared tooltip control (a FramedTextControl in the tool tip font, black,
    // drawn with `image` / `imageRect` as a 9-slice frame) and registers it as 'STTC'. False
    // when the font cannot be created. Caller: the application start-up (Mac 0x13ad50).
    static bool InstallTooltipControl(ControlManager* manager, Texture* image,
                                      const Rect* imageRect, const Vector3& frameHorizontal,
                                      const Vector3& frameVertical, bool drawCenter);    // Mac 0x207b00

protected:
    String m_text;   // Mac +0x20, Win +0x18
};

} // namespace LEGO
