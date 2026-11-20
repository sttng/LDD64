// LEGO Digital Designer — restored source
// LEGO::ConnectionFieldBall — <Ball> fields (ball joints and their sockets): one point,
// kinds in gender pairs.
//
// TU "ConnectionFieldBall.cpp": Mac TU 328, 0x2dd980-0x2de49f; Win 0x4dcbc0-0x4dd2e0.
// Class name from RTTI; member names inferred.
//
// Layout (Mac; Win + 4): ConnectionFieldPointHelper (+0x5c m_occupied), +0x60 UInt32 m_group
// (kind >> 1), +0x64 Real32 m_lowerLimit, +0x68 m_upperLimit, +0x6c m_twist, +0x70 m_flexX,
// +0x74 m_flexY, +0x78 bool m_hasLimits. sizeof 0x7c + Object (Mac 0x84, Win 0x88).
// Mac vtable 0x74ac1c, Win 0xa25864.
#pragma once
#include "ConnectionField.h"

namespace LEGO {

class ConnectionFieldBall : public ConnectionFieldPointHelper {
public:
    // Mac 0x2dd980 (constructor inlined), Win 0x4dce20 -> constructor 0x4dd2e0
    static ConnectionFieldBall* Create(const Matrix3D& transform, UInt32 kind);

    ConnectionFieldBall(const Matrix3D& transform, UInt32 kind);       // Win 0x4dd2e0
    ConnectionFieldBall(const ConnectionFieldBall& o);                 // Win 0x4dccb0

    // <Ball flexAttributes="..."> (the loader passes degrees converted to radians).
    // Mac 0x2de010 (caller Mac 0x83eb0; Win inlined)
    void SetLimits(Real32 lowerLimit, Real32 upperLimit, Real32 twist, Real32 flexX, Real32 flexY);

    virtual void Connect(ConnectionField& other, const ConnectionIndices& indices,
                         Attributes* attributes);                                      // Mac 0x2ddd20, Win 0x4dd070
    virtual void Disconnect(ConnectionField& other, const ConnectionIndices& indices); // Mac 0x2ddec0, Win 0x4dd1b0
    virtual void Clone(ConnectionField*& copy) const;                                  // Mac 0x2de200, Win 0x4dcbc0
    virtual bool Pick(const Matrix3D& transform, const Vector3& origin, const Vector3& direction,
                      Real32 maxDistance, Real32& distance, UInt32& index,
                      Matrix3D& offset) const;                                         // Mac 0x2ddee0, Win 0x4e9160
    using ConnectionFieldPointHelper::Pick;                                            // Mac 0x372b90
    virtual bool IsOccupied() const { return m_occupied; }                             // Mac 0x2de300, Win 0x4df3c0
    virtual void GetFrame(bool self, const ConnectionField& other, const ConnectionIndices& indices,
                          Vector3& axis, Vector3& normal, Vector3& position) const;    // Mac 0x2ddf80, Win 0x4dd200
    virtual UInt32 GetConnectionType(const ConnectionField&, const ConnectionIndices&) const
    {
        return CONNECTION_FREE;                                                        // Mac 0x2de310, Win 0x4df3d0
    }
    virtual bool IsRigidConnection(const ConnectionField&, const ConnectionIndices&) const
    {
        return false;                                                                  // Mac 0x2de320, Win 0x4a9530
    }
    virtual UInt32 GetConnectionState(bool, UInt32, const UInt32*) const { return 2; } // Mac 0x2de330, Win 0x4af0a0
    virtual UInt32 CanConnect(const Matrix3D& transform, const ConnectionField& other,
                              const Matrix3D& otherTransform, ConnectionIndices& indices) const; // Mac 0x2dda60, Win 0x4dce90
    virtual UInt32 CanConnect(const Matrix3D& transform, const ConnectionField& other,
                              const Matrix3D& otherTransform, Real32 maxDistance,
                              Real32& distance, Matrix3D& snapped) const;              // Mac 0x2ddad0, Win 0x4dcee0
    virtual UInt32 CanConnect(const Matrix3D& transform, const ConnectionField& other,
                              const Matrix3D& otherTransform, const Vector3& origin,
                              const Vector3& direction, Real32 maxDistance, Real32& distance,
                              Matrix3D& snapped) const;                                // Mac 0x2ddbe0, Win 0x4dcf80
    virtual ~ConnectionFieldBall();                     // Mac 0x2de340/0x2de370, Win 0x4dcc50/0x4dcd70

    // (accessors added for the brick database dumps; inline, names inferred)
    bool   HasLimits() const { return m_hasLimits; }
    Real32 GetLowerLimit() const { return m_lowerLimit; }
    Real32 GetUpperLimit() const { return m_upperLimit; }
    Real32 GetTwist() const { return m_twist; }
    Real32 GetFlexX() const { return m_flexX; }
    Real32 GetFlexY() const { return m_flexY; }

private:
    UInt32 m_group;         // +0x60
    Real32 m_lowerLimit;    // +0x64
    Real32 m_upperLimit;    // +0x68
    Real32 m_twist;         // +0x6c
    Real32 m_flexX;         // +0x70
    Real32 m_flexY;         // +0x74
    bool   m_hasLimits;     // +0x78
};

} // namespace LEGO
