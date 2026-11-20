// Vortex 5.0.0 build 800 "vortexldd41" - VxEntity (restored).
// Mac TU 484 0x44664e-0x447dd6 (static initialiser 0x447d36: <iostream>, MdtConstraintMaxSize,
// the static FollowAnimationDriver); Win 0x7169b0-0x717cc0. The TU starts with
// VxDynamicsResponseInput's type list (Mac 0x44664e, see VxDynamicsContactInput.cpp).
#include "Vx/VxEntity.h"
#include "Vx/VxUniverse.h"
#include "Vx/VxFrame.h"
#include "Vx/VxSceneGraphInterface.h"
#include "Me/MeMath.h"
#include "Me/MeMemory.h"
#include "Mdt/MdtConstraint.h" // the static MdtConstraintMaxSize (Mac 0x77fdb8)
#include <iostream>

namespace Vx {

VxBase::NameMap* VxEntity::sNameMap = 0;                    // Mac 0x77931c
VxEntity::FollowAnimationDriver VxEntity::sFollowAnimationDriver; // Mac 0x77cdd8

// getStaticClassType Mac 0x446780, isOfClassType Mac 0x44678a / Win 0x717070, getStaticClassTypes
// Mac 0x447090, isOfType Mac 0x447418 / Win 0x717620
VX_IMPLEMENT_CLASS_TYPES(VxEntity, VxBaseUserData, 0x100)

// Mac 0x446eee
VxEntity::VxEntity()
    : mParent(0), mAnimationDriver(0), mAssembly(0), mUniverse(0), mNodeParent(0),
      mTransformPtr(0), mNodeOffset(0), mScale(1.0, 1.0, 1.0), mControl(kControlAnimated),
      mInternalControl(kControlAnimated)
{
    mTransformPtr = &mTransform;
}

// D2 Mac 0x44774e, Win 0x7177d0 (D1 Mac 0x447896, D0 Mac 0x447600 / Win 0x717cc0)
VxEntity::~VxEntity()
{
    while (getChildCount() != 0)
        mChildren[0]->setParent(0, 0);
    setParent(0, 0);
    if (mNode.mHandle != 0) {
        VxNode none = VxNode::sNull;
        setNode(none, 0);
    }
    if (mNodeOffset) {
        MeMemoryAPI.destroy(mNodeOffset);
        mNodeOffset = 0;
    }
    VxBase::setName(0, &sNameMap);
    mNode.release();
}

// Mac 0x447ba8, Win 0x717c50
const char* VxEntity::getClassName() const
{
    return "VxEntity";
}

// Mac 0x446bde, Win 0x7169b0
void VxEntity::setName(const char* name)
{
    VxBase::setName(name, &sNameMap);
}

// Mac 0x446bac, Win 0x7169d0
VxEntity* VxEntity::findFromName(const char* name)
{
    VxEntity* e = 0;
    if (sNameMap)
        e = (VxEntity*)VxBase::findFromName(name, sNameMap);
    return e;
}

// Mac 0x447be0, Win 0x717c80
void VxEntity::freeze(bool b)
{
    if (!b)
        setInternalControl(mControl);
    else
        setInternalControl(kControlFrozen);
}

// Mac 0x447bba, Win 0x719870
int VxEntity::getCollisionGeometryCount() const
{
    return 0;
}

// Mac 0x447bc4, Win 0x7c61f0
VxCollisionGeometry* VxEntity::getCollisionGeometry(int) const
{
    return 0;
}

// Mac 0x446ae0, Win 0x717c60
void VxEntity::setTransform(const VxTransform& tm)
{
    *mTransformPtr = tm;
    mUserTransform.assign(tm);
}

// Mac 0x447bce, Win 0x7076a0 (shared empty function)
void VxEntity::updateCollisionGeometryTransforms()
{
}

// Mac 0x447bd4, Win 0x7076a0
void VxEntity::removeFromUniverse()
{
}

// Mac 0x447bda, Win 0x713eb0
void VxEntity::addToUniverse(VxUniverse*)
{
}

// Mac 0x44681e, Win 0x716ff0: leave the old universe, frozen while switching, then join
void VxEntity::setUniverse(VxUniverse* u)
{
    if (mUniverse == u)
        return;
    if (mUniverse) {
        removeFromUniverse();
        mUniverse = 0;
    }
    int control = mInternalControl;
    freeze(false);
    mUniverse = u;
    if (u)
        addToUniverse(u);
    if (control == kControlFrozen)
        freeze(true);
}

// Mac 0x4478aa, Win 0x716fd0
void VxEntity::getTransform(VxTransform& tm) const
{
    tm = *mTransformPtr;
}

// Mac 0x446c58, Win 0x716a70
const VxReal* VxEntity::getPositionPtr() const
{
    return mTransformPtr->m[3];
}

// Mac 0x4467ae, Win 0x716a80
void VxEntity::getPosition(VxVector3& p) const
{
    const VxTransform* tm = mTransformPtr;
    p[0] = tm->m[3][0];
    p[1] = tm->m[3][1];
    p[2] = tm->m[3][2];
}

// Mac 0x446c0e, Win 0x716c70
void VxEntity::getEulerAngles(VxVector3& angles, int order) const
{
    VxEulerAngles ea = VxEulerAnglesFromTransform(*mTransformPtr, order);
    angles[0] = ea.angle[0];
    angles[1] = ea.angle[1];
    angles[2] = ea.angle[2];
}

// Mac 0x4467da
void VxEntity::setScale(const VxVector3& s)
{
    mScale[0] = s[0];
    mScale[1] = s[1];
    mScale[2] = s[2];
}

// Mac 0x44693a, Win 0x717340
void VxEntity::setInternalControl(int control)
{
    mInternalControl = control;
    updateAnimationMode();
}

// Mac 0x4468a6
void VxEntity::updateAnimationMode()
{
    mAnimationMode = 2;
    switch (mInternalControl) {
    case kControlStatic:
        if (mParent == 0)
            mAnimationMode = 0;
        break;
    case kControlFrozen:
    case kControlAnimated:
        if (getChildCount() > 0)
            mAnimationMode = 0;
        break;
    case kControlDynamic:
        if (getChildCount() > 0)
            mAnimationMode = 1;
        break;
    }
}

// Mac 0x44680c
int VxEntity::getChildCount() const
{
    return mChildren.size();
}

// Mac 0x44745e
void VxEntity::addChild(VxEntity* child)
{
    mChildren.push_back(child);
    updateAnimationMode();
}

// Mac 0x446956
void VxEntity::removeChild(VxEntity* child)
{
    mChildren.remove(child);
    updateAnimationMode();
}

// Mac 0x447544
void VxEntity::setParent(AnimationDriver* driver, VxEntity* parent)
{
    if (mParent)
        mParent->removeChild(this);
    if (mAnimationDriver && mAnimationDriver != driver
        && dynamic_cast<FollowWithOffsetAnimationDriver*>(mAnimationDriver) != 0)
        delete mAnimationDriver;
    mAnimationDriver = driver;
    mParent = parent;
    if (parent == 0)
        return;
    if (mAnimationDriver == 0) {
        mAnimationDriver = 0;
        mParent = 0;
        return;
    }
    // refuse a cycle: this entity may not be one of the new parent's ancestors
    for (VxEntity* e = parent->mParent; e != 0; e = e->mParent) {
        if (e == this) {
            mAnimationDriver = 0;
            mParent = 0;
            return;
        }
    }
    parent->addChild(this);
}

// Mac 0x4469b6
void VxEntity::applyAnimationDriver()
{
    if (mInternalControl == kControlStatic && mAnimationDriver != 0 && mUniverse != 0) {
        mAnimationDriver->drive(mParent, this, mUniverse->getTimeStep());
        updateCollisionGeometryTransforms();
    }
    for (VxEntity** c = mChildren.begin(); c != mChildren.end(); ++c)
        (*c)->applyAnimationDriver();
}

// Mac 0x446a42
void VxEntity::applyAnimationDriverIfMode0()
{
    if (mAnimationMode == 0)
        applyAnimationDriver();
}

// Mac 0x446c66
void VxEntity::applyAnimationDriverIfNotMode2()
{
    if (mAnimationMode != 2)
        applyAnimationDriver();
}

// Mac 0x446c82
void VxEntity::updateBeforeStep()
{
    updateFromMergeParent();
    if (mInternalControl != kControlDynamic && mInternalControl == kControlAnimated)
        moveTo(mUserTransform, 0.0);
    if (mAnimationMode == 1)
        applyAnimationDriver();
}

// Mac 0x446cda
void VxEntity::updateFromNode()
{
    if (mNode.mHandle == 0)
        return;
    VxSceneGraphInterface* sg = vxSceneGraphInterface(VxFrame::instance());
    VxTransform tm;
    VxVector3 scale;
    VxNode node = mNode;
    if (sg->getNodeTransform(node, tm, scale, 0)) {
        setTransform(tm);
        setScale(scale);
    }
}

// Mac 0x446d6e
bool VxEntity::setNode(const VxNode& node, int mode)
{
    bool changed = false;
    if (mNode == node)
        return changed;
    if (mNode.mHandle != 0) {
        VxSceneGraphInterface* sg = vxSceneGraphInterface(VxFrame::instance());
        VxNode old = mNode;
        sg->detachEntity(old);
        mNode.release();
        mNode.mHandle = 0;
        changed = true;
    }
    if (node.mHandle != 0) {
        if (node != mNode) {
            mNode.release();
            mNode = node;
            mNode.addRef();
        }
        VxSceneGraphInterface* sg = vxSceneGraphInterface(VxFrame::currentInstance());
        VxNode n = mNode;
        changed = sg->attachEntity(n, this);
        if (!changed) {
            // VxFrame Mac 0x453ec2 (inlined here)
            VxSceneGraphInterface* sg2 = vxSceneGraphInterface(VxFrame::currentInstance());
            VxNode n2 = node;
            sg2->releaseNode(n2);
        }
        if (mode == 1) {
            VxSceneGraphInterface* sg3 = vxSceneGraphInterface(VxFrame::currentInstance());
            VxNode n3 = mNode;
            sg3->setNodeTransformPtr(n3, &mTransformPtr->m[0][0]);
        } else {
            updateFromNode();
        }
        mUserTransform.assign(*mTransformPtr);
    }
    return changed;
}

// Mac 0x447960, Win 0x716f60
void VxEntity::getNodeTransform(VxTransform& tm) const
{
    if (mNodeParent == 0) {
        if (mNodeOffset == 0)
            tm = *mTransformPtr;
        else
            MeMatrix4TMMakeRelative(tm.m, mTransformPtr->m, mNodeOffset->m);
    } else {
        const VxTransform* base;
        VxTransform t;
        if (mNodeOffset == 0) {
            base = mNodeParent->mTransformPtr;
        } else {
            MeMatrix4TMCompound(t.m, mNodeOffset->m, mNodeParent->mTransformPtr->m);
            base = &t;
        }
        MeMatrix4TMMakeRelative(tm.m, mTransformPtr->m, base->m);
    }
}

// Mac 0x447a94, Win 0x7171f0: the node gets the transform with the translation divided by the scale
void VxEntity::updateNode()
{
    if (mNode.mHandle == 0)
        return;
    VxTransform tm;
    getNodeTransform(tm);
    tm.m[3][0] = tm.m[3][0] / mScale[0];
    tm.m[3][1] = tm.m[3][1] / mScale[1];
    tm.m[3][2] = tm.m[3][2] / mScale[2];
    VxSceneGraphInterface* sg = vxSceneGraphInterface(VxFrame::instance());
    VxNode node = mNode;
    sg->setNodeTransform(node, tm);
}

// --------------------------------------------------------------------------------------
// Mac 0x447cf0 / 0x447cc6, Win 0x717c20
VxEntity::FollowAnimationDriver::~FollowAnimationDriver()
{
}

// Mac 0x447d0a, Win 0x717a30
void VxEntity::FollowAnimationDriver::drive(VxEntity* parent, VxEntity* entity, VxReal dt)
{
    if (parent)
        entity->moveTo(*parent->mTransformPtr, dt);
}

// Win 0x717a60 (Mac dead-stripped)
VxEntity::FollowWithOffsetAnimationDriver::FollowWithOffsetAnimationDriver(const VxTransform& offset)
    : mOffset(offset)
{
}

// Win 0x717be0 (+ the product Win 0x717aa0)
void VxEntity::FollowWithOffsetAnimationDriver::drive(VxEntity* parent, VxEntity* entity, VxReal dt)
{
    if (parent) {
        MeMatrix4TMCompound(mResult.m, mOffset.m, parent->mTransformPtr->m);
        entity->moveTo(mResult, dt);
    }
}

} // namespace Vx
