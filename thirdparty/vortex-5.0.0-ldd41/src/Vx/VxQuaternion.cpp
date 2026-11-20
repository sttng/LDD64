// Vortex 5.0.0 build 800 "vortexldd41" - VxQuaternion out-of-line members (restored).
// Mac TU 512 0x474c9c-0x474fc7 (file name inferred; static initialiser 0x474f62:
// <iostream>), Win 0x7e4c70-0x7e4d60. Used by the fluid module. The Win-only members of
// the TU (0x7e4c90 operator=, 0x7e4cc0 / 0x7e4cf0 copy constructors, 0x7e4d20 (w, x, y, z)
// constructor, 0x7e4d40) are the header-inline ones of VxQuaternion.h.
#include "Vx/VxQuaternion.h"
#include <math.h>
#include "Me/MeCrtMath.h"
#include <iostream>

namespace Vx {

// C2 Mac 0x474f1e -> 0x474eda -> 0x474e60 (C1 0x474f40), Win 0x7e4c70 (-> 0x7e4430): rotation of
// `angle` about `axis` (unit axis assumed)
VxQuaternion::VxQuaternion(const VxVector3& axis, const VxReal& angle)
{
    setAxisAngle(axis[0], axis[1], axis[2], angle);
}

// Mac 0x474e60 (name inferred)
void VxQuaternion::setAxisAngle(VxReal x, VxReal y, VxReal z, VxReal angle)
{
    const VxReal s = sin(angle * 0.5);
    q[0] = cos(angle * 0.5);
    q[1] = s * x;
    q[2] = s * y;
    q[3] = s * z;
}

// Mac 0x474c9c, Win 0x7e4d60: v rotated by this quaternion,
// 2 w (u x v) + (w^2 - u.u) v + 2 (u.v) u
VxVector3 VxQuaternion::rotate(const VxVector3& v) const
{
    const VxReal z = q[3], y = q[2], x = q[1];
    VxVector3 r;
    const VxReal v2 = v[2], v1 = v[1], v0 = v[0];
    r[0] = y * v2 - z * v1;
    r[1] = z * v0 - x * v2;
    r[2] = x * v1 - y * v0;
    const VxReal w2 = q[0] + q[0];
    const VxReal c0 = r[0] * w2, c1 = r[1] * w2, c2 = r[2] * w2;
    const VxReal k = q[0] * q[0] - (x * x + y * y + z * z);
    const VxReal t0 = c0 + k * v[0];
    const VxReal t1 = c1 + k * v[1];
    const VxReal t2 = c2 + k * v[2];
    VxReal d = x * v[0] + y * v[1] + z * v[2];
    d = d + d;
    r[0] = t0 + x * d;
    r[1] = t1 + y * d;
    r[2] = t2 + d * z;
    return r;
}

} // namespace Vx
