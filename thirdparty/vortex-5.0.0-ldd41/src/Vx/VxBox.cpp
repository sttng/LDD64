// Vortex 5.0.0 build 800 "vortexldd41" - VxBox (restored).
// Mac TU 461 0x43271a-0x432e8d (file guess Vx/VxBox.cpp); Win 0x7115a0-0x7118c0.
// computeSmallestEnclosing (slots 10/11) is in VxOrientedVolume.cpp (Mac TU 510).
#include "Vx/VxGeometry.h"
#include "Vinc/VincPrimitives.h"

namespace Vx {

// Mac 0x43271a (299), types Mac 0x432a08 / Win 0x7117c0, isOfType Mac 0x432d9a / Win 0x711840,
// isOfClassType Mac 0x43272e / Win 0x7116f0
VX_IMPLEMENT_CLASS_TYPES(VxBox, VxGeometry, 299)

// operator new Mac 0x43299f / Win 0x7115a0, delete Mac 0x4328e6 / Win 0x7115b0
VX_IMPLEMENT_ALLOCATOR(VxBox)

// C2 Mac 0x432804 (C1 0x432882), Win 0x711620
VxBox::VxBox(const VxVector3& dimensions)
{
    mGeometry = VincBoxCreate(dimensions[0], dimensions[1], dimensions[2]);
}

// Win 0x7115e0 (Win only)
VxBox::VxBox(VxReal dx, VxReal dy, VxReal dz)
{
    mGeometry = VincBoxCreate(dx, dy, dz);
}

// D1 Mac 0x43298c (D2 0x43294c), D0 Mac 0x432908, Win 0x7118c0
VxBox::~VxBox()
{
}

// Mac 0x432de0, Win 0x711870
const char* VxBox::getClassName() const
{
    return "VxBox";
}

// Mac 0x4329b2, Win 0x711710
VxGeometry* VxBox::clone() const
{
    VxVector3 d;
    getDimensions(d);
    return new VxBox(d);
}

// Mac 0x432df2, Win 0x711890
void VxBox::setDimensions(const VxVector3& d)
{
    setDimensions(d[0], d[1], d[2]);
}

// Mac 0x432766, Win 0x7116b0
void VxBox::getDimensions(VxVector3& d) const
{
    MeReal dx = 0, dy = 0, dz = 0;
    VincBoxGetDimensions(mGeometry, &dx, &dy, &dz);
    d.set(dx, dy, dz);
}

// Mac 0x4327cc, Win 0x711680
void VxBox::setDimensions(VxReal dx, VxReal dy, VxReal dz)
{
    VincBoxSetDimensions(mGeometry, dx, dy, dz);
}

} // namespace Vx
