// LEGO Digital Designer — restored source
// LEGO::ContainerControl — part of TU 114 (Mac 0x1c82a0-0x1ca7b0; Win 0x511381-0x5122a3,
// 0x5c8f55, 0x5c9153, 0x5c96ec, 0x5ca0e6). Own file by choice (the original compiled it with
// SliderAugmentation.cpp).
// Not restored here: ObserverN<SliderAugmentation, SliderAugmentationObserver>::Attach (Mac
// 0x1c8eb0), the BasicArray<Control*> destructor helper (0x1cd0f0), the this-adjusting thunks.
#include "ContainerControl.h"
#include <math.h>

namespace LEGO {

namespace {

inline Real32 WidthOf(const Control* control) { return control ? control->GetSize().x : 0.0f; }
inline Real32 HeightOf(const Control* control) { return control ? control->GetSize().y : 0.0f; }

// The control shown, enabled and active (the three calls every Set… function makes).
void Activate(Control* control)
{
    control->SetShown(true);
    control->SetEnabled(true);
    control->SetActive(true);
}

} // namespace

// Win 0x5c8f55 (Mac inlined into the subclasses)
ContainerControl::ContainerControl(ControlManager* manager)
    : BufferedAbleRenderControl(manager),
      m_content(0), m_clip(0), m_corner(0),
      m_horizontalBar(0), m_horizontalThumb(0), m_leftButton(0), m_rightButton(0),
      m_horizontalSlider(0),
      m_verticalBar(0), m_verticalThumb(0), m_upButton(0), m_downButton(0),
      m_verticalSlider(0),
      m_marginLeft(0.0f), m_marginTop(0.0f), m_marginRight(0.0f), m_marginBottom(0.0f),
      m_scrollAlwaysH(true),
      m_scrollAlwaysV(true)
{
}

// Mac 0x1ca6e0/0x1ca740, Win 0x5c9153 / 0x5c96ec
ContainerControl::~ContainerControl()
{
}

// Mac 0x1ca7b0, Win 0x5ca0e6
void ContainerControl::OnFinalRelease()
{
    Observer<ControlManager, ControlManagerObserver>::OnFinalRelease();
    ObserverN<SliderAugmentation, SliderAugmentationObserver>::OnFinalRelease();
    ObserverN<Control, ControlObserver>::OnFinalRelease();
    Control::OnFinalRelease();
}

// Mac 0x1c82a0, Win 0x511381
bool ContainerControl::Initialize()
{
    SetEnabled(true);
    SetShown(true);
    SetActive(true);
    m_clip = new Control(m_manager);
    m_clip->Setup(this, 0, 7, Rect(m_size), 1, true);
    ObserverN<Control, ControlObserver>::Attach(m_clip);
    Observer<ControlManager, ControlManagerObserver>::Attach(m_manager);
    return true;
}

// Mac 0x1c8630, Win 0x51141d
void ContainerControl::SetContent(Control* content)
{
    if (m_content)
        m_content->SetShown(false);
    m_content = content;
    m_content->SetParent(m_clip, 0);
    Activate(m_content);
    m_content->SetPosition(Vector2(0.0f, 0.0f));
    ObserverN<Control, ControlObserver>::Attach(m_content);
    if (m_verticalSlider)
        m_verticalSlider->SetValue(0.0);
    if (m_horizontalSlider)
        m_horizontalSlider->SetValue(0.0);
    Layout();
}

// Mac 0x1c8770, Win 0x5114c6
void ContainerControl::SetScrollBar(Control* bar, Real32 thickness, bool vertical)
{
    RefPointer<Control>& container = vertical ? m_verticalBar : m_horizontalBar;
    RefPointer<Control>& thumb = vertical ? m_verticalThumb : m_horizontalThumb;
    RefPointer<SliderAugmentation>& slider = vertical ? m_verticalSlider : m_horizontalSlider;
    if (!container) {
        container = new Control(m_manager);
        Vector2 size = vertical ? Vector2(thickness, 0.0f) : Vector2(0.0f, thickness);
        container->Setup(this, 0, 7, Rect(size), 1, true);
    }
    thumb = bar;
    thumb->SetParent(container, 0);
    Activate(thumb);
    thumb->SetPosition(vertical ? Vector2(2.0f, 0.0f) : Vector2(0.0f, 2.0f));
    if (!slider) {
        slider = new SliderAugmentation();
        slider->SetDefaultAxis(vertical ? 1 : 0);
        ObserverN<SliderAugmentation, SliderAugmentationObserver>::Attach(slider);
    }
    slider->Attach(thumb, String("tab"));
    LayoutScrollBars();
}

// Mac 0x1c96e0, Win 0x51179e
void ContainerControl::SetScrollButtons(Control* decrease, Control* increase, bool vertical)
{
    if (!vertical) {
        m_leftButton = decrease;
        decrease->SetParent(this, 0);
        m_rightButton = increase;
    } else {
        m_upButton = decrease;
        decrease->SetParent(this, 0);
        m_downButton = increase;
    }
    increase->SetParent(this, 0);
    ObserverN<Control, ControlObserver>::Attach(decrease);
    ObserverN<Control, ControlObserver>::Attach(increase);
    Activate(decrease);
    Activate(increase);
    LayoutScrollBars();
}

// Mac 0x1c9890, Win 0x51186d
void ContainerControl::SetCorner(Control* corner)
{
    m_corner = corner;
    corner->SetParent(this, 0);
    Activate(corner);
    LayoutScrollBars();
}

// Mac 0x1c9930, Win 0x5118b0
void ContainerControl::SetMargins(Real32 left, Real32 top, Real32 right, Real32 bottom)
{
    m_marginLeft = left;
    m_marginTop = top;
    m_marginRight = right;
    m_marginBottom = bottom;
    LayoutScrollBars();
}

// Mac 0x1c9970
void ContainerControl::SetScrollAlways(bool vertical, bool scrollAlways)
{
    if (vertical)
        m_scrollAlwaysV = scrollAlways;
    else
        m_scrollAlwaysH = scrollAlways;
    LayoutScrollBars();
}

// Mac 0x1c93a0, Win 0x511ccd
void ContainerControl::LayoutScrollBars()
{
    // NOTE: as in the original, the bottom-right corner takes the bar thicknesses from the down
    // button's width and the right button's height (not from the bar containers).
    m_clip->SetMargins(-m_marginLeft, m_marginRight + WidthOf(m_verticalBar),
                       -m_marginTop, m_marginBottom + HeightOf(m_horizontalBar), 0);
    if (m_verticalBar) {
        m_verticalBar->SetAxisAlignment(0, 3, 3, 0.0f, 0);
        Real32 top = m_upButton ? -m_upButton->GetSize().y : 0.0f;
        m_verticalBar->SetAxisMargins(1, top, HeightOf(m_downButton) + HeightOf(m_rightButton), 0);
    }
    if (m_upButton)
        m_upButton->SetAlignment(0x13, 0x13, 0.0f, 0.0f, 0);
    if (m_downButton)
        m_downButton->SetAlignment(0x33, 0x33, 0.0f, HeightOf(m_rightButton), 0);
    if (m_horizontalBar) {
        Real32 left = m_leftButton ? -m_leftButton->GetSize().x : 0.0f;
        m_horizontalBar->SetAxisMargins(0, left, WidthOf(m_rightButton) + WidthOf(m_downButton), 0);
        m_horizontalBar->SetAxisAlignment(1, 3, 3, 0.0f, 0);
    }
    if (m_leftButton)
        m_leftButton->SetAlignment(0x31, 0x31, 0.0f, 0.0f, 0);
    if (m_rightButton)
        m_rightButton->SetAlignment(0x33, 0x33, WidthOf(m_downButton), 0.0f, 0);
    if (m_corner) {
        m_corner->SetAlignment(0x33, 0x33, 0.0f, 0.0f, 0);
        m_corner->SetSize(Vector2(WidthOf(m_downButton), HeightOf(m_rightButton)));
    }
    Layout();
}

// Mac 0x1c99a0, Win 0x5118e4
void ContainerControl::Layout()
{
    if (!m_content)
        return;
    Vector2 position(m_content->GetPosition());
    Vector2 size(m_content->GetSize());
    size.x = m_content->GetScale().x * size.x;
    size.y = m_content->GetScale().y * size.y;
    Vector2 clipSize(m_clip->GetSize());

    if (m_verticalSlider) {
        Real32 range = size.y - clipSize.y;
        if (range <= 0.0f) {
            m_verticalSlider->SetMaximum(10.0);
            m_verticalSlider->SetValue(0.0);
        } else {
            m_verticalSlider->SetMaximum((Real64)range);
            m_verticalSlider->SetValue((Real64)-position.y);
        }
        m_verticalThumb->SetShown(range > 0.0f);
        Real32 thumb = 0.0f;
        if (fabsf(size.y) >= 1e-5f)
            thumb = floorf((((m_size.y - HeightOf(m_downButton)) - HeightOf(m_upButton)) -
                            HeightOf(m_horizontalBar)) * m_size.y / size.y);
        if (thumb <= 15.0f)
            thumb = 15.0f;
        m_verticalThumb->SetSize(Vector2(m_verticalThumb->GetSize().x, thumb));
    }
    if (m_horizontalSlider) {
        Real32 range = size.x - clipSize.x;
        if (range <= 0.0f) {
            m_horizontalSlider->SetMaximum(10.0);
            m_horizontalSlider->SetValue(0.0);
        } else {
            m_horizontalSlider->SetMaximum((Real64)range);
            m_horizontalSlider->SetValue((Real64)-position.x);
        }
        m_horizontalThumb->SetShown(range > 0.0f);
        Real32 thumb = 0.0f;
        if (fabsf(size.x) >= 1e-5f)
            thumb = floorf((((m_size.x - WidthOf(m_leftButton)) - WidthOf(m_rightButton)) -
                            WidthOf(m_verticalBar)) * m_size.x / size.x);
        if (thumb <= 15.0f)
            thumb = 15.0f;
        m_horizontalThumb->SetSize(Vector2(thumb, m_horizontalThumb->GetSize().y));
    }
}

// Mac 0x1c9d80, Win 0x511bfe (+ 0x5120bf)
void ContainerControl::UpdateContentPosition()
{
    if (!m_content)
        return;
    Vector2 position(m_content->GetPosition());
    if (m_verticalSlider) {
        bool follow = m_scrollAlwaysV;
        if (!follow) {
            position.y = m_size.y - m_content->GetSize().y;
            follow = position.y < 0.0f;
        }
        if (follow)
            position.y = floorf(-(Real32)m_verticalSlider->GetValue(1));
    }
    if (m_horizontalSlider) {
        bool follow = m_scrollAlwaysH;
        if (!follow) {
            position.x = m_size.x - m_content->GetSize().x;
            follow = position.x < 0.0f;
        }
        if (follow)
            position.x = floorf(-(Real32)m_horizontalSlider->GetValue(0));
    }
    m_content->SetPosition(position);
}

// Mac 0x1ca0f0, Win 0x51208a
void ContainerControl::OnRectangleChanged(const Vector2& oldPosition, const Vector2& oldSize)
{
    Layout();
    if (!m_scrollAlwaysV || !m_scrollAlwaysH)
        UpdateContentPosition();
    BufferedAbleRenderControl::OnRectangleChanged(oldPosition, oldSize);
}

// Mac 0x1c9e70 (thunk 0x1c9e90), Win 0x511f61
void ContainerControl::OnRectangleChanged(Control* control, const Vector2& /*oldPosition*/,
                                          const Vector2& /*oldSize*/)
{
    if (m_content == control)
        Layout();
}

// Mac 0x1c9ec0 (thunk 0x1c9ee0), Win 0x511f7b
void ContainerControl::OnScaleChanged(Control* control, const Vector2& /*oldScale*/)
{
    if (m_content == control)
        Layout();
}

void ContainerControl::Scroll(SliderAugmentation* slider, Real64 delta)
{
    UInt32 axis = slider->GetDefaultAxis() != 0 ? 1 : 0;
    slider->SetValue(slider->GetValue(axis) + delta, axis);
}

// Mac 0x1c9f10 (thunk 0x1c9fa0), Win 0x511f95
void ContainerControl::OnMouseWheel(Control* control, UInt32 axis, Real32 delta)
{
    if (m_manager->GetMouseControl() != control)
        return;
    SliderAugmentation* slider;
    Real32 factor;
    if (axis == 1) {
        slider = m_horizontalSlider;
        factor = -10.0f;
    } else if (axis == 0) {
        slider = m_verticalSlider;
        factor = -40.0f;
    } else {
        return;
    }
    if (!slider)
        return;
    UInt32 own = slider->GetDefaultAxis() != 0 ? 1 : 0;
#if defined(_WIN32)
    // Win: (float)value - delta * |factor| in x87 precision.
    slider->SetValue((Real64)(Real32)slider->GetValue(own) + (Real64)delta * (Real64)factor, own);
#else
    slider->SetValue((Real64)(delta * factor) + slider->GetValue(own), own);
#endif
}

// Mac 0x1ca030 (thunk 0x1ca0d0), Win 0x51201b
void ContainerControl::OnMouseClicked(Control* control, const Vector2& /*position*/, UInt32 /*button*/)
{
    if (m_upButton == control)
        Scroll(m_verticalSlider, -32.0);
    else if (m_downButton == control)
        Scroll(m_verticalSlider, 32.0);
    else if (m_leftButton == control)
        Scroll(m_horizontalSlider, -32.0);
    else if (m_rightButton == control)
        Scroll(m_horizontalSlider, 32.0);
}

// Mac 0x1ca130 (thunk 0x1ca170), Win 0x5120bf
void ContainerControl::OnSliderValueChanged(SliderAugmentation* slider, Real64 /*value*/, UInt32 /*axis*/)
{
    if (m_verticalSlider == slider || m_horizontalSlider == slider)
        UpdateContentPosition();
}

// Mac 0x1ca1a0 (thunk 0x1ca230), Win 0x5120e6
void ContainerControl::OnManagerMouseOverChanged(Control* previous, Control* current)
{
    Control* control = current;
    while (control != this && control)
        control = control->GetParent();
    if (previous && m_content != previous)
        ObserverN<Control, ControlObserver>::Detach(previous);
    if (control && m_content != current)
        ObserverN<Control, ControlObserver>::Attach(current);
}

} // namespace LEGO
