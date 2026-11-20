// LEGO Digital Designer — restored source
// LEGO::ConnectionField — a connection feature of a brick primitive (planar stud/tube grid,
// axle or hole, hinge, ball, fixed point, rail, slider) and the helper bases
// ConnectionFieldLineHelper (axle, rail, slider) and ConnectionFieldPointHelper (ball, fixed,
// hinge). ConnectionField.cpp = Mac TU 384 (0x372340-0x372e1f); subclasses in
// ConnectionField<Kind>.cpp (Mac TUs 327-333). Class names from RTTI; other names inferred.
//
// Fields are created by the LEGOPrimitive XML loader (BrickDatabase, Mac 0x83eb0) from the
// <Connectivity> elements of Primitives/*.xml:
//   <Custom2DField type width height ...>  CustomPlanarConnectionField::Create (grid text)
//   <Axel type length startCapped endCapped grabbing requireGrabbing ...>  ConnectionFieldAxle
//   <Hinge type oriented [LimMin LimMax FlipLimMin FlipLimMax] ...>         ConnectionFieldHinge
//   <Ball type [flexAttributes] ...>                                       ConnectionFieldBall
//   <Fixed type axes ...>                                                  ConnectionFieldFixed
//   <Rail type [length] ...>                                               ConnectionFieldRail
//   <Slider type length startCapped endCapped ...>                         ConnectionFieldSlider
// ("angle ax ay az tx ty tz" give the field transform; the XML "type" is the field kind.)
//
// Field kinds (m_kind) come in gender pairs 2n / 2n+1 (e.g. axle 2 / hole 3); two fields are
// compatible when their types match and, for most classes, kind ^ otherKind == 1. Axles and
// planar points use connection tables instead (ConnectionFieldAxle.cpp / ConnectionFieldPlanar
// .cpp). Connection type values (tables, CanConnect results):
//   0 none, 1 blocked (collision), 2 rigid, 4 free (rotating / sliding), 6 snap (2 | 4).
//
// Layout (Mac i386; Win: + vbptr at +4, every member 4 bytes later):
//   +0x00 vptr  (+0x04 Win vbptr)
//   +0x04 Matrix3D m_transform (field frame in primitive space, 64 bytes)
//   +0x44 ConnectionFieldContext* m_context
//   +0x48 ConnectionFieldObserver* m_observer
//   +0x4c UInt32 m_type        FieldType
//   +0x50 UInt32 m_kind        XML "type"
//   +0x54 UInt32 m_flags       bit 0 toggled by FilterConnectionFieldObserver (Mac 0xc6490)
//   +0x58 UInt32 m_reserved    never initialised by the constructors, copied by Clone
//   +0x5c (Win +0x60) end of ConnectionField; virtual base Object follows the most derived class
//
// Virtual slots (Mac declaration order; Win: the overload groups Pick (6, 7) and CanConnect
// (15-17) appear in reverse order, the MSVC rule for overloaded virtuals, and the destructor
// lives only in the Object part):
//    0 IsCompatible(other)                 9 GetFrame(...)
//    1 IsCompatible(index, other, index)  10 GetConnectionType(other, indices)
//    2 GetConnectionAngle(...)            11 IsRigidConnection(other, indices)
//    3 Connect(...)                       12 GetConnectionState(...)
//    4 Disconnect(...)                    13 ApplyPointGroups(...)
//    5 Clone(...)                         14 SetPointsBlocked(...)
//    6 Pick(ray ...)                      15 CanConnect(m, other, otherM, indices)
//    7 Pick(point, accumulator)           16 CanConnect(m, other, otherM, maxDist, ...)
//    8 IsOccupied()                       17 CanConnect(m, other, otherM, origin, dir, ...)
//   18 ~ConnectionField (Mac D1, 19 D0)   20 SetSize (ConnectionFieldPlanar only)
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "Geometry.h"

namespace LEGO {

class ConnectionField;

// ===========================================================================================
// ConnectionFieldContext — the owner of a field in a scene (implemented by PrimitiveElement,
// Mac vtable 0x706108 at +0x14 of PrimitiveElement). No base class (Win RTTI).
// ===========================================================================================
class ConnectionFieldContext {
public:
    // World transform of the owning primitive. (PrimitiveElement: Mac 0xc4360)
    virtual void GetTransform(Matrix3D& transform) const = 0;
    // Both contexts belong to the same rigid group. (PrimitiveElement: Mac 0x9d820)
    virtual bool IsSameRigid(const ConnectionFieldContext* other) const = 0;
    // (PrimitiveElement: Mac 0x9d880, returns its rigid's +0x38)
    virtual void* GetRigid() const = 0;
};

class ConnectionFieldObserver;

// ===========================================================================================
// ConnectionField
// ===========================================================================================
class ConnectionField : public virtual Object {
public:
    enum FieldType {                                    // m_type (names inferred)
        TYPE_PLANAR = 0, TYPE_HINGE = 1, TYPE_AXLE = 2, TYPE_BALL = 3,
        TYPE_FIXED = 4, TYPE_RAIL = 5, TYPE_SLIDER = 6, TYPE_COUNT = 7
    };
    enum ConnectionType {                               // (names inferred)
        CONNECTION_NONE = 0, CONNECTION_BLOCKED = 1, CONNECTION_RIGID = 2,
        CONNECTION_FREE = 4, CONNECTION_SNAP = 6
    };

    // Pair of connection point indices (this field, other field); planar fields use the grid
    // index (row * (width + 1) + column), all other fields 0. 8 bytes.
    struct ConnectionIndex {
        UInt32 m_index;         // +0x00
        UInt32 m_otherIndex;    // +0x04
        ConnectionIndex() : m_index(0), m_otherIndex(0) {}
        ConnectionIndex(UInt32 index, UInt32 otherIndex) : m_index(index), m_otherIndex(otherIndex) {}
    };

    // The point pairs of one connection (Mac vtable 0x7625ac: D1 0xb4990, D0 0xc8950; Win
    // 0xa23a44: 0x46e0e0). sizeof 0x14.
    struct ConnectionIndices : public BasicArray<ConnectionIndex> {
        ConnectionIndices() : m_reversed(0) {}
        // Axles: the connection runs against the field direction (set by CanConnect, read by
        // GetFrame). (name inferred)
        UInt32 m_reversed;      // +0x10
    };

    // Joint description handed to the physics layer (axes of both fields and limits). Filled
    // by Connect; transformed into world space by Transform. sizeof 0xb4 (both builds).
    struct Attributes {
        Vector3 m_axis;             // +0x00 first field: axis (field Y)
        Vector3 m_normal;           // +0x0c first field: normal (field Z)
        Vector3 m_position;         // +0x18 first field: origin
        Vector3 m_otherAxis;        // +0x24 second field
        Vector3 m_otherNormal;      // +0x30
        Vector3 m_otherPosition;    // +0x3c
        Vector3 m_linear;           // +0x48 (m_hasLinear)
        Vector3 m_linearLimits;     // +0x54 (m_hasLinear) Ball: (flex x, flex y, flex y)
        Vector3 m_lowerLimit;       // +0x60 (m_hasLimits) only x used by the fields here
        Vector3 m_upperLimit;       // +0x6c (m_hasLimits)
        Real32  m_twist;            // +0x78 (m_hasLimits)
        Vector3 m_spring;           // +0x7c (m_hasSpring)
        Vector3 m_damping;          // +0x88 (m_hasSpring)
        Vector3 m_lowerAngle;       // +0x94 (m_hasAngles)
        Vector3 m_upperAngle;       // +0xa0 (m_hasAngles)
        bool    m_hasLinear;        // +0xac
        bool    m_hasLimits;        // +0xad
        bool    m_hasSpring;        // +0xae
        bool    m_hasAngles;        // +0xaf
        bool    m_isFlexible;       // +0xb0

        Attributes();
        // `out` = this with the first frame transformed by `transform` and the second by
        // `otherTransform`; `swap` exchanges the two frames (and mirrors the limits).
        // Mac 0x372380
        void TransformFrames(bool swap, const Matrix3D& transform, const Matrix3D& otherTransform,
                             Attributes& out) const;
        // TransformFrames + m_isFlexible. Mac 0x372340 (callers Mac 0x989f0, 0x98fa0)
        void Transform(bool swap, const Matrix3D& transform, const Matrix3D& otherTransform,
                       Attributes& out) const;
    };

    // Best pick candidates per field type. 7 arrays of PickedFeature (Mac 0x3726b0 GetBest,
    // Win 0x4a2010).
    struct PickerAccumulator {
        struct PickedFeature {                          // sizeof 0x50
            const ConnectionField* m_field;             // +0x00
            UInt32   m_index;                           // +0x04 point index (planar)
            Matrix3D m_transform;                       // +0x08 snapped transform
            UInt32   m_kind;                            // +0x48 field kind
            Real32   m_distance;                        // +0x4c squared distance
        };
        BasicArray<PickedFeature> m_features[TYPE_COUNT];   // +0x00, 16 bytes each

        // Feature with the smallest distance over all types, NULL if none.
        const PickedFeature* GetBest() const;           // Mac 0x3726b0, Win 0x4a2010
        // Records `feature` unless a feature of the same kind is closer. (inlined)
        void Add(UInt32 type, const PickedFeature& feature);
    };

    ConnectionField(const Matrix3D& transform, UInt32 type, UInt32 kind);
    ConnectionField(const ConnectionField& o);

    // --- slot 0: same field type and complementary kind (gender).
    virtual bool IsCompatible(const ConnectionField& other) const = 0;
    // --- slot 1: compatibility of point `index` with point `otherIndex` of `other`.
    virtual bool IsCompatible(UInt32 index, const ConnectionField& other, UInt32 otherIndex) const = 0;
    // --- slot 2: rotation angle of a connected pair about `axis`; returns a ConnectionType
    // (CONNECTION_FREE when an angle was computed). Base: 0.  Mac 0x2dd530, Win 0x4aed50
    virtual UInt32 GetConnectionAngle(const Matrix3D& transform, const ConnectionField& other,
                                      const Matrix3D& otherTransform, const Vector3& axis,
                                      const Vector3& reference, const Vector3& center,
                                      Real32& angle) const;
    // --- slots 3, 4
    virtual void Connect(ConnectionField& other, const ConnectionIndices& indices,
                         Attributes* attributes) = 0;
    virtual void Disconnect(ConnectionField& other, const ConnectionIndices& indices) = 0;
    // --- slot 5: new copy (not referenced: the caller takes the first reference).
    virtual void Clone(ConnectionField*& copy) const = 0;
    // --- slot 6: ray pick (field in world space at `transform`): squared distance, point
    // index and the snap offset (relative to the field) of the picked point.
    virtual bool Pick(const Matrix3D& transform, const Vector3& origin, const Vector3& direction,
                      Real32 maxDistance, Real32& distance, UInt32& index, Matrix3D& offset) const = 0;
    // --- slot 7: point pick into an accumulator.
    virtual void Pick(const Matrix3D& transform, const Vector3& point, Real32 maxDistance,
                      PickerAccumulator& accumulator) const = 0;
    // --- slot 8: no free connection left.
    virtual bool IsOccupied() const = 0;
    // --- slot 9: frame of this field (`self`) or of `other` for the connection `indices`.
    virtual void GetFrame(bool self, const ConnectionField& other, const ConnectionIndices& indices,
                          Vector3& axis, Vector3& normal, Vector3& position) const = 0;
    // --- slot 10: connection type for the connection `indices` (ConnectionType).
    virtual UInt32 GetConnectionType(const ConnectionField& other,
                                     const ConnectionIndices& indices) const = 0;
    // --- slot 11: GetConnectionType(...) == CONNECTION_RIGID (Axle / Planar) or constant.
    virtual bool IsRigidConnection(const ConnectionField& other,
                                   const ConnectionIndices& indices) const = 0;
    // --- slot 12: rigidity of the connections of a set of points (planar). `counts` holds
    // count records { point index, connections per direction [3] }.
    virtual UInt32 GetConnectionState(bool any, UInt32 count, const UInt32* counts) const = 0;
    // --- slot 13: per point group overrides of point flag 0x100 (planar). Base: nothing.
    struct PointGroup { bool m_enabled; UInt32 m_flags; };       // 8 bytes
    virtual void ApplyPointGroups(const BasicArray<PointGroup>& groups);   // Mac 0x2dd550, Win 0x40be80
    // --- slot 14: point flag 0x200 on every point (planar). Base: nothing.
    virtual void SetPointsBlocked(bool blocked);                           // Mac 0x2dd560, Win 0x40be80
    // --- slots 15-17
    virtual UInt32 CanConnect(const Matrix3D& transform, const ConnectionField& other,
                              const Matrix3D& otherTransform, ConnectionIndices& indices) const = 0;
    virtual UInt32 CanConnect(const Matrix3D& transform, const ConnectionField& other,
                              const Matrix3D& otherTransform, Real32 maxDistance,
                              Real32& distance, Matrix3D& snapped) const = 0;
    virtual UInt32 CanConnect(const Matrix3D& transform, const ConnectionField& other,
                              const Matrix3D& otherTransform, const Vector3& origin,
                              const Vector3& direction, Real32 maxDistance, Real32& distance,
                              Matrix3D& snapped) const = 0;
    // --- slot 18
    virtual ~ConnectionField();                         // Mac 0x2dd570/0x2dd590, Win 0x4aed80

    const Matrix3D& GetTransform() const { return m_transform; }
    UInt32 GetType() const { return m_type; }
    UInt32 GetKind() const { return m_kind; }
    UInt32 GetFlags() const { return m_flags; }
    void   SetFlags(UInt32 flags) { m_flags = flags; }
    ConnectionFieldContext* GetContext() const { return m_context; }
    void   SetContext(ConnectionFieldContext* context) { m_context = context; }
    ConnectionFieldObserver* GetObserver() const { return m_observer; }
    void   SetObserver(ConnectionFieldObserver* observer) { m_observer = observer; }
    // m_reserved of a clone owned by a PrimitiveElement is the field's index in
    // PrimitiveElement::m_fields (set by the PrimitiveElement constructor, Mac 0x9e7f0, and
    // used to find the knob collision features, Mac 0x97310 / 0x9dc10 / 0x9ffd0).
    // (name inferred)
    UInt32 GetIndex() const { return m_reserved; }
    void   SetIndex(UInt32 index) { m_reserved = index; }

protected:
    // Observer calls of Connect / Disconnect.        Mac 0x372820 / 0x372880 (Win inlined)
    static void NotifyConnect(ConnectionField& field, ConnectionField& other,
                              const ConnectionIndices& indices, Attributes* attributes);
    static void NotifyDisconnect(ConnectionField& field, ConnectionField& other,
                                 const ConnectionIndices& indices);

    Matrix3D                 m_transform;   // +0x04
    ConnectionFieldContext*  m_context;     // +0x44
    ConnectionFieldObserver* m_observer;    // +0x48
    UInt32                   m_type;        // +0x4c
    UInt32                   m_kind;        // +0x50
    UInt32                   m_flags;       // +0x54
    UInt32                   m_reserved;    // +0x58 NOTE: not initialised by the constructor

private:
    ConnectionField& operator=(const ConnectionField&);
};

// ===========================================================================================
// ConnectionFieldObserver — notified when a field gets connected / disconnected.
// Implementations: FilterConnectionFieldObserver and the MiniFig* observers (Mac 0xc6490 /
// 0xc65b0, Win 0x476410 / 0x476480). No base class, no vtable of its own (abstract).
// `first` is true for the observer of `field`, false for the observer of `other`.
// ===========================================================================================
class ConnectionFieldObserver {
public:
    virtual void OnConnect(bool first, ConnectionField& field, ConnectionField& other,
                           const ConnectionField::ConnectionIndices& indices,
                           ConnectionField::Attributes* attributes) = 0;
    virtual void OnDisconnect(bool first, ConnectionField& field, ConnectionField& other,
                              const ConnectionField::ConnectionIndices& indices) = 0;
};

// ===========================================================================================
// ConnectionFieldLineHelper — fields along the local Y axis, from the origin to
// (0, m_length, 0). +0x5c (Win +0x60). sizeof 0x64 (Win 0x68) + Object.
// Win vtable 0xa246cc (Mac: never emitted, all users override it).
// ===========================================================================================
class ConnectionFieldLineHelper : public ConnectionField {
public:
    ConnectionFieldLineHelper(const Matrix3D& transform, UInt32 type, UInt32 kind, Real32 length,
                              bool startCapped, bool endCapped);   // Win 0x4aee00 (EDX = matrix)
    ConnectionFieldLineHelper(const ConnectionFieldLineHelper& o); // Win 0x4af2f0

    // Mac 0x2dd4e0, Win 0x4dbb30
    virtual bool IsCompatible(const ConnectionField& other) const
    {
        return other.GetType() == m_type && (other.GetKind() ^ m_kind) == 1;
    }
    // Mac 0x2dd510, Win 0x4aedf0
    virtual bool IsCompatible(UInt32, const ConnectionField& other, UInt32) const
    {
        return IsCompatible(other);
    }
    // Mac 0x2dd540, Win 0x4aede0
    virtual bool IsOccupied() const { return false; }
    // Closest point of the segment.                                  Mac 0x3728d0, Win 0x4a2060
    virtual void Pick(const Matrix3D& transform, const Vector3& point, Real32 maxDistance,
                      PickerAccumulator& accumulator) const;
    using ConnectionField::Pick;
    using ConnectionField::IsCompatible;
    virtual ~ConnectionFieldLineHelper() {}             // Win 0x4aeec0/0x4aef30

    Real32 GetLength() const { return m_length; }
    bool IsStartCapped() const { return m_startCapped; }      // Win 0x4aedc0
    bool IsEndCapped() const { return m_endCapped; }          // Win 0x4aedd0

protected:
    Real32 m_length;        // +0x5c
    bool   m_startCapped;   // +0x60
    bool   m_endCapped;     // +0x61
};

// ===========================================================================================
// ConnectionFieldPointHelper — fields at the origin that take one connection.
// +0x5c bool m_occupied. sizeof 0x60 (Win 0x64) + Object.
// Mac vtable 0x74acac, Win 0xa25804.
// ===========================================================================================
class ConnectionFieldPointHelper : public ConnectionField {
public:
    ConnectionFieldPointHelper(const Matrix3D& transform, UInt32 type, UInt32 kind); // Win 0x4dca70
    ConnectionFieldPointHelper(const ConnectionFieldPointHelper& o);                // Win 0x4dcda0

    // Mac 0x2de050, Win 0x4dbb30
    virtual bool IsCompatible(const ConnectionField& other) const
    {
        return other.GetType() == m_type && (other.GetKind() ^ m_kind) == 1;
    }
    // Mac 0x2de080, Win 0x4dca30
    virtual bool IsCompatible(UInt32, const ConnectionField& other, UInt32) const
    {
        return IsCompatible(other) && !IsOccupied() && !other.IsOccupied();
    }
    // Mac 0x2de0c0, Win 0x4df3c0
    virtual bool IsOccupied() const { return m_occupied; }
    // Field origin.                                                  Mac 0x372b90, Win 0x4a21c0
    virtual void Pick(const Matrix3D& transform, const Vector3& point, Real32 maxDistance,
                      PickerAccumulator& accumulator) const;
    using ConnectionField::Pick;
    using ConnectionField::IsCompatible;
    virtual ~ConnectionFieldPointHelper() {}            // Mac 0x2de0d0/0x2de110, Win 0x4dcb20/0x4dcb90

    void SetOccupied(bool occupied) { m_occupied = occupied; }

protected:
    bool m_occupied;        // +0x5c
};

} // namespace LEGO
