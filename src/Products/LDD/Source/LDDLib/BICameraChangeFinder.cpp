// LEGO Digital Designer — restored source
// TU 356 "BICameraChangeFinder.cpp" (Mac 0x336870-0x33ed6f). See BICameraChangeFinder.h.
// Floating point: the Win functions of this TU store every intermediate (no optimisation);
// expressions are written as `(Real32)((EvalReal)a * b + …)` exactly where the Win code keeps
// values on the FPU stack (MathUtil.h), which also gives the Mac (SSE, float) results.
#include "BICameraChangeFinder.h"
#include "BIEvent.h"
#include "BIBrick.h"
#include "BIBrickPrimitive.h"
#include "PrimitiveElement.h"
#include "Primitive.h"
#include "BrickPhysics.h"
#include "Render.h"
#include "RenderQueue.h"
#include "Mesh.h"
#include "Material.h"
#include "Materials.h"
#include "Texture.h"
#include "VertexBuffer.h"
#include "MathUtil.h"
#include "Trace.h"

#include <typeinfo>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <unordered_map>
#include <vector>

namespace LEGO {

// ---- fields of view (Win helpers, inlined on Mac) ---------------------------------------------

// The horizontal field of view: 2 atan(35 / 160) (Mac: the constant 0.43073). The argument is
// not used. Win 0x5810b0 (name inferred)
static Real32 GetFieldOfViewX(Real32 /*fieldOfViewY*/)
{
    Real32 focal = 80.0f;
    Real32 t = (Real32)(35.0f / ((EvalReal)focal * 2.0f));
    return ATan(t) * 2.0f;
}

// The vertical field of view of a `width` x `height` view: 2 atan(35 h / (2 w 80)).
// Win 0x5810f0 (name inferred)
static Real32 GetFieldOfViewY(Real32 width, Real32 height)
{
    Real32 focal = 80.0f;
    Real32 t = (Real32)(((EvalReal)height * 35.0f) / (((EvalReal)width + width) * focal));
    return ATan(t) * 2.0f;
}

// Win 0x581140 (name inferred)
static void SetPerspective(Matrix3D& projection, Real32 width, Real32 height)
{
    Real32 fieldOfViewY = GetFieldOfViewY(width, height);
    projection.SetPerspective(GetFieldOfViewX(fieldOfViewY), fieldOfViewY, 0.9f, 2500.0f);
}

// ---- occlusion culling (LDD64) ------------------------------------------------------------------
//
// The depth buffer at a pixel is the nearest of what is drawn there, in any order, so a brick's
// sample count only depends on occluders that overlap it on screen and are not wholly behind it.
// Rectangles enclose the projected corners of GetMesh's boxes, padded by half a pixel (a corner
// near or behind the eye: the whole screen). "Behind" means more than eight steps of a 16-bit depth
// buffer (the test is GL_LEQUAL). Per camera, the primitives' rectangles at the first test are
// filed in a screen grid; their meshes are made then, as the original's first test does.

// The view and the projection CountVisible sets for `camera` (the same computation).
static void MakeOcclusionView(StorageScene2::Camera* camera, Real32 width, Real32 height, Matrix3D& view,
                              Matrix3D& projection)
{
    Vector3 eye(camera->m_transformation.GetTranslation());
    Vector3 target = Vector3(0.0f, 0.0f, -camera->m_distance) * camera->m_transformation;
    Vector3 up(0.0f, 1.0f, 0.0f);
    Vector3 direction = target - eye;
    Real32 length = direction.GetLength();
    if (!(length > 1.0f))
        length = 1.0f;
    direction.SetLength(length);
    eye = target - direction;
    view.SetLookAt(eye, target, up);
    SetPerspective(projection, width, height);
}

// The corners of the boxes of the occlusion mesh of `brick` (as GetMesh makes them).
static void GetOcclusionCorners(BIBrick* brick, std::vector<Vector3>& corners)
{
    corners.clear();
    for (BasicArray<PrimitiveElement*>::ConstIterator it = brick->GetElements().Begin(); it != brick->GetElements().End(); ++it) {
        Matrix3D elementTransform;
        (*it)->GetTransform(elementTransform);
        const BasicArray<Primitive::CollisionVolume>& list = (*it)->GetBone().m_collisionVolumes;
        for (BasicArray<Primitive::CollisionVolume>::ConstIterator v = list.Begin(); v != list.End(); ++v) {
            BoundingBox box(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
            if (v->m_type == Primitive::CollisionVolume::Capsule) {
                Real32 radius = v->m_size.y;
                box = BoundingBox(Vector3(0.0f, radius * -0.5f, radius * -0.5f),
                                  Vector3(v->m_size.x, radius * 0.5f, radius * 0.5f));
            } else if (v->m_type == Primitive::CollisionVolume::Sphere) {
                Real32 radius = v->m_size.x;
                box = BoundingBox(Vector3(radius, radius, radius));
                box.m_min = -box.m_max;
            } else if (v->m_type == Primitive::CollisionVolume::Box) {
                box = BoundingBox(v->m_size);
                box.m_min = -box.m_max;
            } else {
                continue;
            }
            Matrix3D transform(elementTransform, v->m_transform);
            const Vector3& lo = box.m_min;
            const Vector3& hi = box.m_max;
            corners.push_back(Vector3(lo.x, lo.y, lo.z) * transform);
            corners.push_back(Vector3(hi.x, lo.y, lo.z) * transform);
            corners.push_back(Vector3(lo.x, hi.y, lo.z) * transform);
            corners.push_back(Vector3(hi.x, hi.y, lo.z) * transform);
            corners.push_back(Vector3(lo.x, lo.y, hi.z) * transform);
            corners.push_back(Vector3(hi.x, lo.y, hi.z) * transform);
            corners.push_back(Vector3(lo.x, hi.y, hi.z) * transform);
            corners.push_back(Vector3(hi.x, hi.y, hi.z) * transform);
        }
    }
}

class BIOcclusionCull {
public:
    struct Rect {
        Real32 x0, y0, x1, y1;
        Real32 z0, z1;   // depth range (normalized device coordinates)
        bool Overlaps(const Rect& r) const { return x0 <= r.x1 && r.x0 <= x1 && y0 <= r.y1 && r.y0 <= y1; }
        // true when this occluder can hide part of the brick `r` (overlap, not wholly behind)
        bool CanHide(const Rect& r) const { return Overlaps(r) && !(z0 > r.z1 + kDepthMargin); }
    };
    // eight steps of a 16-bit depth buffer in normalized device coordinates (depth range 2)
    static const Real32 kDepthMargin;
    struct Grid {
        Matrix3D            view;
        Matrix3D            projection;
        double              clip[16];    // projection * view (column-major, as OpenGL applies them)
        std::vector<Rect>   rects;       // per primitive
        std::vector<UInt32> cellStart;   // kCells * kCells + 1
        std::vector<UInt32> items;       // primitive indices per cell
        std::vector<UInt32> large;       // primitives with a rectangle over too many cells
    };
    enum { kCells = 128, kLargeCells = 1024 };

    BIOcclusionCull() : m_built(false), m_stampValue(0), m_padX(0.0f), m_padY(0.0f) {}

    void Reset()
    {
        m_built = false;
        std::vector<BIBrickPrimitive*>().swap(m_primitives);
        std::vector<SingleMesh*>().swap(m_meshes);
        std::unordered_map<const BIBrickPrimitive*, UInt32>().swap(m_index);
        std::vector<Grid>().swap(m_grids);
        std::vector<UInt32>().swap(m_stamp);
        m_stampValue = 0;
    }

    // The occluders the test of `brick` from camera `cameraIndex` must draw: those of `remaining`
    // and `current` that don't intersect the brick and can hide it. False when the grids don't fit
    // (the caller draws them all).
    bool Collect(BICameraChangeFinderOcclusionTesterRIM* tester, BIBrick* brick, UInt32 cameraIndex,
                 const Matrix3D& view, const Matrix3D& projection, const BasicSet<BIBrickPrimitive*>& remaining,
                 const BasicSet<BIBrickPrimitive*>& current, std::vector<SingleMesh*>& draw)
    {
        if (!m_built)
            Build(tester, remaining, current);
        if (cameraIndex >= m_grids.size())
            return false;
        Grid& grid = m_grids[cameraIndex];
        if (std::memcmp(&grid.view, &view, sizeof(Matrix3D)) != 0 ||
            std::memcmp(&grid.projection, &projection, sizeof(Matrix3D)) != 0)
            return false;
        GetOcclusionCorners(brick, m_corners);
        Rect rect = Project(grid, m_corners, 0, m_corners.size());
        if (++m_stampValue == 0) {
            std::fill(m_stamp.begin(), m_stamp.end(), 0u);
            m_stampValue = 1;
        }
        if (rect.x0 <= 1.0f && rect.x1 >= -1.0f && rect.y0 <= 1.0f && rect.y1 >= -1.0f) {
            Int32 cx0, cy0, cx1, cy1;
            Cells(rect, cx0, cy0, cx1, cy1);
            for (Int32 cy = cy0; cy <= cy1; ++cy)
                for (Int32 cx = cx0; cx <= cx1; ++cx) {
                    UInt32 cell = (UInt32)(cy * kCells + cx);
                    for (UInt32 i = grid.cellStart[cell]; i < grid.cellStart[cell + 1]; ++i)
                        Test(grid, grid.items[i], brick, rect, remaining, draw);
                }
        }
        for (std::vector<UInt32>::const_iterator it = grid.large.begin(); it != grid.large.end(); ++it)
            Test(grid, *it, brick, rect, remaining, draw);
        for (BasicSet<BIBrickPrimitive*>::ConstIterator it = current.Begin(); it != current.End(); ++it) {
            BIBrickPrimitive* primitive = *it;
            if (brick->Intersects(primitive))
                continue;
            std::unordered_map<const BIBrickPrimitive*, UInt32>::const_iterator found = m_index.find(primitive);
            Rect other;
            if (found != m_index.end()) {
                other = grid.rects[found->second];
            } else {
                GetOcclusionCorners(primitive, m_corners);
                other = Project(grid, m_corners, 0, m_corners.size());
            }
            if (other.CanHide(rect))
                draw.push_back(found != m_index.end() ? m_meshes[found->second] : tester->GetMesh(primitive));
        }
        return true;
    }

private:
    void Test(const Grid& grid, UInt32 index, BIBrick* brick, const Rect& rect,
              const BasicSet<BIBrickPrimitive*>& remaining, std::vector<SingleMesh*>& draw)
    {
        if (m_stamp[index] == m_stampValue)
            return;
        m_stamp[index] = m_stampValue;
        if (!grid.rects[index].CanHide(rect))
            return;
        BIBrickPrimitive* primitive = m_primitives[index];
        if (remaining.Find(primitive) == remaining.End())
            return;
        if (brick->Intersects(primitive))
            return;
        draw.push_back(m_meshes[index]);
    }

    static void Cells(const Rect& rect, Int32& cx0, Int32& cy0, Int32& cx1, Int32& cy1)
    {
        cx0 = Cell(rect.x0);
        cy0 = Cell(rect.y0);
        cx1 = Cell(rect.x1);
        cy1 = Cell(rect.y1);
    }

    static Int32 Cell(Real32 value)
    {
        double c = std::floor(((double)value + 1.0) * (kCells * 0.5));
        if (!(c >= 0.0))
            return 0;
        if (c > kCells - 1)
            return kCells - 1;
        return (Int32)c;
    }

    Rect Project(const Grid& grid, const std::vector<Vector3>& corners, size_t begin, size_t end) const
    {
        Rect rect;
        rect.x0 = rect.y0 = rect.z0 = 1e30f;
        rect.x1 = rect.y1 = rect.z1 = -1e30f;
        const double* m = grid.clip;
        for (size_t i = begin; i < end; ++i) {
            double x = corners[i].x, y = corners[i].y, z = corners[i].z;
            double cx = m[0] * x + m[4] * y + m[8] * z + m[12];
            double cy = m[1] * x + m[5] * y + m[9] * z + m[13];
            double cz = m[2] * x + m[6] * y + m[10] * z + m[14];
            double cw = m[3] * x + m[7] * y + m[11] * z + m[15];
            if (!(cw > 0.45)) {   // near (0.9) or behind the eye: the whole screen, any depth
                rect.x0 = rect.y0 = rect.z0 = -1e30f;
                rect.x1 = rect.y1 = rect.z1 = 1e30f;
                return rect;
            }
            Real32 nx = (Real32)(cx / cw);
            Real32 ny = (Real32)(cy / cw);
            Real32 nz = (Real32)(cz / cw);
            if (nx < rect.x0) rect.x0 = nx;
            if (nx > rect.x1) rect.x1 = nx;
            if (ny < rect.y0) rect.y0 = ny;
            if (ny > rect.y1) rect.y1 = ny;
            if (nz < rect.z0) rect.z0 = nz;
            if (nz > rect.z1) rect.z1 = nz;
        }
        if (begin < end) {
            rect.x0 -= m_padX;
            rect.x1 += m_padX;
            rect.y0 -= m_padY;
            rect.y1 += m_padY;
        }
        return rect;
    }

    void Build(BICameraChangeFinderOcclusionTesterRIM* tester, const BasicSet<BIBrickPrimitive*>& remaining,
               const BasicSet<BIBrickPrimitive*>& current)
    {
        m_built = true;
        Vector2 size = tester->m_target->GetSize();
        // half a pixel of the occlusion target in normalized device coordinates
        m_padX = size.x > 0.0f ? 1.0f / size.x : 2.0f;
        m_padY = size.y > 0.0f ? 1.0f / size.y : 2.0f;
        m_primitives.clear();
        m_index.clear();
        m_primitives.reserve(remaining.GetSize() + current.GetSize());
        for (BasicSet<BIBrickPrimitive*>::ConstIterator it = remaining.Begin(); it != remaining.End(); ++it)
            if (m_index.insert(std::make_pair((const BIBrickPrimitive*)*it, (UInt32)m_primitives.size())).second)
                m_primitives.push_back(*it);
        for (BasicSet<BIBrickPrimitive*>::ConstIterator it = current.Begin(); it != current.End(); ++it)
            if (m_index.insert(std::make_pair((const BIBrickPrimitive*)*it, (UInt32)m_primitives.size())).second)
                m_primitives.push_back(*it);
        const size_t count = m_primitives.size();
        m_stamp.assign(count, 0u);
        m_stampValue = 0;

        // the corners of every primitive; the meshes as the original's first test makes them
        std::vector<Vector3> corners;
        std::vector<size_t> first(count + 1, 0);
        // (the meshes stay in the tester's map, which is cleared with these grids at Begin)
        m_meshes.resize(count);
        for (size_t i = 0; i < count; ++i) {
            m_meshes[i] = tester->GetMesh(m_primitives[i]);
            GetOcclusionCorners(m_primitives[i], m_corners);
            first[i] = corners.size();
            corners.insert(corners.end(), m_corners.begin(), m_corners.end());
        }
        first[count] = corners.size();

        m_grids.resize(tester->m_cameras.GetSize());
        std::vector<UInt32> fill;
        for (UInt32 c = 0; c < tester->m_cameras.GetSize(); ++c) {
            Grid& grid = m_grids[c];
            MakeOcclusionView(tester->m_cameras[c], tester->m_width, tester->m_height, grid.view, grid.projection);
            const Real32* v = grid.view;
            const Real32* p = grid.projection;
            for (int col = 0; col < 4; ++col)
                for (int row = 0; row < 4; ++row) {
                    double sum = 0.0;
                    for (int k = 0; k < 4; ++k)
                        sum += (double)p[k * 4 + row] * (double)v[col * 4 + k];
                    grid.clip[col * 4 + row] = sum;
                }
            grid.rects.resize(count);
            grid.cellStart.assign(kCells * kCells + 1, 0u);
            grid.large.clear();
            for (size_t i = 0; i < count; ++i)
                grid.rects[i] = Project(grid, corners, first[i], first[i + 1]);
            // count, then file
            for (int pass = 0; pass < 2; ++pass) {
                if (pass == 1) {
                    for (UInt32 cell = 0; cell < kCells * kCells; ++cell)
                        grid.cellStart[cell + 1] += grid.cellStart[cell];
                    grid.items.assign(grid.cellStart[kCells * kCells], 0u);
                    fill.assign(grid.cellStart.begin(), grid.cellStart.end() - 1);
                }
                for (size_t i = 0; i < count; ++i) {
                    const Rect& rect = grid.rects[i];
                    if (!(rect.x0 <= 1.0f && rect.x1 >= -1.0f && rect.y0 <= 1.0f && rect.y1 >= -1.0f))
                        continue;   // off the screen (or no boxes): drawn nowhere
                    Int32 cx0, cy0, cx1, cy1;
                    Cells(rect, cx0, cy0, cx1, cy1);
                    if ((cx1 - cx0 + 1) * (cy1 - cy0 + 1) > kLargeCells) {
                        if (pass == 0)
                            grid.large.push_back((UInt32)i);
                        continue;
                    }
                    for (Int32 cy = cy0; cy <= cy1; ++cy)
                        for (Int32 cx = cx0; cx <= cx1; ++cx) {
                            UInt32 cell = (UInt32)(cy * kCells + cx);
                            if (pass == 0)
                                ++grid.cellStart[cell + 1];
                            else
                                grid.items[fill[cell]++] = (UInt32)i;
                        }
                }
            }
        }
    }

    bool                                                 m_built;
    std::vector<BIBrickPrimitive*>                       m_primitives;
    std::vector<SingleMesh*>                             m_meshes;    // per primitive (GetMesh)
    std::unordered_map<const BIBrickPrimitive*, UInt32>  m_index;
    std::vector<Grid>                                    m_grids;     // per camera
    std::vector<UInt32>                                  m_stamp;     // per primitive: last query
    UInt32                                               m_stampValue;
    Real32                                               m_padX;
    Real32                                               m_padY;
    std::vector<Vector3>                                 m_corners;   // scratch
};

const Real32 BIOcclusionCull::kDepthMargin = 16.0f / 65536.0f;

// ---- BICameraNode / BICameraGraph --------------------------------------------------------------

// Mac 0x33ae20 / 0x33ae80, Win 0x57cdc0
BICameraNode::~BICameraNode()
{
}

// Mac 0x3378e0 (body 0x337760) / 0x3378f0, Win 0x57d5f0 / 0x57d670
BICameraGraph::~BICameraGraph()
{
}

// Mac 0x336870
void BICameraGraph::AddLayer(const BICameraList& cameras)
{
    m_layers.PushBack(BasicArray<RefPointer<BICameraNode> >());
    BasicArray<RefPointer<BICameraNode> >& layer = m_layers.Back();
    for (BICameraList::ConstIterator it = cameras.Begin(); it != cameras.End(); ++it) {
        StorageScene2::Camera* camera = *it;
        layer.PushBack(RefPointer<BICameraNode>(new BICameraNode(m_layers.GetSize() - 1, camera)));
        if (!m_priorities.Contains(camera))
            m_priorities[camera] = PriorityAbove;
    }
}

// Mac 0x337610
UInt8 BICameraGraph::GetDistance(StorageScene2::Camera* a, StorageScene2::Camera* b)
{
    UInt32 pa = m_priorities[a];
    UInt32 pb = m_priorities[b];
    if (pa == PriorityAbove && pb == PriorityAbove)
        return a != b ? 2 : 1;
    if (pa == PriorityBelow && pb == PriorityAbove)
        return 3;
    if (pa == PriorityBelow && pb == PriorityBelow)
        return a != b ? 5 : 4;
    if (pa == PriorityAbove && pb == PriorityBelow)
        return 6;
    return 0;
}

// Mac 0x336f60
void BICameraGraph::FindPath(BICameraList& path)
{
    path.Clear();
    if (m_layers.IsEmpty())
        return;
    typedef BasicMultiset<RefPointer<BICameraNode>, BICameraNodeLess> OpenSet;
    OpenSet open;
    BasicArray<RefPointer<BICameraNode> >& first = m_layers[0];
    for (UInt32 i = 0; i < first.GetSize(); ++i) {
        BICameraNode* node = first[i];
        node->m_cost = m_priorities[node->m_camera] == PriorityAbove ? 0 : 6;
        open.Insert(RefPointer<BICameraNode>(node));
    }
    while (open.GetSize() != 0) {
        OpenSet::Iterator top = open.Begin();
        BICameraNode* current = *top;   // still referenced by its layer
        open.Std().erase(top);
        UInt32 next = current->m_layer + 1;
        if (next >= m_layers.GetSize())
            continue;
        BasicArray<RefPointer<BICameraNode> >& layer = m_layers[next];
        for (UInt32 i = 0; i < layer.GetSize(); ++i) {
            BICameraNode* node = layer[i];
            UInt32 cost = current->m_cost + GetDistance(current->m_camera, node->m_camera);
            if (cost >= node->m_cost)
                continue;
            // the queued node of the same camera and cost, if any, is replaced
            std::pair<OpenSet::Iterator, OpenSet::Iterator> range = open.Std().equal_range(RefPointer<BICameraNode>(node));
            for (OpenSet::Iterator it = range.first; it != range.second; ++it)
                if ((*it)->m_camera == node->m_camera) {
                    open.Std().erase(it);
                    break;
                }
            node->m_cost = current->m_cost + GetDistance(current->m_camera, node->m_camera);
            node->m_previous = current;
            open.Insert(RefPointer<BICameraNode>(node));
        }
    }
    BasicArray<RefPointer<BICameraNode> >& last = m_layers.Back();
    BICameraNode* best = last[0];
    for (UInt32 i = 1; i < last.GetSize(); ++i)
        if (last[i]->m_cost < best->m_cost)
            best = last[i];
    BICameraList reversed;
    for (BICameraNode* node = best;; node = node->m_previous) {
        reversed.PushBack(node->m_camera);
        if (node->m_previous == 0)
            break;
    }
    for (BICameraList::Iterator it = reversed.End(); it != reversed.Begin();) {
        --it;
        path.PushBack(*it);
    }
    m_layers.Clear();
    m_priorities.Clear();
}

// ---- TestDistancesCollisionVolumeVisitor -------------------------------------------------------

// Mac 0x33aa90, Win 0x581330
void BICameraChangeFinder::TestDistancesCollisionVolumeVisitor::Visit(PhysicsGeometrySphere* sphere,
                                                                      const Matrix3D& transform)
{
    Real32 radius = (Real32)((double)sphere->GetRadius() + 0.025);
    m_finder->PushPlanes(Vector3(), radius, transform, m_planes);
}

// Mac 0x33aaf0, Win 0x581380
void BICameraChangeFinder::TestDistancesCollisionVolumeVisitor::Visit(PhysicsGeometryBox* box, const Matrix3D& transform)
{
    BoundingBox bounds(box->GetHalfSize() + 0.025f);
    bounds.m_min = -bounds.m_max;
    const Vector3& lo = bounds.m_min;
    const Vector3& hi = bounds.m_max;
    m_finder->PushPlanes(lo, 0.0f, transform, m_planes);
    m_finder->PushPlanes(hi, 0.0f, transform, m_planes);
    m_finder->PushPlanes(Vector3(lo.x, lo.y, hi.z), 0.0f, transform, m_planes);
    m_finder->PushPlanes(Vector3(lo.x, hi.y, lo.z), 0.0f, transform, m_planes);
    m_finder->PushPlanes(Vector3(hi.x, lo.y, lo.z), 0.0f, transform, m_planes);
    m_finder->PushPlanes(Vector3(hi.x, hi.y, lo.z), 0.0f, transform, m_planes);
    m_finder->PushPlanes(Vector3(hi.x, lo.y, hi.z), 0.0f, transform, m_planes);
    m_finder->PushPlanes(Vector3(lo.x, hi.y, hi.z), 0.0f, transform, m_planes);
}

// Mac 0x33ad60, Win 0x5815c0
void BICameraChangeFinder::TestDistancesCollisionVolumeVisitor::Visit(PhysicsGeometryCapsule* capsule,
                                                                      const Matrix3D& transform)
{
    Real32 radius = (Real32)((double)capsule->GetRadius() + 0.025);
    m_finder->PushPlanes(Vector3(), radius, transform, m_planes);
    radius = (Real32)((double)capsule->GetRadius() + 0.025);
    m_finder->PushPlanes(Vector3(capsule->GetLength(), 0.0f, 0.0f), radius, transform, m_planes);
}

// ---- BICameraChangeFinder ------------------------------------------------------------------------

// Mac 0x338110 / 0x337960, Win 0x57fa60
BICameraChangeFinder::BICameraChangeFinder(Render* render, UInt32 width, UInt32 height, UInt32 maxStepBricks,
                                           Real32 threshold, UInt32 unknown,
                                           const BasicArray<BICameraPosition>& positions)
    : m_initialized(false), m_render(render), m_threshold(threshold), m_unknown1c(unknown), m_positions(positions),
      m_maxStepBricks(maxStepBricks),
      m_width((Real32)width), m_height((Real32)height)   // the target's size until a view is set
{
    m_cull = new BIOcclusionCull;
    bool ok = Initialize(width, height);
    m_initialized = ok;
    String text("Building Guide: Occlusion testing not supported");
    if (!ok)
        Trace(text, true);
}

// Mac 0x339d10 (body 0x339950) / 0x339d20, Win 0x5803d0 / 0x57fc00
BICameraChangeFinder::~BICameraChangeFinder()
{
    delete m_cull;
}

// Mac 0x338120
void BICameraChangeFinder::SetViewportSize(Real32 width, Real32 height)
{
    // NOTE: the original takes any size (and starts with 0 x 0), so generating with an empty
    // view (BuildingGuideGenerateThread::Run passes the scene control's size) divides by zero in
    // GetFieldOfViewY: NaN cameras, which BIGenerator stores in the steps and BGMode::SetCamera
    // then puts into the scene camera; fixed: an empty size is skipped (the previous one, at
    // first the occlusion target's 128 x 128, is kept).
    if (!(width > 0.0f && height > 0.0f))
        return;
    m_width = width;
    m_height = height;
}

// Mac 0x337dd0
bool BICameraChangeFinder::Initialize(UInt32 width, UInt32 height)
{
    RefPointer<BICameraChangeFinderInitializeRIM> rim =
        new BICameraChangeFinderInitializeRIM(m_render, (Real32)width, (Real32)height);
    rim->Run();
    if (!rim->m_supported)
        return false;
    m_target = rim->m_target;
    if (m_target.IsNull())
        return false;
    m_material = rim->m_material;
    return !m_material.IsNull();
}

// Mac 0x338150, Win 0x57fc60
void BICameraChangeFinder::Begin(const BasicSet<BIBrickPrimitive*>& primitives)
{
    m_cull->Reset();
    m_remaining.Clear();
    m_currentBricks.Clear();
    m_events.Clear();
    m_stepCameras.Clear();
    if (&m_remaining != &primitives)
        m_remaining = primitives;
    m_cameras.Clear();
    m_currentCameras.Clear();
    if (m_initialized) {
        RefPointer<BICameraChangeFinderClearMeshMapRIM> rim = new BICameraChangeFinderClearMeshMapRIM(&m_meshes);
        rim->Run();
    }
    for (BasicArray<BICameraPosition>::ConstIterator it = m_positions.Begin(); it != m_positions.End(); ++it) {
        Vector3 eye(it->m_direction);
        m_cameras.PushBack(MakeCamera(eye, m_remaining, m_width, m_height, 50.0f));
    }
}

// Mac 0x338840, Win 0x57fe50
bool BICameraChangeFinder::AddBrick(BIBrick* brick)
{
    BasicSet<BIBrickPrimitive*> primitives;
    brick->GetPrimitives(primitives);
    for (BasicSet<BIBrickPrimitive*>::Iterator it = primitives.Begin(); it != primitives.End(); ++it)
        m_remaining.Erase(*it);
    Process(brick);
    return true;
}

// Mac 0x338930, Win 0x5805b0
bool BICameraChangeFinder::Process(BIBrick* brick)
{
    BasicArray<Real32> visibility;
    if (!m_initialized) {
        visibility.PushBack(1.0f);
    } else {
        RefPointer<BICameraChangeFinderOcclusionTesterRIM> tester =
            new BICameraChangeFinderOcclusionTesterRIM(m_render, m_width, m_height, &m_meshes, m_target, m_material);
        tester->m_brick = brick;
        // the remaining primitives by reference (the generator thread waits for the test)
        tester->m_remaining = &m_remaining;
        tester->m_cull = m_cull;
        if (&tester->m_cameras != &m_cameras)
            tester->m_cameras = m_cameras;
        tester->m_results.Clear();
        for (BasicSet<BIBrick*>::Iterator it = m_currentBricks.Begin(); it != m_currentBricks.End(); ++it)
            for (BasicSet<BIBrickPrimitive*>::ConstIterator p = (*it)->GetPrimitiveSet().Begin();
                 p != (*it)->GetPrimitiveSet().End(); ++p)
                tester->m_currentPrimitives.Insert(*p);
        tester->Run();
        visibility = tester->m_results;
    }
    m_events.PushBack(RefPointer<BIEvent>(new BIEventBrick(brick)));
    UInt32 step = m_events.GetSize() - 1;
    BICameraList visible;
    for (UInt32 i = 0; i < visibility.GetSize(); ++i) {
        if (!(m_threshold <= visibility[i]))
            continue;
        if (m_currentBricks.GetSize() == 0) {
            visible.PushBack(m_cameras[i]);
        } else {
            for (BICameraList::Iterator it = m_currentCameras.Begin(); it != m_currentCameras.End(); ++it)
                if (*it == m_cameras[i]) {
                    visible.PushBack(m_cameras[i]);
                    break;
                }
        }
    }
    if (m_currentBricks.GetSize() == 0 && visible.IsEmpty()) {
        // a step of its own with the camera that sees most of it
        UInt32 best = 0;
        if (visibility.GetSize() > 1) {
            Real32 bestValue = visibility[0];
            for (UInt32 i = 1; i < visibility.GetSize(); ++i)
                if (visibility[i] > bestValue) {
                    bestValue = visibility[i];
                    best = i;
                }
        }
        visible.PushBack(m_cameras[best]);
        m_stepCameras[step] = visible;
        m_currentBricks.Clear();
        m_currentCameras.Clear();
    } else if (visible.IsEmpty()) {
        // the open step ends before this brick; start again with it
        m_stepCameras[step - 1] = m_currentCameras;
        m_currentCameras.Clear();
        m_currentBricks.Clear();
        m_events.PopBack();
        Process(brick);
    } else if (m_currentBricks.GetSize() + 1 != m_maxStepBricks) {
        m_currentCameras = visible;
        m_currentBricks.Insert(brick);
    } else {
        m_stepCameras[step] = visible;
        m_currentBricks.Clear();
        m_currentCameras.Clear();
    }
    return true;
}

// Mac 0x339260, Win 0x57ff40
void BICameraChangeFinder::GetEvents(BasicArray<RefPointer<BIEvent> >& events)
{
    if (!m_currentCameras.IsEmpty()) {
        m_stepCameras[m_events.GetSize() - 1] = m_currentCameras;
        m_currentCameras.Clear();
        m_currentBricks.Clear();
    }
    RefPointer<BICameraGraph> graph = new BICameraGraph;
    for (BasicMap<UInt32, BICameraList>::Iterator it = m_stepCameras.Begin(); it != m_stepCameras.End(); ++it)
        graph->AddLayer(it->second);
    for (UInt32 i = 0; i < m_cameras.GetSize(); ++i)
        graph->m_priorities[m_cameras[i]] = (BICameraGraph::Priority)m_positions[i].m_priority;
    BICameraList path;
    graph->FindPath(path);
    UInt32 k = 0;
    for (BasicMap<UInt32, BICameraList>::Iterator it = m_stepCameras.Begin(); it != m_stepCameras.End(); ++it, ++k) {
        it->second.Clear();
        it->second.PushBack(path[k]);
    }
    BasicArray<RefPointer<BIEvent> > reversed;
    for (BasicArray<RefPointer<BIEvent> >::Iterator it = m_events.End(); it != m_events.Begin();) {
        --it;
        reversed.PushBack(*it);
    }
    for (UInt32 i = 0; i < reversed.GetSize(); ++i) {
        UInt32 key = reversed.GetSize() - 1 - i;
        BasicMap<UInt32, BICameraList>::Iterator found = m_stepCameras.Find(key);
        if (found != m_stepCameras.End() && !found->second.IsEmpty() && !found->second[0].IsNull())
            events.PushBack(RefPointer<BIEvent>(new BIEventStep(found->second[0])));
        events.PushBack(reversed[i]);
    }
}

// Mac 0x338510, Win 0x580db0
RefPointer<StorageScene2::Camera> BICameraChangeFinder::MakeCamera(Vector3 eye, const BasicSet<BIBrickPrimitive*>& primitives,
                                                                   Real32 width, Real32 height, Real32 minDistance)
{
    Vector3 up(0.0f, 1.0f, 0.0f);
    Vector3 target(0.0f, 0.0f, 0.0f);
    Matrix3D view;
    view.SetLookAt(eye, target, up);
    Matrix3D projection;
    SetPerspective(projection, width, height);
    Fit(view, projection, eye, target, width, height, primitives);
    Vector3 offset = eye - target;
    offset *= 1.33f;
    eye = target + offset;
    if (!IsEqual(minDistance, 0.0f, EPSILON) && (eye - target).GetLength() < minDistance) {
        Vector3 direction = eye - target;
        direction.SetLength(minDistance);
        eye = target + direction;
    }
    view.SetLookAt(eye, target, up);
    SetPerspective(projection, width, height);
    RefPointer<StorageScene2::Camera> camera = new StorageScene2::Camera;
    Matrix3D lookAt;
    lookAt.SetLookAt(eye, target, up);
    camera->m_transformation.SetInverse(lookAt);
    camera->m_distance = (eye - target).GetLength();
    return camera;
}

// Mac 0x339fa0, Win 0x581670
void BICameraChangeFinder::Fit(const Matrix3D& view, const Matrix3D& projection, Vector3& eye, Vector3& target,
                               Real32 width, Real32 height, const BasicSet<BIBrickPrimitive*>& primitives)
{
    Matrix3D inverse;
    inverse.SetInverse(view);
    Plane planes[4];
    Real32 angle = (Real32)(((EvalReal)PI - GetFieldOfViewX(0.0f)) * 0.5f);
    Real32 sinX = Sin(angle);
    angle = (Real32)(((EvalReal)PI - GetFieldOfViewY(width, height)) * 0.5f);
    Real32 sinY = Sin(angle);
    angle = (Real32)(((EvalReal)PI - GetFieldOfViewX(0.0f)) * 0.5f);
    Real32 cosX = Cos(angle);
    angle = (Real32)(((EvalReal)PI - GetFieldOfViewY(width, height)) * 0.5f);
    Real32 cosY = Cos(angle);

    // the side planes of the frustum (normals outwards) through the eye
    Vector3 normal(sinX, 0.0f, -cosX);
    normal.Normalize();
    RotateInPlace(normal, inverse);
    planes[0] = Plane(normal, eye);
    normal = Vector3(-sinX, 0.0f, -cosX);
    normal.Normalize();
    RotateInPlace(normal, inverse);
    planes[1] = Plane(normal, eye);
    normal = Vector3(0.0f, -sinY, -cosY);
    normal.Normalize();
    RotateInPlace(normal, inverse);
    planes[2] = Plane(normal, eye);
    normal = Vector3(0.0f, sinY, -cosY);
    normal.Normalize();
    RotateInPlace(normal, inverse);
    planes[3] = Plane(normal, eye);

    // moved to the centre of the model, then out to every collision volume
    BoundingBox bounds(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    bounds.Reset();
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = primitives.Begin(); it != primitives.End(); ++it)
        bounds.Union((*it)->GetBoundingBox());
    Sphere sphere(bounds);
    for (UInt32 i = 0; i < 4; ++i)
        planes[i].SetPoint(sphere.m_center);
    TestDistances(view, projection, planes, primitives);

    // the eye: the point of the two plane intersection lines closest to each other (the one
    // nearer to the centre of the model)
    Vector3 point0;
    Vector3 point1;
    Vector3 direction0;
    Vector3 direction1;
    PlanePlaneIntersect(planes[0], planes[1], point0, direction0);
    PlanePlaneIntersect(planes[2], planes[3], point1, direction1);
    Vector3 end0 = point0 + direction0 * 10.0f;
    Vector3 end1 = point1 + direction1 * 10.0f;
    Vector3 closest0;
    Vector3 closest1;
    Vector3 w;
    Vector3 v;
    Vector3 u;
    w.x = point0.x - point1.x;
    w.y = point0.y - point1.y;
    w.z = point0.z - point1.z;
    v.x = end1.x - point1.x;
    v.y = end1.y - point1.y;
    v.z = end1.z - point1.z;
    u.x = end0.x - point0.x;
    u.y = end0.y - point0.y;
    u.z = end0.z - point0.z;
    Real32 wv = (Real32)((EvalReal)w.x * v.x + (EvalReal)w.y * v.y + (EvalReal)w.z * v.z);
    Real32 vu = (Real32)((EvalReal)v.x * u.x + (EvalReal)v.y * u.y + (EvalReal)v.z * u.z);
    Real32 wu = (Real32)((EvalReal)w.x * u.x + (EvalReal)w.y * u.y + (EvalReal)w.z * u.z);
    Real32 vv = (Real32)((EvalReal)v.x * v.x + (EvalReal)v.y * v.y + (EvalReal)v.z * v.z);
    Real32 uu = (Real32)((EvalReal)u.x * u.x + (EvalReal)u.y * u.y + (EvalReal)u.z * u.z);
    Real32 denominator = (Real32)((EvalReal)uu * vv - (EvalReal)vu * vu);
    Real32 numerator = (Real32)((EvalReal)wv * vu - (EvalReal)wu * vv);
    Real32 s = (Real32)((EvalReal)numerator / denominator);
    Real32 t = (Real32)(((EvalReal)vu * s + wv) / vv);
    closest0.x = (Real32)((EvalReal)s * u.x + point0.x);
    closest0.y = (Real32)((EvalReal)s * u.y + point0.y);
    closest0.z = (Real32)((EvalReal)s * u.z + point0.z);
    closest1.x = (Real32)((EvalReal)t * v.x + point1.x);
    closest1.y = (Real32)((EvalReal)t * v.y + point1.y);
    closest1.z = (Real32)((EvalReal)t * v.z + point1.z);

    Vector3 view0 = target - eye;
    Real32 distance0 = (closest0 - sphere.m_center).GetLengthSquared();
    Real32 distance1 = (closest1 - sphere.m_center).GetLengthSquared();
#if defined(_WIN32)
    // NOTE: on equal distances the Win build takes the first point, the Mac build the second.
    if (distance1 > distance0)
        eye = closest1;
    else
        eye = closest0;
#else
    if (distance0 <= distance1)
        eye = closest1;
    else
        eye = closest0;
#endif
    target = view0 + eye;
    // the target: the point of the view line nearest to the centre of the model
    Vector3 direction = target - eye;
    direction.Normalize();
    Real32 length = direction * (sphere.m_center - eye);
    direction.SetLength(length);
    target = eye + direction;
}

// Inlined on Mac (0x339fa0), Win 0x581240
void BICameraChangeFinder::TestDistances(const Matrix3D& /*view*/, const Matrix3D& /*projection*/, Plane* planes,
                                         const BasicSet<BIBrickPrimitive*>& primitives)
{
    TestDistancesCollisionVolumeVisitor visitor(this, planes);
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = primitives.Begin(); it != primitives.End(); ++it) {
        const BasicArray<PrimitiveElement*>& elements = (*it)->GetElements();
        for (BasicArray<PrimitiveElement*>::ConstIterator e = elements.Begin(); e != elements.End(); ++e) {
            Matrix3D transform;
            (*e)->GetTransform(transform);
            (*e)->VisitCollisionVolumes(visitor, transform);
        }
    }
}

// Mac 0x33a8d0, Win 0x5811a0
void BICameraChangeFinder::PushPlanes(const Vector3& point, Real32 radius, const Matrix3D& transform, Plane* planes)
{
    Vector3 world = point * transform;
    for (UInt32 i = 0; i < 4; ++i)
        if (planes[i].GetDistance(world) < -radius)
            planes[i].SetPoint(world - planes[i].m_normal * radius);
}

// ---- the main-thread jobs ----------------------------------------------------------------------

// Mac 0x33dca0 / 0x33dd00, Win 0x57dbe0
BICameraChangeFinderInitializeRIM::~BICameraChangeFinderInitializeRIM()
{
}

// Mac 0x33ddc0, Win 0x57d7e0
void BICameraChangeFinderInitializeRIM::Update()
{
    m_supported = m_render->IsOcclusionQuerySupported();
    if (m_supported) {
        Texture::RenderTargetCreateArgs2D args;
        args.format = 0x40000420;
        args.depthBuffer = true;
        args.wrapMode = Texture::kWrapClampToEdge;
        args.alphaMode = 0;
        args.width = (UInt32)(Int64)m_width;
        args.height = (UInt32)(Int64)m_height;
        Object* object = m_render->GetObjectFactory()->CreateObject(typeid(Texture), args);
        m_target = object != 0 ? dynamic_cast<Texture*>(object) : 0;
        MaterialColors colors;
        colors.ambient = Color::White();
        colors.specular = Color::White();
        colors.diffuse = Color::White();
        colors.emissive = Color::White();
        colors.shininess = 5.0f;
        m_material = new ColoredMaterial(colors);
    }
    RunInMainThreadObserver::Update();
}

// Mac 0x33aef0 / 0x33af90, Win 0x57f9a0
BICameraChangeFinderClearMeshMapRIM::~BICameraChangeFinderClearMeshMapRIM()
{
}

// Mac 0x33b040, Win 0x57f980
void BICameraChangeFinderClearMeshMapRIM::Update()
{
    m_meshes->Clear();
    RunInMainThreadObserver::Update();
}

// Mac 0x339d60
BICameraChangeFinderOcclusionTesterRIM::BICameraChangeFinderOcclusionTesterRIM(
    Render* render, Real32 width, Real32 height, BasicMap<const BIBrick*, RefPointer<SingleMesh> >* meshes,
    RefPointer<Texture> target, RefPointer<Material> material)
    : m_render(render), m_meshes(meshes), m_width(width), m_height(height), m_target(target), m_material(material),
      m_brick(0), m_remaining(0), m_cull(0)
{
}

// Mac 0x33b350 / 0x33b3b0, Win 0x57f840
BICameraChangeFinderOcclusionTesterRIM::~BICameraChangeFinderOcclusionTesterRIM()
{
}

// Mac 0x33b470, Win 0x57deb0
void BICameraChangeFinderOcclusionTesterRIM::Update()
{
    if (m_remaining != 0) {
        UInt32 index = 0;
        for (BICameraList::Iterator it = m_cameras.Begin(); it != m_cameras.End(); ++it, ++index) {
            // ComputeVisibility with the occluders culled
            UInt32 visible = CountVisibleCulled(m_brick, index, *it);
            BasicSet<BIBrickPrimitive*> none;
            UInt32 total = CountVisible(m_brick, none, *it);
            m_results.PushBack((Real32)((EvalReal)(Real32)visible / (Real32)total));
        }
        RunInMainThreadObserver::Update();
        return;
    }
    for (BICameraList::Iterator it = m_cameras.Begin(); it != m_cameras.End(); ++it)
        m_results.PushBack(ComputeVisibility(m_brick, m_occluders, *it));
    RunInMainThreadObserver::Update();
}

// Mac 0x33ba90, Win 0x57f6d0
Real32 BICameraChangeFinderOcclusionTesterRIM::ComputeVisibility(BIBrick* brick,
                                                                 const BasicSet<BIBrickPrimitive*>& occluders,
                                                                 StorageScene2::Camera* camera)
{
    UInt32 visible = CountVisible(brick, occluders, camera);
    BasicSet<BIBrickPrimitive*> none;
    UInt32 total = CountVisible(brick, none, camera);
    // NOTE: 0/0 (a brick that is not drawn at all) gives NaN in both builds.
    return (Real32)((EvalReal)(Real32)visible / (Real32)total);
}

// Mac 0x33bba0, Win 0x57f210
UInt32 BICameraChangeFinderOcclusionTesterRIM::CountVisible(BIBrick* brick, const BasicSet<BIBrickPrimitive*>& occluders,
                                                            StorageScene2::Camera* camera)
{
    Texture* previous = m_render->BeginOcclusionQueries(m_target);
    // NOTE: built and not used (both builds).
    BasicSet<BIBrickPrimitive*> all(occluders);
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = brick->GetPrimitiveSet().Begin(); it != brick->GetPrimitiveSet().End(); ++it)
        all.Insert(*it);

    Matrix3D view;
    Matrix3D projection;
    Vector3 eye(camera->m_transformation.GetTranslation());
    Vector3 target = Vector3(0.0f, 0.0f, -camera->m_distance) * camera->m_transformation;
    Vector3 up(0.0f, 1.0f, 0.0f);
    Vector3 direction = target - eye;
    Real32 length = direction.GetLength();
    if (!(length > 1.0f))
        length = 1.0f;
    direction.SetLength(length);
    eye = target - direction;
    view.SetLookAt(eye, target, up);
    SetPerspective(projection, m_width, m_height);
    m_render->Clear(7);
    m_render->SetViewMatrix(view);
    m_render->SetProjectionMatrix(projection);
    RenderQueue* queue = m_render->GetRenderQueue(0);
    if (occluders.GetSize() != 0) {
        for (BasicSet<BIBrickPrimitive*>::ConstIterator it = occluders.Begin(); it != occluders.End(); ++it) {
            if (brick->Intersects(*it))
                continue;
            Matrix3D identity;
            queue->Add(GetMesh(*it), identity, 0.0f, 0, 1.0f);
        }
        queue->Flush(0.1f, true, true);
        queue->Clear();
    }
    Matrix3D identity;
    UInt32 count = m_render->GetVisibleSampleCount(GetMesh(brick), identity);
    m_render->SwitchRenderTarget(previous);
    return count;
}

// CountVisible(brick, *m_remaining + m_currentPrimitives, camera) drawing only the occluders that
// overlap the brick on the screen (BIOcclusionCull); the same count.
UInt32 BICameraChangeFinderOcclusionTesterRIM::CountVisibleCulled(BIBrick* brick, UInt32 cameraIndex,
                                                                  StorageScene2::Camera* camera)
{
    Texture* previous = m_render->BeginOcclusionQueries(m_target);
    Matrix3D view;
    Matrix3D projection;
    MakeOcclusionView(camera, m_width, m_height, view, projection);
    m_render->Clear(7);
    m_render->SetViewMatrix(view);
    m_render->SetProjectionMatrix(projection);
    RenderQueue* queue = m_render->GetRenderQueue(0);
    if (m_remaining->GetSize() != 0 || m_currentPrimitives.GetSize() != 0) {
        std::vector<SingleMesh*> draw;
        if (!m_cull->Collect(this, brick, cameraIndex, view, projection, *m_remaining, m_currentPrimitives, draw)) {
            draw.clear();
            BasicSet<BIBrickPrimitive*> occluders(*m_remaining);
            for (BasicSet<BIBrickPrimitive*>::ConstIterator it = m_currentPrimitives.Begin(); it != m_currentPrimitives.End(); ++it)
                occluders.Insert(*it);
            for (BasicSet<BIBrickPrimitive*>::ConstIterator it = occluders.Begin(); it != occluders.End(); ++it)
                if (!brick->Intersects(*it))
                    draw.push_back(GetMesh(*it));
        }
        for (std::vector<SingleMesh*>::const_iterator it = draw.begin(); it != draw.end(); ++it) {
            Matrix3D identity;
            queue->Add(*it, identity, 0.0f, 0, 1.0f);
        }
        queue->Flush(0.1f, true, true);
        queue->Clear();
    }
    Matrix3D identity;
    UInt32 count = m_render->GetVisibleSampleCount(GetMesh(brick), identity);
    m_render->SwitchRenderTarget(previous);
    return count;
}

// Mac 0x33c010, Win 0x57ebc0
SingleMesh* BICameraChangeFinderOcclusionTesterRIM::GetMesh(BIBrick* brick)
{
    BasicMap<const BIBrick*, RefPointer<SingleMesh> >::Iterator found = m_meshes->Find(brick);
    if (found != m_meshes->End())
        return (*m_meshes)[brick];
    UInt32 volumes = 0;
    for (BasicArray<PrimitiveElement*>::ConstIterator it = brick->GetElements().Begin(); it != brick->GetElements().End(); ++it)
        volumes += (*it)->GetBone().m_collisionVolumes.GetSize();
    VertexBuffer::CreateArgs args;
    args.vertexCount = volumes * 24;
    args.indexCount = volumes * 36;
    args.frameCount = 0;
    args.dynamic = false;
    args.hasNormals = true;
    args.colorSetCount = 0;
    Object* object = m_render->GetObjectFactory()->CreateObject(typeid(VertexBuffer), args);
    if (object == 0)
        return 0;
    VertexBuffer* buffer = dynamic_cast<VertexBuffer*>(object);
    if (buffer == 0)
        return 0;
    UInt32 index = 0;
    for (BasicArray<PrimitiveElement*>::ConstIterator it = brick->GetElements().Begin(); it != brick->GetElements().End(); ++it) {
        Matrix3D elementTransform;
        (*it)->GetTransform(elementTransform);
        const BasicArray<Primitive::CollisionVolume>& list = (*it)->GetBone().m_collisionVolumes;
        for (BasicArray<Primitive::CollisionVolume>::ConstIterator v = list.Begin(); v != list.End(); ++v) {
            if (v->m_type == Primitive::CollisionVolume::Capsule) {
                Real32 radius = v->m_size.y;
                // NOTE: the box of a capsule is [(0, -r/2, -r/2), (length, 0, 0)] (both builds).
                BoundingBox box(Vector3(0.0f, radius * -0.5f, radius * -0.5f),
                                Vector3(v->m_size.x, radius * 0.5f, radius * 0.5f));
                Matrix3D transform(elementTransform, v->m_transform);   // element * volume (both builds)
                AddBox(m_render, box, transform, buffer, index);
                ++index;
            } else if (v->m_type == Primitive::CollisionVolume::Sphere) {
                Real32 radius = v->m_size.x;
                BoundingBox box(Vector3(radius, radius, radius));
                box.m_min = -box.m_max;
                Matrix3D transform(elementTransform, v->m_transform);   // element * volume (both builds)
                AddBox(m_render, box, transform, buffer, index);
                ++index;
            } else if (v->m_type == Primitive::CollisionVolume::Box) {
                BoundingBox box(v->m_size);
                box.m_min = -box.m_max;
                Matrix3D transform(elementTransform, v->m_transform);   // element * volume (both builds)
                AddBox(m_render, box, transform, buffer, index);
                ++index;
            }
        }
    }
    RefPointer<SingleMesh> mesh = new SingleMesh(m_material, buffer);
    (*m_meshes)[brick] = mesh;
    return mesh;
}

// Mac 0x33c560, Win 0x57df40
void BICameraChangeFinderOcclusionTesterRIM::AddBox(Render* /*render*/, const BoundingBox& box, const Matrix3D& transform,
                                                    VertexBuffer* buffer, UInt32 index)
{
    const Vector3& lo = box.m_min;
    const Vector3& hi = box.m_max;
    // faces -z, +z, -x, +x, -y, +y; four corners each
    const Vector3 corners[24] = {
        Vector3(lo.x, lo.y, lo.z), Vector3(hi.x, lo.y, lo.z), Vector3(hi.x, hi.y, lo.z), Vector3(lo.x, hi.y, lo.z),
        Vector3(lo.x, lo.y, hi.z), Vector3(hi.x, lo.y, hi.z), Vector3(hi.x, hi.y, hi.z), Vector3(lo.x, hi.y, hi.z),
        Vector3(lo.x, lo.y, lo.z), Vector3(lo.x, lo.y, hi.z), Vector3(lo.x, hi.y, hi.z), Vector3(lo.x, hi.y, lo.z),
        Vector3(hi.x, lo.y, lo.z), Vector3(hi.x, lo.y, hi.z), Vector3(hi.x, hi.y, hi.z), Vector3(hi.x, hi.y, lo.z),
        Vector3(lo.x, lo.y, lo.z), Vector3(lo.x, lo.y, hi.z), Vector3(hi.x, lo.y, hi.z), Vector3(hi.x, lo.y, lo.z),
        Vector3(lo.x, hi.y, lo.z), Vector3(lo.x, hi.y, hi.z), Vector3(hi.x, hi.y, hi.z), Vector3(hi.x, hi.y, lo.z)
    };
    static const Real32 faceNormals[6][3] = {
        { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, 1.0f }, { -1.0f, 0.0f, 0.0f },
        { 1.0f, 0.0f, 0.0f },  { 0.0f, -1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }
    };
    Vector3 positions[24];
    Vector3 normals[24];
    for (UInt32 i = 0; i < 24; ++i)
        positions[i] = corners[i] * transform;
    for (UInt32 i = 0; i < 24; ++i)
        normals[i] = Rotate(Vector3(faceNormals[i / 4][0], faceNormals[i / 4][1], faceNormals[i / 4][2]), transform);
    UInt32 b = index * 24;
    UInt32 indices[36] = {
        b + 0,  b + 2,  b + 1,  b + 0,  b + 3,  b + 2,
        b + 5,  b + 6,  b + 4,  b + 6,  b + 7,  b + 4,
        b + 9,  b + 10, b + 8,  b + 10, b + 11, b + 8,
        b + 12, b + 14, b + 13, b + 12, b + 15, b + 14,
        b + 16, b + 18, b + 17, b + 16, b + 19, b + 18,
        b + 21, b + 22, b + 20, b + 22, b + 23, b + 20
    };
    buffer->SetPositions(positions, b, 24, 0);
    buffer->SetNormals(normals, b, 24);
    buffer->SetIndices(indices, index * 36, 36, 0);
}

} // namespace LEGO
