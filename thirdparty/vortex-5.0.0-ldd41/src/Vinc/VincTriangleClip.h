/* Vortex 5.0.0 build 800 "vortexldd41" - internal clip helpers of the 558e contact generators
 * (VincTriangleClip.c). cdecl on Win. All names (name inferred). */
#ifndef VINC_VINCTRIANGLECLIP_H
#define VINC_VINCTRIANGLECLIP_H

#include "Vinc/VincIntersect.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Mac 0x541fcc, Win 0x7e5320 */
int VincSegmentPlaneZ(const MeReal* a, const MeReal* b, MeReal* out);
/* Mac 0x5421e1, Win 0x7e5870 */
int VincClipSegmentBoxRange(MeReal* tLo, MeReal* tHi, const MeReal* p, const MeReal* d, const MeReal* invD,
                            const MeReal* radii);
/* Mac 0x542530, Win 0x7e5bc0 */
int VincClosestPointOnSegment(MeReal* out, const MeReal* p, const MeReal* d, const MeReal* q);
/* Mac 0x543bb5, Win 0x7e5c80 */
int VincClipSegmentCylinderRange(MeReal* tLo, MeReal* tHi, const MeReal* p, const MeReal* d, const MeReal* r,
                                 const MeReal* hh, int* loKind, int* hiKind);
/* Mac 0x543ed8, Win 0x7e5f50 */
int VincClipSegmentCylinder(MeReal* tLo, MeReal* tHi, const MeReal* p, const MeReal* d, const MeReal* r,
                            const MeReal* hh);

/* Mac 0x542085, Win 0x7e5590 (register arguments in both builds; VincBoxTriangle.c): en = the
 * triangle's edge normals (normal x edges[i]) */
struct VincTriangle;
int VincTriangleAddPointInPrism(MeReal** cursor, MeReal** seps, const MeReal* p, const struct VincTriangle* tri,
                                const MeReal (*en)[3]);

#ifdef __cplusplus
}
#endif

#endif
