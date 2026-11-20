// LEGO Digital Designer — restored source
// LEGO::PreferenceControl — one row of the preferences dialog (PreferencesControl): a
//   RenderControl that edits one preference and can be asked whether it changed, store it,
//   remember / restore the value shown when the dialog opened, go back to the default, be
//   enabled or disabled, and whether a change needs a restart of LDD. Abstract; the rows are
//   PreferenceControlBool (check box, TU 132), PreferenceControlSlider (TU 285) and, Windows
//   only, PreferenceControlDropDown (the language).
//
// No own TU: the destructor and NeedsRestart are emitted with PreferenceControlBool (TU 132,
// Mac 0x1e93c0 / 0x1e9420 / 0x1e9490; Win sdd 0x651073, NeedsRestart 0x650ee3); the constructor
// is inline (copies in the subclasses' constructors).
// RTTI (Mac): si RenderControl, virtual base Object (+0x1f0 in a PreferenceControl alone).
// Mac vtable 0x740bfc (53 entries): RenderControl's 45, new 45-51 pure, 52 NeedsRestart
//   (0x1e9490). Win 0x9d5d24 (48 = 40 + 40-46 pure + 47 NeedsRestart).
// Layout (Mac): RenderControl +0x000..+0x1ec, m_needsRestart +0x1ec, Object +0x1f0.
// Win: m_needsRestart +0x20c? (the subclasses' members start at Win +0x224 / Mac +0x204).
// Every method and member name is inferred (the callers: PreferencesControl, TU 133).
#pragma once
#include "LEGO/Types.h"
#include "RenderControl.h"

namespace LEGO {

class ControlManager;

class PreferenceControl : public RenderControl {
public:
    // (inline; copies in PreferenceControlBool Mac 0x1edc20 / PreferenceControlSlider Mac
    // 0x299420)
    PreferenceControl(ControlManager* manager, bool needsRestart)
        : RenderControl(manager), m_needsRestart(needsRestart) {}
    virtual ~PreferenceControl();                                           // Mac 0x1e93c0/0x1e9420, Win sdd 0x651073

    // ---- new virtuals (Mac slot / Win slot) ----
    // Shows the remembered value again (Cancel). Mac Bool 0x1e8f90, Slider 0x299d70
    virtual void Revert() = 0;                                              // 45/40
    // The shown value differs from the remembered one.
    virtual bool IsChanged() const = 0;                                     // 46/41
    // Writes the shown value into the preferences (OK; every change of a check box).
    virtual void Store() = 0;                                               // 47/42
    // Remembers the shown value (the dialog opens).
    virtual void Remember() = 0;                                            // 48/43
    // Removes the preference (back to the default) and shows the default.
    virtual void ResetToDefault() = 0;                                      // 49/44
    // Enables / disables the row (disabled rows are drawn at 25 % opacity).
    virtual void Enable(bool enabled) = 0;                                  // 50/45
    // The row is on: a ticked and enabled check box, an enabled slider.
    virtual bool IsOn() const = 0;                                          // 51/46
    // A change only takes effect after a restart (the dialog then shows its note).
    virtual bool NeedsRestart() const;                                      // 52/47 Mac 0x1e9490, Win 0x650ee3

protected:
    bool m_needsRestart;   // Mac +0x1ec
                           // Object +0x1f0 (a PreferenceControl alone)

private:
    PreferenceControl(const PreferenceControl&);
    PreferenceControl& operator=(const PreferenceControl&);
};

} // namespace LEGO
