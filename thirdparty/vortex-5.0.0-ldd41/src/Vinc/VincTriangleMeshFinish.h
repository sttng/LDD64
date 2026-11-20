/* Vortex 5.0.0 build 800 "vortexldd41" - the common tail of the height field / triangle list vs
 * triangle mesh pair tests (repeated in the original's Mac 0x4ca3b6 / 0x4d0677,
 * Win 0x764fe0 / 0x766050): the contacts are in the mesh frame. */
#ifndef VINC_VINCTRIANGLEMESHFINISH_H
#define VINC_VINCTRIANGLEMESHFINISH_H

#include "Vinc/VincTriangleMeshContacts.h"
#include "Vinc/VincMath.h"

#include <math.h>

namespace VincTriMeshPairs {

/* result normal = normalised sum of the contact normals ((1, 0, 0) when zero), simplify, then the
 * contacts and the group normals to world space. NOTE: the result normal itself stays in the mesh
 * frame (as in the original). Returns the touch flag (result->touch is not set). */
inline int FinishMeshContacts(VincIntersectResult* r, const MeReal* tmMesh)
{
    r->normal[0] = 0.0;
    r->normal[1] = 0.0;
    r->normal[2] = 0.0;
    for (int i = 0; i < r->contactCount; ++i) {
        const VincContact* c = &r->contacts[i];
        r->normal[0] = r->normal[0] + c->normal[0];
        r->normal[1] = r->normal[1] + c->normal[1];
        r->normal[2] = r->normal[2] + c->normal[2];
    }
    MeReal x = r->normal[0], y = r->normal[1], z = r->normal[2];
    MeReal l2 = x * x + y * y + z * z;
    if (l2 <= 0.0) {
        r->normal[0] = 1.0;
        r->normal[1] = 0.0;
        r->normal[2] = 0.0;
    } else {
        MeReal inv = 1.0 / sqrt(l2);
        r->normal[0] = x * inv;
        r->normal[1] = y * inv;
        r->normal[2] = inv * z;
    }
    VincContactSimplifyResult(r);
    for (int i = 0; i < r->contactCount; ++i) {
        VincContact* c = &r->contacts[i];
        MeVector3 p = {c->position[0], c->position[1], c->position[2]};
        VincTMTransform(tmMesh, p, c->position);
        MeVector3 n = {c->normal[0], c->normal[1], c->normal[2]};
        VincTMRotate(tmMesh, n, c->normal);
    }
    for (int i = 0; i < r->contactGroupCount; ++i) {
        VincContactGroup* g = &r->groups[i];
        MeVector3 n = {g->normal[0], g->normal[1], g->normal[2]};
        VincTMRotate(tmMesh, n, g->normal);
    }
    return 0 < r->contactCount;
}

} // namespace VincTriMeshPairs

#endif
