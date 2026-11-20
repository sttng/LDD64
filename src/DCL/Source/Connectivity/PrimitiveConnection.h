// LEGO Digital Designer — restored source
// LEGO::PrimitiveConnection — all field connections between two PrimitiveElements and the
// joint they form together (rigid, hinge, slide or ball), with the joint frame of each side and
// the joint attributes (limits, springs) of the field connections.
// PrimitiveConnection.cpp = sub-TU 025f of the original Scene.cpp (Mac 0x97310-0x994f0; Win
// 0x472ef0-0x473c40).
//
// Joint types (m_type; also PrimitiveRigidConnection / RigidConnection and PhysicsConnection;
// names inferred): 0 rigid, 1 hinge (rotation about the axis), 2 slide (along the axis),
// 5 ball (about the position), 6 fixed physics joint (RigidConnection only). A field
// connection gives (Mac 0x982c0, both jump tables): planar / axle fields: rigid unless the
// field reports a non-rigid connection (then hinge); hinge field: hinge; ball: ball; rail:
// slide; fixed, slider and others: rigid. Several connections combine (same rules on all three
// levels): two hinges on one line stay a hinge, a ball on a hinge line stays the hinge, a hinge
// through a ball centre becomes that hinge, two balls 0.4 or more apart become a hinge along
// their line, two parallel slides (see NOTE in Update) stay a slide, anything else is rigid.
//
// RTTI: class LEGO::PrimitiveConnection : Object (non-virtual base!). sizeof 0x84 (both builds;
// Win: same offsets). Mac vtable 0x70602c {D1 0x98890, D0 0x98930, Object slots}; Win vftable
// 0xa23c68 {sdd 0x472ef0, …}, destructor 0x4731e0.
// Every member and method name is inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "Geometry.h"
#include "ConnectionField.h"

namespace LEGO {

class PrimitiveElement;
class PrimitiveRigidConnection;
struct PrimitiveConnectionInfo;

class PrimitiveConnection : public Object {
public:
    enum JointType {
        JointRigid = 0,
        JointHinge = 1,
        JointSlide = 2,
        JointBall  = 5,
        JointFixed = 6     // RigidConnection: physics joint of a rigid connection
    };

    // One field connection; m_field belongs to m_element of the PrimitiveConnection.
    // sizeof 0xd0 (both). Constructor Win 0x472f10, destructor Win 0x472f80; the array:
    // RTTI BasicArray<PrimitiveConnection::Connection> (Mac 0x98820 / 0xcdbe0, Win 0x49d930).
    struct Connection {
        Connection() : m_field(0), m_otherField(0) {}

        ConnectionField*                   m_field;        // +0x00
        ConnectionField*                   m_otherField;   // +0x04
        ConnectionField::ConnectionIndices m_indices;      // +0x08 m_field's index first
        ConnectionField::Attributes        m_attributes;   // +0x1c frames of m_field first
    };

    // A new connection of info.m_element and info.m_otherElement with the first field
    // connection; registers itself in both elements' connection maps. Mac 0x977c0
    // (called by PrimitiveElement::Connect, Mac 0x9ffd0).
    PrimitiveConnection(const PrimitiveConnectionInfo& info, const ConnectionField::Attributes& attributes);
    virtual ~PrimitiveConnection();                         // Mac 0x98890 / 0x98930, Win 0x4731e0 / 0x472ef0

    // One more field connection (`info` may come from either element). Returns true when the
    // joint type changed or the connection has joint attributes. Mac 0x989f0, Win 0x473280
    bool AddConnection(const PrimitiveConnectionInfo& info, const ConnectionField::Attributes& attributes);
    // Recomputes the joint type, the joint frames and the attribute list (nothing while
    // suspended). Mac 0x982c0, Win 0x473420
    void Update();
    // Joint frame of `element`'s side (one of the two elements) in its PrimitiveRigidElement
    // (rotated / transformed by the element transform). Mac 0x97700
    void GetFrame(const PrimitiveElement* element, Vector3& axis, Vector3& normal, Vector3& position) const;
    // Suspends Update (joint type rigid, no attributes) while `suspended` is non-zero.
    // Win 0x473a70 (Mac inlined into the Scene)
    void SetSuspended(UInt32 suspended);

    PrimitiveElement* GetElement() const { return m_element; }
    PrimitiveElement* GetOtherElement() const { return m_otherElement; }
    // The other element of the connection (`element` is one of the two).
    PrimitiveElement* GetOther(const PrimitiveElement* element) const
    {
        return m_element == element ? m_otherElement : m_element;
    }
    UInt32 GetType() const { return m_type; }
    const BasicArray<Connection>& GetConnections() const { return m_connections; }
    const BasicArray<ConnectionField::Attributes>& GetAttributes() const { return m_attributes; }

    // ---- data --------------------------------------------------------------------------------
    BasicArray<Connection>    m_connections;       // +0x08
    PrimitiveElement*         m_element;           // +0x18 (not referenced)
    PrimitiveElement*         m_otherElement;      // +0x1c (not referenced)
    Vector3                   m_axis;              // +0x20 joint frame of m_element (element space)
    Vector3                   m_normal;            // +0x2c
    Vector3                   m_position;          // +0x38
    Vector3                   m_otherAxis;         // +0x44 joint frame of m_otherElement
    Vector3                   m_otherNormal;       // +0x50
    Vector3                   m_otherPosition;     // +0x5c
    PrimitiveRigidConnection* m_rigidConnection;   // +0x68 (not referenced)
    UInt32                    m_type;              // +0x6c JointType (set by Update)
    BasicArray<ConnectionField::Attributes> m_attributes;  // +0x70 attributes with joint data
    UInt32                    m_suspended;         // +0x80

private:
    // Joint frame of the second side of `connection` from its field; for axle connections the
    // positions of both sides are the closest points of the two axle segments. Mac 0x98c80,
    // Win 0x4737e0
    void UpdateFrame(const Connection& connection);
    // Joint type of one field connection; JointRigid also for the unsupported fields.
    // (inlined: Mac 0x982c0 jump tables 0x987b4 / 0x987cc, Win 0x472ad0)
    static UInt32 GetJointType(const Connection& connection);

    PrimitiveConnection(const PrimitiveConnection&);
    PrimitiveConnection& operator=(const PrimitiveConnection&);
};

} // namespace LEGO
