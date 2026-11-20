// LEGO Digital Designer — restored source
// LEGO::ConnectionFieldFixed — <Fixed> fields: rigid one-point connections (Technic connectors,
// Mindstorms ports ...) aligned on one (Y) or two (X and Y) axes.
//
// TU "ConnectionFieldFixed.cpp": Mac TU 329, 0x2de4a0-0x2def8f; Win 0x4e8aa0-0x4e9440.
// Class name from RTTI; member names inferred.
//
// Layout (Mac; Win + 4): ConnectionFieldPointHelper (+0x5c m_occupied), +0x60 UInt32 m_group
// (kind >> 1), +0x64 UInt32 m_axes (1 or 2), +0x68 Real32 m_snapDistance (squared),
// +0x6c Real32 m_minCosine (alignment tolerance of the axes when snapping).
// sizeof 0x70 + Object (Mac 0x78, Win 0x7c). Mac vtable 0x74addc, Win 0xa25b74.
#pragma once
#include "ConnectionField.h"

namespace LEGO {

class ConnectionFieldFixed : public ConnectionFieldPointHelper {
public:
    // Kinds 20 / 21: snap distance 16, tolerance cos(78.75 deg); others 0.4, cos(45 deg).
    // Mac 0x2de4a0 (constants folded), Win 0x4e8d30 (Cos 0x404ac0) -> constructor 0x4e8b40
    static ConnectionFieldFixed* Create(const Matrix3D& transform, UInt32 kind, UInt32 axes);

    ConnectionFieldFixed(const Matrix3D& transform, UInt32 kind, UInt32 axes,
                         Real32 snapDistance, Real32 minCosine);         // Win 0x4e8b40
    ConnectionFieldFixed(const ConnectionFieldFixed& o);                 // Win 0x4e8c50

    virtual void Connect(ConnectionField& other, const ConnectionIndices& indices,
                         Attributes* attributes);                                      // Mac 0x2dec10, Win 0x4e9110
    virtual void Disconnect(ConnectionField& other, const ConnectionIndices& indices); // Mac 0x2dec30, Win 0x4dd1b0
    virtual void Clone(ConnectionField*& copy) const;                                  // Mac 0x2ded00, Win 0x4e8aa0
    virtual bool Pick(const Matrix3D& transform, const Vector3& origin, const Vector3& direction,
                      Real32 maxDistance, Real32& distance, UInt32& index,
                      Matrix3D& offset) const;                                         // Mac 0x2dec50, Win 0x4e9160
    using ConnectionFieldPointHelper::Pick;                                            // Mac 0x372b90
    virtual bool IsOccupied() const { return m_occupied; }                             // Mac 0x2dedf0, Win 0x4df3c0
    // Nothing (both builds).                                                          Mac 0x2decf0, Win 0x4e91f0
    virtual void GetFrame(bool, const ConnectionField&, const ConnectionIndices&, Vector3&,
                          Vector3&, Vector3&) const {}
    virtual UInt32 GetConnectionType(const ConnectionField&, const ConnectionIndices&) const
    {
        return CONNECTION_RIGID;                                                       // Mac 0x2dee00, Win 0x4e8b30
    }
    virtual bool IsRigidConnection(const ConnectionField&, const ConnectionIndices&) const
    {
        return true;                                                                   // Mac 0x2dee10, Win 0x52d7b0
    }
    virtual UInt32 GetConnectionState(bool, UInt32, const UInt32*) const { return 2; } // Mac 0x2dee20, Win 0x4af0a0
    virtual UInt32 CanConnect(const Matrix3D& transform, const ConnectionField& other,
                              const Matrix3D& otherTransform, ConnectionIndices& indices) const; // Mac 0x2de620, Win 0x4e8e50
    virtual UInt32 CanConnect(const Matrix3D& transform, const ConnectionField& other,
                              const Matrix3D& otherTransform, Real32 maxDistance,
                              Real32& distance, Matrix3D& snapped) const;              // Mac 0x2de7f0, Win 0x4e9000
    virtual UInt32 CanConnect(const Matrix3D& transform, const ConnectionField& other,
                              const Matrix3D& otherTransform, const Vector3& origin,
                              const Vector3& direction, Real32 maxDistance, Real32& distance,
                              Matrix3D& snapped) const;                                // Mac 0x2deb60, Win 0x4e9070
    virtual ~ConnectionFieldFixed();                    // Mac 0x2dee30/0x2dee60, Win 0x4e8bf0/0x4e8d00

    UInt32 GetAxes() const { return m_axes; }
    Real32 GetSnapDistance() const { return m_snapDistance; }
    Real32 GetMinCosine() const { return m_minCosine; }

private:
    // Aligns `other` (only its rotation about the Y axis is kept for one axis) at this field.
    // Mac 0x2de890, Win 0x4e9200
    UInt32 Snap(const Matrix3D& transform, const ConnectionField& other,
                const Matrix3D& otherTransform, const Vector3& point, Real32 maxDistance,
                Real32& distance, Matrix3D& snapped) const;

    UInt32 m_group;         // +0x60
    UInt32 m_axes;          // +0x64
    Real32 m_snapDistance;  // +0x68
    Real32 m_minCosine;     // +0x6c
};

} // namespace LEGO
