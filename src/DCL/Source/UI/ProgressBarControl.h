// LEGO Digital Designer — restored source
// LEGO::ProgressBarControl — the bar of ProgressBarDialog (TU 122): a frame image and a bar
// child, both from "Graphics/LDDgraphics01.png"; the bar shows value / maximum of a 272-pixel strip.
// TU 105 "ProgressBarControl.cpp" (Mac 0x1b9480-0x1b9b6b; Win 0x6426b4-0x6427fe, 0x6876c1); the
// ctor is inline (Mac: in ProgressBarDialog 0x1d9000; Win 0x6426b4). Names inferred.
//
// Layout (Mac): RenderControl +0x000..+0x1ec, bar +0x1ec, maximum +0x1f0, value +0x1f4,
//   Object +0x1f8 (sizeof 0x200). Win: bar +0x208, maximum +0x20c, value +0x210, Object +0x218.
// Virtual slots: Mac 0x71e2fc (RenderControl's 45 + Create 45); Win 0x9d4394 (40 + Create 40).
#pragma once
#include "LEGO/Types.h"
#include "RenderControl.h"

namespace LEGO {

class ProgressBarControl : public RenderControl {
public:
    // Win 0x6426b4 (Mac inlined)
    explicit ProgressBarControl(ControlManager* manager)
        : RenderControl(manager), m_maximum(0.0f), m_value(0.0f) {}
    virtual ~ProgressBarControl();                                 // Mac 0x1b98d0/0x1b9990, Win 0x642779/sdd 0x6427fe

    // Initialize, Setup below `parent` (flags 7, empty rectangle), load the image and create
    // the bar; false when a step fails. Mac 0x1b9480, Win 0x6876c1 (slot 45 / 40)
    virtual bool Create(Control* parent, Control* after);

    // Set inline by ProgressBarDialog (Mac 0x1d98c0) before SetValue.
    void SetMaximum(Real32 maximum) { m_maximum = maximum; }
    Real32 GetMaximum() const { return m_maximum; }
    // Clamped to the maximum; updates the bar. Mac 0x1b98a0
    void SetValue(Real32 value);
    Real32 GetValue() const { return m_value; }

protected:
    // The bar's image rectangle: (288, 475) + (round(value / maximum * 272), 25). Mac 0x1b97b0
    void UpdateBar();

    // ---- data (Mac / Win offsets) ----
    RefPointer<RenderControl> m_bar;       // +0x1ec / +0x208
    Real32                    m_maximum;   // +0x1f0 / +0x20c 0
    Real32                    m_value;     // +0x1f4 / +0x210 0
                                           // Object +0x1f8 / +0x218
};

} // namespace LEGO
