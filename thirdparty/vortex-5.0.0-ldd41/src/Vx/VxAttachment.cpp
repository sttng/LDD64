// Vortex 5.0.0 build 800 "vortexldd41" - VxAttachment (restored).
// Mac TU 456 0x42e904-0x42f0db (static initialiser 0x42ef01: <iostream> and the five type-name
// strings) + the Win-only members VxConstraint needs (Win 0x7824b0-0x782bb0, 0x7828f0). The
// vector<std::string> insert helper Mac 0x42ecce / Win 0x782f20 is library code (the type-name
// vector Win 0x782bb0 builds).
#include "Vx/VxAttachment.h"
#include "Vx/VxPart.h"
#include "Vx/VxFrame.h"
#include <iostream>

namespace Vx {

namespace {
// Mac 0x77f76c-0x77f77c (atexit 0x42ebbd, 0x42eb62, 0x42ec73, 0x42eb06, 0x42ec18)
const std::string sRelativeToPart("Relative To Part");
const std::string sRelativeToPartCOM("Relative To Part COM");
const std::string sAbsolute("Absolute");
const std::string sRelativeToNode("Relative To Node");
const std::string sSlave("Slave");
} // namespace

// C2 Mac 0x42e904 (C1 0x42e962), Win 0x7824b0
VxAttachment::VxAttachment(int type)
    : mPart(0), mType(type)
{
#if defined(_WIN32)
    mVfptrPad = 0;
#endif
}

// C2 Mac 0x42ea66 (C1 0x42eaec), Win 0x782b20: zero, then operator=
VxAttachment::VxAttachment(const VxAttachment& o)
    : mPart(0), mType(0)
{
#if defined(_WIN32)
    mVfptrPad = 0;
#endif
    *this = o;
}

void VxAttachment::setNode(const VxNode& node)
{
    if (mNode.mHandle != node.mHandle) {
        VxNode old = mNode;
        mNode = node;
        mNode.addRef();
        old.release();
    }
}

// Mac 0x42e99e, Win 0x7825b0
VxAttachment& VxAttachment::operator=(const VxAttachment& o)
{
    if (&o != this) {
        mType = o.mType;
        mPart = o.mPart;
        setNode(o.mNode);
        mPosition = o.mPosition;
        mPrimaryAxis = o.mPrimaryAxis;
        mSecondaryAxis = o.mSecondaryAxis;
    }
    return *this;
}

// Win 0x7824f0
void VxAttachment::reset()
{
    mType = kRelativeToPart;
    mPart = 0;
    mNode.release();
    mNode.mHandle = 0;
    mPosition.setZero();
    mPrimaryAxis.setZero();
    mSecondaryAxis.setZero();
}

// Win 0x782540
bool VxAttachment::isNull() const
{
    return mPrimaryAxis[2] * mPrimaryAxis[2] + mPrimaryAxis[0] * mPrimaryAxis[0]
               + mPrimaryAxis[1] * mPrimaryAxis[1]
           == 0.0;
}

// Win 0x782640
void VxAttachment::getLocalTransform(VxTransform& tm) const
{
    tm = VxTransform();
    tm.setAxis(0, mPrimaryAxis);
    tm.setAxis(1, mSecondaryAxis);
    tm.m[2][0] = tm.m[1][2] * tm.m[0][1] - tm.m[0][2] * tm.m[1][1];
    tm.m[2][1] = tm.m[0][2] * tm.m[1][0] - tm.m[1][2] * tm.m[0][0];
    tm.m[2][2] = tm.m[1][1] * tm.m[0][0] - tm.m[0][1] * tm.m[1][0];
    tm.setTranslation(mPosition);
}

// Win 0x7826f0
void VxAttachment::setLocalTransform(const VxTransform& tm)
{
    mPrimaryAxis = tm.getAxis(0);
    mSecondaryAxis = tm.getAxis(1);
    mPosition = tm.getTranslation();
}

// Win 0x782750
void VxAttachment::getTransform(VxTransform& tm) const
{
    if (isNull()) {
        tm = VxTransform();
        return;
    }
    getLocalTransform(tm);
    switch (mType) {
    case kRelativeToPart:
    case kSlave:
        if (mPart != 0)
            tm.postMultiply(*mPart->mTransformPtr);
        break;
    case kRelativeToPartCOM:
        if (mPart != 0) {
            VxTransform com = *mPart->mTransformPtr;
            VxVector3 p = com.getTranslation();
            mPart->getCOMPosition(p);
            com.setTranslation(p);
            tm.postMultiply(com);
        }
        break;
    case kRelativeToNode:
        if (mNode.mHandle != 0) {
            // NOTE: Win goes through 0x760d80 (the node's transform); here the scene-graph
            // interface is asked directly (LDD never binds nodes)
            VxTransform node;
            VxVector3 scale;
            vxSceneGraphInterface(VxFrame::instance())->getNodeTransform(mNode, node, scale, 0);
            tm.postMultiply(node);
        }
        break;
    default:
        break;
    }
}

// Win 0x7828f0 (Win only): the reference transform as in getTransform, then the local
// frame = world relative to it (Win 0x719940, inlined here: out = A * B^T on the rotation rows,
// translation (A.t - B.t) * B^T; every sum in the order k = 0, 1, 2)
void VxAttachment::setTransform(const VxTransform& world)
{
    VxTransform ref;
    switch (mType) {
    case kRelativeToPart:
    case kSlave:
        if (mPart != 0)
            ref = *mPart->mTransformPtr;
        break;
    case kRelativeToPartCOM:
        if (mPart != 0) {
            VxTransform com = *mPart->mTransformPtr;
            VxVector3 p = com.getTranslation();
            mPart->getCOMPosition(p);
            com.setTranslation(p);
            ref = com;
        }
        break;
    case kRelativeToNode:
        if (mNode.mHandle != 0) {
            // NOTE: Win goes through 0x760d80 (the node's transform), as in getTransform
            VxVector3 scale;
            vxSceneGraphInterface(VxFrame::instance())->getNodeTransform(mNode, ref, scale, 0);
        }
        break;
    default:
        break;
    }
    const VxReal (*A)[4] = world.m;
    const VxReal (*B)[4] = ref.m;
    VxTransform local;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            local.m[i][j] = A[i][0] * B[j][0] + A[i][1] * B[j][1] + A[i][2] * B[j][2];
    VxReal d0 = A[3][0] - B[3][0], d1 = A[3][1] - B[3][1], d2 = A[3][2] - B[3][2];
    for (int j = 0; j < 3; ++j)
        local.m[3][j] = B[j][0] * d0 + B[j][1] * d1 + B[j][2] * d2;
    local.m[0][3] = 0;
    local.m[1][3] = 0;
    local.m[2][3] = 0;
    local.m[3][3] = 1;
    setLocalTransform(local);
}

// Win 0x782a10
void VxAttachment::orthonormalize()
{
    VxTransform tm;
    tm.setAxis(0, mPrimaryAxis);
    tm.setAxis(1, mSecondaryAxis);
    tm.m[2][0] = tm.m[0][1] * tm.m[1][2] - tm.m[0][2] * tm.m[1][1];
    tm.m[2][1] = tm.m[0][2] * tm.m[1][0] - tm.m[1][2] * tm.m[0][0];
    tm.m[2][2] = tm.m[1][1] * tm.m[0][0] - tm.m[0][1] * tm.m[1][0];
    tm.orthonormalize(0);
    mPrimaryAxis = tm.getAxis(0);
    mSecondaryAxis = tm.getAxis(1);
}

// Win 0x782bb0
std::string VxAttachment::getTypeName(int type)
{
    switch (type) {
    case kRelativeToPart: return sRelativeToPart;
    case kRelativeToPartCOM: return sRelativeToPartCOM;
    case kAbsolute: return sAbsolute;
    case kRelativeToNode: return sRelativeToNode;
    case kSlave: return sSlave;
    default: return std::string();
    }
}

} // namespace Vx
