// Vortex 5.0.0 build 800 "vortexldd41" - VxRPRO (restored).
// Mac TU 513 0x474fc8-0x476a32 (static initialiser 0x4769c2: <iostream>, MdtConstraintMaxSize),
// Win 0x714ff0-0x715df0. A "relative position, relative orientation" joint: six equations, the
// attachment frames stored as quaternions in the MdtRPROJoint. The Win-only accessors that
// forward to Win-only MdtRPROJoint functions are not restored (list in VxRPRO.h).
#include "Vx/VxRPRO.h"
#include "Vx/ConstraintAccessor.h"
#include "Vx/VxPart.h"
#include "Vx/VxFrame.h"
#include "Mdt/MdtConstraint.h"
#include "Me/MeMath.h"
#include <iostream>

namespace Vx {

// getStaticClassType Mac 0x474fc8 / Win 0x7150e0, isOfClassType Mac 0x474fd2 / Win 0x7154f0,
// getStaticClassTypes Mac 0x475702 (atexit 0x475365) / Win 0x715730, isOfType Mac 0x475a8a /
// Win 0x7157b0
VX_IMPLEMENT_CLASS_TYPES(VxRPRO, VxConstraint, 0x12a)
// new Mac 0x475489 / Win 0x7150b0, delete Mac 0x4753af / Win 0x7150f0 (+0x7150c0)
VX_IMPLEMENT_ALLOCATOR(VxRPRO)

// C2 Mac 0x476930 (C1 0x47699c), Win 0x715c20 (C2 Win 0x774fd0)
VxRPRO::VxRPRO()
    : VxConstraint(MdtRPROJointCreate((VinPool*)VxFrame::instance()->mConstraintPool), 2)
{
    addAccessors();
}

// C2 Mac 0x4766ec (C1 0x476908), Win 0x715c90
VxRPRO::VxRPRO(VxPart* part0, VxPart* part1, const VxVector3& position)
    : VxConstraint(MdtRPROJointCreate((VinPool*)VxFrame::instance()->mConstraintPool), 2)
{
    addAccessors();
    const VxVector3 zero0(0, 0, 0), x0(1, 0, 0), y0(0, 1, 0);
    setPartAndAttachmentRel(0, part0, zero0, x0, y0);
    const VxVector3 zero1(0, 0, 0), x1(1, 0, 0), y1(0, 1, 0);
    setPartAndAttachmentRel(1, part1, zero1, x1, y1);
    MdtConstraintSetPosition(mConstraint, position[0], position[1], position[2]);
    updateRelativeOrientation();
}

// D2 Mac 0x475422 (D1 0x475476, D0 0x4753c6), Win 0x715df0
VxRPRO::~VxRPRO()
{
    setEnabled(false);
}

// Mac 0x4769b0, Win 0x715de0
const char* VxRPRO::getClassName() const
{
    return "VxRPRO";
}

// Mac 0x475ad0, Win 0x7157e0: six equations; the frictions' axis = 1
void VxRPRO::addAccessors()
{
    addEquation(0, kPrimaryAxisPositionName);
    addEquation(1, kSecondaryAxisPositionName);
    addEquation(2, kThirdAxisPositionName);
    addEquation(3, kPrimaryAxisOrientationName);
    addEquation(4, kSecondaryAxisOrientationName);
    addEquation(5, kThirdAxisOrientationName);
    // every friction takes its direction from part 1's frame (field written directly)
    for (int i = 0; i < 6; ++i)
        mConstraint->bcl.friction[i].mAxis = 1;
}

// Mac 0x475342, Win 0x715150
void VxRPRO::resetParts()
{
    VxConstraint::resetParts();
    MdtRPROJointReset(mConstraint);
}

// Mac 0x474fe8, Win 0x715460: equation i = friction i
bool VxRPRO::getEquationFrictionIndex(int equation, int* friction) const
{
    switch (equation) {
    case 0: *friction = 0; return true;
    case 1: *friction = 1; return true;
    case 2: *friction = 2; return true;
    case 3: *friction = 3; return true;
    case 4: *friction = 4; return true;
    case 5: *friction = 5; return true;
    }
    return false;
}

// Mac 0x4750aa, Win 0x715350
void VxRPRO::setAngularStrength(const VxVector3& v)
{
    wakeBodies();
    MdtRPROJointSetAngularStrength(mConstraint, v[0], v[1], v[2]);
}

// Mac 0x4750f8, Win 0x715310
void VxRPRO::setLinearStrength(const VxVector3& v)
{
    wakeBodies();
    MdtRPROJointSetLinearStrength(mConstraint, v[0], v[1], v[2]);
}

// Mac 0x475146 (Win: inlined)
void VxRPRO::updateRelativeOrientation()
{
    wakeBodies();
    MdtRPROJointUpdateRelativeQuaternion(mConstraint);
}

// Mac 0x47516a, Win 0x7151b0 (the rows by the TU-local helper Win 0x714ff0): the quaternion's rotation rows 0 / 1 become the attachment axes
void VxRPRO::setPartAttachmentOrientationRel(int index, const VxQuaternion& q)
{
    MdtRPROJointSetQuaternion(mConstraint, q[0], q[1], q[2], q[3], index);
    const VxReal w = q[0], x = q[1], y = q[2], z = q[3];
    const VxReal yy = y * y;
    const VxReal zz = z * z;
    const VxReal w2 = w + w;
    const VxReal xy2 = y * (x + x);
    const VxReal xz2 = (x + x) * z;
    const VxReal yz2 = z * (y + y);
    VxVector3 primary, secondary;
    primary[0] = ((x * x + w * w) - yy) - zz;
    secondary[0] = xy2 - w2 * z;
    primary[1] = xy2 + w2 * z;
    const VxReal ww_xx = w * w - x * x;
    secondary[1] = (yy + ww_xx) - zz;
    primary[2] = xz2 - y * w2;
    secondary[2] = x * w2 + yz2;
    VxConstraint::setPartAttachmentAxesRel(index, primary, secondary);
}

// Mac 0x47530a, Win 0x715510
void VxRPRO::setPartAttachmentOrientationRel(int index, const VxTransform& tm)
{
    VxQuaternion q;
    MeQuaternionFromTM(q.q, tm.m);
    setPartAttachmentOrientationRel(index, q);
}

// Mac 0x47561a, Win 0x7155a0: the two axes (and their cross product) as a rotation
void VxRPRO::setPartAttachmentAxesRel(int index, const VxVector3& primary, const VxVector3& secondary)
{
    VxTransform tm;
    tm.setAxis(0, primary);
    tm.setAxis(1, secondary);
    tm.m[2][0] = primary[1] * secondary[2] - primary[2] * secondary[1];
    tm.m[2][1] = primary[2] * secondary[0] - primary[0] * secondary[2];
    tm.m[2][2] = primary[0] * secondary[1] - primary[1] * secondary[0];
    setPartAttachmentOrientationRel(index, tm);
}

// Mac 0x47549c, Win 0x715640: world axes into the part's frame (inverse rotation), then slot 13
// NOTE: like the original, the secondary axis is dereferenced without a null test
void VxRPRO::setPartAttachmentAxes(int index, const VxVector3& primary, const VxVector3* secondary)
{
    VxVector3 p = primary;
    VxVector3 s = *secondary;
    VxPart* part = mParts.begin()[index];
    if (part != 0) {
        const VxTransform& m = *part->mTransformPtr;
        const VxReal a0 = primary[0], a1 = primary[1], a2 = primary[2];
        const VxReal b0 = (*secondary)[0], b1 = (*secondary)[1], b2 = (*secondary)[2];
        p[0] = a0 * m.m[0][0] + a1 * m.m[0][1] + a2 * m.m[0][2];
        p[1] = a0 * m.m[1][0] + a1 * m.m[1][1] + a2 * m.m[1][2];
        p[2] = a0 * m.m[2][0] + a1 * m.m[2][1] + a2 * m.m[2][2];
        s[0] = b0 * m.m[0][0] + b1 * m.m[0][1] + b2 * m.m[0][2];
        s[1] = b0 * m.m[1][0] + b1 * m.m[1][1] + b2 * m.m[1][2];
        s[2] = b0 * m.m[2][0] + b1 * m.m[2][1] + b2 * m.m[2][2];
    }
    setPartAttachmentAxesRel(index, p, s);
}

} // namespace Vx
