// LEGO Digital Designer — restored source
// LEGO::VortexPhysicsBallConnection — the ball joint of the Vortex physics: a Vx::VxHinge whose
// two angular rows (equations 3 / 4) are relaxed into springs, plus a motor and a cone limit per
// field connection (AddConstraint).
// VortexPhysicsBallConnection.cpp (Mac TU 426, 0x3a1f40-0x3a2e9f; Win 0x52c0b0 Rotate, 0x52c3a0
// RotateSimulated, 0x52c660 UpdateConstraint, 0x52cac0 DestroyConstraint, and the bodies shared by
// the four joints: 0x51d530 AddConstraint, 0x52cb10 RemoveConstraint, 0x52a700 GetNativeConstraint,
// 0x51cf20 HasNativeConstraint, 0x51d240 Unknown3). Created by VortexBrickPhysics slot 22 (Mac
// 0x39f060).

// RTTI: class LEGO::VortexPhysicsBallConnection : BaseBrickPhysicsConnectionBall. sizeof 0x80.
//   Mac vtable 0x758410 {base 0-7, 0x3a2b80, 0x3a2bb0, 0x3a1f40, 0x3a1f50, base 0x252620,
//       0x2529e0, D1 0x3a2be0, D0 0x3a2c40, 0x3a2250, 0x3a2cc0, 0x3a2ce0, 0x3a2cf0, 0x3a2d10,
//       0x3a2520, 0x3a2ac0}; helper vtable 0x758474 {0x252c20, 0x3a2d30, 0x3a2d50, 0x3a2d60,
//       0x252ac0, 0x3a2aa0, 0x3a2b20, D1 0x3a2d80, D0 0x3a2de0}.
//   Win vtable 0xa291ec {base 0-7, 0x51d530, 0x52cb10, 0x42e9e0, 0x52c0b0, 0x5c3ff0, 0x52c3a0,
//       0x4aede0}; helper vtable 0xa2922c {0x51e3a0, 0x52a700, 0x51cf20, 0x51d240, 0x526060,
//       0x52c660, 0x52cac0, sdd 0x51e3c0}.
// Every name is inferred unless noted.
#pragma once
#include "LEGO/Types.h"
#include "BaseBrickPhysicsConnectionBall.h"
#include "VortexConnectionAttributeDatas.h"

namespace Vx { class VxConstraint; }

namespace LEGO {

class VortexPhysicsBallConnection : public BaseBrickPhysicsConnectionBall {
public:
    // Mac inlined into VortexBrickPhysics slot 22 (0x39f060)
    VortexPhysicsBallConnection(BaseBrickPhysicsRigidElement* body, const Vector3& axis, const Vector3& normal,
                                const Vector3& position, BaseBrickPhysicsRigidElement* otherBody,
                                const Vector3& otherAxis, const Vector3& otherNormal,
                                const Vector3& otherPosition)
        : BaseBrickPhysicsConnectionBall(body, axis, normal, position, otherBody, otherAxis, otherNormal,
                                         otherPosition),
          m_constraint(0)
    {
    }
    // Mac D1 0x3a2be0 / D0 0x3a2c40 (helper thunks 0x3a2d80 / 0x3a2de0). NOTE: the Vx
    // constraints are not destroyed here (Release detaches the joint; the scene's update
    // destroys them earlier through DestroyConstraint).
    virtual ~VortexPhysicsBallConnection() {}

    // ---- PhysicsConnection slots 8 / 9: the attribute list. Mac 0x3a2b80 / 0x3a2bb0, Win
    // 0x51d530 / 0x52cb10
    virtual void* AddConstraint(const ConnectionField::Attributes& attributes);
    virtual void  RemoveConstraint(void* constraint);
    // ---- PhysicsBallConnection
    virtual bool Unknown10();                                            // Mac 0x3a1f40, Win 0x42e9e0: false
    // Turns the joint by `delta` (|delta| <= PI/2; else false) about `axis` with the scene's
    // hinge solver; |delta| < 1e-5 does nothing (true). Mac 0x3a1f50, Win 0x52c0b0
    virtual bool Rotate(bool interactive, Real32 delta, const Vector3& axis);
    // Rotate through the simulating hinge solver, without the small-delta shortcut. (own slot,
    // Mac 16, Win 13) Mac 0x3a2250, Win 0x52c3a0
    virtual bool RotateSimulated(bool interactive, Real32 delta, const Vector3& axis);
    // ---- BaseBrickPhysicsConnectionHelper slots (Mac: also own slots 17-19, 21, 22)
    // The user data of the Vx constraint (this joint's helper), NULL without one. Mac 0x3a2cc0
    // (thunk 0x3a2d30), Win 0x52a700
    virtual void* GetNativeConstraint();
    virtual bool  HasNativeConstraint();                                 // Mac 0x3a2ce0 / 0x3a2d50, Win 0x51cf20
    // An attribute with a motor (VortexConnectionAttributeDatas::HasMotors). Mac 0x3a2cf0 /
    // 0x3a2d60, Win 0x51d240
    virtual bool  Unknown3();
    // (own slot, Mac 20, Win 14) Mac 0x3a2d10, Win 0x4aede0: false
    virtual bool  Unknown20();
    // A new Vx::VxHinge between the two bodies' VxParts with the joint frames, in the scene's
    // VxMechanism, and the attributes' motors / cone limits. Returns true. Mac 0x3a2520 (thunk
    // 0x3a2aa0), Win 0x52c660
    virtual bool  UpdateConstraint();
    // Takes the Vx constraints out of the mechanism and deletes them. Mac 0x3a2ac0 (thunk
    // 0x3a2b20), Win 0x52cac0
    virtual void  DestroyConstraint();

    Vx::VxConstraint*              m_constraint;       // +0x6c the VxHinge (user data: the helper)
    VortexConnectionAttributeDatas m_attributeDatas;   // +0x70
};

} // namespace LEGO
