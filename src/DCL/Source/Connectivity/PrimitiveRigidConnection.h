// LEGO Digital Designer — restored source
// LEGO::PrimitiveRigidConnection — the PrimitiveConnections between the primitive elements of
// two PrimitiveRigidElements and their combined joint (rules: PrimitiveConnection.h).
// PrimitiveRigidConnection.cpp = sub-TU 025g of the original Scene.cpp (Mac 0x994f0-0x9b2a0,
// plus Update 0x98fa0 and RemoveConnection 0x97500 from 025f; Win 0x473c40-0x474720).
// RTTI: class LEGO::PrimitiveRigidConnection : virtual Object.
//   Mac sizeof 0x84: vptr +0 (vtable 0x70604c {D1 0x9b030, D0 0x9b120}), Object at +0x7c
//       (vtable 0x70606c {thunks 0x9b0a0 / 0x9b1a0, Object slots}).
//   Win sizeof 0x84: vbptr +0 (no own virtual function), same member offsets, Object at +0x7c
//       (vftable 0xa23c7c {sdd 0x473c40, …}); destructor 0x4744f0.
// Every member and method name is inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "Geometry.h"
#include "ConnectionField.h"

namespace LEGO {

class PrimitiveConnection;
class PrimitiveRigidElement;
class RigidConnection;

class PrimitiveRigidConnection : public virtual Object {
public:
    // A new connection of the rigid elements of `connection`'s elements, registered in both
    // rigid elements' connection maps. Mac 0x994f0, Win 0x473aa0
    explicit PrimitiveRigidConnection(PrimitiveConnection* connection);
    virtual ~PrimitiveRigidConnection();                   // Mac 0x9b030 / 0x9b120, Win 0x4744f0 / 0x473c40

    // Adds `connection`; true when the joint type changed or it has joint attributes.
    // Mac 0x99ab0 (Win inlined into 0x477de0)
    bool AddConnection(PrimitiveConnection* connection);
    // Removes `connection`; when it was the last one the connection unregisters itself
    // (true). Mac 0x97500, Win 0x473d80
    bool RemoveConnection(PrimitiveConnection* connection);
    // Takes over the primitive connections of `other` (same two rigid elements), which
    // unregisters itself. True when the joint type changed. Mac 0x9a560, Win 0x473e00
    bool Merge(PrimitiveRigidConnection* other);
    // Moves the connection to new rigid elements. Mac 0x9af30, Win 0x473fd0
    void SetRigidElements(PrimitiveRigidElement* element, PrimitiveRigidElement* otherElement);
    // Removes every primitive connection and unregisters (the inlined "last connection" path
    // of RemoveConnection). Win 0x473c70
    void Clear();
    // Joint type, frames and attributes from the primitive connections. Mac 0x98fa0, Win 0x4740b0
    void Update();
    // Joint frame of `element`'s side, rotated / transformed by its transform (i.e. in the
    // RigidElement). Mac 0x9ae70, Win 0x473ec0
    void GetFrame(const PrimitiveRigidElement* element, Vector3& axis, Vector3& normal,
                  Vector3& position) const;

    PrimitiveRigidElement* GetElement() const { return m_element; }
    PrimitiveRigidElement* GetOtherElement() const { return m_otherElement; }
    UInt32 GetType() const { return m_type; }
    const BasicArray<PrimitiveConnection*>& GetConnections() const { return m_connections; }
    const BasicArray<ConnectionField::Attributes>& GetAttributes() const { return m_attributes; }

    // ---- data (Mac and Win) -------------------------------------------------------------------
    BasicArray<PrimitiveConnection*> m_connections;    // +0x04 (not referenced)
    PrimitiveRigidElement*  m_element;                 // +0x14
    PrimitiveRigidElement*  m_otherElement;            // +0x18
    Vector3                 m_axis;                    // +0x1c frame of m_element (its space)
    Vector3                 m_normal;                  // +0x28
    Vector3                 m_position;                // +0x34
    Vector3                 m_otherAxis;               // +0x40 frame of m_otherElement
    Vector3                 m_otherNormal;             // +0x4c
    Vector3                 m_otherPosition;           // +0x58
    RigidConnection*        m_rigidConnection;         // +0x64 (not referenced)
    UInt32                  m_type;                    // +0x68 PrimitiveConnection::JointType
    BasicArray<ConnectionField::Attributes> m_attributes; // +0x6c (frames in the rigid elements)

private:
    PrimitiveRigidConnection(const PrimitiveRigidConnection&);
    PrimitiveRigidConnection& operator=(const PrimitiveRigidConnection&);
};

} // namespace LEGO
