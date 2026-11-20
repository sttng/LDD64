// LEGO Digital Designer — restored source
// LEGO::VortexPhysicsHingeConnection — the hinge joint of the Vortex physics: a Vx::VxHinge
// between the two bodies plus a motor per field connection with a motor / limits (AddConstraint);
// turned through the scene's hinge solver.
// VortexPhysicsHingeConnection.cpp (Mac TU 428, 0x3a32a0-0x3a422f; Win 0x529e30 Rotate, 0x52a4b0
// RotateUnlimited, 0x52a710 UpdateConstraint, 0x52aa00 DestroyConstraint, the shared joint bodies
// of VortexPhysicsBallConnection.h). Created by VortexBrickPhysics slot 18.

// RTTI: class LEGO::VortexPhysicsHingeConnection : BaseBrickPhysicsConnectionHinge. sizeof 0x80.
//   Mac vtable 0x758540 {base 0-7, 0x3a3fb0, 0x3a3fe0, base 0x252f10, 0x3a32a0, 0x3a3840, base
//       0x252f60, 0x253070, D1 0x3a4010, D0 0x3a4060, 0x3a3b20, 0x3a40d0, 0x3a40e0, 0x3a3b60,
//       0x3a3ef0}; helper vtable 0x7585a0 {0x252e50, 0x3a3b40, 0x3a4100, 0x3a4110, 0x2531c0,
//       0x3a3ed0, 0x3a3f50, D1 0x3a4130, D0 0x3a4180}.
//   Win vtable 0xa29090 {base 0-7, 0x51d530, 0x52cb10, 0x5c4430, 0x529e30, 0x52a4b0, 0x5c4470};
//       helper vtable 0xa290cc {0x51e3f0, 0x52a700, 0x51cf20, 0x51d240, 0x5255e0, 0x52a710,
//       0x52aa00, sdd 0x51e370}.
// Every name is inferred unless noted.
#pragma once
#include "LEGO/Types.h"
#include "BaseBrickPhysicsConnectionHinge.h"
#include "VortexConnectionAttributeDatas.h"

namespace Vx { class VxConstraint; }

namespace LEGO {

class VortexPhysicsHingeConnection : public BaseBrickPhysicsConnectionHinge {
public:
    VortexPhysicsHingeConnection(BaseBrickPhysicsRigidElement* body, const Vector3& axis, const Vector3& normal,
                                 const Vector3& position, BaseBrickPhysicsRigidElement* otherBody,
                                 const Vector3& otherAxis, const Vector3& otherNormal,
                                 const Vector3& otherPosition)
        : BaseBrickPhysicsConnectionHinge(body, axis, normal, position, otherBody, otherAxis, otherNormal,
                                          otherPosition),
          m_constraint(0)
    {
    }
    virtual ~VortexPhysicsHingeConnection() {}            // Mac D1 0x3a4010 / D0 0x3a4060 (thunks 0x3a4130 / 0x3a4180), Win sdd 0x51e370

    virtual void* AddConstraint(const ConnectionField::Attributes& attributes);  // Mac 0x3a3fb0, Win 0x51d530
    virtual void  RemoveConstraint(void* constraint);                            // Mac 0x3a3fe0, Win 0x52cb10
    // Turns the hinge by `delta` (wrapped to [-PI, PI]) through the scene's hinge solver; the
    // target angle is clamped to the limits of the attributes (hasLimits). Logs
    // "VortexPhysicsHingeConnection::Change: ..." (and "Failed at iteration %d"). |step| < 1e-5:
    // true without moving. Mac 0x3a32a0, Win 0x529e30 (Win: a loop of one iteration)
    virtual bool  Rotate(bool interactive, Real32 delta);
    // The same without the limits, the log and EnableManualMotion. Mac 0x3a3840, Win 0x52a4b0
    virtual bool  RotateUnlimited(bool interactive, Real32 delta);
    virtual void* GetNativeConstraint();                  // Mac 0x3a3b20 / 0x3a3b40, Win 0x52a700
    virtual bool  HasNativeConstraint();                  // Mac 0x3a40d0 / 0x3a4100, Win 0x51cf20
    virtual bool  Unknown3();                             // Mac 0x3a40e0 / 0x3a4110, Win 0x51d240
    // A new Vx::VxHinge (+ one motor per attribute with hasLinear / hasLimits). Mac 0x3a3b60
    // (thunk 0x3a3ed0), Win 0x52a710
    virtual bool  UpdateConstraint();
    virtual void  DestroyConstraint();                    // Mac 0x3a3ef0 / 0x3a3f50, Win 0x52aa00

    Vx::VxConstraint*              m_constraint;       // +0x6c the VxHinge (user data: the helper)
    VortexConnectionAttributeDatas m_attributeDatas;   // +0x70
};

} // namespace LEGO
