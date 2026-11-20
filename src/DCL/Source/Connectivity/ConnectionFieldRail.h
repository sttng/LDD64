// LEGO Digital Designer — restored source
// LEGO::ConnectionFieldRail — <Rail> fields (train-track style rails: kind 0 the rail, kind 1
// the part riding on it). The line runs along the local -Z axis.
//
// TU "ConnectionFieldRail.cpp": Mac TU 332, 0x2e98f0-0x2eaaef; Win 0x4dbb50-0x4dc8f0.
// Class name from RTTI; member names inferred.
//
// Layout (Mac; Win + 4): ConnectionFieldLineHelper (+0x5c m_length, +0x60 / +0x61 capped,
// always false), +0x62 bool m_occupied (set on the female side). sizeof 0x64 + Object
// (Mac 0x6c, Win 0x70). Mac vtable 0x74b20c, Win 0xa257a4.
#pragma once
#include "ConnectionField.h"

namespace LEGO {

class ConnectionFieldRail : public ConnectionFieldLineHelper {
public:
    // Mac 0x2e98f0 (constructor inlined), Win 0x4dbde0 -> constructor 0x4dbc20
    static ConnectionFieldRail* Create(const Matrix3D& transform, UInt32 kind, Real32 length);

    ConnectionFieldRail(const Matrix3D& transform, UInt32 kind, Real32 length);   // Win 0x4dbc20
    ConnectionFieldRail(const ConnectionFieldRail& o);                            // Win 0x4dbd20

    // Mac 0x2ea820, Win 0x4dbb30
    virtual bool IsCompatible(const ConnectionField& other) const
    {
        return other.GetType() == m_type && (other.GetKind() ^ m_kind) == 1;
    }
    // Opposite parity of the kinds. Mac 0x2ea850, Win 0x4dbb50
    virtual bool IsCompatible(UInt32, const ConnectionField& other, UInt32) const
    {
        return other.GetType() == m_type && ((other.GetKind() ^ m_kind) & 1) != 0;
    }
    virtual void Connect(ConnectionField& other, const ConnectionIndices& indices,
                         Attributes* attributes);                                      // Mac 0x2ea510, Win 0x4dc760
    virtual void Disconnect(ConnectionField& other, const ConnectionIndices& indices); // Mac 0x2ea530, Win 0x4dc7b0
    virtual void Clone(ConnectionField*& copy) const;                                  // Mac 0x2ea870, Win 0x4dbb80
    // Rails cannot be picked (both overloads; both builds).
    virtual bool Pick(const Matrix3D&, const Vector3&, const Vector3&, Real32, Real32&, UInt32&,
                      Matrix3D&) const { return false; }                               // Mac 0x2ea550, Win 0x4decc0
    virtual void Pick(const Matrix3D&, const Vector3&, Real32, PickerAccumulator&) const {} // Mac 0x2ea560, Win 0x45a130
    virtual bool IsOccupied() const { return m_occupied; }                             // Mac 0x2ea950, Win 0x4dbc10
    virtual void GetFrame(bool self, const ConnectionField& other, const ConnectionIndices& indices,
                          Vector3& axis, Vector3& normal, Vector3& position) const;    // Mac 0x2ea570, Win 0x4dc800
    virtual UInt32 GetConnectionType(const ConnectionField&, const ConnectionIndices&) const
    {
        return CONNECTION_FREE;                                                        // Mac 0x2ea960, Win 0x4df3d0
    }
    virtual bool IsRigidConnection(const ConnectionField&, const ConnectionIndices&) const
    {
        return false;                                                                  // Mac 0x2ea970, Win 0x4a9530
    }
    virtual UInt32 GetConnectionState(bool, UInt32, const UInt32*) const { return 2; } // Mac 0x2ea980, Win 0x4af0a0
    virtual UInt32 CanConnect(const Matrix3D& transform, const ConnectionField& other,
                              const Matrix3D& otherTransform, ConnectionIndices& indices) const; // Mac 0x2e99c0, Win 0x4dbe50
    virtual UInt32 CanConnect(const Matrix3D& transform, const ConnectionField& other,
                              const Matrix3D& otherTransform, Real32 maxDistance,
                              Real32& distance, Matrix3D& snapped) const;              // Mac 0x2e9bb0, Win 0x4dbfe0
    virtual UInt32 CanConnect(const Matrix3D& transform, const ConnectionField& other,
                              const Matrix3D& otherTransform, const Vector3& origin,
                              const Vector3& direction, Real32 maxDistance, Real32& distance,
                              Matrix3D& snapped) const;                                // Mac 0x2ea0b0, Win 0x4dc270
    virtual ~ConnectionFieldRail();                     // Mac 0x2ea990/0x2ea9c0, Win 0x4dbcc0/0x4dbdb0

    void SetOccupied(bool occupied) { m_occupied = occupied; }

private:
    // `other` takes this field's orientation (turned about Y when reversed) at `offset` along
    // the rail. `dot` and `otherTransform` are not used (both builds). Mac 0x2e9eb0, Win 0x4dc8f0
    UInt32 Snap(const Vector3& axis, const Vector3& otherAxis, Real32 offset, Real32 dot,
                bool sameDirection, const Matrix3D& transform, const ConnectionFieldRail& other,
                const Matrix3D& otherTransform, Matrix3D& snapped) const;

    bool m_occupied;        // +0x62
};

} // namespace LEGO
