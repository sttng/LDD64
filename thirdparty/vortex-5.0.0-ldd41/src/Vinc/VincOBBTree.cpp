// Vortex 5.0.0 build 800 "vortexldd41" - OBB tree of the triangle mesh: box-box separating axis
// test and tree teardown (restored; Mac 557c 0x4c274c-0x4c3103). The tree is C++
// (operator delete[] in the teardown); its construction is Win-only (VincOBBTreeBuild.cpp).
#include "Vinc/VincTriangleMesh.h"
#include "Me/MeMemory.h"

#include <math.h>

namespace {

// NOTE: the original takes |x| in single precision (cvtsd2ss / andps / cvtss2sd: a float fabs
// on the doubles, MathEngine's MeFabs heritage); kept for bit-exact results.
inline MeReal AbsF(MeReal x)
{
    return (MeReal)fabsf((float)x) + 1e-6;
}

// Mac 0x4c3103 (name inferred)
void FreeNodes(VincOBBNode* nodes, int /*count*/)
{
    if (nodes != 0)
        MeMemoryAPI.destroy(nodes);
}

} // namespace

extern "C" {

// Mac 0x4c274c, Win 0x7cd8d0 (name inferred; cdecl on Win). relTM: rotation rows p[0..2], p[4..6], p[8..10], translation
// p[12..14] of box b in box a's frame; a, b: half extents.
int VincOBBDisjoint(const MeReal* p, const MeReal* a, const MeReal* b)
{
    MeReal t0 = p[12], t1 = p[13], t2 = p[14];
    MeReal a0 = a[0], a1 = a[1], a2 = a[2];
    MeReal b0 = b[0], b1 = b[1], b2 = b[2];
    MeReal r = a0 * a[0] + a1 * a[1] + a2 * a[2] + (b0 * b[0] + b1 * b[1] + b2 * b[2]);
    if (t0 * t0 + t1 * t1 + t2 * t2 > r + r)
        return 16;
    MeReal s, d;
    MeReal A00 = AbsF(p[0]), A10 = AbsF(p[4]), A20 = AbsF(p[8]);
    s = b0 * A00 + b1 * A10 + b2 * A20 + a0;
    if (s < t0 || t0 < -s)
        return 1;
    MeReal A01 = AbsF(p[1]), A11 = AbsF(p[5]), A21 = AbsF(p[9]);
    s = b0 * A01 + b1 * A11 + b2 * A21 + a1;
    if (s < t1 || t1 < -s)
        return 2;
    MeReal A02 = AbsF(p[2]), A12 = AbsF(p[6]), A22 = AbsF(p[10]);
    s = b0 * A02 + b1 * A12 + b2 * A22 + a2;
    if (s < t2 || t2 < -s)
        return 3;
    d = p[0] * t0 + t1 * p[1] + t2 * p[2];
    s = a0 * A00 + a1 * A01 + a2 * A02 + b0;
    if (s < d || d < -s)
        return 4;
    d = p[4] * t0 + t1 * p[5] + p[6] * t2;
    s = a0 * A10 + a1 * A11 + a2 * A12 + b1;
    if (s < d || d < -s)
        return 5;
    d = p[8] * t0 + t1 * p[9] + p[10] * t2;
    s = a0 * A20 + a1 * A21 + a2 * A22 + b2;
    if (s < d || d < -s)
        return 6;
    d = p[1] * t2 - t1 * p[2];
    s = b1 * A20 + b2 * A10 + a1 * A02 + a2 * A01;
    if (s < d || d < -s)
        return 7;
    d = p[5] * t2 - t1 * p[6];
    s = b0 * A20 + b2 * A00 + a1 * A12 + a2 * A11;
    if (s < d || d < -s)
        return 8;
    d = p[9] * t2 - t1 * p[10];
    s = b0 * A10 + b1 * A00 + a1 * A22 + a2 * A21;
    if (s < d || d < -s)
        return 9;
    d = t0 * p[2] - p[0] * t2;
    s = b1 * A21 + b2 * A11 + a0 * A02 + a2 * A00;
    if (s < d || d < -s)
        return 10;
    d = p[6] * t0 - p[4] * t2;
    s = b0 * A21 + b2 * A01 + a0 * A12 + a2 * A10;
    if (s < d || d < -s)
        return 11;
    d = p[10] * t0 - p[8] * t2;
    s = b0 * A11 + b1 * A01 + a0 * A22 + a2 * A20;
    if (s < d || d < -s)
        return 12;
    d = t1 * p[0] - p[1] * t0;
    s = b1 * A22 + b2 * A12 + a0 * A01 + a1 * A00;
    if (s < d || d < -s)
        return 13;
    d = t1 * p[4] - p[5] * t0;
    s = b0 * A22 + b2 * A02 + a0 * A11 + a1 * A10;
    if (s < d || d < -s)
        return 14;
    d = t1 * p[8] - p[9] * t0;
    s = b0 * A12 + b1 * A02 + a0 * A21 + a1 * A20;
    if (s < d || d < -s)
        return 15;
    return 0;
}

// Mac 0x4c3076 (name inferred)
void MEAPI VincOBBTreeTerm(VincOBBTree* tree)
{
    int n = VincTriangleMeshGetTriangleCount(&tree->mesh->base);
    FreeNodes(tree->nodes, n * 2 - 1);
    if (tree->m18 != 0)
        MeMemoryAPI.destroy(tree->m18);
    if (tree->m28 != 0)
        MeMemoryAPI.destroy(tree->m28);
    if (tree->m1c != 0)
        delete[] (char*)tree->m1c;
    if (tree->m20 != 0)
        delete[] (char*)tree->m20;
}

// Mac 0x4c30f0, Win 0x80e1e0 (name inferred)
void MEAPI VincOBBTreeDestroy(VincOBBTree* tree)
{
    VincOBBTreeTerm(tree);
}

} // extern "C"
