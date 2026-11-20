// LEGO Digital Designer — restored source
// LEGO::StretchableControl — a 9-slice RenderControl built from nine anchored child
// RenderControls showing the parts of one texture (corners fixed, edges stretch along one axis,
// the centre both).
// TU 080 "StretchableControl.cpp": Mac 0x16bf70-0x16d28b, Win 0x51983c (+ dtor 0x627bf6 / sdd
// 0x62d07e). The ctor is inline (derived classes have construction vtables, Mac 0x713bcc …).
//
// RTTI (Mac): si-like (one public base RenderControl), virtual base Object.
//   Primary vtable 0x71312c (46 entries): RenderControl's 0..44 with D1 0x16cbe0 / D0 0x16cc40,
//   new slot 45 Create 0x16bf70; TextLayoutObserver-in 0x7131f0 {0x16ccb0, 0x16cd20, …},
//   TextureObserver-in 0x713214 {0x16cda0, 0x16ce10, …}, Object-in 0x713244 {0x16ce90, 0x16cf00,
//   0xec860, AddRef, Release}. Mac layout: RenderControl +0x000..+0x1ec, parts +0x1ec..+0x20c,
//   Object +0x210 (sizeof 0x218).
//   Win: vtable 0x9d056c (41 entries, new slot 40 = Create 0x51983c); parts +0x208..+0x228;
//   Object-in 0x9d0538 at +0x230 {sdd 0x627bf6 …}.
// Method and member names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Geometry.h"
#include "Texture.h"
#include "RenderControl.h"

namespace LEGO {

class StretchableControl : public RenderControl {
public:
    // Part order of m_parts (and of Create's rectangles).
    enum Part {
        kTopLeft, kTop, kTopRight, kLeft, kCenter, kRight, kBottomLeft, kBottom, kBottomRight,
        kPartCount
    };

    explicit StretchableControl(ControlManager* manager) : RenderControl(manager) {}
    virtual ~StretchableControl();                                          // Mac 0x16cbe0/0x16cc40 (body 0x16cf80), Win 0x627bf6

    // RenderControl::Initialize, then the nine part controls (new RenderControl, Initialize, Setup as
    // a shown, enabled, active child, SetImage(texture, part rect)) anchored to this control: corners
    // to theirs, edges between the corners' sizes, the centre between the edges. False as soon as one
    // Initialize fails (the parts made so far stay). Mac slot 45, Win slot 40.
    virtual bool Create(Texture* texture, const Rect& topLeft, const Rect& top,
                        const Rect& topRight, const Rect& left, const Rect& center,
                        const Rect& right, const Rect& bottomLeft, const Rect& bottom,
                        const Rect& bottomRight);                           // Mac 0x16bf70, Win 0x51983c

    RenderControl* GetPart(UInt32 part) const { return m_parts[part].Get(); }

protected:
    // Creates part `part` (null on failure of Initialize). (helper of Create; inlined)
    RenderControl* CreatePart(UInt32 part, Texture* texture, const Rect& rect);

    RefPointer<RenderControl> m_parts[kPartCount];   // Mac +0x1ec, Win +0x208
};

} // namespace LEGO
