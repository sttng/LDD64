// LEGO Digital Designer — restored source
// LEGO::VortexConnectionAttributeDatas — the limits / springs added to a Vortex joint
// (PhysicsConnection::AddConstraint, one ConnectionField::Attributes per field connection) and
// the Vx constraints UpdateConstraint builds from them. A member of every
// VortexPhysics*Connection (+0x70).
// The original emits the functions in VortexPhysicsScene.cpp (Mac TU 435, 0x3b2100-0x3b24ef;
// Win 0x528100-0x5282ff); restored in VortexConnectionAttributeDatas.cpp.

// RTTI: struct LEGO::VortexConnectionAttributeDatas::AttributesData (element of
//   BasicArray<AttributesData>: Mac vtable 0x778b28 {D1 0x39f1c0, D0 0x39f1f0}, Win 0xa29088
//   {sdd 0x51e340}). sizeof 0xbc (both builds; the stride of every loop).
// The class itself has no vtable: the joints' destructors reset the BasicArray vtable directly.
// Every name is inferred.
#pragma once
#include "LEGO/Types.h"
#include "BasicArray.h"
#include "ConnectionField.h"

namespace Vx {
class VxConstraint;
class VxMechanism;
}

namespace LEGO {

class BaseBrickPhysicsConnectionHelper;

// The factor the joints scale the attribute stiffnesses with (motor stiffness = linear limit *
// scale, damping = stiffness * 0.1f). An extern float on the Mac (0x778d70 = 1000.0f, read
// through the non-lazy pointer 0x7024d8; defined in VortexPhysicsScene.cpp's object), folded by
// /LTCG into the double 1000.0 (0xae0ed8) on Win. (name inferred)
extern const Real32 g_vortexConstraintStiffnessScale;

class VortexConnectionAttributeDatas {
public:
    struct AttributesData {
        // The joint limits / springs of one field connection, world frames (a copy).
        // NOTE: the original element copies 0xb0 bytes of Attributes (the four flags end at +0xaf;
        // Mac copy constructor 0xce060); ConnectionField::Attributes is declared 0xb4 bytes here
        // (m_isFlexible +0xb0), so this element is 0xc0 bytes instead of 0xbc. Nothing reads the
        // copy's m_isFlexible.
        ConnectionField::Attributes m_attributes;    // +0x00
        // The motor of the main coordinate (hasLinear / hasLimits), owned (in the scene's
        // VxMechanism while it exists).
        Vx::VxConstraint*           m_constraint;    // +0xb0
        // Ball: the cone limit (m_twist); slide hinge: the motor of the slide (hasSpring /
        // hasAngles).
        Vx::VxConstraint*           m_constraint2;   // +0xb4
        UInt32                      m_id;            // +0xb8 the AddConstraint handle
    };

    // A new entry for `attributes` (no Vx constraints yet); the joint's engine constraint is
    // destroyed and the joint queued for UpdateConstraint. Returns the handle (a running number).
    // Mac 0x3b2100 (push_back 0x3b2180), Win 0x528100 (EDI = helper)
    void* Add(BaseBrickPhysicsConnectionHelper* helper, const ConnectionField::Attributes& attributes);
    // Removes the entry `handle` and deletes its Vx constraints (the joint is rebuilt as in Add).
    // Mac 0x3b22b0, Win 0x528190
    void Remove(BaseBrickPhysicsConnectionHelper* helper, void* handle);
    // An entry with hasLinear or hasSpring. Mac 0x3b23f0, Win 0x528250
    bool HasMotors() const;
    // Mac 0x3b2430 (Win 0x4aede0): false
    bool Unknown3b2430() const { return false; }
    // Mac 0x3b2440 (Win 0x51d680): 0
    Real32 Unknown3b2440() const { return 0.0f; }
    // Takes every entry's Vx constraints out of `mechanism` and deletes them. Mac 0x3b2450,
    // Win 0x528290
    void DestroyConstraints(Vx::VxMechanism* mechanism);

    BasicArray<AttributesData> m_datas;   // +0x00

    // The last handle given out (Mac 0x779014, Win 0xb44854).
    static UInt32 s_lastId;
};

} // namespace LEGO
