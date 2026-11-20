// Vortex 5.0.0 build 800 "vortexldd41" - VxPartMergeInfo: the merge tree of parts (restored).
// Mac TU 509 0x46b690-0x46c87d (static initialiser 0x46c80d: <iostream>,
// MdtConstraintMaxSize), Win 0x777eb0-0x778000 (most of it inlined on Win). File, class and
// function names inferred (the only string is "VxPart::setMergeParent").
#include "Vx/VxPart.h"
#include "Vx/VxConstraint.h"
#include "Mdt/MdtConstraint.h"
#include <iostream>

namespace Vx {

namespace {
// rel = a's frame in b's frame: rotation a R_b^T, translation (t_a - t_b) R_b^T (Mac 0x46c13a /
// 0x46be4c, inlined)
void RelativeTransform(VxTransform& rel, const VxTransform& a, const VxTransform& b)
{
    const VxReal(*A)[4] = a.m;
    const VxReal(*B)[4] = b.m;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            rel.m[i][j] = A[i][0] * B[j][0] + A[i][1] * B[j][1] + A[i][2] * B[j][2];
    const VxReal dz = A[3][2] - B[3][2];
    const VxReal dy = A[3][1] - B[3][1];
    const VxReal dx = A[3][0] - B[3][0];
    for (int j = 0; j < 3; ++j)
        rel.m[3][j] = dx * B[j][0] + dy * B[j][1] + dz * B[j][2];
}
} // namespace

// C2 Mac 0x46ba86 (C1 0x46bd3e), Win 0x778000
VxPartMergeInfo::VxPartMergeInfo(VxPart* part)
    : mPart(part), mParent(0), mRoot(0), mFirstChild(0), mNext(0), mPrevious(0),
      mMassProperties(part), mSeparationTest(0)
{
    mRoot = this;
    resetTransforms();
    mMassProperties.copy(*part->getMassProperties());
}

void VxPartMergeInfo::resetTransforms()
{
    mRelativeTransform = VxTransform();
    mRootTransform = VxTransform();
}

// Mac 0x46b690
void VxPartMergeInfo::addChild(VxPartMergeInfo* child)
{
    if (mFirstChild)
        mFirstChild->mPrevious = child;
    child->mNext = mFirstChild;
    child->mPrevious = 0;
    mFirstChild = child;
}

// Mac 0x46b6b6
void VxPartMergeInfo::removeChild(VxPartMergeInfo* child)
{
    if (mFirstChild == child)
        mFirstChild = child->mNext;
    if (child->mPrevious)
        child->mPrevious->mNext = child->mNext;
    if (child->mNext)
        child->mNext->mPrevious = child->mPrevious;
    child->mNext = 0;
    child->mPrevious = 0;
    child->mParent = 0;
}

// Mac 0x46b71c, Win 0x777f10
void VxPartMergeInfo::subtractMassProperties(VxMassProperties* m, const VxTransform* tm, bool apply)
{
    if (mFirstChild == 0)
        mMassProperties.copy(*mPart->getMassProperties());
    else
        mMassProperties.subtractMassProperties(*m, *tm);
    if (mParent)
        mParent->subtractMassProperties(m, &mRelativeTransform, apply);
    else if (apply)
        mPart->applyMassProperties(&mMassProperties);
}

// Mac 0x46b7bc, Win 0x777eb0
void VxPartMergeInfo::addMassProperties(VxMassProperties* m, const VxTransform* tm, bool apply)
{
    if (mFirstChild == 0)
        mMassProperties.copy(*m);
    else
        mMassProperties.addMassProperties(*m, *tm);
    if (mParent)
        mParent->addMassProperties(m, &mRelativeTransform, apply);
    else if (apply)
        mPart->applyMassProperties(&mMassProperties);
}

// Mac 0x46b852: this part's transform from its parent part's current one
void VxPartMergeInfo::computeTransform(VxTransform& tm) const
{
    const VxReal(*P)[4] = mParent->mPart->mTransformPtr->m;
    const VxReal(*R)[4] = mRelativeTransform.m;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            tm.m[i][j] = R[i][0] * P[0][j] + R[i][1] * P[1][j] + R[i][2] * P[2][j];
    for (int j = 0; j < 3; ++j)
        tm.m[3][j] = R[3][0] * P[0][j] + R[3][1] * P[1][j] + R[3][2] * P[2][j] + P[3][j];
}

// Mac 0x46bd58
void VxPartMergeInfo::collectChildren(VxArray<VxPartMergeInfo*>& out) const
{
    for (VxPartMergeInfo* c = mFirstChild; c != 0; c = c->mNext)
        out.push_back(c);
}

// Mac 0x46c714
void VxPartMergeInfo::collectDescendants(VxArray<VxPartMergeInfo*>& out) const
{
    for (VxPartMergeInfo* c = mFirstChild; c != 0; c = c->mNext) {
        out.push_back(c);
        c->collectDescendants(out);
    }
}

// Mac 0x46be4c: the body follows the root's body; constraint attachments are set again
void VxPartMergeInfo::updateRoot()
{
    *mPart->getMergeRootBodyPtr() = mRoot->mPart->mBody;
    if (mPart != mRoot->mPart)
        RelativeTransform(mRootTransform, *mPart->mTransformPtr, *mRoot->mPart->mTransformPtr);
    for (VxConstraint** c = mPart->mConstraints.begin(); c != mPart->mConstraints.end(); ++c) {
        VxConstraint* constraint = *c;
        for (int i = 0; i < constraint->getPartCount(); ++i) {
            VxPart* p = constraint->mParts.begin()[i];
            if (p == mPart)
                constraint->setPart(i, p);
        }
    }
    for (VxPartMergeInfo* c = mFirstChild; c != 0; c = c->mNext) {
        c->mRoot = mRoot;
        c->updateRoot();
    }
}

// Mac 0x46c13a
bool VxPartMergeInfo::setParent(VxPartMergeInfo* parent)
{
    VxPartMergeInfo* old = mParent;
    if (old == parent)
        return true;
    if (old) {
        old->removeChild(this);
        old->subtractMassProperties(&mMassProperties, &mRelativeTransform, true);
        mRoot = this;
        resetTransforms();
    }
    if (parent == 0) {
        updateRoot();
        mPart->applyMassProperties(&mMassProperties);
    } else {
        mParent = parent;
        mRoot = parent->mRoot;
        parent->addChild(this);
        RelativeTransform(mRelativeTransform, *mPart->mTransformPtr, *mParent->mPart->mTransformPtr);
        RelativeTransform(mRootTransform, *mPart->mTransformPtr, *mRoot->mPart->mTransformPtr);
        updateRoot();
        mParent->addMassProperties(&mMassProperties, &mRelativeTransform, true);
    }
    return true;
}

} // namespace Vx
