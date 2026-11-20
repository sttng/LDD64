// LEGO Digital Designer — restored source
// LEGO::BaseBrickPhysicsRigidElement — the engine-independent body: IUnknown, the cached
// transform, the user data, the mass properties, its primitive elements and joints, and the
// scene's dirty bookkeeping. VortexPhysicsRigidElement (Mac TU 434) implements slots 9-16.
// BaseBrickPhysicsRigidElement.cpp (Mac TU 210, 0x253910-0x25423f; Win 0x5c7870-0x5c79ff,
// 0x52b800, 0x52b810, 0x52b970). Ctor inline: Mac emits it in TU 434 (0x3aa050, used by the
// VortexPhysicsRigidElement ctor 0x3aa150), Win inlines it. Mac 0x253d90 is PhysicsArrayRemove.

// RTTI: class LEGO::BaseBrickPhysicsRigidElement : PhysicsRigidElement : PhysicsSceneElement :
// IUnknown. sizeof 0xcc (both builds).
//   Mac vtable 0x736878 {0x253910, 0x2539e0, 0x253a00, 0x253a40, 0x254050, 0x254060, 0x254070,
//       0x253ac0, 0x253d30, 6 pure, 0x254080, pure, D1 0x254090, D0 0x2540e0}.
//   Win vtable 0xa2a48c {0x5c7870, 0x5c78e0, 0x5c7900, 0x5c7930, 0x52b800, 0x42c9c0, 0x4f9310,
//       0x5c7950, 0x5c79c0, 6 pure, 0x52b810, pure, sdd 0x52b970}.
#pragma once
#include "LEGO/Types.h"
#include "Geometry.h"
#include "BasicArray.h"
#include "BrickPhysics.h"

namespace LEGO {

class BaseBrickPhysicsScene;
class BaseBrickPhysicsPrimitiveElement;
class BaseBrickPhysicsConnectionHelper;

class BaseBrickPhysicsRigidElement : public PhysicsRigidElement {
public:
    // Mac 0x3aa050 (inline, emitted in TU 434), Win inlined
    BaseBrickPhysicsRigidElement()
        : m_mark(0), m_refCount(1), m_scene(0), m_userData(0), m_massProperties(1.0f, 0), m_dirty(false)
    {
    }
    virtual ~BaseBrickPhysicsRigidElement() {}             // Mac D1 0x254090 / D0 0x2540e0, Win sdd 0x52b970

    // Answers PhysicsRigidElement::GetIID() and PhysicsSceneElement::GetIID(). Mac 0x253910, Win 0x5c7870
    virtual Int32  LEGO_COM_CALL QueryInterface(const InterfaceID& id, void** object);
    virtual UInt32 LEGO_COM_CALL AddRef();                  // Mac 0x2539e0, Win 0x5c78e0
    virtual UInt32 LEGO_COM_CALL Release();                 // Mac 0x253a00, Win 0x5c7900

    virtual void GetTransform(Matrix3D& transform) const;  // Mac 0x253a40, Win 0x5c7930 (m_transform)
    virtual const PhysicsMassProperties& GetMassProperties() const; // Mac 0x254050, Win 0x52b800
    virtual void* GetUserData() const;                      // Mac 0x254060, Win 0x42c9c0
    virtual void SetUserData(RigidElement* element);        // Mac 0x254070, Win 0x4f9310
    // Mac 0x253ac0, Win 0x5c7950
    virtual void AddPrimitiveElement(PhysicsPrimitiveElement* element, const Matrix3D& transform);
    // Mac 0x253d30, Win 0x5c79c0
    virtual void RemovePrimitiveElement(PhysicsPrimitiveElement* element);
    // slots 9-14 pure (engine)
    virtual void Update();                                  // slot 15: Mac 0x254080, Win 0x52b810 (m_dirty = false)
    // slot 16 pure (engine)

    // Joins `scene` at `transform` (BaseBrickPhysicsScene::AddRigidElement). Mac 0x253f90
    void AddToScene(BaseBrickPhysicsScene* scene, const Matrix3D& transform);
    // ReleaseEngineBody and no scene (BaseBrickPhysicsScene::RemoveRigidElement). Mac 0x254030
    void RemoveFromScene();
    // Queues the body for Update at the end of the scene update (once). (inline in the callers)
    void MarkDirty();
    // (added, performance) m_transform changed (PhysicsTransformReporter).
    void NoteTransformChanged() { NotePhysicsTransformChanged(m_userData); }

    // ---- data -------------------------------------------------------------------------------
    UInt32                                          m_mark;          // +0x04 visited flag of the scene's graph walks
    volatile Int32                                  m_refCount;      // +0x08 1
    BaseBrickPhysicsScene*                          m_scene;         // +0x0c
    RigidElement*                                   m_userData;      // +0x10
    PhysicsMassProperties                           m_massProperties; // +0x14 (identity, zero, mass 1, 0)
    BasicArray<BaseBrickPhysicsConnectionHelper*>   m_connections;   // +0x68 the joints of the body
    BasicArray<BaseBrickPhysicsPrimitiveElement*>   m_primitives;    // +0x78
    Matrix3D                                        m_transform;     // +0x88 cached world transform
    bool                                            m_dirty;         // +0xc8 queued in the scene's dirty list

private:
    BaseBrickPhysicsRigidElement(const BaseBrickPhysicsRigidElement&);
    BaseBrickPhysicsRigidElement& operator=(const BaseBrickPhysicsRigidElement&);
};

} // namespace LEGO
