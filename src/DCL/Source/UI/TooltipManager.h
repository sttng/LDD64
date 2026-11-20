// LEGO Digital Designer — restored source
// LEGO::TooltipAugmentation — a control's "tooltip" augmentation: creates / updates / destroys the
// tooltip control (derived: StandardTextTooltipAugmentation (2.4), ExtendedTextTooltipAugmentation,
// LuaTooltipAugmentation …). LEGO::TooltipManager — ControlManagerAugmentation "TooltipManager":
// when the mouse enters a control with a tooltip it waits GetDelay() ms (Trigger), shows the
// tooltip in layer 2 below the mouse, hides it on leave; follows preference SHOWTOOLTIPS.
// TU 049 "TooltipManager.cpp" (Mac 0x111cf0-0x11437b; Win 0x44b470-0x44b65f, 0x512ee0-0x5138f1),
// with ControlAugmentation / ControlManagerAugmentation, Trigger and BasicArray<RefPointer<Control> >.
//
// TooltipAugmentation (RTTI si : ControlAugmentation): Mac vtable 0x71032c {GetControl, Attach,
//   Detach, D1 0x1121f0, D0 0x1122a0, GetDelay 0x112320, CreateTooltip 0x112330, DestroyTooltip
//   0x112340, UpdateTooltip 0x112350}; Win {GetControl, Attach, Detach, 0x513175, 0x513178,
//   0x51317d, …}, sdd 0x513115. Mac layout: ControlAugmentation +0..+0x18, TooltipManager*
//   +0x18, Control* anchor +0x1c, Object +0x20. Win: manager +0x10, anchor +0x14, Object +0x18.
// TooltipManager (RTTI vmi): ControlManagerAugmentation +0x00, ControlManagerObserver +0x18
//   (subject = the manager +0x1c), Trigger +0x20 (updater +0x24, due time +0x28),
//   PreferencesObserver +0x30 (subject +0x34), RefPointer<Control> tooltip +0x38, +0x3c (zeroed,
//   unused), RefPointer<TooltipAugmentation> +0x40, BasicArray<RefPointer<Control> > +0x44,
//   bool showTooltips +0x54 (true), Object +0x58. Mac primary vtable 0x70add8 {D1, D0,
//   OnFinalRelease 0x112d30, OnManagerAttached 0x112f30, OnManagerDetached 0x1131c0, then the
//   Mac-only entries of the overriders of secondary-base virtuals: OnManagerMouseEnter 0x113210,
//   OnManagerMouseLeave 0x113440, OnManagerMouseMove 0x113580, Observer::OnDetached 0x113770,
//   Trigger::OnTrigger 0x1137a0, OnPreferenceChanged 0x1138e0, Observer<Preferences>::OnAttached
//   0x1139f0}. Win: ctor 0x513180 (this+vbptr form), dtor 0x513316 (sdd 0x5132ec).
// Class names from RTTI; method and member names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicArray.h"
#include "ControlAugmentation.h"
#include "Control.h"
#include "ControlManager.h"
#include "Preferences.h"
#include "Trigger.h"

namespace LEGO {

class TooltipManager;

typedef BasicArray<RefPointer<Control> > ControlRefArray;   // (name inferred) Mac vtable 0x764318

// ===========================================================================================
class TooltipAugmentation : public ControlAugmentation {
public:
    TooltipAugmentation() : m_manager(0), m_anchor(0) {}                   // Mac 0x112130
    virtual ~TooltipAugmentation();                                         // Mac 0x1121f0/0x1122a0, Win 0x51313f (sdd 0x513115)

    // Milliseconds before the tooltip appears; 0 = at once. Mac 0x112320, Win 0x513175
    virtual UInt32 GetDelay() { return 0; }
    // Creates the tooltip control into `tooltip` (and any helper controls into `extra`, kept
    // alive while it is shown); false: none. Mac 0x112330, Win 0x513178
    virtual bool CreateTooltip(RefPointer<Control>& /*tooltip*/, ControlRefArray& /*extra*/) { return false; }
    // The tooltip goes away. Mac 0x112340, Win 0x51317d
    virtual void DestroyTooltip(Control* /*tooltip*/, ControlRefArray& /*extra*/) {}
    // Updates the shown tooltip; true keeps it (repositioned), false destroys it. Mac 0x112350
    virtual bool UpdateTooltip(Control* /*tooltip*/, ControlRefArray& /*extra*/) { return false; }

    // Becomes `control`'s "tooltip" augmentation (detaching an existing one); with the mouse over
    // the control, the tooltip manager switches to it at once. Mac 0x111cf0, Win 0x512ffe
    bool AttachTo(Control* control);
    // Hides a shown tooltip, then leaves the control. Mac 0x111f80, Win 0x5130e5
    void Remove();
    // The tooltip content changed: the manager updates (or creates) it. Mac 0x111ff0, Win 0x5130fc
    void Refresh();
    // The tooltip is placed inside this control's rectangle instead of the window's layer 1.
    // Mac 0x112120, Win 0x51310b
    void SetAnchor(Control* anchor) { m_anchor = anchor; }
    Control* GetAnchor() const { return m_anchor; }
    TooltipManager* GetTooltipManager() const { return m_manager; }

protected:
    friend class TooltipManager;

    TooltipManager* m_manager;   // Mac +0x18 the manager showing this augmentation's tooltip
    Control*        m_anchor;    // Mac +0x1c
};

// ===========================================================================================
class TooltipManager : public ControlManagerAugmentation,
                       public ControlManagerObserver,
                       public Trigger,
                       public PreferencesObserver {
public:
    // Named "TooltipManager", attached to `manager`, observes the Preferences. Mac 0x112500,
    // Win 0x513180
    explicit TooltipManager(ControlManager* manager);
    virtual ~TooltipManager();                                              // Mac 0x112910/0x112af0, Win 0x513316

    // Shows the current augmentation's tooltip now (Trigger::OnTrigger): stops the timer; with
    // SHOWTOOLTIPS on and a created tooltip control, puts it into layer 2, positions and shows it;
    // otherwise drops it. Mac 0x1137b0
    void Show();
    // Drops the tooltip and the augmentation (DestroyTooltip). Mac 0x113450
    void Hide();
    // The augmentation's content changed (see TooltipAugmentation::Refresh). Mac 0x112010
    void Refresh();
    // Places the tooltip below the mouse (+20), inside the anchor (or layer 1). Mac 0x1135a0,
    // Win 0x5134df
    void Position();
    // Makes `augmentation` the current one and refreshes. (inlined: Mac 0x111cf0 / 0x113210,
    // Win 0x5135c7)
    void SetAugmentation(TooltipAugmentation* augmentation);

    Control* GetTooltip() const { return m_tooltip.Get(); }
    TooltipAugmentation* GetAugmentation() const { return m_augmentation.Get(); }
    bool IsEnabled() const { return m_showTooltips; }

protected:
    virtual void OnFinalRelease();                                          // Mac 0x112d30, Win 0x5133c2
    // Observes the manager / stops. Mac 0x112f30 / 0x1131c0, Win 0x5133e7 / 0x5133ef
    virtual void OnManagerAttached(ControlManager* manager);
    virtual void OnManagerDetached(ControlManager* manager);
    // ControlManagerObserver: the control under the mouse has a "tooltip" augmentation → it
    // becomes the current one; leaving hides; moving re-positions. Mac 0x113210 / 0x113440 /
    // 0x113580, Win 0x5133fa / 0x5134d4 / …
    virtual void OnManagerMouseEnter(Control* control, const Vector2& position);
    virtual void OnManagerMouseLeave(Control* control, const Vector2& position);
    virtual void OnManagerMouseMove(const Vector2& position);
    // Observer<ControlManager>::OnDetached: hides. Mac 0x113770 (thunk 0x113780), Win 0x5134f2
    virtual void OnDetached(ControlManager* manager);
    // Trigger: Show. Mac 0x1137a0 (thunk 0x1138c0), Win 0x5134fd
    virtual void OnTrigger();
    // PreferencesObserver: SHOWTOOLTIPS changed (off hides). Mac 0x1138e0, Win 0x513505
    virtual void OnPreferenceChanged(const String& key);
    // Observer<Preferences>::OnAttached: reads SHOWTOOLTIPS. Mac 0x1139f0, Win 0x513584
    virtual void OnAttached(Preferences* preferences);
    // (the other bases' equally named handlers stay visible)
    using Observer<ControlManager, ControlManagerObserver>::OnAttached;
    using Observer<Updater, UpdateObserver>::OnAttached;
    using Observer<Updater, UpdateObserver>::OnDetached;
    using Observer<Preferences, PreferencesObserver>::OnDetached;

    // Detaches the tooltip control and releases it and the extras. (inlined: Mac 0x112010,
    // 0x113450, 0x1137b0)
    void DropTooltip();
    // `control`'s "tooltip" augmentation as a TooltipAugmentation. Mac 0x113310
    static bool FindTooltipAugmentation(Control* control, RefPointer<TooltipAugmentation>& augmentation);

    // ---- data (Mac offsets) ----
    RefPointer<Control>             m_tooltip;       // +0x38
    UInt32                          m_value3c;       // +0x3c 0 (unused)
    RefPointer<TooltipAugmentation> m_augmentation;  // +0x40
    ControlRefArray                 m_extra;         // +0x44
    bool                            m_showTooltips;  // +0x54 true (preference SHOWTOOLTIPS)
                                                     // Object +0x58
};

} // namespace LEGO
