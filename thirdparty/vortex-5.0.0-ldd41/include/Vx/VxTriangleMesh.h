// Vortex 5.0.0 build 800 "vortexldd41" - VxTriangleMesh and VxTriangleMeshData (partial restored
// header): the accessors vxTriangleMeshFluidInteractionForce (Mac TU 486) uses.
// Otherwise Win-only (VxTriangleMesh: vtable Win 0xa0047c / 0xa0046c, class id 0x133, ctor
// 0x7aa900, bodies 0x7aa9d0-0x7ab3e0; VxTriangleMeshData: no vtable, ctor 0x7eab40, bodies around
// 0x7ea000-0x7eb000, "VxTriangleMeshData::loadFromFile"). Mac keeps only these accessors: Mac TU 526
// (0x4804d4-0x480623) and TU 527 (0x480624-0x4806ed). All names inferred.
#ifndef VX_VXTRIANGLEMESH_H
#define VX_VXTRIANGLEMESH_H

#include "Vx/VxGeometry.h"

namespace Vx {

// One corner of a triangle in the mesh data (16 bytes; only the vertex pointer is read here).
struct VxTriangleMeshCorner {
    const VxReal* vertex; // +0x00
    int m04;              // +0x04 (meaning unknown)
    int m08;              // +0x08
    int m0c;              // +0x0c
};

class VxTriangleMeshData {
public:
    // Mac 0x480624, Win 0x7ea570: the three vertices of triangle i (no range check)
    void getTriangleVertices(int i, const VxReal*& v0, const VxReal*& v1, const VxReal*& v2) const;

    unsigned char m00[0x48];            // +0x00 (not read here)
    VxArray<VxVector3> mNormals;        // +0x48: per-triangle normals
    VxArray<void*> mTriangles;          // +0x58: one entry per triangle (its size is the count)
    int m68;                            // +0x68
    VxReal m6c;                         // Mac +0x6c, Win +0x70 (Win 0x7aaa00 / 0x7aaa20 set / get it)
    VxTriangleMeshCorner* mCorners;     // Mac +0x74, Win +0x78: 3 per triangle

private:
    VxTriangleMeshData(); // Win 0x7eab40
};

class VxTriangleMesh : public VxGeometry {
public:
    // Mac 0x4804d4, Win 0x7aaea0
    int getTriangleCount() const;
    // Mac 0x4804e8, Win 0x7aaf90: the normal of triangle i (no range check)
    void getTriangleNormal(int i, VxVector3& n) const;
    // Mac 0x480544 (VincTriangleMeshGetRootOBB), Win 0x7aaa90: frame and half extents of the
    // root box of the mesh's OBB tree
    void getBoundingBox(VxTransform& tm, VxReal& hx, VxReal& hy, VxReal& hz) const;
    // Mac 0x48058c, Win 0x7aa9f0 (-> VxTriangleMeshData::getTriangleVertices)
    void getTriangleVertices(int i, const VxReal*& v0, const VxReal*& v1, const VxReal*& v2) const;

    int m6c;                            // Mac +0x6c, Win +0x88
    int m70;                            // Mac +0x70, Win +0x8c
    VxTriangleMeshData* mData;          // Mac +0x74, Win +0x90

private:
    VxTriangleMesh(); // Win 0x7aa900
};

} // namespace Vx

#endif
