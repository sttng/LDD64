// LEGO Digital Designer — restored source
// LEGO::RigidElement — one physics body: the PrimitiveRigidElements that are connected through
// rigid joints, and the RigidConnections (non-rigid joints) to the other bodies.
// RigidElement.cpp = sub-TU 025l of the original Scene.cpp (Mac 0xa2760-0xa4ce0, GetScene
// 0xc4460 in 025s; Win 0x478bf0-0x47a070).
// RTTI: class LEGO::RigidElement : Element (Element : virtual Object).
//   Mac sizeof 0x6c: vptr +0 (vtable 0x706164 {GetBoundingBox 0xa2760, GetScene 0xc4460,
//       D1 0xa2d10, D0 0xa2dd0, OnFinalRelease 0xa2ec0}), Object at +0x64 (vtable 0x706190
//       {thunks 0xa2d70, 0xa2e40, 0xa2f20, AddRef, Release}). VTT 0x7061a4; base destructor
//       0xa2ba0, constructor 0xa2aa0.
//   Win sizeof 0x7c: vfptr +0 (0xa23bb4 {0x478bf0, 0x470600}), vbptr +4 (vbtable 0xa21850), members +4 up to m_connections (+0x44, a 0x24-byte
//       BasicMap on Win), then +0xc: m_body +0x68, m_flag60 +0x6c,
//       vtordisp +0x70, Object at +0x74 (vftable 0xa23bc0 {sdd 0x4a1ad0, OnFinalRelease
//       0x4a1aa0, …}); constructor 0x478e10, destructor 0x478f40.
// Every member and method name is inferred.
#pragma once
#include "LEGO/Types.h"
#include "Element.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "Geometry.h"
#include "BrickPhysics.h"

namespace LEGO {

class HingedElement;
class PrimitiveRigidElement;
class RigidConnection;
class PrimitiveRigidConnection;

class RigidElement : public Element {
public:
    typedef BasicMap<RigidElement*, RefPointer<RigidConnection> > ConnectionMap;

    // Empty element without body (Scene::CreateRigidElement creates the body). Mac 0xa2aa0,
    // Win 0x478e10
    RigidElement();

    // --- slot 0: bounding box in body space (cached or rebuilt). Mac 0xa2760, Win 0x478bf0
    virtual void GetBoundingBox(BoundingBox& box, bool recompute) const;
    // --- slot 1: Mac 0xc4460 (025s), Win 0x470600
    virtual Scene* GetScene() const;
    // --- slots 2/3: releases the body. Mac 0xa2d10 / 0xa2dd0 (base 0xa2ba0), Win 0x478f40 / 0x4a1ad0
    virtual ~RigidElement();

    // World transform of the body. Mac 0xa2740
    void GetTransform(Matrix3D& transform) const;
    // Adds `element` (its m_transform is world-relative on entry and made relative to the
    // body) and attaches its primitive elements to the body. Mac 0xa2f80, Win 0x479190
    void AddElement(PrimitiveRigidElement* element);
    // Merges this body and `other` (the smaller into the larger, which is returned); rigid
    // connections between them disappear, the others are moved and collected in `changed`;
    // the hinged elements are merged too. Mac 0xa0cb0, Win 0x479310
    RigidElement* Merge(RigidElement* other, BasicArray<RigidConnection*>& changed);
    // Splits the body into its rigidly connected groups (new bodies from
    // Scene::CreateRigidElement) and rebuilds the rigid connections (collected in `changed`).
    // `hinged` receives the hinged element when it must be checked for splitting. An empty
    // body removes itself. Mac 0xa38a0, Win 0x4799a0
    void Split(HingedElement*& hinged, BasicArray<RigidConnection*>& changed);
    // Rigid connection to `other` for `connection` (new or extended); true when its joint
    // changed. Win 0x479020 (Mac inlined)
    bool AddConnection(RigidElement* other, PrimitiveRigidConnection* connection,
                       RigidConnection*& rigidConnection);

    // Manipulator for the joints of this body's rigid connections (the Scene's active
    // manipulator; the previous one is detached); NULL when the body has no connection or
    // none of them can move. Mac 0xa2840 (025l position, restored in ConnectionManipulator.cpp),
    // Win 0x475be0 (+ 0x475c70). (name inferred)
    class ConnectionManipulator* CreateManipulator();

    HingedElement*       GetHingedElement() const { return m_hinged; }
    PhysicsRigidElement* GetBody() const { return m_body; }
    const BasicArray<RefPointer<PrimitiveRigidElement> >& GetElements() const { return m_elements; }
    const ConnectionMap& GetConnections() const { return m_connections; }

    // ---- data (Mac offsets; Win +4, after m_connections +0xc; diff_scenegraph.cpp) ---------------------------------------------------------
    HingedElement*        m_hinged;          // +0x04 (not referenced)
    BoundingBox           m_bounding;        // +0x08 body space
    Sphere                m_boundingSphere;  // +0x20
    BasicArray<RefPointer<PrimitiveRigidElement> > m_elements;  // +0x30
    ConnectionMap         m_connections;     // +0x40 by the other body
    PhysicsRigidElement*  m_body;            // +0x5c one reference
    bool                  m_flag60;          // +0x60 false (not used in 025l)
    // (added, performance) Scene::Pick's spatial index: the version changes with the body's
    // elements, transforms or bounds (SceneIndexTouch); the others find the body's cache
    // (Scene.cpp).
    UInt32                m_indexVersion;
    class SceneIndex*     m_indexOwner;
    UInt32                m_indexSlot;
    UInt32                m_indexGeneration;

protected:
    // Releases the elements. Mac 0xa2ec0 (thunk 0xa2f20), Win 0x4a1aa0
    virtual void OnFinalRelease();

private:
    // Moves `element` and every element rigidly connected to it from m_elements to `group`.
    // Mac 0xa35d0, Win 0x479790 (recursive)
    void CollectGroup(BasicArray<RefPointer<PrimitiveRigidElement> >& group, PrimitiveRigidElement* element);
    // m_bounding / m_boundingSphere from the elements. Win 0x479f60 (Mac inlined)
    void UpdateBounds();
};

// (added, performance) Marks the body's cached pick data out of date wherever its elements, their
// transforms or bounds change. The index (Scene.cpp) installs the two functions; only bodies with
// an index owner call them.
struct SceneIndexHooks {
    void (*changed)(RigidElement* body);
    void (*forget)(RigidElement* body);
};
SceneIndexHooks& GetSceneIndexHooks();        // RigidElement.cpp
inline void SceneIndexTouch(RigidElement* body)
{
    if (body != 0) {
        ++body->m_indexVersion;
        if (body->m_indexOwner != 0)
            GetSceneIndexHooks().changed(body);
    }
}

} // namespace LEGO
