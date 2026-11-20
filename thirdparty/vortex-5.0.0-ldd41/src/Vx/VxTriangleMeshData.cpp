// Vortex 5.0.0 build 800 "vortexldd41" - VxTriangleMeshData accessor (restored).
// Mac TU 527 0x480624-0x4806ed (static initialiser 0x480688: <iostream>): the only part of the
// Win-only class the Mac build keeps. The rest (Win ctor 0x7eab40, loadFromFile ...) is not
// restored. See Vx/VxTriangleMesh.h.
#include "Vx/VxTriangleMesh.h"
#include <iostream>

namespace Vx {

// Mac 0x480624, Win 0x7ea570
void VxTriangleMeshData::getTriangleVertices(int i, const VxReal*& v0, const VxReal*& v1, const VxReal*& v2) const
{
    v0 = mCorners[i * 3].vertex;
    v1 = mCorners[i * 3 + 1].vertex;
    v2 = mCorners[i * 3 + 2].vertex;
}

} // namespace Vx
