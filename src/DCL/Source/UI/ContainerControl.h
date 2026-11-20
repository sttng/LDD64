// LEGO Digital Designer — restored source
// LEGO::ContainerControl — a scrolling viewport: a clip control (inside the margins and scroll
// bars) holds one content control; optional scroll bars (a container with a slider thumb driven
// by SliderAugmentation "tab"), scroll buttons (±32 per click) and a corner control; the wheel
// over any control inside scrolls (vertical 40, horizontal 10 per notch). Base of
// ScrollContainerControl, LDDPaneContainerControl, LuaContainerControl's controls …
// Part of TU 114 (Mac 0x1c82a0-0x1ca7b0 + 0x1cd0f0; Win 0x511381-0x5122a3, 0x5c96ec-0x5ca0e6),
// compiled there with SliderAugmentation. Mac ctor inlined into the subclasses'; Win 0x5c8f55.
//
// Bases: BufferedAbleRenderControl (+0), ControlObserver (Mac +0x194 / Win +0x1a8),
//   SliderAugmentationObserver (+0x1a8 / +0x1c0), ControlManagerObserver (+0x1bc / +0x1d8),
//   members below, Object +0x20c / +0x230 (Win sizeof 0x238).
// Virtual slots: Mac vtable 0x71f9bc (46 entries), Win 0x9c9e5c (37 entries). Overrides:
//   OnRectangleChanged (8/5). New: Initialize (38/35), Layout (39/36); Mac-only entries 40-45 =
//   the overriders of ControlObserver 9 / 10 / 24 / 22, SliderAugmentationObserver 5 and
//   ControlManagerObserver 12.
// Names inferred (no strings); "content", "OnContentPositionChanged" etc. are LuaContainerControl's.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Geometry.h"
#include "Control.h"
#include "ControlManager.h"
#include "BufferedAbleRenderControl.h"
#include "SliderAugmentation.h"

namespace LEGO {

class ContainerControl : public BufferedAbleRenderControl,
                         public ControlObserver,
                         public SliderAugmentationObserver,
                         public ControlManagerObserver {
public:
    // Win 0x5c8f55 (Mac inlined): no controls, margins 0, both "scroll always" flags set.
    explicit ContainerControl(ControlManager* manager);
    virtual ~ContainerControl();                                      // Mac 0x1ca6e0/0x1ca740, Win 0x5c9153 (sdd 0x5c96ec)

    // ---- Control override ----
    // Layout, then the content position (unless both scroll-always flags are set).
    virtual void OnRectangleChanged(const Vector2& oldPosition, const Vector2& oldSize); // 8/5 Mac 0x1ca0f0, Win 0x51208a

    // ---- new virtuals ----
    // Enabled, shown, active; the clip control (child, size of the container, mouse input);
    // observes the clip control and the manager.
    virtual bool Initialize();                                        // 38/35 Mac 0x1c82a0, Win 0x511381
    // Scroll ranges from the content size (scaled) and the clip size, thumb sizes (at least 15),
    // scroll bars shown only when needed.
    virtual void Layout();                                            // 39/36 Mac 0x1c99a0, Win 0x5118e4

    // ---- overriders of the observer bases (Mac primary entries 40-45) ----
    // The content's rectangle / scale changed: Layout.
    virtual void OnRectangleChanged(Control* control, const Vector2& oldPosition,
                                    const Vector2& oldSize);          // 40/– Mac 0x1c9e70 (thunk 0x1c9e90), Win 0x511f61
    virtual void OnScaleChanged(Control* control, const Vector2& oldScale); // 41/– Mac 0x1c9ec0 (thunk 0x1c9ee0), Win 0x511f7b
    // Wheel over the control under the mouse: axis 0 scrolls vertically (40 per unit), axis 1
    // horizontally (10).
    virtual void OnMouseWheel(Control* control, UInt32 axis, Real32 delta); // 42/– Mac 0x1c9f10 (thunk 0x1c9fa0), Win 0x511f95
    // Scroll buttons: -32 / +32.
    virtual void OnMouseClicked(Control* control, const Vector2& position, UInt32 button); // 43/– Mac 0x1ca030 (thunk 0x1ca0d0), Win 0x51201b
    // A scroll bar moved: the content follows.
    virtual void OnSliderValueChanged(SliderAugmentation* slider, Real64 value, UInt32 axis); // 44/– Mac 0x1ca130 (thunk 0x1ca170), Win 0x5120bf
    // Observes the control under the mouse while it is inside the container (wheel).
    virtual void OnManagerMouseOverChanged(Control* previous, Control* current); // 45/– Mac 0x1ca1a0 (thunk 0x1ca230), Win 0x5120e6
    // (the other handlers of these names stay the defaults)
    using BufferedAbleRenderControl::OnScaleChanged;
    using BufferedAbleRenderControl::OnMouseWheel;
    using BufferedAbleRenderControl::OnMouseClicked;

    // ---- non-virtual ----
    // The content (child of the clip control at (0, 0), shown, enabled, active, observed); the
    // scroll values back to 0; Layout. The previous content is hidden.
    void SetContent(Control* content);                                // Mac 0x1c8630, Win 0x51141d
    Control* GetContent() const { return m_content; }
    Control* GetClipControl() const { return m_clip; }
    // A scroll bar thumb `bar` (`thickness`: the bar's height / width) in a new bar container, with
    // SliderAugmentation "tab". Mac 0x1c8770, Win 0x5114c6
    void SetScrollBar(Control* bar, Real32 thickness, bool vertical);
    // The scroll buttons (up / left = `decrease`). Mac 0x1c96e0, Win 0x51179e
    void SetScrollButtons(Control* decrease, Control* increase, bool vertical);
    // The control in the corner between the bars. Mac 0x1c9890, Win 0x51186d
    void SetCorner(Control* corner);
    // Margins of the clip area. Mac 0x1c9930, Win 0x5118b0
    void SetMargins(Real32 left, Real32 top, Real32 right, Real32 bottom);
    // The content follows the scroll bar even when it fits (else it is aligned to the far edge).
    // Mac 0x1c9970
    void SetScrollAlways(bool vertical, bool scrollAlways);
    SliderAugmentation* GetSlider(bool vertical) const { return vertical ? m_verticalSlider : m_horizontalSlider; }

protected:
    // Observers leave, then Control::OnFinalRelease.
    virtual void OnFinalRelease();                                    // 2/– Mac 0x1ca7b0, Win 0x5ca0e6

    // Anchors of the clip control, the bars, buttons and corner; then Layout. Mac 0x1c93a0, Win 0x511ccd
    void LayoutScrollBars();
    // The content position from the slider values. Mac 0x1c9d80, Win 0x511bfe
    void UpdateContentPosition();
    // New value = the slider's value on its own axis + delta.
    static void Scroll(SliderAugmentation* slider, Real64 delta);

    // ---- data (Mac offset / Win offset) ----
    RefPointer<Control>            m_content;             // +0x1c4 / +0x1e4
    RefPointer<Control>            m_clip;                // +0x1c8 / +0x1e8
    RefPointer<Control>            m_corner;              // +0x1cc / +0x1ec
    RefPointer<Control>            m_horizontalBar;       // +0x1d0 / +0x1f0 bar container
    RefPointer<Control>            m_horizontalThumb;     // +0x1d4 / +0x1f4
    RefPointer<Control>            m_leftButton;          // +0x1d8 / +0x1f8
    RefPointer<Control>            m_rightButton;         // +0x1dc / +0x1fc
    RefPointer<SliderAugmentation> m_horizontalSlider;    // +0x1e0 / +0x200
    RefPointer<Control>            m_verticalBar;         // +0x1e4 / +0x204
    RefPointer<Control>            m_verticalThumb;       // +0x1e8 / +0x208
    RefPointer<Control>            m_upButton;            // +0x1ec / +0x20c
    RefPointer<Control>            m_downButton;          // +0x1f0 / +0x210
    RefPointer<SliderAugmentation> m_verticalSlider;      // +0x1f4 / +0x214
    Real32                         m_marginLeft;          // +0x1f8 / +0x218
    Real32                         m_marginTop;           // +0x1fc / +0x21c
    Real32                         m_marginRight;         // +0x200 / +0x220
    Real32                         m_marginBottom;        // +0x204 / +0x224
    bool                           m_scrollAlwaysH;       // +0x208 / +0x228 true
    bool                           m_scrollAlwaysV;       // +0x209 / +0x229 true
                                                          // Object +0x20c / +0x230
};

} // namespace LEGO
