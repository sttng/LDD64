// Vortex 5.0.0 build 800 "vortexldd41" - private declarations of the collision objects:
// the functions of other items they call (names inferred): 558c the Vinc regular-grid height
// field, 558d-e the space and the aggregate geometry, Mac TU 486 the default fluid interaction
// forces per shape.
#ifndef VX_VXCOLLISIONINTERNAL_H
#define VX_VXCOLLISIONINTERNAL_H

#include "Vinc/VincModel.h"
#include "Vinc/VincInternal.h"
#include "Vinc/VincSpace.h"
#include "Vx/VxCollisionGeometry.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- 558c: regular-grid height field (type 8, "RGHeightField"): Vinc/VincHeightField.h
 * (VincRGHeightFieldCreate Mac 0x53b363 / Win 0x75c410, VincRGHeightFieldUpdate Mac 0x53b350 /
 * Win 0x75c4a0) ---- */
#ifdef __cplusplus
}
#endif
#include "Vinc/VincHeightField.h"
#ifdef __cplusplus
extern "C" {
#endif

/* ---- 558d-e: space wrappers and the aggregate geometry: Vinc/VincSpace.h, Vinc/VincInternal.h ---- */

#ifdef __cplusplus
}
#endif

namespace Vx {
// ---- Mac TU 486: default fluid interaction forces (Mac pointers 0x7025c0-0x7025dc) ----
bool vxCapsuleFluidInteractionForce(VxCollisionGeometry* cg, void* fluidState);     // Mac 0x44faca
bool vxSphereFluidInteractionForce(VxCollisionGeometry* cg, void* fluidState);      // Mac 0x44fb85
bool vxConvexMeshFluidInteractionForce(VxCollisionGeometry* cg, void* fluidState);  // Mac 0x451aea
bool vxTriangleMeshFluidInteractionForce(VxCollisionGeometry* cg, void* fluidState); // Mac 0x44cadf
bool vxBoxFluidInteractionForce(VxCollisionGeometry* cg, void* fluidState);         // Mac 0x451a33
bool vxCylinderFluidInteractionForce(VxCollisionGeometry* cg, void* fluidState);    // Mac 0x44c5c4
bool vxCompositeFluidInteractionForce(VxCollisionGeometry* cg, void* fluidState);   // Mac 0x448510
// Win only (dead-stripped on Mac; names inferred): the capsule / cylinder forces without the flat
// cap drag (Win 0x749670, called by the Win-only 0x7f0af0; Win 0x7496d0, no caller) and the
// largest cell of the box buoyancy grid (Win 0x745820)
bool vxCapsuleFluidInteractionForceNoCaps(VxCollisionGeometry* cg, void* fluidState);  // Win 0x749670
bool vxCylinderFluidInteractionForceNoCaps(VxCollisionGeometry* cg, void* fluidState); // Win 0x7496d0
void vxSetFluidBoxCellSize(VxReal x, VxReal y, VxReal z);                              // Win 0x745820
} // namespace Vx

#endif
