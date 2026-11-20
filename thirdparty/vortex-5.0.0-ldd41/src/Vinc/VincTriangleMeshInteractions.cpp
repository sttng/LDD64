// Vortex 5.0.0 build 800 "vortexldd41" - TriangleMesh registration helpers of sub-TU 557d
// (restored; Mac 0x4d1f97-0x4d2082, Win 0x75cae0, 0x75cc20, 0x766ba0).
#include "Vinc/VincTriangleMeshContacts.h"
#include "Vinc/VincPrimitives.h"

extern "C" {

// Mac 0x4d1f97, Win 0x75cc20 (name inferred)
MeI16 MEAPI VincTriangleMeshInteractionsGetTypeId(void)
{
    return VincTriangleMeshGetTypeId();
}

// Mac 0x4d1fa5, Win 0x75cae0 (name inferred)
void MEAPI VincTriangleMeshInit(void)
{
    VincTriangleMeshRegisterType();
    VincTriangleMeshRegisterLineSegment();
}

// Mac 0x4d1fb7 (Win: inlined into its caller; name inferred). Mac TU 526 0x480544 reads the
// centre (row 3) of the returned TM.
void MEAPI VincTriangleMeshGetRootOBB(VincGeometryID mesh, MeMatrix4 tm, MeVector3 halfExtents)
{
    const VincOBBNode* root = ((VincTriangleMesh*)mesh)->tree->nodes;
    halfExtents[0] = root->halfExtents[0];
    halfExtents[1] = root->halfExtents[1];
    halfExtents[2] = root->halfExtents[2];
    for (int i = 0; i < 3; ++i) {
        tm[i][0] = root->axis[i][0];
        tm[i][1] = root->axis[i][1];
        tm[i][2] = root->axis[i][2];
        tm[i][3] = 0.0;
    }
    tm[3][0] = root->center[0];
    tm[3][1] = root->center[1];
    tm[3][2] = root->center[2];
    tm[3][3] = 1.0;
}

// Mac 0x4d2082, Win 0x766ba0 (name inferred)
int MEAPI VincTriangleMeshRegisterInteractions(void)
{
    VincTriangleMeshRegisterLineSegment();
    VincTriangleMeshPlaneRegisterInteraction();
    VincTriangleMeshSphereRegisterInteraction();
    VincTriangleMeshBoxRegisterInteraction();
    VincTriangleMeshCylinderRegisterInteraction();
    return 1;
}

} // extern "C"
