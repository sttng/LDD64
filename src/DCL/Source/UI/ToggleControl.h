// LEGO Digital Designer — restored source
// LEGO::ToggleControl — a ButtonControl with an on/off state: a left click toggles it (or only
// switches it on when it cannot be untoggled, radio style); toggled, the states show
// descriptions 4-7 ("ToggledUp/Over/Down/Disabled") instead of 0-3. ToggleControlObservers hear
// OnToggled / OnUntoggled. LEGO::ToggleControlObserver = ObserverN<ToggleControl, ...>.
// TU 089 "ToggleControl.cpp" (Mac 0x1a5c00-0x1a724b; Win 0x51818c-0x518624). Directory UI/ (guess).
//
// Layout (Mac / Win): ButtonControl +0x000..+0x218 / +0x238,
//   Notifier<ToggleControlObserver> +0x218 / +0x234 (Mac: observers +0x21c, iterations +0x22c),
//   bool m_toggled +0x23c / +0x258, bool m_canUntoggle +0x23d / +0x259, Object +0x240 / +0x260.
//   Mac sizeof 0x248. Win ctor 0x518270.
// Virtual slots: Mac vtable 0x71817c (54 entries), Win 0x9d1874 (49 entries). Overrides:
//   OnMouseClicked (21/18), Initialize (38/35). New: IsToggled (48/44), SetToggled (49/43) — an
//   overload-style pair (MSVC order reversed), SetCanUntoggle (50/45), CanUntoggle (51/46),
//   NotifyToggled (52/47), NotifyUntoggled (53/48).
// Names: classes from RTTI, OnToggled (Lua callback); the rest inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Notifier.h"
#include "ObserverN.h"
#include "ButtonControl.h"

namespace LEGO {

class ToggleControl;

// Mac vtable 0x705a4c {D1 0x6eb80, D0 0x6ebd0, OnFinalRelease 0x6ec40, OnAttached 0x6eca0,
// OnDetached 0x6ecb0, 0x6ecc0, 0x6ecd0 (both empty)}; Win 0x9d5e4c (4 entries, all 0x40be80).
class ToggleControlObserver : public ObserverN<ToggleControl, ToggleControlObserver> {
public:
    ToggleControlObserver() {}
    virtual ~ToggleControlObserver() {}                               // Mac 0x6eb80/0x6ebd0
    virtual void OnToggled(ToggleControl* /*control*/) {}             // Mac 5, Win 2 (Lua name)
    virtual void OnUntoggled(ToggleControl* /*control*/) {}           // Mac 6, Win 3 (name inferred)
};

class ToggleControl : public ButtonControl, public Notifier<ToggleControlObserver> {
public:
    // Untoggled, can be untoggled. Mac 0x1a5cf0 (C2 0x1a5c00), Win 0x518270
    explicit ToggleControl(ControlManager* manager);
    virtual ~ToggleControl();                                         // Mac 0x1a6620/0x1a66e0, Win 0x51825a (sdd 0x518238)

    // ---- overrides ----
    // Left button: toggles (SetToggled(!toggled)), or only toggles on when it cannot be untoggled.
    virtual void OnMouseClicked(const Vector2& position, UInt32 button); // 21/18 Mac 0x1a6100, Win 0x518463
    // Eight state descriptions (cursors of 4-7: 1, 2, 3, 1), then ButtonControl::Initialize.
    virtual bool Initialize();                                        // 38/35 Mac 0x1a5df0, Win 0x518338

    // ---- new virtuals ----
    virtual bool IsToggled() const;                                   // 48/44 Mac 0x1a6010, Win 0x5183c2
    // Maps the states to descriptions 4-7 (toggled) or 0-3, then NotifyToggled / NotifyUntoggled.
    virtual void SetToggled(bool toggled);                            // 49/43 Mac 0x1a6020, Win 0x5183c9
    virtual void SetCanUntoggle(bool canUntoggle);                    // 50/45 Mac 0x1a60d0, Win 0x51844f
    virtual bool CanUntoggle() const;                                 // 51/46 Mac 0x1a60f0, Win 0x51845c
    virtual void NotifyToggled();                                     // 52/47 Mac 0x1a6790, Win 0x51818c
    virtual void NotifyUntoggled();                                   // 53/48 Mac 0x1a6a20, Win 0x5181e2

protected:
    // Detaches the observers, then RenderControl::OnFinalRelease (ButtonControl has none).
    virtual void OnFinalRelease();                                    // 2/– Mac 0x1a6720

    bool m_toggled;       // +0x23c / +0x258
    bool m_canUntoggle;   // +0x23d / +0x259
                          // Object +0x240 / +0x260
};

} // namespace LEGO
