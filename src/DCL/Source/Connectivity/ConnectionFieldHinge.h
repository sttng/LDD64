// LEGO Digital Designer — restored source
// LEGO::ConnectionFieldHinge — <Hinge> fields: one-point rotating connections about the local
// Y axis, optionally with angle limits (LimMin / LimMax, FlipLimMin / FlipLimMax for the
// reversed connection). "oriented" hinges only connect with parallel (not antiparallel) axes.
//
// TU "ConnectionFieldHinge.cpp": Mac TU 330, 0x2def90-0x2e016f; Win 0x4df330-0x4dfe90.
// Class name from RTTI; member names inferred.
//
// Layout (Mac; Win + 4): ConnectionFieldPointHelper (+0x5c m_occupied), +0x60 UInt32 m_group
// (kind >> 1), +0x64 bool m_oriented, +0x65 bool m_reversed (set by Connect), +0x66 bool
// m_hasLimits, +0x68 Real32 m_minAngle, +0x6c m_maxAngle, +0x70 m_flipMinAngle,
// +0x74 m_flipMaxAngle. sizeof 0x78 + Object (Mac 0x80, Win 0x84).
// Mac vtable 0x74af9c, Win 0xa2598c.
#pragma once
#include "ConnectionField.h"

namespace LEGO {

class ConnectionFieldHinge : public ConnectionFieldPointHelper {
public:
    // Mac 0x2def90 (constructor inlined), Win 0x4df610 -> constructor 0x4df3f0
    static ConnectionFieldHinge* Create(const Matrix3D& transform, UInt32 kind, bool oriented,
                                        bool hasLimits, Real32 minAngle, Real32 maxAngle,
                                        Real32 flipMinAngle, Real32 flipMaxAngle);

    ConnectionFieldHinge(const Matrix3D& transform, UInt32 kind, bool oriented, bool hasLimits,
                         Real32 minAngle, Real32 maxAngle, Real32 flipMinAngle,
                         Real32 flipMaxAngle);                                   // Win 0x4df3f0
    ConnectionFieldHinge(const ConnectionFieldHinge& o);                         // Win 0x4df520

    virtual void Connect(ConnectionField& other, const ConnectionIndices& indices,
                         Attributes* attributes);                                      // Mac 0x2dfd10, Win 0x4dfc40
    virtual void Disconnect(ConnectionField& other, const ConnectionIndices& indices); // Mac 0x2dfdf0, Win 0x4dfd20
    virtual void Clone(ConnectionField*& copy) const;                                  // Mac 0x2dfee0, Win 0x4df330
    // Hinges cannot be ray-picked (both builds).                                     Mac 0x2dfe10, Win 0x4decc0
    virtual bool Pick(const Matrix3D&, const Vector3&, const Vector3&, Real32, Real32&, UInt32&,
                      Matrix3D&) const { return false; }
    using ConnectionFieldPointHelper::Pick;                                            // Mac 0x372b90
    virtual bool IsOccupied() const { return m_occupied; }                             // Mac 0x2dffd0, Win 0x4df3c0
    virtual void GetFrame(bool self, const ConnectionField& other, const ConnectionIndices& indices,
                          Vector3& axis, Vector3& normal, Vector3& position) const;    // Mac 0x2dfe20, Win 0x4dfd70
    virtual UInt32 GetConnectionType(const ConnectionField&, const ConnectionIndices&) const
    {
        return CONNECTION_FREE;                                                        // Mac 0x2dffe0, Win 0x4df3d0
    }
    virtual bool IsRigidConnection(const ConnectionField&, const ConnectionIndices&) const
    {
        return false;                                                                  // Mac 0x2dfff0, Win 0x4a9530
    }
    virtual UInt32 GetConnectionState(bool, UInt32, const UInt32*) const { return 0; } // Mac 0x2e0000, Win 0x4df3e0
    virtual UInt32 CanConnect(const Matrix3D& transform, const ConnectionField& other,
                              const Matrix3D& otherTransform, ConnectionIndices& indices) const; // Mac 0x2df090, Win 0x4df6b0
    virtual UInt32 CanConnect(const Matrix3D& transform, const ConnectionField& other,
                              const Matrix3D& otherTransform, Real32 maxDistance,
                              Real32& distance, Matrix3D& snapped) const;              // Mac 0x2df4a0, Win 0x4dfb30
    virtual UInt32 CanConnect(const Matrix3D& transform, const ConnectionField& other,
                              const Matrix3D& otherTransform, const Vector3& origin,
                              const Vector3& direction, Real32 maxDistance, Real32& distance,
                              Matrix3D& snapped) const;                                // Mac 0x2dfc50, Win 0x4dfba0
    virtual ~ConnectionFieldHinge();                    // Mac 0x2e0010/0x2e0040, Win 0x4df4c0/0x4df5e0

    bool IsOriented() const { return m_oriented; }
    bool IsReversed() const { return m_reversed; }   // (inline; read by the debug drawing, Mac 0xb8f80)
    bool HasLimits() const { return m_hasLimits; }
    // (accessors added for the brick database dumps; inline, names inferred)
    Real32 GetMinAngle() const { return m_minAngle; }
    Real32 GetMaxAngle() const { return m_maxAngle; }
    Real32 GetFlipMinAngle() const { return m_flipMinAngle; }
    Real32 GetFlipMaxAngle() const { return m_flipMaxAngle; }

private:
    // Aligns the axis of `other`, clamps its angle into the limits and moves it here.
    // Mac 0x2df550, Win 0x4dfe90 + inline
    UInt32 Snap(const Matrix3D& transform, const ConnectionFieldHinge& other,
                const Matrix3D& otherTransform, const Vector3& point, Real32 maxDistance,
                Real32& distance, Matrix3D& snapped) const;

    UInt32 m_group;         // +0x60
    bool   m_oriented;      // +0x64
    bool   m_reversed;      // +0x65
    bool   m_hasLimits;     // +0x66
    Real32 m_minAngle;      // +0x68 LimMin
    Real32 m_maxAngle;      // +0x6c LimMax
    Real32 m_flipMinAngle;  // +0x70 FlipLimMin
    Real32 m_flipMaxAngle;  // +0x74 FlipLimMax
};

} // namespace LEGO
