// LEGO Digital Designer — restored source
// BrickConnectSceneControl (Mac TU 021). See BrickConnectSceneControl.h for the sub-TUs
// restored here and what is not.
// Template code of the TU (BasicArray / BasicMap / std::_Rb_tree instances and the inlined
// NotifierT iterations, e.g. Mac 0x35a10, 0x35b30, 0x35f00, 0x36630, 0x397b0, 0x38100, 0x38de0,
// 0x39dc0, 0x45390, 0x452d0 / Win 0x4c6990, 0x4bba0's erase 0x5a410) comes from the templates.
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneManager.h"
#include "BrickConnectSceneControlTool.h"
#include "BrickDraw.h"
#include "ModelLoaderProgressBarThread.h"
#include "LDDApplication.h"
#include "LDDPart.h"
#include "LDDUtilities.h"
#include "LDDWebCommunicator.h"
#include "LDDPostProcessor.h"
#include "PrimitiveElementBrick.h"
#include "BI.h"
#include "FlexVertexBuffer.h"
#include "RenderControlManager.h"
#include "RenderControl.h"
#include "ControlManager.h"
#include "Control.h"
#include "Application.h"
#include "Platform.h"
#include "Preferences.h"
#include "StringStream.h"
#include "Directory.h"
#include "Path.h"
#include "ObjectFactory.h"
#include "RenderQueue.h"
#include "RenderGrid.h"
#include "PostProcessingEffect.h"
#include "Camera.h"
#include "Materials.h"
#include "Mesh.h"
#include "Texture.h"
#include "VertexBuffer.h"
#include "Skybox.h"
#include "Bitmap.h"
#include "BitmapCodec.h"
#include "DataStream.h"
#include "File.h"
#include "Scene.h"
#include "Brick.h"
#include "Part.h"
#include <string.h>
#include <unordered_map>
#include <map>
#include <tuple>
#include <cmath>
#include <vector>
#include "Updater.h"
#include "SystemTime.h"
#include "BrickConnect.h"
#include "BrickGroup.h"
#include "HingedElement.h"
#include "RigidElement.h"
#include "PrimitiveRigidElement.h"
#include "PrimitiveElement.h"
#include "ConnectionField.h"
#include "StorageScene2.h"
#include "Countries.h"
#include "Behavior.h"
#include "BrickPalette.h"
#include "Primitive.h"
#include "BrickGeometry.h"
#include "TextLayout.h"

#include <stdlib.h>
#include <math.h>
#include <typeinfo>

namespace LEGO {

// Body meshes with equal contents share one vertex buffer. Meshes are in body
// coordinates, so every loose brick of one design and colour, or every copy of an assembly, builds
// the same buffer; the original keeps one per body (about 20 KB per 2x4 brick, in memory and the
// driver). A buffer is shared only when every array is byte-identical; unused ones leave the cache
// after each rebuild (RebuildRenderData, UpdateRenderData).
class BrickConnectSceneControl::MeshBufferCache {
public:
    struct Contents {
        UInt32 vertexCount;
        UInt32 indexCount;
        const Vector3* positions;
        const Vector3* normals;
        const Vector2* texCoords;              // 0: none (material meshes)
        const UInt32* indices;
        bool roundEdges;
        UInt32 roundEdgePointCount;
        UInt32 roundEdgeDataSize;
        const Vector3* roundEdgePoints;
        const UInt32* roundEdgeData;
        const UInt32* roundEdgePointIndices;   // indexCount each
        const UInt32* roundEdgeDataIndices;
    };

    static UInt64 Hash(const Contents& c)
    {
        UInt64 h = 0x9e3779b97f4a7c15ull ^ ((UInt64)c.vertexCount << 32 | c.indexCount);
        h = Mix(h, (UInt64)c.roundEdges << 63 | (UInt64)(c.texCoords != 0) << 62 |
                   (UInt64)c.roundEdgePointCount << 31 | c.roundEdgeDataSize);
        h = Bytes(h, c.positions, c.vertexCount * 12);
        h = Bytes(h, c.normals, c.vertexCount * 12);
        h = Bytes(h, c.indices, c.indexCount * 4);
        if (c.texCoords != 0)
            h = Bytes(h, c.texCoords, c.vertexCount * 8);
        if (c.roundEdges) {
            h = Bytes(h, c.roundEdgePoints, c.roundEdgePointCount * 12);
            h = Bytes(h, c.roundEdgeData, c.roundEdgeDataSize * 4);
            h = Bytes(h, c.roundEdgePointIndices, c.indexCount * 4);
            h = Bytes(h, c.roundEdgeDataIndices, c.indexCount * 4);
        }
        return h;
    }

    VertexBuffer* Find(const Contents& c, UInt64 hash) const
    {
        typedef std::unordered_multimap<UInt64, RefPointer<VertexBuffer> >::const_iterator It;
        std::pair<It, It> range = m_buffers.equal_range(hash);
        for (It it = range.first; it != range.second; ++it)
            if (Same(it->second.Get(), c))
                return it->second.Get();
        return 0;
    }

    void Add(VertexBuffer* buffer, UInt64 hash)
    {
        m_buffers.insert(std::make_pair(hash, RefPointer<VertexBuffer>(buffer)));
    }

    // drops the buffers only the cache holds
    void Prune()
    {
        for (std::unordered_multimap<UInt64, RefPointer<VertexBuffer> >::iterator it = m_buffers.begin();
             it != m_buffers.end();) {
            if (it->second->GetRefCount() == 1)
                it = m_buffers.erase(it);
            else
                ++it;
        }
    }

private:
    static UInt64 Mix(UInt64 h, UInt64 v)
    {
        h ^= v;
        h *= 0xff51afd7ed558ccdull;
        return h ^ (h >> 32);
    }

    static UInt64 Bytes(UInt64 h, const void* data, UInt32 size)
    {
        const UInt8* p = (const UInt8*)data;
        UInt32 i = 0;
        for (; i + 8 <= size; i += 8) {
            UInt64 v;
            memcpy(&v, p + i, 8);
            h = Mix(h, v);
        }
        UInt64 tail = 0;
        if (i < size)
            memcpy(&tail, p + i, size - i);
        return Mix(h, tail ^ size);
    }

    static bool Equal(const void* a, const void* b, UInt32 size)
    {
        return size == 0 || (a != 0 && b != 0 && memcmp(a, b, size) == 0);
    }

    // the buffer holds exactly the arrays the body would put into a new one
    static bool Same(const VertexBuffer* b, const Contents& c)
    {
        if (b->GetVertexCount() != c.vertexCount || b->GetIndexCount() != c.indexCount ||
            b->GetMaxVertexCount() != c.vertexCount || b->GetMaxIndexCount() != c.indexCount)
            return false;
        UInt32 stride = 0;
        const void* positions = b->GetPositions(stride, 0);
        if (stride != 12 || !Equal(positions, c.positions, c.vertexCount * 12))
            return false;
        stride = 0;
        const void* normals = b->GetNormals(stride);
        if (stride != 12 || !Equal(normals, c.normals, c.vertexCount * 12))
            return false;
        if (!Equal(b->GetIndices(), c.indices, c.indexCount * 4))
            return false;
        if (b->GetTexCoordSetCount() != (c.texCoords != 0 ? 1u : 0u))
            return false;
        if (c.texCoords != 0) {
            stride = 0;
            const void* texCoords = b->GetTexCoords(stride, 0);
            if (stride != 8 || !Equal(texCoords, c.texCoords, c.vertexCount * 8))
                return false;
        }
        if (!c.roundEdges)
            return b->GetRoundEdgePointCount() == 0 && b->GetRoundEdgeDataCount() == 0 &&
                   b->GetRoundEdgeData() == 0;
        if (b->GetRoundEdgePointCount() != c.roundEdgePointCount || b->GetRoundEdgeDataCount() != c.roundEdgeDataSize)
            return false;
        stride = 0;
        const void* points = b->GetRoundEdgePoints(stride);
        return stride == 12 && Equal(points, c.roundEdgePoints, c.roundEdgePointCount * 12) &&
               Equal(b->GetRoundEdgeData(), c.roundEdgeData, c.roundEdgeDataSize * 4) &&
               Equal(b->GetRoundEdgePointIndices(), c.roundEdgePointIndices, c.indexCount * 4) &&
               Equal(b->GetRoundEdgeDataIndices(), c.roundEdgeDataIndices, c.indexCount * 4);
    }

    std::unordered_multimap<UInt64, RefPointer<VertexBuffer> > m_buffers;
};

namespace {
// The accumulated mesh buffer, created as CreateAccumulatedMeshes does (texCoordDimensions: 0 for
// the material meshes)
VertexBuffer* NewAccumulatedBuffer(Render* render, UInt32 vertexCount, UInt32 indexCount,
                                   const BasicArray<UInt32>* texCoordDimensions, UInt32 roundEdgeDataSize,
                                   UInt32 roundEdgePointCount)
{
    VertexBuffer* buffer = 0;
    if (!LDDApplication::GetInstance()->UseRoundEdges()) {
        VertexBuffer::CreateArgs args;
        args.vertexCount = vertexCount;
        args.indexCount = indexCount;
        args.frameCount = 0;
        args.dynamic = false;
        args.hasNormals = true;
        args.colorSetCount = 0;
        if (texCoordDimensions != 0)
            args.texCoordDimensions = *texCoordDimensions;
        Object* object = render->GetObjectFactory()->CreateObject(typeid(VertexBuffer), args);
        if (object)
            buffer = dynamic_cast<VertexBuffer*>(object);
    } else {
        VertexBuffer::CreateArgsRoundEdge args;
        args.vertexCount = vertexCount;
        args.indexCount = indexCount;
        args.frameCount = 0;
        args.dynamic = false;
        args.hasNormals = true;
        args.colorSetCount = 0;
        if (texCoordDimensions != 0)
            args.texCoordDimensions = *texCoordDimensions;
        args.roundEdgeDataCount = roundEdgeDataSize;
        args.roundEdgePointCount = roundEdgePointCount;
        Object* object = render->GetObjectFactory()->CreateObject(typeid(VertexBuffer), args);
        if (object)
            buffer = dynamic_cast<VertexBuffer*>(object);
    }
    return buffer;
}
} // namespace


// Out of line here (the original's inline destructor; see BrickConnectSceneControlObserver.h).
BrickConnectSceneControlObserver::~BrickConnectSceneControlObserver()
{
}

namespace {

typedef NotifierT<BrickConnectSceneControl, BrickConnectSceneControlObserver> ControlNotifier;

// The observers' iteration of the control (Mac: the inlined NotifierT::Iterator push / pop).
#define BCSC_NOTIFY(call)                                                               \
    do {                                                                                \
        ControlNotifier::Iterator it_(*this);                                           \
        while (BrickConnectSceneControlObserver* o_ = it_.Next(ControlNotifier::m_observers)) \
            o_->call;                                                                   \
    } while (0)

// Smallest power of two >= n (at least 1). (inline, Mac 0x45870 / Win 0x615ac4)
inline UInt32 NextPowerOfTwo(UInt32 n)
{
    UInt32 p = 1;
    while (p < n)
        p *= 2;
    return p;
}

// ColoredMaterial of the debug render modes (Mac 0x39df0: inline new ColoredMaterial;
// Win 0x60944b MaterialColors + 0x57c8e0): `diffuse`, ambient 0.1 grey, black specular and
// emission, shininess 85.
Material* NewDebugMaterial(const Color& diffuse)
{
    MaterialColors colors;
    colors.diffuse = diffuse;
    colors.ambient = Color(0.1f, 0.1f, 0.1f, 1.0f);
    colors.specular = Color::Black();
    colors.emissive = Color::Black();
    colors.shininess = 85.0f;
    return new ColoredMaterial(colors);
}

// Boolean preference `key` into `value` when it is set (Mac: Preferences::GetValue + the
// FromString(bool) 0x142710, inlined in the constructor and OnPreferenceChanged).
void ReadBoolPreference(const String& key, bool& value)
{
    RefPointer<Preferences> preferences = Preferences::GetInstance();
    String text;
    if (preferences->GetValue(key, text))
        FromString(value, text);
}

// The same for a UInt32 (FromString 0x142570).
void ReadUInt32Preference(const String& key, UInt32& value)
{
    RefPointer<Preferences> preferences = Preferences::GetInstance();
    String text;
    if (preferences->GetValue(key, text))
        FromString(value, text);
}

// The 3D view's camera (and light cameras): near plane, far plane (view distance), focal length,
// zoom-out limit. The original uses 1500 / 1400; the ViewDistance setting (200 .. 1,000,000 LDD
// units) sets the far plane, and the zoom-out limit stays 100 below it.
Camera* NewViewCamera()
{
    Real32 zFar = 1500.0f;
    Real32 maxDistance = 1400.0f;
    RefPointer<Preferences> preferences = Preferences::GetInstance();
    String text;
    Real32 value = 0.0f;
    if (preferences->GetValue(String("ViewDistance"), text) && FromString(value, text) &&
        value >= 200.0f && value <= 1000000.0f) {
        zFar = value;
        maxDistance = value - 100.0f;
    }
    return new Camera(0.9f, zFar, 80.0f, maxDistance, 1);
}

// The ScaledWheelZoom setting: a wheel notch moves the camera by 5 % of its distance to the target,
// at least the original's 5 units. Read once.
bool ScaledWheelZoom()
{
    static int scaled = -1;
    if (scaled < 0) {
        bool value = false;
        ReadBoolPreference(String("ScaledWheelZoom"), value);
        scaled = value ? 1 : 0;
    }
    return scaled == 1;
}

// The view culling's sphere of a body mesh (RenderQueue::DrawEntries), around its vertices,
// computed once; animated or other kinds of buffers get none and are never culled.
void ComputeCullSphere(BrickConnectSceneControl::PlacedMesh& placed)
{
    placed.m_cullRadius = -1.0f;
    SingleMesh* single = dynamic_cast<SingleMesh*>(placed.m_mesh.Get());
    if (single == 0)
        return;
    VertexBuffer* buffer = single->GetVertexBuffer();
    if (buffer == 0 || buffer->GetFrameCount() > 1)
        return;
    UInt32 stride = 0;
    const UInt8* p = (const UInt8*)buffer->GetPositions(stride, 0);
    UInt32 count = buffer->GetVertexCount();
    if (p == 0 || count == 0 || stride < 12)
        return;
    Real32 lo[3], hi[3];
    for (int k = 0; k < 3; ++k)
        lo[k] = hi[k] = ((const Real32*)p)[k];
    for (UInt32 i = 0; i < count; ++i) {
        const Real32* v = (const Real32*)(p + (size_t)i * stride);
        for (int k = 0; k < 3; ++k) {
            if (!(v[k] == v[k]))
                return;   // NaN: no sphere
            if (v[k] < lo[k])
                lo[k] = v[k];
            if (v[k] > hi[k])
                hi[k] = v[k];
        }
    }
    Vector3 center((lo[0] + hi[0]) * 0.5f, (lo[1] + hi[1]) * 0.5f, (lo[2] + hi[2]) * 0.5f);
    Real32 r2 = 0.0f;
    for (UInt32 i = 0; i < count; ++i) {
        const Real32* v = (const Real32*)(p + (size_t)i * stride);
        Real32 dx = v[0] - center.x, dy = v[1] - center.y, dz = v[2] - center.z;
        Real32 d2 = dx * dx + dy * dy + dz * dz;
        if (d2 > r2)
            r2 = d2;
    }
    placed.m_cullCenter = center;
    placed.m_cullHalf = Vector3((hi[0] - lo[0]) * 0.5f, (hi[1] - lo[1]) * 0.5f, (hi[2] - lo[2]) * 0.5f);
    placed.m_cullRadius = Sqrt(r2) * 1.01f + 0.05f;   // a margin for rounding
}

// The sphere of `placed` at `transform` (world space) for the next queued entry.
void SetCullBounds(RenderQueue* queue, BrickConnectSceneControl::PlacedMesh& placed,
                   const Matrix3D& transform)
{
    if (placed.m_cullRadius < -1.5f)
        ComputeCullSphere(placed);
    if (placed.m_cullRadius < 0.0f)
        return;
    // the largest stretch of the transform (rigid: 1)
    Real32 scale = Rotate(Vector3(1.0f, 0.0f, 0.0f), transform).GetLength();
    Real32 sy = Rotate(Vector3(0.0f, 1.0f, 0.0f), transform).GetLength();
    Real32 sz = Rotate(Vector3(0.0f, 0.0f, 1.0f), transform).GetLength();
    if (sy > scale)
        scale = sy;
    if (sz > scale)
        scale = sz;
    queue->SetNextBounds(placed.m_cullCenter * transform, placed.m_cullRadius * scale * 1.01f);
}

// Level of detail (LDD64): a body's elements are grouped by regions of this size (LDD units), so a
// large body (a whole model can be one) is not drawn at one detail.
const Real32 kLodRegionSize = 32.0f;
// Default and accepted range of LevelOfDetailDistance (LDD units; a stud is 0.48 wide, about 2
// pixels at 500 in a 1,000-pixel view).
const Real32 kLodDefaultDistance = 500.0f;
const Real32 kLodMinimumDistance = 10.0f;
const Real32 kLodMaximumDistance = 1000000.0f;

// The plain material of a box: the colours of `material` lit, no round edges or textures.
Material* NewLodBoxMaterial(Material* material)
{
    MaterialColors colors;
    colors.diffuse = Color(0.6f, 0.6f, 0.6f, 1.0f);
    colors.ambient = Color(0.1f, 0.1f, 0.1f, 1.0f);
    colors.specular = Color::Black();
    colors.emissive = Color::Black();
    colors.shininess = 85.0f;
    if (ColoredMaterial* colored = dynamic_cast<ColoredMaterial*>(material))
        colors = colored->GetColors();
    else if (ShadedMaterial* shaded = dynamic_cast<ShadedMaterial*>(material))
        colors = shaded->GetColors();
    return new ColoredMaterial(colors);
}

// The box lo..hi (element space) at `transform` (element -> body space) into a box mesh's
// arrays: 24 vertices with face normals, 12 triangles.
struct LodBoxArrays {
    std::vector<Vector3> positions;
    std::vector<Vector3> normals;
    std::vector<UInt32>  indices;
};

void AddLodBox(LodBoxArrays& arrays, const Vector3& lo, const Vector3& hi, const Matrix3D& transform)
{
    static const int kFaces[6][4] = {{0, 2, 3, 1}, {4, 5, 7, 6}, {0, 1, 5, 4},
                                     {2, 6, 7, 3}, {0, 4, 6, 2}, {1, 3, 7, 5}};
    static const Real32 kNormals[6][3] = {{-1, 0, 0}, {1, 0, 0}, {0, -1, 0},
                                          {0, 1, 0},  {0, 0, -1}, {0, 0, 1}};
    static const UInt32 kTriangles[6] = {0, 1, 2, 0, 2, 3};
    Vector3 corners[8];
    for (int i = 0; i < 8; ++i)
        corners[i] = Vector3((i & 4) ? hi.x : lo.x, (i & 2) ? hi.y : lo.y, (i & 1) ? hi.z : lo.z) * transform;
    for (int f = 0; f < 6; ++f) {
        Vector3 n = Rotate(Vector3(kNormals[f][0], kNormals[f][1], kNormals[f][2]), transform);
        n.Normalize();
        UInt32 base = (UInt32)arrays.positions.size();
        for (int k = 0; k < 4; ++k) {
            arrays.positions.push_back(corners[kFaces[f][k]]);
            arrays.normals.push_back(n);
        }
        for (int k = 0; k < 6; ++k)
            arrays.indices.push_back(base + kTriangles[k]);
    }
}

// The meshes of `regions` at their detail into `queue`: full, without studs beyond reducedDistance2
// (squared), boxes beyond boxDistance2 (when not 0).
void QueueLodRegions(RenderQueue* queue, BasicArray<BrickConnectSceneControl::BIBrick::LodRegion>& regions,
                     const Matrix3D& bodyTransform, const Vector3& eye, Real32 reducedDistance2,
                     Real32 boxDistance2)
{
    typedef BrickConnectSceneControl::PlacedMesh PlacedMesh;
    for (BasicArray<BrickConnectSceneControl::BIBrick::LodRegion>::Iterator r = regions.Begin();
         r != regions.End(); ++r) {
        Real32 d2 = DistanceSquared(r->center * bodyTransform, eye);
        BasicArray<PlacedMesh>* meshes = &r->full;
        if (boxDistance2 > 0.0f && d2 > boxDistance2 && !r->boxes.IsEmpty())
            meshes = &r->boxes;
        else if (d2 > reducedDistance2)
            meshes = &r->reduced;
        for (BasicArray<PlacedMesh>::Iterator m = meshes->Begin(); m != meshes->End(); ++m) {
            Matrix3D transform(bodyTransform, m->m_transform);
            Real32 depth = DistanceSquared(m->m_center * transform, eye);
            SetCullBounds(queue, *m, transform);
            queue->Add(m->m_mesh, transform, depth, m->m_layer, m->m_opacity);
        }
    }
}

// `part` in `parts` (the inlined std::find over RefPointers).
bool ContainsPart(const BasicArray<RefPointer<Part> >& parts, const Part* part)
{
    for (BasicArray<RefPointer<Part> >::ConstIterator it = parts.Begin(); it != parts.End(); ++it)
        if (it->Get() == part)
            return true;
    return false;
}

// One random wire colour component in [0, 1] (Mac rand() * 2^-31; Win rand() / 32767.0, the
// RAND_MAX of each CRT).
inline Real32 RandomUnit()
{
#if defined(_WIN32)
    return (Real32)((double)rand() / 32767.0);
#else
    return (Real32)rand() * 4.65661287e-10f;
#endif
}

} // namespace

// =============================================================================================
// fast navigation (LDD64)
// =============================================================================================
// With the FastNavigation setting, a frame drawn while the camera moves, after a full frame slower
// than FastNavigationThreshold ms (default 50), shows one box per body mesh in its colour, in a few
// batched buffers. A quarter of a second after the camera stops the full view is drawn again, so
// still frames are the original's. The boxes are built when a movement starts.

class BrickConnectSceneControl::NavigationProxy : public UpdateObserver {
public:
    explicit NavigationProxy(BrickConnectSceneControl* control)
        : m_control(control), m_enabled(false), m_threshold(50), m_lastFullMs(0),
          m_haveView(false), m_shown(false), m_lastMove(0)
    {
        ReadBoolPreference(String("FastNavigation"), m_enabled);
        ReadUInt32Preference(String("FastNavigationThreshold"), m_threshold);
        if (m_enabled) {
            RefPointer<Updater> updater = MainUpdater::GetInstance();
            if (!updater.IsNull())
                UpdateObserver::Attach(updater);
        }
    }
    virtual ~NavigationProxy() { UpdateObserver::Detach(); }

    // Per main-loop pass: the full frame once the camera has stood still for 250 ms.
    virtual void Update()
    {
        if (m_shown && GetMilliseconds() - m_lastMove > 250) {
            m_shown = false;
            m_control->Redraw();
        }
    }

    // At the start of a frame with the camera's `view`: whether to draw the boxes.
    bool BeginFrame(const Matrix3D& view)
    {
        if (!m_enabled)
            return false;
        bool moved = m_haveView && memcmp(&view, &m_view, sizeof m_view) != 0;
        m_view = view;
        m_haveView = true;
        if (!moved || m_lastFullMs <= (Int64)m_threshold)
            return false;
        m_lastMove = GetMilliseconds();
        if (!m_shown)
            Build();
        m_shown = !m_meshes.IsEmpty();
        return m_shown;
    }

    // A full frame took `ms`: remembered; the boxes are dropped.
    void EndFullFrame(Int64 ms)
    {
        m_lastFullMs = ms;
        m_shown = false;
        m_meshes.Clear();
    }

    void Queue(RenderQueue* queue)
    {
        for (BasicArray<RefPointer<Mesh> >::Iterator m = m_meshes.Begin(); m != m_meshes.End(); ++m)
            queue->Add(*m, Matrix3D(), 0.0f, 0, 1.0f);
    }

private:
    struct Batch {
        RefPointer<Material> material;
        std::vector<Real32> positions;
        std::vector<Real32> normals;
        std::vector<UInt32> indices;
    };

    // The colours of `material` as a plain lit material (no round edges or textures).
    static Material* ProxyMaterial(Material* material)
    {
        MaterialColors colors;
        colors.diffuse = Color(0.6f, 0.6f, 0.6f, 1.0f);
        colors.ambient = Color(0.1f, 0.1f, 0.1f, 1.0f);
        colors.specular = Color::Black();
        colors.emissive = Color::Black();
        colors.shininess = 85.0f;
        if (ColoredMaterial* colored = dynamic_cast<ColoredMaterial*>(material))
            colors = colored->GetColors();
        else if (ShadedMaterial* shaded = dynamic_cast<ShadedMaterial*>(material))
            colors = shaded->GetColors();
        return new ColoredMaterial(colors);
    }

    static void AddBox(Batch& batch, const Vector3& c, const Vector3& h, const Matrix3D& transform)
    {
        static const int kFaces[6][4] = {{0, 2, 3, 1}, {4, 5, 7, 6}, {0, 1, 5, 4},
                                         {2, 6, 7, 3}, {0, 4, 6, 2}, {1, 3, 7, 5}};
        static const Real32 kNormals[6][3] = {{-1, 0, 0}, {1, 0, 0}, {0, -1, 0},
                                              {0, 1, 0},  {0, 0, -1}, {0, 0, 1}};
        Vector3 corners[8];
        for (int i = 0; i < 8; ++i) {
            Vector3 local(c.x + ((i & 4) ? h.x : -h.x), c.y + ((i & 2) ? h.y : -h.y),
                          c.z + ((i & 1) ? h.z : -h.z));
            corners[i] = local * transform;
        }
        for (int f = 0; f < 6; ++f) {
            Vector3 n = Rotate(Vector3(kNormals[f][0], kNormals[f][1], kNormals[f][2]), transform);
            n.Normalize();
            UInt32 base = (UInt32)(batch.positions.size() / 3);
            for (int k = 0; k < 4; ++k) {
                const Vector3& p = corners[kFaces[f][k]];
                batch.positions.push_back(p.x);
                batch.positions.push_back(p.y);
                batch.positions.push_back(p.z);
                batch.normals.push_back(n.x);
                batch.normals.push_back(n.y);
                batch.normals.push_back(n.z);
            }
            static const UInt32 kTris[6] = {0, 1, 2, 0, 2, 3};
            for (int k = 0; k < 6; ++k)
                batch.indices.push_back(base + kTris[k]);
        }
    }

    void Build()
    {
        m_meshes.Clear();
        Render* render = m_control->GetRenderControlManager()->GetRender();
        std::unordered_map<Material*, size_t> index;
        std::vector<Batch> batches;
        for (BasicArray<BIBrick>::Iterator b = m_control->m_bricks.Begin(); b != m_control->m_bricks.End(); ++b) {
            Matrix3D bodyTransform;
            if (b->m_body != 0)
                b->m_body->GetTransform(bodyTransform);
            for (BasicArray<PlacedMesh>::Iterator m = b->m_meshes.Begin(); m != b->m_meshes.End(); ++m) {
                if (m->m_cullRadius < -1.5f)
                    ComputeCullSphere(*m);
                if (m->m_cullRadius < 0.0f || m->m_mesh.IsNull())
                    continue;
                Material* material = m->m_mesh->GetMaterial();
                std::unordered_map<Material*, size_t>::iterator it = index.find(material);
                if (it == index.end()) {
                    it = index.insert(std::make_pair(material, batches.size())).first;
                    batches.push_back(Batch());
                    batches.back().material = ProxyMaterial(material);
                }
                AddBox(batches[it->second], m->m_cullCenter, m->m_cullHalf,
                       Matrix3D(bodyTransform, m->m_transform));
            }
        }
        for (size_t i = 0; i < batches.size(); ++i) {
            Batch& batch = batches[i];
            UInt32 vertexCount = (UInt32)(batch.positions.size() / 3);
            UInt32 indexCount = (UInt32)batch.indices.size();
            VertexBuffer::CreateArgs args;
            args.vertexCount = vertexCount;
            args.indexCount = indexCount;
            args.frameCount = 0;
            args.dynamic = false;
            args.hasNormals = true;
            args.colorSetCount = 0;
            Object* object = render->GetObjectFactory()->CreateObject(typeid(VertexBuffer), args);
            VertexBuffer* buffer = object != 0 ? dynamic_cast<VertexBuffer*>(object) : 0;
            if (buffer == 0)
                continue;
            RefPointer<VertexBuffer> keep(buffer);
            buffer->SetPositions((const Vector3*)&batch.positions[0], 0, vertexCount, 0);
            buffer->SetNormals((const Vector3*)&batch.normals[0], 0, vertexCount);
            buffer->SetIndices(&batch.indices[0], 0, indexCount, 0);
            m_meshes.PushBack(RefPointer<Mesh>(new SingleMesh(batch.material, buffer)));
        }
    }

    BrickConnectSceneControl* m_control;
    bool m_enabled;
    UInt32 m_threshold;
    Int64 m_lastFullMs;
    Matrix3D m_view;
    bool m_haveView;
    bool m_shown;
    Int64 m_lastMove;
    BasicArray<RefPointer<Mesh> > m_meshes;
};

// =============================================================================================
// 021a prelude: the terrain
// =============================================================================================

// Mac 0x2d5c0, Win 0x61d5cf. The helpers it instantiates are the templates of the prelude:
// BasicArray<BasicArray<Bitmap> >::PushBack (Mac 0x2eed0), BasicArray<Bitmap>::PushBack
// (0x2f280), SingleMesh(Material*, VertexBuffer*) (0x2f3c0, Win 0x57c650),
// BasicArray<PlacedMesh>::PushBack (0x2f550); the destructors of the temporaries (CreateArgs
// 0x2f910, BasicArray<UInt32> 0x2f950, the bitmap arrays 0x2f250 / 0x2f980, LoadArgs 0x2f9e0).
bool BrickConnectSceneControl::LoadTerrain(const Path& heightMap, Texture* texture, Render* render,
                                           BrickConnect* brickConnect, BrickConnectSceneControl* control,
                                           const Matrix3D& transform)
{
    // the image file into a DataStream, decoded by the codec the ObjectFactory picks
    RefPointer<File> file = heightMap.OpenFile(OpenMode(true, false, true, false), false);
    if (file.IsNull())
        return false;
    UInt8* buffer = new UInt8[(UInt32)file->GetSize()];
    BitmapCodec::LoadArgs codecArgs(new DataStream(buffer, (UInt32)file->GetSize(), true));
    if (!file->Read(codecArgs.m_stream->GetBufferSize(), codecArgs.m_stream->GetBuffer(), 0))
        return false;
    RefPointer<BitmapCodec> codec;
    {
        Object* object = ObjectFactory::GetInstance()->CreateObject(typeid(BitmapCodec), codecArgs);
        codec = object ? dynamic_cast<BitmapCodec*>(object) : 0;
    }
    if (codec.IsNull())
        return false;
    // NOTE: the header is not checked for null (both builds).
    RefPointer<BitmapCodec::Header> header = codec->ReadHeader();
    BasicArray<BasicArray<Bitmap> > images;
    images.PushBack(BasicArray<Bitmap>());
    {
        Bitmap empty;
        empty.m_depth = 1;
        images[0].PushBack(empty);
    }
    Bitmap& bitmap = images[0][0];
    const BitmapCodec::Surface& surface = header->m_surfaces[0][0];
    bitmap.m_width = surface.m_width;
    bitmap.m_height = surface.m_height;
    bitmap.m_format = kBitmapFormatIndexed8;   // grey values through the palette
    bitmap.m_pitch = (Int32)bitmap.m_width;
    bitmap.m_data = new UInt8[bitmap.m_height * bitmap.m_width];
    if (Bitmap::IsIndexed(bitmap.m_format))
        bitmap.m_palette = new Color[1u << (Bitmap::GetBitsPerPixel(bitmap.m_format) & 0x1f)];
    if (!codec->Decode(images, 0)) {
        delete[] static_cast<UInt8*>(bitmap.m_data);
        delete[] bitmap.m_palette;
        return false;
    }
    delete[] bitmap.m_palette;

    // the heights: grey value - 127, row by row
    const UInt32 width = bitmap.m_width;
    const UInt32 height = bitmap.m_height;
    const UInt32 vertexCount = width * height;
    BasicArray<Real32> heights;
    heights.Reserve(vertexCount);
    for (UInt32 y = 0; y < height; ++y)
        for (UInt32 x = 0; x < width; ++x)
            heights.PushBack((Real32)static_cast<const UInt8*>(bitmap.m_data)[x + bitmap.m_pitch * y] + -127.0f);
    delete[] static_cast<UInt8*>(bitmap.m_data);

    // the physics: a height field (spacing 0.65, height scale 0.2) in a scenery element
    BrickPhysics* physics = brickConnect->GetPhysics();
    PhysicsGeometrySampledHeightField* field =
        physics->CreateSampledHeightField(UIntVector2(width, height), Vector3(0.65f, 0.2f, 0.65f), false,
                                          heights.Begin());
    if (field == 0)
        return false;
    PhysicsPlacedConfiguredGeometry placed;
    placed.m_geometry = field;
    placed.m_transform = Matrix3D();
    placed.m_type = 1;
    PhysicsSceneryElement* scenery = physics->CreateSceneryElement(&placed, 1, PhysicsMassProperties(0.0f, 0));
    if (scenery == 0) {
        field->Release();
        return false;
    }

    // the mesh: lit, textured, the grid of the height field's samples
    MaterialColors colors;
    colors.diffuse = Color(1.0f, 1.0f, 1.0f, 1.0f);
    colors.ambient = Color(0.1f, 0.1f, 0.1f, 1.0f);
    colors.specular = Color(0.1f, 0.1f, 0.1f, 1.0f);
    colors.emissive = Color(0.0f, 0.0f, 0.0f, 1.0f);
    colors.shininess = 0.5f;
    const UInt32 lastX = width - 1;
    const UInt32 lastY = height - 1;
    const UInt32 indexCount = lastX * lastY * 6;
    VertexBuffer::CreateArgs args;
    args.vertexCount = vertexCount;
    args.indexCount = indexCount;
    args.hasNormals = true;
    args.texCoordDimensions.PushBack(2);
    RefPointer<VertexBuffer> buffer2;
    {
        Object* object = render->GetObjectFactory()->CreateObject(typeid(VertexBuffer), args);
        buffer2 = object ? dynamic_cast<VertexBuffer*>(object) : 0;
    }
    if (buffer2.IsNull()) {
        field->Release();
        scenery->Release();
        return false;
    }
    buffer2->SetPrimitiveType(3);   // triangles
    Vector3* positions = new Vector3[vertexCount];
    Vector3* normals = new Vector3[vertexCount];
    Vector2* texCoords = new Vector2[vertexCount];
    UInt32* indices = new UInt32[indexCount];

    // positions: (x, height, y) scaled like the height field
    const Vector3 scale = field->GetScale();
    {
        Vector3* p = positions;
        for (UInt32 y = 0; y < height; ++y)
            for (UInt32 x = 0; x < width; ++x, ++p) {
                Vector3 v((Real32)x, field->GetHeight(UIntVector2(x, y)), (Real32)y);
                v.Scale(scale.x, scale.y, scale.z);
                *p = v;
            }
    }
    // normals: the sum of the normals of the (up to six) adjacent triangles, normalised
    {
        Vector3* n = normals;
        for (UInt32 y = 0; y < height; ++y) {
            const Vector3* above = positions + (y - 1) * width;   // row y - 1 (unused for y == 0)
            const Vector3* row = positions + y * width;
            const Vector3* below = positions + (y + 1) * width;   // row y + 1 (unused for the last row)
            for (UInt32 x = 0; x < width; ++x, ++n) {
                Vector3 sum;
                Vector3 normal;
                if (y != 0 && x != 0) {
                    normal.SetNormal(above[x], row[x], row[x - 1]);
                    sum += normal;
                }
                if (!(x == lastX || y == 0)) {
                    normal.SetNormal(above[x], above[x + 1], row[x]);
                    sum += normal;
                    normal.SetNormal(above[x + 1], row[x + 1], row[x]);
                    sum += normal;
                }
                if (y != lastY && x != 0) {
                    normal.SetNormal(row[x - 1], row[x], below[x - 1]);
                    sum += normal;
                    normal.SetNormal(row[x], below[x], below[x - 1]);
                    sum += normal;
                }
                if (!(x == lastX || y == lastY)) {
                    normal.SetNormal(row[x], row[x + 1], below[x]);
                    sum += normal;
                }
                sum.Normalize();
                *n = sum;
            }
        }
    }
    // texture coordinates: the map twice in both directions
    {
        const Vector2 repeat(2.0f, 2.0f);
        Vector2* t = texCoords;
        for (UInt32 y = 0; y < height; ++y)
            for (UInt32 x = 0; x < width; ++x, ++t) {
                Vector2 uv((Real32)x / (Real32)width, (Real32)y / (Real32)height);
                uv.Scale(repeat.x, repeat.y);
                *t = uv;
            }
    }
    // two triangles per grid cell (Mac: unrolled by two)
    {
        UInt32* i = indices;
        for (UInt32 y = 0; y < lastY; ++y)
            for (UInt32 x = 0; x < lastX; ++x) {
                const UInt32 a = y * width + x;
                const UInt32 b = (y + 1) * width + x;
                *i++ = a;
                *i++ = b;
                *i++ = a + 1;
                *i++ = a + 1;
                *i++ = b;
                *i++ = b + 1;
            }
    }
    buffer2->SetPositions(positions, 0, vertexCount, 0);
    buffer2->SetNormals(normals, 0, vertexCount);
    buffer2->SetTexCoords(reinterpret_cast<const Real32*>(texCoords), 0);
    buffer2->SetIndices(indices, 0, indexCount, 0);

    BasicArray<PlacedMesh> meshes;
    {
        RefPointer<Mesh> mesh = new SingleMesh(new TexturedMaterial(colors, texture), buffer2);
        meshes.PushBack(PlacedMesh(mesh, Vector3(), Matrix3D(), 0, 1.0f));
    }
    control->AddSceneryElement(scenery, transform, 2, meshes);
    scenery->Release();
    field->Release();
    delete[] indices;
    delete[] texCoords;
    delete[] normals;
    delete[] positions;
    return true;
}

// =============================================================================================
// construction
// =============================================================================================

// Mac 0x30620, Win 0x61e8f7
BrickConnectSceneControl::BrickConnectSceneControl(RenderControlManager* manager,
                                                   BrickConnectSceneManager* sceneManager,
                                                   UndoManager* undoManager, BrickPalette* palette)
    : BufferedAbleRenderControl(manager),
      RenderVisualDebugger(manager->GetRender(), sceneManager->GetSilhouetteMaterial()),
      m_gridVisible(true), m_field234(0),
      m_modified(false), m_loading(false), m_invertedCameraX(false), m_invertedCameraY(false),
      m_pressedButtons(0), m_leftButtonDown(false), m_zoomToSelection(false),
      m_sceneManager(sceneManager), m_field27c(0), m_undoManager(undoManager),
      m_developerMode(false), m_cullingFlags(0xf), m_showStatistics(true), m_countTriangles(true),
      m_showOverlay(false), m_onlySelected(false), m_field3c0(false), m_debugFlag3c1(false),
      m_renderMode(0), m_debugMode3c8(0), m_field3cc(1.0f), m_wireframe(false),
      m_pressedElement(0), m_showSelectionBoxes(false), m_showMissingBricks(false),
      m_field461(false), m_field508(0)
{
    m_meshBufferCache = 0;
    m_navigation = 0;
    m_lodEnabled = false;
    m_lodBoxes = false;
    m_lodReducedDistance2 = 0.0f;
    m_lodBoxDistance2 = 0.0f;
    m_undoManager->AddRef();
    // The lights (param_1[0xa1..0xeb]): type 0, colours from Color() (white), zero vectors, every
    // attenuation / spot value 0, no camera.
    for (UInt32 i = 0; i < 3; ++i) {
        Light& light = m_lights[i].m_light;
        light.type = Light::kDirectional;
        light.diffuse = Color();
        light.specular = Color();
        light.ambient = Color();
        light.position = Vector3();
        light.direction = Vector3();
        light.constantAttenuation = 0.0f;
        light.linearAttenuation = 0.0f;
        light.quadraticAttenuation = 0.0f;
        light.spotCutoff = 0.0f;
        light.spotExponent = 0.0f;
        light.reserved60 = 0;
    }
    PreferencesObserver::Attach(Preferences::GetInstance());
    ReadBoolPreference(String("DEVELOPERMODE"), m_developerMode);
    ReadBoolPreference(String("ZOOMTOSELECTION"), m_zoomToSelection);
    UndoManagerObserver::Attach(undoManager);
    if (palette != 0)   // NOTE: Observer::Attach(0) is a no-op in the original (subject unchanged)
        BrickPaletteObserver::Attach(palette);
    // the 14 wire colours, all free
    static Color (*const kWireColors[14])() = {
        &Color::Green, &Color::Red, &Color::Yellow, &Color::Blue, &Color::Black, &Color::White,
        &Color::Cyan, &Color::DarkGray, &Color::Gray, &Color::LightGray, &Color::Magenta,
        &Color::Orange, &Color::Purple, &Color::Brown};
    for (UInt32 i = 0; i < 14; ++i)
        m_wireColors.PushBack(Pair<Color, bool>(kWireColors[i](), false));
    m_buildingInstructions = new BI;
    m_loadedBuildingInstructions = new BI;
    ReadBoolPreference(String("INVERTEDCAMERAX"), m_invertedCameraX);
    ReadBoolPreference(String("INVERTEDCAMERAY"), m_invertedCameraY);
    m_navigation = new NavigationProxy(this);
    ReadLodPreferences(false);
}

// Mac 0x44560 / 0x44940 (+ thunks 0x445c0-0x448d0, 0x449b0-0x44d30; the member destruction is
// the base-object destructor 0x43260), Win sdd 0x621f82
BrickConnectSceneControl::~BrickConnectSceneControl()
{
    delete m_meshBufferCache;   // (the bodies' meshes hold their buffers themselves)
    delete m_navigation;
    if (m_undoManager)
        m_undoManager->Release();
    delete m_thread;   // (ModelLoaderHandler's thread, normally gone after OnLoadFinished)
    m_thread = 0;
}

// Mac 0x44db0, Win 0x61c2e9
void BrickConnectSceneControl::OnFinalRelease()
{
    while (!m_sceneryElements.IsEmpty())
        RemoveSceneryElement(m_sceneryElements[0].m_object);
    BrickPaletteObserver::OnFinalRelease();
    ModelLoaderHandler::OnFinalRelease();
    PreferencesObserver::OnFinalRelease();
    UndoManagerObserver::OnFinalRelease();
    SceneObserver::OnFinalRelease();
    ControlNotifier::OnFinalRelease();
    Control::OnFinalRelease();
}

RenderControlManager* BrickConnectSceneControl::GetRenderControlManager() const
{
    return static_cast<RenderControlManager*>(GetManager());
}

void BrickConnectSceneControl::Redraw()
{
    if (!GetManager()->IsUpdateLocked())
        Invalidate(false);
}

// Mac 0x32b90, Win 0x620991
bool BrickConnectSceneControl::Initialize(Scene* scene)
{
    SetScene(scene);
    // the terrain (preferences TerrainHeightMap / TerrainTexture in the assets directory)
    String heightMap;
    String terrainTexture;
    RefPointer<Directory> assets =
        Platform::GetInstance()->GetSystemDirectory(Platform::kAssetsDirectory, true);
    if (!assets.IsNull()) {
        bool haveTerrain = false;
        {
            RefPointer<Preferences> preferences = Preferences::GetInstance();
            if (preferences->GetValue(String("TerrainHeightMap"), heightMap)) {
                RefPointer<Preferences> preferences2 = Preferences::GetInstance();
                haveTerrain = preferences2->GetValue(String("TerrainTexture"), terrainTexture);
            }
        }
        if (haveTerrain) {
            Texture::LoadArgs2D args;
            args.path.SetElement(assets);
            args.path.Append(terrainTexture);
            args.create.wrapMode = 0;
            args.create.alphaMode = 0;
            args.create.mipmapMode = 2;
            Render* render = GetRenderControlManager()->GetRender();
            Object* object = render->GetObjectFactory()->CreateObject(typeid(Texture), args);
            Texture* texture = object ? dynamic_cast<Texture*>(object) : 0;
            if (texture != 0) {
                RefPointer<Texture> held = texture;
                Path path;
                path.SetElement(assets);
                path.Append(heightMap);
                LoadTerrain(path, texture, GetRenderControlManager()->GetRender(),
                            m_sceneManager->GetBrickConnect(), this, Matrix3D());
            }
        }
    }
    InitializeLights();

    Render* render = GetRenderControlManager()->GetRender();
    m_dynamicGrid = new DynamicGrid;
    if (!m_dynamicGrid->Initialize(render))
        return false;
    m_noneGrid = new NoneGrid;
    if (!m_noneGrid->Initialize(GetRenderControlManager()->GetRender()))
        return false;
    m_grid = m_dynamicGrid;
    m_selectionMaterial = m_sceneManager->GetSilhouetteMaterial();
    SetClearColor(Color(0.8f, 0.8f, 0.8f, 1.0f));
    SetInputFlags(m_inputFlags | 3);
    SetShown(true);
    SetEnabled(true);
    SetActive(true);
    m_camera = NewViewCamera();
    ResetCamera();
    m_camera->SetViewportSize(m_size.x, m_size.y);

    UInt32 effectIndex = 0;
    ReadUInt32Preference(String("SelectedPostProcessingEffect"), effectIndex);
    const BasicArray<RefPointer<PostProcessingEffect> >& effects =
        LDDApplication::GetInstance()->GetPostProcessingEffects();
    UInt32 count = effects.GetSize();
    if (effectIndex < count)
        count = effectIndex;
    SetPostProcessor(count == 0 ? 0 : effects[count - 1].Get());
    m_postProcessorControl = new LDDPostProcessor(this, GetRenderControlManager());
    m_postProcessorControl->Initialize();
    return true;
}

// The lights / light cameras of Initialize (Mac 0x32ed2-0x333c1).
void BrickConnectSceneControl::InitializeLights()
{
    static const Real32 kGrey[3] = {0.7f, 0.65f, 0.3f};
    const Vector3 directions[3] = {Vector3(1.01f, 1.01f, 1.01f), Vector3(-1.01f, 1.01f, -1.01f),
                                   Vector3(0.501f, -1.01f, -1.01f)};
    for (UInt32 i = 0; i < 3; ++i) {
        Light& light = m_lights[i].m_light;
        light.diffuse = Color(kGrey[i], kGrey[i], kGrey[i], 1.0f);
        light.specular = Color(kGrey[i], kGrey[i], kGrey[i], 1.0f);
        light.direction = directions[i];
        light.type = Light::kDirectional;
        m_lights[i].m_camera = NewViewCamera();
        m_lights[i].m_camera->SetLookAt(light.direction, Vector3(0.0f, 0.0f, 0.0f),
                                        Vector3(0.0f, 1.0f, 0.0f));
    }
}

// HOST-ONLY (see the header): the lights and the view camera of Initialize (Mac 0x335db-0x3365d)
// without the grids / post-processor.
void BrickConnectSceneControl::InitializeLightsAndCamera()
{
    InitializeLights();
    m_camera = NewViewCamera();
    ResetCamera();
    m_camera->SetViewportSize(m_size.x, m_size.y);
}

namespace {

// Removes `*it` by moving the last element into its place (Mac 0x58bc0 / 0x50b70: the
// unordered erase of BasicArray; nothing happens for End()).
template <class T>
void EraseUnordered(BasicArray<T>& array, typename BasicArray<T>::Iterator it)
{
    if (it == array.End())
        return;
    typename BasicArray<T>::Iterator last = array.End() - 1;
    if (last != it)
        *it = *last;
    array.PopBack();
}

// The elements as raw pointers (Scene's static transform helpers take BasicArray<Element*>;
// the original passes the RefPointer array's memory, which has the same layout).
void ToRawElements(const BasicArray<RefPointer<PrimitiveElement> >& elements,
                   BasicArray<PrimitiveElement*>& raw)
{
    raw.Reserve(elements.GetSize());
    for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator it = elements.Begin(); it != elements.End(); ++it)
        raw.PushBack(it->Get());
}

} // namespace

// =============================================================================================
// scene
// =============================================================================================

// Mac 0x339d0
void BrickConnectSceneControl::SetScene(Scene* scene)
{
    if (scene == 0)
        scene = m_sceneManager->GetBrickConnect()->NewScene(0);
    m_scene = scene;
    Scene* current = m_scene;
    current->m_flag124 = true;
    current->m_flag120 = true;
    current->m_flag121 = true;
    current->m_flag122 = true;
    current->m_flag123 = true;
    current->SetListener(static_cast<VisualDebugger*>(this));
    SceneObserver::Attach(current);
    UpdateMissingBricks();
    BCSC_NOTIFY(OnEvent08());
}

// Mac 0x380e0, Win 0x6133ff
bool BrickConnectSceneControl::IsSceneEmpty() const
{
    return m_scene->GetBricks().IsEmpty();
}

// Mac 0x341f0
void BrickConnectSceneControl::ClearScene()
{
    ShowAllBricks();
    m_sceneManager->SetActiveTool(0);
    ClearSelection();
    m_selectedParts.Clear();
    m_field400.Clear();
    m_flexParts.Clear();
    m_bricks.Clear();
    m_loadedParts.Clear();
    m_loadingStorage = 0;
    m_pressedElement = 0;
    m_scene->Clear();
    m_wires.Clear();
    for (BasicArray<Pair<Color, bool> >::Iterator it = m_wireColors.Begin(); it != m_wireColors.End(); ++it)
        it->second = false;
    UpdateSelectionMesh();
    ResetCamera();
    m_buildingInstructions->Reset();
    m_loadedBuildingInstructions->Reset();
    UndoManagerObserver::GetSubject()->Clear();
    m_modified = false;
    UndoManagerObserver::GetSubject()->Clear();
    NotifyUndoState();
    BCSC_NOTIFY(OnEvent12());
}

// Mac 0x33e90 (thunk 0x346a0)
void BrickConnectSceneControl::OnLoadStarted(StorageScene2* storage)
{
    BCSC_NOTIFY(OnEvent10(storage));
    if (m_thread != 0) {
        delete m_thread;
        m_thread = 0;
    }
    ClearScene();
    m_scene->BeginUpdate();
    m_loading = true;
    m_loadingStorage = storage;
    m_thread = new ModelLoaderProgressBarThread();
    m_thread->Start(m_sceneManager, m_scene, storage, BrickPaletteObserver::GetSubject(),
                    &m_loadedParts, m_loadedBuildingInstructions);
}

// Mac 0x346c0 (thunk 0x35420)
void BrickConnectSceneControl::OnLoadFinished()
{
    const bool succeeded = m_thread->Succeeded();
    if (!succeeded) {
        ClearScene();
        Application::GetInstance()->CloseDocument();
    } else {
        for (BasicArray<RefPointer<Part> >::Iterator it = m_loadedParts.Begin(); it != m_loadedParts.End(); ++it)
            AddFlexPart(*it);
        m_loadedParts.Clear();
        const BasicArray<RefPointer<Brick> >& bricks = m_scene->GetBricks();
        for (UInt32 i = 0; i < bricks.GetSize(); ++i)
            AddWire(bricks[i]);
        SetCameraFromStorage(*m_loadingStorage);
        m_sceneManager->Load(m_thread->GetLazyLoadData());
        RefPointer<BIStorage> steps = m_loadedBuildingInstructions->GetStorage();
        m_buildingInstructions->SetStorage(steps);
        m_buildingInstructions->SetName(m_loadedBuildingInstructions->GetName());
    }
    m_scene->EndUpdate();
    RefPointer<StorageScene2> storage = m_loadingStorage;
    m_loading = false;
    m_loadingStorage = 0;
    BCSC_NOTIFY(OnEvent11(storage, succeeded));
}

// Mac 0x35da0
void BrickConnectSceneControl::RemoveBrick(Brick* brick)
{
    BasicArray<RefPointer<Brick> > bricks;
    bricks.PushBack(brick);
    RemoveBricks(bricks);
}

// Mac 0x36040
void BrickConnectSceneControl::RemoveBricks(const BasicArray<RefPointer<Brick> >& bricks)
{
    if (bricks.IsEmpty())
        return;
    UndoManagerObserver::GetSubject()->AddAction(new UndoRemoveSelection(this, bricks));
    m_scene->BeginUpdate();
    m_scene->Remove(bricks);
    const UInt32 selectionBefore = m_selection.GetSize();
    for (BasicArray<RefPointer<Brick> >::ConstIterator it = bricks.Begin(); it != bricks.End(); ++it)
        OnBrickRemoved(*it);
    if (selectionBefore != 0 && m_selection.IsEmpty())
        BCSC_NOTIFY(OnEvent14(true));
    if (selectionBefore != m_selection.GetSize())
        BCSC_NOTIFY(OnEvent15(m_selection));
    UpdateSelectionMesh();
    m_scene->EndUpdate();
}

// Mac 0x366a0
void BrickConnectSceneControl::RemoveParts(const BasicArray<RefPointer<Part> >& parts)
{
    BasicArray<RefPointer<Brick> > bricks;
    Scene::GetBricks(parts, bricks);
    RemoveBricks(bricks);
}

// Mac 0x367a0
void BrickConnectSceneControl::RemoveSelection()
{
    RemoveParts(m_selection);
}

// Mac 0x367c0
bool BrickConnectSceneControl::CanConnect(const BasicArray<RigidElement*>& bodies,
                                          Scene::ConnectResult& result)
{
    return m_scene->CanConnect(bodies, result);
}

// Mac 0x367f0
void BrickConnectSceneControl::Reconnect(const Scene::ConnectResult& result)
{
    m_scene->Reconnect(result);
}

// Mac 0x36810, Win 0x61c1d9
void BrickConnectSceneControl::MoveParts(const BasicArray<RefPointer<Part> >& parts, Scene* target)
{
    BasicArray<RefPointer<Part> > expanded;
    Scene::ExpandParts(parts, expanded, true, false, true);
    if (!expanded.IsEmpty()) {
        ClearSelection();
        BasicArray<RefPointer<PrimitiveElement> > elements;
        Scene::GetElements(expanded, elements);
        SetWiresSelected(elements, false);
        m_scene->MoveParts(expanded, target);
    }
}

// Mac 0x370c0
void BrickConnectSceneControl::MergeScene(Scene* other, const Scene::ConnectResult& result, bool insert)
{
    if (!insert) {
        InsertScene(other, result);
        return;
    }
    const BasicArray<RefPointer<PrimitiveElement> >& elements = other->GetPrimitiveElements();
    Matrix3D transform(result.m_transform);
    if (!transform.IsEqual(Matrix3D(), 1e-5f)) {
        UndoManager* undo = UndoManagerObserver::GetSubject();
        UndoMoveSnapshot* snapshot = new UndoMoveSnapshot(this, BasicArray<Pair<PrimitiveElement*, Matrix3D> >(), false);
        BasicArray<PrimitiveElement*> raw;
        ToRawElements(elements, raw);
        Scene::GetTransforms(raw, snapshot->m_transforms);
        undo->AddAction(snapshot);
    }
    SetWiresSelected(elements, true);
    m_scene->Merge(other, result);
}

// Mac 0x37250
void BrickConnectSceneControl::InsertScene(Scene* other, const Scene::ConnectResult& result)
{
    m_scene->BeginUpdate();
    UndoManagerObserver::GetSubject()->AddAction(new UndoInsertSelection(this, other->GetBricks()));
    const BasicArray<RefPointer<Brick> >& bricks = other->GetBricks();
    for (UInt32 i = 0; i < bricks.GetSize(); ++i) {
        Brick* brick = bricks[i];
        const BasicArray<RefPointer<Part> >& parts = brick->GetParts();
        for (UInt32 p = 0; p < parts.GetSize(); ++p)
            AddFlexPart(parts[p]);
        AddWire(brick);
    }
    m_scene->MergeScene(other, result);
    m_scene->EndUpdate();
}

// Mac 0x37340
void BrickConnectSceneControl::CopyParts(const BasicArray<RefPointer<Part> >& parts, Scene* target,
                                         Scene::CopyMap& map, bool copyGroups, UInt32 flags)
{
    m_scene->CopyParts(parts, target, map, copyGroups, flags);
}

// Mac 0x4d7f0, Win 0x620625
void BrickConnectSceneControl::InsertBricks(const BasicArray<RefPointer<Brick> >& bricks)
{
    m_scene->BeginUpdate();
    BasicArray<RefPointer<Brick> > failed;
    m_scene->BatchInsert(bricks, failed, 0, false, 0);
    for (UInt32 i = 0; i < bricks.GetSize(); ++i) {
        Brick* brick = bricks[i];
        const BasicArray<RefPointer<Part> >& parts = brick->GetParts();
        for (UInt32 p = 0; p < parts.GetSize(); ++p)
            AddFlexPart(parts[p]);
        AddWire(brick);
    }
    UndoManagerObserver::GetSubject()->AddAction(new UndoInsertSelection(this, bricks));
    m_scene->EndUpdate();
}

// Mac 0x374a0
void BrickConnectSceneControl::ShowAllBricks()
{
    m_scene->BeginUpdate();
    const BasicArray<RefPointer<Brick> >& bricks = m_scene->GetBricks();
    for (UInt32 i = 0; i < bricks.GetSize(); ++i)
        bricks[i]->SetHidden(false);
    m_scene->EndUpdate();
    BCSC_NOTIFY(OnEvent22(0));
}

// Mac 0x4d660
void BrickConnectSceneControl::OnBrickRemoved(Brick* brick)
{
    brick->SetHidden(false);
    RemoveWire(brick);
    const BasicArray<RefPointer<Part> >& parts = brick->GetParts();
    for (BasicArray<RefPointer<Part> >::ConstIterator it = parts.Begin(); it != parts.End(); ++it) {
        Part* part = *it;
        if (part->m_elements.GetSize() > 1) {
            LDDPart* flexPart = static_cast<LDDPart*>(part);
            BasicArray<LDDPart*>::Iterator f = m_flexParts.Begin();
            while (f != m_flexParts.End() && *f != flexPart)
                ++f;
            EraseUnordered(m_flexParts, f);
        }
        RefPointer<Part> held = part;
        BasicArray<RefPointer<Part> >::Iterator s = m_selection.Begin();
        while (s != m_selection.End() && s->Get() != part)
            ++s;
        EraseUnordered(m_selection, s);
    }
}

// Mac 0x38ef0
void BrickConnectSceneControl::GetTransforms(BasicArray<Pair<PrimitiveElement*, Matrix3D> >& transforms) const
{
    m_scene->GetTransforms(transforms);
}

// Mac 0x38f10
void BrickConnectSceneControl::GetChangedTransforms(const BasicArray<Pair<PrimitiveElement*, Matrix3D> >& transforms,
                                                    BasicArray<Pair<PrimitiveElement*, Matrix3D> >& changed) const
{
    Scene::GetChangedTransforms(transforms, changed, 0);
}

// Mac 0x38f40, Win 0x6174bf
void BrickConnectSceneControl::SetTransforms(const BasicArray<Pair<PrimitiveElement*, Matrix3D> >& transforms,
                                             bool undo)
{
    if (undo)
        UndoManagerObserver::GetSubject()->AddAction(new UndoMoveSnapshot(this, transforms, true));
    m_scene->SetTransforms(transforms);
    UpdateSelectionMesh();
    Redraw();
}

// =============================================================================================
// selection
// =============================================================================================

// Mac 0x36a40
void BrickConnectSceneControl::ClearSelection()
{
    if (!m_selection.IsEmpty()) {
        m_selection.Clear();
        UpdateSelectionMesh();
        BCSC_NOTIFY(OnEvent14(true));
    }
    BCSC_NOTIFY(OnEvent15(m_selection));
}

// Mac 0x37780
void BrickConnectSceneControl::UpdateSelectionMesh()
{
    Redraw();
    if (!m_showSelectionBoxes)
        return;
    m_selectionBuffer = 0;
    m_selectionMesh = 0;
    if (m_selection.IsEmpty())
        return;
    BasicArray<Matrix3D> boxes;
    boxes.Reserve(m_selection.GetSize());
    for (BasicArray<RefPointer<Part> >::Iterator it = m_selection.Begin(); it != m_selection.End(); ++it) {
        Matrix3D box;
        GetPartBoxTransform(*it, box, false);
        boxes.PushBack(box);
    }
    Render* render = GetRenderControlManager()->GetRender();
    const UInt32 vertexCount = boxes.GetSize() * 8;
    const UInt32 indexCount = boxes.GetSize() * 24;
    VertexBuffer::CreateArgs args;
    args.vertexCount = vertexCount;
    args.indexCount = indexCount;
    args.frameCount = 0;
    args.dynamic = false;
    args.hasNormals = false;
    args.colorSetCount = 1;
    Object* object = render->GetObjectFactory()->CreateObject(typeid(VertexBuffer), args);
    m_selectionBuffer = object ? dynamic_cast<VertexBuffer*>(object) : 0;
    m_selectionBuffer->SetPrimitiveType(1);

    // the eight corners of the unit box through each box transform
    Vector3* positions = new Vector3[vertexCount];
    UInt32* indices = new UInt32[indexCount];
    static const Real32 kCorners[8][3] = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
                                          {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
    Vector3* p = positions;
    for (BasicArray<Matrix3D>::Iterator b = boxes.Begin(); b != boxes.End(); ++b, p += 8)
        for (UInt32 c = 0; c < 8; ++c)
            p[c] = Vector3(kCorners[c][0], kCorners[c][1], kCorners[c][2]) * *b;
    m_selectionBuffer->SetPositions(positions, 0, vertexCount, 0);
    delete[] positions;
    // the twelve edges
    static const UInt32 kEdges[24] = {0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4,
                                      0, 4, 1, 5, 2, 6, 3, 7};
    for (UInt32 b = 0; b < boxes.GetSize(); ++b)
        for (UInt32 e = 0; e < 24; ++e)
            indices[b * 24 + e] = b * 8 + kEdges[e];
    m_selectionBuffer->SetIndices(indices, 0, indexCount, 0);
    delete[] indices;
    Color* colors = new Color[vertexCount];
    const Color cyan = Color::Cyan();
    for (UInt32 i = 0; i < vertexCount; ++i)
        colors[i] = cyan;
    m_selectionBuffer->SetColors(colors, 0, vertexCount, 0);
    delete[] colors;
    m_selectionMesh = new SingleMesh(m_selectionMaterial, m_selectionBuffer);
}

// =============================================================================================
// wires
// =============================================================================================

// Mac 0x34e90, Win 0x61e41c
void BrickConnectSceneControl::AddWire(Brick* brick)
{
    if (!Scene::IsWire(brick->GetDesignID()))
        return;
    {
        RefPointer<const Brick> key = brick;
        if (m_wires.Find(key) != m_wires.End())
            return;
    }
    Color color;
    bool found = false;
    for (BasicArray<Pair<Color, bool> >::Iterator it = m_wireColors.Begin(); it != m_wireColors.End(); ++it) {
        if (!it->second) {
            it->second = true;
            color = it->first;
            found = true;
            break;
        }
    }
    if (!found) {
#if defined(_WIN32)
        const Real32 b = RandomUnit();
        const Real32 g = RandomUnit();
        const Real32 r = RandomUnit();
#else
        const Real32 r = RandomUnit();
        const Real32 g = RandomUnit();
        const Real32 b = RandomUnit();
#endif
        color = Color(r, g, b, 1.0f);
    }
    MaterialColors colors;
    colors.diffuse = color;
    colors.ambient = Color::Black();
    colors.specular = Color::Black();
    colors.emissive = Color::Black();
    colors.shininess = 0.0f;
    RefPointer<Material> material = new ColoredMaterial(colors);
    SingleMesh* mesh = CreateTriangleMesh(GetRenderControlManager()->GetRender(), color, material);
    RefPointer<WireData> wire = new WireData(color, true, mesh);
    m_wires[RefPointer<const Brick>(brick)] = wire;
}

// Mac 0x4f420
void BrickConnectSceneControl::RemoveWire(Brick* brick)
{
    if (!Scene::IsWire(brick->GetDesignID()))
        return;
    BasicMap<RefPointer<const Brick>, RefPointer<WireData> >::Iterator it =
        m_wires.Find(RefPointer<const Brick>(brick));
    if (it == m_wires.End())
        return;
    for (BasicArray<Pair<Color, bool> >::Iterator c = m_wireColors.Begin(); c != m_wireColors.End(); ++c) {
        if (c->first == it->second->m_color) {
            c->second = false;
            break;
        }
    }
    m_wires.Erase(it);
}

// Mac 0x36fd0, Win 0x61badf
void BrickConnectSceneControl::SetWiresSelected(const BasicArray<RefPointer<PrimitiveElement> >& elements,
                                                bool selected)
{
    for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator it = elements.Begin(); it != elements.End(); ++it) {
        Brick* brick = (*it)->m_part->GetBrick();
        if (!Scene::IsWire(brick->GetDesignID()))
            continue;
        BasicMap<RefPointer<const Brick>, RefPointer<WireData> >::Iterator w =
            m_wires.Find(RefPointer<const Brick>(brick));
        if (w != m_wires.End())
            w->second->m_selected = selected;
    }
}

// Mac 0x4f220, Win 0x61a038
void BrickConnectSceneControl::SetAllWiresSelected(bool selected)
{
    for (BasicMap<RefPointer<const Brick>, RefPointer<WireData> >::Iterator it = m_wires.Begin(); it != m_wires.End(); ++it)
        it->second->m_selected = selected;
}

// Mac 0x4acd0 (Win not identified; 0x61d5cf is LoadTerrain)
void BrickConnectSceneControl::QueueWireMeshes(RenderQueue* queue)
{
    const Real32 scale = 0.65f;
    for (BasicMap<RefPointer<const Brick>, RefPointer<WireData> >::Iterator it = m_wires.Begin(); it != m_wires.End(); ++it) {
        WireData* wire = it->second;
        if (!wire->m_selected)
            continue;
        const Brick* brick = it->first;
        const BasicArray<RefPointer<Part> >& parts = brick->GetParts();
        for (BasicArray<RefPointer<Part> >::ConstIterator p = parts.Begin(); p != parts.End(); ++p) {
            Part* part = *p;
            Matrix3D marker;
            if (!ConnectivityWorldLookupTables::GetWireMarker(part->GetDesignID(), marker))
                continue;
            // the marker's rotation scaled by 0.65
            for (UInt32 row = 0; row < 3; ++row)
                for (UInt32 column = 0; column < 3; ++column)
                    marker.m[row * 4 + column] *= scale;
            for (UInt32 e = 0; e < part->m_elements.GetSize(); ++e) {
                Matrix3D elementTransform;
                part->m_elements[e]->GetTransform(elementTransform);
                Matrix3D transform(elementTransform, marker);
                queue->Add(wire->m_mesh, transform, 0.0f, 0, 1.0f);
            }
        }
    }
}

// =============================================================================================
// undo / modified
// =============================================================================================

// Mac 0x380b0
void BrickConnectSceneControl::SetModified(bool modified)
{
    m_modified = modified;
    if (!modified)
        UndoManagerObserver::GetSubject()->Clear();
    NotifyUndoState();
}

// Mac 0x43130, Win 0x612cea
bool BrickConnectSceneControl::IsModified() const
{
    if (m_modified)
        return true;
    return UndoManagerObserver::GetSubject()->CanUndo();
}

// Mac 0x42e80, Win 0x6184ae
void BrickConnectSceneControl::NotifyUndoState()
{
    ControlNotifier::Iterator it(*this);
    while (BrickConnectSceneControlObserver* observer = it.Next(ControlNotifier::m_observers))
        observer->OnEvent13(IsModified());
}

// Mac 0x4c020 (thunk 0x4c030)
void BrickConnectSceneControl::OnUndoAvailable(bool)
{
    NotifyUndoState();
}

// Mac 0x4c050 (thunk 0x4c060)
void BrickConnectSceneControl::OnRedoAvailable(bool)
{
}

// Mac 0x4c580 (thunk 0x4c5c0), Win 0x6184bc
void BrickConnectSceneControl::OnAttached(UndoManager* undoManager)
{
    m_modified = undoManager->CanUndo();
    if (!m_modified)
        UndoManagerObserver::GetSubject()->Clear();
    NotifyUndoState();
}

// =============================================================================================
// camera
// =============================================================================================

// Mac 0x39b60, Win 0x615222
void BrickConnectSceneControl::FitCamera(Camera* camera, Real32 width, Real32 height)
{
    if (m_scene->IsEmpty()) {
        camera->SetPosition(Vector3(40.0f, 40.0f, 40.0f));
        camera->SetTarget(Vector3(0.0f, 0.0f, 0.0f));
        return;
    }
    // the points as spheres in one array of their size (Scene::GetSceneCVSpheres)
    BasicArray<Sphere> spheres;
    m_scene->GetSceneCVSpheres(spheres, 0.025f);
    if (spheres.IsEmpty())
        return;
    BoundingBox box(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    m_scene->GetBoundingBox(box, true);
    Sphere bounds(box);
    camera->FitToSpheres(bounds.m_center, spheres, width, height);
}

// Mac 0x39d10 (and inline at its other call sites)
void BrickConnectSceneControl::UpdateGrid(const BoundingBox* extraBox)
{
    BoundingBox box(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    m_scene->GetBoundingBox(box, true);
    if (m_grid.IsNull())
        return;   // NOTE: HOST-ONLY guard (see the header)
    const Vector3& position = m_camera->GetPosition();
    Vector3 view = m_camera->GetPosition() - m_camera->GetTarget();
    m_grid->Update(position, view, box, extraBox, true);
}

// Mac 0x33da0, Win 0x61769a (+ 0x6137c8)
void BrickConnectSceneControl::ResetCamera()
{
    FitCamera(m_camera, m_size.x, m_size.y);
    m_camera->Update();
    UpdateGrid(0);
}

// Mac 0x352a0, Win 0x618139
void BrickConnectSceneControl::SetCameraFromStorage(const StorageScene2& storage)
{
    const StorageScene2::Camera* stored = storage.m_camera;
    if (stored == 0) {
        ResetCamera();
        return;
    }
    Matrix3D transformation(stored->m_transformation);
    Vector3 position(transformation.GetTranslation());
    // a distance below 0.5 gives the default 10
    Real32 distance = stored->m_distance < 0.5f ? -10.0f : -stored->m_distance;
    Vector3 forward(0.0f, 0.0f, distance);
    Vector3 target = forward * stored->m_transformation;
    Vector3 up;
    UInt32 type = m_camera->GetType();
    if (type == 1 || type == 2)
        up = Vector3(0.0f, 1.0f, 0.0f);
    else
        up = Rotate(Vector3(0.0f, 1.0f, 0.0f), stored->m_transformation);
    m_camera->SetLookAt(position, target, up, stored->m_fieldOfView);
}

// Mac 0x426c0, Win 0x6145dd
void BrickConnectSceneControl::RotateCamera(Real32 angle)
{
    if (m_invertedCameraX)
        angle = -angle;
    m_camera->Rotate(angle);
    UpdateGrid(0);
    Redraw();
}

// Mac 0x427c0, Win 0x61461a
void BrickConnectSceneControl::RotateCameraVertical(Real32 angle)
{
    if (m_invertedCameraY)
        angle = -angle;
    m_camera->RotateVertical(angle);
    UpdateGrid(0);
    Redraw();
}

// Mac 0x428c0, Win 0x614657
void BrickConnectSceneControl::ZoomCamera(Real32 amount)
{
    m_camera->Zoom(amount);
    UpdateGrid(0);
    Redraw();
}

// Mac 0x429b0
void BrickConnectSceneControl::FocusOnElement(PrimitiveElement* element, bool fit)
{
    Matrix3D transform;
    BoundingBox box(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    element->m_part->GetOrientedBoundingBox(box, transform);
    Sphere bounds(box);
    Vector3 center = box.GetCenter() * transform;
    m_camera->SetTarget(bounds, center, fit);
    Redraw();
}

const Vector3& BrickConnectSceneControl::GetCameraPosition() const { return m_camera->GetPosition(); } // Mac 0x42660
const Vector3& BrickConnectSceneControl::GetCameraTarget() const { return m_camera->GetTarget(); }     // Mac 0x42680
const Vector3& BrickConnectSceneControl::GetCameraUp() const { return m_camera->GetUp(); }             // Mac 0x426a0, Win 0x61371f

// =============================================================================================
// grid / skybox / post-processing
// =============================================================================================

// Mac 0x43180, Win 0x61385a
void BrickConnectSceneControl::SetGridVisible(bool visible)
{
    m_grid = visible ? m_dynamicGrid : m_noneGrid;
}

// Mac 0x431f0, Win 0x61387d
void BrickConnectSceneControl::SetSkybox(Skybox* skybox)
{
    m_skybox = skybox;
    Redraw();
}

// Mac 0x454e0
void BrickConnectSceneControl::NextSkybox()
{
    if (m_skybox.IsNull())
        return;
    m_skybox->Next();
    Redraw();
}

// Mac 0x4f2b0
void BrickConnectSceneControl::SetPostProcessor(PostProcessingEffect* effect)
{
    if (!LDDApplication::GetInstance()->IsPostProcessingEnabled())
        return;
    if (m_postProcessor.IsNull())
        SetInputFlags(m_inputFlags | 4);
    else
        m_postProcessor->Deactivate();
    m_postProcessor = effect;
    if (!m_postProcessor.IsNull() && !m_buffer.IsNull()) {
        m_postProcessor->Resize(m_size, m_buffer->GetImageSize());
        Redraw();
    }
    if (m_postProcessor.IsNull())
        SetInputFlags(m_inputFlags & ~4u);
}

// Mac 0x4f260, Win 0x6616e2
void BrickConnectSceneControl::SetShowMissingBricks(bool show)
{
    if (show == m_showMissingBricks)
        return;
    m_showMissingBricks = show;
    UpdateMissingBricks();
}

// Mac 0x4c850
bool BrickConnectSceneControl::IsShowingMissingBricks() const
{
    if (!m_showMissingBricks)
        return false;
    return !LDDApplication::GetInstance()->IsBrandLDDExtended();
}

// Mac 0x4f3b0
UInt32 BrickConnectSceneControl::GetMissingBrickCount(const BrickPalette::Brick* key) const
{
    BasicMap<const BrickPalette::Brick*, UInt32>::ConstIterator it = m_missingBricks.Find(key);
    return it == m_missingBricks.End() ? 0 : it->second;
}

// Mac 0x42a80, Win 0x612484
void BrickConnectSceneControl::AddPressedButton(bool pressed)
{
    m_pressedButtons += (Int32)pressed * 2 - 1;
    Redraw();
}

// =============================================================================================
// Control overrides
// =============================================================================================

// Mac 0x454a0
bool BrickConnectSceneControl::IsInside(const Vector2&) const
{
    return true;
}

// Mac 0x4aed0, Win 0x6138b6
void BrickConnectSceneControl::OnRectangleChanged(const Vector2& oldPosition, const Vector2& oldSize)
{
    m_camera->SetViewportSize(m_size.x, m_size.y);
    m_lights[0].m_camera->SetViewportSize(m_size.x, m_size.y);
    BufferedAbleRenderControl::OnRectangleChanged(oldPosition, oldSize);
    if (!m_postProcessor.IsNull() && !m_buffer.IsNull())
        m_postProcessor->Resize(m_size, m_buffer->GetImageSize());
}

// Mac 0x4c7f0, Win 0x613937
bool BrickConnectSceneControl::OnBufferingEnabled()
{
    bool result = BufferedAbleRenderControl::OnBufferingEnabled();
    if (!m_postProcessor.IsNull())
        m_postProcessor->Resize(m_size, m_buffer->GetImageSize());
    return result;
}

// Mac 0x4d9c0, Win 0x6148d1
void BrickConnectSceneControl::OnMouseMove(const Vector2& position)
{
    if (GetPressedButtons() & 2) {
        if (m_pressedElement != 0 && DistanceSquared(m_mousePosition, position) > 3.0f)
            m_pressedElement = 0;
        if ((GetManager()->GetModifiers() & 2) == 0) {
            if (m_mousePosition.x != position.x)
                RotateCamera((m_mousePosition.x - position.x) * 0.005f);
            if (m_mousePosition.y != position.y)
                RotateCameraVertical((m_mousePosition.y - position.y) * 0.005f);
        } else {
            bool moved = false;
            if (m_mousePosition.x != position.x) {
                m_camera->PanHorizontal(m_mousePosition.x, position.x);
                moved = true;
            }
            if (m_mousePosition.y != position.y) {
                m_camera->PanVertical(m_mousePosition.y, position.y);
                moved = true;
            }
            if (moved)
                m_camera->Update();
        }
        Redraw();
        m_mousePosition = position;
    }
    Control::OnMouseMove(position);
}

// Mac 0x4db30, Win 0x6219b2
void BrickConnectSceneControl::OnMousePressed(const Vector2& position, UInt32 button)
{
    if (button == 1) {
        m_leftButtonDown = true;
        ++m_pressedButtons;
        Redraw();
        m_mousePosition = position;
        BasicArray<RefPointer<Part> > skip;
        BasicArray<RefPointer<Part> > only;
        Vector3 origin;
        Vector3 direction;
        Real32 length;
        GetPickRay(position, origin, direction, length);
        m_pressedElement = PickElement(origin, direction, length, 0, true, skip, only);
    }
    if (button == 2 && m_developerMode && GetManager()->GetWindow()->IsKeyPressed(0x32)) {
        // the measure features (developer mode, 'M' held)
        Vector3 point;
        BasicArray<RefPointer<Part> > skip;
        BasicArray<RefPointer<Part> > only;
        PrimitiveElement* element = PickElement(position, &point, true, 0, true, skip, only);
        if (element == 0) {
            m_measureFeature1 = 0;
            m_measurePoint1 = Vector3();
            m_measureFeature2 = 0;
            m_measurePoint2 = Vector3();
            // (the original builds the text "Measure features cleared." and drops it)
            Redraw();
        } else {
            Matrix3D elementTransform;
            element->GetTransform(elementTransform);
            ConnectionField::PickerAccumulator accumulator;
            for (UInt32 i = 0; i < element->GetFieldCount(); ++i) {
                ConnectionField* field = element->GetField(i);
                Matrix3D fieldTransform(elementTransform, field->GetTransform());
                field->Pick(fieldTransform, point, 9.0f, accumulator);
            }
            const ConnectionField::PickerAccumulator::PickedFeature* best = accumulator.GetBest();
            if (best != 0) {
                Matrix3D featureTransform(best->m_field->GetTransform(), best->m_transform);
                ConnectionFieldContext* context = best->m_field->GetContext();
                PrimitiveElement* owner = context ? static_cast<PrimitiveElement*>(context) : 0;
                if (m_measureFeature1.IsNull()) {
                    m_measureFeature1 = owner;
                    m_measurePoint1 = featureTransform.GetTranslation();
                    // (the original builds the text "Measure 1 feature picked." and drops it)
                } else {
                    m_measureFeature2 = owner;
                    m_measurePoint2 = featureTransform.GetTranslation();
                    if (m_measureBuffer.IsNull()) {
                        VertexBuffer::CreateArgs args;
                        args.vertexCount = 2;
                        args.indexCount = 2;
                        args.frameCount = 0;
                        args.dynamic = false;
                        args.hasNormals = false;
                        args.colorSetCount = 1;
                        Render* render = GetRenderControlManager()->GetRender();
                        Object* object = render->GetObjectFactory()->CreateObject(typeid(VertexBuffer), args);
                        m_measureBuffer = object ? dynamic_cast<VertexBuffer*>(object) : 0;
                        m_measureBuffer->SetPrimitiveType(1);
                        const Color colors[2] = {Color::Yellow(), Color::Blue()};
                        const UInt32 indices[2] = {0, 1};
                        m_measureBuffer->SetIndices(indices, 0, 2, 0);
                        m_measureBuffer->SetColors(colors, 0, 2, 0);
                        m_measureMesh = new SingleMesh(m_sceneManager->GetSilhouetteMaterial(), m_measureBuffer);
                    }
                    Matrix3D transform1;
                    Matrix3D transform2;
                    m_measureFeature1->GetTransform(transform1);
                    m_measureFeature2->GetTransform(transform2);
                    Vector3 point1 = m_measurePoint1 * transform1;
                    Vector3 point2 = m_measurePoint2 * transform2;
                    // (the original builds "Measure 2 features picked, " << distance and drops it)
                    (void)Distance(point1, point2);
                    Redraw();
                }
            }
        }
    }
    Control::OnMousePressed(position, button);
}

// Mac 0x4e7c0, Win 0x621e46
void BrickConnectSceneControl::OnMouseReleased(const Vector2& position, UInt32 button)
{
    if (button == 1) {
        m_leftButtonDown = false;
        --m_pressedButtons;
        Redraw();
        PrimitiveElement* pressed = m_pressedElement;
        if (pressed != 0) {
            BasicArray<RefPointer<Part> > skip;
            BasicArray<RefPointer<Part> > only;
            Vector3 origin;
            Vector3 direction;
            Real32 length;
            GetPickRay(position, origin, direction, length);
            PrimitiveElement* element = PickElement(origin, direction, length, 0, true, skip, only);
            if (pressed == element) {
                FocusOnElement(m_pressedElement, m_zoomToSelection);
                UpdateGrid(0);
            }
        }
        m_pressedElement = 0;
    }
    Control::OnMouseReleased(position, button);
}

// Mac 0x4eaa0, Win 0x614a00
void BrickConnectSceneControl::OnMouseWheel(UInt32 axis, Real32 delta)
{
    if (axis == 0) {
        delta = delta * 5.0f;
        Real32 amount = delta;
        if (ScaledWheelZoom()) {
            Real32 step = (m_camera->GetTarget() - m_camera->GetPosition()).GetLength() * 0.05f;
            if (step > 5.0f)
                amount = delta * (step / 5.0f);
        }
        ZoomCamera(amount);
    }
    Control::OnMouseWheel(axis, delta);
}

// Mac 0x454b0, Win 0x6124af
void BrickConnectSceneControl::OnKeyboardFocusGained(bool flag, Control* previous, UInt32 modifiers)
{
    m_sceneManager->SetSceneControl(this);
    Control::OnKeyboardFocusGained(flag, previous, modifiers);
}

// Mac 0x4eb00, Win 0x61269d
void BrickConnectSceneControl::OnKeyPressed(UInt32 key, UInt32 modifiers, bool repeat)
{
    if (key == 1)
        m_sceneManager->SetActiveTool(0);
    Control::OnKeyPressed(key, modifiers, repeat);
}

// Mac 0x4eb30, Win 0x622afc — the developer-mode debug keys (no modifier or modifier 2 only).
void BrickConnectSceneControl::OnChar(UniChar16 c, UInt32 modifiers, bool repeated)
{
    if ((modifiers | 2) == 2 && m_developerMode) {
        bool rebuild = true;    // the cases that end with RebuildRenderData
        bool redraw = false;    // the cases that end with Redraw only
        switch (c) {
        case '1': case '2': case '3': {
            // two-bit counters of the debug mode: 0 -> 1 -> 2 -> 3 -> 0 (bits 4-5, 6-7, 8-9)
            const UInt32 shift = c == '1' ? 4 : (c == '2' ? 6 : 8);
            const UInt32 mask = 3u << shift;
            UInt32 mode = m_debugMode3c8;
            UInt32 value = (mode & mask) >> shift;
            if (value == 2)
                mode |= 1u << shift;
            else if (value == 1)
                mode = (mode & ~mask) | (2u << shift);
            else if (value == 0)
                mode |= 1u << shift;
            else
                mode &= ~mask;
            m_debugMode3c8 = mode;
            break;
        }
        case 'A':
            ToggleVersionControl();
            rebuild = false;
            break;
        case 'G':
            m_gridVisible = !m_gridVisible;
            m_grid = m_gridVisible ? m_dynamicGrid : m_noneGrid;
            rebuild = false;
            redraw = true;
            break;
        case 'J':
            RenderVisualDebugger::RemoveAllShapes();
            rebuild = false;
            redraw = true;
            break;
        case 'K':
            m_cullingFlags ^= 8;
            break;
        case 'M':
            m_selectedParts.Clear();
            m_field400.Clear();
            m_onlySelected = false;
            break;
        case 'P':
            m_showOverlay = !m_showOverlay;
            break;
        case 'Q':
            m_cullingFlags ^= 4;
            break;
        case 'R':
            ResetCamera();
            rebuild = false;
            break;
        case 'S':
            ToggleStatisticsControl();
            rebuild = false;
            break;
        case 'V':
            m_debugFlag3c1 = !m_debugFlag3c1;
            if (m_renderMode - 5u > 1u)
                rebuild = false;
            break;
        case 'W':
            m_wireframe = !m_wireframe;
            break;
        case 'b':
            m_debugMode3c8 ^= 8;
            break;
        case 'c':
            m_debugMode3c8 ^= 1;
            break;
        case 'g':
            m_countTriangles = !m_countTriangles;
            UpdateStatistics();
            rebuild = false;
            redraw = true;
            break;
        case 'h':
            m_renderMode = m_renderMode != 3 ? 3 : 0;
            break;
        case 'j':
            m_camera->SetPosition(Vector3(1.0f, 0.0f, 10.0f));
            m_camera->SetTarget(Vector3(1.0f, 0.0f, 0.0f));
            m_camera->Update();
            rebuild = false;
            break;
        case 'k':
            m_cullingFlags ^= 1;
            break;
        case 'l':
            m_scene->GetPhysicsScene()->Simulate6(false);
            rebuild = false;
            break;
        case 'm':
            ToggleOnlySelected(false);
            rebuild = false;
            break;
        case 'o':
            m_renderMode = m_renderMode != 4 ? 4 : 0;
            break;
        case 'p':
            m_field3c0 = !m_field3c0;
            rebuild = false;
            redraw = true;
            break;
        case 'q':
            m_cullingFlags ^= 2;
            break;
        case 'r':
            if (m_renderMode == 2)
                m_renderMode = 0;
            else if (m_renderMode == 1)
                m_renderMode = 2;
            else
                m_renderMode = 1;
            break;
        case 't':
            rebuild = false;
            if (!m_postProcessorControl.IsNull()) {
                String name;
                if (!m_postProcessor.IsNull())
                    name = m_postProcessor->GetName();
                LDDPostProcessor* control = m_postProcessorControl;
                control->SetShown(!control->IsShown(), name);
                if (!m_postProcessorControl->IsShown())
                    m_field504 = 0;
            }
            break;
        case 'v':
            if (m_renderMode == 6)
                m_renderMode = 0;
            else if (m_renderMode == 5)
                m_renderMode = 6;
            else
                m_renderMode = 5;
            break;
        case 'x':
            if (!m_postProcessor.IsNull())
                m_postProcessor->LoadShaders();
            rebuild = false;
            redraw = true;
            break;
        default:
            rebuild = false;
            break;
        }
        if (redraw)
            Redraw();
        else if (rebuild)
            RebuildRenderData();
    }
    Control::OnChar(c, modifiers, repeated);
}

// =============================================================================================
// observers
// =============================================================================================

// Mac 0x4af80 (thunk 0x4b450), Win 0x619ceb
void BrickConnectSceneControl::OnSceneNotification5()
{
    UpdateGrid(0);
    UpdateFlexParts();
    Redraw();
    if (!m_measureFeature1.IsNull() && !m_measureFeature2.IsNull()) {
        Matrix3D transform1;
        Matrix3D transform2;
        m_measureFeature1->GetTransform(transform1);
        m_measureFeature2->GetTransform(transform2);
        Vector3 point1 = m_measurePoint1 * transform1;
        Vector3 point2 = m_measurePoint2 * transform2;
        // (the original builds the text "Measure " << distance and drops it)
        (void)Distance(point1, point2);
    }
    if (!m_loading)
        m_buildingInstructions->Reset();
    BCSC_NOTIFY(OnEvent18());
}

// Mac 0x4b470 (thunk 0x4b7f0), Win 0x6200da
void BrickConnectSceneControl::OnStructureChanged()
{
    UpdateMissingBricks();
    BCSC_NOTIFY(OnEvent19());
    UpdateGrid(0);
    if (!m_loading)
        m_buildingInstructions->Reset();
}

// Mac 0x4b810 (thunk 0x4bfd0), Win 0x622a7c
void BrickConnectSceneControl::OnRigidElementsChanged(const BasicArray<RigidElement*>& added,
                                                      const BasicArray<RigidElement*>& changed,
                                                      const BasicArray<RigidElement*>& removed)
{
    UpdateGrid(0);
    if (m_renderMode == 0)
        UpdateRenderData(added, changed, removed);
    else
        RebuildRenderData();
    BCSC_NOTIFY(OnEvent20());
}

// Mac 0x4c000 (thunk 0x4c010)
void BrickConnectSceneControl::OnGroupsChanged(const BasicArray<RefPointer<BrickGroup> >&,
                                               const BasicArray<RefPointer<BrickGroup> >&,
                                               const BasicArray<RefPointer<BrickGroup> >&,
                                               const BasicArray<RefPointer<BrickGroup> >&)
{
}

// Mac 0x4c070 (thunk 0x4c560), Win 0x614681
void BrickConnectSceneControl::OnPreferenceChanged(const String& key)
{
    if (key.EqualsNoCase(String("DEVELOPERMODE"))) {
        ReadBoolPreference(key, m_developerMode);
        Redraw();
    } else if (key.EqualsNoCase(String("ZoomToSelection"))) {
        ReadBoolPreference(key, m_zoomToSelection);
    } else if (key.EqualsNoCase(String("SelectedPostProcessingEffect"))) {
        UInt32 effectIndex = 0;
        ReadUInt32Preference(key, effectIndex);
        const BasicArray<RefPointer<PostProcessingEffect> >& effects =
            LDDApplication::GetInstance()->GetPostProcessingEffects();
        UInt32 count = effects.GetSize();
        if (effectIndex < count)
            count = effectIndex;
        SetPostProcessor(count == 0 ? 0 : effects[count - 1].Get());
    } else if (key.EqualsNoCase(String("INVERTEDCAMERAX"))) {
        ReadBoolPreference(key, m_invertedCameraX);
    } else if (key.EqualsNoCase(String("INVERTEDCAMERAY"))) {
        ReadBoolPreference(key, m_invertedCameraY);
    } else if (key.EqualsNoCase(String("LevelOfDetail")) || key.EqualsNoCase(String("LevelOfDetailDistance")) ||
               key.EqualsNoCase(String("LevelOfDetailBoxes"))) {
        ReadLodPreferences(true);
    }
}

// Mac 0x4c600 (thunk 0x4c610), Win 0x620153
void BrickConnectSceneControl::OnAttached(BrickPalette*)
{
    UpdateMissingBricks();
}

// Mac 0x4c630 (thunk 0x4c640), Win 0x620153 (ICF)
void BrickConnectSceneControl::OnBagAdded(const BrickPalette::Bag*)
{
    UpdateMissingBricks();
}

// Mac 0x4c660 (thunk 0x4c670), Win 0x620153 (ICF)
void BrickConnectSceneControl::OnBrickAdded(const BrickPalette::Brick*)
{
    UpdateMissingBricks();
}

// Mac 0x4c690 (thunk 0x4c7d0), Win 0x620161
void BrickConnectSceneControl::OnPaletteChanged()
{
    BrickPalette* palette = BrickPaletteObserver::GetSubject();
    const BasicArray<RefPointer<Brick> >& bricks = m_scene->GetBricks();
    for (UInt32 i = 0; i < bricks.GetSize(); ++i) {
        LDDBrick* brick = static_cast<LDDBrick*>(bricks[i].Get());
        RefPointer<PrimitiveElementBrick> key = new PrimitiveElementBrick(brick);
        const BrickDatabase& database = *m_sceneManager->GetDatabase();
        const BrickPalette::Brick* paletteBrick = palette->FindBrick(*key, database);
        if (paletteBrick == 0)
            paletteBrick = palette->FindBrick(*key, database, (BrickPaletteBuckets)1);
        brick->SetPaletteBrick(paletteBrick);
    }
    UpdateMissingBricks();
}

// =============================================================================================
// render data (021f)
// =============================================================================================

// Mac 0x4f590
void BrickConnectSceneControl::BIBrick::ClearMeshes()
{
    m_meshes.Clear();
    m_secondaryMeshes.Clear();
    m_overlay20.Clear();
    m_overlay30.Clear();
    m_overlay40.Clear();
    m_lodRegions.Clear();
}

// Mac 0x4bba0
void BrickConnectSceneControl::UpdateRenderData(const BasicArray<RigidElement*>& added,
                                                const BasicArray<RigidElement*>& changed,
                                                const BasicArray<RigidElement*>& removed)
{
    // removed bodies: their BIBrick goes (the later ones move up)
    for (BasicArray<RigidElement*>::ConstIterator r = removed.Begin(); r != removed.End(); ++r) {
        for (UInt32 i = 0; i < m_bricks.GetSize(); ++i) {
            if (m_bricks[i].m_body == *r) {
                m_bricks.Erase(i);
                break;
            }
        }
    }
    // added bodies: a new BIBrick each, built in place
    for (BasicArray<RigidElement*>::ConstIterator a = added.Begin(); a != added.End(); ++a) {
        BIBrick brick;
        brick.m_body = *a;
        m_bricks.PushBack(brick);
        BuildBodyMeshes(m_bricks.Back(), 0);
    }
    // changed bodies: rebuilt
    for (BasicArray<RigidElement*>::ConstIterator c = changed.Begin(); c != changed.End(); ++c) {
        for (BasicArray<BIBrick>::Iterator b = m_bricks.Begin(); b != m_bricks.End(); ++b) {
            if (b->m_body == *c) {
                b->ClearMeshes();
                BuildBodyMeshes(*b, 0);
                break;
            }
        }
    }
    // NOT restored (debug overlays): Mac 0x47980 per BIBrick when m_debugMode3c8,
    // Mac 0x47a50 per BIBrick when m_showOverlay.
    if (m_meshBufferCache != 0)
        m_meshBufferCache->Prune();
    UpdateFlexParts();
    UpdateStatistics();
    // the first light camera fits the scene (its shadow view)
    // the points as spheres in one array of their size (Scene::GetSceneCVSpheres)
    BasicArray<Sphere> spheres;
    m_scene->GetSceneCVSpheres(spheres, 0.1f);
    if (!spheres.IsEmpty()) {
        BoundingBox box(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        m_scene->GetBoundingBox(box, true);
        Sphere bounds(box);
        Camera* lightCamera = m_lights[0].m_camera;
        lightCamera->FitToSpheres(bounds.m_center, spheres, m_size.x, m_size.y);
        lightCamera->Update();
    }
    Redraw();
}

// =============================================================================================
// storage / statistics / scenery / version text
// =============================================================================================

// fixed (M8, not in the original): see the header.
void BrickConnectSceneControl::ClearToolPreviews()
{
    BCSC_NOTIFY(OnClearPreview());
}

// Mac 0x35440
// NOTE: the original saves the parts' materials as they are, so the paint bucket's hover
// preview (SurfacePaintBucketTool::UpdateHighlight sets the colour on the part) is saved when
// the user saves or exports (Ctrl+S, the menu) while hovering a part; fixed: the previews are
// undone first (M8). (Saves, Save As, exports and uploads all come here through
// LDDApplication::SaveModel, the .lxf thumbnail is rendered after it.)
void BrickConnectSceneControl::SaveToStorage(StorageScene2& storage)
{
    ClearToolPreviews();
    storage.Clear();
    Scene::SaveMap map;
    RefPointer<StorageScene2::Camera> camera = new StorageScene2::Camera;
    storage.m_camera = camera;
    storage.m_cameras.PushBack(camera);
    camera->m_transformation = m_camera->GetViewMatrix();
    camera->m_distance = Distance(m_camera->GetPosition(), m_camera->GetTarget());
    Matrix3D inverse;
    inverse.SetInverse(camera->m_transformation);
    camera->m_transformation = inverse;
    m_scene->Save(storage, map);
    storage.m_buildingInstructions.PushBack(StorageScene2::BI());
    storage.m_buildingInstructions.Back().m_name = m_buildingInstructions->GetName();
    RefPointer<BIStorage> steps = m_buildingInstructions->GetStorage();
    steps->Save(storage, map);
}

// Mac 0x38480, Win 0x616132
void BrickConnectSceneControl::UpdateStatistics()
{
    if (m_statisticsControl.IsNull())
        return;
    UInt32 brickTriangles = 0;
    UInt32 sceneryTriangles = 0;
    if (!m_bricks.IsEmpty() && m_showStatistics) {
        if (m_countTriangles) {
            for (BasicArray<BIBrick>::Iterator b = m_bricks.Begin(); b != m_bricks.End(); ++b) {
                const BasicArray<PlacedMesh>* lists[3] = {&b->m_meshes, &b->m_secondaryMeshes, &b->m_overlay20};
                for (UInt32 l = 0; l < 3; ++l)
                    for (BasicArray<PlacedMesh>::ConstIterator m = lists[l]->Begin(); m != lists[l]->End(); ++m) {
                        SingleMesh* mesh = m->m_mesh.IsNull() ? 0 : dynamic_cast<SingleMesh*>(m->m_mesh.Get());
                        if (mesh != 0)
                            brickTriangles += mesh->GetVertexBuffer()->GetIndexCount() / 3;
                    }
            }
        }
        for (BasicArray<LDDPart*>::Iterator it = m_flexParts.Begin(); it != m_flexParts.End(); ++it) {
            LDDPart* part = *it;
            if (part->m_hidden || part->GetScene() != m_scene)
                continue;
            if (m_onlySelected && !ContainsPart(m_selectedParts, part))
                continue;
            for (BasicArray<LDDPart::FlexSurface>::Iterator s = part->m_flexSurfaces.Begin();
                 s != part->m_flexSurfaces.End(); ++s) {
                if (!s->m_mesh.IsNull())
                    brickTriangles += s->m_vertexBuffer->GetIndexCount() / 3;
                if (!s->m_decalMesh.IsNull())
                    brickTriangles += s->m_vertexBuffer->GetIndexCount() / 3;
            }
        }
    }
    for (BasicArray<SceneryElement>::Iterator e = m_sceneryElements.Begin(); e != m_sceneryElements.End(); ++e)
        for (BasicArray<PlacedMesh>::Iterator m = e->m_meshes.Begin(); m != e->m_meshes.End(); ++m) {
            SingleMesh* mesh = m->m_mesh.IsNull() ? 0 : dynamic_cast<SingleMesh*>(m->m_mesh.Get());
            if (mesh != 0)
                sceneryTriangles += mesh->GetVertexBuffer()->GetIndexCount() / 3;
        }
    Scene::Statistics s;
    m_scene->GetStatistics(s);
    const char* const kEnd = "\r\n";
    String text;
    text << "HingedElements: " << s.m_counts[0] << kEnd;
    text << "RigidElements: " << s.m_counts[2] << ", connections: " << s.m_counts[3] << kEnd;
    text << "PrimitiveRigidElements: " << s.m_counts[4] << ", connections: " << s.m_counts[5] << kEnd;
    text << "PrimitiveElements: " << s.m_counts[6] << ", connections: " << s.m_counts[7] << kEnd;
    // (indices per Mac 0x38480's stack offsets -0x68 .. -0x3c; [9] is the field
    // connection count printed after the fields)
    text << "ConnectionFields: " << s.m_counts[8] << ", connections: " << s.m_counts[9] << kEnd;
    text << "UniqueBricks: " << s.m_counts[10] << kEnd;
    text << "UniqueParts: " << s.m_counts[11] << kEnd;
    text << "UniqueBones: " << s.m_counts[12] << kEnd;
    text << "UniqueMaterials: " << s.m_counts[13] << kEnd;
    text << "UniqueDecorations: " << s.m_counts[14] << kEnd;
    text << "InstanceBricks: " << s.m_counts[15] << kEnd;
    text << "InstanceParts: " << s.m_counts[16] << kEnd;
    text << "InstanceBones: " << s.m_counts[17] << kEnd;
    text << "InstanceMaterials: " << s.m_counts[18] << kEnd;
    text << "InstanceDecorations: " << s.m_counts[19] << kEnd;
    text << "Default collision volumes: " << s.m_counts[20] << ", active: " << s.m_counts[21] << kEnd;
    text << "Brick triangles: " << brickTriangles << kEnd;
    text << "Scenery triangles: " << sceneryTriangles << kEnd;
    m_statisticsControl->SetText(text);
    m_statisticsControl->ResizeHeightToFitText(0);
}

// ---- clipboard / click-stick helpers ----------------------------------------------------------

// Mac 0x37370, Win 0x61e2a9
void BrickConnectSceneControl::GetElementBricks(const BasicArray<RefPointer<Part> >& parts,
                                                BasicArray<ObjectRefPointer<PrimitiveElementBrick> >& bricks)
{
    CreateElementBricks(parts, bricks);
}

// Mac 0x37390
void BrickConnectSceneControl::CutParts(const BasicArray<RefPointer<Part> >& parts,
                                        BasicArray<ObjectRefPointer<PrimitiveElementBrick> >& bricks)
{
    CreateElementBricks(parts, bricks);
    BasicArray<RefPointer<Brick> > partBricks;
    Scene::GetBricks(parts, partBricks);
    RemoveBricks(partBricks);
}

// Mac 0x46220 (helper BasicArray<ObjectRefPointer<PrimitiveElementBrick> >::Assign Mac 0x59480)
bool BrickConnectSceneControl::SetCarriedBricks(const BasicArray<ObjectRefPointer<PrimitiveElementBrick> >& bricks)
{
    m_carriedBricks.Clear();
    if (&m_carriedBricks != &bricks)
        m_carriedBricks = bricks;
    return true;
}

// Mac 0x462a0
void BrickConnectSceneControl::ClearCarriedBricks()
{
    m_carriedBricks.Clear();
}

// Mac 0x46300 (the same body as 0x462a0; a second inline member in the original)
void BrickConnectSceneControl::ResetCarriedBricks()
{
    m_carriedBricks.Clear();
}

// Mac 0x46350, Win 0x618438
void BrickConnectSceneControl::SetShowStatistics(bool show)
{
    m_showStatistics = show;
    UpdateStatistics();
}

// Mac 0x2f730 (021a), Win 0x61d22c
void BrickConnectSceneControl::AddSceneryElement(PhysicsSceneryElement* object, const Matrix3D& transform,
                                                 UInt32 flags, const BasicArray<PlacedMesh>& meshes)
{
    SceneryElement element;
    element.m_object = object;
    element.m_meshes = meshes;
    m_sceneryElements.PushBack(element);   // (Mac 0x38100)
    object->AddRef();
    m_scene->GetPhysicsScene()->AddSceneryElement(object, transform, flags);
    UpdateStatistics();
}

// Mac 0x38d60
void BrickConnectSceneControl::RemoveSceneryElement(PhysicsSceneryElement* object)
{
    for (UInt32 i = 0; i < m_sceneryElements.GetSize(); ++i) {
        if (m_sceneryElements[i].m_object != object)
            continue;
        m_scene->GetPhysicsScene()->RemoveSceneryElement(object);   // (slot 26)
        m_sceneryElements[i].m_object->Release();
        m_sceneryElements.Erase(i);   // (Mac 0x38de0: the later elements move up)
        UpdateStatistics();
        return;
    }
}

// Mac 0x39150 (021b; name inferred)
void BrickConnectSceneControl::CreatePhysicsSnapshot(PhysicsSnapshot*& snapshot)
{
    if (snapshot != 0) {
        snapshot->Release();
        snapshot = 0;
    }
    if (m_scene->IsEmpty())
        return;
    BasicArray<PhysicsRigidElement*> bodies;
    bodies.Reserve(0x100);
    const BasicArray<RefPointer<HingedElement> >& hinged = m_scene->GetHingedElements();
    for (BasicArray<RefPointer<HingedElement> >::ConstIterator h = hinged.Begin(); h != hinged.End(); ++h) {
        const BasicArray<RefPointer<RigidElement> >& elements = (*h)->GetElements();
        for (BasicArray<RefPointer<RigidElement> >::ConstIterator r = elements.Begin(); r != elements.End(); ++r)
            bodies.PushBack((*r)->GetBody());
    }
    snapshot = m_scene->GetPhysicsScene()->CreateSnapshot(bodies.Begin(), bodies.GetSize());
}

// Mac 0x39480 (021b; name inferred). Helper: BasicArray<RefPointer<
// PrimitiveElement> >::PushBack Mac 0x397b0.
void BrickConnectSceneControl::RestorePhysicsSnapshot(PhysicsSnapshot* snapshot, bool undo)
{
    UInt32 count = snapshot->GetCount();
    if (count == 0)
        return;
    if (undo) {
        BasicArray<RefPointer<PrimitiveElement> > elements;
        for (UInt32 i = 0; i < count; ++i) {
            RigidElement* rigid = snapshot->GetRigidElement(i)->GetRigidElement();
            const BasicArray<RefPointer<PrimitiveRigidElement> >& primitives = rigid->GetElements();
            for (BasicArray<RefPointer<PrimitiveRigidElement> >::ConstIterator p = primitives.Begin();
                 p != primitives.End(); ++p) {
                const BasicArray<RefPointer<PrimitiveElement> >& list = (*p)->GetElements();
                for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator e = list.Begin(); e != list.End(); ++e)
                    elements.PushBack(*e);
            }
        }
        // (Mac 0xb26a0 takes the RefPointer array as it is; the same pointers here)
        BasicArray<PrimitiveElement*> raw;
        raw.Reserve(elements.GetSize());
        for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator e = elements.Begin(); e != elements.End(); ++e)
            raw.PushBack(e->Get());
        BasicArray<Pair<PrimitiveElement*, Matrix3D> > transforms;
        Scene::GetTransforms(raw, transforms);
        m_undoManager->AddAction(new UndoMoveSnapshot(this, transforms, false));
    }
    m_scene->GetPhysicsScene()->RestoreSnapshot(snapshot);
    m_scene->NotifyObservers5();
}

// Mac 0x4c880, Win 0x6125c8
String BrickConnectSceneControl::GetBrandURLText(Brand brand) const
{
    String text;
    String name;
    ToString(brand, name);
    text << "   Brand - " << name << ":\n";
    text << "      GetUploadModelURL = " << LDDWebCommunicator::GetInstance()->GetUploadModelURL(brand) << "\n";
    text << "      GetCheckPriceURL = " << LDDWebCommunicator::GetInstance()->GetCheckPriceURL(brand) << "\n";
    return text;
}

// =============================================================================================
// 021h: picking, selection, hiding, groups, part descriptions, missing bricks,
// debug text controls
// =============================================================================================

namespace {

// `part` in `parts` (raw pointers; the inlined std::find of the pick's visited flexible parts).
bool ContainsRaw(const BasicArray<Part*>& parts, const Part* part)
{
    for (BasicArray<Part*>::ConstIterator it = parts.Begin(); it != parts.End(); ++it)
        if (*it == part)
            return true;
    return false;
}

// Removes `part` from `parts` by moving the last element into its place (the order changes);
// nothing when it is not there. Mac 0x3f680 (std::find + the swap-with-last erase 0x58bc0, a
// BasicArray<RefPointer<Part>> template instance; InvertSelectionCommand calls it too).
void SwapRemove(BasicArray<RefPointer<Part> >& parts, const Part* part)
{
    for (UInt32 i = 0; i < parts.GetSize(); ++i) {
        if (parts[i].Get() != part)
            continue;
        if (i + 1 != parts.GetSize())
            parts[i] = parts[parts.GetSize() - 1];
        parts.PopBack();
        return;
    }
}

// RefPointer arrays as the raw pointer arrays Scene::SetGroups / AddGroup take (the original
// passes the RefPointer arrays: same layout).
void ToRaw(const BasicArray<RefPointer<Part> >& in, BasicArray<Part*>& out)
{
    out.Reserve(in.GetSize());
    for (BasicArray<RefPointer<Part> >::ConstIterator it = in.Begin(); it != in.End(); ++it)
        out.PushBack(it->Get());
}
void ToRaw(const BasicArray<RefPointer<BrickGroup> >& in, BasicArray<BrickGroup*>& out)
{
    out.Reserve(in.GetSize());
    for (BasicArray<RefPointer<BrickGroup> >::ConstIterator it = in.Begin(); it != in.End(); ++it)
        out.PushBack(it->Get());
}

// The parts and their current groups (UndoAddGroup / UndoAddParts constructors).
void CollectGroups(const BasicArray<RefPointer<Part> >& parts, BasicArray<RefPointer<Part> >& outParts,
                   BasicArray<RefPointer<BrickGroup> >& outGroups)
{
    for (BasicArray<RefPointer<Part> >::ConstIterator it = parts.Begin(); it != parts.End(); ++it) {
        outParts.PushBack(*it);
        outGroups.PushBack(RefPointer<BrickGroup>((*it)->m_group));
    }
}

// A debug text control (statistics, versions): a RenderControl child of `parent`, aligned to the
// right (-15), transparent, application font 6. (inline twice, Mac 0x3adb0 / 0x3afe0)
RefPointer<RenderControl> NewDebugTextControl(Control* parent, ControlManager* manager)
{
    RefPointer<RenderControl> control = new RenderControl(manager);
    control->Initialize();
    control->Setup(parent, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    control->SetAxisAlignment(0, 1, 1, -15.0f, 0);
    control->SetTransparent(true);
    control->SetFont(LDDApplication::GetInstance()->GetFont(6));
    RefPointer<TextLayoutRender> layout;
    control->GetTextLayout(layout, false);
    return control;
}

} // namespace

// ---- picking ---------------------------------------------------------------------------------

// Mac 0x3b750, Win 0x61344d
void BrickConnectSceneControl::GetPickRay(const Vector2& position, Vector3& origin, Vector3& direction,
                                          Real32& length) const
{
    Matrix3D inverse;
    const Vector2& size = GetSize();
    Real32 x = position.x / size.x;
    x = x + x + -1.0f;
    direction.x = x / m_camera->GetProjectionMatrix().m[0];
    Real32 y = position.y / size.y;
    y = -(y + y + -1.0f);
    direction.y = y / m_camera->GetProjectionMatrix().m[5];
    direction.z = -1.0f;
    inverse.SetInverse(m_camera->GetViewMatrix());
    direction = Rotate(direction, inverse);
    direction.Normalize();
    origin = inverse.GetTranslation();
    origin += direction * m_camera->GetNear();
    length = (Real32)fabs(m_camera->GetFar() - m_camera->GetNear());
}

// Mac 0x3b8f0, Win 0x613556
void BrickConnectSceneControl::GetPickFrustum(const Rect& rect, Frustum& frustum) const
{
    Vector3 apex;
    Matrix3D inverse;
    inverse.SetInverse(m_camera->GetViewMatrix());
    apex = inverse.GetTranslation();
    Vector3 corner0;
    Vector3 corner1;
    Vector3 corner2;
    Vector3 corner3;
    Vector3 direction;
    Real32 length;
    GetPickRay(rect.GetMin(), corner0, direction, length);
    GetPickRay(rect.GetTopRight(), corner1, direction, length);
    GetPickRay(rect.GetBottomLeft(), corner3, direction, length);
    GetPickRay(rect.m_max, corner2, direction, length);
    frustum = Frustum(apex, corner0, corner1, corner2, corner3);
}

// Mac 0x3ba30, Win 0x6214ee
PrimitiveElement* BrickConnectSceneControl::PickElement(const Vector2& position, Vector3* point, bool local,
                                                        UInt32* surface, bool materialSlot,
                                                        const BasicArray<RefPointer<Part> >& filterParts,
                                                        const BasicArray<RefPointer<Part> >& extraParts)
{
    Vector3 direction;
    Vector3 origin;
    Real32 length;
    GetPickRay(position, origin, direction, length);
    PrimitiveElement* element = PickElement(origin, direction, length, surface, materialSlot, filterParts, extraParts);
    if (point != 0 && element != 0) {
        *point = origin + direction * length;
        if (local) {
            Matrix3D transform;
            Matrix3D inverse;
            element->GetTransform(transform);
            inverse.SetInverse(transform);
            *point *= inverse;
        }
    }
    return element;
}

// Mac 0x3bb30, Win 0x61f511 (+ 0x61d29c)
PrimitiveElement* BrickConnectSceneControl::PickElement(const Vector3& origin, const Vector3& direction,
                                                        Real32& length, UInt32* surface, bool materialSlot,
                                                        const BasicArray<RefPointer<Part> >& filterParts,
                                                        const BasicArray<RefPointer<Part> >& extraParts)
{
    typedef Scene::PickResult::Set Set;
    Scene::PickResult picked;
    m_scene->PickRay(origin, direction, &picked);
    FilterOnlySelected(picked);
    if (!filterParts.IsEmpty()) {
        Set& set = picked.Std();
        for (Set::iterator it = set.begin(); it != set.end();) {
            if (!ContainsPart(filterParts, it->m_element->m_part))
                set.erase(it++);
            else
                ++it;
        }
    }
    // the elements of the extra parts: bone box sphere, centre at the element's transform
    for (BasicArray<RefPointer<Part> >::ConstIterator p = extraParts.Begin(); p != extraParts.End(); ++p) {
        const BasicArray<RefPointer<PrimitiveElement> >& elements = (*p)->m_elements;
        for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator e = elements.Begin(); e != elements.End(); ++e) {
            PrimitiveElement* element = *e;
            Matrix3D transform;
            element->GetTransform(transform);
            Sphere sphere(element->m_primitive->GetBone(element->m_boneIndex).m_bounding);
            sphere.m_center *= transform;   // NOTE: only the centre moves (no scale in practice)
            picked.Insert(Scene::PrimitiveElementWithDistance(element, sphere.m_center.GetDistanceSquared(origin),
                                                              sphere));
        }
    }
    if (&m_selectedParts != &filterParts)
        FilterOnlySelected(picked);
    // by the distance to the sphere's front
    Scene::PickResult sorted;
    for (Set::const_iterator it = picked.Std().begin(); it != picked.Std().end(); ++it)
        sorted.Insert(Scene::PrimitiveElementWithDistance(it->m_element, sqrtf(it->m_distance) - it->m_sphere.m_radius,
                                                          it->m_sphere));
    picked.Std().swap(sorted.Std());

    const GeometryAccum::ImportPrimitiveMap& imports = m_sceneManager->GetImportPrimitives();
    BasicArray<Part*> visited;   // flexible parts tested
    PrimitiveElement* result = 0;
    for (Set::const_iterator it = picked.Std().begin(); it != picked.Std().end() && !(it->m_distance > length); ++it) {
        PrimitiveElement* element = it->m_element;
        Part* part = element->m_part;
        if (part->m_elements.GetSize() < 2) {
            // rigid: the bone box, then the import geometry's triangles, in element space
            Matrix3D transform;
            Matrix3D inverse;
            element->GetTransform(transform);
            inverse.SetInverse(transform);
            Vector3 localOrigin = origin * inverse;
            Vector3 localDirection = Rotate(direction, inverse);
            Vector3 hit;
            const Primitive* primitive = element->m_primitive;
            if (!HitBoundingBox(primitive->GetBone(element->m_boneIndex).m_bounding, localOrigin, localDirection, hit))
                continue;
            GeometryAccum::ImportPrimitiveMap::ConstIterator found = imports.Find(primitive->GetDesignID());
            if (found == imports.End())
                continue;
            const BasicArray<GeometryAccum::ImportGeometry>& geometries = found->second.geometries;
            for (UInt32 s = 0; s < geometries.GetSize(); ++s) {
                const GeometryAccum::ImportGeometry& g = geometries[s];
                const Vector3* positions = (const Vector3*)g.positions;
                for (UInt32 t = 0; t < g.indexCount; t += 3) {
                    Real32 distance, u, v;
                    if (RayTriangleIntersect(localOrigin, localDirection, positions[g.indices[t]],
                                             positions[g.indices[t + 1]], positions[g.indices[t + 2]], distance, u, v) &&
                        distance < length) {
                        length = distance;
                        result = element;
                        if (surface != 0)
                            *surface = s;
                    }
                }
            }
        } else if (!ContainsRaw(visited, part)) {
            // flexible: the skinned surfaces' triangles, in world space, once per part
            visited.PushBack(part);
            LDDPart* flex = static_cast<LDDPart*>(part);
            if (flex->m_flexTransforms.IsEmpty() || flex->m_flexSurfaces.IsEmpty())
                continue;
            for (UInt32 s = 0; s < flex->m_flexSurfaces.GetSize(); ++s) {
                const LDDPart::FlexSurface& fs = flex->m_flexSurfaces[s];
                UInt32 stride;
                const Vector3* positions = (const Vector3*)fs.m_vertexBuffer->GetPositions(stride, 0);
                const UInt32* indices = fs.m_vertexBuffer->GetIndices();
                const UInt32 count = fs.m_vertexBuffer->GetIndexCount();
                for (UInt32 t = 0; t < count; t += 3) {
                    Real32 distance, u, v;
                    const UInt32* tri = indices + t;
                    if (!RayTriangleIntersect(origin, direction, positions[tri[0]], positions[tri[1]], positions[tri[2]],
                                              distance, u, v) ||
                        !(distance < length))
                        continue;
                    length = distance;
                    // the element of the corner nearest to the hit (its first bone weight)
                    Vector3 point = origin + direction * distance;
                    const Real32 d0 = Distance(point, positions[tri[0]]);
                    const Real32 d1 = Distance(point, positions[tri[1]]);
                    const Real32 d2 = Distance(point, positions[tri[2]]);
                    const UInt32* nearest;
                    if (d0 <= d1)
                        nearest = d2 < d0 ? &tri[2] : &tri[0];
                    else
                        nearest = d2 < d1 ? &tri[2] : &tri[1];
                    const UInt8* weights = fs.m_geometry->GetVertexBoneWeights()[*nearest];
                    result = part->m_elements[*(const UInt32*)(weights + 4)];
                    if (surface != 0)
                        *surface = s;
                }
            }
        }
    }
    if (surface != 0 && result != 0 && materialSlot) {
        const BasicArray<UInt32>& redirect = result->m_primitive->GetSubMaterialRedirect();
        if (!redirect.IsEmpty())
            *surface = redirect[*surface];
    }
    return result;
}

// Mac 0x3cbf0, Win 0x614569
bool BrickConnectSceneControl::PickFeatures(const Vector2& position, BasicArray<Scene::Feature>& features)
{
    Vector3 direction;
    Vector3 origin;
    Real32 length;
    GetPickRay(position, origin, direction, length);
    features.Clear();
    Scene::HideAwareSamePlaceFeaturesPicker picker(&features, origin, direction, length * length);
    m_scene->Pick(picker);
    return !features.IsEmpty();
}

// Mac 0x3d4e0, Win 0x61d2c5
void BrickConnectSceneControl::PickFrustum(const Frustum& frustum, Scene::PickResult& result)
{
    m_scene->PickFrustum(frustum, &result);
    FilterOnlySelected(result);
}

// Mac 0x3d360, Win 0x61ceb2
void BrickConnectSceneControl::FilterOnlySelected(Scene::PickResult& result) const
{
    if (!m_onlySelected)
        return;
    Scene::PickResult::Set& set = result.Std();
    for (Scene::PickResult::Set::iterator it = set.begin(); it != set.end();) {
        if (!ContainsPart(m_selectedParts, it->m_element->m_part))
            set.erase(it++);
        else
            ++it;
    }
}

// ---- debug toggles -----------------------------------------------------------------------------

// Mac 0x3aad0
void BrickConnectSceneControl::ToggleOnlySelected(bool clear)
{
    if (!clear) {
        const bool was = m_onlySelected;
        m_onlySelected = !was;
        if (!was && !m_selection.IsEmpty()) {
            m_selectedParts = m_selection;
            m_field400.Clear();
            m_field400.Reserve(m_selectedParts.GetSize() * 2);   // (the original's byte count / 2)
            for (BasicArray<RefPointer<Part> >::ConstIterator p = m_selectedParts.Begin(); p != m_selectedParts.End(); ++p)
                for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator e = (*p)->m_elements.Begin();
                     e != (*p)->m_elements.End(); ++e)
                    m_field400.PushBack(*e);
        }
    } else {
        m_selectedParts.Clear();
        m_field400.Clear();
        m_onlySelected = false;
    }
    RebuildRenderData();
}

// Mac 0x3adb0
void BrickConnectSceneControl::ToggleStatisticsControl()
{
    if (m_statisticsControl.IsNull()) {
        m_statisticsControl = NewDebugTextControl(this, GetManager());
        UpdateStatistics();
    } else {
        m_statisticsControl->SetParent(0, 0);
        m_statisticsControl = 0;
    }
}

// Mac 0x3afe0
void BrickConnectSceneControl::ToggleVersionControl()
{
    if (m_versionControl.IsNull())
        m_versionControl = NewDebugTextControl(this, GetManager());
    if (!UpdateVersionText()) {
        m_versionControl->SetParent(0, 0);
        m_versionControl = 0;
    }
}

// Mac 0x3b230, Win (part) 0x613a73
bool BrickConnectSceneControl::UpdateVersionText()
{
    if (m_versionControl.IsNull())
        return true;
    String text;
    if (m_field234 == 1) {
        String list;
        LDDWebCommunicator* web = LDDWebCommunicator::GetInstance();
        text << "\nLDDCommunicator values:\n";
        text << "   CollectionURL = " << web->GetCollectionURL() << "\n";
        ToString(LDDWebCommunicator::GetInstance()->GetCollections(), list);
        text << "   Collections = " << list << "\n";
        text << "   GetGeometryURL = " << LDDWebCommunicator::GetInstance()->GetGeometryURL() << "\n";
        text << "   GetInstallerURLPC = " << LDDWebCommunicator::GetInstance()->GetInstallerURLPC() << "\n";
        text << "   GetInstallerURLMAC = " << LDDWebCommunicator::GetInstance()->GetInstallerURLMac() << "\n";
        list = "";
        LDDWebCommunicator::GetInstance()->GetCountries().Format(list);
        text << "   GetCountries = " << list << "\n";
        text << "   GetDefaultCountryIndex = " << LDDWebCommunicator::GetInstance()->GetDefaultCountryIndex() << "\n";
        text << "   GetCheckPriceURL = " << LDDWebCommunicator::GetInstance()->GetCheckPriceURL() << "\n";
        text << "   GetGenerateBGBeforeUpload = "
             << (LDDWebCommunicator::GetInstance()->GetGenerateBGBeforeUpload() ? "true" : "false") << "\n";
        text << GetBrandURLText((Brand)0);
        text << GetBrandURLText((Brand)1);
        text << GetBrandURLText((Brand)2);
        text << "\n\nVersions:\n";
        text << "   Brickset: ";
        ToString(m_sceneManager->GetDatabase()->GetVersion(), text);
        text << "\n   Application: ";
        Version version;
        Application::GetInstance()->GetVersion(version);
        ToString(version, text);
    } else if (m_field234 != 0) {
        m_field234 = 0;
        return false;
    } else {
        text = "\nPreference values:\n";
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        preferences->Dump(text);
    }
    ++m_field234;
    m_versionControl->SetText(text);
    m_versionControl->ResizeHeightToFitText(0);
    return true;
}

// Mac 0x42d90
void BrickConnectSceneControl::SetBuildingGuideMode(bool on)
{
    m_field461 = on;
    if (!on) {
        RebuildRenderData();
        return;
    }
    ClearSelection();
    m_selectedParts.Clear();
    m_field400.Clear();
    m_onlySelected = false;
    UpdateStatistics();
}

// Mac 0x42e60
bool BrickConnectSceneControl::IsBuildingGuideMode() const
{
    return m_field461;
}

// Mac 0x43160, Win 0x61384f
Real32 BrickConnectSceneControl::GetGridHeight() const
{
    return m_grid->GetHeight();
}

// Mac 0x42600, Win 0x6136ca
void BrickConnectSceneControl::SetCameraLookAt(const Vector3& position, const Vector3& target, const Vector3& up)
{
    m_camera->SetLookAt(position, target, up);
    m_camera->SetViewportSize(GetSize().x, GetSize().y);
}

// Mac 0x42ac0
void BrickConnectSceneControl::SetToolCursor(UInt32 cursor, bool flag)
{
    Control::SetCursor(cursor, true);
    BCSC_NOTIFY(OnEvent21(cursor, flag));
}

// ---- selection ----------------------------------------------------------------------------------

// Mac 0x3d520, Win 0x617806
void BrickConnectSceneControl::SetSelection(const BasicArray<RefPointer<Part> >& parts)
{
    const bool wasEmpty = m_selection.IsEmpty();
    if (&m_selection != &parts)
        m_selection = parts;
    UpdateSelectionMesh();
    if (wasEmpty) {
        if (!m_selection.IsEmpty())
            BCSC_NOTIFY(OnEvent14(false));
    } else if (m_selection.IsEmpty()) {
        BCSC_NOTIFY(OnEvent14(true));
    }
    BCSC_NOTIFY(OnEvent15(m_selection));
}

// Mac 0x3dd10, Win 0x617914
void BrickConnectSceneControl::ToggleSelection(Part* part)
{
    UInt32 index = 0;
    while (index < m_selection.GetSize() && m_selection[index].Get() != part)
        ++index;
    if (index == m_selection.GetSize()) {
        m_selection.PushBack(part);
        if (m_selection.GetSize() == 1)
            BCSC_NOTIFY(OnEvent14(false));
    } else {
        m_selection.Erase(index);
        if (m_selection.IsEmpty())
            BCSC_NOTIFY(OnEvent14(true));
    }
    Redraw();
    UpdateSelectionMesh();
    BCSC_NOTIFY(OnEvent15(m_selection));
}

// Mac 0x3e800, Win 0x617b46
void BrickConnectSceneControl::SelectAll()
{
    m_selection.Clear();
    m_scene->GetParts(m_selection);
    UpdateSelectionMesh();
    if (!m_selection.IsEmpty())
        BCSC_NOTIFY(OnEvent14(false));
    BCSC_NOTIFY(OnEvent15(m_selection));
}

// ---- hiding -------------------------------------------------------------------------------------

// Mac 0x3fa60
void BrickConnectSceneControl::GetHiddenBricks(BasicArray<RefPointer<Brick> >& bricks) const
{
    const BasicArray<RefPointer<Brick> >& all = m_scene->GetBricks();
    for (BasicArray<RefPointer<Brick> >::ConstIterator it = all.Begin(); it != all.End(); ++it)
        if ((*it)->IsHidden())
            bricks.PushBack(*it);
}

// Mac 0x3ee00, Win 0x617c0a
void BrickConnectSceneControl::HideBricks(const BasicArray<RefPointer<Brick> >& bricks)
{
    if (bricks.IsEmpty())
        return;
    const UInt32 before = m_selection.GetSize();
    m_scene->BeginUpdate();
    for (BasicArray<RefPointer<Brick> >::ConstIterator b = bricks.Begin(); b != bricks.End(); ++b) {
        Brick* brick = *b;
        brick->SetHidden(true);
        const BasicArray<RefPointer<Part> >& parts = brick->GetParts();
        for (BasicArray<RefPointer<Part> >::ConstIterator p = parts.Begin(); p != parts.End(); ++p)
            SwapRemove(m_selection, *p);
    }
    m_scene->EndUpdate();
    if (before != m_selection.GetSize()) {
        UpdateSelectionMesh();
        if (m_selection.IsEmpty())
            BCSC_NOTIFY(OnEvent14(true));
        BCSC_NOTIFY(OnEvent15(m_selection));
    }
    BCSC_NOTIFY(OnEvent22(1));
}

// Mac 0x3f770, Win 0x61533d
void BrickConnectSceneControl::ShowBricks(const BasicArray<RefPointer<Brick> >& bricks)
{
    if (bricks.IsEmpty())
        return;
    m_scene->BeginUpdate();
    for (BasicArray<RefPointer<Brick> >::ConstIterator b = bricks.Begin(); b != bricks.End(); ++b)
        (*b)->SetHidden(false);
    m_scene->EndUpdate();
    BCSC_NOTIFY(OnEvent22(0));
}

// ---- groups -------------------------------------------------------------------------------------

// Mac 0x3fab0, Win 0x617dd1
BrickGroup* BrickConnectSceneControl::CreateGroup(BrickGroup* parent, const BasicArray<RefPointer<Part> >& parts)
{
    BasicArray<RefPointer<Part> > expanded;
    Scene::ExpandParts(parts, expanded, true, false, true);
    UndoAddGroup* undo = new UndoAddGroup(this, expanded);
    undo->m_group = m_scene->CreateGroup(parent, expanded);
    UndoManagerObserver::GetSubject()->AddAction(undo);
    return undo->m_group;
}

// Mac 0x3fe80, Win 0x617e81
void BrickConnectSceneControl::UnGroup(BrickGroup* group)
{
    UndoManagerObserver::GetSubject()->AddAction(new UndoUnGroup(this, group));
    m_scene->RemoveGroup(group);
}

// Mac inlined into 0x54890, Win 0x617ed9
void BrickConnectSceneControl::UnGroup(BrickGroup* group, const BasicArray<RefPointer<Part> >& parts,
                                       const BasicArray<RefPointer<BrickGroup> >& groups)
{
    UndoManagerObserver::GetSubject()->AddAction(new UndoUnGroup(this, group));
    BasicArray<Part*> rawParts;
    BasicArray<BrickGroup*> rawGroups;
    ToRaw(parts, rawParts);
    ToRaw(groups, rawGroups);
    m_scene->SetGroups(rawParts, rawGroups);
    m_scene->RemoveGroup(group);
}

// Mac 0x40110, Win 0x613669
void BrickConnectSceneControl::ReGroup(BrickGroup* group, BrickGroup* parent,
                                       const BasicArray<RefPointer<Part> >& parts,
                                       const BasicArray<RefPointer<BrickGroup> >& groups)
{
    UndoManagerObserver::GetSubject()->AddAction(new UndoReGroup(this, group));
    BasicArray<Part*> rawParts;
    BasicArray<BrickGroup*> rawGroups;
    ToRaw(parts, rawParts);
    ToRaw(groups, rawGroups);
    m_scene->AddGroup(group, parent, &rawParts, &rawGroups);
}

// Mac 0x401f0, Win 0x617f40
void BrickConnectSceneControl::AddPartsToGroup(BrickGroup* group, const BasicArray<RefPointer<Part> >& parts)
{
    if (parts.IsEmpty())
        return;
    BasicArray<RefPointer<Part> > expanded;
    Scene::ExpandParts(parts, expanded, true, false, true);
    RefPointer<UndoAddParts> undo = new UndoAddParts(this);
    group->AddParts(expanded, &undo->m_parts, &undo->m_groups);
    if (!undo->m_parts.IsEmpty())
        UndoManagerObserver::GetSubject()->AddAction(undo);
}

// Mac 0x40400, Win 0x61800f
void BrickConnectSceneControl::RemovePartsFromGroup(BrickGroup* group, const BasicArray<RefPointer<Part> >& parts)
{
    if (parts.IsEmpty())
        return;
    BasicArray<RefPointer<Part> > expanded;
    Scene::ExpandParts(parts, expanded, true, false, true);
    RefPointer<UndoAddParts> undo = new UndoAddParts(this);
    group->RemoveParts(expanded, &undo->m_parts, &undo->m_groups);
    if (!undo->m_parts.IsEmpty())
        UndoManagerObserver::GetSubject()->AddAction(undo);
}

// ---- materials / decorations --------------------------------------------------------------------

// Mac 0x4cd60
BrickConnectSceneControl::PartDescription::PartDescription(const PrimitiveElementBrick& brick, UInt32 index)
    : m_materials(brick.GetMaterials(index)), m_decorations(brick.GetDecorations(index))
{
}

// Mac 0x41010, Win 0x620251
void BrickConnectSceneControl::SetPartDescriptions(const BasicArray<RefPointer<Part> >& parts,
                                                   const BasicArray<PartDescription>& descriptions)
{
    if (parts.IsEmpty())
        return;
    RefPointer<UndoPartDescription> undo = new UndoPartDescription(this);
    m_scene->BeginUpdate();
    // NOTE: the bodies of the parts whose materials changed are collected and never used (the
    // original builds and frees the list as well).
    BasicArray<RigidElement*> bodies;
    for (UInt32 i = 0; i < parts.GetSize(); ++i) {
        Part* part = parts[i];
        const PartDescription& description = descriptions[i];
        bool materialsSame = true;
        bool changed = false;
        if (!description.m_materials.IsEmpty()) {
            // NOTE: the part's own material array is read at the description's indices
            // without a range check (both builds)
            bool materialChanged = false;
            for (UInt32 u = 0;;) {
                if (description.m_materials[u] != part->m_materials.Begin()[u]) {
                    bool effective = true;
                    if (u != 0) {
                        const UInt32 material =
                            description.m_materials[u] != 0 ? description.m_materials[u] : description.m_materials[0];
                        changed = true;
                        if (material == part->GetMaterial(u))
                            effective = false;
                    }
                    if (effective)
                        materialChanged = changed = true;
                }
                materialsSame = !materialChanged;
                if (materialChanged || ++u >= description.m_materials.GetSize())
                    break;
            }
        }
        if (!changed) {
            // the decorations from face 1 on (NOTE: face 0 is not compared)
            const UInt32 faces = part->m_primitive->GetDecorationFaceCount();
            for (UInt32 u = 1; u < faces && !changed; ++u) {
                if (u < description.m_decorations.GetSize())
                    changed = description.m_decorations[u] != part->GetDecoration(u);
                else
                    changed = part->GetDecoration(u) != 0;
            }
        }
        undo->m_parts.PushBack(part);
        undo->m_descriptions.PushBack(PartDescription(*part));
        if (changed) {
            // NOTE: with an empty material description SetMaterials reads past it (both builds)
            part->SetMaterials(description.m_materials);
            part->SetDecorations(description.m_decorations);
            if (!materialsSame)
                for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator e = part->m_elements.Begin();
                     e != part->m_elements.End(); ++e) {
                    RigidElement* body = (*e)->m_rigid->m_rigidElement;
                    UInt32 k = 0;
                    while (k < bodies.GetSize() && bodies[k] != body)
                        ++k;
                    if (k == bodies.GetSize())
                        bodies.PushBack(body);
                }
        }
    }
    // the palette bricks of the changed bricks
    BasicArray<RefPointer<Brick> > changedBricks;
    BrickPalette* palette = BrickPaletteObserver::GetSubject();
    for (BasicArray<RefPointer<Part> >::ConstIterator p = parts.Begin(); p != parts.End(); ++p) {
        LDDBrick* brick = static_cast<LDDBrick*>((*p)->m_brick);
        RefPointer<PrimitiveElementBrick> key = new PrimitiveElementBrick(brick);
        const BrickDatabase& database = *m_sceneManager->GetDatabase();
        const BrickPalette::Brick* paletteBrick = palette->FindBrick(*key, database);
        if (paletteBrick == 0)
            paletteBrick = palette->FindBrick(*key, database, (BrickPaletteBuckets)1);
        if (brick->GetPaletteBrick() == paletteBrick)
            continue;
        brick->SetPaletteBrick(paletteBrick);
        UInt32 k = 0;
        while (k < changedBricks.GetSize() && changedBricks[k].Get() != brick)
            ++k;
        if (k == changedBricks.GetSize())
            changedBricks.PushBack(RefPointer<Brick>(brick));
    }
    if (!changedBricks.IsEmpty())
        BCSC_NOTIFY(OnEvent17(changedBricks));
    if (!undo->m_parts.IsEmpty()) {
        UndoManagerObserver::GetSubject()->AddAction(undo);
        UpdateMissingBricks();
    }
    m_scene->EndUpdate();
}

// Mac 0x40800, Win 0x6215bb
void BrickConnectSceneControl::SetBrickDescription(Brick* brick, const BrickPalette::Brick* paletteBrick)
{
    BasicArray<RefPointer<Part> > parts;
    BasicArray<PartDescription> descriptions;
    const BasicArray<RefPointer<Part> >& brickParts = brick->GetParts();
    for (BasicArray<RefPointer<Part> >::ConstIterator p = brickParts.Begin(); p != brickParts.End(); ++p) {
        for (UInt32 i = 0; i < paletteBrick->m_parts.GetSize(); ++i) {
            const BrickPalette::Part* palettePart = paletteBrick->m_parts[i];
            if ((*p)->GetDesignID() != palettePart->m_designID)
                continue;
            parts.PushBack(*p);
            PartDescription description;
            description.m_materials = palettePart->m_materials;
            description.m_decorations = palettePart->m_decorations;
            descriptions.PushBack(description);
            break;
        }
    }
    SetPartDescriptions(parts, descriptions);
}

// Mac 0x41dc0, Win 0x6216d5
void BrickConnectSceneControl::SetBrickDescription(Brick* brick, const PrimitiveElementBrick* snapshot)
{
    BasicArray<RefPointer<Part> > parts;
    BasicArray<PartDescription> descriptions;
    const BasicArray<RefPointer<Part> >& brickParts = brick->GetParts();
    for (BasicArray<RefPointer<Part> >::ConstIterator p = brickParts.Begin(); p != brickParts.End(); ++p) {
        for (UInt32 i = 0; i < snapshot->GetBrick()->m_parts.GetSize(); ++i) {
            if (snapshot->GetPalettePart(i)->m_designID != (*p)->GetDesignID())
                continue;
            parts.PushBack(*p);
            descriptions.PushBack(PartDescription(*snapshot, i));
            break;
        }
    }
    SetPartDescriptions(parts, descriptions);
}

// Mac 0x42120, Win 0x621806
void BrickConnectSceneControl::SetPartMaterial(Part* part, UInt32 material, UInt32 index)
{
    if (part->GetMaterial(index) == material)
        return;
    BasicArray<RefPointer<Part> > parts;
    BasicArray<PartDescription> descriptions;
    parts.PushBack(part);
    PartDescription description(*part);
    description.m_materials[index] = material;   // (no range check)
    descriptions.PushBack(description);
    SetPartDescriptions(parts, descriptions);
}

// Mac 0x423f0, Win 0x6218e6
void BrickConnectSceneControl::SetPartsMaterial(UInt32 material, const BasicArray<RefPointer<Part> >& parts)
{
    if (parts.IsEmpty())
        return;
    BasicArray<PartDescription> descriptions;
    for (BasicArray<RefPointer<Part> >::ConstIterator p = parts.Begin(); p != parts.End(); ++p) {
        PartDescription description(**p);
        description.m_materials[0] = material;   // NOTE: no check for a part without materials
        for (UInt32 i = 1; i < description.m_materials.GetSize(); ++i)
            description.m_materials[i] = 0;
        descriptions.PushBack(description);
    }
    SetPartDescriptions(parts, descriptions);
}

// Mac 0x425d0, Win 0x62199f
void BrickConnectSceneControl::SetSelectionMaterial(UInt32 material)
{
    SetPartsMaterial(material, m_selection);
}

// ---- missing bricks -----------------------------------------------------------------------------

// Mac 0x3cf20, Win 0x61e309 (thunk 0x620153)
void BrickConnectSceneControl::UpdateMissingBricks()
{
    m_missingBricks.Clear();
    if (!m_showMissingBricks)
        return;
    if (LDDApplication::GetInstance()->IsBrandLDDExtended())
        return;
    if (m_scene.IsNull())
        return;
    const BasicArray<RefPointer<Brick> >& bricks = m_scene->GetBricks();
    for (BasicArray<RefPointer<Brick> >::ConstIterator it = bricks.Begin(); it != bricks.End(); ++it)
        ++m_missingBricks.Std()[static_cast<const LDDBrick*>(it->Get())->GetPaletteBrick()];
    BCSC_NOTIFY(OnEvent16());
}

// ---- the frame (021e) ----------------------------------------------------------------------------

// Mac 0x49440, Win 0x61afcb
void BrickConnectSceneControl::Draw(ControlDrawArgs& args)
{
    const Int64 drawStart = GetMilliseconds();
    const bool boxes = m_navigation != 0 && !m_loading && m_navigation->BeginFrame(m_camera->GetViewMatrix());
    Render* render = args.m_render;
    args.Setup(true);
    if (!m_buffered) {
        render->SetClearColor(m_clearColor);
        render->Clear((m_skybox.IsNull() ? 1 : 0) | 6);
    }
    RenderQueue* overlayQueue = render->GetRenderQueue(0);
    RenderQueue* sceneQueue = render->GetRenderQueue(1);
    if (m_postProcessor.IsNull())
        sceneQueue = overlayQueue;
    render->SetViewMatrix(Matrix3D());
    render->SetLight(&m_lights[0].m_light, 0);
    render->SetLight(&m_lights[1].m_light, 1);
    render->SetLight(&m_lights[2].m_light, 2);
    render->SetViewMatrix(m_camera->GetViewMatrix());
    if (!m_postProcessor.IsNull())
        m_postProcessor->Begin(m_clearColor);   // Mac 0x495b1
    const Vector2& size = GetSize();
    if (!m_skybox.IsNull()) {
        const Real32 focalLength = m_camera->GetFocalLength();
        Real32 skyFocalLength = m_camera->GetFocalLength() * 0.25f;
        if (skyFocalLength < 12.0f)
            skyFocalLength = 12.0f;
        m_camera->SetFocalLength(skyFocalLength);
        m_camera->SetViewportSize(size.x, size.y);
        render->SetProjectionMatrix(m_camera->GetProjectionMatrix());
        m_skybox->Draw();
        m_camera->SetFocalLength(focalLength);
        m_camera->SetViewportSize(size.x, size.y);
    }
    render->SetProjectionMatrix(m_camera->GetProjectionMatrix());

    // developer mode: the axes and the measure line
    if (m_developerMode) {
        overlayQueue->Add(m_sceneManager->GetAxesMesh(), Matrix3D::Identity(), 0.0f, 0, 1.0f);
        if (!m_measureFeature1.IsNull() && !m_measureFeature2.IsNull() && !m_measureMesh.IsNull()) {
            Matrix3D transform1;
            Matrix3D transform2;
            m_measureFeature1->GetTransform(transform1);
            m_measureFeature2->GetTransform(transform2);
            Vector3 points[2];
            points[0] = m_measurePoint1 * transform1;
            points[1] = m_measurePoint2 * transform2;
            m_measureBuffer->SetPositions(points, 0, 2, 0);
            overlayQueue->Add(m_measureMesh, Matrix3D(), 0.0f, 0, 1.0f);
        }
    }
    // the scenery elements at their physics objects' transforms
    for (BasicArray<SceneryElement>::Iterator e = m_sceneryElements.Begin(); e != m_sceneryElements.End(); ++e) {
        if (e->m_meshes.IsEmpty())
            continue;
        Matrix3D objectTransform;
        static_cast<PhysicsSceneElement*>(e->m_object)->GetTransform(objectTransform);
        for (BasicArray<PlacedMesh>::Iterator m = e->m_meshes.Begin(); m != e->m_meshes.End(); ++m)
            overlayQueue->Add(m->m_mesh, Matrix3D(objectTransform, m->m_transform), 0.0f, 0, 1.0f);
    }
    m_grid->Draw(render, !m_postProcessor.IsNull());

    BasicArray<PlacedMesh> physicsMeshes;
    if (!m_loading) {
        if (m_field3c0) {
            // NOT restored (debug key 'p'): the physics scene's debug object
            // (PhysicsScene slot 4) as meshes (Mac 0x467d0) into the scene queue, released
        }
        RenderVisualDebugger::QueueDebugMeshes(overlayQueue, Matrix3D());
        // the flexible parts (world space)
        if (m_showStatistics && m_countTriangles && (m_cullingFlags & 2) != 0 && m_renderMode < 5) {
            for (BasicArray<LDDPart*>::Iterator it = m_flexParts.Begin(); it != m_flexParts.End(); ++it) {
                LDDPart* part = *it;
                if (part->m_flexTransforms.IsEmpty() || part->m_hidden)
                    continue;
                if (m_onlySelected && !ContainsPart(m_selectedParts, part))
                    continue;
                if (part->GetScene() != m_scene)
                    continue;
                for (BasicArray<LDDPart::FlexSurface>::Iterator s = part->m_flexSurfaces.Begin();
                     s != part->m_flexSurfaces.End(); ++s) {
                    sceneQueue->Add(s->m_mesh, Matrix3D(), 0.0f, 0, 1.0f);
                    if (!s->m_decalMesh.IsNull())
                        sceneQueue->Add(s->m_decalMesh, Matrix3D(), 0.0f, 0, 1.0f);
                }
            }
        }
        // NOTE: without bodies the wires and the selection are not drawn either (both builds)
        if (!m_bricks.IsEmpty()) {
            if (m_showStatistics) {
                if (boxes && m_countTriangles)
                    m_navigation->Queue(sceneQueue);   // instead of the body meshes below
                for (BasicArray<BIBrick>::Iterator b = m_bricks.Begin(); b != m_bricks.End(); ++b) {
                    Matrix3D bodyTransform;
                    if (b->m_body != 0)
                        b->m_body->GetTransform(bodyTransform);
                    if (m_countTriangles && !boxes) {
                        if (m_lodEnabled && !b->m_lodRegions.IsEmpty())
                            QueueLodRegions(sceneQueue, b->m_lodRegions, bodyTransform, m_camera->GetPosition(),
                                            m_lodReducedDistance2, m_lodBoxDistance2);
                        else
                        for (BasicArray<PlacedMesh>::Iterator m = b->m_meshes.Begin(); m != b->m_meshes.End(); ++m) {
                            Matrix3D transform(bodyTransform, m->m_transform);
                            Real32 depth = DistanceSquared(m->m_center * transform, m_camera->GetPosition());
                            SetCullBounds(sceneQueue, *m, transform);
                            sceneQueue->Add(m->m_mesh, transform, depth, m->m_layer, m->m_opacity);
                        }
                        for (BasicArray<PlacedMesh>::Iterator m = b->m_secondaryMeshes.Begin();
                             m != b->m_secondaryMeshes.End(); ++m) {
                            Matrix3D transform(bodyTransform, m->m_transform);
                            Real32 depth = DistanceSquared(m->m_center * transform, m_camera->GetPosition());
                            SetCullBounds(sceneQueue, *m, transform);
                            sceneQueue->AddWithDepthPrepass(m->m_mesh, transform, depth, m->m_layer,
                                                            m->m_opacity * m_field3cc, false);
                        }
                        for (BasicArray<PlacedMesh>::Iterator m = b->m_overlay20.Begin(); m != b->m_overlay20.End(); ++m) {
                            Matrix3D transform(bodyTransform, m->m_transform);
                            Real32 depth = DistanceSquared(m->m_center * transform, m_camera->GetPosition());
                            SetCullBounds(sceneQueue, *m, transform);
                            sceneQueue->AddTransparent(m->m_mesh, transform, depth, m->m_layer, m->m_opacity);
                        }
                    }
                    for (BasicArray<PlacedMesh>::Iterator m = b->m_overlay30.Begin(); m != b->m_overlay30.End(); ++m) {
                        Matrix3D transform(bodyTransform, m->m_transform);
                        Real32 depth = DistanceSquared(m->m_center * transform, m_camera->GetPosition());
                        SetCullBounds(overlayQueue, *m, transform);
                        overlayQueue->AddTransparent(m->m_mesh, transform, depth, m->m_layer, m->m_opacity);
                    }
                    for (BasicArray<PlacedMesh>::Iterator m = b->m_overlay40.Begin(); m != b->m_overlay40.End(); ++m) {
                        Matrix3D transform(bodyTransform, m->m_transform);
                        Real32 depth = DistanceSquared(m->m_center * transform, m_camera->GetPosition());
                        SetCullBounds(overlayQueue, *m, transform);
                        overlayQueue->Add(m->m_mesh, transform, depth, m->m_layer, 1.0f);
                    }
                }
                QueueWireMeshes(overlayQueue);
            }
            // the selection: its box mesh, or the scene manager's unit box per selected part
            if (!m_selectionMesh.IsNull()) {
                overlayQueue->Add(m_selectionMesh, Matrix3D(), 2.0f, 0, 1.0f);
            } else {
                for (BasicArray<RefPointer<Part> >::ConstIterator p = m_selection.Begin(); p != m_selection.End(); ++p) {
                    Matrix3D transform;
                    GetPartBoxTransform(*p, transform, false);
                    overlayQueue->Add(m_sceneManager->GetSelectionBoxMesh(), transform, 2.0f, 0, 1.0f);
                }
            }
        }
    }
    // the tools add theirs
    BCSC_NOTIFY(OnEvent05(Matrix3D::Identity(), render, overlayQueue, sceneQueue));
    // skip the meshes outside the view (RenderQueue.cpp), with the camera's projection at this
    // viewport
    sceneQueue->EnableCulling(m_camera->GetViewMatrix(), m_camera->GetFieldOfViewX(),
                              m_camera->GetFieldOfViewY(size.x, size.y), m_camera->GetNear(),
                              m_camera->GetFar());
    overlayQueue->EnableCulling(m_camera->GetViewMatrix(), m_camera->GetFieldOfViewX(),
                                m_camera->GetFieldOfViewY(size.x, size.y), m_camera->GetNear(),
                                m_camera->GetFar());
    sceneQueue->Flush(0.02f, true, true);
    if (!m_postProcessor.IsNull()) {
        // Mac 0x4a4ad-0x4a5b4
        m_postProcessor->Apply(m_size, sceneQueue, m_buffer, &m_lights[0].m_light,
                               &m_lights[1].m_light, &m_lights[2].m_light,
                               m_camera->GetProjectionMatrix(), m_camera->GetFieldOfViewX(),
                               m_size.x / m_size.y, m_camera->GetNear(), m_camera->GetFar());
    }
    sceneQueue->Clear();
    overlayQueue->Flush(0.02f, true, true);
    overlayQueue->Clear();
    sceneQueue->DisableCulling();
    overlayQueue->DisableCulling();
    BCSC_NOTIFY(OnEvent06(render));
    render->SetLight(0, 0);
    render->SetLight(0, 1);
    render->SetLight(0, 2);
    args.Setup(false);
    render->Clear(4);
    BCSC_NOTIFY(OnEvent07(render, args));
    m_status.m_invalid = 0;
    if (m_navigation != 0 && !boxes && !m_loading)
        m_navigation->EndFullFrame(GetMilliseconds() - drawStart);
}

// ---- undo actions (021h constructors; Execute bodies emitted in 021d) ---------------------------

// Mac 0x3fc60, Win 0x616f92
UndoAddGroup::UndoAddGroup(BrickConnectSceneControl* control, const BasicArray<RefPointer<Part> >& parts)
    : m_control(control)
{
    CollectGroups(parts, m_parts, m_groups);
}

// Mac 0x54890, Win 0x619c40 (→ 0x617ed9)
void UndoAddGroup::Execute()
{
    m_control->UnGroup(m_group, m_parts, m_groups);
}

// Mac 0x3ff00, Win 0x61711a
UndoUnGroup::UndoUnGroup(BrickConnectSceneControl* control, BrickGroup* group)
    : m_control(control), m_group(group), m_parent(group->m_parent), m_parts(group->m_parts),
      m_groups(group->m_groups)
{
}

// Mac 0x54d00, Win 0x6171f6
void UndoUnGroup::Execute()
{
    m_control->ReGroup(m_group, m_parent, m_parts, m_groups);
}

// Mac 0x55320, Win 0x619c92
void UndoReGroup::Execute()
{
    m_control->UnGroup(m_group);
}

// Mac 0x40610
UndoAddParts::UndoAddParts(BrickConnectSceneControl* control, const BasicArray<RefPointer<Part> >& parts)
    : m_control(control)
{
    CollectGroups(parts, m_parts, m_groups);
}

// Mac 0x555a0, Win 0x619c9e
void UndoAddParts::Execute()
{
    UndoManager* undo = m_control->UndoManagerObserver::GetSubject();
    undo->AddAction(new UndoAddParts(m_control, m_parts));
    BasicArray<Part*> rawParts;
    BasicArray<BrickGroup*> rawGroups;
    ToRaw(m_parts, rawParts);
    ToRaw(m_groups, rawGroups);
    m_control->GetScene()->SetGroups(rawParts, rawGroups);
}

// Mac 0x55e50, Win 0x62091d
void UndoPartDescription::Execute()
{
    m_control->SetPartDescriptions(m_parts, m_descriptions);
}

// Mac 0x50f10, Win 0x6196ff
void UndoMoveSnapshot::Execute()
{
    m_control->SetTransforms(m_transforms, true);
}

// Mac 0x55930, Win 0x61e1cc
void UndoInsertSelection::Execute()
{
    m_control->RemoveBricks(m_bricks);
}

// Mac 0x56300, Win 0x62092e
void UndoRemoveSelection::Execute()
{
    m_scene->BeginUpdate();
    m_control->InsertBricks(m_bricks);
    for (UInt32 i = 0; i < m_groups.GetSize(); ++i)
        m_groups[i].first->SetGroup(m_groups[i].second);
    m_scene->EndUpdate();
}

// =============================================================================================
// undo actions (constructors; Execute and the destructors are the deferred tail 021d)
// =============================================================================================

// Mac 0x39070
UndoMoveSnapshot::UndoMoveSnapshot(BrickConnectSceneControl* control,
                                   const BasicArray<Pair<PrimitiveElement*, Matrix3D> >& entries,
                                   bool fromScene)
    : m_control(control)
{
    if (fromScene)
        Scene::GetTransforms(entries, m_transforms);
    else
        m_transforms = entries;
}

// Mac 0x453c0
UndoInsertSelection::UndoInsertSelection(BrickConnectSceneControl* control,
                                         const BasicArray<RefPointer<Brick> >& bricks)
    : m_control(control), m_bricks(bricks)
{
}

// Mac 0x4d370
UndoRemoveSelection::UndoRemoveSelection(BrickConnectSceneControl* control,
                                         const BasicArray<RefPointer<Brick> >& bricks)
    : m_control(control), m_bricks(bricks), m_scene(control->GetScene())
{
    m_scene->BeginUpdate();
    for (BasicArray<RefPointer<Brick> >::ConstIterator b = bricks.Begin(); b != bricks.End(); ++b) {
        const BasicArray<RefPointer<Part> >& parts = (*b)->GetParts();
        for (BasicArray<RefPointer<Part> >::ConstIterator p = parts.Begin(); p != parts.End(); ++p) {
            Part* part = *p;
            BrickGroup* group = part->GetGroup();
            if (group == 0)
                continue;
            RefPointer<BrickGroup> held = group;
            m_groups.PushBack(Pair<Part*, RefPointer<BrickGroup> >(part, held));
            part->SetGroup(0);
        }
    }
    m_scene->EndUpdate();
}

// =============================================================================================
// render data
// =============================================================================================

// Mac 0x34b80 (Win: inlined into vf38)
void BrickConnectSceneControl::AddFlexPart(Part* part)
{
    if (part->m_elements.GetSize() < 2)
        return;
    LDDPart* flexPart = static_cast<LDDPart*>(part);
    for (BasicArray<LDDPart*>::Iterator it = m_flexParts.Begin(); it != m_flexParts.End(); ++it)
        if (*it == flexPart)
            return;
    m_flexParts.PushBack(flexPart);
}

// Mac 0x39df0, Win 0x622104
void BrickConnectSceneControl::RebuildRenderData()
{
    m_bricks.Clear();
    srand(1);
    Matrix3D identity;
    (void)identity;
    // the materials of the debug modes 1-4 (random colours per element / body / hinged group)
    RefPointer<Material> green = NewDebugMaterial(Color::Green());
    RefPointer<Material> red = NewDebugMaterial(Color::Red());
    RefPointer<Material> debugMaterial;

    const BasicArray<RefPointer<HingedElement> >& hinged = m_scene->GetHingedElements();
    for (UInt32 h = 0; h < hinged.GetSize(); ++h) {
        HingedElement* group = hinged[h];
        if (m_renderMode == 3) {
            // NOT restored: random colour per hinged group (Mac 0x46370, Win 0x6124dc; with
            // UseRoundEdges a shader material, BCSM::CreateShaderMaterial)
        }
        for (UInt32 r = 0; r < group->m_elements.GetSize(); ++r) {
            RigidElement* body = group->m_elements[r];
            if (m_renderMode == 2) {
                // NOT restored: random colour per body (Mac 0x46370)
            }
            BIBrick brick;
            brick.m_body = body;
            brick.m_field54 = 0;
            if (m_renderMode == 0) {
                BuildBodyMeshes(brick, 0);
            } else if (m_renderMode == 5 || m_renderMode == 6) {
                // NOT restored: the physics debug geometry of the body (Scene::GetDebugInfo with
                // flags 2/4 (+0x400), 0x467d0 overlay vertex buffers)
            } else {
                // NOT restored: modes 1-4, one mesh per primitive element (BCSM vertex buffers,
                // random colours for mode 1, IsFullyConnected for mode 4)
            }
            if (m_debugMode3c8 != 0) {
                // NOT restored: Mac 0x47980 (debug info overlay)
            }
            if (m_showOverlay) {
                // NOT restored: Mac 0x47a50 (overlay meshes of the body)
            }
            if (brick.HasMeshes())
                m_bricks.PushBack(brick);
        }
    }
    if (m_meshBufferCache != 0)
        m_meshBufferCache->Prune();
    UpdateFlexParts();
    UpdateStatistics();
    Redraw();
}

// Mac 0x46560, Win 0x622082 (+ 0x61325f / 0x613218)
void BrickConnectSceneControl::BuildBodyMeshes(BIBrick& brick, Material* material)
{
    if (m_lodEnabled) {
        BuildLodRegions(brick, material);
        return;
    }
    GeometryAccum::ImportPrimitiveMap& imports = m_sceneManager->GetImportPrimitives();
    GeometryAccum accum;
    RigidElement* body = brick.m_body;
    const UInt32 flags = m_cullingFlags;
    Matrix3D identity;
    for (UInt32 i = 0; i < body->m_elements.GetSize(); ++i) {
        PrimitiveRigidElement* rigid = body->m_elements[i];
        Matrix3D rigidTransform(identity, rigid->m_transform);
        for (UInt32 e = 0; e < rigid->m_elements.GetSize(); ++e) {
            PrimitiveElement* element = rigid->m_elements[e];
            Matrix3D transform(rigidTransform, element->m_transform);
            GeometryAccum::AddElement(imports, element, flags, transform, accum);
        }
    }
    FilterPatches(accum);
    if (accum.m_materials.GetSize() != 0) {
        Vector3 center = brick.m_body->m_bounding.GetCenter();
        CreateAccumulatedMeshes(accum, material, brick, false, center);
    }
}

// The body's elements by region (kLodRegionSize), per region the meshes at full detail (also
// appended to m_meshes), without studs and tubes (GeometryAccum::kFlagFarDetail) and, with the
// boxes option, as boxes.
void BrickConnectSceneControl::BuildLodRegions(BIBrick& brick, Material* material)
{
    struct Region {
        BasicArray<PrimitiveElement*> elements;
        BasicArray<Matrix3D>          transforms;
        Vector3                       sum;
    };
    GeometryAccum::ImportPrimitiveMap& imports = m_sceneManager->GetImportPrimitives();
    RigidElement* body = brick.m_body;
    std::map<std::tuple<Int32, Int32, Int32>, Region> regions;
    Matrix3D identity;
    for (UInt32 i = 0; i < body->m_elements.GetSize(); ++i) {
        PrimitiveRigidElement* rigid = body->m_elements[i];
        Matrix3D rigidTransform(identity, rigid->m_transform);
        for (UInt32 e = 0; e < rigid->m_elements.GetSize(); ++e) {
            PrimitiveElement* element = rigid->m_elements[e];
            Matrix3D transform(rigidTransform, element->m_transform);
            Vector3 origin = Vector3(0.0f, 0.0f, 0.0f) * transform;
            std::tuple<Int32, Int32, Int32> key((Int32)std::floor(origin.x / kLodRegionSize),
                                                (Int32)std::floor(origin.y / kLodRegionSize),
                                                (Int32)std::floor(origin.z / kLodRegionSize));
            Region& region = regions[key];
            region.elements.PushBack(element);
            region.transforms.PushBack(transform);
            region.sum = region.sum + origin;
        }
    }
    for (std::map<std::tuple<Int32, Int32, Int32>, Region>::iterator it = regions.begin(); it != regions.end(); ++it) {
        Region& region = it->second;
        BIBrick::LodRegion lod;
        lod.center = region.sum * (1.0f / (Real32)region.elements.GetSize());
        for (int pass = 0; pass < 2; ++pass) {
            const UInt32 flags = pass == 0 ? m_cullingFlags : (m_cullingFlags | GeometryAccum::kFlagFarDetail);
            GeometryAccum accum;
            for (UInt32 e = 0; e < region.elements.GetSize(); ++e)
                GeometryAccum::AddElement(imports, region.elements[e], flags, region.transforms[e], accum);
            FilterPatches(accum);
            if (accum.m_materials.GetSize() == 0)
                continue;
            BIBrick meshes;
            CreateAccumulatedMeshes(accum, material, meshes, false, lod.center);
            (pass == 0 ? lod.full : lod.reduced) = meshes.m_meshes;
        }
        if (m_lodBoxes)
            BuildLodBoxes(lod, region.elements, region.transforms);
        for (BasicArray<PlacedMesh>::Iterator m = lod.full.Begin(); m != lod.full.End(); ++m)
            brick.m_meshes.PushBack(*m);
        brick.m_lodRegions.PushBack(lod);
    }
}

// One box per element (its primitive's vertex box, at its body transform), one mesh per material
// of face 0
void BrickConnectSceneControl::BuildLodBoxes(BIBrick::LodRegion& region,
                                             const BasicArray<PrimitiveElement*>& elements,
                                             const BasicArray<Matrix3D>& transforms)
{
    GeometryAccum::ImportPrimitiveMap& imports = m_sceneManager->GetImportPrimitives();
    std::map<UInt32, LodBoxArrays> byMaterial;
    for (UInt32 e = 0; e < elements.GetSize(); ++e) {
        const PrimitiveElement* element = elements[e];
        if (element->m_flagAC)
            continue;
        if (m_onlySelected && !ContainsPart(m_selectedParts, element->m_part))
            continue;
        GeometryAccum::ImportPrimitiveMap::Iterator it = imports.Find(element->m_primitive->GetDesignID());
        if (it == imports.End())
            continue;
        bool any = false;
        Vector3 lo, hi;
        for (UInt32 g = 0; g < it->second.geometries.GetSize(); ++g) {
            const GeometryAccum::ImportGeometry& geometry = it->second.geometries[g];
            const Vector3* positions = (const Vector3*)geometry.positions;
            for (UInt32 v = 0; positions != 0 && v < geometry.vertexCount; ++v) {
                const Vector3& p = positions[v];
                if (!any) {
                    lo = hi = p;
                    any = true;
                    continue;
                }
                lo.x = Min(lo.x, p.x); lo.y = Min(lo.y, p.y); lo.z = Min(lo.z, p.z);
                hi.x = Max(hi.x, p.x); hi.y = Max(hi.y, p.y); hi.z = Max(hi.z, p.z);
            }
        }
        if (any)
            AddLodBox(byMaterial[element->m_part->GetMaterial(0)], lo, hi, transforms[e]);
    }
    Render* render = GetRenderControlManager()->GetRender();
    for (std::map<UInt32, LodBoxArrays>::iterator it = byMaterial.begin(); it != byMaterial.end(); ++it) {
        LodBoxArrays& arrays = it->second;
        const UInt32 vertexCount = (UInt32)arrays.positions.size();
        const UInt32 indexCount = (UInt32)arrays.indices.size();
        VertexBuffer* buffer = 0;
        MeshBufferCache::Contents contents = {vertexCount, indexCount, &arrays.positions[0], &arrays.normals[0], 0,
                                              &arrays.indices[0], false, 0, 0, 0, 0, 0, 0};
        UInt64 hash = MeshBufferCache::Hash(contents);
        if (m_meshBufferCache == 0)
            m_meshBufferCache = new MeshBufferCache;
        buffer = m_meshBufferCache->Find(contents, hash);
        RefPointer<VertexBuffer> keep;
        if (buffer == 0) {
            VertexBuffer::CreateArgs args;
            args.vertexCount = vertexCount;
            args.indexCount = indexCount;
            args.frameCount = 0;
            args.dynamic = false;
            args.hasNormals = true;
            args.colorSetCount = 0;
            Object* object = render->GetObjectFactory()->CreateObject(typeid(VertexBuffer), args);
            buffer = object != 0 ? dynamic_cast<VertexBuffer*>(object) : 0;
            if (buffer == 0)
                continue;
            keep = buffer;
            buffer->SetPositions(&arrays.positions[0], 0, vertexCount, 0);
            buffer->SetNormals(&arrays.normals[0], 0, vertexCount);
            buffer->SetIndices(&arrays.indices[0], 0, indexCount, 0);
            m_meshBufferCache->Add(buffer, hash);
        }
        Material* boxMaterial;
        if (m_wireframe) {
            boxMaterial = m_sceneManager->CreateWireframeMaterial();
        } else {
            Material* source = m_sceneManager->GetMaterial(it->first, 0);
            RefPointer<Material>& plain = m_lodBoxMaterials[source];
            if (plain.IsNull())
                plain = NewLodBoxMaterial(source);
            boxMaterial = plain;
        }
        RefPointer<Mesh> mesh = new SingleMesh(boxMaterial, buffer);
        region.boxes.PushBack(PlacedMesh(mesh, region.center, Matrix3D(), 0, 1.0f));
    }
}

// The level-of-detail settings; with `rebuild`, the meshes are rebuilt when what is built changed,
// else the view is redrawn.
void BrickConnectSceneControl::ReadLodPreferences(bool rebuild)
{
    bool enabled = false;
    bool boxes = false;
    ReadBoolPreference(String("LevelOfDetail"), enabled);
    ReadBoolPreference(String("LevelOfDetailBoxes"), boxes);
    Real32 distance = kLodDefaultDistance;
    {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        String text;
        Real32 value = 0.0f;
        if (preferences->GetValue(String("LevelOfDetailDistance"), text) && FromString(value, text) &&
            value >= kLodMinimumDistance && value <= kLodMaximumDistance)
            distance = value;
    }
    const bool changed = enabled != m_lodEnabled || (enabled && boxes != m_lodBoxes);
    m_lodEnabled = enabled;
    m_lodBoxes = boxes;
    m_lodReducedDistance2 = distance * distance;
    m_lodBoxDistance2 = boxes ? 4.0f * distance * distance : 0.0f;
    if (!rebuild)
        return;
    if (changed && m_renderMode == 0) {
        // as UpdateRenderData rebuilds a changed body
        for (BasicArray<BIBrick>::Iterator b = m_bricks.Begin(); b != m_bricks.End(); ++b) {
            if (b->m_body == 0)
                continue;
            b->ClearMeshes();
            BuildBodyMeshes(*b, 0);
        }
        if (m_meshBufferCache != 0)
            m_meshBufferCache->Prune();
        if (!m_lodEnabled)
            m_lodBoxMaterials.Clear();
        UpdateStatistics();
    }
    Redraw();
}

// Mac 0x4d140, Win 0x619e2e
void BrickConnectSceneControl::FilterPatches(GeometryAccum& accum)
{
    if (!m_onlySelected)
        return;
    for (GeometryAccum::PatchMap::Iterator it = accum.m_materials.Begin(); it != accum.m_materials.End(); ++it) {
        GeometryAccum::AccumulatedPatch& patches = it->second;
        for (BasicArray<GeometryAccum::Patch>::Iterator p = patches.patches.Begin(); p != patches.patches.End(); ++p) {
            const Part* part = p->element->m_part;
            bool selected = false;
            for (UInt32 s = 0; s < m_selectedParts.GetSize(); ++s)
                if (m_selectedParts[s].Get() == part) {
                    selected = true;
                    break;
                }
            if (!selected) {
                patches.vertexCount -= p->vertexCount;
                patches.indexCount -= p->indexCount;
                p->indexCount = 0;
                p->vertexCount = 0;
            }
        }
    }
}

// Mac 0x4f6a0, Win 0x61bb85 / 0x61a0e5
void BrickConnectSceneControl::CreateAccumulatedMeshes(GeometryAccum& accum, Material* material,
                                                       BIBrick& brick, bool secondary,
                                                       const Vector3& center)
{
    Render* render = GetRenderControlManager()->GetRender();
    BasicArray<PlacedMesh>& placed = secondary ? brick.m_secondaryMeshes : brick.m_meshes;

    // ---- one buffer per material ----
    for (GeometryAccum::PatchMap::Iterator it = accum.m_materials.Begin(); it != accum.m_materials.End(); ++it) {
        BasicArray<GeometryAccum::Patch>& patches = it->second.patches;
        UInt32 stamp = m_sceneManager->NextImportStamp();
        if (patches.IsEmpty())
            continue;
        UInt32 roundEdgePointCount = 0;
        UInt32 roundEdgeDataSize = 0;
        UInt32 vertexCount = 0;
        UInt32 indexCount = 0;
        for (BasicArray<GeometryAccum::Patch>::Iterator p = patches.Begin(); p != patches.End(); ++p) {
            vertexCount += p->vertexCount;
            indexCount += p->indexCount;
            if (LDDApplication::GetInstance()->UseRoundEdges() && p->geometry->field40 != stamp) {
                roundEdgePointCount += p->geometry->roundEdgePointCount;
                roundEdgeDataSize += p->geometry->roundEdgeDataSize;
                p->geometry->field40 = stamp;
            }
        }
        if (vertexCount == 0)
            continue;
        stamp = m_sceneManager->NextImportStamp();
        VertexBuffer* buffer = 0;
        // NOTE: no null check of the buffer (both builds)
        Vector3* positions = new Vector3[vertexCount];
        Vector3* normals = new Vector3[vertexCount];
        Vector3* roundEdgePoints = 0;
        UInt32* roundEdgeData = 0;
        UInt32* roundEdgePointIndices = 0;
        UInt32* roundEdgeDataIndices = 0;
        if (LDDApplication::GetInstance()->UseRoundEdges()) {
            roundEdgePoints = new Vector3[roundEdgePointCount];
            // NOTE: the alignment padding of each geometry's data stays uninitialised (both
            // builds); fixed: zero (low)
            roundEdgeData = new UInt32[roundEdgeDataSize]();
            roundEdgePointIndices = new UInt32[indexCount];
            roundEdgeDataIndices = new UInt32[indexCount];
        }
        UInt32* indices = new UInt32[indexCount];
        GeometryAccum::Output out;
        out.positions = positions;
        out.normals = normals;
        out.texCoords = 0;
        out.roundEdgeData = roundEdgeData;
        out.roundEdgePoints = roundEdgePoints;
        out.roundEdgeDataIndices = roundEdgeDataIndices;
        out.roundEdgePointIndices = roundEdgePointIndices;
        out.indices = indices;
        for (BasicArray<GeometryAccum::Patch>::Iterator p = patches.Begin(); p != patches.End(); ++p)
            GeometryAccum::WritePatch(*p, out, stamp);
        // the buffer of a body with the same contents, else a new one (shared from now on)
        MeshBufferCache::Contents contents = {vertexCount, indexCount, positions, normals, 0, indices,
                                              LDDApplication::GetInstance()->UseRoundEdges(), roundEdgePointCount,
                                              roundEdgeDataSize, roundEdgePoints, roundEdgeData,
                                              roundEdgePointIndices, roundEdgeDataIndices};
        UInt64 hash = MeshBufferCache::Hash(contents);
        if (m_meshBufferCache == 0)
            m_meshBufferCache = new MeshBufferCache;
        buffer = m_meshBufferCache->Find(contents, hash);
        if (buffer == 0) {
            buffer = NewAccumulatedBuffer(render, vertexCount, indexCount, 0, roundEdgeDataSize, roundEdgePointCount);
        buffer->SetPositions(positions, 0, vertexCount, 0);
        buffer->SetNormals(normals, 0, vertexCount);
        if (LDDApplication::GetInstance()->UseRoundEdges()) {
            buffer->SetRoundEdgePoints(roundEdgePoints, roundEdgePointCount);
            buffer->SetRoundEdgeData(roundEdgeData, roundEdgeDataSize);
            buffer->SetRoundEdgePointIndices(roundEdgePointIndices, indexCount);
            buffer->SetRoundEdgeDataIndices(roundEdgeDataIndices, indexCount);
        }
        buffer->SetIndices(indices, 0, indexCount, 0);
            m_meshBufferCache->Add(buffer, hash);
        }
        delete[] positions;
        delete[] normals;
        if (LDDApplication::GetInstance()->UseRoundEdges()) {
            delete[] roundEdgePoints;
            delete[] roundEdgeData;
            delete[] roundEdgePointIndices;
            delete[] roundEdgeDataIndices;
        }
        delete[] indices;

        Material* meshMaterial;
        if (m_wireframe)
            meshMaterial = m_sceneManager->CreateWireframeMaterial();
        else if (material != 0)
            meshMaterial = material;
        else
            meshMaterial = m_sceneManager->GetMaterial(it->first, 0);
        RefPointer<Mesh> mesh = new SingleMesh(meshMaterial, buffer);
        placed.PushBack(PlacedMesh(mesh, center, Matrix3D(), 0, 1.0f));
    }

    // ---- one buffer per decoration (texture coordinates, decal material) ----
    for (GeometryAccum::PatchMap::Iterator it = accum.m_decorations.Begin(); it != accum.m_decorations.End(); ++it) {
        BasicArray<GeometryAccum::Patch>& patches = it->second.patches;
        BasicArray<UInt32> texCoordDimensions;
        texCoordDimensions.PushBack(2);
        UInt32 stamp = m_sceneManager->NextImportStamp();
        UInt32 roundEdgePointCount = 0;
        UInt32 roundEdgeDataSize = 0;
        UInt32 vertexCount = 0;
        UInt32 indexCount = 0;
        for (BasicArray<GeometryAccum::Patch>::Iterator p = patches.Begin(); p != patches.End(); ++p) {
            vertexCount += p->vertexCount;
            indexCount += p->indexCount;
            if (LDDApplication::GetInstance()->UseRoundEdges() && p->geometry->field40 != stamp) {
                roundEdgePointCount += p->geometry->roundEdgePointCount;
                roundEdgeDataSize += p->geometry->roundEdgeDataSize;
                p->geometry->field40 = stamp;
            }
        }
        if (vertexCount == 0)
            continue;
        stamp = m_sceneManager->NextImportStamp();
        VertexBuffer* buffer = 0;
        Vector3* positions = new Vector3[vertexCount];
        Vector3* normals = new Vector3[vertexCount];
        Vector3* roundEdgePoints = 0;
        UInt32* roundEdgeData = 0;
        UInt32* roundEdgePointIndices = 0;
        UInt32* roundEdgeDataIndices = 0;
        if (LDDApplication::GetInstance()->UseRoundEdges()) {
            roundEdgePoints = new Vector3[roundEdgePointCount];
            roundEdgeData = new UInt32[roundEdgeDataSize];
            roundEdgePointIndices = new UInt32[indexCount];
            roundEdgeDataIndices = new UInt32[indexCount];
        }
        Vector2* texCoords = new Vector2[vertexCount];
        UInt32* indices = new UInt32[indexCount];
        GeometryAccum::Output out;
        out.positions = positions;
        out.normals = normals;
        out.texCoords = texCoords;
        out.roundEdgeData = roundEdgeData;
        out.roundEdgePoints = roundEdgePoints;
        out.roundEdgeDataIndices = roundEdgeDataIndices;
        out.roundEdgePointIndices = roundEdgePointIndices;
        out.indices = indices;
        for (BasicArray<GeometryAccum::Patch>::Iterator p = patches.Begin(); p != patches.End(); ++p)
            GeometryAccum::WritePatch(*p, out, stamp);
        MeshBufferCache::Contents contents = {vertexCount, indexCount, positions, normals, texCoords, indices,
                                              LDDApplication::GetInstance()->UseRoundEdges(), roundEdgePointCount,
                                              roundEdgeDataSize, roundEdgePoints, roundEdgeData,
                                              roundEdgePointIndices, roundEdgeDataIndices};
        UInt64 hash = MeshBufferCache::Hash(contents);
        if (m_meshBufferCache == 0)
            m_meshBufferCache = new MeshBufferCache;
        buffer = m_meshBufferCache->Find(contents, hash);
        RefPointer<VertexBuffer> keep(buffer);
        if (buffer == 0) {
            buffer = NewAccumulatedBuffer(render, vertexCount, indexCount, &texCoordDimensions, roundEdgeDataSize,
                                          roundEdgePointCount);
            keep = buffer;
        buffer->SetPositions(positions, 0, vertexCount, 0);
        buffer->SetNormals(normals, 0, vertexCount);
        if (LDDApplication::GetInstance()->UseRoundEdges()) {
            buffer->SetRoundEdgePoints(roundEdgePoints, roundEdgePointCount);
            buffer->SetRoundEdgeData(roundEdgeData, roundEdgeDataSize);
            buffer->SetRoundEdgePointIndices(roundEdgePointIndices, indexCount);
            buffer->SetRoundEdgeDataIndices(roundEdgeDataIndices, indexCount);
        }
        buffer->SetTexCoords((const Real32*)texCoords, 0);
        buffer->SetIndices(indices, 0, indexCount, 0);
            m_meshBufferCache->Add(buffer, hash);
        }
        delete[] positions;
        delete[] normals;
        if (LDDApplication::GetInstance()->UseRoundEdges()) {
            delete[] roundEdgePoints;
            delete[] roundEdgeData;
            delete[] roundEdgePointIndices;
            delete[] roundEdgeDataIndices;
        }
        delete[] texCoords;
        delete[] indices;

        Material* decal = m_sceneManager->GetDecalMaterial(it->first);
        if (decal != 0) {
            RefPointer<Mesh> mesh = new SingleMesh(decal, buffer);
            placed.PushBack(PlacedMesh(mesh, center, Matrix3D(), 0, 1.0f));
        }
    }
}

// Mac 0x48320, Win 0x619439
void BrickConnectSceneControl::UpdateFlexParts()
{
    Render* render = GetRenderControlManager()->GetRender();
    for (BasicArray<LDDPart*>::Iterator it = m_flexParts.Begin(); it != m_flexParts.End(); ++it) {
        LDDPart* part = *it;
        BasicArray<Matrix3D>& bones = part->m_flexTransforms;
        BasicArray<LDDPart::FlexSurface>& surfaces = part->m_flexSurfaces;
        if (bones.IsEmpty()) {
            // first time: the bone transforms of the elements, then one surface per material face
            bones.Resize(part->m_elements.GetSize(), Matrix3D());
            for (UInt32 i = 0; i < part->m_elements.GetSize(); ++i)
                part->m_elements[i]->GetTransform(bones[i]);
            const UInt32 faces = part->m_materials.GetSize();
            if (faces == 0)
                continue;
            for (UInt32 face = 0; face < faces; ++face) {
                LDDPart::FlexSurface surface;
                RefPointer<Material> material = m_sceneManager->GetMaterial(part->GetMaterial(face), 0);
                bool failed = material.IsNull();
                if (!failed) {
                    surface.m_geometry = m_sceneManager->GetFlexGeometry(part->m_primitive, face);
                    failed = surface.m_geometry.IsNull() ||
                             !CreateFlexVertexBuffer(part->m_primitive, surface.m_geometry, bones, render,
                                                     true, surface.m_vertexBuffer);
                }
                if (!failed) {
                    surface.m_mesh = new SingleMesh(material, surface.m_vertexBuffer);
                    if (face != 0 && part->GetDecoration(face) != 0) {
                        material = m_sceneManager->GetDecalMaterial(part->GetDecoration(face));
                        if (material.IsNull())
                            failed = true;
                        else
                            surface.m_decalMesh = new SingleMesh(material, surface.m_vertexBuffer);
                    }
                }
                if (failed) {
                    // drop everything of the part (it is tried again on the next rebuild)
                    bones.Clear();
                    surfaces.Clear();
                    break;
                }
                surfaces.PushBack(surface);
            }
        } else if (!surfaces.IsEmpty()) {
            // the bones moved: re-skin every surface
            for (UInt32 i = 0; i < part->m_elements.GetSize(); ++i)
                part->m_elements[i]->GetTransform(bones[i]);
            for (BasicArray<LDDPart::FlexSurface>::Iterator s = surfaces.Begin(); s != surfaces.End(); ++s)
                UpdateFlexVertexBuffer(part->m_primitive, s->m_geometry, bones, s->m_vertexBuffer);
        }
    }
}

// Mac 0x488e0, Win 0x61ce6b
void BrickConnectSceneControl::RebuildPartRenderData()
{
    BuildRenderDataForParts(m_field48c, BasicArray<RefPointer<Part> >());
}

// Mac 0x48940, Win 0x61c369
void BrickConnectSceneControl::BuildRenderDataForParts(const BasicArray<RefPointer<Part> >& parts,
                                                       const BasicArray<RefPointer<Part> >& highlighted)
{
    m_bricks.Clear();
    for (BasicArray<RefPointer<Part> >::ConstIterator it = parts.Begin(); it != parts.End(); ++it) {
        Part* part = *it;
        if (part->m_elements.GetSize() == 1) {
            BIBrick brick;
            bool isHighlighted = false;
            {
                RefPointer<Part> key(part);
                for (BasicArray<RefPointer<Part> >::ConstIterator h = highlighted.Begin();
                     h != highlighted.End(); ++h)
                    if (*h == key) {
                        isHighlighted = true;
                        break;
                    }
            }
            PrimitiveElement* element = part->m_elements[0];
            Matrix3D transform;
            element->GetTransform(transform);
            BoundingBox box(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
            BasicArray<RefPointer<Mesh> > meshes;
            BrickDraw::BuildMeshes(part->GetDesignID(), part->m_materials, part->m_decorations,
                                   m_sceneManager, meshes, box, 0);
            BasicArray<PlacedMesh>& placed = isHighlighted ? brick.m_overlay20 : brick.m_meshes;
            for (BasicArray<RefPointer<Mesh> >::Iterator m = meshes.Begin(); m != meshes.End(); ++m) {
                // depth reference: the centre of the element's bone box (not of `box`)
                const Primitive* primitive = element->GetPrimitive();
                Vector3 center = primitive->m_bones[element->GetBoneIndex()].m_bounding.GetCenter();
                placed.PushBack(PlacedMesh(*m, center, transform, 0, 1.0f));
            }
            m_bricks.PushBack(brick);
        } else {
            // NOTE: the bone transforms are collected but not used: the original calls the rigid
            // BrickDraw::BuildMeshes (Mac 0x231d10), which skins a flexible primitive with no
            // bone transforms (its rest pose).
            BasicArray<Matrix3D> bones;
            Matrix3D bone;
            for (BasicArray<RefPointer<PrimitiveElement> >::Iterator e = part->m_elements.Begin();
                 e != part->m_elements.End(); ++e) {
                (*e)->GetTransform(bone);
                bones.PushBack(bone);
            }
            BoundingBox box(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
            BasicArray<RefPointer<Mesh> > meshes;
            BrickDraw::BuildMeshes(part->GetDesignID(), part->m_materials, part->m_decorations,
                                   m_sceneManager, meshes, box, 0);
            BIBrick brick;
            for (BasicArray<RefPointer<Mesh> >::Iterator m = meshes.Begin(); m != meshes.End(); ++m)
                brick.m_meshes.PushBack(PlacedMesh(*m, box.GetCenter(), Matrix3D(), 0, 1.0f));
            m_bricks.PushBack(brick);
        }
    }
    Redraw();
}

// ---------------------------------------------------------------------------------------------
// pictures
// ---------------------------------------------------------------------------------------------

// Mac 0x45520, Win 0x618282
bool BrickConnectSceneControl::RenderToBitmap(Bitmap& bitmap, Texture* target, bool fitCamera,
                                              bool noPostProcessing)
{
    RenderQueue* queue = GetRenderControlManager()->GetRender()->GetRenderQueue(0);
    Vector3 cameraPosition(m_camera->GetPosition());
    // the flexible parts: skinned in world space
    for (BasicArray<LDDPart*>::Iterator it = m_flexParts.Begin(); it != m_flexParts.End(); ++it) {
        LDDPart* part = *it;
        if (part->m_flexTransforms.IsEmpty() || part->GetScene() != m_scene || part->m_hidden)
            continue;
        for (BasicArray<LDDPart::FlexSurface>::Iterator s = part->m_flexSurfaces.Begin();
             s != part->m_flexSurfaces.End(); ++s) {
            queue->Add(s->m_mesh, Matrix3D(), 0.0f, 0, 1.0f);
            if (!s->m_decalMesh.IsNull())
                queue->Add(s->m_decalMesh, Matrix3D(), 0.0f, 0, 1.0f);
        }
    }
    // the bodies
    for (BasicArray<BIBrick>::Iterator b = m_bricks.Begin(); b != m_bricks.End(); ++b) {
        Matrix3D bodyTransform;
        if (b->m_body != 0)
            b->m_body->GetTransform(bodyTransform);
        for (BasicArray<PlacedMesh>::Iterator m = b->m_meshes.Begin(); m != b->m_meshes.End(); ++m) {
            Matrix3D transform(bodyTransform, m->m_transform);
            Mesh* mesh = m->m_mesh;
            Real32 depth = DistanceSquared(m->m_center * transform, cameraPosition);
            queue->Add(mesh, transform, depth, m->m_layer, m->m_opacity);
        }
    }
    return RenderScene(queue, bitmap, target, fitCamera, noPostProcessing);
}

// Mac 0x45790, Win 0x615a54
bool BrickConnectSceneControl::RenderScene(RenderQueue* queue, Bitmap& bitmap, Texture* target,
                                           bool fitCamera, bool noPostProcessing)
{
    Render* render = GetRenderControlManager()->GetRender();
    Texture* previousTarget = render->GetRenderTarget();
    RefPointer<Texture> renderTarget;
    bool toBackBuffer = false;
    if (target != 0) {
        renderTarget = target;
    } else if (!noPostProcessing && m_postProcessor != 0) {
        // Mac 0x46000-0x4610e: an ABGR render target of the next power-of-two
        // size through the render's factory; on failure the back buffer.
        Texture::RenderTargetCreateArgs2D args;
        args.format = 0x40000420;
        args.depthBuffer = true;
        args.wrapMode = Texture::kWrapClampToEdge;
        args.alphaMode = 0;
        args.width = NextPowerOfTwo(bitmap.m_width);
        args.height = NextPowerOfTwo(bitmap.m_height);
        Object* object = render->GetObjectFactory()->CreateObject(typeid(Texture), args);
        renderTarget = object ? dynamic_cast<Texture*>(object) : 0;
    }
    if (renderTarget.IsNull()) {
        const Vector2& windowSize = GetRenderControlManager()->GetRootControl()->GetSize();
        if ((Real32)bitmap.m_width > windowSize.x || (Real32)bitmap.m_height > windowSize.y)
            return false;
        toBackBuffer = true;
    } else if (m_postProcessor != 0 && m_buffer != 0) {
        // Mac 0x45809-0x458e9. NOTE (both builds): also with noPostProcessing
        // (then the effect is not resized back below).
        m_postProcessor->Resize(Vector2((Real32)bitmap.m_width, (Real32)bitmap.m_height),
                                Vector2((Real32)NextPowerOfTwo(bitmap.m_width),
                                        (Real32)NextPowerOfTwo(bitmap.m_height)));
    }

    render->BeginFrame();
    render->SetRenderTarget(renderTarget);
    render->SetViewport(Rect(0.0f, 0.0f, (Real32)bitmap.m_width, (Real32)bitmap.m_height));
    render->DisableScissor();
    render->SetMaterial(0);
    render->SetTexture(0, 0);
    render->EnableDepthTest(true);
    render->EnableLighting(true);
    render->SetCullMode(1);
    render->SetClearColor(Color(0.8f, 0.8f, 0.8f, 0.0f));
    render->Clear(7);
    render->SetViewMatrix(Matrix3D());
    render->SetLight(&m_lights[0].m_light, 0);
    render->SetLight(&m_lights[1].m_light, 1);
    render->SetLight(&m_lights[2].m_light, 2);

    RefPointer<Camera> camera = new Camera(*m_camera);
    if (fitCamera)
        FitCamera(camera, (Real32)bitmap.m_width, (Real32)bitmap.m_height);
    camera->Update();
    render->SetViewMatrix(camera->GetViewMatrix());
    if (m_skybox != 0) {
        Real32 focalLength = camera->GetFocalLength();
        Real32 skyFocalLength = camera->GetFocalLength() * 0.25f;
        if (skyFocalLength < 12.0f)
            skyFocalLength = 12.0f;
        camera->SetFocalLength(skyFocalLength);
        camera->SetViewportSize((Real32)bitmap.m_width, (Real32)bitmap.m_height);
        render->SetProjectionMatrix(camera->GetProjectionMatrix());
        m_skybox->Draw();
        camera->SetFocalLength(focalLength);
    }
    camera->SetViewportSize((Real32)bitmap.m_width, (Real32)bitmap.m_height);
    render->SetProjectionMatrix(camera->GetProjectionMatrix());
    const bool postProcess = !noPostProcessing && m_postProcessor != 0 && !toBackBuffer &&
                             m_buffer != 0;
    if (postProcess)
        m_postProcessor->Begin(Color(0.8f, 0.8f, 0.8f, 0.0f));   // Mac 0x45ced
    queue->Flush(0.02f, true, true);
    if (postProcess) {
        // Mac 0x45d42-0x45e85
        m_postProcessor->Apply(Vector2((Real32)bitmap.m_width, (Real32)bitmap.m_height), queue,
                               renderTarget, &m_lights[0].m_light, &m_lights[1].m_light,
                               &m_lights[2].m_light, camera->GetProjectionMatrix(),
                               camera->GetFieldOfViewX(),
                               (Real32)bitmap.m_width / (Real32)bitmap.m_height, camera->GetNear(),
                               camera->GetFar());
    }
    queue->Clear();
    render->SetLight(0, 0);
    render->SetLight(0, 1);
    render->SetLight(0, 2);
    render->ReadPixels((UInt8*)bitmap.m_data, bitmap.m_width, bitmap.m_height, 0, 0, toBackBuffer,
                       Bitmap::HasAlpha(bitmap.m_format));
    render->SetRenderTarget(previousTarget);
    render->EndFrame();
    if (toBackBuffer)
        GetRenderControlManager()->GetRootControl()->Invalidate(0);
    if (postProcess)
        m_postProcessor->Resize(m_size, m_buffer->GetImageSize());   // Mac 0x45f92
    return true;
}

} // namespace LEGO
