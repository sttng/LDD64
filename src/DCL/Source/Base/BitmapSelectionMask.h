// LEGO Digital Designer — restored source
// LEGO::SelectionMask (abstract hit-test shape) and BitmapSelectionMask (over a rectangle of a
// BitmapMask). Mac TU 275 (0x28e5f0-0x28ec1f), Win 0x46c5e0 (ctor), 0x518788-0x5188ab.
// Names inferred except the classes (RTTI).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Geometry.h"
#include "BitmapMask.h"

namespace LEGO {

// Mac vtables 0x73ed2c {pure, pure, D1 0x28e910, D0 0x28e930} / 0x73ed54; Win 0x9dbab0 / 0x9dba9c.
class SelectionMask : public virtual Object {
public:
    virtual Vector2 GetSize() const = 0;                              // slot 0
    virtual bool    Contains(const Vector2& point) const = 0;         // slot 1
    virtual ~SelectionMask() {}                                       // Mac 0x28e910 / 0x28e930
};

// Mac sizeof 0x24 {vptr, m_rect +4, m_value +0x14, m_inside +0x18, m_mask +0x1c, Object +0x20};
// vtables 0x73eccc {GetSize 0x28e5f0, Contains 0x28e620, D1 0x28e9e0, D0 0x28ea50} / 0x73ecf4.
// Win sizeof 0x2c {vfptr, vbptr, m_rect +8, m_value +0x18, m_inside +0x1c, m_mask +0x20,
// Object +0x24}; vtables 0x9dbae8 {GetSize 0x518788, Contains 0x51879f} / 0x9dbad4.
class BitmapSelectionMask : public SelectionMask {
public:
    BitmapSelectionMask();                                            // Win 0x46c5e0 (inlined on Mac)
    virtual ~BitmapSelectionMask() {}                                 // Mac 0x28e9e0 / 0x28ea50

    // The size of the mask rectangle.
    virtual Vector2 GetSize() const;                                  // Mac 0x28e5f0, Win 0x518788
    // `point` relative to the rectangle (mirrored in x for a flipped rectangle) lies in the
    // rectangle and its mask value equals / differs from the picked value as `inside` says.
    // Without a mask: !m_inside. Outside the rectangle: !m_inside.
    virtual bool Contains(const Vector2& point) const;                // Mac 0x28e620, Win 0x51879f

    // Uses `rect` of `mask` (all of it when null; a rectangle not inside the mask fails).
    bool SetMask(BitmapMask* mask, const Rect* rect);                 // Mac 0x28e800, Win 0x5188ab
    // Picks the mask value at `point` (relative as in Contains); `inside` selects whether
    // Contains matches that value or every other one. False outside the rectangle / no mask.
    bool Pick(const Vector2& point, bool inside);                     // Mac 0x28e710, Win 0x51882c

private:
    // The mask coordinates of `point`, false when outside the rectangle.
    bool ToMask(const Vector2& point, Vector2& maskPoint) const;      // inlined in Contains / Pick

    Rect                   m_rect;     // Mac +0x04, Win +0x08
    UInt32                 m_value;    // Mac +0x14, Win +0x18
    bool                   m_inside;   // Mac +0x18, Win +0x1c
    RefPointer<BitmapMask> m_mask;     // Mac +0x1c, Win +0x20
};

} // namespace LEGO
