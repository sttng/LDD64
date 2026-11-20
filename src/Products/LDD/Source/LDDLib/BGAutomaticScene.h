// LEGO Digital Designer — restored source
// LEGO::BGAutomaticScene — the BGScene of the automatic building guide: the primitives placed
//   so far (with their connected groups) on top of a model scene, whose physics answers the
//   collision queries. Created by the generators (BIGeneratorConstructive /
//   BIGeneratorDestructive) with BGApplication::GetBrickScene().
// LEGO::BICollisionCallback — the collision filter of IsMoveFree (same TU).
//
// Home TU 368 "BGAutomaticScene.cpp" (Mac 0x359740-0x35a9df; Win 0x59b420-0x59c3af).
// RTTI: class LEGO::BGAutomaticScene : BGScene, Object (Object non-virtual at +4);
//   class LEGO::BICollisionCallback : PhysicsCollisionCallback.
// Mac layout (sizeof 0x4c): BGScene vptr +0x00, Object +0x04 (vptr, refcount), m_scene +0x0c,
//   m_bricks +0x10, m_primitives +0x20, m_graphs +0x3c. Vtable 0x74fc58 {20 BGScene slots, D1
//   0x35a680, D0 0x35a690}, Object-in vtable 0x74fcb8 {thunks 0x35a6d0 / 0x35a6f0, Object
//   slots}. Win (VC9 sets are 0x24 bytes): m_scene +0x0c, m_bricks +0x10, m_primitives +0x20,
//   m_graphs +0x44; vtable 0xa2c80c, Object-in 0xa2c860 {sdd 0x59c3b0}.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "Geometry.h"
#include "BrickPhysics.h"
#include "BGScene.h"

namespace LEGO {

class Scene;
class Brick;
class BIBrick;
class BIBrickPrimitive;
class BIGraph;

class BGAutomaticScene : public BGScene, public Object {
public:
    // An empty scene over `scene` (copies the scene's brick list). Mac 0x359740 (C2) / 0x3598e0
    // (C1), Win 0x59b580
    explicit BGAutomaticScene(Scene* scene);
    virtual ~BGAutomaticScene();                                           // Mac 0x35a680 / 0x35a690 (body 0x35a800), Win sdd 0x59c3b0

    virtual bool IsEmpty() const;                                          // Mac 0x35a610, Win 0x59b420
    virtual bool Intersects(const BIBrick* brick) const;                   // Mac 0x35a620, Win 0x59b440
    virtual bool Contains(const BIBrick* brick) const;                     // Mac 0x35a640, Win 0x59b470
    virtual const BasicSet<BIBrickPrimitive*>& GetPrimitives() const;      // Mac 0x35a660, Win 0x59b4a0
    virtual void GetPrimitives(BasicSet<BIBrickPrimitive*>& primitives) const;   // Mac 0x359940, Win 0x59b690
    virtual Scene* GetModelScene();                                        // Mac 0x35a670, Win 0x5d5710
    virtual BasicArray<RefPointer<BIGraph> >& GetGraphs();                // Mac 0x35a5c0, Win 0x59c350
    virtual BIGraph* FindGraph(BIBrickPrimitive* primitive);               // Mac 0x35a5d0, Win 0x59c390
    virtual BIGraph* FindBrickGraph(BIBrick* brick);                       // Mac 0x35a5f0, Win 0x59c370
    virtual bool CanAddBricks(const BasicSet<BIBrick*>& bricks);           // Mac 0x35a580, Win 0x59c300
    virtual bool IsValid(void* argument);                                  // Mac 0x35a0d0, Win 0x5e4100
    virtual bool GetInsertDirection(const BIBrick* brick, Vector3& direction, UInt32 mode, bool flag,
                                    BasicSet<BIBrickPrimitive*>& primitives);   // Mac 0x359d00, Win 0x59b9c0
    virtual bool GetRemoveDirection(const BIBrick* brick, Vector3& direction, bool flag,
                                    BasicSet<BIBrickPrimitive*>& primitives);   // Mac 0x3599d0, Win 0x59b6b0
    virtual bool IsDirectionFree(const BIBrick* brick, Vector3 direction,
                                 const BasicSet<BIBrickPrimitive*>& primitives);   // Mac 0x35a0e0, Win 0x59bce0
    virtual bool IsMoveFree(const BIBrick* brick, Vector3 direction);      // Mac 0x35a290, Win 0x59bf50
    virtual void Clear();                                                  // Mac 0x3598f0, Win 0x59b660
    virtual bool AddBrick(BIBrick* brick, bool update);                    // Mac 0x35a3a0, Win 0x59c080
    virtual bool AddBrick(const BasicSet<BIBrick*>& bricks, bool update);  // Mac 0x35a410, Win 0x59c110
    virtual bool RemoveBrick(BIBrick* brick, bool update);                 // Mac 0x35a4b0, Win 0x59c1f0
    virtual bool RemoveBrick(const BasicSet<BIBrick*>& bricks, bool update);   // Mac 0x35a520, Win 0x59c280

    Scene*                           m_scene;        // +0x0c (name inferred)
    BasicArray<RefPointer<Brick> >   m_bricks;       // +0x10 the scene's bricks at construction (never read)
    BasicSet<BIBrickPrimitive*>      m_primitives;   // +0x20
    BasicArray<RefPointer<BIGraph> > m_graphs;       // +0x3c
};

// The callback of BGAutomaticScene::IsMoveFree: contacts with elements of the moved brick
// itself are ignored; a contact with an enabled primitive of `primitives` is a collision (and
// sets m_hit). sizeof 0x14 (both builds). Mac vtable 0x7782e8 {0x35a730}, Win 0xa2c874
// {0x59be80}; constructor inlined on Mac, Win 0x59be30. (member names inferred)
class BICollisionCallback : public PhysicsCollisionCallback {
public:
    BICollisionCallback(const BIBrick* brick, PhysicsPrimitiveElement* element,
                        const BasicSet<BIBrickPrimitive*>* primitives)                 // Win 0x59be30
        : m_hit(false), m_brick(brick), m_element(element), m_primitives(primitives) {}

    virtual UInt32 OnCollision(const PhysicsGeometryIdentifier& feature,
                               const PhysicsGeometryIdentifier& otherFeature);       // Mac 0x35a730, Win 0x59be80

    bool                               m_hit;          // +0x04
    const BIBrick*                     m_brick;        // +0x08
    PhysicsPrimitiveElement*           m_element;      // +0x0c the moved element
    const BasicSet<BIBrickPrimitive*>* m_primitives;   // +0x10 the placed primitives
};

} // namespace LEGO
