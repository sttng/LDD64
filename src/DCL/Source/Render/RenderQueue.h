// LEGO Digital Designer — restored source
// LEGO::RenderQueue — a frame's mesh draws in six lists (opaque / decal / transparent, plain and
// with depth pre-pass), sorted and drawn with the right blend / depth-write state. Names inferred.
// TU 042 (Mac 0x102690-0x10631f, Win 0x4eb7f0-0x4ebd90 + Flush 0x4f3690, helpers
// 0x4f3a40-0x4f3c10..) and TU 041 (sort predicates Mac 0x1025b0 / 0x102600, inlined on Win).
// Inline ctor: Mac 0xf4730 (in GetRenderQueue 0xf4640), Win 0x4eb7f0.
// Vtables: Mac 0x709a9c {Add 0x1032e0, AddWithDepthPrepass 0x103540, AddTransparent 0x1037b0,
// Clear 0x103880, Flush 0x102690, D1 0x1039c0, D0 0x103a20}; Win 0x9d7180 {0x4eb900, 0x4eb9f0,
// 0x4ebaf0, 0x4ebb80, 0x4f3690} (+ Object vtable 0x9d716c, dtor 0x4ebc20 / sdd 0x4ebd90).
// Layout: Mac {vptr, 6 x BasicArray<RenderEntry> +0x04..+0x64, Render* +0x64, Object +0x68}
// (sizeof 0x70); Win {vptr, vbptr, arrays +0x08..+0x68, Render* +0x68, Object +0x6c} (0x74).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "Geometry.h"
#include "Render.h"
#include "Mesh.h"

namespace LEGO {

class RenderQueue : public virtual Object {
public:
    // Mac 0xf4730, Win 0x4eb7f0 (thiscall(render, capacity, mostDerived)): reserves `capacity`
    // entries in the opaque and the transparent list.
    RenderQueue(Render* render, UInt32 capacity) : m_render(render)
    {
        m_cull = false;
        m_nextRadius = -1.0f;
        m_opaque.Reserve(capacity);
        m_transparent.Reserve(capacity);
    }
    virtual ~RenderQueue() {}                                         // Mac 0x1039c0/0x103a20, Win 0x4ebc20

    // Queues `mesh` at `transform`: opaque meshes into the opaque list, decals into the decal
    // list, everything else into the transparent list.
    virtual void Add(Mesh* mesh, const Matrix3D& transform, Real32 depth, UInt32 layer,
                     Real32 opacity);                                 // 0 Mac 0x1032e0, Win 0x4eb900
    // Like Add, into the second set of lists. Opaque meshes go to m_twoPass unless
    // `noDepthPrepass` (then, like non-decal transparent meshes, to m_twoPassTransparent, which
    // is drawn the same way).
    virtual void AddWithDepthPrepass(Mesh* mesh, const Matrix3D& transform, Real32 depth,
                                     UInt32 layer, Real32 opacity,
                                     bool noDepthPrepass);            // 1 Mac 0x103540, Win 0x4eb9f0
    // Always into the transparent list.
    virtual void AddTransparent(Mesh* mesh, const Matrix3D& transform, Real32 depth, UInt32 layer,
                                Real32 opacity);                      // 2 Mac 0x1037b0, Win 0x4ebaf0
    // Empties every list (keeps the capacity).
    virtual void Clear();                                             // 3 Mac 0x103880, Win 0x4ebb80
    // Optionally sorts, then draws every list (see RenderQueue.cpp for the order and states).
    // `unused` is not read by either build.
    virtual void Flush(Real32 unused, bool sort, bool useMaterial);   // 4 Mac 0x102690, Win 0x4f3690

    Render* GetRender() const { return m_render; }

    // A sphere (world space) around the mesh of the next Add / AddWithDepthPrepass /
    // AddTransparent call; entries queued without one are never culled.
    void SetNextBounds(const Vector3& center, Real32 radius)
    {
        m_nextCenter = center;
        m_nextRadius = radius;
    }
    // From now until DisableCulling, Flush skips the entries whose sphere lies wholly outside
    // the view of a camera with view matrix `view` (looking down -z), full fields of view
    // `fovX` / `fovY` (radians) and near / far planes.
    void EnableCulling(const Matrix3D& view, Real32 fovX, Real32 fovY, Real32 zNear, Real32 zFar);
    void DisableCulling() { m_cull = false; }

protected:
    typedef BasicArray<RenderEntry> EntryArray;

    // Win 0x4f3a40 (EBX = list, stdcall(queue, useMaterial)); inlined six times on Mac.
    void DrawEntries(const EntryArray& entries, bool useMaterial);
    static void Push(EntryArray& entries, Mesh* mesh, const Matrix3D& transform, Real32 depth,
                     UInt32 layer, Real32 opacity);                   // Mac 0x103b70 (+ entry copy)
    // Push with the pending SetNextBounds sphere (then cleared).
    void PushBounded(EntryArray& entries, Mesh* mesh, const Matrix3D& transform, Real32 depth,
                     UInt32 layer, Real32 opacity);
    // Whether `entry` lies wholly outside the culling view.
    bool IsCulled(const RenderEntry& entry) const;

    // Sorted with LessFrontToBack: opaque lists; with LessBackToFront: decal/transparent lists.
    EntryArray m_opaque;              // Mac +0x04, Win +0x08 Add: opaque meshes (drawn plainly)
    EntryArray m_decal;               // Mac +0x14, Win +0x18 Add: decals (blended)
    EntryArray m_transparent;         // Mac +0x24, Win +0x28 Add/AddTransparent: the rest (blended)
    EntryArray m_twoPass;             // Mac +0x34, Win +0x38 AddWithDepthPrepass: opaque meshes
                                      //   (depth-only pass, then blended)
    EntryArray m_twoPassDecal;        // Mac +0x44, Win +0x48 AddWithDepthPrepass: decals (blended)
    EntryArray m_twoPassTransparent;  // Mac +0x54, Win +0x58 AddWithDepthPrepass: the rest, and
                                      //   opaque meshes with noDepthPrepass (two passes)
    Render*    m_render;              // Mac +0x64, Win +0x68 (not referenced)
    bool       m_cull;
    Matrix3D   m_cullView;
    Real32     m_cullCosX, m_cullSinX, m_cullCosY, m_cullSinY, m_cullNear, m_cullFar;
    Vector3    m_nextCenter;
    Real32     m_nextRadius;

private:
    RenderQueue(const RenderQueue&);
    RenderQueue& operator=(const RenderQueue&);
};

} // namespace LEGO
