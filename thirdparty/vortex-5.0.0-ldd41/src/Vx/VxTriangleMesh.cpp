// Vortex 5.0.0 build 800 "vortexldd41" - VxTriangleMesh accessors (restored).
// Mac TU 526 0x4804d4-0x480623 (static initialiser 0x4805be: <iostream>): the only part of the
// Win-only class the Mac build keeps (used by the fluid force function, Mac TU 486). The rest of
// the class (Win 0x7aa900-0x7ab3e0) is not restored. See Vx/VxTriangleMesh.h.
#include "Vx/VxTriangleMesh.h"
#include "Vinc/VincTriangleMeshContacts.h"
#include <iostream>

namespace Vx {

// Mac 0x4804d4, Win 0x7aaea0
int VxTriangleMesh::getTriangleCount() const
{
    return mData->mTriangles.size();
}

// Mac 0x4804e8, Win 0x7aaf90
void VxTriangleMesh::getTriangleNormal(int i, VxVector3& n) const
{
    const VxVector3* src = mData->mNormals.begin() + i;
    if (src != &n)
        n = *src;
}

// Mac 0x480544, Win 0x7aaa90
void VxTriangleMesh::getBoundingBox(VxTransform& tm, VxReal& hx, VxReal& hy, VxReal& hz) const
{
    MeVector3 h;
    VincTriangleMeshGetRootOBB((VincGeometryID)mGeometry, (MeReal(*)[4])tm.m, h);
    hx = h[0];
    hy = h[1];
    hz = h[2];
}

// Mac 0x48058c, Win 0x7aa9f0
void VxTriangleMesh::getTriangleVertices(int i, const VxReal*& v0, const VxReal*& v1, const VxReal*& v2) const
{
    mData->getTriangleVertices(i, v0, v1, v2);
}

} // namespace Vx
