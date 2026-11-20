// LEGO Digital Designer — restored source
// LEGO::Behavior — a named set of key/value attributes on a BehaviorElement (LXFML 5
// <Behavior BehaviorName=…><Attribute Key=… Value=…/>, StorageScene2::Behavior in the model).
// Behavior.cpp = sub-TU 025a of the original Scene.cpp (Mac 0x920c0-0x93260; Win
// 0x470d00-0x4713a0.
// RTTI: class LEGO::Behavior : virtual Object.
//   Mac sizeof 0x3c: vptr +0 (vtable 0x705eec {D1 0x92870, D0 0x92920}), Object at +0x34
//       (vtable 0x705f0c {thunks 0x92900 / 0x92960, Object slots}).
//   Win sizeof 0x38: vbptr +0 (no own virtual function: the destructor lives in the Object
//       part, vftable 0xa23d38 {sdd 0x470d00, …}; destructor body 0x471060), Object at +0x30.
// Every member name is inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicMap.h"
#include "BasicArray.h"
#include "StorageScene2.h"
#include "Geometry.h"

namespace LEGO {

// Graphics Gems "Fast Ray-Box Intersection" (A. Woo): the first point of `box`
// on the ray (`origin` when inside), double precision per slab; false when behind or missed.
// Mac 0x920c0 (in this TU's range); only caller BrickConnectSceneControl::PickElement (Mac
// 0x3bb30). Win twin not identified. (name inferred)
bool HitBoundingBox(const BoundingBox& box, const Vector3& origin, const Vector3& direction, Vector3& point);

class Behavior : public virtual Object {
public:
    typedef BasicMap<String, String> AttributeMap;   // RTTI BasicMap<String, String, Less<String>>

    // Inlined into BehaviorElement::AddBehaviors (Mac 0x929e0, Win 0x471110: Win ctor 0x470c60).
    explicit Behavior(const String& name) : m_name(name), m_reserved(0) {}
    // Mac 0x92870 / 0x92920 (thunks 0x92900 / 0x92960), Win 0x471060 / sdd 0x470d00. Implicit
    // in the original (members only).
    virtual ~Behavior() {}

    const String&       GetName() const { return m_name; }
    const AttributeMap& GetAttributes() const { return m_attributes; }
    AttributeMap&       GetAttributes() { return m_attributes; }

    // Appends this behavior to `behaviors` as a StorageScene2::Behavior (name and every
    // attribute in key order). Mac 0x92400, Win 0x470d30.
    void AppendTo(BasicArray<StorageScene2::Behavior>& behaviors) const;

    String       m_name;         // Mac +0x04 / Win +0x04 <Behavior BehaviorName=…>
    UInt32       m_reserved;     // Mac +0x14 / Win +0x08 always 0 in TU 025 (meaning unknown)
    AttributeMap m_attributes;   // Mac +0x18 / Win +0x0c <Attribute Key=… Value=…/> (std::map +0x1c / +0x10)

private:
    Behavior(const Behavior&);
    Behavior& operator=(const Behavior&);
};

} // namespace LEGO
