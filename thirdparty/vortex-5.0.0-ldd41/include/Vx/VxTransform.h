// Vortex 5.0.0 build 800 "vortexldd41" - VxTransform (restored header; out-of-line
// members Mac 0x47f740-0x4800f2, src/Vx/VxTransform.cpp).
// A 4x4 VxReal matrix, row-major, 128 bytes, no vtable (VxEntity::getTransform Mac 0x4478aa copies
// 16 doubles). Rows 0..2 are the rotated axes, row 3 the translation (the glue reads +0x60/+0x68/
// +0x70, Mac TU 435 0x3abc30); column 3 is (0,0,0,1). Points are row vectors (p' = p * M), so
// "a.postMultiply(b)" = a then b.
#ifndef VX_VXTRANSFORM_H
#define VX_VXTRANSFORM_H

#include "Vx/VxPlatform.h"
#include "Vx/VxVector3.h"

namespace Vx {

// Euler angles + order code (Ken Shoemake's "Euler angle conversion" representation, as used
// by Mac 0x44810c: order bits = frame (bit 0), repetition (bit 1), parity (bit 2), first axis
// (bits 3-4) - EulOrd(i,p,r,f)). 28 bytes on Mac, 32 on Win. (name inferred)
struct VxEulerAngles {
    VxReal angle[3]; // +0x00
    int order;       // +0x18
};

// Mac 0x44810c: Shoemake Eul_ToHMatrix: rotation part of out from ea (src/Vx/VxEulerAngles.cpp)
// (name inferred)
void VxEulerAnglesToTransform(const VxEulerAngles& ea, class VxTransform& out);
// Mac 0x447df8, Win 0x752570: Shoemake Eul_FromHMatrix: Euler angles of tm's rotation in the given
// order (src/Vx/VxEulerAngles.cpp, Mac TU 485) (name inferred)
VxEulerAngles VxEulerAnglesFromTransform(const class VxTransform& tm, int order);

class VxTransform {
public:
    // Mac 0x47fa42, Win 0x71a290 (out of line; also inlined): identity
    VxTransform();
    // C2 Mac 0x47f98a / C1 0x47fa1a: rotation from Euler angles (makeRotation), then the
    // translation (name inferred)
    VxTransform(const VxVector3& translation, const VxVector3& eulerAngles, int eulerOrder);
    // Win 0x71a240 (Win only): identity rotation with a translation (name inferred)
    explicit VxTransform(const VxVector3& translation);
    // Mac 0x47f840 (called by VxEntity / VxPart): copies the 16 values - probably the implicit
    // operator= emitted out of line, kept here under a name (name inferred)
    VxTransform& assign(const VxTransform& other);

    VxReal* operator[](int row) { return m[row]; }
    const VxReal* operator[](int row) const { return m[row]; }

    VxVector3 getTranslation() const { return VxVector3(m[3][0], m[3][1], m[3][2]); }
    void setTranslation(const VxVector3& t) { m[3][0] = t[0]; m[3][1] = t[1]; m[3][2] = t[2]; }
    VxVector3 getAxis(int i) const { return VxVector3(m[i][0], m[i][1], m[i][2]); }
    void setAxis(int i, const VxVector3& a) { m[i][0] = a[0]; m[i][1] = a[1]; m[i][2] = a[2]; }

    // Mac 0x47f740, Win 0x719cc0: |translation| and |rotation - I| all <= tolerance (glue: 1e-14) (name inferred)
    bool isIdentity(VxReal tolerance) const;
    // Mac 0x47f914, Win 0x71a1b0: MeMatrix4IsTM(m, 1e-7) (name inferred)
    bool isValid() const;
    // Mac 0x47fda4, Win 0x71a4e0: normalise row `axis`, rebuild the other two rows by cross products
    // (fallback when the cross product is <= 0.01) (name inferred)
    void orthonormalize(int axis);
    // Mac 0x47faae: *this = *this * b (rotation 3x3 + translation), returns *this (name inferred)
    VxTransform& postMultiply(const VxTransform& b);
    // Mac 0x4800f2: *this = b * *this, returns *this (name inferred)
    VxTransform& preMultiply(const VxTransform& b);
    // Mac 0x47f944: rotation from Euler angles (VxEulerAngles{rx, ry, rz, order} -> 0x44810c),
    // returns *this; the glue passes order 0x20 (name inferred)
    VxTransform& makeRotation(VxReal rx, VxReal ry, VxReal rz, int order);

    // as inlined in the VxData connection code (Mac 0x40ad1a / 0x40ea78): compares the
    // first three columns of the four rows only (column 3 is the constant (0,0,0,1))
    bool operator==(const VxTransform& o) const
    {
        for (int r = 0; r < 4; ++r)
            if (!(m[r][0] == o.m[r][0] && m[r][1] == o.m[r][1] && m[r][2] == o.m[r][2]))
                return false;
        return true;
    }
    bool operator!=(const VxTransform& o) const { return !(*this == o); }

    VxReal m[4][4]; // +0x00, row stride 0x20
};

} // namespace Vx

#endif
