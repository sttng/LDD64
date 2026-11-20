// Vortex 5.0.0 build 800 "vortexldd41" - VxTransform out-of-line members
// (restored). Mac TU 524 0x47f740-0x4803e8; Win 0x719cc0-0x71a4e0.
#include "Vx/VxTransform.h"
#include "Me/MeMath.h"
#include <math.h>

namespace Vx {

// Mac 0x47f740, Win 0x719cc0.
// NOTE: compares "tolerance < |x|", so NaN entries pass (original).
bool VxTransform::isIdentity(VxReal tolerance) const
{
    if (tolerance < fabs(m[3][0]) || tolerance < fabs(m[3][1]) || tolerance < fabs(m[3][2])
        || tolerance < fabs(m[0][0] - 1.0) || tolerance < fabs(m[1][1] - 1.0)
        || tolerance < fabs(m[2][2] - 1.0) || tolerance < fabs(m[0][1])
        || tolerance < fabs(m[0][2]) || tolerance < fabs(m[1][0]) || tolerance < fabs(m[2][0])
        || tolerance < fabs(m[2][1]))
        return false;
    return fabs(m[1][2]) <= tolerance;
}

// Mac 0x47f840
VxTransform& VxTransform::assign(const VxTransform& other)
{
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            m[i][j] = other.m[i][j];
    return *this;
}

// Mac 0x47f914, Win 0x71a1b0
bool VxTransform::isValid() const
{
    return MeMatrix4IsTM((const MeReal(*)[4])m, 1e-7) != 0;
}

// Mac 0x47f944
VxTransform& VxTransform::makeRotation(VxReal rx, VxReal ry, VxReal rz, int order)
{
    VxEulerAngles ea;
    ea.angle[0] = rx;
    ea.angle[1] = ry;
    ea.angle[2] = rz;
    ea.order = order;
    VxEulerAnglesToTransform(ea, *this);
    return *this;
}

// C2 Mac 0x47f98a, C1 Mac 0x47fa1a
VxTransform::VxTransform(const VxVector3& translation, const VxVector3& eulerAngles, int eulerOrder)
{
    m[2][3] = 0.0;
    m[1][3] = 0.0;
    m[0][3] = 0.0;
    m[3][3] = 1.0;
    makeRotation(eulerAngles[0], eulerAngles[1], eulerAngles[2], eulerOrder);
    m[3][0] = translation[0];
    m[3][1] = translation[1];
    m[3][2] = translation[2];
}

// Win 0x71a240
VxTransform::VxTransform(const VxVector3& translation)
{
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 4; ++j)
            m[i][j] = (i == j) ? 1.0 : 0.0;
    m[3][3] = 1.0;
    m[3][0] = translation[0];
    m[3][1] = translation[1];
    m[3][2] = translation[2];
}

// Mac 0x47fa42, Win 0x71a290
VxTransform::VxTransform()
{
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            m[i][j] = (i == j) ? 1.0 : 0.0;
}

// Mac 0x47faae
VxTransform& VxTransform::postMultiply(const VxTransform& b)
{
    VxTransform r;
    for (int i = 0; i < 3; ++i) {
        r.m[i][0] = m[i][0] * b.m[0][0] + m[i][1] * b.m[1][0] + m[i][2] * b.m[2][0];
        r.m[i][1] = m[i][0] * b.m[0][1] + m[i][1] * b.m[1][1] + m[i][2] * b.m[2][1];
        r.m[i][2] = m[i][0] * b.m[0][2] + m[i][1] * b.m[1][2] + m[i][2] * b.m[2][2];
        r.m[i][3] = 0.0;
    }
    r.m[3][0] = m[3][0] * b.m[0][0] + m[3][1] * b.m[1][0] + m[3][2] * b.m[2][0] + b.m[3][0];
    r.m[3][1] = m[3][0] * b.m[0][1] + m[3][1] * b.m[1][1] + m[3][2] * b.m[2][1] + b.m[3][1];
    r.m[3][2] = m[3][0] * b.m[0][2] + m[3][1] * b.m[1][2] + m[3][2] * b.m[2][2] + b.m[3][2];
    r.m[3][3] = 1.0;
    *this = r;
    return *this;
}

// Mac 0x47fda4, Win 0x71a4e0
void VxTransform::orthonormalize(int axis)
{
    VxReal* a = m[axis];
    VxReal* b = m[(axis + 1) % 3];
    VxReal* c = m[(axis + 2) % 3];
    VxReal len2 = a[0] * a[0] + a[1] * a[1] + a[2] * a[2];
    if (0.0 < len2) {
        VxReal inv = 1.0 / sqrt(len2);
        a[0] = a[0] * inv;
        a[1] = a[1] * inv;
        a[2] = inv * a[2];
    }
    VxReal oldC[3] = { c[0], c[1], c[2] };
    c[0] = a[1] * b[2] - a[2] * b[1];
    c[1] = a[2] * b[0] - a[0] * b[2];
    c[2] = a[0] * b[1] - a[1] * b[0];
    VxReal len = sqrt(c[0] * c[0] + c[1] * c[1] + c[2] * c[2]);
    if (len <= (VxReal)0.01f) {
        // a and b (nearly) parallel: use the old third axis instead of b
        b[0] = oldC[0];
        b[1] = oldC[1];
        b[2] = oldC[2];
        c[0] = a[1] * b[2] - a[2] * b[1];
        c[1] = a[2] * b[0] - a[0] * b[2];
        c[2] = a[0] * b[1] - a[1] * b[0];
        len2 = c[0] * c[0] + c[1] * c[1] + c[2] * c[2];
        if (0.0 < len2) {
            VxReal inv = 1.0 / sqrt(len2);
            c[0] = c[0] * inv;
            c[1] = c[1] * inv;
            c[2] = inv * c[2];
        }
    } else {
        VxReal inv = 1.0 / len;
        c[0] = c[0] * inv;
        c[1] = c[1] * inv;
        c[2] = inv * c[2];
    }
    b[0] = c[1] * a[2] - c[2] * a[1];
    b[1] = c[2] * a[0] - c[0] * a[2];
    b[2] = c[0] * a[1] - c[1] * a[0];
    len2 = b[0] * b[0] + b[1] * b[1] + b[2] * b[2];
    if (0.0 < len2) {
        VxReal inv = 1.0 / sqrt(len2);
        b[0] = b[0] * inv;
        b[1] = b[1] * inv;
        b[2] = inv * b[2];
    }
}

// Mac 0x4800f2
VxTransform& VxTransform::preMultiply(const VxTransform& b)
{
    VxTransform r;
    for (int i = 0; i < 3; ++i) {
        r.m[i][0] = b.m[i][0] * m[0][0] + b.m[i][1] * m[1][0] + b.m[i][2] * m[2][0];
        r.m[i][1] = b.m[i][0] * m[0][1] + b.m[i][1] * m[1][1] + b.m[i][2] * m[2][1];
        r.m[i][2] = b.m[i][0] * m[0][2] + b.m[i][1] * m[1][2] + b.m[i][2] * m[2][2];
        r.m[i][3] = 0.0;
    }
    r.m[3][0] = b.m[3][0] * m[0][0] + b.m[3][1] * m[1][0] + b.m[3][2] * m[2][0] + m[3][0];
    r.m[3][1] = b.m[3][0] * m[0][1] + b.m[3][1] * m[1][1] + b.m[3][2] * m[2][1] + m[3][1];
    r.m[3][2] = b.m[3][0] * m[0][2] + b.m[3][1] * m[1][2] + b.m[3][2] * m[2][2] + m[3][2];
    r.m[3][3] = 1.0;
    *this = r;
    return *this;
}

} // namespace Vx
