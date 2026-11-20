// LEGO Digital Designer — restored source
// LEGO::ConnectionFieldSlider — <Slider> fields: linear sliding connections along the local Y
// axis (Technic linear actuators, channels ...), kinds in gender pairs.
//
// TU "ConnectionFieldSlider.cpp": Mac TU 333, 0x2eaaf0-0x2ebeaf; Win 0x4de020-0x4def50.
// Class name from RTTI; member names inferred.
//
// Layout (Mac; Win + 4): ConnectionFieldLineHelper (… +0x61), +0x64 Int32 m_connectionCount.
// sizeof 0x68 + Object (Mac 0x70, Win 0x74). Mac vtable 0x74b33c, Win 0xa2592c.
#pragma once
#include "ConnectionField.h"

namespace LEGO {

class ConnectionFieldSlider : public ConnectionFieldLineHelper {
public:
    // Mac 0x2eaaf0 (constructor inlined), Win 0x4de280 -> constructor 0x4de0b0
    static ConnectionFieldSlider* Create(const Matrix3D& transform, UInt32 kind, Real32 length,
                                         bool startCapped, bool endCapped);

    ConnectionFieldSlider(const Matrix3D& transform, UInt32 kind, Real32 length,
                          bool startCapped, bool endCapped);            // Win 0x4de0b0
    ConnectionFieldSlider(const ConnectionFieldSlider& o);              // Win 0x4de1c0

    virtual void Connect(ConnectionField& other, const ConnectionIndices& indices,
                         Attributes* attributes);                                      // Mac 0x2eb9c0, Win 0x4dec20
    virtual void Disconnect(ConnectionField& other, const ConnectionIndices& indices); // Mac 0x2eb9e0, Win 0x4dec70
    virtual void Clone(ConnectionField*& copy) const;                                  // Mac 0x2ebc40, Win 0x4de020
    // Not ray-pickable (both builds).                                                Mac 0x2eba00, Win 0x4decc0
    virtual bool Pick(const Matrix3D&, const Vector3&, const Vector3&, Real32, Real32&, UInt32&,
                      Matrix3D&) const { return false; }
    using ConnectionFieldLineHelper::Pick;                                             // Mac 0x3728d0
    // Nothing (both builds).                                                          Mac 0x2eba10, Win 0x4e91f0
    virtual void GetFrame(bool, const ConnectionField&, const ConnectionIndices&, Vector3&,
                          Vector3&, Vector3&) const {}
    virtual UInt32 GetConnectionType(const ConnectionField&, const ConnectionIndices&) const
    {
        return CONNECTION_RIGID;                                                       // Mac 0x2ebd20, Win 0x4e8b30
    }
    virtual bool IsRigidConnection(const ConnectionField&, const ConnectionIndices&) const
    {
        return true;                                                                   // Mac 0x2ebd30, Win 0x52d7b0
    }
    virtual UInt32 GetConnectionState(bool, UInt32, const UInt32*) const { return 2; } // Mac 0x2ebd40, Win 0x4af0a0
    virtual UInt32 CanConnect(const Matrix3D& transform, const ConnectionField& other,
                              const Matrix3D& otherTransform, ConnectionIndices& indices) const; // Mac 0x2eabd0, Win 0x4de300
    virtual UInt32 CanConnect(const Matrix3D& transform, const ConnectionField& other,
                              const Matrix3D& otherTransform, Real32 maxDistance,
                              Real32& distance, Matrix3D& snapped) const;              // Mac 0x2eae50, Win 0x4de520
    virtual UInt32 CanConnect(const Matrix3D& transform, const ConnectionField& other,
                              const Matrix3D& otherTransform, const Vector3& origin,
                              const Vector3& direction, Real32 maxDistance, Real32& distance,
                              Matrix3D& snapped) const;                                // Mac 0x2eb570, Win 0x4de770
    virtual ~ConnectionFieldSlider();                   // Mac 0x2ebd50/0x2ebd80, Win 0x4de160/0x4de250

    Int32 GetConnectionCount() const { return m_connectionCount; }

private:
    // `other` takes this field's orientation (turned about X when reversed; its X axis within
    // 45 degrees of that of `otherTransform`) at the snapped position. `dot` is not used.
    // Mac 0x2eb140, Win 0x4def50
    UInt32 Snap(const Vector3& axis, const Vector3& otherAxis, Real32 offset, Real32 dot,
                bool sameDirection, const Matrix3D& transform, const ConnectionFieldSlider& other,
                const Matrix3D& otherTransform, Matrix3D& snapped) const;

    Int32 m_connectionCount;    // +0x64
};

} // namespace LEGO
