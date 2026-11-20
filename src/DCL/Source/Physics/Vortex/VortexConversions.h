// LEGO Digital Designer — restored source
// The LEGO <-> Vortex value conversions the Vortex glue inlines everywhere (float -> VxReal).
// (file and function names inferred. Win emits ToVxTransform out of line once,
// 0x521310 in VortexPhysicsScene.cpp's range; the Mac inlines both.)
#pragma once
#include "LEGO/Types.h"
#include "Geometry.h"
#include "Vx/VxVector3.h"
#include "Vx/VxTransform.h"

namespace LEGO {

inline Vx::VxVector3 ToVxVector3(const Vector3& v)
{
    return Vx::VxVector3((Vx::VxReal)v.x, (Vx::VxReal)v.y, (Vx::VxReal)v.z);
}

// The column-major Matrix3D (translation m[12..14]) as a row-major VxTransform (rows = axes,
// row 3 = translation), the rotation re-orthonormalised about the third axis (z).
// Win 0x521310 (EAX = matrix, ECX = transform), Mac inlined (e.g. 0x3a8af0, 0x3a9170).
inline void ToVxTransform(const Matrix3D& matrix, Vx::VxTransform& transform)
{
    for (int row = 0; row < 4; ++row) {
        transform.m[row][0] = (Vx::VxReal)matrix.m[row * 4 + 0];
        transform.m[row][1] = (Vx::VxReal)matrix.m[row * 4 + 1];
        transform.m[row][2] = (Vx::VxReal)matrix.m[row * 4 + 2];
        transform.m[row][3] = row == 3 ? 1.0 : 0.0;
    }
    transform.orthonormalize(2);
}

} // namespace LEGO
