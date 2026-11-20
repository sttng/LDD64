// Vortex 5.0.0 build 800 "vortexldd41" - VxDistanceJoint (restored header: the
// Mac remnant TU 481). A constraint on an MdtType14 (limit + one value at +0xbe0). Mac keeps the
// class id and the value accessors (TU 481, 0x445f56-0x446013, + the MdtConstraintMaxSize copy);
// the rest is Win-only (bodies 0x823d30-0x8240e0 belong with not restored here).
#ifndef VX_VXDISTANCEJOINT_H
#define VX_VXDISTANCEJOINT_H

#include "Vx/VxConstraint.h"

namespace Vx {

class VxDistanceJoint : public VxConstraint {
public:
    // Mac 0x445f56, Win 0x823d20: 0x11d
    static int getStaticClassType();
    // Mac 0x445f98, Win 0x823db0: MdtType14SetValue(mConstraint, d) (name inferred)
    void setDistance(VxReal d);
    // Mac 0x445f82, Win 0x823dd0: MdtType14GetValue(mConstraint) (name inferred)
    VxReal getDistance() const;

    // Win vtable (Win-only; Mac slot numbers): 0 0x823e80, 1 0x823de0, D 0x8240e0 / 0x8240d0,
    // 4 0x71fec0, 5 0x7221a0, 6 0x71ffb0, 7 0x71ff80, 8 0x71ffe0, 9 0x720100, 10 0x720080,
    // 11 0x720150, 12 0x721a70, 13 0x721ba0, 14 0x7217d0, 15 0x823d90, 16 0x716340
    virtual ~VxDistanceJoint();

private:
    VxDistanceJoint();
};

} // namespace Vx

#endif
