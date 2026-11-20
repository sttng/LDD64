// LEGO Digital Designer — restored source
// LEGO::Element — the abstract node of the connectivity scene graph (Scene.cpp, Mac TU 025 /
// Win 0x46e000-0x490e50, one object file in the original.
//
// The graph, bottom up (one class per level, each level aggregating the one below):
//   PrimitiveElement       one bone of one part's primitive, placed in its PrimitiveRigidElement
//   PrimitiveRigidElement  primitive elements that are rigidly connected (moved as one)
//   RigidElement           primitive rigid elements that form one physics body
//   HingedElement          rigid elements that are linked by (non rigid) joints
//   Scene                  the whole model
// and the user-level objects Brick / Part / BrickGroup. Element types (BehaviorElement::GetType):
// 1 Scene, 2 BrickGroup, 3 Brick, 4 Part, 5 PrimitiveElement.
//
// RTTI: class LEGO::Element : virtual Object (both builds). Element has no data member:
//   Mac: vptr (+0x00), virtual base Object follows the most derived class. The primary vtable
//        is only emitted as construction vtables (e.g. Element-in-PrimitiveRigidElement
//        0x70672c {pure, pure, D1 0xc32f0, D0 0xc3310}).
//   Win: vfptr (+0x00, 0xa23d54 {pure, pure}), vbptr (+0x04), Object (+0x08 when standalone);
//        the destructor only lives in the Object part (sdd 0x46f060).
// Virtual slots (declaration order): 0 GetBoundingBox, 1 GetScene, 2/3 destructor (Mac D1/D0).
// Every override of an Object virtual (OnFinalRelease) gets a new slot in the derived class's
// primary vtable on Mac (Object is a virtual, non-primary base); on Win it stays in the Object
// vtable and MSVC adds a vtordisp in front of the virtual base (/vd1: the class overrides a
// virtual of its virtual base and declares a constructor) — the reason for 4 extra bytes in
// the Win layouts of PrimitiveElement, PrimitiveRigidElement, RigidElement and HingedElement.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Geometry.h"

namespace LEGO {

class Scene;

class Element : public virtual Object {
public:
    // Element types returned by BehaviorElement::GetType (names inferred).
    enum Type {
        TypeScene            = 1,   // Mac 0xa5b20
        TypeBrickGroup       = 2,   // Mac 0x95c80
        TypeBrick            = 3,   // Mac 0x933b0
        TypePart             = 4,   // Mac 0x94100
        TypePrimitiveElement = 5    // Mac 0x9d7f0, Win 0x475ed0
    };

    Element() {}

    // --- slot 0: bounding box in the element's own space; `recompute` rebuilds it from the
    // elements below instead of returning the cached box (where there is one). (name inferred)
    virtual void GetBoundingBox(BoundingBox& box, bool recompute) const = 0;
    // --- slot 1: the Scene the element belongs to (NULL while it is not inserted). (name inferred)
    virtual Scene* GetScene() const = 0;
    // --- slots 2/3. Header inline: Mac 0xc32f0 / 0xc3310 (025s), Win sdd 0x46f060.
    virtual ~Element() {}

private:
    Element(const Element&);
    Element& operator=(const Element&);
};

} // namespace LEGO
