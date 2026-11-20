// Vortex 5.0.0 build 800 "vortexldd41" - VxSpring (restored).
// Mac TU 517 0x478cf0-0x4795b6 (static initialiser 0x479546: <iostream>, MdtConstraintMaxSize),
// Win 0x71f870-0x71fd50. One coordinate ("Distance").
#include "Vx/VxSpring.h"
#include "Vx/ConstraintAccessor.h"
#include "Vx/VxFrame.h"
#include "Mdt/MdtConstraint.h"
#include <iostream>
#include <math.h>

namespace Vx {

// getStaticClassType Mac 0x478cf0 / Win 0x71f870, isOfClassType Mac 0x478cfa / Win 0x71fa30,
// getStaticClassTypes Mac 0x478ec4 (atexit 0x478d8d) / Win 0x71fa50, isOfType Mac 0x47924c /
// Win 0x71fad0
VX_IMPLEMENT_CLASS_TYPES(VxSpring, VxConstraint, 0x126)
// new Mac 0x478eb1 / Win 0x71f840, delete Mac 0x478dd7 / Win 0x71f880
VX_IMPLEMENT_ALLOCATOR(VxSpring)

// C2 Mac 0x4794b4 (C1 0x479520), Win 0x71fbd0
VxSpring::VxSpring()
    : VxConstraint(MdtSpringCreate((VinPool*)VxFrame::instance()->mConstraintPool), 2)
{
    addAccessors();
}

// Win 0x71fc40
VxSpring::VxSpring(VxPart* part0, VxPart* part1, VxReal length, VxReal stiffness, VxReal damping)
    : VxConstraint(MdtSpringCreate((VinPool*)VxFrame::instance()->mConstraintPool), 2)
{
    addAccessors();
    setPart(0, part0);
    setPart(1, part1);
    MdtSpringSetNaturalLength(mConstraint, length);
    MdtSpringSetStiffness(mConstraint, stiffness);
    MdtSpringSetDamping(mConstraint, damping);
    setPartAttachmentPositionRel(0, 0, 0, 0);
    setPartAttachmentPositionRel(1, 0, 0, 0);
}

// D2 Mac 0x478e4a (D1 0x478e9e, D0 0x478dee), Win 0x71fd50
VxSpring::~VxSpring()
{
    setEnabled(false);
}

// Mac 0x479534, Win 0x71fd40
const char* VxSpring::getClassName() const
{
    return "VxSpring";
}

// Mac 0x479292, Win 0x71fb00
void VxSpring::addAccessors()
{
    addCoordinate(0, "Distance");
}

// Mac 0x478d6a, Win 0x71f8e0
void VxSpring::resetParts()
{
    VxConstraint::resetParts();
    MdtSpringReset(mConstraint);
}

// Mac 0x478d48, Win 0x71f900
void VxSpring::setNaturalLength(VxReal length)
{
    MdtSpringSetNaturalLength(mConstraint, length);
}

// Mac 0x478d32, Win 0x71fa00
VxReal VxSpring::getNaturalLength() const
{
    return MdtSpringGetNaturalLength(mConstraint);
}

// Win 0x71f920
void VxSpring::setNaturalLengthFromAttachments()
{
    VxVector3 p0, p1;
    getPartAttachmentPosition(0, p0);
    getPartAttachmentPosition(1, p1);
    const VxReal dx = p0[0] - p1[0];
    const VxReal dy = p0[1] - p1[1];
    const VxReal dz = p0[2] - p1[2];
    MdtSpringSetNaturalLength(mConstraint, sqrt(dx * dx + dy * dy + dz * dz));
}

// Win 0x71f9b0
void VxSpring::setStiffness(VxReal k)
{
    MdtSpringSetStiffness(mConstraint, k);
}

// Win 0x71f9d0
void VxSpring::setDamping(VxReal d)
{
    MdtSpringSetDamping(mConstraint, d);
}

} // namespace Vx
