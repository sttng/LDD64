// LEGO Digital Designer — restored source
// LEGO::VortexPhysicsFreeConnection — the joint of BrickPhysics::CreateFixedConnection in the
// Vortex physics: no engine constraint of its own (UpdateConstraint only destroys), but it keeps
// the attribute list like the other joints.
// VortexPhysicsFreeConnection.cpp (Mac TU 427, 0x3a2ea0-0x3a329f; Win: only the shared joint
// bodies 0x51d530 / 0x52cb10 / 0x52a700 / 0x51d240, 0x52cb40 UpdateConstraint, 0x52aa00
// DestroyConstraint, 0x51d680 Unknown18). Also emits the inline BaseBrickPhysicsConnectionFree::
// CheckSeparation (Mac 0x3a3070, thunk 0x3a3190: return 0). Made by VortexBrickPhysics slot 17.

// RTTI: class LEGO::VortexPhysicsFreeConnection : BaseBrickPhysicsConnectionFree. sizeof 0x80.
//   Mac vtable 0x7584b0 {base 0-7, 0x3a2ee0, 0x3a2f10, 0x3a2ea0, 0x3a3040, 0x3a3050, 0x3a3070,
//       0x3a2f40, 0x3a2f80, D1 0x3a3080, D0 0x3a30d0, 0x3a3140}; helper vtable 0x758504
//       {0x3b5a50, 0x3a2ec0, 0x3a3160, 0x3a3170, 0x3a3190, 0x3a2f60, 0x3a2fe0, D1 0x3a31a0,
//       D0 0x3a31f0}.
//   Win vtable 0xa29250 {base 0-7, 0x51d530, 0x52cb10, 0x51d680}; helper vtable 0xa29280
//       {0x51e400, 0x52a700, 0x4aede0, 0x51d240, 0x4f9670, 0x52cb40, 0x52aa00, sdd 0x51e380}.
// Every name is inferred unless noted.
#pragma once
#include "LEGO/Types.h"
#include "BaseBrickPhysicsConnectionFree.h"
#include "VortexConnectionAttributeDatas.h"

namespace Vx { class VxConstraint; }

namespace LEGO {

class VortexPhysicsFreeConnection : public BaseBrickPhysicsConnectionFree {
public:
    // Mac inlined into VortexBrickPhysics slot 17
    VortexPhysicsFreeConnection(BaseBrickPhysicsRigidElement* body, BaseBrickPhysicsRigidElement* otherBody)
        : BaseBrickPhysicsConnectionFree(body, otherBody), m_constraint(0)
    {
    }
    virtual ~VortexPhysicsFreeConnection() {}             // Mac D1 0x3a3080 / D0 0x3a30d0 (thunks 0x3a31a0 / 0x3a31f0), Win sdd 0x51e380

    virtual void* AddConstraint(const ConnectionField::Attributes& attributes);  // Mac 0x3a2ee0, Win 0x51d530
    virtual void  RemoveConstraint(void* constraint);                            // Mac 0x3a2f10, Win 0x52cb10
    virtual void* GetNativeConstraint();                  // Mac 0x3a2ea0 / 0x3a2ec0, Win 0x52a700
    // NOTE: false even with a constraint (the original's). Mac 0x3a3040 / 0x3a3160, Win 0x4aede0
    virtual bool  HasNativeConstraint();
    virtual bool  Unknown3();                             // Mac 0x3a3050 / 0x3a3170, Win 0x51d240
    // DestroyConstraint, true. Mac 0x3a2f40 / 0x3a2f60, Win 0x52cb40
    virtual bool  UpdateConstraint();
    virtual void  DestroyConstraint();                    // Mac 0x3a2f80 / 0x3a2fe0, Win 0x52aa00
    // (own slot, Mac 18, Win 10) Mac 0x3a3140, Win 0x51d680: 0
    virtual Real32 Unknown18();

    Vx::VxConstraint*              m_constraint;       // +0x6c (never set by this class)
    VortexConnectionAttributeDatas m_attributeDatas;   // +0x70
};

} // namespace LEGO
