// Vortex 5.0.0 build 800 "vortexldd41" - VxConvexMesh (partial restored header):
// the face accessors vxConvexMeshFluidInteractionForce (Mac TU 486) uses. Otherwise Win-only (vtable
// Win 0xa00778 / 0xa00768, class id 0x12d; bodies 0x7aba60-0x7abc70). Mac keeps these four
// inline forwarders out of line in Mac TU 474 (0x4423c0-0x442443).
// A VxGeometry (Mac 0x6c, Win 0x88 bytes) on a Convex (type 9) hull.
#ifndef VX_VXCONVEXMESH_H
#define VX_VXCONVEXMESH_H

#include "Vx/VxGeometry.h"
#include "Vinc/VincConvexInteractions.h"

namespace Vx {

class VxConvexMesh : public VxGeometry {
public:
    // Mac 0x4423de, Win 0x7ab930
    int getFaceCount() const { return VincConvexGetFaceCount((VincGeometryID)mGeometry); }
    // Mac 0x4423c0, Win 0x7ab940
    int getFaceVertexCount(int face) const { return VincConvexGetFaceVertexCount((VincGeometryID)mGeometry, face); }
    // Mac 0x4423f4, Win 0x7ab980
    void getFaceVertex(int face, int k, VxVector3& v) const
    {
        VincConvexGetFaceVertex((VincGeometryID)mGeometry, face, k, v.data());
    }
    // Mac 0x442420, Win 0x7ab960
    void getFaceNormal(int face, VxVector3& n) const { VincConvexGetFaceNormal((VincGeometryID)mGeometry, face, n.data()); }

private:
    VxConvexMesh(); // made by the Win-only factory code
};

} // namespace Vx

#endif
