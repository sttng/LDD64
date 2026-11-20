// LEGO Digital Designer — restored source
// LEGO::SlidingHideAugmentation — slides its control out of / into view along one axis: anchored
// to its parent's edge (SetAxisAlignment), the anchor offset moves between 0 and the control's
// size over a duration (MainUpdater); fully hidden, the control is SetShown(false). Lua:
// LuaSlidingHideAugmentation (TU 389). SlidingHideAugmentationObserver: slide start / stop.
// TU 289 "SlidingHideAugmentation.cpp": Mac 0x29d1c0-0x29e94b, Win 0x5155f4-0x515b64; the
// observer's empty handlers are emitted with TU 288 (LDDPaneToggleControl, Mac 0x29cb10,
// 0x29cb20), inline here.
//
// RTTI (Mac): SlidingHideAugmentation vmi, bases ControlAugmentation (+0x00), UpdateObserver
//   (+0x18), Notifier<SlidingHideAugmentationObserver> (+0x20), virtual base Object.
//   Primary vtable 0x74225c {GetControl 0x113da0, Attach 0x29d2d0, Detach 0x29d350, D1 0x29dd80,
//   D0 0x29df00, Start 0x29d390, Stop 0x29d930, IsSliding 0x29dcb0, OnFinalRelease 0x29e0d0,
//   Update 0x29e240} (the last two Mac-only primary entries); UpdateObserver-in 0x742290
//   {0x29dde0, 0x29df70, 0x29e1e0, 0x1e380, 0x1e390, Update thunk 0x29e2e0}; Notifier-in 0x7422b4
//   {0x29de40, 0x29dfe0, 0x29e200}; Object-in 0x7422d8 {0x29dea0, 0x29e050, 0x29e220, AddRef,
//   Release}. Ctor (C2, VTT) Mac 0x29d1c0 (used by LuaSlidingHideAugmentation).
//   Layout Mac: ControlAugmentation +0x00..+0x18, UpdateObserver +0x18 (subject +0x1c),
//   Notifier +0x20 (observers +0x24, iterators +0x34), members +0x44.., Object +0x64 (sizeof 0x6c).
//   Win: vtable 0xa28948 {GetControl 0x52f4b0, Attach 0x51561e, Detach 0x51564d, Start 0x51565d,
//   Stop 0x515747, IsSliding 0x5157c0}, UpdateObserver-in 0xa28964 at +0x10 {OnAttached,
//   OnDetached, Update 0x515844}, Notifier at +0x1c, Object-in 0xa28974 at +0x64 {sdd 0x515b7d,
//   OnFinalRelease 0x515b6d}; members +0x40.. (alignment +0x40, axis +0x44, +0x48, +0x4c,
//   start +0x50, end +0x58).
//   SlidingHideAugmentationObserver: Mac 0x74218c {D1 0x29ca70, D0 0x29caa0, OnFinalRelease
//   0x29c8e0, OnAttached 0x29caf0, OnDetached 0x29cb00, OnSlideStarted 0x29cb10,
//   OnSlideStopped 0x29cb20}; Win 0x9dd1e0 {4 × 0x65a12a (empty, ICF)}.
// Method and member names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Notifier.h"
#include "Updater.h"
#include "ControlAugmentation.h"

namespace LEGO {

class Control;
class SlidingHideAugmentation;

class SlidingHideAugmentationObserver
    : public Observer<SlidingHideAugmentation, SlidingHideAugmentationObserver> {
public:
    virtual ~SlidingHideAugmentationObserver() {}                           // Mac 0x29ca70/0x29caa0
    // A slide of `control` started / stopped (the augmentation's control is passed).
    virtual void OnSlideStarted(Control* /*control*/) {}                    // Mac 0x29cb10, Win slot 2
    virtual void OnSlideStopped(Control* /*control*/) {}                    // Mac 0x29cb20, Win slot 3
};

class SlidingHideAugmentation : public ControlAugmentation,
                                public UpdateObserver,
                                public Notifier<SlidingHideAugmentationObserver> {
public:
    // Alignment 1 (start edge), axis 0 (horizontal), not sliding. Mac 0x29d1c0 (C2)
    SlidingHideAugmentation();
    virtual ~SlidingHideAugmentation();                                     // Mac 0x29dd80/0x29df00 (body 0x29dcc0), Win sdd 0x5155f4 (body 0x5157c8)

    // Anchors `control` on the axis (offset 0), then ControlAugmentation::Attach.
    virtual bool Attach(Control* control, const String& name);              // Mac 0x29d2d0, Win 0x51561e
    // Stop, then ControlAugmentation::Detach.
    virtual void Detach();                                                  // Mac 0x29d350, Win 0x51564d

    // Starts sliding over `duration` milliseconds: hides a visible control, else shows it (shown
    // first). Ignored while sliding or when detached. Observers: OnSlideStarted.
    virtual void Start(UInt64 duration);                                    // Mac 0x29d390, Win 0x51565d
    // Ends the slide at its end position. Observers: OnSlideStopped.
    virtual void Stop();                                                    // Mac 0x29d930, Win 0x515747
    // Registered with the MainUpdater (a slide is running).
    virtual bool IsSliding() const;                                         // Mac 0x29dcb0, Win 0x5157c0

    // Advances the slide (UpdateObserver); Stop once the end time has passed.
    virtual void Update();                                                  // Mac 0x29e240 (thunk 0x29e2e0), Win 0x515844

    // Edge alignment (1 start, 3 end; Control::SetAxisAlignment) and axis (0 horizontal,
    // 1 vertical). Plain member access (inlined in LuaSlidingHideAugmentation).
    UInt32 GetAlignment() const { return m_alignment; }
    void   SetAlignment(UInt32 alignment) { m_alignment = alignment; }
    UInt32 GetAxis() const { return m_axis; }
    void   SetAxis(UInt32 axis) { m_axis = axis; }
    // 0 = in place, 1 = slid out by the control's size.
    Real32 GetProgress() const { return m_progress; }

protected:
    // Notifier, UpdateObserver and ControlAugmentation final releases, in this order.
    virtual void OnFinalRelease();                                          // Mac 0x29e0d0, Win 0x515b6d

    // Offset = progress * size on the axis (negated unless alignment 1); at progress 1 after a
    // hide the control is hidden. Mac 0x29dc00, Win 0x5158d1 (ESI = this)
    void ApplyProgress();

    UInt32 m_alignment;     // Mac +0x44, Win +0x40 (1)
    UInt32 m_axis;          // Mac +0x48, Win +0x44 (0)
    bool   m_wasVisible;    // Mac +0x4c, Win +0x48 the control was visible at Start (= hiding)
    Real32 m_progress;      // Mac +0x50, Win +0x4c
    UInt64 m_startTime;     // Mac +0x54, Win +0x50 (GetMilliseconds)
    UInt64 m_endTime;       // Mac +0x5c, Win +0x58
};

} // namespace LEGO
