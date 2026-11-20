// Vortex 5.0.0 build 800 "vortexldd41" - VxCylinder (restored).
// Mac TU 475 0x4424aa-0x442dc3 (file guess Vx/VxCylinder.cpp); Win 0x711210-0x711550.
// The TU also holds the out-of-line copies of VxGeometry's inline slots 6/8/9 (Mac
// 0x442ba8-0x442bbc) and VxArray<int> template code (0x442bd8, 0x442c3a).
// computeSmallestEnclosing (slots 10/11) is in VxOrientedVolume.cpp (Mac TU 510).
#include "Vx/VxGeometry.h"
#include "Vinc/VincPrimitives.h"

namespace Vx {

// Mac 0x4424aa (0x12e), types Mac 0x4427d0 / Win 0x711420, isOfType Mac 0x442b62 / Win 0x7114a0,
// isOfClassType Mac 0x4424be / Win 0x711330
VX_IMPLEMENT_CLASS_TYPES(VxCylinder, VxGeometry, 0x12e)

// operator new Mac 0x44274d / Win 0x711210, delete Mac 0x442695 / Win 0x711220
VX_IMPLEMENT_ALLOCATOR(VxCylinder)

// C2 Mac 0x442588 (C1 0x442622), Win 0x711350
VxCylinder::VxCylinder(VxReal radius, VxReal height)
{
    mGeometry = VincCylinderCreate(radius, height);
    setCosTolerance(0.98f); // NOTE: a float constant: 0.98000001907...
}

// D1 Mac 0x44273a (D2 0x4426fa), D0 Mac 0x4426b6, Win 0x711550
VxCylinder::~VxCylinder()
{
}

// Mac 0x442bc6, Win 0x7114f0
const char* VxCylinder::getClassName() const
{
    return "VxCylinder";
}

// Mac 0x442760, Win 0x7113a0
VxGeometry* VxCylinder::clone() const
{
    VxReal r = getRadius();
    VxReal h = getHeight();
    return new VxCylinder(r, h);
}

// Mac 0x442bfa, Win 0x711510
void VxCylinder::setDimensions(const VxVector3& d)
{
    setRadius(d[0]);
    setHeight(d[1]);
}

// Mac 0x4424f6, Win 0x7112d0
void VxCylinder::setCosTolerance(VxReal c)
{
    VincCylinderSetCosTolerance(mGeometry, c);
}

// Mac 0x442518, Win 0x7112c0
VxReal VxCylinder::getHeight() const
{
    return VincCylinderGetHeight(mGeometry);
}

// Mac 0x44252e, Win 0x7112a0
void VxCylinder::setHeight(VxReal h)
{
    VincCylinderSetHeight(mGeometry, h);
}

// Mac 0x442550, Win 0x711290
VxReal VxCylinder::getRadius() const
{
    return VincCylinderGetRadius(mGeometry);
}

// Mac 0x442566
void VxCylinder::setRadius(VxReal r)
{
    VincCylinderSetRadius(mGeometry, r);
}

} // namespace Vx
