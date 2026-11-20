/* Vortex 5.0.0 build 800 "vortexldd41" - contact simplification (restored;
 * src/Vinc/VincContactSimplify.c, Mac sub-TU 559a 0x54de3f-0x5502d0, Win 0x755390-0x7572a0).
 * The one declaration of these functions (VincInternal.h, VincPairSupport.h and
 * VincTriangleMeshContacts.h include it).
 *
 * A pair test collects its raw contacts and hands them to VincContactSimplify, which
 *   1. merges contacts closer than the tolerance whose normals agree (VincContactSimplifySetTolerances),
 *   2. groups them by normal direction (up to min(6, request->contactGroupMaxCount) groups, cosine
 *      threshold request->m1c) and computes each group's normal (request->m28: 1 = sum of the
 *      normals, 3 = depth-weighted sum, 2 = "Not implemented"),
 *   3. reduces each group to at most min(request->contactMaxCount, result->contactMaxCount)
 *      contacts (VincContactSimplifyCore: the deepest contact plus the convex hull of the contact
 *      points around the normal, the hull pruned by area and collinearity),
 *   4. records each group's contact count, normal and a size measure (VincContactGroup.m04).
 * Names: "VincContactSimplify" and "VincContactSimplifyResult" are strings of the binaries; the
 * other names are inferred. Win: the core runs the "mcd" FLEXlm license gate;
 * restored as the licensed behaviour (the gate is VxLicense). */
#ifndef VINC_VINCCONTACTSIMPLIFY_H
#define VINC_VINCCONTACTSIMPLIFY_H

#include "Vinc/VincIntersect.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Mac 0x54de3f, Win 0x755390: distance tolerance of the duplicate merge (squared internally, also
 * the area tolerance of the hull) and the cosine of the largest normal angle of merged contacts.
 * Defaults (MeReal)1e-2f^2 = 9.99999975e-5 / (MeReal)0.99f. (name inferred) */
void MEAPI VincContactSimplifySetTolerances(MeReal distance, MeReal cosAngle);
/* Mac 0x54dfd2, Win 0x7553c0 (name inferred) */
void MEAPI VincContactSimplifyGetTolerances(MeReal* distance, MeReal* cosAngle);

/* Mac 0x779d48, Win 0xabc84c: 1 (no setter survived): the core keeps the deepest contact; 0: it
 * never does (the search then starts from -DBL_MAX) (name inferred) */
extern int VincContactSimplifyKeepDeepest;

/* Mac 0x54e8e3, Win 0x755a70 (the "mcd"-licensed core): reduces `count` contacts (modified in
 * place: duplicates are merged) to at most maxCount contacts in out[], returns their number.
 * normal: the reference normal of the hull; area: if not NULL, receives the hull area (1.0 when the
 * request allows more than 8 contacts). (name inferred) */
int MEAPI VincContactSimplifyCore(MeReal* normal, VincContact* contacts, int count, VincContact* out, int maxCount,
                                  VincRequest* request, MeReal* area);

/* Mac 0x54f6a8, Win 0x756dc0: reduces `count` raw contacts into result->contacts and fills the
 * result's contact groups; returns the contact count (the caller stores it). */
int MEAPI VincContactSimplify(MeReal* normal, VincContact* contacts, int count, VincIntersectResult* result);

/* Mac 0x550188, Win 0x757140: VincContactSimplify, then result->contactCount = the count and
 * result->normal = normal (pairName: the calling pair test, unused). (name inferred) */
void MEAPI VincContactSimplifyNamed(MeReal* normal, VincContact* contacts, int count, VincIntersectResult* result,
                                    const char* pairName);

/* Mac 0x5501e5, Win 0x757180: when result->contactCount exceeds the request's contactMaxCount,
 * simplify a copy of the result's contacts back into the result ("VincContactSimplifyResult"). */
void MEAPI VincContactSimplifyResult(VincIntersectResult* result);

#ifdef __cplusplus
}
#endif

#endif
