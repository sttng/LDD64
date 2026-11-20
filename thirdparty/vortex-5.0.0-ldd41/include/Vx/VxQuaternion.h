// Vortex 5.0.0 build 800 "vortexldd41" - VxQuaternion (restored header).
// Header-inline value type of four VxReal (32 bytes); name from the "VxQuaternion::attempt..."
// warning string. Order w, x, y, z as MeQuaternion (inferred: the glue never touches components).
#ifndef VX_VXQUATERNION_H
#define VX_VXQUATERNION_H

#include "Vx/VxPlatform.h"
#include "Vx/VxVector3.h"

namespace Vx {

class VxQuaternion {
public:
    VxQuaternion() { q[0] = 1; q[1] = q[2] = q[3] = 0; }
    VxQuaternion(VxReal w, VxReal x, VxReal y, VxReal z) { q[0] = w; q[1] = x; q[2] = y; q[3] = z; }
    VxReal& operator[](int i) { return q[i]; }
    const VxReal& operator[](int i) const { return q[i]; }

    // out of line (src/Vx/VxQuaternion.cpp, Mac TU 512; names inferred)
    VxQuaternion(const VxVector3& axis, const VxReal& angle);       // C1 Mac 0x474f40, Win 0x7e4c70
    void setAxisAngle(VxReal x, VxReal y, VxReal z, VxReal angle);  // Mac 0x474e60
    VxVector3 rotate(const VxVector3& v) const;                     // Mac 0x474c9c, Win 0x7e4d60

    VxReal q[4]; // +0x00 w, +0x08 x, +0x10 y, +0x18 z
};

} // namespace Vx

#endif
