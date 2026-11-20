/* Vortex 5.0.0 build 800 "vortexldd41" - the cone pair tests that only ask GJK whether the two
 * convex geometries touch: cone / box, cone / cone, cone / cylinder (restored).
 * Mac 0x4e5da3-0x4e5fdd (557f; three files on the Mac: one registration + one test each),
 * Win 0x7e2380 / 0x7e23b0 / 0x7e2410 / 0x7e2440 / 0x7dd3f0 / 0x7dd420. Names (name inferred).
 * No contacts are produced: the result only says touch / no touch. */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincPairTests.h"
#include "Vinc/VincGJK.h"

#include <string.h>

/* Mac 0x4e5e19, Win 0x7e2410 (model1 = cone, model2 = box) */
int MEAPI VincConeBoxIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    result->contactGroupCount = 0;
    result->contactCount = 0;
    return (unsigned char)VincGJKIntersect(pair->model1, pair->model2, 0, 0, 0);
}

/* Mac 0x4e5da3, Win 0x7e2440 */
int MEAPI VincConeBoxRegisterInteraction(void)
{
    VincInteractions i;
    memset(&i, 0, sizeof i);
    i.intersectFn = VincConeBoxIntersect;
    VincGeometryPairRegisterInteractions(VincConeGetTypeId(), VincBoxGetTypeId(), &i, 0);
    return 1;
}

/* Mac 0x4e5ed7, Win 0x7e2380 */
int MEAPI VincConeConeIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    result->contactGroupCount = 0;
    result->contactCount = 0;
    return (unsigned char)VincGJKIntersect(pair->model1, pair->model2, 0, 0, 0);
}

/* Mac 0x4e5e61, Win 0x7e23b0 */
int MEAPI VincConeConeRegisterInteraction(void)
{
    VincInteractions i;
    memset(&i, 0, sizeof i);
    i.intersectFn = VincConeConeIntersect;
    VincGeometryPairRegisterInteractions(VincConeGetTypeId(), VincConeGetTypeId(), &i, 0);
    return 1;
}

/* Mac 0x4e5f95, Win 0x7dd3f0 (model1 = cone, model2 = cylinder) */
int MEAPI VincConeCylinderIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    result->contactGroupCount = 0;
    result->contactCount = 0;
    return (unsigned char)VincGJKIntersect(pair->model1, pair->model2, 0, 0, 0);
}

/* Mac 0x4e5f1f, Win 0x7dd420 */
int MEAPI VincConeCylinderRegisterInteraction(void)
{
    VincInteractions i;
    memset(&i, 0, sizeof i);
    i.intersectFn = VincConeCylinderIntersect;
    VincGeometryPairRegisterInteractions(VincConeGetTypeId(), VincCylinderGetTypeId(), &i, 0);
    return 1;
}
