// LEGO Digital Designer — restored source
// LEGO::RigidConnection — the PrimitiveRigidConnections between two RigidElements (bodies), the
// combined joint (rules: PrimitiveConnection.h) and the physics joint built for it.
// RigidConnection.cpp = sub-TU 025h of the original Scene.cpp (Mac 0x9b2a0-0x9cc30, plus Update
// 0x99f00 and RemoveConnection 0x998d0 from 025g; Win 0x474720-0x4753c0).
// RTTI: class LEGO::RigidConnection : virtual Object.
//   Mac sizeof 0x84: vptr +0 (vtable 0x706094 {D1 0x9ca20, D0 0x9cb10}), Object at +0x7c
//       (vtable 0x7060b4 {thunks 0x9ca90 / 0x9cb90, Object slots}).
//   Win sizeof 0x84: vbptr +0, same member offsets, Object at +0x7c (vftable 0xa23c98 {sdd
//       0x474720, …}); destructor 0x474ba0.
// Every member and method name is inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "Geometry.h"
#include "ConnectionField.h"
#include "BrickPhysics.h"

namespace LEGO {

class PrimitiveRigidConnection;
class RigidElement;

class RigidConnection : public virtual Object {
public:
    // A new connection of the bodies of `connection`'s rigid elements (registered in both
    // bodies' connection maps), with its physics joint. Mac 0x9b2a0, Win 0x474580
    explicit RigidConnection(PrimitiveRigidConnection* connection);
    // NOTE: does not release m_joint (the owners clear it first). Mac 0x9ca20 / 0x9cb10,
    // Win 0x474ba0 / 0x474720
    virtual ~RigidConnection();

    // Adds `connection` (if it has no rigid connection yet) and updates; true when the
    // physics joint type changed or `connection` has joint attributes. Mac 0x9b7b0, Win 0x474860
    bool AddConnection(PrimitiveRigidConnection* connection);
    // Removes `connection`; when it was the last one the joint is released and the connection
    // unregisters itself (true). Mac 0x998d0, Win 0x4748d0
    bool RemoveConnection(PrimitiveRigidConnection* connection);
    // Takes over the primitive rigid connections of `other`, which releases its joint and
    // unregisters itself. True when the joint type changed. Mac 0x9bc30, Win 0x474920
    bool Merge(RigidConnection* other);
    // Moves the connection to new bodies; an existing joint gets new frames. Mac 0x9c550,
    // Win 0x474a70
    void SetRigidElements(RigidElement* element, RigidElement* otherElement);
    // Removes every primitive rigid connection, releases the joint and unregisters. Win 0x474750
    void Clear();
    // Joint type and frames from the primitive rigid connections; rebuilds the physics joint
    // when its type changed. Mac 0x99f00, Win 0x474c30
    void Update();
    // Re-adds the joint attributes of every primitive connection to the physics joint.
    // Mac 0x9c6e0, Win 0x475140
    void UpdateConstraints();
    // Stored joint frame of `element`'s side (body space). Win 0x4749a0 (Mac inlined)
    void GetFrame(const RigidElement* element, Vector3& axis, Vector3& normal, Vector3& position) const;

    RigidElement* GetElement() const { return m_element; }
    RigidElement* GetOtherElement() const { return m_otherElement; }
    PhysicsConnection* GetJoint() const { return m_joint; }
    // Type of the physics joint (PrimitiveConnection::JointType; 0 without joint).
    UInt32 GetJointType() const { return m_joint ? m_joint->GetType() : 0; }
    const BasicArray<PrimitiveRigidConnection*>& GetConnections() const { return m_connections; }

    // ---- data (Mac and Win) -------------------------------------------------------------------
    BasicArray<PrimitiveRigidConnection*> m_connections;  // +0x04 (not referenced)
    RigidElement*      m_element;                         // +0x14
    RigidElement*      m_otherElement;                    // +0x18
    Vector3            m_axis;                            // +0x1c frame of m_element (body space)
    Vector3            m_normal;                          // +0x28
    Vector3            m_position;                        // +0x34
    Vector3            m_otherAxis;                       // +0x40 frame of m_otherElement
    Vector3            m_otherNormal;                     // +0x4c
    Vector3            m_otherPosition;                   // +0x58
    PhysicsConnection* m_joint;                           // +0x64 one reference
    bool               m_flag68;                          // +0x68 false (not used in 025h)
    BasicArray<void*>  m_constraints;                     // +0x6c PhysicsConnection::AddConstraint handles

private:
    RigidConnection(const RigidConnection&);
    RigidConnection& operator=(const RigidConnection&);
};

} // namespace LEGO
