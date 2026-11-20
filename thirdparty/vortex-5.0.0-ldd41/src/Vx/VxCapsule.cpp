// Vortex 5.0.0 build 800 "vortexldd41" - VxCapsule (restored).
// Mac TU 466 0x4372ac-0x437a38 (file guess Vx/VxCapsule.cpp); Win 0x711910-0x711be0.
// computeSmallestEnclosing (slots 10/11) is in VxOrientedVolume.cpp (Mac TU 510).
#include "Vx/VxGeometry.h"
#include "Vinc/VincPrimitives.h"

namespace Vx {

// Mac 0x4372ac (300), types Mac 0x4375aa / Win 0x711ad0, isOfType Mac 0x43793c / Win 0x711b50,
// isOfClassType Mac 0x4372c0 / Win 0x711a10
VX_IMPLEMENT_CLASS_TYPES(VxCapsule, VxGeometry, 300)

// operator new Mac 0x437527 / Win 0x711910, delete Mac 0x43746f / Win 0x711920
VX_IMPLEMENT_ALLOCATOR(VxCapsule)

// C2 Mac 0x43737c (C1 0x4373fc), Win 0x711950
VxCapsule::VxCapsule(VxReal radius, VxReal height)
{
    mGeometry = VincCapsuleCreate(radius, height);
}

// D1 Mac 0x437514 (D2 0x4374d4), D0 Mac 0x437490, Win 0x711be0
VxCapsule::~VxCapsule()
{
}

// Mac 0x437982, Win 0x711b80
const char* VxCapsule::getClassName() const
{
    return "VxCapsule";
}

// Mac 0x43753a, Win 0x711a30
VxGeometry* VxCapsule::clone() const
{
    VxReal r = getRadius();
    VxReal h = getHeight();
    return new VxCapsule(r, h);
}

// Mac 0x437994, Win 0x711ba0
void VxCapsule::setDimensions(const VxVector3& d)
{
    setRadius(d[0]);
    setHeight(d[1]);
}

// Mac 0x437344, Win 0x7119d0
VxReal VxCapsule::getRadius() const
{
    return VincCapsuleGetRadius(mGeometry);
}

// Mac 0x43735a
void VxCapsule::setRadius(VxReal r)
{
    VincCapsuleSetRadius(mGeometry, r);
}

// Mac 0x4372f8, Win 0x711a00
VxReal VxCapsule::getHeight() const
{
    VxReal h = VincCapsuleGetHalfHeight(mGeometry);
    return h + h;
}

// Mac 0x437322, Win 0x7119e0
void VxCapsule::setHeight(VxReal h)
{
    VincCapsuleSetHeight(mGeometry, h);
}

} // namespace Vx
