// LEGO Digital Designer — restored source
// LEGO::PreferenceControlSlider — a slider row of the preferences dialog: a label (font 3)
//   and an LDDSliderControl right-aligned in the row that keeps its value in a preference
//   (the slider writes it on every change: SliderControl::SetPreferenceName).
//
// TU 285 "PreferenceControlSlider.cpp" (Mac 0x299420-0x29aa1b; Win 0x68a90b-0x68ac81,
// 0x68aca3). The TU also emits BasicArray<SliderControl*> (ObserverN, Mac 0x29a0d0).
// Static initialiser Mac 0x29a9a0 = <iostream> + a header static.
// RTTI (Mac): vmi {PreferenceControl +0, SliderControlObserver +0x1f0}, virtual base Object.
// Mac vtable 0x7409dc (56 entries): D1 0x29a220 / D0 0x29a440, OnFinalRelease 2 (0x29a6b0),
//   45-51 (below), Create 53 (0x299510), primary entries 54 (OnAttached, 0x29a930, thunk
//   0x29a940) and 55 (OnValueChanged 0x29a950, thunk 0x29a970) of the observer overriders.
// Win vtable 0x9dc8cc (49 entries): 40 Revert 0x68a91c, 41 IsChanged 0x68a932, 42 Store
//   0x68a957, 43 Remember 0x68a9b7, 44 ResetToDefault 0x68a9cd, 45 Enable 0x68aa6e, 46 IsOn
//   0x68aad2, 47 NeedsRestart 0x650ee3, 48 Create 0x68aca3; observer-in 0x9dc898
//   {OnAttached / OnDetached 0x65a12a (ICF, empty), OnValueChanged 0x68a90b}; constructor
//   0x68aae4, sdd 0x68ac81.
// Layout (Mac): PreferenceControl +0x000..+0x1f0, SliderControlObserver +0x1f0 (subjects
//   +0x1f4), remembered value +0x204, slider +0x208, label +0x20c, Object +0x210 (sizeof
//   0x218).
// Method and member names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "SliderControl.h"
#include "LDDSliderControl.h"
#include "PreferenceControl.h"

namespace LEGO {

class Control;
class ControlManager;

class PreferenceControlSlider : public PreferenceControl, public SliderControlObserver {
public:
    // NOTE: the original leaves m_value uninitialised (Remember sets it before any use).
    PreferenceControlSlider(ControlManager* manager, bool needsRestart);    // Mac 0x299420, Win 0x68aae4
    virtual ~PreferenceControlSlider();                                     // Mac 0x29a220/0x29a440, Win sdd 0x68ac81

    // ---- PreferenceControl ----
    virtual void Revert();                                                  // 45/40 Mac 0x299d70, Win 0x68a91c
    virtual bool IsChanged() const;                                         // 46/41 Mac 0x299da0, Win 0x68a932
    // The slider's preference = its value (ToString(Real32)).
    virtual void Store();                                                   // 47/42 Mac 0x299de0, Win 0x68a957
    virtual void Remember();                                                // 48/43 Mac 0x299ec0, Win 0x68a9b7
    // Removes the preference and shows its default.
    // NOTE (both builds): without a default the value read is uninitialised (here 0).
    virtual void ResetToDefault();                                          // 49/44 Mac 0x299ef0, Win 0x68a9cd
    // Opacity 1 / 0.25 of the label and the slider; the slider enabled / disabled.
    virtual void Enable(bool enabled);                                      // 50/45 Mac 0x29a030, Win 0x68aa6e
    // The slider is enabled (in its tree).
    virtual bool IsOn() const;                                              // 51/46 Mac 0x29a0b0, Win 0x68aad2

    // ---- new virtual ----
    // RenderControl::Initialize; Setup below `parent` (no size), transparent; the label
    // (`label`, font 3, ResizeHeightToFitText(0), transparent) at (0, 0); the
    // LDDSliderControl (minimum, maximum, step, value = minimum) created with `style` and
    // `preferenceName` inside the row and aligned to its right edge, observed; the row fits
    // its children. Mac 0x299510, Win 0x68aca3
    virtual bool Create(Control* parent, Real32 minimum, Real32 maximum, Real32 step,
                        UInt32 style, const String& preferenceName, const String& label); // 53/48

protected:
    // ---- SliderControlObserver ----
    // (empty override in both builds) Mac 0x29a930 (thunk 0x29a940)
    virtual void OnAttached(SliderControl* slider);                         // 54
    using SliderControlObserver::OnAttached;
    using TextLayoutObserver::OnAttached;
    using TextureObserver::OnAttached;
    // Store. Mac 0x29a950 (thunk 0x29a970), Win 0x68a90b
    virtual void OnValueChanged(SliderControl* slider, Real32 value, Real32 oldValue); // 55

    // The observer part, then RenderControl's. Mac 0x29a6b0 (thunks 0x29a720 / 0x29a7a0 /
    // 0x29a820 / 0x29a8a0)
    virtual void OnFinalRelease();

    Real32                       m_value;    // +0x204 remembered value
    RefPointer<LDDSliderControl> m_slider;   // +0x208
    RefPointer<RenderControl>    m_label;    // +0x20c
                                             // Object +0x210
};

} // namespace LEGO
