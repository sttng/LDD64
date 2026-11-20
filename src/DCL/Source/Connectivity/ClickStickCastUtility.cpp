// LEGO Digital Designer — restored source
// LEGO::ClickStickCastUtility / ClickStickCastSimpleUtility — see ClickStickCastUtility.h.
// TU 376 "ClickStickCastUtility.cpp": Mac 0x363640-0x366b3a, Win 0x5aa470-0x5ac22f.
// Not code: Mac 0x366920 = <iostream> + the five ProfileTimers below.
#include "ClickStickCastUtility.h"
#include "ProfileTimer.h"
#include "MathUtil.h"
#include "SimplePhysicsSceneDebugInfo.h"
#include "Part.h"

#include <string.h>

namespace LEGO {

// The profiling timers of the TU initialiser (Mac 0x366920; reported at exit). NOTE: the "2D"
// timer measures the ray snap of the 3D cast and the "3D" one its SnapDrag; the 2D cast has none.
static ProfileTimer s_timer2D(String("ClickStickCastUtility::CanConnect(2D)"), true);                  // Mac 0x77f074
static ProfileTimer s_timer3D(String("ClickStickCastUtility::CanConnect(3D)"), true);                  // Mac 0x77f0a4
static ProfileTimer s_timer3DEnvironment(String("ClickStickCastUtility::CanConnect(3D env)"), true);   // Mac 0x77f0d4
static ProfileTimer s_timer3DPhysics(String("ClickStickCastUtility::CanConnect(3D phy)"), true);       // Mac 0x77f104
static ProfileTimer s_timer3DPhysicsDown(String("ClickStickCastUtility::CanConnect(3D phy down)"), true); // Mac 0x77f134

namespace {

// Record magic numbers (little endian "20D2" / "10D3") and the file header.
const UInt32 kMagic3D = 0x32443032;
const UInt32 kMagic2D = 0x33443031;
const char   kRecordingHeader[8] = { 'C', 'S', 'C', 'U', '0', '0', '2', 0 };   // Mac 0x778338, Win 0xae0dc8

// The scene listener code of a cast ('csu ').
const UInt32 kListenerCode = 0x63737520;

// Rounds to a multiple of `step`, ties away from zero, the quotient truncated: the snapping of
// the grid and of the brick heights (Mac: inline, single precision, cvttss2si; Win 0x4aecb0).
// NOTE: platform difference: Win divides on the x87 (double precision, _ftol), the Mac in single
// precision (divss): a quotient just below an integer can truncate differently.
inline Real32 RoundToStep(Real32 value, Real32 step, Real32 half)
{
#if defined(_WIN32)
    Real32 q = (Real32)(Int32)((Real64)value / (Real64)step) * step;
#else
    Real32 q = (Real32)(Int32)(value / step) * step;
#endif
    Real32 r = value - q;
    if (r >= 0.0f) {
        if (r >= half)
            q += step;
    } else if (-half >= r) {
        q += -step;
    }
    return q;
}

// The brick height (0.32) the physics fallback snaps y to (centred: +0.16 first).
inline Real32 SnapHeight(Real32 y)
{
    return RoundToStep(y + 0.16f, 0.32f, 0.16f);
}

} // namespace

// =============================================================================================
// ClickStickCastUtility
// =============================================================================================

// Mac 0x363640, Win 0x5aa630
ClickStickCastUtility::ClickStickCastUtility()
    : m_distance(0.0f), m_connected(false), m_hasPhysics(false), m_aligned(false),
      m_fineGrid(false), m_recording(false), m_playing(false), m_frame(0)
{
}

// Mac 0x366880 (D1) / 0x366890 (D0), Win sdd 0x5aa530 (body 0x5aa470)
ClickStickCastUtility::~ClickStickCastUtility()
{
}

// Mac 0x363a60, Win 0x5abb10
void ClickStickCastUtility::ClearResult()
{
    m_result.m_transform.SetIdentity();
    m_result.m_connections.Clear();
    m_transform.SetIdentity();
    m_connected = false;
}

// Mac 0x363900, Win 0x5abb60
void ClickStickCastUtility::Reset()
{
    ClearResult();
    m_distance = 0.0f;
    m_offset.SetIdentity();
}

// Mac 0x363790, Win 0x5aa700
bool ClickStickCastUtility::Begin(Scene* scene, Scene* dragScene, PrimitiveElement* first, bool single)
{
    Reset();
    if (dragScene->IsEmpty())
        return false;
    m_scene = scene;
    m_dragScene = dragScene;
    // the physics scene can sweep the drag scene: it has its engine object or a terrain
    PhysicsScene* physics = m_scene->GetPhysicsScene();
    m_hasPhysics = physics->Unknown32() != 0 || physics->GetSceneryElement(0, 2) != 0;
    m_scene->BeginDrag(m_dragScene, first, single);
    return true;
}

// Mac 0x363990, Win 0x5aa7b0
void ClickStickCastUtility::End(bool clear)
{
    if (m_scene == 0)
        return;
    m_scene->EndDrag();
    m_scene = 0;
    m_dragScene = 0;
    if (clear)
        ClearResult();
}

// Mac 0x3651f0, Win 0x5ab7e0
bool ClickStickCastUtility::Cast(const Matrix3D& transform)
{
    VisualDebugger* debugger = m_scene->GetListener();
    if (debugger)
        debugger->OnSceneEvent(kListenerCode);
    return DoCast(transform);
}

// Mac 0x363ae0, Win 0x5aa820
bool ClickStickCastUtility::Cast(const Vector3& origin, const Vector3& direction, Real32 maxDistance)
{
    VisualDebugger* debugger = m_scene->GetListener();
    if (debugger)
        debugger->OnSceneEvent(kListenerCode);
    return DoCast(origin, direction, maxDistance, debugger);
}

// Mac 0x364eb0, Win 0x5abcb0
void ClickStickCastUtility::StopRecording(bool truncate)
{
    if (m_file != 0) {
        if (m_recording && truncate)
            m_file->SetSize(m_file->GetPosition());
        m_recording = false;
        m_playing = false;
        m_file = 0;
    } else {
        m_recording = false;
        m_playing = false;
    }
}

// Mac 0x3657e0, Win 0x5abb90
bool ClickStickCastUtility::StartRecording(File* file)
{
    if (!file->Write(sizeof kRecordingHeader, kRecordingHeader, 0))
        return false;
    m_recording = true;
    m_frame = 0;
    m_file = file;
    return true;
}

// Mac 0x365860, Win 0x5abc00
bool ClickStickCastUtility::StartPlayback(File* file)
{
    char header[sizeof kRecordingHeader];
    if (!file->Read(sizeof header, header, 0))
        return false;
    if (memcmp(kRecordingHeader, header, sizeof header) != 0)
        return false;
    m_playing = true;
    m_frame = 0;
    m_file = file;
    return true;
}

// (inline in the original, Mac 0x36475d / 0x364372 / 0x364f30)
void ClickStickCastUtility::AbortPlayback()
{
    Reset();
    StopRecording(true);
    OnPlaybackFinished(false);
}

// Mac 0x364f30, Win 0x5ac230
bool ClickStickCastUtility::RecordFrame(UInt32 flags)
{
    UInt32 value = flags;
    if (m_recording) {
        m_file->Write(sizeof(Matrix3D), &m_transform, 0);
        m_file->Write(sizeof value, &value, 0);
    } else if (m_playing) {
        Matrix3D recorded;
        UInt32 recordedFlags = 0;   // (uninitialised in the original; only read after a good read)
        const bool readTransform = m_file->Read(sizeof recorded, &recorded, 0);
        const bool readFlags = m_file->Read(sizeof recordedFlags, &recordedFlags, 0);
        if (!readTransform || !readFlags) {
            AbortPlayback();
            return false;
        }
        if (value != recordedFlags || !recorded.IsEqual(m_transform, 1e-5f))
            OnPlaybackMismatch(m_frame, recordedFlags, value, recorded, m_transform);
        if (m_file->GetPosition() == m_file->GetSize()) {
            StopRecording(true);
            OnPlaybackFinished(true);
        }
    }
    ++m_frame;
    return true;
}

// Mac 0x365230, Win 0x5ab810
bool ClickStickCastUtility::DoCast(const Matrix3D& transform)
{
    ClearResult();
    if (IsSimple() && m_file == 0) {
        // NOTE: at the transform just reset (the identity), as the 3D cast.
        m_connected = m_scene->CanConnectDrag(m_transform, m_result);
        return true;
    }
    Matrix3D target(transform);
    if (m_recording) {
        UInt32 magic = kMagic2D;
        m_file->Write(sizeof magic, &magic, 0);
        m_file->Write(sizeof target, &target, 0);
        WriteFrameData(m_frame, m_file);
    } else if (m_playing) {
        UInt32 magic = 0;
        const bool readMagic = m_file->Read(sizeof magic, &magic, 0);
        const bool readTarget = m_file->Read(sizeof target, &target, 0);
        const bool readData = ReadFrameData(m_frame, m_file);
        if (!readMagic || !readTarget || !readData || magic != kMagic2D) {
            AbortPlayback();
            return false;
        }
    }
    bool snapped = false;
    m_connected = m_scene->SnapDrag(target, 2.0f, m_result, snapped);
    UInt32 flags;
    if (!snapped && !m_connected) {
        m_transform = target;
        flags = CastFree;
    } else {
        flags = m_connected ? CastConnected : CastSnapped;
        m_transform = m_result.m_transform;
    }
    return RecordFrame(flags);
}

// Mac 0x363b40, Win 0x5aa860
bool ClickStickCastUtility::DoCast(const Vector3& origin, const Vector3& direction, Real32 maxDistance,
                                   VisualDebugger* debugger)
{
    ClearResult();
    Vector3 rayOrigin(origin);
    Vector3 rayDirection(direction);
    Real32 distance = maxDistance;
    BoundingBox box(GetBoundingBox());
    Real32 planeHeight = GetPlaneHeight();
    UInt32 physicsMode = GetPhysicsMode();
    if (m_recording) {
        UInt32 magic = kMagic3D;
        m_file->Write(sizeof magic, &magic, 0);
        m_file->Write(sizeof rayOrigin, &rayOrigin, 0);
        m_file->Write(sizeof rayDirection, &rayDirection, 0);
        m_file->Write(sizeof distance, &distance, 0);
        m_file->Write(sizeof box, &box, 0);
        m_file->Write(sizeof planeHeight, &planeHeight, 0);
        m_file->Write(sizeof physicsMode, &physicsMode, 0);
        m_file->Write(sizeof m_distance, &m_distance, 0);
        m_file->Write(sizeof m_offset, &m_offset, 0);
    } else if (m_playing) {
        UInt32 magic = 0;   // (uninitialised in the original)
        bool ok = m_file->Read(sizeof magic, &magic, 0);
        ok &= m_file->Read(sizeof rayOrigin, &rayOrigin, 0);
        ok &= m_file->Read(sizeof rayDirection, &rayDirection, 0);
        ok &= m_file->Read(sizeof distance, &distance, 0);
        ok &= m_file->Read(sizeof box, &box, 0);
        ok &= m_file->Read(sizeof planeHeight, &planeHeight, 0);
        ok &= m_file->Read(sizeof physicsMode, &physicsMode, 0);
        ok &= m_file->Read(sizeof m_distance, &m_distance, 0);
        ok &= m_file->Read(sizeof m_offset, &m_offset, 0);
        if (magic != kMagic3D || !ok) {
            AbortPlayback();
            return false;
        }
    } else if (IsSimple()) {
        // NOTE: at the transform just reset (the identity).
        m_connected = m_scene->CanConnectDrag(m_transform, m_result);
        return true;
    }

    // NOTE: the cast point uses the arguments, not the replayed ray (both builds).
    bool onPlane = false;
    Vector3 point;
    if (!GetCastPoint(origin, direction, maxDistance, box, planeHeight, point, onPlane) && !m_playing)
        return false;
    Matrix3D pivot(GetPivot());
    Matrix3D grip(GetGrip());
    if (m_distance != 0.0f)
        m_transform.SetTranslationMatrix(rayOrigin + rayDirection * m_distance);
    else
        m_transform.SetTranslationMatrix(point);
    m_transform.Multiply(pivot);
    m_transform.Multiply(m_offset);
    m_transform.Multiply(grip);

    if (m_recording) {
        m_file->Write(sizeof pivot, &pivot, 0);
        m_file->Write(sizeof grip, &grip, 0);
        m_file->Write(sizeof point, &point, 0);
        m_file->Write(sizeof m_transform, &m_transform, 0);
        WriteFrameData(m_frame, m_file);
    } else if (m_playing) {
        const bool readPivot = m_file->Read(sizeof pivot, &pivot, 0);
        const bool readGrip = m_file->Read(sizeof grip, &grip, 0);
        const bool readPoint = m_file->Read(sizeof point, &point, 0);
        const bool readTransform = m_file->Read(sizeof m_transform, &m_transform, 0);
        const bool readData = ReadFrameData(m_frame, m_file);
        if (!readPivot || !readGrip || !readPoint || !readTransform || !readData) {
            AbortPlayback();
            return false;
        }
    }

    // 1. the ray snap (the connections of the last placement, then the elements in the cone)
    bool snapped = false;
    s_timer2D.Start();
    m_connected = m_scene->SnapDragRay(rayOrigin, rayDirection, m_transform, distance * distance,
                                       m_result, snapped);
    s_timer2D.Stop();
    UInt32 flags;
    bool snapDrag;
    if (m_connected || snapped) {
        flags = m_connected ? CastConnected : CastSnapped;
        m_transform = m_result.m_transform;
        m_distance = Distance(rayOrigin, (-grip.GetTranslation()) * m_transform);
        snapDrag = false;
    } else {
        bool placed = false;
        if (!m_hasPhysics) {
            flags = 0;
        } else {
            // 2. a physics sweep of the drag scene (or the terrain only) along the ray
            // NOTE (both builds, Vortex physics): CastDragScene never reports a hit in LDD (its
            // sweep / ray filters want a shape of kind 4 and see only composites without a shape;
            // LDD's scenery uses flags 1 / 2), so the `hit` branch below never runs
            // (the restoration's diff tests).
            PhysicsScene* physics = m_scene->GetPhysicsScene();
            Matrix3D cast;
            cast.SetTranslationMatrix(rayOrigin);
            cast.Multiply(pivot);
            bool terrainOnly = true;
            if (physicsMode <= 1) {
                cast.Multiply(grip);
                terrainOnly = false;
            }
            Real32 castDistance = Distance(box.m_min, box.m_max);
            Vector3 target = cast.GetTranslation() + rayDirection * castDistance;
            s_timer3DPhysics.Start();
            const bool hit = physics->CastDragScene(cast, target, castDistance, terrainOnly);
            s_timer3DPhysics.Stop();
            if (!hit) {
                flags = CastFree;
            } else {
                onPlane = true;
                Vector3 position = cast.GetTranslation() + rayDirection * (castDistance + -0.01f);
                if (physicsMode == 2) {
                    // dropped from above onto what lies below the hit
                    position += Rotate(grip.GetTranslation(), pivot);
                    SnapToGrid(position);
                    position.y = SnapHeight(position.y);
                    cast.SetTranslation(position);
                    m_transform = cast;
                    cast.m[13] += 1000.0f;
                    position.y += -1000.0f;
                    Vector3 down(0.0f, -1.0f, 0.0f);
                    s_timer3DPhysicsDown.Start();
                    const bool below = physics->CastDragScene(cast, position, castDistance, false);
                    s_timer3DPhysicsDown.Stop();
                    if (below) {
                        position = cast.GetTranslation() + down * castDistance;
                        position.y = SnapHeight(position.y);
                        m_transform.SetTranslation(position);
                    }
                } else if (physicsMode == 1) {
                    // the first free brick level from the hit up to the ray origin
                    Vector3 start(cast.GetTranslation());
                    position.y = SnapHeight(position.y);
                    // NOTE: the original reuses an uninitialised point when the plane is missed.
                    Vector3 level;
                    while (start.y > position.y) {
                        Plane plane(0.0f, 1.0f, 0.0f, -position.y);
                        RayPlaneIntersect(start, rayDirection, plane, level);
                        SnapToGrid(level);
                        cast.SetTranslation(level);
                        s_timer3DEnvironment.Start();
                        const bool blocked = physics->CastDragScene(cast, level, castDistance, false);
                        s_timer3DEnvironment.Stop();
                        if (!blocked) {
                            m_transform = cast;
                            break;
                        }
                        position.y += 0.32f;
                    }
                } else if (physicsMode == 0) {
                    m_transform = cast;
                    m_transform.SetTranslation(position);
                }
                m_distance = Distance(rayOrigin, (-grip.GetTranslation()) * m_transform);
                flags = CastFree | CastPhysics;
                placed = true;
            }
        }
        if (!placed) {
            // 3. at the cast point (box exit or grid plane), snapped to the grid
            m_transform.SetTranslationMatrix(point);
            m_transform.Multiply(pivot);
            m_transform.Multiply(grip);
            SnapTransformToGrid(m_transform);
            m_distance = 0.0f;
        }
        snapDrag = onPlane;
    }
    if (snapDrag) {
        s_timer3D.Start();
        m_connected = m_scene->SnapDrag(m_transform, 0.25f, m_result, snapped);
        s_timer3D.Stop();
        if (m_connected) {
            flags |= CastSnapDrag | CastSnapDragHit;
            m_transform = m_result.m_transform;
        } else {
            flags |= CastSnapDrag;
        }
    }

    // the grip offset of a connecting placement: the placement relative to the pivot, moved so
    // that the grip lies on the ray
    Matrix3D offset;
    if (m_connected && !m_result.m_connections.IsEmpty()) {
        flags |= CastOffset;
        Matrix3D rotation(m_transform);
        rotation.SetTranslation(Vector3(0.0f, 0.0f, 0.0f));
        offset.SetInverse(pivot);
        offset.Multiply(rotation);
        Vector3 gripPoint = (-grip.GetTranslation()) * m_transform;
        Plane plane(rayDirection, gripPoint);
        Vector3 hit;
        LinePlaneIntersect(rayOrigin, rayDirection, plane, hit);
        gripPoint.Subtract(hit);
        offset.SetTranslation(gripPoint);
    }
    if (m_offset.IsIdentity()) {
        flags |= CastFirstOffset;
        m_offset = offset;
        return RecordFrame(flags);
    }
    if ((flags & CastOffset) != 0 && m_offset.IsRotationEqual(offset, 0.1f)
        && m_offset.GetTranslation().IsEqual(offset.GetTranslation(), 0.8f))
        return RecordFrame(flags);
    // the offset changed: forget it and cast again
    flags |= CastOffsetReset;
    m_offset.SetIdentity();
    if (!RecordFrame(flags))
        return false;
    return DoCast(origin, direction, maxDistance, debugger);
}

// Mac 0x3655d0
void ClickStickCastUtility::GetConnectedParts(BasicArray<RefPointer<Part> >& parts) const
{
    parts.Clear();
    if (!m_connected)
        return;
    for (BasicArray<PrimitiveConnectionInfo>::ConstIterator it = m_result.m_connections.Begin();
         it != m_result.m_connections.End(); ++it) {
        // the element of the pair that is not dragged
        PrimitiveElement* element = it->m_element->GetScene() == m_dragScene ? it->m_otherElement
                                                                            : it->m_element;
        RefPointer<Part> part(element->GetPart());
        bool found = false;
        for (UInt32 i = 0; i < parts.GetSize() && !found; ++i)
            found = parts[i].Get() == part.Get();
        if (!found)
            parts.PushBack(part);
    }
}

// Mac 0x365910, Win 0x5abd20
bool ClickStickCastUtility::GetCastPoint(const Vector3& origin, const Vector3& direction,
                                         Real32 maxDistance, const BoundingBox& box,
                                         Real32 planeHeight, Vector3& point, bool& onPlane)
{
    point = ClipRayToBox(origin, direction, maxDistance, box);
    Vector3 hit;
    Plane plane(0.0f, 1.0f, 0.0f, planeHeight);
    if (RayPlaneIntersect(origin, direction, plane, hit)) {
        Real32 hitDistance = origin.GetDistanceSquared(hit);
        Real32 pointDistance = origin.GetDistanceSquared(point);
        if (pointDistance >= hitDistance) {
            onPlane = true;
            point = hit;
        }
    }
    return true;
}

// Mac 0x365a00, Win 0x5ac050 (+ 0x5abfd0 per plane)
Vector3 ClickStickCastUtility::ClipRayToBox(const Vector3& origin, const Vector3& direction,
                                           Real32 maxDistance, const BoundingBox& box)
{
    // the six faces, normals pointing inside
    const Plane planes[6] = {
        Plane(Vector3(1.0f, 0.0f, 0.0f), box.m_min),
        Plane(Vector3(0.0f, 1.0f, 0.0f), box.m_min),
        Plane(Vector3(0.0f, 0.0f, 1.0f), box.m_min),
        Plane(Vector3(-1.0f, 0.0f, 0.0f), box.m_max),
        Plane(Vector3(0.0f, -1.0f, 0.0f), box.m_max),
        Plane(Vector3(0.0f, 0.0f, -1.0f), box.m_max)
    };
    Real32 t = maxDistance;
    for (UInt32 i = 0; i < 6; ++i) {
        Real32 hit;
        if (0.0f <= planes[i].GetDistance(origin) && RayPlaneIntersect(origin, direction, planes[i], hit)
            && hit < t)
            t = hit;
    }
    return origin + direction * t;
}

// Mac 0x365d10, Win 0x5abdf0
void ClickStickCastUtility::SnapTransformToGrid(Matrix3D& transform)
{
    SnapToGrid(transform.GetTranslation());
}

// Mac 0x365d30, Win 0x5abe10
void ClickStickCastUtility::SnapToGrid(Vector3& point)
{
    if (m_fineGrid) {
        point.x = RoundToStep(point.x, 0.4f, 0.2f);
        point.z = RoundToStep(point.z, 0.4f, 0.2f);
    } else {
        Real32 x = RoundToStep(point.x, 0.8f, 0.4f);
        point.x = x;
        Real32 z = RoundToStep(point.z, 0.8f, 0.4f);
        point.z = z;
        if (!m_aligned) {
            // NOTE: platform difference: Win adds the double 0.4 (x87), the Mac 0.4f (addss).
#if defined(_WIN32)
            point.x = (Real32)((Real64)x + 0.4);
            point.z = (Real32)((Real64)z + 0.4);
#else
            point.x = x + 0.4f;
            point.z = z + 0.4f;
#endif
        }
    }
}

// Mac 0x365ee0, Win 0x5abed0
void ClickStickCastUtility::DrawDebugInfo(const Matrix3D& transform, VisualDebugger* debugger)
{
    PhysicsSceneDebugInfo* info = m_dragScene->GetDebugInfo(0x804, transform);
    if (info == 0)
        return;
    UInt32 count = info->GetPointCount();
    for (const PhysicsSceneDebugInfo::Point* p = info->GetPoints(); count != 0; --count, ++p)
        debugger->AddPoint(kListenerCode, 0, p->m_position, p->m_color, 0.5f);
    count = info->GetLineCount();
    for (const PhysicsSceneDebugInfo::Line* l = info->GetLines(); count != 0; --count, ++l)
        debugger->AddLine(kListenerCode, 0, l->m_from, l->m_to, l->m_color);
    count = info->GetTriangleCount();
    for (const PhysicsSceneDebugInfo::Triangle* t = info->GetTriangles(); count != 0; --count, ++t)
        debugger->AddTriangle(kListenerCode, 0, t->m_vertices[0], t->m_vertices[1], t->m_vertices[2],
                              t->m_color);
    info->Release();
}

// =============================================================================================
// ClickStickCastSimpleUtility
// =============================================================================================

// Mac 0x366030
ClickStickCastSimpleUtility::ClickStickCastSimpleUtility()
    : m_planeHeight(0.0f), m_heightOffset(0.0f),
      m_box(-1000.0f, -1000.0f, -1000.0f, 1000.0f, 1000.0f, 1000.0f), m_physicsMode(1),
      m_simple(false)
{
}

// Mac 0x3668d0 (D1) / 0x3668e0 (D0), Win sdd 0x5aa610 (body 0x5aa550)
ClickStickCastSimpleUtility::~ClickStickCastSimpleUtility()
{
}

// Mac 0x366100
void ClickStickCastSimpleUtility::UpdateHeightOffset()
{
    BoundingBox box(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    m_dragScene->GetBoundingBox(box, true);
    box.Translate(m_grip.GetTranslation());
    BoundingBox rotated = LEGO::Rotate(box, m_pivot);
    m_heightOffset = -rotated.m_min.y;
}

// Mac 0x366190
void ClickStickCastSimpleUtility::Rotate(UInt32 direction, const Vector3& up, const Vector3& view)
{
    Reset();
    Matrix3D rotation;
    Vector3 axis;
    Vector3 x(1.0f, 0.0f, 0.0f);
    Vector3 y(0.0f, 1.0f, 0.0f);
    Vector3 z(0.0f, 0.0f, 1.0f);
    Vector3 u;
    Vector3 v;
    // NOTE: both checks test the length of `view` (both builds).
    u.Set(up);
    if (!(1e-5f > Abs(view.GetLength() + -1.0f)))
        u.Normalize();
    v.Set(view);
    if (!(1e-5f > Abs(view.GetLength() + -1.0f)))
        v.Normalize();
    const Real32 kAxisLimit = 0.39269909262657166f;   // pi / 8
    if (direction - 5 < 2) {
        // about the world axis nearest to the view axis (z if the view is not flat)
        Real32 dy = v.Dot(y);
        Real32 dx = v.Dot(x);
        Real32 dz = v.Dot(z);
        if (kAxisLimit > Abs(dz)) {
            if (Abs(dx) >= Abs(dy)) {
                if (0.0f > dx)
                    x = -x;
                axis.Set(x);
            } else {
                if (0.0f > dy)
                    y = -y;
                axis.Set(y);
            }
        } else {
            if (0.0f > dz)
                z = -z;
            axis.Set(z);
        }
    }
    if (direction - 3 < 2) {
        // about the world axis nearest to the up direction (y unless the view is steep)
        Real32 dy = u.Dot(y);
        Real32 dx = u.Dot(x);
        Real32 dz = u.Dot(z);
        if (kAxisLimit > Abs(dy)) {
            if (Abs(dz) >= Abs(dx)) {
                if (0.0f > dz)
                    z = -z;
                axis.Set(z);
            } else {
                if (0.0f > dx)
                    x = -x;
                axis.Set(x);
            }
        } else {
            if (0.0f > dy)
                y = -y;
            axis.Set(y);
        }
    }
    if (direction - 1 < 2) {
        // about the horizontal world axis across the view
        Vector3 across = u.Cross(v);
        Real32 cx = across.Dot(x);
        across.Dot(y);
        Real32 cz = across.Dot(z);
        if (Abs(v.z) >= Abs(v.x)) {
            if (0.0f > cx)
                x = -x;
            axis.Set(x);
        } else {
            if (0.0f > cz)
                z = -z;
            axis.Set(z);
        }
    }
    bool turn = true;
    if (direction <= 6) {
        if (((0x2aU >> direction) & 1) != 0)
            rotation.SetRotation(axis, 1.57079637f);
        else if (((0x54U >> direction) & 1) != 0)
            rotation.SetRotation(axis, -1.57079637f);
        else {
            m_pivot.SetIdentity();
            turn = false;
        }
    }
    if (turn) {
        rotation.Multiply(m_pivot);
        m_pivot = rotation;
    }
    UpdateHeightOffset();
}

} // namespace LEGO
