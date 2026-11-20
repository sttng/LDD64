// LEGO Digital Designer — restored source
// LEGO::HingedElement — RigidElements (bodies) linked through RigidConnections (any joint); one
// per connected mechanism of the scene. HingedElement.cpp = sub-TU 025m of the original
// Scene.cpp (Mac 0xa4ce0-0xa5a70, inline virtuals 025s 0xc3e90-0xc4010, Merge 0xa3430 from 025l;
// Win 0x47a070-0x47a950).
// RTTI: class LEGO::HingedElement : Element (Element : virtual Object).
//   Mac sizeof 0x20: vptr +0 (vtable 0x706780 {GetBoundingBox 0xa4ce0, GetScene 0xc3e90,
//       D1 0xc3ea0, D0 0xc3f50, OnFinalRelease 0xc3f90}), Object at +0x18 (vtable 0x7067ac
//       {thunks 0xc3fb0, 0xc3fd0, 0xc4010, AddRef, Release}).
//   Win sizeof 0x28: vfptr +0 (0xa23bd4 {0x47a070, 0x52f4b0}), vbptr +4 (vbtable 0xa2be90), members +4 (constructor 0x46fc30),
//       vtordisp +0x1c, Object at +0x20 (vftable 0xa23be0 {sdd 0x4a1a90, 0x459a60, …}).
// Every member and method name is inferred.
#pragma once
#include "LEGO/Types.h"
#include "Element.h"
#include "BasicArray.h"
#include "Geometry.h"

namespace LEGO {

class RigidElement;

class HingedElement : public Element {
public:
    // Win 0x46fc30 (m_scene assigned by the caller); Mac inlined (0xa5190, Scene 0xacfc0).
    explicit HingedElement(Scene* scene) : m_scene(scene) {}

    // --- slot 0: world bounding box of the bodies (from their cached boxes, or rebuilt from
    // the primitive elements). Mac 0xa4ce0, Win 0x47a070
    virtual void GetBoundingBox(BoundingBox& box, bool recompute) const;
    // --- slot 1: Mac 0xc3e90 (025s), Win 0x52f4b0
    virtual Scene* GetScene() const { return m_scene; }
    // --- slots 2/3: Mac 0xc3ea0 / 0xc3f50 (025s), Win sdd 0x4a1a90
    virtual ~HingedElement() {}

    // Merges this element and `other` (the smaller into the larger, which is returned; the
    // smaller one leaves the scene). Mac 0xa3430 (025l), Win 0x47a240
    HingedElement* Merge(HingedElement* other);
    // Splits the element into its connected groups of bodies (new hinged elements are added
    // to the scene). Mac 0xa5190, Win 0x47a530
    void Split();

    const BasicArray<RefPointer<RigidElement> >& GetElements() const { return m_elements; }

    // ---- data (Mac offsets; Win +4) ---------------------------------------------------------
    Scene*                                m_scene;      // +0x04 (not referenced)
    BasicArray<RefPointer<RigidElement> > m_elements;   // +0x08

protected:
    // Mac 0xc3f90 (025s, thunk 0xc4010), Win 0x459a60 (shared body)
    virtual void OnFinalRelease() { Object::OnFinalRelease(); }

private:
    // Moves `element` and every body connected to it from m_elements to `group`.
    // Mac 0xa4ec0, Win 0x47a340 (recursive)
    void CollectGroup(BasicArray<RefPointer<RigidElement> >& group, RigidElement* element);
};

} // namespace LEGO
