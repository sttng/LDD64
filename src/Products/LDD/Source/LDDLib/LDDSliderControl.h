// LEGO Digital Designer — restored source
// LEGO::LDDSliderControl — LDD's slider (preferences dialog, PreferenceControlSlider): a
//   SliderControl with the LDDgraphics01.png track / thumb images and a "-" and a "+" button left
//   and right of the track that step the value; each button is disabled at its end of the range,
//   and the track shows its disabled image while the slider is disabled.
//
// TU 401 "LDDSliderControl.cpp": Mac 0x389400-0x38b26b, Win 0x696085-0x696456. The TU also emits
// a BasicArray<Control*> helper (Mac 0x38a4e0). Mac 0x38b1f0 = <iostream> + a header static.
// RTTI: vmi {SliderControl +0, ControlObserver (Mac +0x280), SliderControlObserver (Mac +0x294)},
//   virtual base Object. Mac vtable 0x75556c (48 entries: OnEnabled 12 = 0x38aed0, OnDisabled
//   13 = 0x38afc0, then the Itanium primary entries of the observer overriders OnMouseClicked
//   46 = 0x38b010, OnValueChanged 47 = 0x38b0b0); ControlObserver-in 0x755698 {22: 0x38b060},
//   SliderControlObserver-in 0x75572c {5: 0x38b150}. Win: OnEnabled 0x696085, OnDisabled
//   0x696126, OnMouseClicked 0x696163, OnValueChanged 0x69619b; sdd 0x696456.
// Layout (Mac): SliderControl +0x000..+0x280, ControlObserver +0x280, SliderControlObserver
//   +0x294, minus button +0x2a8, plus button +0x2ac, track rectangles +0x2b0 (enabled) / +0x2c0
//   (disabled), Object +0x2d0 (sizeof 0x2d8).
// Method and member names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "Geometry.h"
#include "SliderControl.h"
#include "ButtonControl.h"

namespace LEGO {

class LDDSliderControl : public SliderControl, public ControlObserver,
                         public SliderControlObserver {
public:
    // Mac 0x389400
    LDDSliderControl(ControlManager* manager, Real32 minimum, Real32 maximum, Real32 step,
                     Real32 value);
    virtual ~LDDSliderControl();                                            // Mac 0x38a6d0/0x38a9d0, Win sdd 0x696456

    // SliderControl::Initialize(preferenceName); Setup below `parent` behind `after` (flags 7,
    // empty rectangle, hit-testable); the track (`style` bit 0: the shorter track image) and
    // thumb images; the "+" / "-" buttons (17 x 16) right / left of the track; observes itself;
    // the buttons' enable state; grows to its children. False without the texture or when a
    // button cannot be created. Mac 0x389670
    bool Create(UInt32 style, const String& preferenceName, Control* parent, Control* after);

    // ---- Control overrides ----
    using ControlObserver::OnEnabled;
    using ControlObserver::OnDisabled;
    // "+" enabled unless at the maximum, "-" unless at the minimum; the enabled track image.
    virtual void OnEnabled();                                               // 12/9  Mac 0x38aed0, Win 0x696085
    // Both buttons disabled; the disabled track image.
    virtual void OnDisabled();                                              // 13/10 Mac 0x38afc0, Win 0x696126

protected:
    // ---- ControlObserver: left clicks on the buttons step the value ----
    using SliderControl::OnMouseClicked;
    virtual void OnMouseClicked(Control* control, const Vector2& position, UInt32 button); // 46 Mac 0x38b010 (thunk 0x38b060), Win 0x696163
    // ---- SliderControlObserver: the buttons' enable state follows the value ----
    virtual void OnValueChanged(SliderControl* slider, Real32 value, Real32 oldValue); // 47 Mac 0x38b0b0 (thunk 0x38b150), Win 0x69619b

    // Both observer parts, then SliderControl's. Mac 0x38ad40
    virtual void OnFinalRelease();

    RefPointer<ButtonControl> m_minusButton;        // Mac +0x2a8
    RefPointer<ButtonControl> m_plusButton;         // Mac +0x2ac
    Rect                      m_trackRect;          // Mac +0x2b0
    Rect                      m_disabledTrackRect;  // Mac +0x2c0
};

} // namespace LEGO
