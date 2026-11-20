// Vortex 5.0.0 build 800 "vortexldd41" - VxSphere (restored).
// Mac TU 516 0x47862e-0x478cef (+ getStaticClassType 0x478604); Win 0x714210-0x714470.
// computeSmallestEnclosing (slots 10/11) is in VxOrientedVolume.cpp (Mac TU 510).
#include "Vx/VxGeometry.h"
#include "Vinc/VincPrimitives.h"

namespace Vx {

// Mac 0x478604 (0x131), types Mac 0x478880 / Win 0x714380, isOfType Mac 0x478c12 / Win 0x714400,
// isOfClassType Mac 0x478618 / Win 0x7142d0
VX_IMPLEMENT_CLASS_TYPES(VxSphere, VxGeometry, 0x131)

// operator new Mac 0x478813 / Win 0x714210, delete Mac 0x47875a / Win 0x714220
VX_IMPLEMENT_ALLOCATOR(VxSphere)

// C2 Mac 0x478688 (C1 0x4786f2), Win 0x714250
VxSphere::VxSphere(VxReal radius)
{
    mGeometry = VincSphereCreate(radius);
}

// D1 Mac 0x478800 (D2 0x4787c0), D0 Mac 0x47877c, Win 0x714470
VxSphere::~VxSphere()
{
}

// Mac 0x478c58, Win 0x714430
const char* VxSphere::getClassName() const
{
    return "VxSphere";
}

// Mac 0x478826, Win 0x7142f0
VxGeometry* VxSphere::clone() const
{
    return new VxSphere(getRadius());
}

// Mac 0x478c6a, Win 0x714450
void VxSphere::setDimensions(const VxVector3& d)
{
    setRadius(d[0]);
}

// Mac 0x478650
VxReal VxSphere::getRadius() const
{
    return VincSphereGetRadius(mGeometry);
}

// Mac 0x478666, Win 0x7742c0
void VxSphere::setRadius(VxReal r)
{
    VincSphereSetRadius(mGeometry, r);
}

} // namespace Vx
