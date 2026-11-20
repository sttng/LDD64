// LEGO Digital Designer — restored source (Windows only)
// LEGO::PreferenceControlDropDown — the language row of the Windows preferences dialog: a label
//   (font 3) and a DropDownControl of (value, text) items to the right of it; choosing an item
//   shows its text, keeps its value and stores it in the preference at once.
//
// Windows-only class without a Mac twin (RTTI only in LDD.exe); its bodies sit in the Win range
// of PreferencesControl.cpp / PreferenceControlBool.cpp (Win 0x651619-0x651869, 0x68a328-
// 0x68a89d), so they are restored here in their own file (name inferred).
// RTTI (Win): PreferenceControl, DropDownControlObserver (+0x20c), virtual base Object
//   (+0x260; sizeof 0x26c). Constructor Win 0x651619, destructor 0x65177d (sdd 0x651869).
// Win vtable 0x9d5fdc (49 entries): 40 Revert 0x68a5b4, 41 IsChanged 0x65176e, 42 Store
//   0x68a346, 43 Remember 0x68a328, 44 ResetToDefault 0x68a5e9, 45 Enable 0x65a12a (empty,
//   ICF), 46 IsOn 0x5fbd39 (true, ICF), 47 NeedsRestart 0x650ee3, 48 Create 0x68a89d;
//   DropDownControlObserver-in 0x9d5fac {OnItemSelected 0x68a568}.
// Layout (Win): PreferenceControl +0x000..+0x20c, DropDownControlObserver +0x20c, key +0x224,
//   label +0x228, items +0x22c, changed +0x23c, remembered value +0x240, value +0x244,
//   drop-down +0x248, label control +0x24c, BasicArray<RefPointer<Control> > +0x250 (not used),
//   Object +0x260.
// Method and member names are inferred.
#pragma once
#if defined(_WIN32)
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "DropDownControl.h"
#include "PreferenceControl.h"

namespace LEGO {

class Control;
class ControlManager;

class PreferenceControlDropDown : public PreferenceControl, public DropDownControlObserver {
public:
    // `items`: (preference value, shown text). Win 0x651619
    PreferenceControlDropDown(ControlManager* manager, const String& key, const String& label,
                              const BasicArray<Pair<String, String> >& items, bool needsRestart);
    virtual ~PreferenceControlDropDown();                                   // Win 0x65177d (sdd 0x651869)

    // ---- PreferenceControl ----
    // Selects the item of the remembered value. Win 0x68a5b4
    virtual void Revert();                                                  // 40
    virtual bool IsChanged() const;                                         // 41 Win 0x65176e
    // The preference = the value. Win 0x68a346
    virtual void Store();                                                   // 42
    // Remembers the value; not changed. Win 0x68a328
    virtual void Remember();                                                // 43
    // Removes the preference and selects the item of its default. Win 0x68a5e9
    virtual void ResetToDefault();                                          // 44
    virtual void Enable(bool /*enabled*/) {}                                // 45 Win 0x65a12a (ICF)
    virtual bool IsOn() const { return true; }                              // 46 Win 0x5fbd39 (ICF)

    // ---- new virtual ----
    // RenderControl::Initialize; Setup below `parent` (no size); the label and the drop-down
    // (its pop-up below `parent`); transparent. Win 0x68a89d
    virtual bool Create(Control* parent);                                   // 48

protected:
    // ---- DropDownControlObserver: shows the item's text, takes its value, changed = it
    //      differs from the remembered one, Store. Win 0x68a568
    virtual void OnItemSelected(Control* item, UInt32 index);

    // The label control at (0, 0), vertically centred; the row grows to it. Win 0x68a396
    bool CreateLabel();
    // The drop-down (one item control per item) at x 165 with its pop-up below
    // `popupParent`, observed; the preference's value selected; the row grows to it. False
    // without items. Win 0x68a69e
    bool CreateDropDown(Control* popupParent);
    // Selects the last item whose value is `value` (the first item without one). Win 0x68a4f4
    void SelectValue(String value);

    virtual void OnFinalRelease();

    String                                m_key;            // +0x224
    String                                m_label;          // +0x228
    BasicArray<Pair<String, String> >     m_items;          // +0x22c
    bool                                  m_changed;        // +0x23c
    String                                m_remembered;     // +0x240
    String                                m_value;          // +0x244
    RefPointer<DropDownControl>           m_dropDown;       // +0x248
    RefPointer<RenderControl>             m_labelControl;   // +0x24c
    BasicArray<RefPointer<Control> >      m_controls;       // +0x250 (not used)
                                                            // Object +0x260
};

} // namespace LEGO
#endif
