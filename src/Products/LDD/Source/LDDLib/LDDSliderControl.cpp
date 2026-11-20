// LEGO Digital Designer — restored source
// TU 401 "LDDSliderControl.cpp": Mac 0x389400-0x38b26b, Win 0x696085-0x696456.
// Not code: Mac 0x38b1f0 = <iostream> + a header static.
#include "LDDSliderControl.h"
#include "RenderControlManager.h"
#include "LDDUtilities.h"
#include "Texture.h"
#include "MathUtil.h"

namespace LEGO {

namespace {

// Away from `bound` by more than the slider tolerance (1e-5).
inline bool IsAwayFrom(Real32 value, Real32 bound)
{
    return 1e-05f <= Abs(value - bound);
}

} // namespace

// Mac 0x389400
LDDSliderControl::LDDSliderControl(ControlManager* manager, Real32 minimum, Real32 maximum,
                                   Real32 step, Real32 value)
    : SliderControl(manager, minimum, maximum, step, value), m_minusButton(0), m_plusButton(0),
      m_trackRect(0.0f, 0.0f, 0.0f, 0.0f), m_disabledTrackRect(0.0f, 0.0f, 0.0f, 0.0f)
{
}

// Mac 0x38a6d0 (D1) / 0x38a9d0 (D0), Win sdd 0x696456
LDDSliderControl::~LDDSliderControl()
{
}

// Mac 0x38ad40 (thunks 0x38ae10 … 0x38aeb0)
void LDDSliderControl::OnFinalRelease()
{
    SliderControlObserver::OnFinalRelease();
    ControlObserver::OnFinalRelease();
    SliderControl::OnFinalRelease();
}

// Mac 0x389670
bool LDDSliderControl::Create(UInt32 style, const String& preferenceName, Control* parent,
                              Control* after)
{
    if (!SliderControl::Initialize(preferenceName))
        return false;
    Setup(parent, after, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, true);
    RefPointer<Texture> image = LoadApplicationTexture(String("Graphics/LDDgraphics01.png"),
        static_cast<RenderControlManager*>(m_manager)->GetRender());
    if (!image)
        return false;

    if (!(style & 1)) {
        m_trackRect.Set(Rect(Vector2(169.0f, 257.0f), Vector2(60.0f, 7.0f)));
        m_disabledTrackRect.Set(Rect(Vector2(169.0f, 248.0f), Vector2(61.0f, 7.0f)));
    } else {
        m_trackRect.Set(Rect(Vector2(99.0f, 257.0f), Vector2(61.0f, 7.0f)));
        m_disabledTrackRect.Set(Rect(Vector2(99.0f, 248.0f), Vector2(61.0f, 7.0f)));
    }
    const Vector2 thumbSize(11.0f, 15.0f);
    const Rect thumbUp      (Vector2(251.0f, 267.0f), thumbSize);
    const Rect thumbOver    (Vector2(264.0f, 267.0f), thumbSize);
    const Rect thumbDown    (Vector2(277.0f, 267.0f), thumbSize);
    const Rect thumbDisabled(Vector2(251.0f, 284.0f), thumbSize);
    const Rect noMask(0.0f, 0.0f, 0.0f, 0.0f);
    SetImages(image, m_trackRect, thumbUp, thumbOver, thumbDown, thumbDisabled, 0.0f, 0,
              noMask, Vector2(), noMask, Vector2());

    ButtonControl::StateDescription description;
    description.m_image = image;
    description.m_transparent = false;
    const Vector2 buttonSize(17.0f, 16.0f);

    m_plusButton = new ButtonControl(m_manager);
    if (!m_plusButton->Create(this, 0, 7, Rect(Vector2(0.0f, 0.0f), buttonSize), 0, true))
        return false;
    static const Real32 kPlusX[4] = { 175.0f, 194.0f, 213.0f, 232.0f };
    for (UInt32 i = 0; i < 4; ++i) {
        description.m_imageRect.Set(Rect(Vector2(kPlusX[i], 266.0f), buttonSize));
        m_plusButton->SetStateDescription(i, description);
    }
    ControlObserver::Attach(m_plusButton);

    m_minusButton = new ButtonControl(m_manager);
    if (!m_minusButton->Create(this, 0, 7, Rect(Vector2(0.0f, 0.0f), buttonSize), 0, true))
        return false;
    static const Real32 kMinusX[4] = { 99.0f, 118.0f, 137.0f, 156.0f };
    for (UInt32 i = 0; i < 4; ++i) {
        description.m_imageRect.Set(Rect(Vector2(kMinusX[i], 266.0f), buttonSize));
        m_minusButton->SetStateDescription(i, description);
    }
    ControlObserver::Attach(m_minusButton);

    // "+" right of the track, "-" left of it, both vertically centred on it.
    m_plusButton->SetAlignment(0x21, 0x23, 0.0f, 0.0f, m_track);
    m_minusButton->SetAlignment(0x23, 0x21, 0.0f, 0.0f, m_track);
    SliderControlObserver::Attach(this);
    m_plusButton->SetEnabled(IsAwayFrom(GetValue(), GetMaximum()));
    m_minusButton->SetEnabled(IsAwayFrom(GetValue(), GetMinimum()));
    GrowToChildren(true, true, true, true);
    return true;
}

// Mac 0x38aed0, Win 0x696085
void LDDSliderControl::OnEnabled()
{
    m_plusButton->SetEnabled(IsAwayFrom(GetValue(), GetMaximum()));
    m_minusButton->SetEnabled(IsAwayFrom(GetValue(), GetMinimum()));
    m_track->SetImage(m_texture, &m_trackRect);
    SliderControl::OnEnabled();
}

// Mac 0x38afc0, Win 0x696126
void LDDSliderControl::OnDisabled()
{
    m_plusButton->SetEnabled(false);
    m_minusButton->SetEnabled(false);
    m_track->SetImage(m_texture, &m_disabledTrackRect);
    SliderControl::OnDisabled();
}

// Mac 0x38b010 (thunk 0x38b060), Win 0x696163
void LDDSliderControl::OnMouseClicked(Control* control, const Vector2& /*position*/, UInt32 button)
{
    if (button != 0)
        return;
    if (m_minusButton == control)
        IncreaseSteps(-1);
    if (m_plusButton == control)
        IncreaseSteps(1);
}

// Mac 0x38b0b0 (thunk 0x38b150), Win 0x69619b
void LDDSliderControl::OnValueChanged(SliderControl* /*slider*/, Real32 value, Real32 /*oldValue*/)
{
    m_plusButton->SetEnabled(IsAwayFrom(value, GetMaximum()));
    m_minusButton->SetEnabled(IsAwayFrom(value, GetMinimum()));
}

} // namespace LEGO
