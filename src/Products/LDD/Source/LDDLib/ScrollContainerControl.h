// LEGO Digital Designer — restored source
// LEGO::ScrollContainerControl — LDD's ContainerControl with the standard scroll bars of
//   "Graphics/EditScreen/EditMode_Final.png" (Create builds the frame, the bar tracks, thumbs,
//   arrow buttons and the corner) and the scroll sound "IA_scroll.wav" (played on wheel, arrow
//   clicks and every scroll value change; reloaded when the PLAYSOUND preference changes).
//
// TU 143 "ScrollContainerControl.cpp" (Mac 0x204d90-0x20786b; Win 0x608091-0x609097). Directory:
// LDDLib (it uses LDDUtilities, an application class). The Mac constructor is not in the TU
// (inlined into its callers); Win 0x5c929d (with ContainerControl's 0x5c8f55).
//
// Bases: ContainerControl (+0), PreferencesObserver (Mac +0x20c, subject +0x210; Win +0x22c),
//   members below, Object Mac +0x240 (sizeof 0x248), Win +0x264.
// Virtual slots: Mac vtable 0x729acc (48 entries), Win 0x9c9ffc (38 entries). Overrides of
//   ContainerControl's Mac-only overrider entries 42 (wheel), 43 (clicked), 44 (slider value);
//   new: Create (46/37) and the PreferencesObserver overrider (47/–).
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Preferences.h"
#include "ContainerControl.h"
#include "RenderControl.h"
#include "ButtonControl.h"

namespace LEGO {

class SimpleSound;

class ScrollContainerControl : public ContainerControl, public PreferencesObserver {
public:
    // Which scroll bars Create builds. (names inferred)
    enum ScrollBars {
        kVertical   = 0,
        kHorizontal = 1,
        kBoth       = 2
    };

    // Win 0x5c929d (Mac inlined)
    explicit ScrollContainerControl(ControlManager* manager);
    virtual ~ScrollContainerControl();                                // Mac 0x206c90/0x206cf0, Win 0x5c943a (sdd 0x5c970e)

    // ---- ContainerControl overriders (Mac primary entries) ----
    // Wheel scrolling plays the scroll sound.
    virtual void OnMouseWheel(Control* control, UInt32 axis, Real32 delta); // 42/– Mac 0x206910 (thunk 0x206970), Win 0x608091
    // Arrow clicks play it.
    virtual void OnMouseClicked(Control* control, const Vector2& position, UInt32 button); // 43/– Mac 0x2069f0 (thunk 0x206a40), Win 0x6080f7
    // Every scroll value change plays it.
    virtual void OnSliderValueChanged(SliderAugmentation* slider, Real64 value, UInt32 axis); // 44/– Mac 0x206ab0 (thunk 0x206ae0), Win 0x608137
    using ContainerControl::OnMouseWheel;
    using ContainerControl::OnMouseClicked;

    // ---- new virtual ----
    // Below `parent`: the scroll sound, the EditMode texture; with `background` a dark frame
    // (0.2 grey) around a white inner area (1 pixel inset); the bar thumbs (16 thick), arrow
    // buttons (four states each), bar tracks, the corner (kBoth, 0.85 grey); ContainerControl
    // Initialize; shown / enabled / active; margins (1, 1, 1 without a vertical track, 1
    // without a horizontal one); observes the preferences. False when a control fails to
    // initialize or the texture is missing.
    virtual bool Create(UInt32 scrollBars, bool background, Control* parent); // 46/37 Mac 0x204d90, Win 0x6081f2
    // PLAYSOUND changed: the scroll sound is reloaded.
    virtual void OnPreferenceChanged(const String& key);              // 47/– Mac 0x206b40 (thunk 0x206c70), Win 0x608165

protected:
    // Leaves the preferences, then ContainerControl::OnFinalRelease.
    virtual void OnFinalRelease();                                    // 2/– Mac 0x206d60

    void PlayScrollSound();

    // ---- data (Mac offset / Win offset) ----
    RefPointer<RenderControl> m_frame;              // +0x214 / +0x238 dark background
    RefPointer<RenderControl> m_frameInner;         // +0x218 / +0x23c
    RefPointer<RenderControl> m_verticalTrack;      // +0x21c / +0x240
    RefPointer<RenderControl> m_verticalThumbImage; // +0x220 / +0x244
    RefPointer<RenderControl> m_horizontalTrack;    // +0x224 / +0x248
    RefPointer<RenderControl> m_horizontalThumbImage; // +0x228 / +0x24c
    RefPointer<ButtonControl> m_upArrow;            // +0x22c / +0x250
    RefPointer<ButtonControl> m_downArrow;          // +0x230 / +0x254
    RefPointer<ButtonControl> m_leftArrow;          // +0x234 / +0x258
    RefPointer<ButtonControl> m_rightArrow;         // +0x238 / +0x25c
    RefPointer<SimpleSound>   m_scrollSound;        // +0x23c / +0x260
                                                    // Object +0x240 / +0x264
};

} // namespace LEGO
