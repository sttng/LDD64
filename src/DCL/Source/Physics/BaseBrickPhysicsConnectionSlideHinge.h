// LEGO Digital Designer — restored source
// LEGO::BaseBrickPhysicsConnectionSlideHinge — the engine-independent slide hinge joint (type
// 4); VortexPhysicsSlideHingeConnection (Mac TU 429) implements the engine slots.
// BaseBrickPhysicsConnectionSlideHinge.cpp (Mac TU 207, 0x2533d0-0x25374f; Win 0x5c4b80 QI,
// 0x51d050 ctor (Win only; Mac inlines it into VortexBrickPhysics slot 21 0x39efa0), the shared
// joint bodies of BaseBrickPhysicsConnectionBall.h).

// RTTI: class LEGO::BaseBrickPhysicsConnectionSlideHinge : PhysicsSlideHingeConnection (+0),
// BaseBrickPhysicsConnectionHelper (+4). sizeof 0x6c.
//   Mac vtable 0x736798 {0x2533d0, 0x2534a0, 0x2534c0, 0x253510, 0x253520, 0x253540, 0x253570,
//       0x253590, 2 pure, CheckSeparation 0x2535d0, D1 0x2535e0, D0 0x253610}; helper vtable
//       0x7367d4 {0x253660, 3 pure, 0x253670, 2 pure, 0x253680, 0x2536b0}.
//   Win vtable 0xa290f0 {0x5c4b80, 0x5c43a0, 0x5c43c0, 0x51e840, 0x5c48d0, 0x5c4400, 0x51d030,
//       0x51d2f0, 2 pure}; helper vtable 0xa2911c {0x51e3d0, 3 pure, 0x4f9670, 2 pure, sdd 0x51e3b0}.
#pragma once
#include "BaseBrickPhysicsConnectionHelper.h"

namespace LEGO {

class BaseBrickPhysicsConnectionSlideHinge : public PhysicsSlideHingeConnection,
                                             public BaseBrickPhysicsConnectionHelper {
public:
    // Refcount 1. Win 0x51d050 (Mac inlined)
    BaseBrickPhysicsConnectionSlideHinge(BaseBrickPhysicsRigidElement* body, const Vector3& axis,
                                         const Vector3& normal, const Vector3& position,
                                         BaseBrickPhysicsRigidElement* otherBody, const Vector3& otherAxis,
                                         const Vector3& otherNormal, const Vector3& otherPosition)
        : BaseBrickPhysicsConnectionHelper(body, axis, normal, position, otherBody, otherAxis, otherNormal,
                                           otherPosition),
          m_refCount(1)
    {
    }

    // Answers PhysicsSlideHingeConnection::GetIID() and PhysicsConnection::GetIID(). Mac 0x2533d0, Win 0x5c4b80
    virtual Int32  LEGO_COM_CALL QueryInterface(const InterfaceID& id, void** object);
    virtual UInt32 LEGO_COM_CALL AddRef();                  // Mac 0x2534a0, Win 0x5c43a0
    virtual UInt32 LEGO_COM_CALL Release();                 // Mac 0x2534c0, Win 0x5c43c0 (Detach, delete at 0)
    virtual UInt32 GetType() const;                         // Mac 0x253510 / helper 0x253660, Win 0x51e840: 4
    virtual PhysicsRigidElement* GetRigidElement(bool first) const;          // Mac 0x253520
    virtual void GetLocalFrame(bool first, Vector3* axis, Vector3* normal, Vector3* position) const; // Mac 0x253540
    virtual void GetWorldFrame(Vector3* axis, Vector3* normal, Vector3* position) const;            // Mac 0x253570
    // Mac 0x253590
    virtual void SetFrames(PhysicsRigidElement* body, const Vector3& axis, const Vector3& normal,
                           const Vector3& position, PhysicsRigidElement* otherBody,
                           const Vector3& otherAxis, const Vector3& otherNormal,
                           const Vector3& otherPosition);
    // Always holds: 0. Mac 0x2535d0 (helper 0x253670), Win 0x4f9670
    virtual UInt32 CheckSeparation(const PhysicsSeparationLimits& limits);
    virtual ~BaseBrickPhysicsConnectionSlideHinge() {}     // Mac D1 0x2535e0 / D0 0x253610, Win sdd 0x51e3b0

    volatile Int32 m_refCount;                              // +0x68 1
};

} // namespace LEGO
