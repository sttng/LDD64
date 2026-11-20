// LEGO Digital Designer — restored source
// TU 288 "LDDPaneToggleControl.cpp": Mac 0x29bb40-0x29d1bb, Win 0x6924c6-0x692cc4.
// Template code emitted here: Observer<SlidingHideAugmentation, SlidingHideAugmentationObserver>
// (Notifier.h) and NotifierT<SlidingHideAugmentation, …>::RemoveObserver (Mac 0x29cd50 / 0x29cf40).
// Not code: Mac 0x29d140 = <iostream> + a header static.
#include "LDDPaneToggleControl.h"
#include "Command.h"
#include "RenderControlManager.h"
#include "LDDUtilities.h"
#include "Texture.h"

namespace LEGO {

// Mac 0x29bdb0, Win 0x692be8
LDDPaneToggleControl::LDDPaneToggleControl(ControlManager* manager, Command* command,
                                           SlidingHideAugmentation* augmentation)
    : ToggleControl(manager), m_command(command)
{
    SlidingHideAugmentationObserver::Attach(augmentation);
}

// Mac 0x29c550 (D1) / 0x29c6b0 (D0), Win sdd 0x692cc4
LDDPaneToggleControl::~LDDPaneToggleControl()
{
}

// Mac 0x29c830 (thunks 0x29c930 … 0x29c9b0)
void LDDPaneToggleControl::OnFinalRelease()
{
    SlidingHideAugmentationObserver::OnFinalRelease();
    ToggleControl::OnFinalRelease();
}

// Mac 0x29bf00, Win 0x692681
bool LDDPaneToggleControl::Create(Control* parent, Control* after, UInt32 flags, bool rightSide)
{
    RefPointer<Texture> image = LoadApplicationTexture(String("Graphics/LDDgraphics01.png"),
        static_cast<RenderControlManager*>(m_manager)->GetRender());
    if (!image)
        return false;
    if (!ToggleControl::Initialize())
        return false;
    if (!Setup(parent, after, flags, Rect(Vector2(15.0f, 103.0f)), 0, false))
        return false;

    // The handle images (13 x 28): the left-pointing set at x 299 + 40 * n, the right-pointing
    // one 20 pixels further; up, over, down, then the toggled up, over, down.
    StateDescription description;
    description.m_image = image;
    description.m_transparent = false;

    const Rect upLeft   (299.0f, 165.0f, 312.0f, 193.0f);
    const Rect upRight  (319.0f, 165.0f, 332.0f, 193.0f);
    description.m_imageRect.Set(rightSide ? upRight : upLeft);
    SetStateDescription(0, description);
    const Rect overLeft (339.0f, 165.0f, 352.0f, 193.0f);
    const Rect overRight(359.0f, 165.0f, 372.0f, 193.0f);
    description.m_imageRect.Set(rightSide ? overRight : overLeft);
    SetStateDescription(1, description);
    const Rect downLeft (379.0f, 165.0f, 392.0f, 193.0f);
    const Rect downRight(399.0f, 165.0f, 412.0f, 193.0f);
    description.m_imageRect.Set(rightSide ? downRight : downLeft);
    SetStateDescription(2, description);

    const Rect toggledUpLeft   (419.0f, 165.0f, 432.0f, 193.0f);
    const Rect toggledUpRight  (439.0f, 165.0f, 452.0f, 193.0f);
    description.m_imageRect.Set(rightSide ? toggledUpRight : toggledUpLeft);
    SetStateDescription(4, description);
    const Rect toggledOverLeft (459.0f, 165.0f, 472.0f, 193.0f);
    const Rect toggledOverRight(479.0f, 165.0f, 492.0f, 193.0f);
    description.m_imageRect.Set(rightSide ? toggledOverRight : toggledOverLeft);
    SetStateDescription(5, description);
    const Rect toggledDownLeft (499.0f, 165.0f, 512.0f, 193.0f);
    const Rect toggledDownRight(519.0f, 165.0f, 532.0f, 193.0f);
    description.m_imageRect.Set(rightSide ? toggledDownRight : toggledDownLeft);
    SetStateDescription(6, description);
    return true;
}

// Mac 0x29c9d0, Win 0x6924c6
void LDDPaneToggleControl::OnMouseClicked(const Vector2& /*position*/, UInt32 /*button*/)
{
    m_command->Do();
}

// Mac 0x29c9f0 (thunk 0x29ca30), Win 0x6924d3
void LDDPaneToggleControl::OnSlideStopped(Control* /*control*/)
{
    SetToggled(!IsToggled());
}

} // namespace LEGO
