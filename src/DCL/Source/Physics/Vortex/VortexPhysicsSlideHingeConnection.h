// LEGO Digital Designer — restored source
// LEGO::VortexPhysicsSlideHingeConnection — the slide hinge (axle in a slider) of the Vortex
// physics: a Vx::VxCylindrical between the two bodies, plus per field connection an angular motor
// (hasLinear / hasLimits) and a linear one (hasSpring / hasAngles).
// VortexPhysicsSlideHingeConnection.cpp (Mac TU 429, 0x3a4230-0x3a4adf; Win 0x52aad0
// UpdateConstraint, 0x52aa00 DestroyConstraint (shared with the hinge), the shared joint bodies of
// VortexPhysicsBallConnection.h). Created by VortexBrickPhysics slot 21 (Mac 0x39efa0, Win
// 0x51dfd0).

// RTTI: class LEGO::VortexPhysicsSlideHingeConnection : BaseBrickPhysicsConnectionSlideHinge.
//   sizeof 0x80. Mac vtable 0x7585e0 {base 0-7, 0x3a4820, 0x3a4850, base 0x2535d0, D1 0x3a4880,
//       D0 0x3a48e0, 0x3a4230, 0x3a4960, 0x3a4970, 0x3a4270, 0x3a4760}; helper vtable 0x758630
//       {0x253660, 0x3a4250, 0x3a4990, 0x3a49a0, 0x253670, 0x3a4740, 0x3a47c0, D1 0x3a49c0,
//       D0 0x3a4a20}.
//   Win vtable 0xa29140 {base 0-7, 0x51d530, 0x52cb10}; helper vtable 0xa2916c {0x51e3d0,
//       0x52a700, 0x51cf20, 0x51d240, 0x4f9670, 0x52aad0, 0x52aa00, sdd 0x51e3e0}.
// Every name is inferred unless noted.
#pragma once
#include "LEGO/Types.h"
#include "BaseBrickPhysicsConnectionSlideHinge.h"
#include "VortexConnectionAttributeDatas.h"

namespace Vx { class VxConstraint; }

namespace LEGO {

class VortexPhysicsSlideHingeConnection : public BaseBrickPhysicsConnectionSlideHinge {
public:
    // Mac inlined into VortexBrickPhysics slot 21 (0x39efa0)
    VortexPhysicsSlideHingeConnection(BaseBrickPhysicsRigidElement* body, const Vector3& axis,
                                      const Vector3& normal, const Vector3& position,
                                      BaseBrickPhysicsRigidElement* otherBody, const Vector3& otherAxis,
                                      const Vector3& otherNormal, const Vector3& otherPosition)
        : BaseBrickPhysicsConnectionSlideHinge(body, axis, normal, position, otherBody, otherAxis, otherNormal,
                                               otherPosition),
          m_constraint(0)
    {
    }
    virtual ~VortexPhysicsSlideHingeConnection() {}       // Mac D1 0x3a4880 / D0 0x3a48e0 (thunks 0x3a49c0 / 0x3a4a20), Win sdd 0x51e3e0

    virtual void* AddConstraint(const ConnectionField::Attributes& attributes);  // Mac 0x3a4820, Win 0x51d530
    virtual void  RemoveConstraint(void* constraint);                            // Mac 0x3a4850, Win 0x52cb10
    virtual void* GetNativeConstraint();                  // Mac 0x3a4230 / 0x3a4250, Win 0x52a700
    virtual bool  HasNativeConstraint();                  // Mac 0x3a4960 / 0x3a4990, Win 0x51cf20
    virtual bool  Unknown3();                             // Mac 0x3a4970 / 0x3a49a0, Win 0x51d240
    virtual bool  UpdateConstraint();                     // Mac 0x3a4270 / 0x3a4740, Win 0x52aad0
    virtual void  DestroyConstraint();                    // Mac 0x3a4760 / 0x3a47c0, Win 0x52aa00

    Vx::VxConstraint*              m_constraint;       // +0x6c the VxCylindrical (user data: the helper)
    VortexConnectionAttributeDatas m_attributeDatas;   // +0x70
};

} // namespace LEGO
