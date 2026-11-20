// Vortex 5.0.0 build 800 "vortexldd41" - VxStateEntity (restored).
// Mac TU 521 0x47d60a-0x47e3aa (static initialiser 0x47e345: <iostream> only), Win
// 0x7f6840-0x7f7370. The two merge helpers (Mac 0x47d66e / 0x47d62b, register arguments; Win
// 0x7f6940 / 0x7f69a0) take a part out of its merge tree before it is moved and put it back
// afterwards.
#include "Vx/VxStateEntity.h"
#include "Vx/VxEntity.h"
#include "Vx/VxPart.h"
#include "Vx/VxAssemblyBase.h"
#include "Vx/VxMessage.h"
#include <iostream>

namespace Vx {

namespace {

// Mac 0x47d66e, Win 0x7f6940: remembers the merge parent, unmerges the part and its direct
// merge children (collected into `children`) (name inferred)
void detachFromMergeTree(VxPart* part, VxPart** oldParent, VxArray<VxPart*>& children)
{
    *oldParent = part->getMergeParent();
    part->setMergeParent(0);
    if (part->mMergeInfo != 0) {
        // NOTE: the original's collector (Mac 0x46bd58) yields the children's parts;
        // VxPartMergeInfo::collectChildren here yields their merge infos, hence ->mPart.
        VxArray<VxPartMergeInfo*> infos;
        part->mMergeInfo->collectChildren(infos);
        for (VxPartMergeInfo** i = infos.begin(); i != infos.end(); ++i)
            children.push_back((*i)->mPart);
        for (VxPart** i = children.begin(); i != children.end(); ++i)
            (*i)->setMergeParent(0);
    }
}

// Mac 0x47d62b, Win 0x7f69a0: merges the children back into the part, the part into parent
// (name inferred)
void reattachToMergeTree(VxPart* part, VxPart* parent, VxArray<VxPart*>& children)
{
    for (VxPart** i = children.begin(); i != children.end(); ++i)
        (*i)->setMergeParent(part);
    part->setMergeParent(parent);
}

bool isPartClass(VxEntity* e)
{
    return e->isOfClassType(VxPart::getStaticClassType());
}

} // namespace

// Mac 0x47d6d8, Win 0x7f6840
void VxStateEntity::save(VxEntity* e)
{
    setBaseObject(e);
    e->getTransform(mTransform);
    mControl = e->mControl;
    if (e->isPart() && (e->mInternalControl == VxEntity::kControlDynamic || e->mInternalControl == VxEntity::kControlStatic)) {
        VxPart* p = static_cast<VxPart*>(e);
        p->getLinearVelocity(mLinearVelocity);
        p->getAngularVelocity(mAngularVelocity);
        mEnabled = p->isEnabled();
    } else {
        mLinearVelocity = VxVector3(0, 0, 0);
        mAngularVelocity = VxVector3(0, 0, 0);
        mEnabled = true;
    }
    mFrozen = e->mInternalControl == VxEntity::kControlFrozen;
    if (isPartClass(e))
        mMergeParent = static_cast<VxPart*>(e)->getMergeParent();
    else
        mMergeParent = 0;
}

// Mac 0x47d824, Win 0x7f7180: the name of the entity's assembly is kept as well
void VxStateEntity::setBaseObject(VxBase* object)
{
    VxStateBase::setBaseObject(object);
    mAssemblyName.clear();
    if (object != 0) {
        VxEntity* e = dynamic_cast<VxEntity*>(object);
        if (e != 0 && e->mAssembly != 0)
            mAssemblyName = static_cast<VxBase*>(e->mAssembly)->getName();
    }
}

// C2 Mac 0x47d8ec (C1 0x47da8e), Win 0x7f7070
VxStateEntity::VxStateEntity(const VxStateEntity& o)
    : VxStateBase(o), mTransform(o.mTransform), mLinearVelocity(o.mLinearVelocity),
      mAngularVelocity(o.mAngularVelocity), mControl(o.mControl), mFrozen(o.mFrozen),
      mEnabled(o.mEnabled), mAssemblyName(o.mAssemblyName), mMergeParent(o.mMergeParent)
{
}

// C2 Mac 0x47daa8 (C1 0x47dbc4), Win 0x7f6f10. NOTE: the original only sets column 3 of
// mTransform (0, 0, 0, 1); save() overwrites the whole transform right away, the restored
// member starts as the identity.
VxStateEntity::VxStateEntity(VxEntity* e)
    : VxStateBase(e), mTransform(), mLinearVelocity(0, 0, 0), mAngularVelocity(0, 0, 0),
      mControl(0), mFrozen(false), mEnabled(true), mAssemblyName(), mMergeParent(0)
{
    save(e);
}

// Mac 0x47dbde, Win 0x7f69e0
void VxStateEntity::save()
{
    if (mObject != 0)
        save(static_cast<VxEntity*>(mObject));
}

// Mac 0x47dbfc, Win 0x7f6af0: by name in the assembly of that name, else in all entities
void VxStateEntity::reconnectBaseObject()
{
    if (mObject == 0) {
        VxEntity* e = 0;
        VxAssemblyBase* a = VxAssemblyBase::findFromName(mAssemblyName.c_str());
        if (a != 0)
            e = (VxEntity*)a->findPart(mObjectName.c_str());
        if (e == 0) {
            e = VxEntity::findFromName(mObjectName.c_str());
            if (e == 0)
                VxWarning(0, "VxStateEntity::reconnectBaseObject : cannot find entity named '%s'\n", mObjectName.c_str());
        }
        setBaseObject(e);
    }
    VxStateBase::reconnectBaseObject();
}

// Mac 0x47dc90, Win 0x7f69f0: the saved state moved by t (row vectors: tm * t, v * R(t))
void VxStateEntity::transform(const VxTransform& t)
{
    mTransform.postMultiply(t);
    VxReal x = mLinearVelocity[0], y = mLinearVelocity[1], z = mLinearVelocity[2];
    mLinearVelocity = VxVector3(x * t.m[0][0] + y * t.m[1][0] + z * t.m[2][0],
                                x * t.m[0][1] + y * t.m[1][1] + z * t.m[2][1],
                                x * t.m[0][2] + y * t.m[1][2] + z * t.m[2][2]);
    x = mAngularVelocity[0];
    y = mAngularVelocity[1];
    z = mAngularVelocity[2];
    mAngularVelocity = VxVector3(x * t.m[0][0] + y * t.m[1][0] + z * t.m[2][0],
                                 x * t.m[0][1] + y * t.m[1][1] + z * t.m[2][1],
                                 x * t.m[0][2] + y * t.m[1][2] + z * t.m[2][2]);
    VxStateBase::transform(t);
}

// Mac 0x47de18, Win 0x7f6d00. NOTE: the part goes back to its current merge parent (the full
// restore uses the saved one).
void VxStateEntity::restoreKinematic(VxEntity* e, bool notify)
{
    if (e == 0) {
        VxWarning(0, "VxStateEntity::restore : entity is NULL\n");
        return;
    }
    if (e->mInternalControl == VxEntity::kControlFrozen)
        return;
    VxArray<VxPart*> children;
    VxPart* oldParent = 0;
    if (isPartClass(e))
        detachFromMergeTree(static_cast<VxPart*>(e), &oldParent, children);
    e->setTransform(mTransform);
    bool update = true;
    if (e->mControl == VxEntity::kControlDynamic) {
        VxPart* p = static_cast<VxPart*>(e);
        p->resetForces();
        p->setLinearVelocity(mLinearVelocity);
        p->setAngularVelocity(mAngularVelocity);
        p->enableDynamics(mEnabled);
        update = notify;
    }
    if (update && e->mInternalControl != VxEntity::kControlAnimated)
        e->updateNode();
    if (isPartClass(e))
        reattachToMergeTree(static_cast<VxPart*>(e), oldParent, children);
}

// Mac 0x47dfa8, Win 0x7f7010
void VxStateEntity::restoreKinematic(bool notify)
{
    if (mObject != 0)
        restoreKinematic(static_cast<VxEntity*>(mObject), notify);
}

// Mac 0x47dfd2, Win 0x7f6b90
void VxStateEntity::restore(VxEntity* e, bool notify)
{
    if (e == 0) {
        VxWarning(0, "VxStateEntity::restore : entity is NULL\n");
        return;
    }
    VxArray<VxPart*> children;
    VxPart* oldParent = 0;
    if (isPartClass(e))
        detachFromMergeTree(static_cast<VxPart*>(e), &oldParent, children);
    e->freeze(false);
    e->setTransform(mTransform);
    e->setControl(mControl);
    bool update = true;
    if (e->mControl == VxEntity::kControlDynamic) {
        VxPart* p = static_cast<VxPart*>(e);
        p->resetForces();
        p->setLinearVelocity(mLinearVelocity);
        p->setAngularVelocity(mAngularVelocity);
        p->enableDynamics(mEnabled);
        update = notify;
    }
    if (update)
        e->updateNode();
    if (mFrozen)
        e->freeze(true);
    if (isPartClass(e))
        reattachToMergeTree(static_cast<VxPart*>(e), mMergeParent, children);
}

// Mac 0x47e176, Win 0x7f7030
void VxStateEntity::restoreMode(int mode, bool notify)
{
    if (mObject == 0)
        return;
    if (mode == 0)
        restore(static_cast<VxEntity*>(mObject), notify);
    else if (mode == 1)
        restoreKinematic(static_cast<VxEntity*>(mObject), notify);
}

// Mac 0x47e1c8, Win 0x7f6ff0
void VxStateEntity::restore(bool notify)
{
    if (mObject != 0)
        restore(static_cast<VxEntity*>(mObject), notify);
}

// Mac 0x47e1f2, Win 0x7f7310
VxStateBase* VxStateEntity::clone() const
{
    return new VxStateEntity(*this);
}

// Mac D1 0x47e23a / D0 0x47e2bc, Win 0x7f71f0 / 0x7f72b0
VxStateEntity::~VxStateEntity()
{
}

} // namespace Vx
