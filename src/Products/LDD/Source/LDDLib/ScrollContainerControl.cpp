// LEGO Digital Designer — restored source
// LEGO::ScrollContainerControl — TU 143 "ScrollContainerControl.cpp" (Mac 0x204d90-0x20786b,
// Win 0x608091-0x609097 + constructor 0x5c929d). See ScrollContainerControl.h.
// Not restored here: the this-adjusting thunks and construction-vtable helpers (Mac
// 0x206dc0-0x207490).
#include "ScrollContainerControl.h"
#include "RenderControlManager.h"
#include "LDDUtilities.h"
#include "SimpleSound.h"
#include "Texture.h"
#include "LEGO/String.h"

namespace LEGO {

namespace {

// Rectangle of the EditMode texture (position, size in pixels).
Rect ImageRect(Real32 x, Real32 y, Real32 width, Real32 height)
{
    return Rect(Vector2(x, y), Vector2(width, height));
}

// The four states of an arrow button: images at `x0`, +32, +64, +96 (inlined 16 times).
void SetArrowImages(ButtonControl* button, ButtonControl::StateDescription& description,
                    const Real32* xs, Real32 y, Real32 width, Real32 height)
{
    for (UInt32 state = 0; state < 4; ++state) {
        description.m_imageRect = ImageRect(xs[state], y, width, height);
        button->SetStateDescription(state, description);
    }
}

const Real32 kVerticalArrowX[4] = { 309.0f, 341.0f, 373.0f, 405.0f };
const Real32 kHorizontalArrowX[4] = { 376.0f, 405.0f, 434.0f, 463.0f };

} // namespace

// Win 0x5c929d (Mac inlined)
ScrollContainerControl::ScrollContainerControl(ControlManager* manager)
    : ContainerControl(manager),
      m_frame(0), m_frameInner(0),
      m_verticalTrack(0), m_verticalThumbImage(0),
      m_horizontalTrack(0), m_horizontalThumbImage(0),
      m_upArrow(0), m_downArrow(0), m_leftArrow(0), m_rightArrow(0),
      m_scrollSound(0)
{
}

// Mac 0x206c90/0x206cf0, Win 0x5c943a / 0x5c970e
ScrollContainerControl::~ScrollContainerControl()
{
}

// Mac 0x206d60
void ScrollContainerControl::OnFinalRelease()
{
    Observer<Preferences, PreferencesObserver>::OnFinalRelease();
    ContainerControl::OnFinalRelease();
}

void ScrollContainerControl::PlayScrollSound()
{
    m_scrollSound->Play();
}

// Mac 0x204d90, Win 0x6081f2
bool ScrollContainerControl::Create(UInt32 scrollBars, bool background, Control* parent)
{
    SetParent(parent, 0);
    m_scrollSound = LDDUtilities::GetInstance()->CreateSound(String("IA_scroll.wav"));

    RefPointer<Texture> texture = LoadApplicationTexture(
        String("Graphics/EditScreen/EditMode_Final.png"),
        static_cast<RenderControlManager*>(m_manager)->GetRender());
    if (!texture)
        return false;

    if (background) {
        m_frame = new RenderControl(m_manager);
        if (!m_frame->Initialize())
            return false;
        m_frame->Setup(this, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
        m_frame->SetMargins(0.0f, 0.0f, 0.0f, 0.0f, 0);
        m_frame->SetColor(Color(0.2f, 0.2f, 0.2f, 1.0f));

        m_frameInner = new RenderControl(m_manager);
        if (!m_frameInner->Initialize())
            return false;
        m_frameInner->Setup(m_frame, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
        m_frameInner->SetMargins(-1.0f, 1.0f, -1.0f, 1.0f, 0);
        m_frameInner->SetColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
    }

    bool horizontal = false;
    if (scrollBars != kHorizontal) {
        m_verticalThumbImage = new RenderControl(m_manager);
        if (!m_verticalThumbImage->Initialize())
            return false;
        Rect rect = ImageRect(311.0f, 160.0f, 12.0f, 8.0f);
        m_verticalThumbImage->SetImage(texture, &rect);
    }
    if (scrollBars != kVertical) {
        m_horizontalThumbImage = new RenderControl(m_manager);
        if (!m_horizontalThumbImage->Initialize())
            return false;
        Rect rect = ImageRect(494.0f, 252.0f, 8.0f, 12.0f);
        m_horizontalThumbImage->SetImage(texture, &rect);
        horizontal = true;
    }

    ButtonControl::StateDescription description;
    if (scrollBars != kHorizontal) {
        m_upArrow = new ButtonControl(m_manager);
        if (!m_upArrow->Initialize())
            return false;
        description.m_transparent = false;
        description.m_image = texture;
        SetArrowImages(m_upArrow, description, kVerticalArrowX, 125.0f, 16.0f, 14.0f);
        m_downArrow = new ButtonControl(m_manager);
        if (!m_downArrow->Initialize())
            return false;
        SetArrowImages(m_downArrow, description, kVerticalArrowX, 141.0f, 16.0f, 14.0f);
    }
    if (horizontal) {
        m_leftArrow = new ButtonControl(m_manager);
        if (!m_leftArrow->Initialize())
            return false;
        description.m_transparent = false;
        description.m_image = texture;
        SetArrowImages(m_leftArrow, description, kHorizontalArrowX, 270.0f, 14.0f, 16.0f);
        m_rightArrow = new ButtonControl(m_manager);
        if (!m_rightArrow->Initialize())
            return false;
        SetArrowImages(m_rightArrow, description, kHorizontalArrowX, 252.0f, 14.0f, 16.0f);
    }

    if (!ContainerControl::Initialize())
        return false;
    if (scrollBars != kHorizontal) {
        SetScrollBar(m_verticalThumbImage, 16.0f, true);
        SetScrollButtons(m_upArrow, m_downArrow, true);
    }
    if (horizontal) {
        SetScrollBar(m_horizontalThumbImage, 16.0f, false);
        SetScrollButtons(m_leftArrow, m_rightArrow, false);
    }

    if (scrollBars != kHorizontal) {
        m_verticalTrack = new RenderControl(m_manager);
        if (!m_verticalTrack->Initialize())
            return false;
        Control* bar = m_verticalThumbImage->GetParent();
        m_verticalTrack->Setup(bar, bar, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
        m_verticalTrack->SetMargins(0.0f, 0.0f, 0.0f, 0.0f, 0);
        Rect rect = ImageRect(341.0f, 163.0f, 16.0f, 3.0f);
        m_verticalTrack->SetImage(texture, &rect);
    }
    if (horizontal) {
        m_horizontalTrack = new RenderControl(m_manager);
        if (!m_horizontalTrack->Initialize())
            return false;
        Control* bar = m_horizontalThumbImage->GetParent();
        m_horizontalTrack->Setup(bar, bar, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
        m_horizontalTrack->SetMargins(0.0f, 0.0f, 0.0f, 0.0f, 0);
        Rect rect = ImageRect(493.0f, 270.0f, 4.0f, 16.0f);
        m_horizontalTrack->SetImage(texture, &rect);
    }
    if (scrollBars == kBoth) {
        RefPointer<RenderControl> corner = new RenderControl(m_manager);
        if (!corner->Initialize())
            return false;
        corner->SetColor(Color(0.85f, 0.85f, 0.85f, 1.0f));
        SetCorner(corner);
    }

    SetShown(true);
    SetEnabled(true);
    SetActive(true);
    SetMargins(1.0f, 1.0f, m_verticalTrack ? 0.0f : 1.0f, m_horizontalTrack ? 0.0f : 1.0f);
    RefPointer<Preferences> preferences = Preferences::GetInstance();
    Observer<Preferences, PreferencesObserver>::Attach(preferences);
    return true;
}

// Mac 0x206b40 (thunk 0x206c70), Win 0x608165
void ScrollContainerControl::OnPreferenceChanged(const String& key)
{
    if (!key.EqualsNoCase(String("PLAYSOUND")))
        return;
    m_scrollSound = LDDUtilities::GetInstance()->CreateSound(String("IA_scroll.wav"));
}

// Mac 0x206910 (thunk 0x206970), Win 0x608091
void ScrollContainerControl::OnMouseWheel(Control* control, UInt32 axis, Real32 delta)
{
    if (m_manager->GetMouseControl() == control) {
        SliderAugmentation* slider = 0;
        if (axis == 1)
            slider = m_horizontalSlider;
        else if (axis == 0)
            slider = m_verticalSlider;
        if (slider && m_scrollSound)
            PlayScrollSound();
    }
    ContainerControl::OnMouseWheel(control, axis, delta);
}

// Mac 0x2069f0 (thunk 0x206a40), Win 0x6080f7
void ScrollContainerControl::OnMouseClicked(Control* control, const Vector2& position, UInt32 button)
{
    if ((m_upButton == control || m_downButton == control || m_leftButton == control ||
         m_rightButton == control) && m_scrollSound)
        PlayScrollSound();
    ContainerControl::OnMouseClicked(control, position, button);
}

// Mac 0x206ab0 (thunk 0x206ae0), Win 0x608137
void ScrollContainerControl::OnSliderValueChanged(SliderAugmentation* slider, Real64 value, UInt32 axis)
{
    if (m_scrollSound)
        PlayScrollSound();
    ContainerControl::OnSliderValueChanged(slider, value, axis);
}

} // namespace LEGO
