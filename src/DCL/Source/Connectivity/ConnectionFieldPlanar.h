// LEGO Digital Designer — restored source
// LEGO::ConnectionFieldPlanar — grids of connection points in the local XZ plane (studs, tubes,
// ribs, pins ...), and its nine preset subclasses (KnobArrayConnectionField ...
// CustomPlanarConnectionField).
//
// TU "ConnectionFieldPlanar.cpp": Mac TU 331, 0x2e0170-0x2e98ef; Win 0x4c9600-0x4d06e0.
// Class names from RTTI (both builds); member and method names inferred.
//
// The grid has (width + 1) x (height + 1) points 0.4 apart (half the 0.8 stud pitch):
// point (column c, row r) sits at (c * 0.4, 0, r * 0.4) in field space and has the index
// r * (width + 1) + c. The field normal is the local Y axis. Every point has a point type
// (0 .. TYPE_COUNT-1, XML "type" in <Custom2DField> grids); s_connectionTable gives the
// connection type (ConnectionField::ConnectionType) of two point types.
//
// Field kinds (m_kind, XML "type" of the field): 3 KnobArray, 5 TechnicKnobArray, 10 TubeRib-
// Array, 12 TubeArray, 14 SecondaryPinArray, 16 SecondaryPin, 18 Tube, 20 NoTubeArray, 22 / 23
// Custom (Create). Compatible fields have kinds of opposite parity.
//
// Layout (Mac; Win + 4):
//   ConnectionField (... +0x58)
//   +0x5c UIntVector2 m_size              width, height (point count - 1 per direction)
//   +0x64 BasicArray<ConnectionPoint> m_points
//   +0x74 Int32 m_freeCount               points of a "linkable" type with room left
//   +0x78 Vector3 m_widthVector           (W, 0, 0)        W = width * 0.4
//   +0x84 Vector3 m_heightVector          (0, 0, H)        H = height * 0.4
//   +0x90 Vector3 m_diagonal              (W, 0, H)
//   +0x9c Vector3 m_center                (W / 2, 0, H / 2)
//   +0xa8 bool m_hasExtendedTypes         a point type > 29 was set (SetPoint)
//   sizeof 0xac + Object (Mac 0xb4, Win 0xb8).
// Mac vtable ConnectionFieldPlanar 0x74b15c.
#pragma once
#include "ConnectionField.h"

namespace LEGO {

class ConnectionFieldPlanar;

// ===========================================================================================
// ConnectionPoint — one grid point. 16 bytes. (name from the BasicArray RTTI, both builds)
// ===========================================================================================
struct ConnectionPoint {
    enum Flags {                                    // m_flags (names inferred)
        FLAG_DIRECTION_MASK = 0x3,                  // index into the GetConnectionState counts
        FLAG_GROUP          = 0x100,                // set / cleared per point group (ApplyPointGroups)
        FLAG_BLOCKED        = 0x200,                // SetPointsBlocked
        FLAG_FIXED          = 0x80000000            // point types 0-4, 30, 31, 36 (SetPoint)
    };

    ConnectionPoint* m_connection;      // +0x00 connected point of the other field (linkable types)
    ConnectionFieldPlanar* m_field;     // +0x04 owner
    UInt32 m_flags;                     // +0x08
    UInt8  m_type;                      // +0x0c point type
    UInt8  m_free;                      // +0x0d remaining capacity
    UInt8  m_capacity;                  // +0x0e
    UInt8  m_group;                     // +0x0f point group (ApplyPointGroups)
};

// ===========================================================================================
// ConnectionFieldPlanar
// ===========================================================================================
class ConnectionFieldPlanar : public ConnectionField {
public:
    // Number of point types. The Win table is 43 x 43, the Mac table 38 x 38; they agree on
    // types 0-29 (all that the shipped primitives use) and differ for the extended types.
#if defined(_WIN32)
    enum { TYPE_COUNT = 43 };
#else
    enum { TYPE_COUNT = 38 };
#endif
    enum { KIND_CUSTOM = 22 };

    // Preset subclass for `kind` (NULL for an unknown kind), sized with SetSize.
    // Mac 0x2e0f50, Win 0x4cc930
    static ConnectionFieldPlanar* Create(const Matrix3D& transform, UInt32 kind, UInt32 width,
                                         UInt32 height);

    // Fills s_connectionTable, s_linkable and s_pickable.
    // Mac 0x2e0170, Win 0x4cc3d0 (callers: BrickDatabase, BrickConnect).
    static void InitializeTables();
    // Connection type (ConnectionField::ConnectionType) of two point types.
    static UInt32 s_connectionTable[TYPE_COUNT][TYPE_COUNT];    // Mac 0x77b6e0, Win 0xb407d0
    // Point types that link to their partner point (ConnectionPoint::m_connection) and count
    // towards m_freeCount.                                         Mac 0x77cd70, Win 0xb407a0
    static bool s_linkable[TYPE_COUNT];
    // Point types offered by Pick.                                 Mac 0x77cd96, Win 0xb3ffe4
    static bool s_pickable[TYPE_COUNT];

    ConnectionFieldPlanar(const Matrix3D& transform, UInt32 kind);     // Mac inlined, Win 0x4cebe0 (EDX = transform)
    ConnectionFieldPlanar(const ConnectionFieldPlanar& o);             // Mac 0x2e5750, Win 0x4cecf0 (ECX = source)

    // Same field type and kinds of opposite parity.                  Mac 0x2e5a40, Win 0x4c96d0
    virtual bool IsCompatible(const ConnectionField& other) const;
    // The point types connect (table & 6) and neither point is full.  Mac 0x2e5a60, Win 0x4c9700
    virtual bool IsCompatible(UInt32 index, const ConnectionField& other, UInt32 otherIndex) const;
    virtual void Connect(ConnectionField& other, const ConnectionIndices& indices,
                         Attributes* attributes);                                      // Mac 0x2e43f0, Win 0x4cdf80
    virtual void Disconnect(ConnectionField& other, const ConnectionIndices& indices); // Mac 0x2e4510, Win 0x4ce060
    virtual void Clone(ConnectionField*& copy) const = 0;
    // Free grid point nearest to the ray hit on the field plane; `offset` = translation to it.
    virtual bool Pick(const Matrix3D& transform, const Vector3& origin, const Vector3& direction,
                      Real32 maxDistance, Real32& distance, UInt32& index,
                      Matrix3D& offset) const;                                         // Mac 0x2e4610, Win 0x4ce120
    // Every free grid point within reach, best per point type.       Mac 0x2e4d60, Win 0x4ce660
    virtual void Pick(const Matrix3D& transform, const Vector3& point, Real32 maxDistance,
                      PickerAccumulator& accumulator) const;
    virtual bool IsOccupied() const;                                                   // Mac 0x2e5ae0, Win 0x4c9770
    // Y axis, Z axis and the position of the first point of the connection.
    virtual void GetFrame(bool self, const ConnectionField& other, const ConnectionIndices& indices,
                          Vector3& axis, Vector3& normal, Vector3& position) const;    // Mac 0x2e5af0, Win 0x4c9780
    // Table entry of a single point pair, CONNECTION_RIGID for several pairs.
    virtual UInt32 GetConnectionType(const ConnectionField& other,
                                     const ConnectionIndices& indices) const;          // Mac 0x2e5c10, Win 0x4c98e0
    virtual bool IsRigidConnection(const ConnectionField& other,
                                   const ConnectionIndices& indices) const;            // Mac 0x2e5c70, Win 0x4af080
    // 2 if one of the points is unconnected (or, unless `any`, a connection is fixed); else
    // 1 when the connections let the parts turn, 0 otherwise.         Mac 0x2e52b0, Win 0x4ce990
    virtual UInt32 GetConnectionState(bool any, UInt32 count, const UInt32* counts) const;
    virtual void ApplyPointGroups(const BasicArray<PointGroup>& groups);              // Mac 0x2e54f0, Win 0x4ceb60
    virtual void SetPointsBlocked(bool blocked);                                       // Mac 0x2e5580, Win 0x4ceba0
    // Fields placed: the overlapping point pairs go to `indices`.    Mac 0x2e13e0, Win 0x4ccbc0
    virtual UInt32 CanConnect(const Matrix3D& transform, const ConnectionField& other,
                              const Matrix3D& otherTransform, ConnectionIndices& indices) const;
    // Snap `other` (at otherTransform) onto this grid.               Mac 0x2e2c00, Win 0x4ccd70
    virtual UInt32 CanConnect(const Matrix3D& transform, const ConnectionField& other,
                              const Matrix3D& otherTransform, Real32 maxDistance,
                              Real32& distance, Matrix3D& snapped) const;
    // Snap along a pick ray from `origin`.                            Mac 0x2e35f0, Win 0x4ccfe0
    virtual UInt32 CanConnect(const Matrix3D& transform, const ConnectionField& other,
                              const Matrix3D& otherTransform, const Vector3& origin,
                              const Vector3& direction, Real32 maxDistance, Real32& distance,
                              Matrix3D& snapped) const;
    virtual ~ConnectionFieldPlanar();                   // Mac 0x2e5ca0/0x2e5cf0, Win 0x4c9980
    // --- slot 20 (Win 18): grid size and derived vectors; the subclasses also lay out the
    // points.                                                         Mac 0x2e5d60, Win 0x4c9a20
    virtual void SetSize(UInt32 width, UInt32 height);

    using ConnectionField::Pick;

    UInt32 GetWidth() const { return m_size.x; }
    UInt32 GetHeight() const { return m_size.y; }
    UInt32 GetPointCount() const { return m_points.GetSize(); }
    const ConnectionPoint& GetPoint(UInt32 index) const { return m_points[index]; }
    ConnectionPoint& GetPoint(UInt32 index) { return m_points[index]; }
    Int32 GetFreeCount() const { return m_freeCount; }
    bool HasExtendedTypes() const { return m_hasExtendedTypes; }
    const Vector3& GetWidthVector() const { return m_widthVector; }
    const Vector3& GetHeightVector() const { return m_heightVector; }
    const Vector3& GetDiagonal() const { return m_diagonal; }
    const Vector3& GetCenter() const { return m_center; }

    // One point of a <Custom2DField> grid ("type:capacity:flags:group"); keeps m_freeCount
    // and m_hasExtendedTypes up to date. Only the low 8 bits of `flags` are kept; FLAG_FIXED is
    // derived from the type. (inlined in the LEGOPrimitive loader, Mac 0x87ef0)
    void SetPoint(UInt32 index, UInt8 type, UInt8 capacity, UInt32 flags, UInt8 group)
    {
        if (type > 29)
            m_hasExtendedTypes = true;
        ConnectionPoint& p = m_points[index];
        if (s_linkable[p.m_type])
            --m_freeCount;
        p.m_type = type;
        flags &= 0xff;
        if ((type | 2) == 3 || (type < 5 && (type & 1) == 0) ||
            (type == 30 || type == 31 || type == 36))
            flags |= ConnectionPoint::FLAG_FIXED;
        p.m_flags = flags;
        p.m_free = capacity;
        p.m_capacity = capacity;
        p.m_group = group;
        if (s_linkable[type])
            ++m_freeCount;
    }

protected:
    // Sizes m_points to (width + 1) * (height + 1); new points are copies of `fill`.
    void ResizePoints(const ConnectionPoint& fill);
    // Counts point `index` in m_freeCount when its type is linkable.
    void CountPoint(UInt32 index);
    // A free point offered by Pick (Win 0x4c96a0).
    bool IsPickable(const ConnectionPoint& p) const
    {
        return (p.m_free != 0 || p.m_capacity == 0) && s_pickable[p.m_type];
    }

private:
    // Placement of `otherTransform` in the frame of `transform`: false unless the Y axes are
    // parallel and the planes coincide (0.04). `dx`, `dz`: offset of the other origin; `angle`
    // of the other X axis about Y; `aligned` with `quarter` (0-3) when it is a right angle.
    // Mac 0x2e1660, Win 0x4cfb40
    static bool GetRelativePlacement(const Matrix3D& transform, const Matrix3D& otherTransform,
                                     Real32& dx, Real32& dz, Real32& angle, UInt32& quarter,
                                     bool& aligned);
    // Bounding spheres of the two grids intersect.                  Mac 0x2e19d0, Win 0x4d0070
    bool BoundsIntersect(const Matrix3D& transform, const ConnectionFieldPlanar& other,
                         const Matrix3D& otherTransform) const;
    // Rectangle of this grid covered by `other` placed at grid point (x, z) turned by
    // `quarter` * 90 degrees, and the steps of the other grid's columns / rows in this grid.
    // Mac 0x2e5900, Win 0x4cee50
    bool GetOverlap(const ConnectionFieldPlanar& other, UInt32 quarter, Int32 x, Int32 z,
                    IntVector2& stepX, IntVector2& stepY, IntRect& rect) const;
    // Combines the table entries of all coinciding point pairs: 0 none, 1 blocked, 2 rigid,
    // 4 a single free (rotating) pair. Records the pairs in `indices` when given.
    // Mac 0x2e1c60, Win 0x4cef90
    UInt32 TestOverlap(const ConnectionFieldPlanar& other, UInt32 quarter, Int32 x, Int32 z,
                       ConnectionIndices* indices) const;
    // Same for an arbitrary angle (points within 0.1 of a grid point).
    // Mac 0x2e2350, Win 0x4cf1a0
    UInt32 TestFreeAngle(const ConnectionFieldPlanar& other, Real32 angle, Real32 dx, Real32 dz,
                         ConnectionIndices* indices) const;
    // Snaps the other field, whose origin is at `position` (field plane coordinates) minus the
    // rotated `offset`, to the nearest grid position (0.4, then 0.8 / 1.6 for extended types).
    // `point` is the reference for `distance`. Mac 0x2e2e50, Win 0x4cf510
    UInt32 Snap(const Vector2& position, const Vector2& offset, const Matrix3D& transform,
                const ConnectionFieldPlanar& other, const Matrix3D& otherTransform,
                const Vector3& point, Real32 maxDistance, Real32& distance,
                Matrix3D& snapped) const;

protected:
    UIntVector2 m_size;                     // +0x5c
    BasicArray<ConnectionPoint> m_points;   // +0x64
    Int32   m_freeCount;                    // +0x74
    Vector3 m_widthVector;                  // +0x78
    Vector3 m_heightVector;                 // +0x84
    Vector3 m_diagonal;                     // +0x90
    Vector3 m_center;                       // +0x9c
    bool    m_hasExtendedTypes;             // +0xa8
};

// ===========================================================================================
// The presets. Point layouts are written by SetSize; the comments give the point types.
// Each: Clone (Mac / Win addresses), destructor, SetSize.
// ===========================================================================================

// Kind 3: studs. Corners 0x12 (capacity 1), edges 0x12 / 0x17, stud centres 0 (fixed).
// Mac vtable 0x775d84
class KnobArrayConnectionField : public ConnectionFieldPlanar {
public:
    explicit KnobArrayConnectionField(const Matrix3D& transform);           // Win 0x4c9e70
    KnobArrayConnectionField(const KnobArrayConnectionField& o);            // Win 0x4c9f60
    virtual void Clone(ConnectionField*& copy) const;                       // Mac 0x2e5ee0, Win 0x4c9af0
    virtual ~KnobArrayConnectionField();                                    // Mac 0x2e5f90/0x2e5fe0, Win 0x4c9f00
    virtual void SetSize(UInt32 width, UInt32 height);                      // Mac 0x2e6050, Win 0x4c9b80
};

// Kind 5: Technic studs (hollow): as KnobArray with type 1 stud centres.
class TechnicKnobArrayConnectionField : public ConnectionFieldPlanar {
public:
    explicit TechnicKnobArrayConnectionField(const Matrix3D& transform);    // Win 0x4ca360
    TechnicKnobArrayConnectionField(const TechnicKnobArrayConnectionField& o); // Win 0x4ca450
    virtual void Clone(ConnectionField*& copy) const;                       // Mac 0x2e6ec0, Win 0x4ca030
    virtual ~TechnicKnobArrayConnectionField();                             // Mac 0x2e6f70/0x2e6fc0, Win 0x4ca3f0
    virtual void SetSize(UInt32 width, UInt32 height);                      // Mac 0x2e7030, Win 0x4ca0c0
};

// Kind 10: underside with tubes every 2 studs and ribs.
class TubeRibArrayConnectionField : public ConnectionFieldPlanar {
public:
    explicit TubeRibArrayConnectionField(const Matrix3D& transform);        // Win 0x4ca980
    TubeRibArrayConnectionField(const TubeRibArrayConnectionField& o);      // Win 0x4caa70
    virtual void Clone(ConnectionField*& copy) const;                       // Mac 0x2e7430, Win 0x4ca520
    virtual ~TubeRibArrayConnectionField();                                 // Mac 0x2e74e0/0x2e7530, Win 0x4caa10
    virtual void SetSize(UInt32 width, UInt32 height);                      // Mac 0x2e75a0, Win 0x4ca5b0
};

// Kind 12: underside with tubes.
class TubeArrayConnectionField : public ConnectionFieldPlanar {
public:
    explicit TubeArrayConnectionField(const Matrix3D& transform);           // Win 0x4cae80
    TubeArrayConnectionField(const TubeArrayConnectionField& o);            // Win 0x4caf70
    virtual void Clone(ConnectionField*& copy) const;                       // Mac 0x2e7a10, Win 0x4cab40
    virtual ~TubeArrayConnectionField();                                    // Mac 0x2e7ac0/0x2e7b10, Win 0x4caf10
    virtual void SetSize(UInt32 width, UInt32 height);                      // Mac 0x2e7b80, Win 0x4cabd0
};

// Kind 14: underside of a 1-wide plate with secondary pins.
class SecondaryPinArrayConnectionField : public ConnectionFieldPlanar {
public:
    explicit SecondaryPinArrayConnectionField(const Matrix3D& transform);   // Win 0x4cb3d0
    SecondaryPinArrayConnectionField(const SecondaryPinArrayConnectionField& o); // Win 0x4cb4c0
    virtual void Clone(ConnectionField*& copy) const;                       // Mac 0x2e7f00, Win 0x4cb040
    virtual ~SecondaryPinArrayConnectionField();                            // Mac 0x2e7fb0/0x2e8000, Win 0x4cb460
    virtual void SetSize(UInt32 width, UInt32 height);                      // Mac 0x2e8070, Win 0x4cb0d0
};

// Kind 16: a single secondary pin (point 4, the centre of a 2 x 2 grid).
class SecondaryPinConnectionField : public ConnectionFieldPlanar {
public:
    explicit SecondaryPinConnectionField(const Matrix3D& transform);        // Win 0x4cb6f0
    SecondaryPinConnectionField(const SecondaryPinConnectionField& o);      // Win 0x4cb7e0
    virtual void Clone(ConnectionField*& copy) const;                       // Mac 0x2e83d0, Win 0x4cb590
    virtual ~SecondaryPinConnectionField();                                 // Mac 0x2e8480/0x2e84d0, Win 0x4cb780
    virtual void SetSize(UInt32 width, UInt32 height);                      // Mac 0x2e8540, Win 0x4cb620
};

// Kind 18: a single tube (point 4).
class TubeConnectionField : public ConnectionFieldPlanar {
public:
    explicit TubeConnectionField(const Matrix3D& transform);                // Win 0x4cba10
    TubeConnectionField(const TubeConnectionField& o);                      // Win 0x4cbb00
    virtual void Clone(ConnectionField*& copy) const;                       // Mac 0x2e8760, Win 0x4cb8b0
    virtual ~TubeConnectionField();                                         // Mac 0x2e8810/0x2e8860, Win 0x4cbaa0
    virtual void SetSize(UInt32 width, UInt32 height);                      // Mac 0x2e88d0, Win 0x4cb940
};

// Kind 20: underside without tubes.
class NoTubeArrayConnectionField : public ConnectionFieldPlanar {
public:
    explicit NoTubeArrayConnectionField(const Matrix3D& transform);         // Win 0x4cbf20
    NoTubeArrayConnectionField(const NoTubeArrayConnectionField& o);        // Win 0x4cc010
    virtual void Clone(ConnectionField*& copy) const;                       // Mac 0x2e8af0, Win 0x4cbbd0
    virtual ~NoTubeArrayConnectionField();                                  // Mac 0x2e8ba0/0x2e8bf0, Win 0x4cbfb0
    virtual void SetSize(UInt32 width, UInt32 height);                      // Mac 0x2e8c60, Win 0x4cbc60
};

// Kinds 22 / 23 (`female`): <Custom2DField>, points set by the loader (SetPoint).
class CustomPlanarConnectionField : public ConnectionFieldPlanar {
public:
    CustomPlanarConnectionField(const Matrix3D& transform, bool female);    // Win 0x4cc200
    CustomPlanarConnectionField(const CustomPlanarConnectionField& o);      // Win 0x4cc300
    virtual void Clone(ConnectionField*& copy) const;                       // Mac 0x2e9000, Win 0x4cc0e0
    virtual ~CustomPlanarConnectionField();                                 // Mac 0x2e90b0/0x2e9100, Win 0x4cc2a0
    // All points type 29 (no connection), capacity 0.               Mac 0x2e9170, Win 0x4cc170
    virtual void SetSize(UInt32 width, UInt32 height);
};

} // namespace LEGO
