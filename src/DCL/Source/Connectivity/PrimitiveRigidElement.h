// LEGO Digital Designer — restored source
// LEGO::PrimitiveRigidElement — PrimitiveElements that are connected rigidly (directly or
// through each other) and therefore move as one; owns their PrimitiveRigidConnections to the
// other primitive rigid elements of the scene.
// PrimitiveRigidElement.cpp = sub-TU 025k of the original Scene.cpp (Mac 0xa0210-0xa2760,
// inline virtuals 025s 0xc3be0-0xc3e30; Win 0x477d10-0x478bf0).
// RTTI: class LEGO::PrimitiveRigidElement : Element (Element : virtual Object).
//   Mac sizeof 0xa4: vptr +0 (vtable 0x7066d0 {GetBoundingBox 0xa0210, GetScene 0xc3be0,
//       D1 0xc3c10, D0 0xc3c70, OnFinalRelease 0xc3ce0}), Object at +0x9c (vtable 0x7066fc
//       {thunks 0xc3d40, 0xc3db0, 0xc3e30, AddRef, Release}). VTT 0x706710.
//   Win sizeof 0xb4: vfptr +0 (0xa23b94 {0x477d10, 0x4705e0}), vbptr +4 (vbtable 0xa28c04), members +4 (the BasicMap is 0x24 bytes on Win: m_connections +0x84),
//       vtordisp +0xa8, Object at +0xac (vftable 0xa23ba0 {sdd 0x4a1ac0, OnFinalRelease
//       0x4a1b20, …}).
// Every member and method name is inferred.
#pragma once
#include "LEGO/Types.h"
#include "Element.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "Geometry.h"

namespace LEGO {

class PrimitiveElement;
class PrimitiveRigidConnection;
class PrimitiveConnection;
class RigidElement;
class RigidConnection;

class PrimitiveRigidElement : public Element {
public:
    typedef BasicMap<PrimitiveRigidElement*, RefPointer<PrimitiveRigidConnection> > ConnectionMap;

    // `transform`: the element in its RigidElement. Mac 0xa2450, Win 0x46f9c0
    explicit PrimitiveRigidElement(const Matrix3D& transform);

    // --- slot 0: union of the primitive elements' bone boxes (element space) or the cached
    // box. Mac 0xa0210, Win 0x477d10
    virtual void GetBoundingBox(BoundingBox& box, bool recompute) const;
    // --- slot 1: Mac 0xc3be0 (025s), Win 0x4705e0
    virtual Scene* GetScene() const;
    // --- slots 2/3: Mac 0xc3c10 / 0xc3c70 (025s), Win sdd 0x4a1ac0
    virtual ~PrimitiveRigidElement();

    // Adds `element` (its m_transform is world-relative on entry and made relative to this
    // element), extends the boxes and attaches the element's geometry to the body.
    // Mac 0xa02a0 (callers: Scene, Mac 0xbe6c0)
    void AddElement(PrimitiveElement* element);
    // Merges this element and `other` (the smaller into the larger, which is returned), the
    // primitive rigid connections between them become internal, and the bodies are merged too.
    // Connections whose joint changed are added to `changed`, rigid connections to
    // `changedRigid`. Mac 0xa0530 (callers: Scene 0xbe6c0, ConnectionManipulator 0xc5aa0)
    PrimitiveRigidElement* Merge(PrimitiveRigidElement* other,
                                 BasicArray<PrimitiveRigidConnection*>& changed,
                                 BasicArray<RigidConnection*>& changedRigid);
    // After a disconnection: splits the element into its rigidly connected groups (new
    // elements in the same body) and rebuilds the primitive rigid connections. `split`
    // receives the body when it must be checked for splitting, `update` when it only changed.
    // An empty element removes itself from its body. Mac 0xa1590 (caller: Scene, Mac 0xb0080)
    void Split(RigidElement*& split, RigidElement*& update);
    // Primitive rigid connection to `other` for `connection` (new or extended); true when
    // its joint changed. Win 0x477de0 (Mac inlined)
    bool AddConnection(PrimitiveRigidElement* other, PrimitiveConnection* connection,
                       PrimitiveRigidConnection*& rigidConnection);

    const Matrix3D&        GetLocalTransform() const { return m_transform; }
    RigidElement*          GetRigidElement() const { return m_rigidElement; }
    const BasicArray<RefPointer<PrimitiveElement> >& GetElements() const { return m_elements; }
    const ConnectionMap&   GetConnections() const { return m_connections; }

    // ---- data (Mac offsets; Win +4; checked by the restoration's diff tests) ---------------------------------------------------------
    Matrix3D               m_transform;       // +0x04 in the RigidElement
    RigidElement*          m_rigidElement;    // +0x44 (not referenced)
    BoundingBox            m_bounding;        // +0x48 of the elements (element space)
    Sphere                 m_boundingSphere;  // +0x60
    BasicArray<RefPointer<PrimitiveElement> > m_elements;   // +0x70
    ConnectionMap          m_connections;     // +0x80 by the other element

protected:
    // Releases the elements. Mac 0xc3ce0 (025s, thunk 0xc3e30), Win 0x4a1b20
    virtual void OnFinalRelease();

private:
    // Moves `element` and every element rigidly connected to it from m_elements to `group`.
    // Mac 0xa12c0 (recursive)
    void CollectGroup(BasicArray<RefPointer<PrimitiveElement> >& group, PrimitiveElement* element);
    // m_bounding / m_boundingSphere from the elements. (inlined: Mac 0xa1590 ×3)
    void UpdateBounds();
};

} // namespace LEGO
