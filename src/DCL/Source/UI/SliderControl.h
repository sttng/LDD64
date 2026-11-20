// LEGO Digital Designer — restored source
// LEGO::SliderControl — a horizontal slider: track and thumb (two child RenderControls, images
// from one texture), a value in [minimum, maximum] snapped to a step, dragged with the left
// button; optionally kept in a preference (written on every change, read when the name is set).
// SliderControlObservers hear every change. LEGO::SliderControlObserver = ObserverN<...>.
// TU 272 "SliderControl.cpp" (Mac 0x286480-0x288c9b; Win 0x518aa7-0x519813). Directory UI/ (guess).
//
// Layout (Mac / Win): RenderControl +0x000..+0x1ec / +0x208, Notifier<SliderControlObserver>
//   +0x1ec / +0x208 (Mac: observers +0x1f0, iterations +0x200), members below, Object +0x280.
//   Mac sizeof 0x288 (ctor 0x286750 C1-with-arguments form).
// Virtual slots: Mac vtable 0x73e57c (46 entries), Win 0xa28c14 (41 entries). Overrides:
//   OnRectangleChanged (8/5), OnEnabled (12/9), OnDisabled (13/10), OnMouseEnter (16/13),
//   OnMouseLeave (17/14), OnMouseMove (18/15), OnMousePressed (19/16), OnMouseReleased (20/17),
//   Initialize (38/35 = Initialize("")). New: Initialize(const String& preferenceName) (45/40).
// Names: classes from RTTI; methods after the Lua SliderControl bindings where one fits
// ("SetMinMaxStep", "SetValue", "GetValue", "IncreaseSteps", "SetPreferenceName",
// "GetPreferenceName"); the rest inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "Geometry.h"
#include "Notifier.h"
#include "ObserverN.h"
#include "BitmapMask.h"
#include "RenderControl.h"

namespace LEGO {

class SliderControl;
class Texture;

// Mac vtable 0x705c8c {D1 0x6f3f0, D0 0x6f440, OnFinalRelease 0x6f4b0, OnAttached 0x6f510,
// OnDetached 0x6f520, 0x6f530 (empty)}; Win 0x9cffac (3 entries).
class SliderControlObserver : public ObserverN<SliderControl, SliderControlObserver> {
public:
    SliderControlObserver() {}
    virtual ~SliderControlObserver() {}                               // Mac 0x6f3f0/0x6f440
    // The value changed from `oldValue` to `value`. (name inferred)
    virtual void OnValueChanged(SliderControl* /*slider*/, Real32 /*value*/,
                                Real32 /*oldValue*/) {}               // Mac 5, Win 2
};

class SliderControl : public RenderControl, public Notifier<SliderControlObserver> {
public:
    // Stores the range and the value as given (no snapping, no notification).
    // Mac 0x286750 (C2 0x286480)
    SliderControl(ControlManager* manager, Real32 minimum, Real32 maximum, Real32 step, Real32 value);
    virtual ~SliderControl();                                         // Mac 0x287e00/0x288020, Win 0x519367 (sdd 0x518aa7)

    // ---- overrides (Mac slot / Win slot) ----
    // The thumb follows the value.
    virtual void OnRectangleChanged(const Vector2& oldPosition, const Vector2& oldSize); // 8/5  Mac 0x287af0, Win 0x519100
    virtual void OnEnabled();                                         // 12/9  Mac 0x287a40, Win 0x519083
    virtual void OnDisabled();                                        // 13/10 Mac 0x287a80, Win 0x5190a7
    virtual void OnMouseEnter(const Vector2& position);               // 16/13 Mac 0x287ab0, Win 0x5190ca
    virtual void OnMouseLeave(const Vector2& position);               // 17/14 Mac 0x287ad0, Win 0x5190e5
    // Dragging: the value under the mouse (only right of the track's left edge).
    virtual void OnMouseMove(const Vector2& position);                // 18/15 Mac 0x287870, Win 0x518f81
    // Left button: mouse capture, dragging.
    virtual void OnMousePressed(const Vector2& position, UInt32 button);  // 19/16 Mac 0x287830, Win 0x518f4b
    // Any button: the value under the mouse, capture released.
    virtual void OnMouseReleased(const Vector2& position, UInt32 button); // 20/17 Mac 0x287960, Win 0x519007
    // Initialize(String("")).
    virtual bool Initialize();                                        // 38/35 Mac 0x286dd0, Win 0x518c3f

    // ---- new virtual ----
    // RenderControl::Initialize, the track and thumb children (transparent, flags 7), mouse
    // input, transparent, then SetPreferenceName(preferenceName).
    virtual bool Initialize(const String& preferenceName);            // 45/40 Mac 0x286a30, Win 0x518ad7

    // ---- non-virtual ----
    // Track (size of `trackRect`, at (0, trackY)) and thumb (image `thumbUp` of `texture`); thumb
    // images for over / down / disabled; optional hit masks of `mask` (the rects, picked at the
    // points; a zero rect: none). The control then grows to its children. Mac 0x286e40, Win 0x518c82
    void SetImages(Texture* texture, const Rect& trackRect, const Rect& thumbUp,
                   const Rect& thumbOver, const Rect& thumbDown, const Rect& thumbDisabled,
                   Real32 trackY, BitmapMask* mask, const Rect& trackMaskRect,
                   const Vector2& trackMaskPoint, const Rect& thumbMaskRect,
                   const Vector2& thumbMaskPoint);
    // Range and step, then SetValue(value). Mac 0x287250, Win 0x518e7d
    void SetMinMaxStep(Real32 minimum, Real32 maximum, Real32 step, Real32 value);
    // Clamped and snapped to the step (from the minimum); a change (> 1e-5) is written to the
    // preference, moves the thumb and notifies. Mac 0x2872a0, Win 0x519167
    void SetValue(Real32 value);
    Real32 GetValue() const;                                          // Mac 0x287b90, Win 0x5192dd
    // SetValue(value + steps * step). Mac 0x287ba0, Win 0x5192b8
    void IncreaseSteps(Int32 steps);
    Real32 GetMinimum() const;                                        // Mac 0x287be0, Win 0x5192e4
    Real32 GetMaximum() const;                                        // Mac 0x287bf0, Win 0x5192eb
    Real32 GetStep() const { return m_step; }
    // Keeps the value in this preference; reads it now when it exists. Mac 0x286cd0, Win 0x5192f2
    void SetPreferenceName(const String& name);
    const String& GetPreferenceName() const;                          // Mac 0x287c00, Win 0x519360
    RenderControl* GetTrack() const { return m_track; }
    RenderControl* GetThumb() const { return m_thumb; }

protected:
    virtual void OnFinalRelease();                                    // 2/– Mac 0x288290

    // Thumb image for the mouse at `position`. Mac 0x287740, Win 0x518eab
    void UpdateThumbImage(const Vector2& position);
    // Thumb x from the value. (inlined on Mac; Win 0x51911d)
    void UpdateThumbPosition();
    // Value at local x `x` (snapped to the step without the minimum; inlined).
    Real32 ValueAt(Real32 x) const;

    // ---- data (Mac offset / Win offset) ----
    RefPointer<RenderControl> m_track;           // +0x210 / +0x22c
    RefPointer<RenderControl> m_thumb;           // +0x214 / +0x230
    RefPointer<Texture>       m_texture;         // +0x218 / +0x234
    Rect                      m_thumbUp;         // +0x21c
    Rect                      m_thumbOver;       // +0x22c (zero: the thumb image never changes)
    Rect                      m_thumbDown;       // +0x23c
    Rect                      m_thumbDisabled;   // +0x24c
    Real32                    m_minimum;         // +0x25c / +0x278
    Real32                    m_maximum;         // +0x260 / +0x27c
    Real32                    m_step;            // +0x264 / +0x280
    Real32                    m_value;           // +0x268 / +0x284
    String                    m_preferenceName;  // +0x26c / +0x288
    bool                      m_dragging;        // +0x27c / +0x28c
                                                 // Object +0x280
};

} // namespace LEGO
