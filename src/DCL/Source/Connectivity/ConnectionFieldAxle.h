// LEGO Digital Designer — restored source
// LEGO::ConnectionFieldAxle — <Axel> fields: axles, pins, holes and their end stops, along the
// local Y axis from the origin to (0, length, 0).
//
// TU "ConnectionFieldAxle.cpp": Mac TU 327, 0x2db0a0-0x2dd97f; Win 0x4aef60-0x4b0630 (plus the
// connection table init 0x4b0630 and the test helper 0x4b08a0).
// Class name from RTTI; member names inferred.
//
// Kinds (XML "type", < 22): even kinds are the "male" side (axle 2, ...), odd kinds the
// "female" side (holes 3, 19, ...). Compatibility and the kind of connection come from the
// 22 x 22 table s_connectionTable (InitializeTables, called by BrickDatabase and BrickConnect).
// Special kinds in the snapping / overlap code: 2 axle, 3 and 19 axle holes (fixed steps of
// m_step along an axle), 4 / 5 a pair whose Z axes must be parallel or perpendicular,
// 0x11 (17) an end stop that only fits at either end of an axle.
//
// Layout (Mac; Win + 4): ConnectionFieldLineHelper (… +0x61), +0x64 Real32 m_step,
// +0x68 bool m_grabbing, +0x69 bool m_requireGrabbing, +0x6c Int32 m_connectionCount.
// sizeof 0x70 + Object (Mac 0x78, Win 0x7c). Mac vtable 0x74aaec, Win 0xa2472c.
#pragma once
#include "ConnectionField.h"

namespace LEGO {

class ConnectionFieldAxle : public ConnectionFieldLineHelper {
public:
    enum { KIND_COUNT = 22 };

    // NULL for kind >= 22. step = length for kind 2 (axles), 0.08 otherwise.
    // Mac 0x2db0a0 (constructor inlined), Win 0x4af370 caller side / constructor 0x4af0c0
    static ConnectionFieldAxle* Create(const Matrix3D& transform, UInt32 kind, Real32 length,
                                       bool startCapped, bool endCapped, bool grabbing,
                                       bool requireGrabbing);

    ConnectionFieldAxle(const Matrix3D& transform, UInt32 kind, Real32 length, bool startCapped,
                        bool endCapped, bool grabbing, bool requireGrabbing, Real32 step);
    ConnectionFieldAxle(const ConnectionFieldAxle& o);                // Win 0x4af210

    // Fills s_connectionTable. Mac 0x2dcf90, Win 0x4b0630 (callers: BrickDatabase, BrickConnect).
    static void InitializeTables();
    static UInt32 s_connectionTable[KIND_COUNT][KIND_COUNT];          // Mac 0x77af50, Win 0xb40010

    // Snaps `*offset` to 0 (anchored) or `length` (!anchored) when it lies within 0.4 of it.
    // Mac 0x2dcf20 (caller AxleAligner Mac 0x32c340; Win inlined).
    static bool SnapToEnd(Real32* offset, Real32 length, bool anchored);

    virtual bool IsCompatible(const ConnectionField& other) const;                     // Mac 0x2dd640, Win 0x4aef60
    virtual bool IsCompatible(UInt32 index, const ConnectionField& other, UInt32 otherIndex) const; // Mac 0x2dd680, Win 0x4aef90
    virtual UInt32 GetConnectionAngle(const Matrix3D& transform, const ConnectionField& other,
                                      const Matrix3D& otherTransform, const Vector3& axis,
                                      const Vector3& reference, const Vector3& center,
                                      Real32& angle) const;                            // Mac 0x2dc7e0, Win 0x4afeb0
    virtual void Connect(ConnectionField& other, const ConnectionIndices& indices,
                         Attributes* attributes);                                      // Mac 0x2dcab0, Win 0x4b0100
    virtual void Disconnect(ConnectionField& other, const ConnectionIndices& indices); // Mac 0x2dcad0, Win 0x4b0150
    virtual void Clone(ConnectionField*& copy) const;                                  // Mac 0x2dd6c0, Win 0x4aefd0
    virtual bool Pick(const Matrix3D& transform, const Vector3& origin, const Vector3& direction,
                      Real32 maxDistance, Real32& distance, UInt32& index,
                      Matrix3D& offset) const;                                         // Mac 0x2dcaf0, Win 0x4b01a0
    using ConnectionFieldLineHelper::Pick;                                             // Mac 0x3728d0
    virtual void GetFrame(bool self, const ConnectionField& other, const ConnectionIndices& indices,
                          Vector3& axis, Vector3& normal, Vector3& position) const;    // Mac 0x2dcdf0, Win 0x4b0430
    virtual UInt32 GetConnectionType(const ConnectionField& other,
                                     const ConnectionIndices& indices) const;          // Mac 0x2dd7b0, Win 0x4af060
    virtual bool IsRigidConnection(const ConnectionField& other,
                                   const ConnectionIndices& indices) const;            // Mac 0x2dd7e0, Win 0x4af080
    virtual UInt32 GetConnectionState(bool any, UInt32 count, const UInt32* counts) const; // Mac 0x2dd810, Win 0x4af0a0
    virtual UInt32 CanConnect(const Matrix3D& transform, const ConnectionField& other,
                              const Matrix3D& otherTransform, ConnectionIndices& indices) const; // Mac 0x2db1c0, Win 0x4af490
    virtual UInt32 CanConnect(const Matrix3D& transform, const ConnectionField& other,
                              const Matrix3D& otherTransform, Real32 maxDistance,
                              Real32& distance, Matrix3D& snapped) const;              // Mac 0x2db930, Win 0x4af780
    virtual UInt32 CanConnect(const Matrix3D& transform, const ConnectionField& other,
                              const Matrix3D& otherTransform, const Vector3& origin,
                              const Vector3& direction, Real32 maxDistance, Real32& distance,
                              Matrix3D& snapped) const;                                // Mac 0x2dc390, Win 0x4af9e0
    virtual ~ConnectionFieldAxle();                     // Mac 0x2dd820/0x2dd850, Win 0x4af180/0x4af2c0

    Real32 GetStep() const { return m_step; }
    bool IsGrabbing() const { return m_grabbing; }
    bool RequiresGrabbing() const { return m_requireGrabbing; }      // Win 0x4af0b0
    Int32 GetConnectionCount() const { return m_connectionCount; }

private:
    // Overlap / end-cap test of two axles on one line (TestLineOverlap with the axle table),
    // grabbing and the special kinds. Mac 0x2db460, Win 0x4b08a0
    UInt32 TestPosition(bool sameDirection, const Vector3& position, const Vector3& otherPosition,
                        const Vector3& axis, const Vector3& otherAxis,
                        const ConnectionFieldAxle& other) const;
    // Aligns `other` with this axle and snaps its position along the axis. Mac 0x2dbc20
    UInt32 Snap(const Vector3& axis, Vector3& otherAxis, Real32 offset, Real32 dot,
                bool sameDirection, const Matrix3D& transform, const ConnectionFieldAxle& other,
                const Matrix3D& otherTransform, Matrix3D& snapped) const;

    Real32 m_step;              // +0x64
    bool   m_grabbing;          // +0x68
    bool   m_requireGrabbing;   // +0x69
    Int32  m_connectionCount;   // +0x6c
};

} // namespace LEGO
