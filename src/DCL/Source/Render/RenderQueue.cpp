// LEGO Digital Designer — restored source
// LEGO::RenderQueue — see RenderQueue.h. TU 042 (Mac 0x102690-0x10631f) + TU 041 (predicates).
//
// Sorting is std::sort with the two TU 041 predicates (Mac: the libstdc++ introsort
// 0x104160/0x1052d0 + insertion sorts 0x104e00/0x1050d0/0x105e10/0x1060e0 are its template
// instances; Win 0x4f3ac0/0x4f3c10 are the VC9 std::sort instances).
// NOTE: std::sort is not stable and the VC9 and libstdc++ algorithms differ, so entries whose
// keys (layer, depth, material) compare equal can come out in a different order in the two
// original builds (and in the restored Win build, which uses the host library).
#include "RenderQueue.h"
#include "Material.h"
#include "OpenGLFactories.h"

#include <algorithm>
#include <functional>
#include "MathUtil.h"
#include <limits>

namespace LEGO {

namespace {

// The depth as a sort key: NaN taken as +infinity (M16).
inline Real32 SortDepth(Real32 depth)
{
    return depth != depth ? std::numeric_limits<Real32>::infinity() : depth;
}

// Mac 0x1025b0 (TU 041, name inferred): layer ascending, then depth ascending (front to back),
// then material address. Written with the original's comparisons.
// NOTE: in the original an unordered depth (NaN) compares as "less" in both directions, which is
// not a strict weak ordering (undefined behaviour in std::sort: entries out of order, or reads
// past the range in the unguarded insertion sort); fixed: NaN sorts as +infinity (M16).
struct LessFrontToBack {
    bool operator()(const RenderEntry& a, const RenderEntry& b) const
    {
        if (!((Int32)b.layer <= (Int32)a.layer))
            return true;
        if ((Int32)b.layer < (Int32)a.layer)
            return false;
        const Real32 depthA = SortDepth(a.depth);
        const Real32 depthB = SortDepth(b.depth);
        if (!(depthB <= depthA))
            return true;
        if (!(depthA <= depthB))
            return false;
        return std::less<Material*>()(a.mesh->GetMaterial(), b.mesh->GetMaterial());
    }
};

// Mac 0x102600 (TU 041, name inferred): layer descending, then depth descending (back to
// front), then material address ascending. NOTE: NaN depths as in LessFrontToBack (M16).
struct LessBackToFront {
    bool operator()(const RenderEntry& a, const RenderEntry& b) const
    {
        if (!((Int32)a.layer <= (Int32)b.layer))
            return true;
        if ((Int32)a.layer < (Int32)b.layer)
            return false;
        const Real32 depthA = SortDepth(a.depth);
        const Real32 depthB = SortDepth(b.depth);
        if (!(depthA <= depthB))
            return true;
        if (!(depthB <= depthA))
            return false;
        return std::less<Material*>()(a.mesh->GetMaterial(), b.mesh->GetMaterial());
    }
};

} // namespace

// Mac 0x103b70 (BasicArray<RenderEntry>::PushBack instance) with the entry built in place.
void RenderQueue::Push(EntryArray& entries, Mesh* mesh, const Matrix3D& transform, Real32 depth,
                       UInt32 layer, Real32 opacity)
{
    RenderEntry entry;                                                // Win 0x4eb750
    entry.cullRadius = -1.0f;
    entry.mesh = mesh;
    entry.transform = transform;
    entry.depth = depth;
    entry.layer = layer;
    entry.opacity = opacity;
    entries.PushBack(entry);
}

// ---- view culling (LDD64) --------------------------------------------------------------------
// DrawEntries skips entries whose sphere lies wholly outside the view volume (they draw no pixel).
// Skipping happens after the original's sort, so the meshes drawn and their order are the
// original's (std::sort is not stable). The next drawn entry sets the state a skipped one would.

void RenderQueue::PushBounded(EntryArray& entries, Mesh* mesh, const Matrix3D& transform,
                              Real32 depth, UInt32 layer, Real32 opacity)
{
    Push(entries, mesh, transform, depth, layer, opacity);
    entries.Back().cullCenter = m_nextCenter;
    entries.Back().cullRadius = m_nextRadius;
    m_nextRadius = -1.0f;
}

void RenderQueue::EnableCulling(const Matrix3D& view, Real32 fovX, Real32 fovY, Real32 zNear,
                                Real32 zFar)
{
    // the half angles a little wider than the projection's (a margin for rounding)
    Real32 halfX = fovX * 0.51f + 0.01f;
    Real32 halfY = fovY * 0.51f + 0.01f;
    if (!(halfX > 0.0f && halfX < 1.55f && halfY > 0.0f && halfY < 1.55f && zNear >= 0.0f &&
          zFar > zNear)) {
        m_cull = false;
        return;
    }
    m_cullView = view;
    m_cullCosX = Cos(halfX);
    m_cullSinX = Sin(halfX);
    m_cullCosY = Cos(halfY);
    m_cullSinY = Sin(halfY);
    m_cullNear = zNear;
    m_cullFar = zFar;
    m_cull = true;
}

bool RenderQueue::IsCulled(const RenderEntry& entry) const
{
    const Real32 r = entry.cullRadius;
    if (!(r >= 0.0f))
        return false;
    const Vector3 v = entry.cullCenter * m_cullView;
    const Real32 d = -v.z;   // distance in front of the eye
    // NaN anywhere: every comparison is false, the entry is drawn
    if (d + r < m_cullNear || d - r > m_cullFar)
        return true;
    if (v.x * m_cullCosX + v.z * m_cullSinX > r || -v.x * m_cullCosX + v.z * m_cullSinX > r)
        return true;
    if (v.y * m_cullCosY + v.z * m_cullSinY > r || -v.y * m_cullCosY + v.z * m_cullSinY > r)
        return true;
    return false;
}

// Mac 0x1032e0, Win 0x4eb900
void RenderQueue::Add(Mesh* mesh, const Matrix3D& transform, Real32 depth, UInt32 layer,
                      Real32 opacity)
{
    if (mesh->IsOpaque())
        PushBounded(m_opaque, mesh, transform, depth, layer, opacity);
    else if (mesh->IsDecal())
        PushBounded(m_decal, mesh, transform, depth, layer, opacity);
    else
        PushBounded(m_transparent, mesh, transform, depth, layer, opacity);
}

// Mac 0x103540, Win 0x4eb9f0
void RenderQueue::AddWithDepthPrepass(Mesh* mesh, const Matrix3D& transform, Real32 depth,
                                      UInt32 layer, Real32 opacity, bool noDepthPrepass)
{
    bool opaque = mesh->IsOpaque();
    if (!noDepthPrepass && opaque)
        PushBounded(m_twoPass, mesh, transform, depth, layer, opacity);
    else if (mesh->IsDecal())
        PushBounded(m_twoPassDecal, mesh, transform, depth, layer, opacity);
    else
        PushBounded(m_twoPassTransparent, mesh, transform, depth, layer, opacity);
}

// Mac 0x1037b0, Win 0x4ebaf0
void RenderQueue::AddTransparent(Mesh* mesh, const Matrix3D& transform, Real32 depth, UInt32 layer,
                                 Real32 opacity)
{
    PushBounded(m_transparent, mesh, transform, depth, layer, opacity);
}

// Mac 0x103880, Win 0x4ebb80: releases the meshes, keeps the storage.
void RenderQueue::Clear()
{
    m_opaque.Clear();
    m_decal.Clear();
    m_transparent.Clear();
    m_twoPass.Clear();
    m_twoPassDecal.Clear();
    m_twoPassTransparent.Clear();
}

// Win 0x4f3a40; inlined on Mac. Per entry: world matrix (Render slot 41), opacity (48),
// Mesh::BeginDraw, Mesh::Bind(depth, useMaterial), DrawVertexBuffer (63), Mesh::EndDraw.
void RenderQueue::DrawEntries(const EntryArray& entries, bool useMaterial)
{
    for (EntryArray::ConstIterator it = entries.Begin(); it != entries.End(); ++it) {
        if (m_cull && IsCulled(*it))
            continue;
        Render* render = m_render;
        render->SetWorldMatrix(it->transform);
        render->SetOpacity(it->opacity);
        it->mesh->BeginDraw(render);
        it->mesh->Bind(render, it->depth, useMaterial);
        render->DrawVertexBuffer();
        it->mesh->EndDraw(render);
    }
}

// Mac 0x102690, Win 0x4f3690.
// Order: opaque; decals (blend SRC_ALPHA/ONE_MINUS_SRC_ALPHA = blend func indices 2/3, no depth
// writes); two-pass (colour+alpha writes off: depth only; then blended); two-pass decals
// (blended); transparent (blended); two-pass transparent (two passes). Material reset at the end.
void RenderQueue::Flush(Real32 unused, bool sort, bool useMaterial)
{
    (void)unused;
    if (sort) {
        std::sort(m_opaque.Begin(), m_opaque.End(), LessFrontToBack());
        std::sort(m_twoPass.Begin(), m_twoPass.End(), LessFrontToBack());
        std::sort(m_twoPassTransparent.Begin(), m_twoPassTransparent.End(), LessFrontToBack());
        std::sort(m_transparent.Begin(), m_transparent.End(), LessBackToFront());
        std::sort(m_decal.Begin(), m_decal.End(), LessBackToFront());
        std::sort(m_twoPassDecal.Begin(), m_twoPassDecal.End(), LessBackToFront());
    }

    DrawEntries(m_opaque, useMaterial);

    if (!m_decal.IsEmpty()) {
        m_render->EnableBlend(true);
        m_render->SetBlendFunc(2, 3);
        m_render->EnableDepthWrite(false);
        m_render->SetMaterial(0);
        DrawEntries(m_decal, useMaterial);
        m_render->EnableDepthWrite(true);
        m_render->EnableBlend(false);
    }

    if (!m_twoPass.IsEmpty()) {
        m_render->EnableColorWrite(false);
        m_render->EnableAlphaWrite(false);
        DrawEntries(m_twoPass, useMaterial);
        m_render->EnableAlphaWrite(true);
        m_render->EnableColorWrite(true);
        m_render->EnableBlend(true);
        m_render->SetBlendFunc(2, 3);
        m_render->SetMaterial(0);
        DrawEntries(m_twoPass, useMaterial);
        m_render->SetOpacity(1.0f);
        m_render->SetMaterial(0);
        m_render->EnableBlend(false);
    }

    if (!m_twoPassDecal.IsEmpty()) {
        m_render->EnableBlend(true);
        m_render->SetBlendFunc(2, 3);
        m_render->EnableDepthWrite(false);
        m_render->SetMaterial(0);
        DrawEntries(m_twoPassDecal, useMaterial);
        m_render->EnableDepthWrite(true);
        m_render->EnableBlend(false);
    }

    if (!m_transparent.IsEmpty()) {
        m_render->EnableBlend(true);
        m_render->SetBlendFunc(2, 3);
        m_render->EnableDepthWrite(false);
        m_render->SetMaterial(0);
        DrawEntries(m_transparent, useMaterial);
        m_render->EnableDepthWrite(true);
        m_render->EnableBlend(false);
    }

    if (!m_twoPassTransparent.IsEmpty()) {
        m_render->EnableColorWrite(false);
        m_render->EnableAlphaWrite(false);
        DrawEntries(m_twoPassTransparent, useMaterial);
        m_render->EnableAlphaWrite(true);
        m_render->EnableColorWrite(true);
        m_render->EnableBlend(true);
        m_render->SetBlendFunc(2, 3);
        m_render->SetMaterial(0);
        DrawEntries(m_twoPassTransparent, useMaterial);
        m_render->SetOpacity(1.0f);
        m_render->SetMaterial(0);
        m_render->EnableBlend(false);
    }

    m_render->SetMaterial(0);
}

// OpenGLFactories.h seam: `new RenderQueue(render, capacity)` in Render::GetRenderQueue
// (Mac 0xf4640, Win 0x4eb7f0 call site).
RenderQueue* CreateRenderQueue(Render* render, UInt32 capacity)
{
    return new RenderQueue(render, capacity);
}

} // namespace LEGO
