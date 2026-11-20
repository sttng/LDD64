// LEGO Digital Designer — restored source
// LEGO::LDDPaneToggleControl — the narrow "<<" / ">>" handle on the edge of the edit mode's side
//   pane (LDDPaneContainerControl): a click runs the show/hide command (AnimateShowHideControl-
//   Command, which slides the pane through its SlidingHideAugmentation); when a slide stops the
//   arrow flips (toggled state). Images from "Graphics/LDDgraphics01.png" (13 x 28 px, a left
//   and a right variant per state).
//
// TU 288 "LDDPaneToggleControl.cpp": Mac 0x29bb40-0x29d1bb, Win 0x6924c6-0x692cc4. The TU also
// emits the Observer<SlidingHideAugmentation, SlidingHideAugmentationObserver> template code
// (Attach Mac 0x29bb40, OnFinalRelease 0x29c8e0, dtors 0x29cc30/0x29cc60), the inline
// SlidingHideAugmentationObserver bodies (SlidingHideAugmentation.h: dtors 0x29ca70/0x29caa0,
// handlers 0x29cb10/0x29cb20) and the NotifierT<SlidingHideAugmentation, …> RemoveObserver
// helpers (Mac 0x29cd50, 0x29cf40). Mac 0x29d140 = <iostream> + a header static (not code).
//
// RTTI: vmi {ToggleControl +0, SlidingHideAugmentationObserver (Mac +0x240, Win +0x25c)},
//   virtual base Object. Mac vtable 0x74172c (56 entries: ToggleControl's 54, Create 54,
//   OnSlideStopped overrider 55), observer-in 0x741878 {…, OnSlideStarted 0x29cb10 (default),
//   OnSlideStopped thunk 0x29ca30}; Win 0x9dd28c (50 entries: Create 49), observer-in 0x9dd258
//   {3 × 0x65a12a, OnSlideStopped 0x6924d3}.
// Layout (Mac / Win): ToggleControl +0x000..+0x240 / +0x25c, SlidingHideAugmentationObserver
//   +0x240 / +0x25c (subject +0x244), command +0x248 / +0x268, Object +0x24c (Mac sizeof 0x254,
//   operator new 0x254 in LDDPaneContainerControl::Create; Win 0x278).
// Method and member names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "ToggleControl.h"
#include "SlidingHideAugmentation.h"

namespace LEGO {

class Command;

class LDDPaneToggleControl : public ToggleControl, public SlidingHideAugmentationObserver {
public:
    // Watches `augmentation` (the pane's SlidingHideAugmentation); a click runs `command`.
    // Mac 0x29bdb0, Win 0x692be8
    LDDPaneToggleControl(ControlManager* manager, Command* command,
                         SlidingHideAugmentation* augmentation);
    virtual ~LDDPaneToggleControl();                                        // Mac 0x29c550/0x29c6b0, Win sdd 0x692cc4

    // ---- overrides ----
    // Runs the command (Command slot 3, Do). NOTE: no toggling here (OnSlideStopped flips).
    virtual void OnMouseClicked(const Vector2& position, UInt32 button);   // 21/18 Mac 0x29c9d0, Win 0x6924c6
    // The slide ended: the arrow flips (SetToggled(!IsToggled())).
    virtual void OnSlideStopped(Control* control);                          // 55/obs 3 Mac 0x29c9f0 (thunk 0x29ca30), Win 0x6924d3

    // ---- new virtual ----
    // ToggleControl::Initialize, Setup below `parent` behind `after` (`flags`, 15 x 103, no
    // input flags, not hit-testable), then the state images: up / over / down and their toggled
    // variants (descriptions 0-2, 4-6), the right-pointing set when `rightSide`. False without
    // the texture or when a step fails.
    virtual bool Create(Control* parent, Control* after, UInt32 flags, bool rightSide); // 54/49 Mac 0x29bf00, Win 0x692681

    using ToggleControl::Create;

protected:
    // The observer part (leaves the augmentation), then ToggleControl's (its observers, then
    // RenderControl's). Mac 0x29c830
    virtual void OnFinalRelease();

    Command* m_command;   // Mac +0x248, Win +0x268 (not referenced: the container owns it)
};

} // namespace LEGO
