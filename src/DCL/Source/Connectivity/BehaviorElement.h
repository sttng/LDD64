// LEGO Digital Designer — restored source
// LEGO::BehaviorElement — an Element that carries Behaviors (Scene, BrickGroup, Brick, Part,
// PrimitiveElement) and reports its element type. BehaviorElement.cpp = sub-TU 025b of the
// original Scene.cpp (Mac 0x93260-0x933b0; Win 0x4713a0-0x471470); the behavior helpers are in
// Behavior.cpp (025a).
// RTTI: class LEGO::BehaviorElement : Element (non-virtual), Element : virtual Object.
//   Mac: vptr +0 (vtable 0x705f34 {pure, pure, D1 0x93260, D0 0x93330, pure}), m_behaviors +0x04,
//        Object at +0x14 when standalone (vtable 0x705f60 {thunks 0x93310 / 0x93370, …}).
//        Base-object constructor / destructor with VTT: Mac 0x93120 / 0x93170.
//   Win: vfptr +0 (0xa23b18 {pure ×3}), vbptr +4, m_behaviors +0x08, Object at +0x18
//        (vftable 0xa23b28 {sdd 0x4713a0, …}); constructor 0x4712f0, destructor 0x4713d0.
// Virtual slots: Element's (0 GetBoundingBox, 1 GetScene, 2/3 destructor), 4 GetType.
#pragma once
#include "LEGO/Types.h"
#include "Element.h"
#include "Behavior.h"
#include "BasicArray.h"
#include "StorageScene2.h"

namespace LEGO {

class BehaviorElement : public Element {
public:
    BehaviorElement() {}                                  // Mac 0x93120 (base, VTT), Win 0x4712f0
    virtual ~BehaviorElement();                           // Mac 0x93260 / 0x93330, Win 0x4713d0 / 0x4713a0

    // --- slot 4: Element::Type of the element. (name inferred)
    virtual UInt32 GetType() const = 0;

    // Appends every behavior as a StorageScene2::Behavior. Mac 0x929a0, Win 0x4710d0
    void GetBehaviors(BasicArray<StorageScene2::Behavior>& behaviors) const;
    // Creates one Behavior per StorageScene2::Behavior and appends it. Mac 0x929e0, Win 0x471110
    void AddBehaviors(const BasicArray<StorageScene2::Behavior>& behaviors);
    // AddBehaviors(GetBehaviors()) onto `target`. Mac 0x92dd0, Win 0x471270
    void CopyBehaviorsTo(BehaviorElement& target) const;

    const BasicArray<RefPointer<Behavior> >& GetBehaviorList() const { return m_behaviors; }

    BasicArray<RefPointer<Behavior> > m_behaviors;    // Mac +0x04 / Win +0x08 (name inferred)
};

} // namespace LEGO
