// Vortex 5.0.0 build 800 "vortexldd41" - VxPart (restored).
// Mac TU 508 0x467efe-0x46b68f (static initialiser 0x46b61f: <iostream>, MdtConstraintMaxSize);
// Win 0x71b7f0-0x71e930 (+ 0x521370, 0x707f90, 0x714600, 0x719ec0, 0x71a430, 0x730a50, 0x730a80,
// 0x731ac0 placed by /LTCG).
#include "Vx/VxPart.h"
#include "Vx/VxCollisionGeometry.h"
#include "Vx/VxConstraint.h"
#include "Vx/VxUniverse.h"
#include "Vx/VxFrame.h"
#include "Vx/VxAssemblyBase.h"
#include "Vx/VxDynamicsContactInput.h"
#include "Mdt/MdtBody.h"
#include "Mdt/MdtConstraint.h"
#include "Mdt/MdtWorld.h"
#include "Me/MeMath.h"
#include "Vinc/VincModel.h"
#include <math.h>
#include <float.h>
#include <iostream>

namespace Vx {

namespace {

// Mac 0x467efe: the static identity 3x3 at Mac 0x779440 (initialised data)
const VxReal (&IdentityInertia())[3][3]
{
    static const VxReal sIdentity[3][3] = { { 1.0, 0, 0 }, { 0, 1.0, 0 }, { 0, 0, 1.0 } };
    return sIdentity;
}

// Mac 0x4682ab, Win 0x71b7f0: a function-local static identity transform
const VxTransform& IdentityTransform()
{
    static VxTransform sIdentity;
    return sIdentity;
}

// The "collision geometry contributes to the mass" flag mask (Mac 0x643d30 = 1, read through
// 0x7025e0) (name inferred)
const unsigned int kMassFlagMask = 1;
// VincModelSetM80's value for a part's models (Mac 0x644328 = 0 through 0x702500)
const int kDefaultModelM80 = 0;

// Mac 0x46891d (eax = model, edx = body), Win 0x71b880: binds a collision model to a body (0:
// unbinds): body transform (unless the model is an aggregate element) and velocity pointers
void ConnectModel(VincModelID m, MdtBody* body)
{
    if (m == 0)
        return;
    m->m6c = (ptrdiff_t)body; // the body in the model (ptrdiff_t: no truncation on 64-bit hosts)
    if (body == 0) {
        m->setVelocityPtrsFn(m, 0, 0);
        return;
    }
    if (m->parent == 0)
        VincModelSetTransformPtr(m, MdtBodyGetTransformPtr(body));
    m->setVelocityPtrsFn(m, MdtBodyGetLinearVelocityPtr(body), MdtBodyGetAngularVelocityPtr(body));
    VincModelSetM80(m, (void*)(size_t)kDefaultModelM80);
}

// the setTransform comparisons (Mac 0x46931c): rows 0-2 and the translation, not column 3
const int kCompared[12] = { 0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14 };

bool SameRelative(const VxReal* cur, const VxReal* tm)
{
    for (int k = 0; k != 12; ++k) {
        const VxReal c = cur[kCompared[k]];
        const VxReal n = tm[kCompared[k]];
        if (fabs(c) >= 1e-14 || c != c) {
            const VxReal d = fabs((n - c) / c);
            if (!(d < 1e-14))
                return false;
        } else if (fabs(n) >= 1e-14) {
            return false;
        }
    }
    return true;
}

bool SameExact(const VxReal* cur, const VxReal* tm)
{
    for (int k = 0; k != 12; ++k)
        if (!(cur[kCompared[k]] == tm[kCompared[k]]))
            return false;
    return true;
}

// the separation test of a merged part (Mac: inlined in every force / state call)
bool ShouldSeparate(const VxPartMergeInfo* info)
{
    return info->mSeparationTest != 0 && info->mSeparationTest->shouldSeparate();
}

} // namespace

bool VxPart::sSweepByDefault = false; // Mac 0x779420, Win 0xaf14c4

// getStaticClassType Mac 0x467f12 / Win 0x71b910, isOfClassType Mac 0x467f1c / Win 0x71c7a0,
// getStaticClassTypes Mac 0x46acd6, isOfType Mac 0x46b05e / Win 0x71e530
VX_IMPLEMENT_CLASS_TYPES(VxPart, VxCollisionSensor, 0x108)
// new Mac 0x468237 / Win 0x71b8e0, delete Mac 0x468294 / Win 0x707f90
VX_IMPLEMENT_ALLOCATOR(VxPart)

// C2 Mac 0x46b0a4 (C1 0x46b1ae), Win 0x71e560
VxPart::VxPart()
    : mBody(0), mMassProperties(0), mUserMass(false), mUserCOM(false), mUserInertia(false),
      mMergeInfo(0)
{
    mBody = MdtBodyCreateFromPool((VinPool*)VxFrame::instance()->mBodyPool);
    init();
    mControl = kControlStatic;
    setInternalControl(kControlStatic);
    setControl(kControlAnimated);
}

// Win 0x71e620 (Mac dead-stripped)
VxPart::VxPart(VxReal mass)
    : mBody(0), mMassProperties(0), mUserMass(false), mUserCOM(false), mUserInertia(false),
      mMergeInfo(0)
{
    mBody = MdtBodyCreateFromPool((VinPool*)VxFrame::instance()->mBodyPool);
    init();
    mControl = kControlStatic;
    setInternalControl(kControlStatic);
    if (mass == 0.0) {
        setControl(kControlAnimated);
    } else {
        if (sSweepByDefault)
            mSweepEnabled = true;
        setControl(kControlDynamic);
        setMass(mass);
    }
}

// Mac 0x4689da, Win 0x71d8c0
void VxPart::init()
{
    mConstraints.clear();
    mResponseModel = 0; // the constant Mac 0x644328 (through 0x702500)
    mTransformPtr = (VxTransform*)MdtBodyGetTransformPtr(mBody);
    mMassProperties = new VxMassProperties(this);
    MdtBodySetUserData(mBody, this);
    setGrouping(0);
}

// D2 Mac 0x46b1c2, Win 0x71d480 (D1 Mac 0x46b384, D0 Mac 0x46b398 / Win 0x71e930)
VxPart::~VxPart()
{
    mUserInertia = true;
    mUserCOM = true;
    mUserMass = true;
    setContactHandler(0);
    if (mMergeInfo) {
        separateMergedParts();
        delete mMergeInfo;
        mMergeInfo = 0;
    }
    int n;
    while ((n = getConstraintCount()) > 0) {
        VxConstraint* c = getConstraint(n - 1);
        for (int i = 0; i < c->mParts.size(); ++i)
            if (c->mParts.begin()[i] == this)
                c->setPart(i, 0);
    }
    delete mMassProperties;
    mMassProperties = 0;
    mControl = kControlAnimated;
    MdtBodySetUserData(mBody, 0);
    MdtBodyDisable(mBody);
    MdtBodyFreeToPool(mBody, (VinPool*)VxFrame::instance()->mBodyPool);
}

// Mac 0x46b56a, Win 0x71e920
const char* VxPart::getClassName() const
{
    return "VxPart";
}

// Mac 0x4680ee, Win 0x71c580
void VxPart::setName(const char* name)
{
    VxCollisionSensor::setName(name);
    mBody->name = getName();
}

// Mac 0x46b560, Win 0x752ce0
bool VxPart::isPart() const
{
    return true;
}

// Mac 0x467f32, Win 0x71c880
int VxPart::getConstraintCount() const
{
    return mConstraints.size();
}

// Mac 0x467f4a, Win 0x71d390 (NOTE: no range check beyond "not empty", original)
VxConstraint* VxPart::getConstraint(int i) const
{
    VxConstraint* c = 0;
    if (mConstraints.begin() != mConstraints.end())
        c = mConstraints.begin()[i];
    return c;
}

// ---- mass ------------------------------------------------------------------------------------

// Mac 0x467ff2, Win 0x71bbe0
VxReal VxPart::getMass() const
{
    if (mControl == kControlDynamic)
        return mMassProperties->mMass;
    return 0;
}

// Mac 0x468adc, Win 0x71bd40
void VxPart::getCOMPosition(VxVector3& p) const
{
    if (mControl == kControlDynamic)
        mMassProperties->getCOMWorldPosition(p);
}

// Mac 0x46a670, Win 0x71e060
void VxPart::setMass(VxReal mass)
{
    if (mControl != kControlDynamic)
        return;
    VxAssert(0.0 < mass, "VxPart::setMass: mass must be larger than 0.\n");
    VxMassProperties* mp = mMassProperties;
    VxReal current = mp->mMass;
    if (mass != current) {
        if (current == 0.0) {
            mp->setInertiaTensor(IdentityInertia());
            current = 1.0;
        }
        if (mMergeInfo)
            mMergeInfo->subtractMassProperties(mMassProperties, &IdentityTransform(), false);
        mMassProperties->scaleInertiaTensor(mass / current);
        mMassProperties->setMass(mass);
        if (mMergeInfo == 0)
            applyMassProperties(mMassProperties);
        else
            mMergeInfo->addMassProperties(mMassProperties, &IdentityTransform(), true);
    }
    mUserMass = true;
}

// Win 0x71cb00 (Mac dead-stripped)
void VxPart::setMassAndInertia(VxReal mass, const VxReal inertia[3][3])
{
    if (mControl != kControlDynamic || !(mass > 0.0))
        return;
    if (0.0 < inertia[0][0] && 0.0 < inertia[1][1] && 0.0 < inertia[2][2]) {
        mUserMass = true;
        mUserInertia = true;
        mMassProperties->setMass(mass);
        mMassProperties->setInertiaTensor(inertia);
        applyMass(mMassProperties);
        applyInertia(mMassProperties);
    }
}

// Mac 0x468564, Win 0x730a50
void VxPart::applyMass(VxMassProperties* mp)
{
    MdtBodySetMass(mBody, mp->mMass);
}

// Mac 0x46853e, Win 0x730a80
void VxPart::applyInertia(VxMassProperties* mp)
{
    MdtBodySetInertiaTensor(mBody, mp->mInertia);
}

// Mac 0x46a066, Win 0x71d9a0
void VxPart::applyMassProperties(VxMassProperties* mp)
{
    applyMass(mp);
    applyInertia(mp);
    applyCenterOfMass(mp);
}

// Mac 0x46831a, Win 0x71c440
void VxPart::updateCenterOfRotation()
{
    VxVector3 com;
    mMassProperties->getCOMPosition(com);
    for (int i = 0; i < getCollisionGeometryCount(); ++i)
        VincModelSetCenterOfRotation(getCollisionGeometry(i)->mModel, &com[0]);
}

// Mac 0x46837e, Win 0x71a430
void VxPart::updateConstraintAttachments(VxConstraint* c, const VxTransform& comTransform)
{
    const MeReal(*rel)[4] = mBody->relativeTransform;
    MeMatrix4 m;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            m[i][j] = comTransform.m[i][0] * rel[0][j] + comTransform.m[i][1] * rel[1][j]
                      + comTransform.m[i][2] * rel[2][j] + comTransform.m[i][3] * rel[3][j];
    if (MeMatrix4IsIdentity(m, 1e-14))
        return;
    MeMatrix4TMInvert(m);
    MdtConstraint* mc = c->getMdtConstraint();
    for (int i = 0; i < mc->bcl.numBodies; ++i) {
        if (mc->body[i] == mBody) {
            MeVector3 p;
            MdtConstraintGetBodyPositionRel(mc, i, p);
            MdtConstraintSetBodyPositionRel(mc, i,
                                            p[0] * m[0][0] + p[1] * m[1][0] + p[2] * m[2][0] + m[3][0],
                                            p[0] * m[0][1] + p[1] * m[1][1] + p[2] * m[2][1] + m[3][1],
                                            p[0] * m[0][2] + p[1] * m[1][2] + p[2] * m[2][2] + m[3][2]);
        }
    }
}

// Mac 0x469dd4, Win 0x71d6f0
void VxPart::applyCenterOfMass(VxMassProperties* mp)
{
    if (mUniverse == 0) {
        if (isMergeRoot()) {
            for (VxConstraint** c = mConstraints.begin(); c != mConstraints.end(); ++c)
                updateConstraintAttachments(*c, mp->mCOMTransform);
            if (mMergeInfo) {
                VxArray<VxPartMergeInfo*> merged;
                mMergeInfo->collectDescendants(merged);
                for (VxPartMergeInfo** i = merged.begin(); i != merged.end(); ++i) {
                    VxPart* p = (*i)->mPart;
                    for (VxConstraint** c = p->mConstraints.begin(); c != p->mConstraints.end(); ++c)
                        updateConstraintAttachments(*c, mp->mCOMTransform);
                }
            }
            for (int i = 0; i < 4; ++i)
                for (int j = 0; j < 4; ++j)
                    mBody->relativeTransform[i][j] = mp->mCOMTransform.m[i][j];
            MeMatrix4TMInvert(mBody->relativeTransform);
            mBody->useRelativeTransform = 1;
            MdtBodyUpdateCOMFromTransform(mBody);
        }
    } else {
        MdtBodySetCenterOfMassRelativeTransform(mBody, mp->mCOMTransform.m);
    }
    updateCenterOfRotation();
}

// Mac 0x46a0a4, Win 0x71d9e0
void VxPart::computeMassProperties()
{
    if (getCollisionGeometryCount() == 0)
        return;
    if (mUserMass && mUserInertia && mUserCOM)
        return;
    VincModelID* models = (VincModelID*)vxAlloc(getCollisionGeometryCount() * sizeof(VincModelID));
    int count = 0;
    for (int i = 0; i < getCollisionGeometryCount(); ++i)
        if (getCollisionGeometry(i)->mFlags & kMassFlagMask)
            models[count++] = getCollisionGeometry(i)->mModel;
    if (count == 0) {
        vxFree(models);
        return;
    }
    VxTransform relTM;
    VxReal inertia[3][3];
    VxReal volume, mass;
    VincModelsGetMassProperties(models, count, relTM.m, inertia, &volume, &mass);
    if (volume == 0.0) {
        vxFree(models);
        return;
    }
    if (mMergeInfo)
        mMergeInfo->subtractMassProperties(mMassProperties, &IdentityTransform(), false);
    VxMassProperties* mp = mMassProperties;
    bool done = false;
    if (!mUserInertia) {
        if (!mUserMass) {
            mp->setInertiaTensor(inertia);
            mp->setMass(mass);
            mp->scaleInertiaTensor(mass);
            mp->makeInertiaTensorValid();
            done = true;
        }
    } else if (!mUserMass) {
        mp->scaleInertiaTensor(mass / mp->mMass);
        mp->setMass(mass);
        done = true;
    }
    if (!done && !mUserInertia) {
        mp->setInertiaTensor(inertia);
        mp->scaleInertiaTensor(mp->mMass);
        mp->makeInertiaTensorValid();
    }
    if (!mUserCOM) {
        mp->setCOMTransform(relTM);
        updateCenterOfRotation();
    }
    if (mMergeInfo == 0)
        applyMassProperties(mp);
    else
        mMergeInfo->addMassProperties(mp, &IdentityTransform(), true);
    vxFree(models);
}

// Mac 0x46a4e8, Win 0x714600
void VxPart::updateMassPropertiesFromGeometry()
{
    if (mControl == kControlDynamic && getCollisionGeometryCount() > 0) {
        mUserCOM = false;
        mUserInertia = false;
        mUserMass = false;
        computeMassProperties();
    }
}

// ---- control / state -------------------------------------------------------------------------

// Mac 0x46a9aa, Win 0x71dd60
void VxPart::setControl(int control)
{
    if (mControl == control)
        return;
    if (mUniverse && mUniverse->mStepping)
        VxWarning(0, "VxPart::setControl: cannot be called while stepping (check intersect subscribers).\n");
    if (!isMergeRoot()) {
        if (!ShouldSeparate(mMergeInfo)) {
            VxWarning(0, "VxPart::setControl: part must be separated from its parent prior to this call.\n");
            return;
        }
        separateMergedParts();
    }
    resetForces();
    const bool frozen = mInternalControl == kControlFrozen;
    if (frozen)
        freeze(false);
    mControl = control;
    setInternalControl(control);
    if (control >= 0) {
        if (control < kControlDynamic) {
            MdtBodySetAnimated(mBody);
            MdtBodyDisable(mBody);
        } else if (control == kControlDynamic) {
            MdtBodySetDynamic(mBody);
            VxMassProperties* mp = mMassProperties;
            if (mp->mMass == 0.0) {
                mp->setInertiaTensor(IdentityInertia());
                mMassProperties->setMass(1.0);
            }
            if (getCollisionGeometryCount() < 1)
                applyMassProperties(mMassProperties);
            else
                computeMassProperties();
            if (!frozen) {
                if (mInternalControl != kControlDynamic) {
                    if (frozen)
                        freeze(true);
                    return;
                }
                enableDynamics(true);
            }
        }
    }
    if (mInternalControl == kControlDynamic && sSweepByDefault)
        setSweepEnabled(true);
    if (frozen)
        freeze(true);
}

// Mac 0x4699e0, Win 0x71c9e0
void VxPart::freeze(bool b)
{
    if (mUniverse && mUniverse->mStepping)
        VxWarning(0, "VxPart::freeze: cannot be called while stepping (check intersect subscribers).\n");
    if (!isMergeRoot() && ShouldSeparate(mMergeInfo))
        separateMergedParts();
    if ((mInternalControl == kControlFrozen) == b)
        return;
    if (getMergeParent() != 0) {
        VxWarning(0, "VxPart::freeze: part with name \"%s\" must be separated from its parent prior to this call.\n",
                  getName());
        return;
    }
    VxCollisionSensor::freeze(b);
    if (mUniverse) {
        if (!b) {
            MdtBodyUnfreeze(mBody);
            if (mInternalControl == kControlDynamic)
                enableDynamics(true);
        } else {
            MdtBodyFreeze(mBody);
        }
    }
}

// Mac 0x468c72, Win 0x71c7c0
void VxPart::enableDynamics(bool enable)
{
    if (!isMergeRoot()) {
        if (!ShouldSeparate(mMergeInfo)) {
            getMergeRoot()->enableDynamics(enable);
            return;
        }
        separateMergedParts();
    }
    const int control = mInternalControl;
    if (control == kControlDynamic) {
        if (!enable) {
            if (mUniverse)
                updateCollisionGeometryStates();
            MdtBodyDisable(mBody);
        } else if (!MdtBodyIsEnabled(mBody)) {
            MdtBodyEnable(mBody);
        }
    } else if (control == kControlAnimated || control == kControlStatic) {
        MdtBodyWakeUp(mBody);
    }
}

// Mac 0x4688ee, Win 0x71b9b0
bool VxPart::isEnabled() const
{
    bool enabled = false;
    if (mInternalControl == kControlDynamic)
        enabled = MdtBodyIsEnabled(mBody) != 0;
    return enabled;
}

// Mac 0x4688aa
int VxPart::getEnabledTime() const
{
    return MdtBodyGetEnabledTime(mBody);
}

// Mac 0x4688c4, Win 0x719ec0
void VxPart::setEnabledTime(int t)
{
    if (mInternalControl == kControlDynamic)
        MdtBodySetEnabledTime(mBody, t);
}

// ---- transform -------------------------------------------------------------------------------

// Mac 0x46931c, Win 0x71cbb0
void VxPart::setTransform(const VxTransform& tm)
{
    if (mControl == kControlDynamic) {
        if (SameRelative(&mTransformPtr->m[0][0], &tm.m[0][0])) {
            mUserTransform.assign(tm);
            return;
        }
    } else if (SameExact(&mUserTransform.m[0][0], &tm.m[0][0])) {
        return;
    }
    if (mInternalControl == kControlFrozen) {
        VxCollisionSensor::freeze(false);
        MdtBodySetTransform(mBody, tm.m);
        VxCollisionSensor::freeze(true);
    } else {
        MdtBodySetTransform(mBody, tm.m);
        updateCollisionGeometryTransforms();
        enableDynamics(true);
        setEnabledTime(0);
    }
    mUserTransform.assign(tm);
}

// Mac 0x468b76, Win 0x71bf60
void VxPart::moveTo(const VxTransform& tm, VxReal dt)
{
    if (dt == 0.0) {
        if (mInternalControl != kControlFrozen) {
            setTransform(tm);
            return;
        }
    } else if (mInternalControl != kControlFrozen) {
        if (mUniverse) {
            MdtBodyMoveTo(mBody, tm.m, dt);
            updateCollisionGeometryTransforms();
            return;
        }
        setTransform(tm);
        return;
    }
    VxInfo(0, "VxPart::setTransform(tm, dt): This method should not be called if the part is frozen. \n. Please think to use setTransform(tm) in that case.\n");
    setTransform(tm);
}

// Mac 0x4686e8, Win 0x71c710
void VxPart::updateFromMergeParent()
{
    VxPartMergeInfo* info = mMergeInfo;
    if (info == 0 || info->mParent == 0)
        return;
    info->computeTransform(mUserTransform);
    MdtBodySetTransform(mBody, mUserTransform.m);
    updateCollisionGeometryTransforms();
    VxPart* root = info->mRoot->mPart;
    setAngularVelocity(*(const VxVector3*)root->getAngularVelocityPtr());
    MdtBodyGetVelocityAtPoint(root->mBody, mUserTransform.m[3], MdtBodyGetLinearVelocityPtr(mBody));
}

// ---- universe --------------------------------------------------------------------------------

// Mac 0x468ff2, Win 0x71c890
void VxPart::removeFromUniverse()
{
    if (mUniverse == 0)
        return;
    if (mUniverse->mStepping)
        VxWarning(0, "VxPart cannot be remove from universe while stepping (check intersect subscribers).\n");
    separateMergedParts();
    for (VxConstraint** c = mConstraints.begin(); c != mConstraints.end(); ++c)
        if ((*c)->mEnabled)
            (*c)->suspend();
    MdtBodyWakeUp(mBody);
    MdtBodySetUserData(mBody, 0);
    VxCollisionSensor::removeFromUniverse();
    MdtBodyDisable(mBody);
    MdtWorldRemoveBody(mBody);
    MdtBodySetUserData(mBody, this);
}

// Mac 0x469bc2, Win 0x71c940
void VxPart::addToUniverse(VxUniverse* u)
{
    VxCollisionSensor::addToUniverse(u);
    enableDynamics(true);
    setEnabledTime(0);
    u->insertBody(mBody);
    if (mInternalControl != kControlDynamic)
        MdtBodyDisable(mBody);
    int g = getGrouping();
    if (0 < g)
        u->getGrouping(g);
    for (VxConstraint** c = mConstraints.begin(); c != mConstraints.end(); ++c)
        if ((*c)->mEnabled)
            (*c)->setEnabled(true);
}

// ---- collision geometries --------------------------------------------------------------------

// Mac 0x46a5b4, Win 0x71dc50
void VxPart::addCollisionGeometry(VxCollisionGeometry* cg)
{
    const bool added = cg->mEntity != this;   // else the sensor returns at once
    VxCollisionSensor::addCollisionGeometry(cg);
    ConnectModel(cg->mModel, mBody);
    if (mControl == kControlDynamic && (cg->mFlags & kMassFlagMask))
        computeMassProperties();
    VxUniverse* u = mUniverse;
    // When `cg` was just added, the universe is set and `cg` is enabled, the sensor has already
    // disabled its pairs with every enabled earlier geometry; this loop would disable the same
    // pairs again (no state changes) - a second O(n^2) pass for parts of thousands of geometries.
    // Only its warnings remain: those of the geometries that are not enabled. Otherwise the loop
    // runs as before.
    if (u && added && cg->isEnabled()) {
        // (a sensor in pair group mode whose geometries were all enabled: none to warn about)
        if (!pairGroupAllEnabled())
        for (int i = 0; i < getCollisionGeometryCount() - 1; ++i) {
            VxCollisionGeometry* g = getCollisionGeometry(i);
            if (!g->isEnabled())
                u->disablePairIntersect(g, cg);
        }
    } else
    if (u)
        for (int i = 0; i < getCollisionGeometryCount() - 1; ++i)
            u->disablePairIntersect(getCollisionGeometry(i), cg);
    VincModelSetCenterOfRotation(cg->mModel, mMassProperties->mCOMTransform.m[3]);
}

// Mac 0x46a52a, Win 0x71dcf0
bool VxPart::removeCollisionGeometry(int index)
{
    VxCollisionGeometry* cg = getCollisionGeometry(index);
    if (cg == 0 || !VxCollisionSensor::removeCollisionGeometry(index))
        return false;
    ConnectModel(cg->mModel, 0);
    if (mControl == kControlDynamic && (cg->mFlags & kMassFlagMask))
        computeMassProperties();
    return true;
}

// Mac 0x46a7cc, Win 0x71e720
VxCollisionGeometry* VxPart::addGeometryByMass(VxGeometry* geometry, VxMaterial* material,
                                               const VxTransform* relative, VxReal mass)
{
    if (mControl != kControlDynamic)
        return addGeometry(geometry, material, relative, 1.0);
    if (mUserMass)
        setMass(getMass() + mass);
    VxTransform relTM;
    VxCollisionGeometry* cg = new VxCollisionGeometry(geometry);
    VxReal inertia[3][3];
    VxReal volume, m;
    VincModelsGetMassProperties(&cg->mModel, 1, relTM.m, inertia, &volume, &m);
    VxReal density;
#if defined(_WIN32)
    // Win also refuses an infinite volume (_finite, Win 0x6e4348)
    if (volume <= 0.0)
        density = 1.0;
    else
        density = _finite(volume) ? mass / volume : 1.0;
#else
    if (0.0 < volume)
        density = mass / volume;
    else
        density = 1.0;
#endif
    cg->setDensity(density);
    cg->setMaterial(material);
    addCollisionGeometry(cg);
    return cg;
}

// ---- velocities / forces ---------------------------------------------------------------------

// Mac 0x46886e, Win 0x71ba00
void VxPart::setLinearVelocity(const VxVector3& v)
{
    MdtBodySetLinearVelocity(mBody, v[0], v[1], v[2]);
}

// Mac 0x4687f8, Win 0x71ba60
void VxPart::setAngularVelocity(const VxVector3& w)
{
    MdtBodySetAngularVelocity(mBody, w[0], w[1], w[2]);
}

// Mac 0x468834, Win 0x71ba30
void VxPart::setLinearVelocity(VxReal x, VxReal y, VxReal z)
{
    MdtBodySetLinearVelocity(mBody, x, y, z);
}

// Mac 0x4687be, Win 0x71ba90
void VxPart::setAngularVelocity(VxReal x, VxReal y, VxReal z)
{
    MdtBodySetAngularVelocity(mBody, x, y, z);
}

// Mac 0x467f6e, Win 0x71bac0
void VxPart::getLinearVelocity(VxReal& x, VxReal& y, VxReal& z) const
{
    VxVector3 v;
    getLinearVelocity(v);
    x = v[0];
    y = v[1];
    z = v[2];
}

// Mac 0x467fb0, Win 0x71bb00
void VxPart::getAngularVelocity(VxReal& x, VxReal& y, VxReal& z) const
{
    VxVector3 w;
    getAngularVelocity(w);
    x = w[0];
    y = w[1];
    z = w[2];
}

// Mac 0x46879e, Win 0x71bb40
void VxPart::getLinearVelocity(VxVector3& v) const
{
    MdtBodyGetLinearVelocity(mBody, &v[0]);
}

// Mac 0x46877e, Win 0x71bb60
void VxPart::getAngularVelocity(VxVector3& w) const
{
    MdtBodyGetAngularVelocity(mBody, &w[0]);
}

// Mac 0x468b2a, Win 0x71bb80
void VxPart::getLinearAcceleration(VxVector3& a) const
{
    if (mInternalControl == kControlDynamic) {
        const MeReal* acc = mBody->keaBody.m2c4; // MdtBody +0x404 / Win +0x408 (Kea acceleration)
        a[0] = acc[0];
        a[1] = acc[1];
        a[2] = acc[2];
    }
}

// Mac 0x46858c
void VxPart::getAngularAcceleration(VxVector3& a) const
{
    MdtBodyGetAngularAcceleration(mBody->self, &a[0]);
}

// Mac 0x468c16
void VxPart::getFrameLinearAcceleration(VxVector3& a) const
{
    if (!isMergeRoot())
        MdtBodyGetAccelerationAtPoint(mBody->self, mTransformPtr->m[3], &a[0]);
    else
        MdtBodyGetLinearAcceleration(mBody, &a[0]);
}

// Mac 0x4685b2
void VxPart::getVelocityAtPoint(const VxVector3& p, VxVector3& v) const
{
    MdtBodyGetVelocityAtPoint(mBody, &p[0], &v[0]);
}

// Mac 0x4686ce
const VxReal* VxPart::getLinearVelocityPtr() const
{
    return MdtBodyGetLinearVelocityPtr(mBody);
}

// Mac 0x4686b4
const VxReal* VxPart::getAngularVelocityPtr() const
{
    return MdtBodyGetAngularVelocityPtr(mBody);
}

// Mac 0x4689c0, Win 0x521370
void VxPart::resetForces()
{
    MdtBodyResetForces(mBody);
}

// Mac 0x469d20: a merged part pushes its root at its own position
void VxPart::addForce(const VxVector3& f)
{
    if (mInternalControl != kControlDynamic)
        return;
    if (!isMergeRoot()) {
        if (!ShouldSeparate(mMergeInfo)) {
            getMergeRoot()->addForceAtPosition(f, *(const VxVector3*)mTransformPtr->m[3]);
            return;
        }
        separateMergedParts();
    }
    enableDynamics(true);
    MdtBodyAddForce(mBody, f[0], f[1], f[2]);
}

// Mac 0x469248
void VxPart::addForceAtPosition(const VxVector3& f, const VxVector3& p)
{
    if (mInternalControl != kControlDynamic)
        return;
    if (!isMergeRoot()) {
        if (!ShouldSeparate(mMergeInfo)) {
            getMergeRoot()->addForceAtPosition(f, p);
            return;
        }
        separateMergedParts();
    }
    enableDynamics(true);
    MdtBodyAddForceAtPosition(mBody, f[0], f[1], f[2], p[0], p[1], p[2]);
}

// Mac 0x469c76
void VxPart::addTorque(const VxVector3& t)
{
    if (mInternalControl != kControlDynamic)
        return;
    if (!isMergeRoot()) {
        if (!ShouldSeparate(mMergeInfo)) {
            getMergeRoot()->addTorque(t);
            return;
        }
        separateMergedParts();
    }
    enableDynamics(true);
    MdtBodyAddTorque(mBody, t[0], t[1], t[2]);
}

// Mac 0x469194, Win 0x71d3c0
void VxPart::addImpulse(const VxVector3& i)
{
    if (mInternalControl != kControlDynamic)
        return;
    if (!isMergeRoot()) {
        if (!ShouldSeparate(mMergeInfo)) {
            getMergeRoot()->addImpulseAtPosition(i, *(const VxVector3*)mTransformPtr->m[3]);
            return;
        }
        separateMergedParts();
    }
    enableDynamics(true);
    MdtBodyAddImpulse(mBody, i[0], i[1], i[2]);
}

// Mac 0x4690c0, Win 0x71cfe0
void VxPart::addImpulseAtPosition(const VxVector3& i, const VxVector3& p)
{
    if (mInternalControl != kControlDynamic)
        return;
    if (!isMergeRoot()) {
        if (!ShouldSeparate(mMergeInfo)) {
            getMergeRoot()->addImpulseAtPosition(i, p);
            return;
        }
        separateMergedParts();
    }
    enableDynamics(true);
    MdtBodyAddImpulseAtPosition(mBody, i[0], i[1], i[2], p[0], p[1], p[2]);
}

// Mac 0x4685f4, Win 0x71c020
VxReal VxPart::getLinearVelocityDamping() const
{
    return MdtBodyGetLinearVelocityDamping(mBody);
}

// Mac 0x4685da, Win 0x71c030
VxReal VxPart::getAngularVelocityDamping() const
{
    return MdtBodyGetAngularVelocityDamping(mBody);
}

// Mac 0x46860e, Win 0x71bfe0
void VxPart::setLinearVelocityDamping(VxReal d)
{
    MdtBodySetLinearVelocityDamping(mBody, d);
}

// Mac 0x468b06, Win 0x71c000
void VxPart::setAngularVelocityDamping(VxReal d)
{
    MdtBodySetAngularVelocityDamping(mBody, d);
}

// Mac 0x46869a
VxReal VxPart::getAutoDisableVelocityThreshold() const
{
    return MdtBodyGetAutoDisableVelocityThreshold(mBody);
}

// Mac 0x468666
VxReal VxPart::getAutoDisableAngularVelocityThreshold() const
{
    return MdtBodyGetAutoDisableAngularVelocityThreshold(mBody);
}

// Mac 0x468680
VxReal VxPart::getAutoDisableAccelerationThreshold() const
{
    return MdtBodyGetAutoDisableAccelerationThreshold(mBody);
}

// Mac 0x46864c
VxReal VxPart::getAutoDisableAngularAccelerationThreshold() const
{
    return MdtBodyGetAutoDisableAngularAccelerationThreshold(mBody);
}

// Mac 0x468632
int VxPart::getAutoDisableAliveWindow() const
{
    return MdtBodyGetAutoDisableAliveWindow(mBody);
}

// Mac 0x468a90
void VxPart::getDefaultAutoDisableParameters(VxReal& velocity, VxReal& acceleration,
                                             VxReal& angularVelocity, VxReal& angularAcceleration,
                                             int& aliveWindow)
{
    MdtAutoDisableParams p;
    MdtBodyGetDefaultAutoDisableParams(&p);
    velocity = p.velocityThreshold;
    acceleration = p.accelerationThreshold;
    angularVelocity = p.angularVelocityThreshold;
    angularAcceleration = p.angularAccelerationThreshold;
    aliveWindow = p.aliveWindow;
}

// Mac 0x4680d4, Win 0x731ac0
int VxPart::getGrouping() const
{
    return MdtBodyGetGrouping(mBody);
}

// Mac 0x468128, Win 0x71c530
void VxPart::setGrouping(int grouping)
{
    MdtBodySetGrouping(mBody, grouping);
    if (mUniverse)
        mUniverse->getGrouping(grouping);
}

// Mac 0x4681ca
void VxPart::contactCallback(MdtBody* body, MdtConstraint* contact)
{
    ContactHandler* h = (ContactHandler*)body->m54c;
    VxDynamicsContactInput input(contact);
    h->handleContact((VxPart*)body->userData, &input);
}

// Mac 0x46801e
VxPart::ContactHandler* VxPart::getContactHandler() const
{
    return (ContactHandler*)mBody->m54c;
}

// Mac 0x468168
void VxPart::setContactHandler(ContactHandler* h)
{
    ContactHandler* old = getContactHandler();
    if (old)
        delete old;
    MdtBodySetM548(mBody, h ? &VxPart::contactCallback : 0, h);
}

// ---- constraints -----------------------------------------------------------------------------

// Mac 0x46ab92, Win 0x71e8c0
void VxPart::addConstraint(VxConstraint* c)
{
    bool found = false;
    for (VxConstraint** i = mConstraints.begin(); i != mConstraints.end(); ++i)
        if (*i == c)
            found = true;
    if (!found)
        mConstraints.push_back(c);
    if (mInternalControl == kControlDynamic)
        enableDynamics(true);
}

// Mac 0x469afa
void VxPart::removeConstraint(VxConstraint* c)
{
    VxConstraint** it = mConstraints.begin();
    while (it != mConstraints.end() && *it != c)
        ++it;
    if (it != mConstraints.end() && 0 < c->mParts.size()) {
        int uses = 0;
        for (int i = 0; i < c->mParts.size(); ++i)
            if (c->mParts.begin()[i] == this && ++uses > 1)
                break;
        if (uses == 1)
            mConstraints.remove(c);
    }
    enableDynamics(true);
}

// ---- merging ---------------------------------------------------------------------------------

// Mac 0x468032, Win 0x71c690
VxPart* VxPart::getMergeParent() const
{
    if (mMergeInfo == 0 || mMergeInfo->mParent == 0)
        return 0;
    return mMergeInfo->mParent->mPart;
}

// Mac 0x468068, Win 0x71c6c0
VxPart* VxPart::getMergeRoot()
{
    if (mMergeInfo)
        return mMergeInfo->mRoot->mPart;
    return this;
}

// Mac 0x468080
bool VxPart::isMergeRoot() const
{
    return mMergeInfo == 0 || mMergeInfo->mRoot == mMergeInfo;
}

// Mac 0x468054
const VxTransform& VxPart::getMergeRootTransform() const
{
    return mMergeInfo->mRootTransform;
}

// Mac 0x4680a0, Win 0x71c700
MdtBody** VxPart::getMergeRootBodyPtr() const
{
    return &mBody->self;
}

// Mac 0x468f9c, Win 0x71b920
void VxPart::separateMergedParts()
{
    if (mMergeInfo == 0)
        return;
    setMergeParent(0);
    while (mMergeInfo->mFirstChild != 0)
        mMergeInfo->mFirstChild->mPart->setMergeParent(0);
}

// Mac 0x468d3e, Win 0x71d180
bool VxPart::setMergeParent(VxPart* parent)
{
    if (mUniverse && mUniverse->mStepping)
        VxWarning(0, "VxPart::setMergeParent: cannot be called while stepping (check intersect subscribers).\n");
    if (parent == this)
        parent = 0;
    VxPart* current = getMergeParent();
    if (current == parent)
        return false;
    if (parent != 0 && parent->mInternalControl == kControlFrozen) {
        const char* pa = parent->mAssembly ? parent->mAssembly->getName() : "";
        const char* ta = mAssembly ? mAssembly->getName() : "";
        VxWarning(0, "VxPart::setMergeParent: can't merge part \"%s\" from assembly \"%s\" to parent part \"%s\" from assembly \"%s\" because it is frozen. Cannot merge to a frozen parent part.\n",
                  getName(), ta, parent->getName(), pa);
        return false;
    }
    if (mInternalControl != kControlDynamic) {
        // NOTE: the original reads parent->mAssembly / getName here even when parent == 0
        const char* pa = parent->mAssembly ? parent->mAssembly->getName() : "";
        const char* pn = parent->getName();
        const char* ta = mAssembly ? mAssembly->getName() : "";
        VxWarning(0, "VxPart::setMergeParent: can't merge non-dynamically controlled part \"%s\" from assembly \"%s\" to parent part \"%s\" from assembly \"%s\". Only dynamically controlled part can be merged.\n",
                  getName(), ta, pn, pa);
        return false;
    }
    if (mMergeInfo == 0) {
        enableDynamics(true);
        mMergeInfo = new VxPartMergeInfo(this);
    } else if (current == 0) {
        enableDynamics(true);
    }
    VxPartMergeInfo* parentInfo = 0;
    if (parent != 0) {
        parentInfo = parent->mMergeInfo;
        if (parentInfo == 0) {
            parentInfo = new VxPartMergeInfo(parent);
            parent->mMergeInfo = parentInfo;
        }
    }
    bool result = mMergeInfo->setParent(parentInfo);
    if (mAssembly)
        mAssembly->setMergeChanged();
    return result;
}

} // namespace Vx
