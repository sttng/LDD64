// LEGO Digital Designer — restored source
// TU "ConnectionFieldPlanar.cpp"  Mac TU 331, 0x2e0170-0x2e98ef (initialiser 0x2e98b0: <iostream>
// only); Win 0x4c9600-0x4d06e0. See ConnectionFieldPlanar.h.
// The BasicArray<ConnectionPoint> members (Mac 0x2e6450 Insert, 0x2e9390 assign, 0x2e9360 dtor;
// Win 0x4d07a0, 0x4d06e0, 0x4d06b0) are the BasicArray template.
#include "ConnectionFieldPlanar.h"

#include <math.h>
#include <iostream>

namespace LEGO {

namespace {

const Real32 GRID = 0.4f;           // point pitch (Mac 0x3ecccccd)

// floor or ceil, whichever is nearer (ceil on a tie); Mac inline _floorf/_ceilf + cvttss2si,
// Win 0x4c9630 + _ftol.
inline Int32 RoundToNearest(Real32 value)
{
    Real32 f = floorf(value);
    Real32 c = ceilf(value);
    return (Int32)(value - f < c - value ? f : c);
}

const Vector3& AxisX() { static const Vector3 v(1.0f, 0.0f, 0.0f); return v; }
const Vector3& AxisY() { static const Vector3 v(0.0f, 1.0f, 0.0f); return v; }
const Vector3& AxisZ() { static const Vector3 v(0.0f, 0.0f, 1.0f); return v; }

// The table data. The original builds the tables with a long sequence of assignments and
// row / column copies (Mac 0x2e0170, Win 0x4cc3d0); restored as the resulting rows, one
// digit per entry (ConnectionField::ConnectionType). Mac and Win agree on types 0-29.
#if defined(_WIN32)
// Win 0x4cc3d0 result (43 x 43; checked against the original data by diff_connectionfield)
static const char* const kTableRows[ConnectionFieldPlanar::TYPE_COUNT] = {
    "1111144441111144141111101111101111111111111", // 0
    "1111144444141144141111100111101111111111111", // 1
    "1111144441111144441111101111101111111111111", // 2
    "1111144444141144441111100111101111111111111", // 3
    "1111111111111111121111111111101111111111111", // 4
    "4444111110101111114401111111101111111111111", // 5
    "4444111110101111114101111111101111111111111", // 6
    "4444111110101111110401101111101111111111111", // 7
    "4444111110101111110101101111101111111111111", // 8
    "1414100001111000000011101111101111111111111", // 9
    "1111111111111000000011101111101111111141111", // 10
    "1414100001111000000041101111101111111141111", // 11
    "1111111111111000000041101111101111111141111", // 12
    "1111111110000111112101111111101111111111111", // 13
    "4444111110000111112101111111101111111111111", // 14
    "4444111110000111110101101111101111111111111", // 15
    "1144111110000111111100111111101111111111111", // 16
    "4444211110000111110101101111101111111111111", // 17
    "1111144000000220100110000111101111111111111", // 18
    "1111141410000111111110010111101111111111111", // 19
    "1111100001144000001114111111101111111111111", // 20
    "1111111111111111010041101111101111111111111", // 21
    "1111111111111111110011001111101111101111111", // 22
    "0000111000000110100110000111100000100111111", // 23
    "1010111111111111110011101111101111111111111", // 24
    "1111111111111111111111111112101111111111111", // 25
    "1111111111111111111111111111201111111111111", // 26
    "1111111111111111111111111211101111111111111", // 27
    "1111111111111111111111111121101111111111111", // 28
    "0000000000000000000000000000000000000000000", // 29
    "1111111111111111111111101111101144411010111", // 30
    "1111111111111111111111101111101144411010440", // 31
    "1111111111111111111111101111104411101111001", // 32
    "1111111111111111111111101111104411141111100", // 33
    "1111111111111111111111111111104411121111100", // 34
    "1111111111111111111111001111101104201012111", // 35
    "1111111111111111111111101111101111111414140", // 36
    "1111111111111111111111111111100011104111100", // 37
    "1111111111444111111111111111101111111111111", // 38
    "1111111111111111111111111111100011124111100", // 39
    "1111111111111111111111111111101401111111111", // 40
    "1111111111111111111111111111101400014010111", // 41
    "1111111111111111111111111111101010010010111", // 42
};
static const char kLinkable[] = "1111111111011111110111000111101111101100000";
static const char kPickable[] = "1111111111011111011111000111101111111110000";
#else
// Mac 0x2e0170 result (38 x 38)
static const char* const kTableRows[ConnectionFieldPlanar::TYPE_COUNT] = {
    "11111444411111441411111011111011111111", // 0
    "11111444441411441411111001111011111111", // 1
    "11111444411111444411111011111011111111", // 2
    "11111444441411444411111001111011111111", // 3
    "11111111111111111211111111111011111111", // 4
    "44441111101011111144011111111011111111", // 5
    "44441111101011111141011111111011111111", // 6
    "44441111101011111104011011111011111111", // 7
    "44441111101011111101011011111011111111", // 8
    "14141000011110000000111011111011111111", // 9
    "11111111111110000000111011111011111111", // 10
    "14141000011110000000411011111011111111", // 11
    "11111111111110000000411011111011111111", // 12
    "11111111100001111121011111111011111111", // 13
    "44441111100001111121011111111011111111", // 14
    "44441111100001111101011011111011111111", // 15
    "11441111100001111111001111111011111111", // 16
    "44442111100001111101011011111011111111", // 17
    "11111440000002201001100001111011111111", // 18
    "11111414100001111111100101111011111111", // 19
    "11111000011440000011141111111011111111", // 20
    "11111111111111110100411011111011111111", // 21
    "11111111111111111100110011111011111011", // 22
    "00001110000001101001100001111000001001", // 23
    "10101111111111111100111011111011111111", // 24
    "11111111111111111111111111121011111111", // 25
    "11111111111111111111111111112011111111", // 26
    "11111111111111111111111112111011111111", // 27
    "11111111111111111111111111211011111111", // 28
    "00000000000000000000000000000000000000", // 29
    "11111111111111111111111011111011411111", // 30
    "11111111111111111111111011111011411111", // 31
    "11111111111111111111111011111044111011", // 32
    "11111111111111111111111011111011111411", // 33
    "11111111111111111111111111111011111211", // 34
    "11111111111111111111110011111011042010", // 35
    "11111111111111111111111011111011111114", // 36
    "11111111111111111111111111111011111041", // 37
};
static const char kLinkable[] = "11111111110111111101110001111011111011";
static const char kPickable[] = "11111111110111110111110001111011111111";
#endif

} // namespace

UInt32 ConnectionFieldPlanar::s_connectionTable[TYPE_COUNT][TYPE_COUNT];
bool ConnectionFieldPlanar::s_linkable[TYPE_COUNT];
bool ConnectionFieldPlanar::s_pickable[TYPE_COUNT];

// Mac 0x2e0170, Win 0x4cc3d0
void ConnectionFieldPlanar::InitializeTables()
{
    for (UInt32 i = 0; i < TYPE_COUNT; ++i) {
        for (UInt32 j = 0; j < TYPE_COUNT; ++j)
            s_connectionTable[i][j] = (UInt32)(kTableRows[i][j] - '0');
        s_linkable[i] = kLinkable[i] == '1';
        s_pickable[i] = kPickable[i] == '1';
    }
}

// Mac 0x2e0f50, Win 0x4cc930
ConnectionFieldPlanar* ConnectionFieldPlanar::Create(const Matrix3D& transform, UInt32 kind,
                                                     UInt32 width, UInt32 height)
{
    ConnectionFieldPlanar* field;
    switch (kind) {
    case 3:  field = new KnobArrayConnectionField(transform); break;
    case 5:  field = new TechnicKnobArrayConnectionField(transform); break;
    case 10: field = new TubeRibArrayConnectionField(transform); break;
    case 12: field = new TubeArrayConnectionField(transform); break;
    case 14: field = new SecondaryPinArrayConnectionField(transform); break;
    case 16: field = new SecondaryPinConnectionField(transform); break;
    case 18: field = new TubeConnectionField(transform); break;
    case 20: field = new NoTubeArrayConnectionField(transform); break;
    case 22: field = new CustomPlanarConnectionField(transform, false); break;
    case 23: field = new CustomPlanarConnectionField(transform, true); break;
    default: return 0;
    }
    field->SetSize(width, height);
    return field;
}

// =============================================================================================
// ConnectionFieldPlanar
// =============================================================================================

// Mac: inlined in Create (0x2e5620 is the shared base-object constructor); Win 0x4cebe0
ConnectionFieldPlanar::ConnectionFieldPlanar(const Matrix3D& transform, UInt32 kind)
    : ConnectionField(transform, TYPE_PLANAR, kind), m_size(), m_points(), m_freeCount(0),
      m_widthVector(), m_heightVector(), m_diagonal(), m_center(), m_hasExtendedTypes(false)
{
}

// Mac 0x2e5750, Win 0x4cecf0
// NOTE: the m_connection pointers are copied as they are (they keep pointing at the points of
// the fields connected to the source).
ConnectionFieldPlanar::ConnectionFieldPlanar(const ConnectionFieldPlanar& o)
    : Object(), ConnectionField(o), m_size(o.m_size), m_points(), m_freeCount(o.m_freeCount),
      m_widthVector(o.m_widthVector), m_heightVector(o.m_heightVector),
      m_diagonal(o.m_diagonal), m_center(o.m_center), m_hasExtendedTypes(o.m_hasExtendedTypes)
{
    if (&o.m_points != &m_points)
        m_points = o.m_points;
    for (ConnectionPoint* p = m_points.Begin(); p != m_points.End(); ++p)
        p->m_field = this;
}

// Mac 0x2e5ca0 (D1), 0x2e5cf0 (D0), 0x2e5e00 / 0x2e5e60 (thunks); Win 0x4c9980
ConnectionFieldPlanar::~ConnectionFieldPlanar()
{
}

// Mac 0x2e5d60, Win 0x4c9a20 (inlined in every subclass SetSize)
void ConnectionFieldPlanar::SetSize(UInt32 width, UInt32 height)
{
    m_size.x = width;
    m_size.y = height;
    m_freeCount = 0;
    Real32 w = (Real32)width * GRID;
    Real32 h = (Real32)height * GRID;
    m_widthVector.x = w;
    m_heightVector.z = h;
    m_diagonal.x = w;
    m_diagonal.z = h;
    m_center.x = w * 0.5f;
    m_center.z = h * 0.5f;
}

void ConnectionFieldPlanar::ResizePoints(const ConnectionPoint& fill)
{
    m_points.Resize((m_size.y + 1) * (m_size.x + 1), fill);
}

void ConnectionFieldPlanar::CountPoint(UInt32 index)
{
    if (s_linkable[m_points[index].m_type])
        ++m_freeCount;
}

// Mac 0x2e5a40, Win 0x4c96d0
bool ConnectionFieldPlanar::IsCompatible(const ConnectionField& other) const
{
    return other.GetType() == m_type && ((other.GetKind() ^ m_kind) & 1) != 0;
}

// Mac 0x2e5a60, Win 0x4c9700
bool ConnectionFieldPlanar::IsCompatible(UInt32 index, const ConnectionField& other,
                                         UInt32 otherIndex) const
{
    if (other.GetType() != m_type)
        return false;
    const ConnectionPoint& p = m_points[index];
    const ConnectionPoint& q = static_cast<const ConnectionFieldPlanar&>(other).m_points[otherIndex];
    UInt32 t = s_connectionTable[p.m_type][q.m_type];
    return (t & 6) != 0 && (p.m_free != 0 || p.m_capacity == 0) &&
           (q.m_free != 0 || q.m_capacity == 0);
}

// Mac 0x2e43f0, Win 0x4cdf80
void ConnectionFieldPlanar::Connect(ConnectionField& other, const ConnectionIndices& indices,
                                    Attributes* attributes)
{
    ConnectionFieldPlanar& planar = static_cast<ConnectionFieldPlanar&>(other);
    for (const ConnectionIndex* i = indices.Begin(); i != indices.End(); ++i) {
        ConnectionPoint& p = m_points[i->m_index];
        ConnectionPoint& q = planar.m_points[i->m_otherIndex];
        UInt8 capacity = p.m_capacity;
        UInt8 otherCapacity = q.m_capacity;
        UInt8 n = otherCapacity <= capacity ? otherCapacity : capacity;
        bool linkable = s_linkable[p.m_type];
        if (linkable && s_linkable[q.m_type]) {
            p.m_connection = &q;
            q.m_connection = &p;
        }
        p.m_free -= n;
        q.m_free -= n;
        if (linkable && p.m_free == 0 && capacity != 0)
            --m_freeCount;
        if (q.m_free == 0 && s_linkable[q.m_type] && otherCapacity != 0)
            --planar.m_freeCount;
    }
    NotifyConnect(*this, other, indices, attributes);
}

// Mac 0x2e4510, Win 0x4ce060
void ConnectionFieldPlanar::Disconnect(ConnectionField& other, const ConnectionIndices& indices)
{
    ConnectionFieldPlanar& planar = static_cast<ConnectionFieldPlanar&>(other);
    for (const ConnectionIndex* i = indices.Begin(); i != indices.End(); ++i) {
        ConnectionPoint& p = m_points[i->m_index];
        ConnectionPoint& q = planar.m_points[i->m_otherIndex];
        if (s_linkable[p.m_type] && p.m_free == 0 && p.m_capacity != 0)
            ++m_freeCount;
        if (s_linkable[q.m_type] && q.m_free == 0 && q.m_capacity != 0)
            ++planar.m_freeCount;
        UInt8 n = q.m_capacity <= p.m_capacity ? q.m_capacity : p.m_capacity;
        q.m_connection = 0;
        p.m_connection = 0;
        p.m_free += n;
        q.m_free += n;
    }
    NotifyDisconnect(*this, other, indices);
}

// Mac 0x2e4610, Win 0x4ce120
bool ConnectionFieldPlanar::Pick(const Matrix3D& transform, const Vector3& origin,
                                 const Vector3& direction, Real32 maxDistance, Real32& distance,
                                 UInt32& index, Matrix3D& offset) const
{
    Vector3 x = Rotate(AxisX(), transform);
    Vector3 y = Rotate(AxisY(), transform);
    Vector3 z = Rotate(AxisZ(), transform);
    Vector3 hit;
    Plane plane(y, transform.GetTranslation());
    RayPlaneIntersect(origin, direction, plane, hit);
    distance = origin.GetDistanceSquared(hit);
    if (!(distance <= maxDistance))
        return false;
    Vector3 v = hit - transform.GetTranslation();
    Real32 px = x * v;
    Real32 pz = z * v;
    if (px < 0.0f || pz < 0.0f)
        return false;
    if (!(px < (Real32)m_size.x * GRID && pz < (Real32)m_size.y * GRID))
        return false;

    UInt32 column = (UInt32)RoundToNearest(px / GRID);
    UInt32 row = (UInt32)RoundToNearest(pz / GRID);
    index = 0;
    Real32 best = 0.64000005f;
    // NOTE: only points with column < width and row < height are candidates, and point 0 can
    // never be picked (0 means "nothing found"); both builds.
    for (Int32 dr = -1; dr != 2; ++dr) {
        UInt32 r = row + dr;
        for (UInt32 c = column - 1; c != column + 2; ++c) {
            if (c < m_size.x && r < m_size.y) {
                UInt32 i = (m_size.x + 1) * r + c;
                if (IsPickable(m_points[i])) {
                    Real32 d = DistanceSquared(Vector2(px, pz),
                                               Vector2((Real32)c * GRID, (Real32)r * GRID));
                    if (d < best) {
                        index = i;
                        best = d;
                    }
                }
            }
        }
    }
    if (index == 0)
        return false;
    UInt32 n = m_size.x + 1;
    offset.SetTranslationMatrix(Vector3((Real32)(index % n) * GRID, 0.0f,
                                        (Real32)(index / n) * GRID));
    return true;
}

// Mac 0x2e4d60, Win 0x4ce660
// The picked transform is the field transform with the point's field-space position as its
// translation (both builds).
void ConnectionFieldPlanar::Pick(const Matrix3D& transform, const Vector3& point,
                                 Real32 maxDistance, PickerAccumulator& accumulator) const
{
    Plane plane(Rotate(AxisY(), transform), transform.GetTranslation());
    Vector3 projected = ProjectOntoPlane(plane, point);
    if (!(point.GetDistanceSquared(projected) <= maxDistance))
        return;
    UInt32 rowStart = 0;
    for (UInt32 row = 0; row <= m_size.y; ++row, rowStart += m_size.x + 1) {
        Real32 z = (Real32)row * GRID;
        for (UInt32 column = 0; column <= m_size.x; ++column) {
            UInt32 index = column + rowStart;
            const ConnectionPoint& p = m_points[index];
            if (!IsPickable(p))
                continue;
            Vector3 local((Real32)column * GRID, 0.0f, z);
            Vector3 world = local * transform;
            Real32 d = point.GetDistanceSquared(world);
            if (d <= maxDistance) {
                PickerAccumulator::PickedFeature feature;
                feature.m_field = this;
                feature.m_index = index;
                feature.m_transform = transform;
                feature.m_transform.SetTranslation(local);
                feature.m_kind = p.m_type;
                feature.m_distance = d;
                accumulator.Add(m_type, feature);
            }
        }
    }
}

// Mac 0x2e5ae0, Win 0x4c9770
bool ConnectionFieldPlanar::IsOccupied() const
{
    return m_freeCount == 0;
}

// Mac 0x2e5af0, Win 0x4c9780
void ConnectionFieldPlanar::GetFrame(bool self, const ConnectionField& other,
                                     const ConnectionIndices& indices, Vector3& axis,
                                     Vector3& normal, Vector3& position) const
{
    const ConnectionFieldPlanar& field =
        self ? *this : static_cast<const ConnectionFieldPlanar&>(other);
    axis = Rotate(Vector3(0.0f, 1.0f, 0.0f), field.m_transform);
    normal = Rotate(Vector3(0.0f, 0.0f, 1.0f), field.m_transform);
    UInt32 index = self ? indices[0].m_index : indices[0].m_otherIndex;
    UInt32 n = field.m_size.x + 1;
    position = Vector3((Real32)(index % n) * GRID, 0.0f, (Real32)(index / n) * GRID) *
               field.m_transform;
}

// Mac 0x2e5c10, Win 0x4c98e0
UInt32 ConnectionFieldPlanar::GetConnectionType(const ConnectionField& other,
                                                const ConnectionIndices& indices) const
{
    if (indices.GetSize() != 1)
        return CONNECTION_RIGID;
    const ConnectionFieldPlanar& planar = static_cast<const ConnectionFieldPlanar&>(other);
    return s_connectionTable[m_points[indices[0].m_index].m_type]
                            [planar.m_points[indices[0].m_otherIndex].m_type];
}

// Mac 0x2e5c70, Win 0x4af080 (shared with ConnectionFieldAxle)
bool ConnectionFieldPlanar::IsRigidConnection(const ConnectionField& other,
                                              const ConnectionIndices& indices) const
{
    return GetConnectionType(other, indices) == CONNECTION_RIGID;
}

// Mac 0x2e52b0, Win 0x4ce990
// `counts` holds `count` records { point index, connections in direction 0, 1, 2 }.
UInt32 ConnectionFieldPlanar::GetConnectionState(bool any, UInt32 count, const UInt32* counts) const
{
    if (count == 0)
        return 0;
    if (any) {
        for (; count != 0; --count, counts += 4) {
            if (m_points[counts[0]].m_connection == 0)
                return 2;
        }
        return 0;
    }
    UInt32 result = 0;
    for (; count != 0; --count, counts += 4) {
        const ConnectionPoint& p = m_points[counts[0]];
        const ConnectionPoint* connection = p.m_connection;
        if (connection == 0)
            return 2;
        if ((p.m_flags & ConnectionPoint::FLAG_GROUP) != 0)
            return 2;
        if ((connection->m_flags & (ConnectionPoint::FLAG_GROUP | ConnectionPoint::FLAG_BLOCKED)) != 0)
            return 2;
        UInt32 direction = connection->m_flags & ConnectionPoint::FLAG_DIRECTION_MASK;
        if (counts[1 + direction] > 1)
            return 2;
        if (counts[1 + direction] == 1) {
            result = 1;
        } else if (result == 0 && direction == 1 &&
                   (p.m_flags & ConnectionPoint::FLAG_DIRECTION_MASK) == 1 && (m_kind & 1) == 0) {
            ConnectionFieldContext* otherContext = connection->m_field->m_context;
            result = 1;
            if (m_context->IsSameRigid(otherContext)) {
                Matrix3D transform;
                Matrix3D otherTransform;
                m_context->GetTransform(transform);
                otherContext->GetTransform(otherTransform);
                Vector3 x = Rotate(Vector3(1.0f, 0.0f, 0.0f), transform);
                Vector3 otherX = Rotate(Vector3(1.0f, 0.0f, 0.0f), otherTransform);
                Real32 d = Abs(x * otherX);
                result = 0.0175f < d && d < 0.99985f;
            }
        }
    }
    return result;
}

// Mac 0x2e54f0, Win 0x4ceb60
void ConnectionFieldPlanar::ApplyPointGroups(const BasicArray<PointGroup>& groups)
{
    for (ConnectionPoint* p = m_points.Begin(); p != m_points.End(); ++p) {
        const PointGroup& group = groups[p->m_group];
        if (group.m_enabled)
            p->m_flags = (p->m_flags & ~(UInt32)ConnectionPoint::FLAG_GROUP) | group.m_flags;
    }
}

// Mac 0x2e5580, Win 0x4ceba0
void ConnectionFieldPlanar::SetPointsBlocked(bool blocked)
{
    for (ConnectionPoint* p = m_points.Begin(); p != m_points.End(); ++p)
        p->m_flags = (p->m_flags & ~(UInt32)ConnectionPoint::FLAG_BLOCKED) |
                     (blocked ? (UInt32)ConnectionPoint::FLAG_BLOCKED : 0);
}

// ---------------------------------------------------------------------------------------------
// Placement helpers
// ---------------------------------------------------------------------------------------------

// Mac 0x2e1660, Win 0x4cfb40 (ESI = transform, EDI = otherTransform)
bool ConnectionFieldPlanar::GetRelativePlacement(const Matrix3D& transform,
                                                 const Matrix3D& otherTransform, Real32& dx,
                                                 Real32& dz, Real32& angle, UInt32& quarter,
                                                 bool& aligned)
{
    Vector3 otherY = Rotate(AxisY(), otherTransform);
    Vector3 otherX = Rotate(AxisX(), otherTransform);
    Vector3 x = Rotate(AxisX(), transform);
    Vector3 y = Rotate(AxisY(), transform);
    Vector3 z = Rotate(AxisZ(), transform);
    if (!otherY.IsEqual(y, 0.0176f))
        return false;
    Real32 height = y * transform.GetTranslation();
    Real32 otherHeight = y * otherTransform.GetTranslation();
    if (0.04f < Abs(height - otherHeight))
        return false;

    if (otherX.IsEqual(x, 1e-6f)) {
        angle = 0.0f;
        quarter = 0;
        aligned = true;
    } else if (otherX.IsEqual(z, 1e-6f)) {
        angle = HALF_PI;
        quarter = 1;
        aligned = true;
    } else if (otherX.IsEqual(-x, 1e-6f)) {
        angle = PI;
        quarter = 2;
        aligned = true;
    } else if (otherX.IsEqual(-z, 1e-6f)) {
        angle = 4.712389f;                      // 3 PI / 2
        quarter = 3;
        aligned = true;
    } else {
        aligned = false;
        Real32 c = otherX * x;
        Real32 s = otherX * z;
        Real32 a = ACos(c);
        angle = 0.0f < s ? a : 2.0f * PI - a;
    }
    Vector3 d(otherTransform.GetTranslation());
    d -= transform.GetTranslation();
    dx = x * d;
    dz = z * d;
    return true;
}

// Mac 0x2e19d0, Win 0x4d0070
bool ConnectionFieldPlanar::BoundsIntersect(const Matrix3D& transform,
                                            const ConnectionFieldPlanar& other,
                                            const Matrix3D& otherTransform) const
{
    Vector3 otherHalf = (Rotate(AxisX(), otherTransform) * (Real32)other.m_size.x +
                         Rotate(AxisZ(), otherTransform) * (Real32)other.m_size.y) * GRID * 0.5f;
    Sphere otherSphere(otherTransform.GetTranslation() + otherHalf, otherHalf.GetLength());
    Vector3 half = (Rotate(AxisX(), transform) * (Real32)m_size.x +
                    Rotate(AxisZ(), transform) * (Real32)m_size.y) * GRID * 0.5f;
    Sphere sphere(transform.GetTranslation() + half, half.GetLength());
    return otherSphere.Intersects(sphere);
}

// Mac 0x2e5900, Win 0x4cee50
bool ConnectionFieldPlanar::GetOverlap(const ConnectionFieldPlanar& other, UInt32 quarter,
                                       Int32 x, Int32 z, IntVector2& stepX, IntVector2& stepY,
                                       IntRect& rect) const
{
    Int32 w = (Int32)other.m_size.x;
    Int32 h = (Int32)other.m_size.y;
    IntRect covered(0, 0, 0, 0);
    switch (quarter) {
    case 0:
        covered = IntRect(x, z, x + w, z + h);
        stepX.x = 1;
        stepY.y = 1;
        break;
    case 1:
        covered = IntRect(x - h, z, x, z + w);
        stepX.y = -1;
        stepY.x = 1;
        break;
    case 2:
        covered = IntRect(x - w, z - h, x, z);
        stepX.x = -1;
        stepY.y = -1;
        break;
    case 3:
        covered = IntRect(x, z - w, x + h, z);
        stepX.y = 1;
        stepY.x = -1;
        break;
    }
    rect = IntRect(0, 0, (Int32)m_size.x, (Int32)m_size.y);
    rect.Intersect(covered);
    return rect.IsValid();
}

// Mac 0x2e1c60, Win 0x4cef90 (EAX = z)
UInt32 ConnectionFieldPlanar::TestOverlap(const ConnectionFieldPlanar& other, UInt32 quarter,
                                          Int32 x, Int32 z, ConnectionIndices* indices) const
{
    IntVector2 stepX(0, 0);
    IntVector2 stepY(0, 0);
    IntRect rect(0, 0, 0, 0);
    if (!GetOverlap(other, quarter, x, z, stepX, stepY, rect))
        return 0;

    UInt32 result = 0;
    IntVector2 start((rect.m_top - z) * stepY.x + (rect.m_left - x) * stepX.x,
                     (rect.m_top - z) * stepY.y + (rect.m_left - x) * stepX.y);
    for (Int32 row = rect.m_top; row <= rect.m_bottom; ++row) {
        IntVector2 p = start;
        for (Int32 column = rect.m_left; column <= rect.m_right; ++column) {
            UInt32 otherIndex = (other.m_size.x + 1) * p.y + p.x;
            UInt32 index = (m_size.x + 1) * row + column;
            const ConnectionPoint& a = m_points[index];
            const ConnectionPoint& b = other.m_points[otherIndex];
            UInt32 t = s_connectionTable[a.m_type][b.m_type];
            if ((t & 6) != 0 && ((a.m_free == 0 && a.m_capacity != 0) ||
                                 (b.m_free == 0 && b.m_capacity != 0)))
                t = CONNECTION_BLOCKED;
            if (t == CONNECTION_BLOCKED)
                return CONNECTION_BLOCKED;
            if (t == CONNECTION_RIGID) {
                result = CONNECTION_RIGID;
                if (indices)
                    indices->PushBack(ConnectionIndex(index, otherIndex));
            } else if (t == CONNECTION_FREE) {
                // two free (rotating) pairs hold the parts rigidly
                if (result == 0)
                    result = CONNECTION_FREE;
                else if (result == CONNECTION_FREE)
                    result = CONNECTION_RIGID;
                if (indices)
                    indices->PushBack(ConnectionIndex(index, otherIndex));
            }
            p += stepX;
        }
        start += stepY;
    }
    return result;
}

// Mac 0x2e2350, Win 0x4cf1a0
UInt32 ConnectionFieldPlanar::TestFreeAngle(const ConnectionFieldPlanar& other, Real32 angle,
                                            Real32 dx, Real32 dz,
                                            ConnectionIndices* indices) const
{
    Real32 c = Cos(angle);
    Real32 s = Sin(angle);
    Vector2 stepX(c * GRID, s * GRID);
    Vector2 stepY(-stepX.y, stepX.x);
    UInt32 result = 0;
    for (UInt32 row = 0; row <= other.m_size.y; ++row) {
        Vector2 p = Vector2(dx, dz) + stepY * (Real32)row;
        for (UInt32 column = 0; column <= other.m_size.x; ++column, p += stepX) {
            Real32 fx = Abs(fmodf(p.x, GRID));
            if ((double)fx > 0.1 && 0.3f > fx)
                continue;
            Real32 fz = Abs(fmodf(p.y, GRID));
            if ((double)fz > 0.1 && 0.3f > fz)
                continue;
            Int32 ix = RoundToNearest(p.x / GRID);
            Int32 iz = RoundToNearest(p.y / GRID);
            if (ix < 0 || (UInt32)ix > m_size.x || iz < 0 || (UInt32)iz > m_size.y)
                continue;
            UInt32 otherIndex = (other.m_size.x + 1) * row + column;
            UInt32 index = (m_size.x + 1) * (UInt32)iz + (UInt32)ix;
            const ConnectionPoint& a = m_points[index];
            const ConnectionPoint& b = other.m_points[otherIndex];
            UInt32 t = s_connectionTable[a.m_type][b.m_type];
            if ((t & 6) != 0 && ((a.m_free == 0 && a.m_capacity != 0) ||
                                 (b.m_free == 0 && b.m_capacity != 0)))
                t = CONNECTION_BLOCKED;
            if (t == CONNECTION_BLOCKED)
                return CONNECTION_BLOCKED;
            if (t == CONNECTION_RIGID) {
                result = CONNECTION_RIGID;
                if (indices)
                    indices->PushBack(ConnectionIndex(index, otherIndex));
            } else if (t == CONNECTION_FREE) {
                if (result == 0)
                    result = CONNECTION_FREE;
                else if (result == CONNECTION_FREE)
                    result = CONNECTION_RIGID;
                if (indices)
                    indices->PushBack(ConnectionIndex(index, otherIndex));
            }
        }
    }
    return result;
}

// Mac 0x2e2e50, Win 0x4cf510
// NOTE: when no grid position fits, `distance` is not written, and the result (0 or 1) is
// still cleared when the caller's previous `distance` is >= maxDistance (both builds).
UInt32 ConnectionFieldPlanar::Snap(const Vector2& position, const Vector2& offset,
                                   const Matrix3D& transform, const ConnectionFieldPlanar& other,
                                   const Matrix3D& otherTransform, const Vector3& point,
                                   Real32 maxDistance, Real32& distance, Matrix3D& snapped) const
{
    Vector3 otherX = Rotate(AxisX(), otherTransform);
    Real32 c = otherX * Rotate(AxisX(), transform);
    Real32 s = otherX * Rotate(AxisZ(), transform);
    Real32 a = ACos(c);
    Real32 angle = 0.0f < s ? a : 2.0f * PI - a;

    // Origin of the other grid, turned to the nearest quarter. (Win: PI / 180 * 45 etc.)
    UInt32 quarter;
    Real32 rotation;
    Vector2 p;
    if (0.78539819f > angle || angle > 5.4977870f) {
        quarter = 0;
        rotation = 0.0f;
        p = Vector2(position.x - offset.x, position.y - offset.y);
    } else if (2.3561945f > angle) {
        quarter = 1;
        rotation = HALF_PI;
        p = Vector2(position.x + offset.y, position.y - offset.x);
    } else if (3.9269907f > angle) {
        quarter = 2;
        rotation = PI;
        p = Vector2(position.x + offset.x, position.y + offset.y);
    } else {
        quarter = 3;
        rotation = 4.712389f;
        p = Vector2(position.x - offset.y, position.y + offset.x);
    }

    // Grid positions tried: 0.4 then 0.8; with extended point types on both sides 0.8 then 1.6.
    Int32 x;
    Int32 z;
    UInt32 result;
    if (!m_hasExtendedTypes || !other.m_hasExtendedTypes) {
        x = RoundToNearest(p.x / GRID);
        z = RoundToNearest(p.y / GRID);
        result = TestOverlap(other, quarter, x, z, 0);
        if ((result & 6) == 0) {
            x = RoundToNearest(p.x / 0.8f) * 2;
            z = RoundToNearest(p.y / 0.8f) * 2;
            result = TestOverlap(other, quarter, x, z, 0);
        }
    } else {
        x = RoundToNearest(p.x / 0.8f) * 2;
        z = RoundToNearest(p.y / 0.8f) * 2;
        result = TestOverlap(other, quarter, x, z, 0);
        if ((result & 6) == 0) {
            x = RoundToNearest(p.x / 1.6f) * 4;
            z = RoundToNearest(p.y / 1.6f) * 4;
            result = TestOverlap(other, quarter, x, z, 0);
        }
    }

    Real32 d;
    if ((result & 6) == 0) {
        d = distance;
    } else {
        Matrix3D t;
        t.SetRotationY(-rotation);
        t.SetTranslation(Vector3((Real32)x * GRID, 0.0f, (Real32)z * GRID));
        snapped.Multiply(transform, t);
        d = point.GetDistanceSquared(snapped.GetTranslation());
        distance = d;
    }
    if (maxDistance <= d)
        result = 0;
    return result;
}

// ---------------------------------------------------------------------------------------------
// CanConnect
// ---------------------------------------------------------------------------------------------

// Mac 0x2e13e0, Win 0x4ccbc0
UInt32 ConnectionFieldPlanar::CanConnect(const Matrix3D& transform, const ConnectionField& other,
                                         const Matrix3D& otherTransform,
                                         ConnectionIndices& indices) const
{
    if (other.GetType() != m_type)
        return 0;
    const ConnectionFieldPlanar& planar = static_cast<const ConnectionFieldPlanar&>(other);
    Real32 dx, dz, angle;
    UInt32 quarter;
    bool aligned;
    if (!GetRelativePlacement(transform, otherTransform, dx, dz, angle, quarter, aligned))
        return 0;
    if (!BoundsIntersect(transform, planar, otherTransform))
        return 0;
    if (!aligned)
        return TestFreeAngle(planar, angle, dx, dz, &indices);
    // The other origin must lie within 0.08 of a grid point in both directions.
    Real32 fx = fmodf(Abs(dx), GRID);
    if (0.08f < fx && 0.32f > fx)
        return 0;
    Real32 fz = fmodf(Abs(dz), GRID);
    if (0.08f < fz && 0.32f > fz)
        return 0;
    return TestOverlap(planar, quarter, RoundToNearest(dx / GRID), RoundToNearest(dz / GRID),
                       &indices);
}

// Mac 0x2e2c00, Win 0x4ccd70
UInt32 ConnectionFieldPlanar::CanConnect(const Matrix3D& transform, const ConnectionField& other,
                                         const Matrix3D& otherTransform, Real32 maxDistance,
                                         Real32& distance, Matrix3D& snapped) const
{
    if (IsOccupied() || other.IsOccupied())
        return 0;
    const ConnectionFieldPlanar& planar = static_cast<const ConnectionFieldPlanar&>(other);
    Vector3 y = Rotate(AxisY(), transform);
    Vector3 otherY = Rotate(AxisY(), otherTransform);
    if (!(0.70710677f <= y * otherY))
        return 0;
    Plane plane(y, transform.GetTranslation());
    Vector3 projected = ProjectOntoPlane(plane, otherTransform.GetTranslation());
    if (!(otherTransform.GetTranslation().GetDistanceSquared(projected) < maxDistance))
        return 0;
    Vector3 v = projected - transform.GetTranslation();
    Vector2 position(Rotate(AxisX(), transform) * v, Rotate(AxisZ(), transform) * v);
    return Snap(position, Vector2(), transform, planar, otherTransform,
                otherTransform.GetTranslation(), maxDistance, distance, snapped);
}

// Mac 0x2e35f0, Win 0x4ccfe0
// Parallel grids with axes at right angles: the other origin is projected from `origin` onto
// this plane. Otherwise both grid outlines are projected onto the view plane through this
// origin; the centre of their intersection polygon (corners inside the other outline and edge
// crossings) is cast back onto both planes to get the two positions.
UInt32 ConnectionFieldPlanar::CanConnect(const Matrix3D& transform, const ConnectionField& other,
                                         const Matrix3D& otherTransform, const Vector3& origin,
                                         const Vector3& direction, Real32 maxDistance,
                                         Real32& distance, Matrix3D& snapped) const
{
    if (IsOccupied() || other.IsOccupied())
        return 0;
    const ConnectionFieldPlanar& planar = static_cast<const ConnectionFieldPlanar&>(other);
    Vector3 y = Rotate(AxisY(), transform);
    Vector3 otherY = Rotate(AxisY(), otherTransform);
    Real32 c = y * otherY;
    if (c < 0.70710677f)
        return 0;
    Vector3 x = Rotate(AxisX(), transform);
    Vector3 otherX = Rotate(AxisX(), otherTransform);
    Real32 xx = x * otherX;

    if (IsEqual(c, 1.0f, EPSILON) &&
        (IsEqual(xx, 0.0f, EPSILON) || IsEqual(Abs(xx), 1.0f, EPSILON))) {
        Plane plane(y, transform.GetTranslation());
        Vector3 hit;
        RayPlaneIntersectPoints(origin, otherTransform.GetTranslation(), plane, hit);
        if (!(origin.GetDistanceSquared(hit) < maxDistance))
            return 0;
        Vector3 v = hit - transform.GetTranslation();
        Vector2 position(v * x, v * Rotate(AxisZ(), transform));
        return Snap(position, Vector2(), transform, planar, otherTransform, origin, maxDistance,
                    distance, snapped);
    }

    // Outlines projected from `origin` onto the plane through this origin facing `direction`.
    Vector3 corners[4];
    Vector3 otherCorners[4];
    Vector3 width = Rotate(m_widthVector, transform);
    Vector3 height = Rotate(m_heightVector, transform);
    Vector3 otherWidth = Rotate(planar.m_widthVector, otherTransform);
    Vector3 otherHeight = Rotate(planar.m_heightVector, otherTransform);
    const Vector3& t = transform.GetTranslation();
    const Vector3& ot = otherTransform.GetTranslation();
    Plane view(direction, t);
    corners[0] = t;
    LinePlaneIntersectPoints(origin, t + width, view, corners[1]);
    LinePlaneIntersectPoints(origin, t + width + height, view, corners[2]);
    LinePlaneIntersectPoints(origin, t + height, view, corners[3]);
    LinePlaneIntersectPoints(origin, ot, view, otherCorners[0]);
    LinePlaneIntersectPoints(origin, ot + otherWidth, view, otherCorners[1]);
    LinePlaneIntersectPoints(origin, ot + otherWidth + otherHeight, view, otherCorners[2]);
    LinePlaneIntersectPoints(origin, ot + otherHeight, view, otherCorners[3]);
    // Same winding for both outlines (Win 0x4d0660 swaps two Vector3).
    if (0.0f < direction * y) {
        Vector3 swap = corners[1];
        corners[1] = corners[3];
        corners[3] = swap;
    }
    if (0.0f < direction * otherY) {
        Vector3 swap = otherCorners[1];
        otherCorners[1] = otherCorners[3];
        otherCorners[3] = swap;
    }

    // Bit i of inside[j]: corner j lies on the inner side of edge i of the other outline (side
    // planes through `origin`); a separating edge means no overlap.
    UInt32 inside[4] = { 0, 0, 0, 0 };
    UInt32 otherInside[4] = { 0, 0, 0, 0 };
    for (UInt32 i = 0; i < 4; ++i) {
        UInt32 next = (i + 1) & 3;
        Plane side(origin, corners[i], corners[next]);
        Plane otherSide(origin, otherCorners[i], otherCorners[next]);
        UInt32 bit = 1u << i;
        UInt32 otherCount = 0;
        UInt32 count = 0;
        for (UInt32 j = 0; j < 4; ++j) {
            if (side.GetDistance(otherCorners[j]) >= 0.0f) {
                otherInside[j] |= bit;
                ++otherCount;
            }
            if (otherSide.GetDistance(corners[j]) >= 0.0f) {
                inside[j] |= bit;
                ++count;
            }
        }
        if (otherCount == 0 || count == 0)
            return 0;
    }

    Vector3 sum;
    UInt32 n = 0;
    for (UInt32 j = 0; j < 4; ++j) {
        if (inside[j] == 0xf) {
            ++n;
            sum += corners[j];
        }
        if (otherInside[j] == 0xf) {
            ++n;
            sum += otherCorners[j];
        }
        UInt32 crossing = inside[(j + 1) & 3] ^ inside[j];
        for (UInt32 k = 0; k < 4; ++k) {
            if ((crossing & (1u << k)) != 0 &&
                ((otherInside[(k + 1) & 3] ^ otherInside[k]) & (1u << j)) != 0) {
                Real32 s = 0.0f;
                Real32 u = 0.0f;
                Vector3 directionA;
                Vector3 directionB;
                Vector3 w;
                LineLineParameters(corners[j], corners[(j + 1) & 3], otherCorners[k],
                                   otherCorners[(k + 1) & 3], s, u, directionA, directionB, w);
                ++n;
                sum += corners[j] + directionA * s;
            }
        }
    }
    Vector3 center = sum / (Real32)n;

    Plane plane(y.x, y.y, y.z, -(y * t));
    Plane otherPlane(otherY.x, otherY.y, otherY.z, -(otherY * ot));
    Vector3 ray = center - origin;
    Vector3 hit;
    Vector3 otherHit;
    LinePlaneIntersect(origin, ray, plane, hit);
    LinePlaneIntersect(origin, ray, otherPlane, otherHit);
    Vector3 v = hit - t;
    Vector3 ov = otherHit - ot;
    Vector2 position(v * x, v * Rotate(AxisZ(), transform));
    Vector2 offset(ov * otherX, ov * Rotate(AxisZ(), otherTransform));
    return Snap(position, offset, transform, planar, otherTransform, origin, maxDistance,
                distance, snapped);
}

// =============================================================================================
// Presets
// =============================================================================================

namespace {
ConnectionPoint MakePoint(ConnectionFieldPlanar* field, UInt32 flags, UInt8 type, UInt8 capacity)
{
    ConnectionPoint p;
    p.m_connection = 0;
    p.m_field = field;
    p.m_flags = flags;
    p.m_type = type;
    p.m_free = capacity;
    p.m_capacity = capacity;
    p.m_group = 0;
    return p;
}
} // namespace

#define PLANAR_PRESET_BOILERPLATE(Class, KIND)                                                  \
    Class::Class(const Matrix3D& transform) : ConnectionFieldPlanar(transform, KIND) {}        \
    Class::Class(const Class& o) : Object(), ConnectionFieldPlanar(o) {}                       \
    Class::~Class() {}                                                                         \
    void Class::Clone(ConnectionField*& copy) const { copy = new Class(*this); }

PLANAR_PRESET_BOILERPLATE(KnobArrayConnectionField, 3)
PLANAR_PRESET_BOILERPLATE(TechnicKnobArrayConnectionField, 5)
PLANAR_PRESET_BOILERPLATE(TubeRibArrayConnectionField, 10)
PLANAR_PRESET_BOILERPLATE(TubeArrayConnectionField, 12)
PLANAR_PRESET_BOILERPLATE(SecondaryPinArrayConnectionField, 14)
PLANAR_PRESET_BOILERPLATE(SecondaryPinConnectionField, 16)
PLANAR_PRESET_BOILERPLATE(TubeConnectionField, 18)
PLANAR_PRESET_BOILERPLATE(NoTubeArrayConnectionField, 20)

#undef PLANAR_PRESET_BOILERPLATE

// Win 0x4cc200
CustomPlanarConnectionField::CustomPlanarConnectionField(const Matrix3D& transform, bool female)
    : ConnectionFieldPlanar(transform, KIND_CUSTOM + (female ? 1 : 0))
{
}

// Win 0x4cc300
CustomPlanarConnectionField::CustomPlanarConnectionField(const CustomPlanarConnectionField& o)
    : Object(), ConnectionFieldPlanar(o)
{
}

CustomPlanarConnectionField::~CustomPlanarConnectionField()
{
}

// Mac 0x2e9000, Win 0x4cc0e0
void CustomPlanarConnectionField::Clone(ConnectionField*& copy) const
{
    copy = new CustomPlanarConnectionField(*this);
}

// Shared shape of the knob layouts: corners 0x12 (capacity 1), side points 0x12 / 0x17, inner
// even points 0x12 / 0x17, stud centres (odd, odd) `center` (fixed).
static void SetKnobLayout(ConnectionFieldPlanar& field, UInt32 column, UInt32 row, UInt32 index,
                          UInt8 center, ConnectionPoint* points)
{
    UInt32 w = field.GetWidth();
    UInt32 h = field.GetHeight();
    bool columnEdge = column == 0 || column == w;
    bool rowEdge = row == 0 || row == h;
    ConnectionPoint& p = points[index];
    if (columnEdge) {
        if (rowEdge)
            p = MakePoint(&field, 0, 0x12, 1);
        else
            p = (row & 1) == 0 ? MakePoint(&field, 0, 0x12, 2) : MakePoint(&field, 0, 0x17, 4);
    } else if ((column & 1) == 0) {
        if (rowEdge)
            p = MakePoint(&field, 0, 0x12, 2);
        else
            p = (row & 1) == 0 ? MakePoint(&field, 0, 0x12, 4) : MakePoint(&field, 0, 0x17, 4);
    } else {
        if (rowEdge || (row & 1) == 0)
            p = MakePoint(&field, 0, 0x17, 4);
        else
            p = MakePoint(&field, ConnectionPoint::FLAG_FIXED, center, 4);
    }
}

// Mac 0x2e6050, Win 0x4c9b80
void KnobArrayConnectionField::SetSize(UInt32 width, UInt32 height)
{
    ConnectionFieldPlanar::SetSize(width, height);
    ResizePoints(MakePoint(this, 0, 0x0f, 4));
    for (UInt32 column = 0; column <= m_size.x; ++column) {
        for (UInt32 row = 0; row <= m_size.y; ++row) {
            UInt32 index = row * (m_size.x + 1) + column;
            SetKnobLayout(*this, column, row, index, 0x00, m_points.Begin());
            CountPoint(index);
        }
    }
}

// Mac 0x2e7030, Win 0x4ca0c0
void TechnicKnobArrayConnectionField::SetSize(UInt32 width, UInt32 height)
{
    ConnectionFieldPlanar::SetSize(width, height);
    ResizePoints(MakePoint(this, 0, 0x0f, 4));
    for (UInt32 column = 0; column <= m_size.x; ++column) {
        for (UInt32 row = 0; row <= m_size.y; ++row) {
            UInt32 index = row * (m_size.x + 1) + column;
            SetKnobLayout(*this, column, row, index, 0x01, m_points.Begin());
            CountPoint(index);
        }
    }
}

// Mac 0x2e75a0, Win 0x4ca5b0
// Outline 0x16; tubes (0x07) on the 4-grid and between, ribs 0x17, centres 0x0f.
void TubeRibArrayConnectionField::SetSize(UInt32 width, UInt32 height)
{
    ConnectionFieldPlanar::SetSize(width, height);
    ResizePoints(MakePoint(this, ConnectionPoint::FLAG_FIXED, 0x00, 4));
    UInt32 w = m_size.x;
    UInt32 h = m_size.y;
    for (UInt32 column = 0; column <= w; ++column) {
        for (UInt32 row = 0; row <= h; ++row) {
            UInt32 index = row * (w + 1) + column;
            bool rowEdge = row == 0 || row == h;
            UInt8 type;
            if (column == 0 || column == w || rowEdge)
                type = 0x16;
            else if ((column & 3) == 0)
                type = (row & 3) == 0 ? 0x07 : 0x16;
            else if ((column & 1) == 0)
                type = (row & 1) != 0 ? 0x17 : ((row & 3) == 0 ? 0x16 : 0x07);
            else
                type = (row & 1) != 0 ? 0x0f : ((row & 3) == 0 ? 0x16 : 0x17);
            m_points[index] = MakePoint(this, 0, type, 4);
            CountPoint(index);
        }
    }
}

// Mac 0x2e7b80, Win 0x4cabd0
// Outline 0x16; tubes 0x07 at (even, even), 0x17 between, 0x0f at (odd, odd).
void TubeArrayConnectionField::SetSize(UInt32 width, UInt32 height)
{
    ConnectionFieldPlanar::SetSize(width, height);
    ResizePoints(MakePoint(this, ConnectionPoint::FLAG_FIXED, 0x00, 4));
    UInt32 w = m_size.x;
    UInt32 h = m_size.y;
    for (UInt32 column = 0; column <= w; ++column) {
        for (UInt32 row = 0; row <= h; ++row) {
            UInt32 index = row * (w + 1) + column;
            UInt8 type;
            if (column == 0 || column == w || row == 0 || row == h)
                type = 0x16;
            else if ((column & 1) == 0)
                type = (row & 1) == 0 ? 0x07 : 0x17;
            else
                type = (row & 1) == 0 ? 0x17 : 0x0f;
            m_points[index] = MakePoint(this, 0, type, 4);
            CountPoint(index);
        }
    }
}

// Mac 0x2e8070, Win 0x4cb0d0
// Outline 0x16; columns 1 and width-1: pins 0x09 at even rows, 0x0f between; inner points
// alternate 0x0f / 0x09 in a checkerboard.
void SecondaryPinArrayConnectionField::SetSize(UInt32 width, UInt32 height)
{
    ConnectionFieldPlanar::SetSize(width, height);
    ResizePoints(MakePoint(this, ConnectionPoint::FLAG_FIXED, 0x00, 4));
    UInt32 w = m_size.x;
    UInt32 h = m_size.y;
    for (UInt32 column = 0; column <= w; ++column) {
        for (UInt32 row = 0; row <= h; ++row) {
            UInt32 index = row * (w + 1) + column;
            bool rowEdge = row == 0 || row == h;
            UInt8 type;
            if (column == 0 || column == w || rowEdge)
                type = 0x16;
            else if (column == 1 || column == w - 1)
                type = (row & 1) == 0 ? 0x09 : 0x0f;
            else
                type = ((row + column) & 1) == 0 ? 0x0f : 0x09;
            m_points[index] = MakePoint(this, 0, type, 4);
            CountPoint(index);
        }
    }
}

// Mac 0x2e8540, Win 0x4cb620
// NOTE: assumes at least 5 points (the 2 x 2 grid of the primitives); only point 4 is counted.
void SecondaryPinConnectionField::SetSize(UInt32 width, UInt32 height)
{
    ConnectionFieldPlanar::SetSize(width, height);
    ResizePoints(MakePoint(this, 0, 0x1d, 0));
    m_points[4] = MakePoint(this, 0, 0x09, 4);
    CountPoint(4);
}

// Mac 0x2e88d0, Win 0x4cb940 (as SecondaryPin with type 0x05)
void TubeConnectionField::SetSize(UInt32 width, UInt32 height)
{
    ConnectionFieldPlanar::SetSize(width, height);
    ResizePoints(MakePoint(this, 0, 0x1d, 0));
    m_points[4] = MakePoint(this, 0, 0x05, 4);
    CountPoint(4);
}

// Mac 0x2e8c60, Win 0x4cbc60
// Outline 0x16; columns 1 and width-1: 0x17 at even rows, 0x0f between; inner points 0x0f
// where row + column is even.
// NOTE: the other inner points are not written (both builds): they keep the type 0x1d of a new
// point, or their previous contents when the grid was already that large.
void NoTubeArrayConnectionField::SetSize(UInt32 width, UInt32 height)
{
    ConnectionFieldPlanar::SetSize(width, height);
    ResizePoints(MakePoint(this, 0, 0x1d, 0));
    UInt32 w = m_size.x;
    UInt32 h = m_size.y;
    for (UInt32 column = 0; column <= w; ++column) {
        for (UInt32 row = 0; row <= h; ++row) {
            UInt32 index = row * (w + 1) + column;
            bool rowEdge = row == 0 || row == h;
            if (column == 0 || column == w || rowEdge)
                m_points[index] = MakePoint(this, 0, 0x16, 4);
            else if (column == 1 || column == w - 1)
                m_points[index] = MakePoint(this, 0, (row & 1) == 0 ? 0x17 : 0x0f, 4);
            else if (((row + column) & 1) == 0)
                m_points[index] = MakePoint(this, 0, 0x0f, 4);
            CountPoint(index);
        }
    }
}

// Mac 0x2e9170, Win 0x4cc170
void CustomPlanarConnectionField::SetSize(UInt32 width, UInt32 height)
{
    ConnectionFieldPlanar::SetSize(width, height);
    ResizePoints(MakePoint(this, 0, 0x1d, 0));
}

} // namespace LEGO
