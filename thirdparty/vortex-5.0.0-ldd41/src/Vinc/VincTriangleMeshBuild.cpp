// Vortex 5.0.0 build 800 "vortexldd41" - VincTriangleMeshBuild (restored; Win-only
// 0x7b3e80 / 0x7b3f60, dead-stripped on the Mac). The OBB tree construction it calls (Win 0x80e270 /
// 0x80dc60 / 0x80e150) is in VincOBBTreeBuild.cpp (fixes item). LDD never builds a triangle mesh.
#include "Vinc/VincTriangleMesh.h"
#include "Me/MeMemory.h"

extern "C" {

// Win 0x7b3e80 (name inferred)
int MEAPI VincTriangleMeshBuildTree(VincGeometryID g)
{
    VincTriangleMesh* m = (VincTriangleMesh*)g;
    if (m->tree != 0) {
        VincOBBTreeDestroy(m->tree);
        MeMemoryAPI.destroy(m->tree);
    }
    VincOBBTree* t = (VincOBBTree*)MeMemoryAPI.create(sizeof(VincOBBTree));
    m->tree = t;
    if (t != 0) {
        unsigned int flags = (unsigned int)m->flags;
        t->nodes = 0;
        t->m08 = 0;
        t->m0c = flags & 1;
        t->m10 = flags & 2;
        t->m14 = -1;
        t->m18 = 0;
        t->m1c = 0;
        t->m20 = 0;
        t->m24 = 0;
        t->m28 = 0;
        VincOBBTreeInit(t, g);
    }
    // NOTE: the original goes on with a NULL tree when the allocation failed
    VincOBBTreeSetMaxDepth(m->tree, m->m80);
    VincOBBTreeBuild(m->tree);
    m->m6c = m->radius * (MeReal)0.01f;
    m->m74 = (MeReal)0.8f;
    return 1;
}

// Win 0x7b3f60 ("VincTriangleMeshBuild(mesh)" in VincErrorTable id 29) (name from the string)
int MEAPI VincTriangleMeshBuild(VincGeometryID g)
{
    VincTriangleMesh* m = (VincTriangleMesh*)g;
    if (m->areas != 0)
        MeMemoryAPI.destroy(m->areas);
    m->areas = (MeReal*)MeMemoryAPI.create(m->triangleCount * sizeof(MeReal));
    if (m->m08 != 0)
        MeMemoryAPI.destroy(m->m08);
    m->m08 = (int*)MeMemoryAPI.create(m->triangleCount * sizeof(int));
    for (int i = 0; i < m->triangleCount; ++i)
        m->m08[i] = 1;
    VincTriangleMeshComputeAreas(g);
    VincTriangleMeshComputeBounds(g);
    return VincTriangleMeshBuildTree(g);
}

} // extern "C"
