// LEGO Digital Designer — restored source
// LEGO::BaseBrickPhysicsConnectionFree — the joint of BrickPhysics::CreateFixedConnection (type
// 6): no engine constraint, all frames zero. VortexPhysicsFreeConnection (Mac TU 427) derives
// from it.
// BaseBrickPhysicsConnectionFree.cpp (Mac TU 438, 0x3b5900-0x3b5e0f; Win 0x5c4870 QI, the shared
// joint bodies of BaseBrickPhysicsConnectionBall.h, ICF return bodies). CheckSeparation is
// inline: Mac emits it in TU 427 (0x3a3070, thunk 0x3a3190).

// RTTI: class LEGO::BaseBrickPhysicsConnectionFree : PhysicsFreeConnection (+0),
// BaseBrickPhysicsConnectionHelper (+4). sizeof 0x6c.
//   Mac vtable 0x758e38 {0x3b5900, 0x3b59d0, 0x3b59f0, 0x3b5a40, 0x3b5a60, 0x3b5a80, 0x3b5ab0,
//       0x3b5ad0, 0x3b5b10, 0x3b5b20, 0x3b5d30, 0x3b5d40, 0x3b5d50, 0x3a3070, 0x3b5d60, 0x3b5d70,
//       D1 0x3b5c20, D0 0x3b5c80} (D2 0x3b5bf0); helper vtable 0x758e88 {0x3b5a50, 0x3b5d80,
//       0x3b5d90, 0x3b5da0, 0x3a3190, 0x3b5db0, 0x3b5dc0, 0x3b5c50, 0x3b5cd0}.
//   Win vtable 0xa2d46c {0x5c4870, 0x5c43a0, 0x5c43c0, 0x51e880, 0x5c48d0, 0x5c4400, 0x51d030,
//       0x51d2f0, 0x4f9670, 0x40be80}; helper vtable 0xa2d498 {0x51e400, 0x51e7a0, 0x4aede0,
//       0x4aede0, 0x4f9670, 0x4f5b70, 0x459790, sdd 0x5c4aa0}.
#pragma once
#include "BaseBrickPhysicsConnectionHelper.h"

namespace LEGO {

class BaseBrickPhysicsConnectionFree : public PhysicsFreeConnection, public BaseBrickPhysicsConnectionHelper {
public:
    // Zero frames, refcount 1. Mac 0x3b5b30
    BaseBrickPhysicsConnectionFree(BaseBrickPhysicsRigidElement* body, BaseBrickPhysicsRigidElement* otherBody);

    // Answers PhysicsFreeConnection::GetIID() and PhysicsConnection::GetIID(). Mac 0x3b5900, Win 0x5c4870
    virtual Int32  LEGO_COM_CALL QueryInterface(const InterfaceID& id, void** object);
    virtual UInt32 LEGO_COM_CALL AddRef();                  // Mac 0x3b59d0, Win 0x5c43a0
    virtual UInt32 LEGO_COM_CALL Release();                 // Mac 0x3b59f0, Win 0x5c43c0 (Detach, delete at 0)
    virtual UInt32 GetType() const;                         // Mac 0x3b5a40 / helper 0x3b5a50, Win 0x51e880: 6
    virtual PhysicsRigidElement* GetRigidElement(bool first) const;          // Mac 0x3b5a60
    virtual void GetLocalFrame(bool first, Vector3* axis, Vector3* normal, Vector3* position) const; // Mac 0x3b5a80
    virtual void GetWorldFrame(Vector3* axis, Vector3* normal, Vector3* position) const;            // Mac 0x3b5ab0
    // Mac 0x3b5ad0
    virtual void SetFrames(PhysicsRigidElement* body, const Vector3& axis, const Vector3& normal,
                           const Vector3& position, PhysicsRigidElement* otherBody,
                           const Vector3& otherAxis, const Vector3& otherNormal,
                           const Vector3& otherPosition);
    virtual void* AddConstraint(const ConnectionField::Attributes& attributes);  // Mac 0x3b5b10, Win 0x4f9670: NULL
    virtual void  RemoveConstraint(void* constraint);                            // Mac 0x3b5b20, Win 0x40be80
    virtual void* GetNativeConstraint();                    // Mac 0x3b5d30 / 0x3b5d80, Win 0x51e7a0: NULL
    virtual bool  HasNativeConstraint();                    // Mac 0x3b5d40 / 0x3b5d90, Win 0x4aede0: false
    virtual bool  Unknown3();                               // Mac 0x3b5d50 / 0x3b5da0, Win 0x4aede0: false
    // Mac 0x3a3070 / 0x3a3190 (emitted in TU 427), Win 0x4f9670
    virtual UInt32 CheckSeparation(const PhysicsSeparationLimits& /*limits*/) { return 0; }
    virtual bool  UpdateConstraint();                       // Mac 0x3b5d60 / 0x3b5db0, Win 0x4f5b70: true
    virtual void  DestroyConstraint();                      // Mac 0x3b5d70 / 0x3b5dc0, Win 0x459790
    virtual ~BaseBrickPhysicsConnectionFree() {}           // Mac D1 0x3b5c20 / D0 0x3b5c80, Win sdd 0x5c4aa0

    volatile Int32 m_refCount;                              // +0x68 1
};

} // namespace LEGO
