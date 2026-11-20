// Vortex 5.0.0 build 800 "vortexldd41" - VxVector3 (restored header).
// Header-inline value type of three VxReal (24 bytes); neither binary has out-of-line VxVector3
// code. The glue builds them from LEGO floats and passes them by const reference (e.g.
// VxConstraint::setPartAndAttachment, Mac 0x43db02).
#ifndef VX_VXVECTOR3_H
#define VX_VXVECTOR3_H

#include "Vx/VxPlatform.h"
#include <math.h>

namespace Vx {

class VxVector3 {
public:
    VxVector3() { v[0] = v[1] = v[2] = 0; }
    VxVector3(VxReal x, VxReal y, VxReal z) { v[0] = x; v[1] = y; v[2] = z; }
    explicit VxVector3(const VxReal3 r) { v[0] = r[0]; v[1] = r[1]; v[2] = r[2]; }

    VxReal& operator[](int i) { return v[i]; }
    const VxReal& operator[](int i) const { return v[i]; }
    VxReal x() const { return v[0]; }
    VxReal y() const { return v[1]; }
    VxReal z() const { return v[2]; }
    void set(VxReal x, VxReal y, VxReal z) { v[0] = x; v[1] = y; v[2] = z; }
    void setZero() { v[0] = v[1] = v[2] = 0; }
    VxReal* data() { return v; }
    const VxReal* data() const { return v; }

    VxVector3 operator+(const VxVector3& o) const { return VxVector3(v[0] + o.v[0], v[1] + o.v[1], v[2] + o.v[2]); }
    VxVector3 operator-(const VxVector3& o) const { return VxVector3(v[0] - o.v[0], v[1] - o.v[1], v[2] - o.v[2]); }
    VxVector3 operator-() const { return VxVector3(-v[0], -v[1], -v[2]); }
    VxVector3 operator*(VxReal s) const { return VxVector3(v[0] * s, v[1] * s, v[2] * s); }
    VxVector3& operator+=(const VxVector3& o) { v[0] += o.v[0]; v[1] += o.v[1]; v[2] += o.v[2]; return *this; }
    VxVector3& operator-=(const VxVector3& o) { v[0] -= o.v[0]; v[1] -= o.v[1]; v[2] -= o.v[2]; return *this; }
    VxVector3& operator*=(VxReal s) { v[0] *= s; v[1] *= s; v[2] *= s; return *this; }

    VxReal dot(const VxVector3& o) const { return v[0] * o.v[0] + v[1] * o.v[1] + v[2] * o.v[2]; }
    VxVector3 cross(const VxVector3& o) const
    {
        return VxVector3(v[1] * o.v[2] - v[2] * o.v[1], v[2] * o.v[0] - v[0] * o.v[2], v[0] * o.v[1] - v[1] * o.v[0]);
    }
    VxReal norm2() const { return dot(*this); }
    VxReal norm() const { return sqrt(norm2()); }
    // Same shape as the inlined row normalisation in VxTransform::orthonormalize (Mac 0x47fda4):
    // scale by 1/sqrt(n2) only when n2 > 0.
    VxReal normalize()
    {
        VxReal n2 = norm2();
        if (n2 > 0) {
            VxReal s = 1.0 / sqrt(n2);
            v[0] *= s;
            v[1] *= s;
            v[2] *= s;
        }
        return n2;
    }

    // exact compare, as inlined in the VxData connection code (Mac 0x40abd2 / 0x40eca0):
    // a NaN component makes the vectors unequal
    bool operator==(const VxVector3& o) const { return v[0] == o.v[0] && v[1] == o.v[1] && v[2] == o.v[2]; }
    bool operator!=(const VxVector3& o) const { return !(*this == o); }

    VxReal v[3]; // +0x00 x, +0x08 y, +0x10 z
};

} // namespace Vx

#endif
