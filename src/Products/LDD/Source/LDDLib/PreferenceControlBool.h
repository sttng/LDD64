// LEGO Digital Designer — restored source
// LEGO::PreferenceControlBool — a check box row of the preferences dialog: a "yes"/"no"
//   preference shown as a ToggleControl with its label (LDDUtilities' CreateCheckBox). Every
//   toggle stores the value at once (the dialog then re-evaluates its dependent rows); a click
//   anywhere on the row toggles the box.
//
// TU 132 "PreferenceControlBool.cpp" (Mac 0x1e8db0-0x1ea0fb; Win 0x651073-0x65154a,
// 0x68a011-0x68a327). The constructor is inline (its copy: Mac 0x1edc20 in PreferencesControl.cpp,
// Win 0x651095). Static initialiser Mac 0x1ea080 = <iostream> + a header static.
// RTTI (Mac): vmi {PreferenceControl +0, ToggleControlObserver +0x1f0}, virtual base Object.
// Mac vtable 0x7240fc (56 entries): PreferenceControl's 53 with D1 0x1e9770 / D0 0x1e97d0,
//   OnFinalRelease 2 (0x1e9840), OnMouseClicked 21 (0x1e92d0), 45-51 (below), Create 53
//   (0x1e8db0) and the primary entries 54 / 55 of the ToggleControlObserver overriders
//   (0x1e9320 / 0x1e9370; thunks 0x1e9340 / 0x1e9390 in the observer-in vtable 0x724230).
// Win vtable 0x9d5eac (49 entries): OnMouseClicked 18 0x68a1f5, 40 Revert 0x68a022,
//   41 IsChanged 0x6511b5, 42 Store 0x68a03b, 43 Remember 0x68a099, 44 ResetToDefault 0x68a0b2,
//   45 Enable 0x68a142, 46 IsOn 0x68a1cc, 47 NeedsRestart 0x650ee3, 48 Create 0x68a23c;
//   ToggleControlObserver-in 0x9d5e74 {OnToggled = OnUntoggled = 0x68a011}; sdd 0x65154a.
// Layout (Mac): PreferenceControl +0x000..+0x1f0, ToggleControlObserver +0x1f0 (subjects
//   +0x1f4), key +0x204, label +0x214, remembered value +0x224, toggle +0x228, box +0x22c,
//   label control +0x230, Object +0x234 (sizeof 0x23c). Win: key +0x224.
// Method and member names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "ToggleControl.h"
#include "PreferenceControl.h"

namespace LEGO {

class Control;
class ControlManager;

class PreferenceControlBool : public PreferenceControl, public ToggleControlObserver {
public:
    // `key`: the preference; `label`: the text next to the box. (inline; Mac 0x1edc20,
    // Win 0x651095)
    // NOTE: the original leaves m_value uninitialised (Remember sets it before any use).
    PreferenceControlBool(ControlManager* manager, const String& key, const String& label,
                          bool needsRestart)
        : PreferenceControl(manager, needsRestart), m_key(key), m_label(label), m_value(false) {}
    virtual ~PreferenceControlBool();                                       // Mac 0x1e9770/0x1e97d0, Win sdd 0x65154a

    // ---- Control: a left click anywhere on the row toggles the box (when it is enabled),
    //      then Control's handler. Mac 0x1e92d0, Win 0x68a1f5
    virtual void OnMouseClicked(const Vector2& position, UInt32 button);    // 21/18
    using PreferenceControl::OnMouseClicked;

    // ---- PreferenceControl ----
    virtual void Revert();                                                  // 45/40 Mac 0x1e8f90, Win 0x68a022
    virtual bool IsChanged() const;                                         // 46/41 Mac 0x1e98b0, Win 0x6511b5
    // The preference = "yes" / "no".
    virtual void Store();                                                   // 47/42 Mac 0x1e8fc0, Win 0x68a03b
    virtual void Remember();                                                // 48/43 Mac 0x1e9090, Win 0x68a099
    virtual void ResetToDefault();                                          // 49/44 Mac 0x1e90c0, Win 0x68a0b2
    // Opacity 1 / 0.25 of the box, its image and its label; the box enabled / disabled.
    virtual void Enable(bool enabled);                                      // 50/45 Mac 0x1e91f0, Win 0x68a142
    // Ticked and enabled (in its tree).
    virtual bool IsOn() const;                                              // 51/46 Mac 0x1e9290, Win 0x68a1cc

    // ---- new virtual ----
    // RenderControl::Initialize, the check box (CreateCheckBox at (0, 0), label offset 45)
    // ticked as the preference says (FromString of its value), observed; Setup below `parent`
    // with the box's size; transparent. Mac 0x1e8db0, Win 0x68a23c
    virtual bool Create(Control* parent);                                   // 53/48

protected:
    // ---- ToggleControlObserver: Store. Mac 0x1e9320 / 0x1e9370 (thunks 0x1e9340 / 0x1e9390),
    //      Win 0x68a011 (both)
    virtual void OnToggled(ToggleControl* control);                         // 54
    virtual void OnUntoggled(ToggleControl* control);                       // 55

    // The observer part, then RenderControl's. Mac 0x1e9840 (thunks 0x1e99d0 / 0x1e9b40 /
    // 0x1e9cb0 / 0x1e9e20)
    virtual void OnFinalRelease();

    String                    m_key;            // +0x204 / +0x224
    String                    m_label;          // +0x214
    bool                      m_value;          // +0x224 remembered value
    RefPointer<ToggleControl> m_toggle;         // +0x228
    RefPointer<RenderControl> m_box;            // +0x22c
    RefPointer<RenderControl> m_labelControl;   // +0x230
                                                // Object +0x234
};

} // namespace LEGO
