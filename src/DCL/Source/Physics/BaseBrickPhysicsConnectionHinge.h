// LEGO Digital Designer — restored source
// LEGO::BaseBrickPhysicsConnectionHinge — the engine-independent hinge joint (type 1);
// VortexPhysicsHingeConnection (Mac TU 428) implements Rotate and the engine slots.
// BaseBrickPhysicsConnectionHinge.cpp (Mac TU 206, 0x252d00-0x2533cf; Win 0x5c4340 QI, 0x5c4430
// SetAngle, 0x5c4470 GetAngle, 0x5c4630 ctor, 0x5255e0 CheckSeparation, the shared joint
// bodies listed in BaseBrickPhysicsConnectionBall.h).

// RTTI: class LEGO::BaseBrickPhysicsConnectionHinge : PhysicsHingeConnection (+0),
// BaseBrickPhysicsConnectionHelper (+4). sizeof 0x6c.
//   Mac vtable 0x7366f8 {0x252d00, 0x252dd0, 0x252df0, 0x252e40, 0x252e60, 0x252e80, 0x252eb0,
//       0x252ed0, 2 pure, 0x252f10, 2 pure, 0x252f60, CheckSeparation 0x253070, D1 0x253280,
//       D0 0x2532e0} (D2 0x253250); helper vtable 0x736744 {0x252e50, 3 pure, 0x2531c0, 2 pure,
//       0x2532b0, 0x253330}.
//   Win vtable 0xa2d3e0 {0x5c4340, 0x5c43a0, 0x5c43c0, 0x51e780, 0x5c48d0, 0x5c4400, 0x51d030,
//       0x51d2f0, 2 pure, 0x5c4430, 2 pure, 0x5c4470}; helper vtable 0xa2d41c {0x51e3f0, 3 pure,
//       0x5255e0, 2 pure, sdd 0x5c4790}.
#pragma once
#include "BaseBrickPhysicsConnectionHelper.h"

namespace LEGO {

class BaseBrickPhysicsConnectionHinge : public PhysicsHingeConnection, public BaseBrickPhysicsConnectionHelper {
public:
    // Refcount 1. Mac 0x2531e0, Win 0x5c4630
    BaseBrickPhysicsConnectionHinge(BaseBrickPhysicsRigidElement* body, const Vector3& axis,
                                    const Vector3& normal, const Vector3& position,
                                    BaseBrickPhysicsRigidElement* otherBody, const Vector3& otherAxis,
                                    const Vector3& otherNormal, const Vector3& otherPosition);

    // Answers PhysicsHingeConnection::GetIID() and PhysicsConnection::GetIID(). Mac 0x252d00, Win 0x5c4340
    virtual Int32  LEGO_COM_CALL QueryInterface(const InterfaceID& id, void** object);
    virtual UInt32 LEGO_COM_CALL AddRef();                  // Mac 0x252dd0, Win 0x5c43a0
    virtual UInt32 LEGO_COM_CALL Release();                 // Mac 0x252df0, Win 0x5c43c0 (Detach, delete at 0)
    virtual UInt32 GetType() const;                         // Mac 0x252e40 / helper 0x252e50, Win 0x51e780: 1
    virtual PhysicsRigidElement* GetRigidElement(bool first) const;          // Mac 0x252e60
    virtual void GetLocalFrame(bool first, Vector3* axis, Vector3* normal, Vector3* position) const; // Mac 0x252e80
    virtual void GetWorldFrame(Vector3* axis, Vector3* normal, Vector3* position) const;            // Mac 0x252eb0
    // Mac 0x252ed0
    virtual void SetFrames(PhysicsRigidElement* body, const Vector3& axis, const Vector3& normal,
                           const Vector3& position, PhysicsRigidElement* otherBody,
                           const Vector3& otherAxis, const Vector3& otherNormal,
                           const Vector3& otherPosition);
    virtual bool   SetAngle(bool interactive, Real32 angle); // Mac 0x252f10, Win 0x5c4430
    virtual Real32 GetAngle();                               // Mac 0x252f60, Win 0x5c4470
    // The joint positions' squared distance and |axis . otherAxis| (engine transforms).
    // Mac 0x253070 (helper thunk 0x2531c0), Win 0x5255e0
    virtual UInt32 CheckSeparation(const PhysicsSeparationLimits& limits);
    virtual ~BaseBrickPhysicsConnectionHinge() {}          // Mac D1 0x253280 / D0 0x2532e0, Win sdd 0x5c4790

    volatile Int32 m_refCount;                              // +0x68 1
};

} // namespace LEGO
