// LEGO Digital Designer — restored source
// LEGO::BaseBrickPhysicsConnectionBall — the engine-independent ball joint (type 5);
// VortexPhysicsBallConnection (Mac TU 426) implements the engine slots.
// BaseBrickPhysicsConnectionBall.cpp (Mac TU 205, 0x2524e0-0x252cff; Win 0x5c3f90 QI, 0x5c3ff0
// GetAngles; AddRef / Release / slots 4-7 are ICF bodies shared by the four joints: 0x5c43a0,
// 0x5c43c0, 0x5c48d0, 0x5c4400, 0x51d030, 0x51d2f0). Ctor inline (Mac: into 0x39f060, slot 22).

// RTTI: class LEGO::BaseBrickPhysicsConnectionBall : PhysicsBallConnection (+0),
// BaseBrickPhysicsConnectionHelper (+4). sizeof 0x6c.
//   Mac vtable 0x736658 {0x2524e0, 0x2525b0, 0x2525d0, 0x252ae0, 0x252af0, 0x252b10, 0x252b40,
//       0x252b60, 4 pure, 0x252620, CheckSeparation 0x2529e0, D1 0x252ba0, D0 0x252bd0};
//       helper vtable 0x7366a0 {0x252c20, 3 pure, 0x252ac0, 2 pure, 0x252c30, 0x252c60}.
//   Win vtable 0xa29190 {0x5c3f90, 0x5c43a0, 0x5c43c0, 0x475ed0, 0x5c48d0, 0x5c4400, 0x51d030,
//       0x51d2f0, 4 pure, 0x5c3ff0}; helper vtable 0xa291c8 {0x51e3a0, 3 pure, 0x526060, 2 pure,
//       sdd 0x51e390}.
#pragma once
#include "BaseBrickPhysicsConnectionHelper.h"

namespace LEGO {

class BaseBrickPhysicsConnectionBall : public PhysicsBallConnection, public BaseBrickPhysicsConnectionHelper {
public:
    // Mac inlined into 0x39f060 (refcount 1)
    BaseBrickPhysicsConnectionBall(BaseBrickPhysicsRigidElement* body, const Vector3& axis,
                                   const Vector3& normal, const Vector3& position,
                                   BaseBrickPhysicsRigidElement* otherBody, const Vector3& otherAxis,
                                   const Vector3& otherNormal, const Vector3& otherPosition)
        : BaseBrickPhysicsConnectionHelper(body, axis, normal, position, otherBody, otherAxis, otherNormal,
                                           otherPosition),
          m_refCount(1)
    {
    }

    // Answers PhysicsBallConnection::GetIID() and PhysicsConnection::GetIID(). Mac 0x2524e0, Win 0x5c3f90
    virtual Int32  LEGO_COM_CALL QueryInterface(const InterfaceID& id, void** object);
    virtual UInt32 LEGO_COM_CALL AddRef();                  // Mac 0x2525b0, Win 0x5c43a0
    // Detach and delete at 0. Mac 0x2525d0, Win 0x5c43c0
    virtual UInt32 LEGO_COM_CALL Release();
    virtual UInt32 GetType() const;                         // Mac 0x252ae0 / helper 0x252c20, Win 0x475ed0: 5
    virtual PhysicsRigidElement* GetRigidElement(bool first) const;          // Mac 0x252af0, Win 0x5c48d0
    virtual void GetLocalFrame(bool first, Vector3* axis, Vector3* normal, Vector3* position) const; // Mac 0x252b10
    virtual void GetWorldFrame(Vector3* axis, Vector3* normal, Vector3* position) const;            // Mac 0x252b40
    // Mac 0x252b60
    virtual void SetFrames(PhysicsRigidElement* body, const Vector3& axis, const Vector3& normal,
                           const Vector3& position, PhysicsRigidElement* otherBody,
                           const Vector3& otherAxis, const Vector3& otherNormal,
                           const Vector3& otherPosition);
    // Mac 0x252620, Win 0x5c3ff0
    virtual void GetAngles(Real32& pitch, Real32& yaw, Real32& roll);
    // The joint positions' squared distance (engine transforms). Mac 0x2529e0 (helper thunk
    // 0x252ac0), Win 0x526060
    virtual UInt32 CheckSeparation(const PhysicsSeparationLimits& limits);
    virtual ~BaseBrickPhysicsConnectionBall() {}           // Mac D1 0x252ba0 / D0 0x252bd0

    volatile Int32 m_refCount;                              // +0x68 1
};

} // namespace LEGO
