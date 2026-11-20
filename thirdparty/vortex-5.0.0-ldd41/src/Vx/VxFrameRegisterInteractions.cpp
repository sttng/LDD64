// Vortex 5.0.0 build 800 "vortexldd41" - VxFrame::registerCollisionTypes (restored).
// Mac TU 488 (0x45740e-0x4574d8: the function + the <iostream> initialiser 0x457473),
// Win 0x7111d0. A file of its own in the original (its own static initialiser); LDD's
// VortexBrickPhysics calls it on the frame once (Mac TU 422 0x39e7c0). File name inferred.
#include "Vx/VxFrame.h"
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincHeightField.h"
#include "Vinc/VincTriangleMesh.h"
#include "Vinc/VincTriangleMeshContacts.h"
#include "Vinc/VincConvexInteractions.h"
#include <iostream>

namespace Vx {

// Mac 0x45742f, Win 0x7111d0: the pair tests of the geometry types (the frame registered the
// types themselves)
void VxFrame::registerCollisionTypes(VxFrame*)
{
    VincPrimitivesRegisterInteractions();
    VincCapsuleRegisterTypeAndInteractions();
    VincHeightFieldRegisterInteractions();
    VincTriangleMeshTriangleMeshRegisterInteraction();
    VincConvexMeshTriangleListRegisterInteraction();
    VincConvexMeshRegisterInteractions();
    VincTriangleMeshRegisterInteractions();
    VincTriangleMeshTriangleListRegisterInteraction();
    VincTriangleMeshHeightFieldRegisterInteraction();
    VincTriangleMeshCapsuleRegisterInteraction();
    VincCapsuleConvexRegisterInteraction();
    VincConvexMeshHeightFieldRegisterInteraction();
}

} // namespace Vx
