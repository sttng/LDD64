/* Vortex 5.0.0 build 800 "vortexldd41" - line segment against a capsule (restored).
 * Mac 0x4e3b23-0x4e3e62 (557e), Win 0x7db6b0-0x7db900. Names (name inferred). */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincPairTests.h"
#include "Vinc/VincPairTestsB.h"
#include "Vinc/VincMath.h"

#include <string.h>

/* Mac 0x4e3b23, Win 0x7db8e0 */
void MEAPI VincCapsuleRegisterLineSegment(void)
{
    VincGeometryRegisterLineSegmentFn(VincCapsuleGetTypeId(), VincCapsuleLineSegment);
}

/* Mac 0x4e3b4e, Win 0x7db730: the segment against the top end sphere, then the bottom one (a hit
 * counts only on the sphere's outer half: at least hh from the capsule centre), then the shaft
 * cylinder (V3.6's VincLineSegmentSphere / VincLineSegmentCylinder). A capsule with hh < 1e-14 is
 * never hit (NOTE: both builds, even though it is a sphere). */
int MEAPI VincCapsuleLineSegment(VincModelID m, const MeVector3 p0, const MeVector3 p1, VincLineSegResult* r)
{
    VincGeometryID g = VincModelGetGeometry(m);
    MeReal* tm = VincModelGetTransformPtr(m);
    MeReal radius = VincCapsuleGetRadius(g);
    MeReal hh = VincCapsuleGetHalfHeight(g);
    MeReal end[16], h2;
    if (hh < 1e-14)
        return 0;
    memcpy(end, tm, sizeof end); /* Win 0x7db6b0: a matrix copy */
    end[12] = tm[8] * hh + tm[12];
    end[13] = tm[9] * hh + tm[13];
    end[14] = tm[10] * hh + tm[14];
    if (VincLineSegmentSphere(end, radius, p0, p1, r)) {
        MeReal dx = r->position[0] - tm[12], dy = r->position[1] - tm[13], dz = r->position[2] - tm[14];
        if ((dx * dx + dy * dy) + dz * dz >= hh * hh)
            return 1;
    }
    h2 = hh + hh;
    end[12] = end[12] - tm[8] * h2;
    end[13] = end[13] - tm[9] * h2;
    end[14] = end[14] - tm[10] * h2;
    if (VincLineSegmentSphere(end, radius, p0, p1, r)) {
        MeReal dx = r->position[0] - tm[12], dy = r->position[1] - tm[13], dz = r->position[2] - tm[14];
        if ((dx * dx + dy * dy) + dz * dz >= hh * hh)
            return 1;
    }
    return VincLineSegmentCylinder(tm, radius, hh, p0, p1, r);
}
