// Vortex 5.0.0 build 800 "vortexldd41" - VxAssemblyBase (restored).
// Mac TU 455 (0x425346-0x42e904; file guess "(anonymous namespace)_FirstDynamicPartFinder.cpp",
// static initialiser 0x42e89f: <iostream> only), Win 0x725960-0x729b80. The TU also holds the
// anonymous AcceptIntersectPair / FirstDynamicPartFinder, VxAssemblyBase::UpdateSubscriber's and
// ExposedFields' out-of-line members and InternalConnection's destructor (Vx/
// VxCompositeController.h). The std::find / for_each / vector instances (Mac 0x42b3de-0x42c8ac,
// 0x42e738) are library code; the pair collection of disablePairIntersect / enablePairIntersect
// (VxAssemblyBase*) is the AssemblyPairs helper below (Mac 0x42e212 with 0x42c8ac / 0x42cc06 /
// 0x42d564, the latter two inlined several levels deep).
#include "Vx/VxAssemblyBase.h"
#include "Vx/VxUniverse.h"
#include "Vx/VxEntity.h"
#include "Vx/VxPart.h"
#include "Vx/VxConstraint.h"
#include "Vx/VxCollisionGeometry.h"
#include "Vx/VxIntersect.h"
#include "Vx/VxVisitor.h"
#include "Vx/VxState.h"
#include "Vx/VxTransform.h"
#include "Vx/VxMessage.h"
#include "Vx/VxInputOutput.h"
#include "VxData/FieldBase.h"
#include "VxData/Container.h"
#include "Me/MeMath.h"
#include "UniverseConnector.h"
#include <string>
#include <iostream>
#include <limits>

namespace Vx {

VxBase::NameMap* VxAssemblyBase::sNameMap = 0;  // Mac 0x7791a4
bool VxAssemblyBase::sExposeFields = true;      // Mac 0x7791a0 (initialised data: 1)

// =============================================================================================
// UpdateSubscriber / ExposedFields / InternalConnection pieces
// =============================================================================================

// Mac 0x425366: 0x13a
int VxAssemblyBase::UpdateSubscriber::getStaticClassType()
{
    return 0x13a;
}

// Mac 0x425370, Win 0x725fc0
bool VxAssemblyBase::UpdateSubscriber::isOfClassType(int type) const
{
    return getStaticClassType() == type;
}

// Mac 0x425386, Win 0x725af0: through slot 1
bool VxAssemblyBase::UpdateSubscriber::isOfType(int type) const
{
    return isOfClassType(type);
}

// C2 Mac 0x428aea (C1 0x428ba4), Win 0x725960
VxAssemblyBase::ExposedFields::ExposedFields(VxData::FieldBase* original, VxData::FieldBase* exposed, int kind)
{
    mOriginal = (Entry*)operator new(sizeof(Entry));
    mOriginal->field = original;
    mOriginal->kind = kind;
    mExposed = (Entry*)operator new(sizeof(Entry));
    mExposed->field = exposed;
    mExposed->kind = kind;
}

// D2 Mac 0x4289da (D1 0x428ad6, D0 0x4288d6), Win 0x728dc0: the exposed copy is deleted
VxAssemblyBase::ExposedFields::~ExposedFields()
{
    if (mExposed->field != 0)
        delete mExposed->field;
    operator delete(mOriginal);
    operator delete(mExposed);
}

// =============================================================================================
// local classes
// =============================================================================================

namespace {

// (anonymous namespace)::AcceptIntersectPair (Mac vtable 0x75d178: D1 0x42568c, D0 0x425662,
// accept 0x42611e / Win 0x728a20, clone 0x4257f8 / Win 0x728f70): results of this assembly's
// sensors only
class AcceptIntersectPair : public VxIntersectResultFilter {
public:
    explicit AcceptIntersectPair(VxAssemblyBase* a) : mAssembly(a) {}
    virtual ~AcceptIntersectPair() {}
    virtual bool accept(VxIntersectResult* result)
    {
        VxCollisionSensor* s0;
        VxCollisionSensor* s1;
        result->getCollisionSensors(s0, s1);
        if (s0->mAssembly == mAssembly)
            return true;
        return mAssembly == s1->mAssembly;
    }
    virtual VxIntersectResultFilter* clone() const { return new AcceptIntersectPair(mAssembly); }
    VxAssemblyBase* mAssembly;  // +0x04
};

// (anonymous namespace)::FirstDynamicPartFinder (Mac vtable 0x75d148: visitEntity 0x42615e /
// Win 0x728cf0, dtor 0x4274a2 / 0x425776): the first dynamic part (with mRootOnly, only one
// whose assembly has one merge root)
class FirstDynamicPartFinder : public VxVisitor {
public:
    FirstDynamicPartFinder() : mPart(0), mRootOnly(false) {}
    virtual ~FirstDynamicPartFinder() {}
    virtual int visitEntity(VxEntity* e)
    {
        if (e == 0)
            return 0;
        VxPart* p = dynamic_cast<VxPart*>(e);
        if (p == 0 || p->mInternalControl != VxEntity::kControlDynamic)
            return 0;
        if (!mRootOnly) {
            mPart = p;
            return 2;
        }
        VxAssemblyBase* a = p->mAssembly;
        if (a != 0 && a->getMergeState() == 1) {
            mPart = p;
            return 2;
        }
        return 0;
    }
    VxPart* mPart;      // +0x04
    bool mRootOnly;     // +0x08 (never set in either binary)
};

// The part ranges of two assemblies whose pairs get enabled / disabled (Mac 0x42e212: the state
// {mode, universe, ranges A, ranges B}; 0x42d564 collects A, 0x42cc06 B) (names inferred)
struct PartRange {
    VxPart** begin;
    VxPart** end;
};

class AssemblyPairs {
public:
    AssemblyPairs() : mMode(3), mUniverse(0)
    {
        mA.reserve(0x100);
        mB.reserve(0x100);
    }
    static PartRange range(VxAssemblyBase* a)
    {
        PartRange r;
        r.begin = a->mParts.begin();
        r.end = a->mParts.end();
        return r;
    }
    // Mac 0x42cc06 (recursive)
    void collectB(VxAssemblyBase* a, VxAssemblyBase* stop)
    {
        if (stop == a && mMode != 2 && mMode != 0) {
            mMode = 1;
            return;
        }
        mB.push_back(range(a));
        for (VxAssemblyBase** i = a->m44.begin(); i != a->m44.end(); ++i)
            collectB(*i, stop);
    }
    // Mac 0x42d564 (recursive)
    void collectA(VxAssemblyBase* a, VxAssemblyBase* other)
    {
        if (a == other) {
            mMode = 0;
            mB.push_back(range(other));
            for (VxAssemblyBase** i = other->m44.begin(); i != other->m44.end(); ++i)
                collectB(*i, a);
            return;
        }
        mA.push_back(range(a));
        for (VxAssemblyBase** i = a->m44.begin(); i != a->m44.end(); ++i)
            collectA(*i, other);
    }
    void apply(VxPart* p0, VxPart* p1, bool disable)
    {
        if (!disable)
            mUniverse->enablePairIntersect(p0, p1);
        else
            mUniverse->disablePairIntersect(p0, p1);
    }
    // every pair of A x B
    void applyAB(bool disable)
    {
        for (PartRange* a = mA.begin(); a != mA.end(); ++a)
            for (PartRange* b = mB.begin(); b != mB.end(); ++b)
                for (VxPart** i = a->begin; i != a->end; ++i)
                    for (VxPart** j = b->begin; j != b->end; ++j)
                        apply(*i, *j, disable);
    }
    // every pair within B (a range with itself: i <= j)
    void applyBB(bool disable)
    {
        for (PartRange* r = mB.begin(); r != mB.end(); ++r) {
            for (PartRange* s = r; s != mB.end(); ++s) {
                if (s == r) {
                    for (VxPart** i = r->begin; i != r->end; ++i)
                        for (VxPart** j = i; j != r->end; ++j)
                            apply(*i, *j, disable);
                } else {
                    for (VxPart** i = r->begin; i != r->end; ++i)
                        for (VxPart** j = s->begin; j != s->end; ++j)
                            apply(*i, *j, disable);
                }
            }
        }
    }
    // Mac 0x42e212 (Win 0x729fc0)
    void run(VxAssemblyBase* a0, VxAssemblyBase* a1, bool disable)
    {
        if (a0 == 0 || a1 == 0 || a0->mUniverse == 0 || a0->mUniverse != a1->mUniverse)
            return;
        mUniverse = a0->mUniverse;
        if (a0 == a1) {
            mMode = 2;
            mB.push_back(range(a0));
            for (VxAssemblyBase** i = a0->m44.begin(); i != a0->m44.end(); ++i)
                collectB(*i, a0);
        } else {
            mA.push_back(range(a0));
            for (VxAssemblyBase** i = a0->m44.begin(); i != a0->m44.end(); ++i)
                collectA(*i, a1);
            if (mMode == 3) {
                mB.push_back(range(a1));
                for (VxAssemblyBase** i = a1->m44.begin(); i != a1->m44.end(); ++i)
                    collectB(*i, a0);
            }
        }
        if (mMode == 2) {
            applyBB(disable);
        } else if (mMode < 3) {
            if (mMode >= 0) {
                applyAB(disable);
                applyBB(disable);
            }
        } else if (mMode == 3) {
            applyAB(disable);
        }
    }

    int mMode;                  // 3 = disjoint, 2 = same assembly, 0 / 1 = nested
    VxUniverse* mUniverse;
    VxArray<PartRange> mA;
    VxArray<PartRange> mB;
};

} // namespace

// =============================================================================================
// VxAssemblyBase
// =============================================================================================

// Mac 0x425346 (0x109), 0x425350, 0x42769a, 0x427a22; Win 0x725fa0, 0x727880
VX_IMPLEMENT_CLASS_TYPES(VxAssemblyBase, VxBaseUserData, 0x109)

// Mac 0x4293fc
VxAssemblyBase::VxAssemblyBase()
    : VxBaseUserData(), VxSubscriberListResponseHandler(), VxEntityConstraintCollection<VxAssemblyBase>(),
      mParent(0), m44(), mController(), mParts(), mConstraints(), mUniverse(0), mDisabledPairs(new VxDisabledPairs()),
      mInternalCollisionEnabled(true), m3d4(0), mSavedState(0), mNode(), mUpdateSubscribers(), mMergeChanged(false),
      mMergeState(2), mExposedFields()
{
    mController.setParent(this);
}

// D2 Mac 0x42a7fc, Win 0x7278b0 (the parts, constraints and sub-assemblies are deleted)
VxAssemblyBase::~VxAssemblyBase()
{
    if (mParent != 0)
        mParent->removeAssembly(this);
    if (mSavedState != 0)
        delete mSavedState;
    if (mUniverse != 0)
        mUniverse->removeAssembly(this);
    while (mConstraints.size() != 0) {
        VxConstraint* c = mConstraints.end()[-1];
        _removeConstraint(c);
        if (c != 0)
            delete c;
    }
    while (mParts.size() != 0) {
        VxPart* p = mParts.end()[-1];
        _removePart(p);
        if (p != 0)
            delete p;
    }
    while (m44.size() != 0) {
        VxAssemblyBase* a = m44.end()[-1];
        VxAssemblyBase::removeAssembly(a);
        if (a != 0)
            delete a;
    }
    delete mDisabledPairs;
    VxAssemblyBase::setName(0);
    mController.setParent(0);
    mNode.release();
}

// Mac 0x42b3b8, Win 0x729b80
const char* VxAssemblyBase::getClassName() const
{
    return "VxAssemblyBase";
}

// Mac 0x42b3ca / 0x42b3d4, Win 0x719870 / 0x7c61f0
int VxAssemblyBase::vf35() const
{
    return 0;
}

void* VxAssemblyBase::vf36(int) const
{
    return 0;
}

// Mac 0x4253a4, Win 0x725b50
void VxAssemblyBase::setInternalCollisionEnabled(bool b)
{
    if (mUniverse != 0 && mInternalCollisionEnabled != b) {
        if (!b)
            disablePairIntersect(this);
        else
            enablePairIntersect(this);
    }
    mInternalCollisionEnabled = b;
}

// Mac 0x4253f4, Win 0x725e30
void VxAssemblyBase::saveState()
{
    if (mSavedState == 0)
        mSavedState = createState();
    else
        mSavedState->save();
}

// Mac 0x42582c, Win 0x725e60
VxStateBase* VxAssemblyBase::createState()
{
    return new VxState(this);
}

// Mac 0x42703e, Win 0x7287a0
void VxAssemblyBase::restoreState()
{
    for (VxSmartPtr<UpdateSubscriber>* i = mUpdateSubscribers.begin(); i != mUpdateSubscribers.end(); ++i)
        i->get()->reset();
    if (mSavedState == 0)
        VxWarning(0, "VxAssemblyBase::restoreState: A saved state does not exist.\n");
    else
        mSavedState->restore(false);
}

// Mac 0x425434, Win 0x7265c0
int VxAssemblyBase::accept(VxVisitor* v)
{
    int r = v->visitAssembly(this);
    if (r != 0)
        return r;
    r = 0;
    bool stop = false;
    for (VxPart** i = mParts.begin(); i != mParts.end(); ++i) {
        r = v->visitEntity(*i);
        if (r == 2) {
            stop = true;
            break;
        }
    }
    if (!stop) {
        for (VxConstraint** i = mConstraints.begin(); i != mConstraints.end(); ++i) {
            r = v->visitConstraint(*i);
            if (r == 2) {
                stop = true;
                break;
            }
        }
    }
    if (!stop) {
        int n = vf35();
        for (int i = 0; i < n && r != 2; ++i)
            r = v->visitOther(vf36(i));
    } else {
        vf35(); // NOTE: the original calls it once more here and ignores the count
    }
    if (r != 2) {
        for (VxAssemblyBase** i = m44.begin(); i != m44.end(); ++i)
            if ((*i)->accept(v) == 2)
                return 2;
        r = v->endVisitAssembly(this);
    }
    return r;
}

// Mac 0x4255b4, Win 0x725f50
void VxAssemblyBase::setMergeChanged()
{
    if (!mMergeChanged) {
        mMergeChanged = true;
        if (mParent != 0)
            mParent->setMergeChanged();
    }
}

// Mac 0x4255de, Win 0x725f80
VxAssemblyBase* VxAssemblyBase::getTopLevelAssembly()
{
    VxAssemblyBase* below = this;
    VxAssemblyBase* a = this;
    for (VxAssemblyBase* p = a->mParent; p != 0; p = a->mParent) {
        below = a;
        a = p;
    }
    return below;
}

// Mac 0x425622
bool VxAssemblyBase::hasUpdateSubscriber(UpdateSubscriber* s) const
{
    for (const VxSmartPtr<UpdateSubscriber>* i = mUpdateSubscribers.begin(); i != mUpdateSubscribers.end(); ++i)
        if (i->get() == s)
            return true;
    return false;
}

// Mac 0x4256c6, Win 0x7268a0
void VxAssemblyBase::mergeRecursive(VxPart* root)
{
    for (VxPart** i = mParts.begin(); i != mParts.end(); ++i) {
        VxPart* p = *i;
        if (p != root && p->mInternalControl == VxEntity::kControlDynamic) {
            if (root != 0 && root->getMergeRoot() == p->getMergeRoot())
                continue;
            p->setMergeParent(root);
        }
    }
    for (VxAssemblyBase** i = m44.begin(); i != m44.end(); ++i)
        (*i)->mergeRecursive(root);
}

// Mac 0x42575c, Win 0x726930
void VxAssemblyBase::setMergeRoot(VxPart* root)
{
    mergeRecursive(root);
}

// Mac 0x425894, Win 0x726fb0
void VxAssemblyBase::connectParts(VxUniverse* u)
{
    for (VxPart** i = mParts.begin(); i != mParts.end(); ++i)
        UniverseConnector::connectEntity(this, *i);
    for (VxAssemblyBase** i = m44.begin(); i != m44.end(); ++i)
        (*i)->connectParts(u);
}

// Mac 0x42591a
void VxAssemblyBase::connectConstraints(VxUniverse* u)
{
    for (VxConstraint** i = mConstraints.begin(); i != mConstraints.end(); ++i)
        UniverseConnector::connectConstraint(this, *i);
    for (VxAssemblyBase** i = m44.begin(); i != m44.end(); ++i)
        (*i)->connectConstraints(u);
}

// Mac 0x4259a0, Win 0x726ed0
void VxAssemblyBase::setUniverseOfSubscriberLists(VxUniverse* u)
{
    for (VxSubscriberListResponse** i = mLists.begin(); i != mLists.end(); ++i) {
        VxSubscriberListResponse* l = *i;
        if (mUniverse != 0)
            mUniverse->removeIntersectSubscriber(l->mType1, l->mType2, l->mEvent, l);
        if (u != 0)
            u->addIntersectSubscriber(l->mType1, l->mType2, l->mEvent, l, 0);
    }
    for (VxAssemblyBase** i = m44.begin(); i != m44.end(); ++i)
        (*i)->setUniverseOfSubscriberLists(u);
}

// Mac 0x425a70, Win 0x727070
void VxAssemblyBase::setUniverse(VxUniverse* u)
{
    if (mUniverse == u)
        return;
    setUniverseOfSubscriberLists(u);
    setUniverseRecursive(u);
    connectConstraints(u);
    connectParts(u);
    if (mUniverse != 0) {
        // the internal collisions are applied again when they match the space's pair default
        bool b = mInternalCollisionEnabled;
        if (b == u->mFlag) {
            mInternalCollisionEnabled = !b;
            setInternalCollisionEnabled(b);
            if (mUniverse == 0)
                return;
        }
        mUniverse->applyDisabledCollisions(true);
    }
}

// Mac 0x425b0a, Win 0x726e80
void VxAssemblyBase::updateNodes()
{
    for (VxPart** i = mParts.begin(); i != mParts.end(); ++i)
        (*i)->updateNode();
    for (VxAssemblyBase** i = m44.begin(); i != m44.end(); ++i)
        (*i)->updateNodes();
}

// Mac 0x425b72, Win 0x726de0
void VxAssemblyBase::transformLocalRecursive(const VxTransform& t)
{
    for (VxPart** i = mParts.begin(); i != mParts.end(); ++i) {
        VxEntity* e = *i;
        VxTransform tm, out;
        e->getTransform(tm);
        MeMatrix4TMCompound(out.m, t.m, tm.m);
        e->setTransform(out);
        if (e->isPart())
            static_cast<VxPart*>(e)->resetForces();
    }
    for (VxAssemblyBase** i = m44.begin(); i != m44.end(); ++i)
        (*i)->transformLocalRecursive(t);
}

// Mac 0x425c6a, Win 0x727380
void VxAssemblyBase::transformLocal(const VxTransform& t)
{
    transformLocalRecursive(t);
    updateNodes();
}

// Mac 0x425c92
void VxAssemblyBase::transformParts(const VxTransform& t)
{
    for (VxPart** i = mParts.begin(); i != mParts.end(); ++i) {
        VxEntity* e = *i;
        VxTransform tm, out;
        e->getTransform(tm);
        MeMatrix4TMCompound(out.m, tm.m, t.m);
        e->setTransform(out);
        if (e->isPart())
            static_cast<VxPart*>(e)->resetForces();
    }
}

// Mac 0x425d38
void VxAssemblyBase::transformRecursive(const VxTransform& t)
{
    transformParts(t);
    for (VxAssemblyBase** i = m44.begin(); i != m44.end(); ++i)
        (*i)->transformRecursive(t);
}

// Mac 0x425daa, Win 0x727320
void VxAssemblyBase::transform(const VxTransform& t)
{
    transformRecursive(t);
    updateNodes();
}

// Mac 0x425dd2, Win 0x725b30
VxAssemblyBase* VxAssemblyBase::findFromName(const char* name)
{
    if (sNameMap == 0)
        return 0;
    return static_cast<VxAssemblyBase*>(VxBase::findFromName(name, sNameMap));
}

// Mac 0x425e04, Win 0x725b00
void VxAssemblyBase::setName(const char* name)
{
    VxBase::setName(name, &sNameMap);
    mController.VxBase::setName(name);
}

// Mac 0x425e50, Win 0x727010
void VxAssemblyBase::setUniverseRecursive(VxUniverse* u)
{
    mUniverse = u;
    for (VxAssemblyBase** i = m44.begin(); i != m44.end(); ++i)
        (*i)->setUniverseRecursive(u);
    mController.VxCompositeController::setUniverse(u);
}

// Mac 0x425ebc, Win 0x726cd0
void VxAssemblyBase::setGrouping(int grouping)
{
    for (VxPart** i = mParts.begin(); i != mParts.end(); ++i)
        if ((*i)->isPart())
            (*i)->setGrouping(grouping);
    for (VxAssemblyBase** i = m44.begin(); i != m44.end(); ++i)
        (*i)->setGrouping(grouping);
}

// Mac 0x425f4c, Win 0x7261e0
void VxAssemblyBase::enablePairIntersect(VxCollisionSensor* sensor)
{
    for (VxPart** i = mParts.begin(); i != mParts.end(); ++i) {
        VxCollisionSensor* s = *i != 0 ? dynamic_cast<VxCollisionSensor*>(static_cast<VxEntity*>(*i)) : 0;
        if (s->mUniverse != 0)
            s->mUniverse->enablePairIntersect(s, sensor);
    }
    for (VxAssemblyBase** i = m44.begin(); i != m44.end(); ++i)
        (*i)->enablePairIntersect(sensor);
}

// Mac 0x425fea, Win 0x7260e0
void VxAssemblyBase::disablePairIntersect(VxCollisionGeometry* cg)
{
    VxUniverse* u = mUniverse;
    if (u == 0)
        return;
    for (VxPart** i = mParts.begin(); i != mParts.end(); ++i) {
        VxCollisionSensor* s = *i != 0 ? dynamic_cast<VxCollisionSensor*>(static_cast<VxEntity*>(*i)) : 0;
        u->disablePairIntersect(cg, s);
    }
    for (VxAssemblyBase** i = m44.begin(); i != m44.end(); ++i)
        (*i)->disablePairIntersect(cg);
}

// Mac 0x426090, Win 0x726d30
void VxAssemblyBase::freeze(bool b)
{
    for (VxPart** i = mParts.begin(); i != mParts.end(); ++i)
        (*i)->freeze(b);
    for (VxAssemblyBase** i = m44.begin(); i != m44.end(); ++i)
        (*i)->freeze(b);
}

// Mac 0x4261ea, Win 0x725d70
void VxAssemblyBase::applyDisabledCollisions()
{
    mDisabledPairs->apply(mUniverse);
}

// Mac 0x426208, Win 0x726160
void VxAssemblyBase::enablePairIntersect(VxCollisionGeometry* cg)
{
    VxUniverse* u = mUniverse;
    if (u == 0)
        return;
    for (VxPart** i = mParts.begin(); i != mParts.end(); ++i) {
        VxCollisionSensor* s = *i != 0 ? dynamic_cast<VxCollisionSensor*>(static_cast<VxEntity*>(*i)) : 0;
        u->enablePairIntersect(cg, s);
    }
    for (VxAssemblyBase** i = m44.begin(); i != m44.end(); ++i)
        (*i)->enablePairIntersect(cg);
}

// Mac 0x4262ae, Win 0x726700
int VxAssemblyBase::getFrozenState() const
{
    bool frozen = false, notFrozen = false;
    for (VxPart* const* i = mParts.begin(); i != mParts.end(); ++i) {
        if ((*i)->mInternalControl == VxEntity::kControlFrozen) {
            if (notFrozen)
                return 2;
            frozen = true;
        } else {
            if (frozen)
                return 2;
            notFrozen = true;
        }
    }
    for (VxAssemblyBase* const* i = m44.begin(); i != m44.end(); ++i) {
        int s = (*i)->getFrozenState();
        if (s == 0) {
            if (frozen)
                return 2;
            notFrozen = true;
        } else {
            if (s != 1 || notFrozen)
                return 2;
            frozen = true;
        }
    }
    return frozen ? !notFrozen : 0;
}

// Mac 0x426376, Win 0x726580
VxReal VxAssemblyBase::getMass() const
{
    VxReal m = getPartsMass();
    for (VxAssemblyBase* const* i = m44.begin(); i != m44.end(); ++i)
        m = m + (*i)->getMass();
    return m;
}

// Mac 0x4263c2, Win 0x7264a0
VxVector3 VxAssemblyBase::getCOMPosition(VxReal* mass) const
{
    VxReal m = 0;
    VxVector3 com = getPartsCOMPosition(&m);
    com[0] = com[0] * m;
    com[1] = com[1] * m;
    com[2] = com[2] * m;
    VxReal total = m;
    for (VxAssemblyBase* const* i = m44.begin(); i != m44.end(); ++i) {
        VxReal sm;
        VxVector3 c = (*i)->getCOMPosition(&sm);
        total = total + sm;
        com[0] = sm * c[0] + com[0];
        com[1] = com[1] + sm * c[1];
        com[2] = com[2] + sm * c[2];
    }
    if (total != 0.0) {
        VxReal inv = 1.0 / total;
        com[0] = com[0] * inv;
        com[1] = com[1] * inv;
        com[2] = inv * com[2];
    }
    if (mass != 0)
        *mass = total;
    return com;
}

// Mac 0x426518, Win 0x727ee0
void VxAssemblyBase::preStep()
{
    mController.preStep();
    for (VxPart** i = mParts.begin(); i != mParts.end(); ++i)
        (*i)->applyAnimationDriverIfMode0();
    for (VxAssemblyBase** i = m44.begin(); i != m44.end(); ++i)
        (*i)->preStep();
    VxReal dt = mUniverse->mTimeStep;
    for (VxSmartPtr<UpdateSubscriber>* i = mUpdateSubscribers.begin(); i != mUpdateSubscribers.end(); ++i)
        i->get()->update(dt);
}

// Mac 0x426680, Win 0x726c80
void VxAssemblyBase::applyAnimationDrivers()
{
    for (VxPart** i = mParts.begin(); i != mParts.end(); ++i)
        (*i)->applyAnimationDriverIfNotMode2();
    for (VxAssemblyBase** i = m44.begin(); i != m44.end(); ++i)
        (*i)->applyAnimationDrivers();
}

// Mac 0x4266f6, Win 0x726c30
void VxAssemblyBase::updateCollisionGeometryTransforms()
{
    for (VxPart** i = mParts.begin(); i != mParts.end(); ++i)
        (*i)->updateCollisionGeometryTransforms();
    for (VxAssemblyBase** i = m44.begin(); i != m44.end(); ++i)
        (*i)->updateCollisionGeometryTransforms();
}

// Mac 0x42675e, Win 0x725ef0
bool VxAssemblyBase::contains(VxEntity* e) const
{
    for (VxAssemblyBase* a = e->mAssembly; a != 0; a = a->mParent)
        if (a == this)
            return true;
    return false;
}

// Mac 0x426792, Win 0x726bf0
void VxAssemblyBase::step()
{
    mController.step();
    for (VxAssemblyBase** i = m44.begin(); i != m44.end(); ++i)
        (*i)->step();
}

// Mac 0x4267dc, Win 0x727f90
void VxAssemblyBase::postStep()
{
    for (VxPart** i = mParts.begin(); i != mParts.end(); ++i)
        (*i)->updateBeforeStep();
    for (VxAssemblyBase** i = m44.begin(); i != m44.end(); ++i)
        (*i)->postStep();
    VxReal dt = mUniverse->mTimeStep;
    for (VxSmartPtr<UpdateSubscriber>* i = mUpdateSubscribers.begin(); i != mUpdateSubscribers.end(); ++i)
        i->get()->postUpdate(dt);
    mController.postStep();
}

// Mac 0x426938, Win 0x725bb0
bool VxAssemblyBase::getPartsBoundingSphere(VxVector3& center, VxReal& radius, VxVector3* min, VxVector3* max) const
{
    VxVector3 a, b;
    bool ok = getPartsBoundingBox(a, b);
    radius = 0;
    if (ok) {
        for (int i = 0; i < 3; ++i)
            center[i] = (a[i] + b[i]) * 0.5;
        VxReal dx = center[0] - b[0], dy = center[1] - b[1], dz = center[2] - b[2];
        radius = sqrt(dz * dz + (dy * dy + (dx * dx + 0.0)));
        if (min != 0 && max != 0) {
            *min = a;
            *max = b;
        }
    }
    return ok;
}

// Mac 0x427296, Win 0x725c80
bool VxAssemblyBase::getBoundingSphere(VxVector3& center, VxReal& radius, VxVector3* min, VxVector3* max) const
{
    VxVector3 a, b;
    bool ok = getBoundingBox(a, b);
    radius = 0;
    if (ok) {
        for (int i = 0; i < 3; ++i)
            center[i] = (a[i] + b[i]) * 0.5;
        VxReal dx = center[0] - b[0], dy = center[1] - b[1], dz = center[2] - b[2];
        radius = sqrt(dz * dz + (dy * dy + (dx * dx + 0.0)));
        if (min != 0 && max != 0) {
            *min = a;
            *max = b;
        }
    }
    return ok;
}

// Mac 0x426a4c, Win 0x726060
void VxAssemblyBase::disablePairIntersect(VxCollisionSensor* sensor)
{
    for (VxPart** i = mParts.begin(); i != mParts.end(); ++i) {
        VxCollisionSensor* s = *i != 0 ? dynamic_cast<VxCollisionSensor*>(static_cast<VxEntity*>(*i)) : 0;
        if (s->mUniverse != 0)
            s->mUniverse->disablePairIntersect(s, sensor);
    }
    for (VxAssemblyBase** i = m44.begin(); i != m44.end(); ++i)
        (*i)->disablePairIntersect(sensor);
}

// Mac 0x426aea, Win 0x726260
bool VxAssemblyBase::getPartsBoundingBox(VxVector3& min, VxVector3& max) const
{
    const VxReal big = std::numeric_limits<VxReal>::infinity();   // the original 1.79769313486232e+308 (> DBL_MAX)
    min = VxVector3(big, big, big);
    max = VxVector3(-big, -big, -big);
    bool found = false;
    for (VxPart* const* i = mParts.begin(); i != mParts.end(); ++i) {
        VxEntity* e = *i;
        VxVector3 mn(0, 0, 0), mx(0, 0, 0);
        if (e->isPart() && 0 < e->getCollisionGeometryCount() && e->getBoundingBox(mn, mx)) {
            for (int k = 0; k < 3; ++k)
                if (mn[k] < min[k])
                    min[k] = mn[k];
            for (int k = 0; k < 3; ++k)
                if (max[k] < mx[k])
                    max[k] = mx[k];
            found = true;
        }
    }
    return found;
}

// Mac 0x426ca8, Win 0x7269a0
VxReal VxAssemblyBase::getPartsMass() const
{
    VxReal m = 0.0;
    for (unsigned int i = 0; i < (unsigned int)mParts.size(); ++i) {
        VxPart* p = mParts.begin()[i];
        if (p->isOfType(VxPart::getStaticClassType()) && p->mControl == VxEntity::kControlDynamic)
            m = m + p->getMass();
    }
    return m;
}

// Mac 0x426d4c, Win 0x726a30
VxVector3 VxAssemblyBase::getPartsCOMPosition(VxReal* mass) const
{
    VxVector3 com(0, 0, 0);
    VxReal total = 0.0;
    if (mParts.size() != 0) {
        for (unsigned int i = 0; i < (unsigned int)mParts.size(); ++i) {
            VxPart* p = mParts.begin()[i];
            if (p->isOfType(VxPart::getStaticClassType()) && p->mControl == VxEntity::kControlDynamic) {
                VxReal m = p->getMass();
                VxVector3 c(0, 0, 0);
                total = total + m;
                p->getCOMPosition(c);
                com[0] = m * c[0] + com[0];
                com[1] = m * c[1] + com[1];
                com[2] = m * c[2] + com[2];
            }
        }
        if (total != 0.0) {
            VxReal inv = 1.0 / total;
            com[0] = com[0] * inv;
            com[1] = com[1] * inv;
            com[2] = inv * com[2];
        }
    }
    if (mass != 0)
        *mass = total;
    return com;
}

// Mac 0x426ef0, Win 0x7263a0. NOTE: a sub-assembly without a box still widens the box with its
// zeroed corners (the original's).
bool VxAssemblyBase::getBoundingBox(VxVector3& min, VxVector3& max) const
{
    bool found = getPartsBoundingBox(min, max);
    for (VxAssemblyBase* const* i = m44.begin(); i != m44.end(); ++i) {
        VxVector3 mn(0, 0, 0), mx(0, 0, 0);
        if ((*i)->getBoundingBox(mn, mx))
            found = true;
        for (int k = 0; k < 3; ++k)
            if (mn[k] < min[k])
                min[k] = mn[k];
        for (int k = 0; k < 3; ++k)
            if (max[k] < mx[k])
                max[k] = mx[k];
    }
    return found;
}

// Mac 0x427132, Win 0x7270f0
bool VxAssemblyBase::removeAssembly(VxAssemblyBase* a)
{
    VxAssemblyBase** i = m44.begin();
    for (; i != m44.end(); ++i)
        if (*i == a)
            break;
    if (i == m44.end())
        return false;
    UniverseConnector::removeChildAssembly(a);
    a->setParent(0);
    m44.remove(a);
    setMergeChanged();
    return true;
}

// Mac 0x4271c0 (+ thunk 0x4271b5); Win: the handler slot only
VxSubscriberListResponse* VxAssemblyBase::createSubscriberList(int type1, int type2, int event)
{
    AcceptIntersectPair filter(this);
    VxSubscriberListResponse* l = new VxSubscriberListResponse(type1, type2, event);
    l->mFilter = filter.clone();
    if (mUniverse != 0)
        mUniverse->addIntersectSubscriber(type1, type2, event, l, 0);
    return l;
}

// Mac 0x4273aa, Win 0x7267b0
void VxAssemblyBase::mergeParts(int mode, VxPart* part)
{
    if (mode == 0) {
        setMergeRoot(0);
    } else if (mode == 1) {
        if (part == 0) {
            FirstDynamicPartFinder finder;
            accept(&finder);
            setMergeRoot(finder.mPart);
        } else {
            setMergeRoot(part);
        }
    }
}

// Mac 0x427480, Win 0x726880
void VxAssemblyBase::mergeParts(int mode)
{
    mergeParts(mode, 0);
}

// Mac 0x4274cc, Win 0x727cb0. NOTE: the Mac returns nothing meaningful; true when added here.
bool VxAssemblyBase::addAssembly(VxAssemblyBase* a)
{
    VxAssert(a->mParent == 0, "VxAssemblyBase::addAssembly: Assembly '%s' already child of an assembly.\n", a->getName());
    VxAssert(a->mUniverse == 0, "VxAssemblyBase::addAssembly: Assembly '%s' already in a universe.\n", a->getName());
    for (VxAssemblyBase** i = m44.begin(); i != m44.end(); ++i)
        if (*i == a)
            return false;
    UniverseConnector::addChildAssembly(this, a);
    if (a->mUniverse != 0 && !mInternalCollisionEnabled)
        getTopLevelAssembly()->disablePairIntersect(a);
    m44.push_back(a);
    a->setParent(this);
    setMergeChanged();
    if (mUniverse != 0)
        mUniverse->applyDisabledCollisions(true);
    return true;
}

// Mac 0x427a68, Win 0x727570
void VxAssemblyBase::removeUpdateSubscriber(UpdateSubscriber* s)
{
    VxSmartPtr<UpdateSubscriber> keep(s);
    VxSmartPtr<UpdateSubscriber>* i = mUpdateSubscribers.begin();
    for (; i != mUpdateSubscribers.end(); ++i)
        if (i->get() == s)
            break;
    if (i == mUpdateSubscribers.end())
        return;
    for (VxSmartPtr<UpdateSubscriber>* j = i + 1; j != mUpdateSubscribers.end(); ++j)
        *(j - 1) = *j;
    VxSmartPtr<UpdateSubscriber>* last = mUpdateSubscribers.end() - 1;
    --mUpdateSubscribers.mEnd;
    *last = (UpdateSubscriber*)0;
    last->~VxSmartPtr<UpdateSubscriber>();
}

namespace {

VxSmartPtr<VxAssemblyBase::ExposedFields>* findExposed(VxArray<VxSmartPtr<VxAssemblyBase::ExposedFields> >& a,
                                                       VxData::FieldBase* field)
{
    VxSmartPtr<VxAssemblyBase::ExposedFields>* i = a.begin();
    for (; i != a.end(); ++i)
        if ((*i)->mOriginal->field == field)
            break;
    return i;
}

} // namespace

// Mac 0x427b82, Win 0x7273a0
void VxAssemblyBase::_unexposeField(VxData::FieldBase* field)
{
    if (!sExposeFields)
        return;
    VxSmartPtr<ExposedFields>* ef = findExposed(mExposedFields, field);
    if (ef == mExposedFields.end()) {
        VxWarning(0, "_unexposeField called with a field that was not previously exposed.\n");
        return;
    }
    VxData::FieldBase* exposed = (*ef)->mExposed->field;
    // the connections of the composite that use either field (a copy of the list)
    VxArray<VxConnection*> connections(mController.mConnections);
    for (VxConnection** i = connections.begin(); i != connections.end(); ++i) {
        VxConnectionFunctor* f = (*i)->mFunctor;
        VxData::FieldBase* out = f != 0 ? f->getOutputField() : 0;
        VxData::FieldBase* in = f != 0 ? f->getInputField() : 0;
        if (out == field || in == field)
            mController.deleteConnection(*i);
        else if (out == exposed || in == exposed)
            mController.deleteConnection(*i);
    }
    if (mParent != 0 && forwardsExposedFields())
        mParent->_unexposeField(exposed);
    for (VxSmartPtr<ExposedFields>* j = ef + 1; j != mExposedFields.end(); ++j)
        *(j - 1) = *j;
    VxSmartPtr<ExposedFields>* last = mExposedFields.end() - 1;
    --mExposedFields.mEnd;
    *last = (ExposedFields*)0;
    last->~VxSmartPtr<ExposedFields>();
}

// Mac 0x427e02, Win 0x7272b0
void VxAssemblyBase::_removeConstraint(VxConstraint* c)
{
    if (c == 0 || (VxAssemblyBase*)c->m34 != this)
        return;
    bool found = false;
    for (VxConstraint** i = mConstraints.begin(); i != mConstraints.end(); ++i)
        if (*i == c) {
            found = true;
            break;
        }
    VxAssert(found, "VxAssemblyBase::_removeConstraint: cannot remove constraint.");
    mConstraints.remove(c);
    UniverseConnector::disconnectConstraint(c);
    notifyCollection(&VxEntityConstraintCollection<VxAssemblyBase>::Subscriber::notifyConstraint, this, kConstraintRemoved, c);
}

// Mac 0x428098, Win 0x727610
void VxAssemblyBase::computeMergeState(VxArray<VxPart*>& parts)
{
    for (VxAssemblyBase** i = m44.begin(); i != m44.end(); ++i) {
        VxArray<VxPart*> sub;
        (*i)->computeMergeState(sub);
        parts.insert(parts.end(), sub.begin(), sub.end());
    }
    for (VxPart** i = mParts.begin(); i != mParts.end(); ++i) {
        VxPart* p = *i;
        if (p->isOfType(VxPart::getStaticClassType()) && p->mInternalControl == VxEntity::kControlDynamic)
            parts.push_back(p);
    }
    if ((unsigned int)parts.size() < 2) {
        mMergeState = 2;
    } else {
        VxArray<VxPart*> roots;
        roots.reserve(parts.size());
        for (VxPart** i = parts.begin(); i != parts.end(); ++i)
            roots.pushBackUnique((*i)->getMergeRoot());
        if (roots.size() == 1)
            mMergeState = 1;
        else
            mMergeState = (roots.size() != parts.size()) * 2;
    }
    mMergeChanged = false;
}

// Mac 0x4286be, Win 0x727780
int VxAssemblyBase::getMergeState()
{
    if (mMergeChanged) {
        VxArray<VxPart*> parts;
        computeMergeState(parts);
    }
    return mMergeState;
}

// Mac 0x428768, Win 0x7280b0
void VxAssemblyBase::enablePairIntersect(VxAssemblyBase* other)
{
    AssemblyPairs pairs;
    pairs.run(this, other, false);
}

// Mac 0x42a25e, Win 0x728040
void VxAssemblyBase::disablePairIntersect(VxAssemblyBase* other)
{
    AssemblyPairs pairs;
    pairs.run(this, other, true);
}

// Mac 0x428bcc, Win 0x728120
void VxAssemblyBase::_exposeField(VxData::FieldBase* field, int kind)
{
    if (!sExposeFields)
        return;
    if (findExposed(mExposedFields, field) != mExposedFields.end()) {
        VxWarning(0, "_exposeField called with a field that is already exposed.\n");
        return;
    }
    VxData::FieldBase* exposed = 0;
    VxInputOutputContainer& fields = mController.getFields();
    if (kind == 2) {
        mController.exposeOutput(field->getName(), field, 2, true);
        exposed = fields.getOutputs().getFields().end()[-1];
    } else if (kind == 3) {
        mController.exposeParameter(field->getName(), field, 3, true);
        exposed = fields.getParameters().getFields().end()[-1];
    } else if (kind == 1) {
        mController.exposeInput(field->getName(), field, 1, true);
        exposed = fields.getInputs().getFields().end()[-1];
        VxConnection::FieldInfo from = {field, kind};
        VxConnection::FieldInfo to = {exposed, 1};
        InternalConnection* c = new InternalConnection();
        c->connect(from, to);
        c->setType(1);
        c->mNotDependency = true;
        mController.addConnection(c);
    } else {
        return;
    }
    if (exposed == 0)
        return;
    VxSmartPtr<ExposedFields> ef(new ExposedFields(field, exposed, kind));
    mExposedFields.push_back(ef);
    ef = (ExposedFields*)0;
    if (mParent != 0 && forwardsExposedFields())
        mParent->_exposeField(exposed, kind);
}

// Mac 0x428fc4, Win 0x728950
void VxAssemblyBase::setParent(VxAssemblyBase* parent)
{
    if (mParent == parent)
        return;
    if (sExposeFields && mParent != 0 && forwardsExposedFields())
        for (VxSmartPtr<ExposedFields>* i = mExposedFields.begin(); i != mExposedFields.end(); ++i)
            mParent->_unexposeField((*i)->mExposed->field);
    mParent = parent;
    if (sExposeFields && parent != 0 && forwardsExposedFields())
        for (VxSmartPtr<ExposedFields>* i = mExposedFields.begin(); i != mExposedFields.end(); ++i)
            mParent->_exposeField((*i)->mExposed->field, (*i)->mExposed->kind);
}

// Mac 0x429094, Win 0x727b90. NOTE: a part already in an assembly answers true (the original's).
bool VxAssemblyBase::_addPart(VxPart* p)
{
    if (p->mAssembly != 0)
        return true;
    if (p == 0)
        return false;
    mParts.push_back(p);
    UniverseConnector::connectEntity(this, p);
    setMergeChanged();
    if (!mInternalCollisionEnabled && mUniverse != 0 && p != 0) {
        VxCollisionSensor* s = dynamic_cast<VxCollisionSensor*>(static_cast<VxEntity*>(p));
        if (s != 0)
            getTopLevelAssembly()->disablePairIntersect(s);
    }
    notifyCollection(&VxEntityConstraintCollection<VxAssemblyBase>::Subscriber::notifyEntity, this, kEntityAdded, static_cast<VxEntity*>(p));
    return this == p->mAssembly;
}

namespace {

// Mac 0x4298b0 / 0x429e20 (recursive by value; the find loops 0x42977d / 0x429ced compare
// VxBase::getName with the string)
VxPart* findPartIn(VxAssemblyBase* a, std::string name)
{
    for (VxPart** i = a->mParts.begin(); i != a->mParts.end(); ++i)
        if (name.compare((*i)->getName()) == 0)
            return *i;
    VxPart* r = 0;
    for (VxAssemblyBase** i = a->m44.begin(); i != a->m44.end(); ++i) {
        r = findPartIn(*i, name);
        if (r != 0)
            break;
    }
    return r;
}

VxConstraint* findConstraintIn(VxAssemblyBase* a, std::string name)
{
    for (VxConstraint** i = a->mConstraints.begin(); i != a->mConstraints.end(); ++i)
        if (name.compare((*i)->getName()) == 0)
            return *i;
    VxConstraint* r = 0;
    for (VxAssemblyBase** i = a->m44.begin(); i != a->m44.end(); ++i) {
        r = findConstraintIn(*i, name);
        if (r != 0)
            break;
    }
    return r;
}

} // namespace

// Mac 0x429b1c, Win 0x728680
VxPart* VxAssemblyBase::findPart(const char* name)
{
    return findPartIn(this, std::string(name != 0 ? name : ""));
}

// Mac 0x42a08c, Win 0x7286e0
VxConstraint* VxAssemblyBase::findConstraint(const char* name)
{
    return findConstraintIn(this, std::string(name != 0 ? name : ""));
}

// Mac 0x42a4d8, Win 0x727220
bool VxAssemblyBase::_removePart(VxPart* p)
{
    if (p == 0 || p->mAssembly != this)
        return false;
    VxEntity* e = p;
    notifyCollection(&VxEntityConstraintCollection<VxAssemblyBase>::Subscriber::notifyEntity, this, kEntityRemoving, e);
    bool found = false;
    for (VxPart** i = mParts.begin(); i != mParts.end(); ++i)
        if (*i == p) {
            found = true;
            break;
        }
    VxAssert(found, "VxAssemblyBase::_removeEntity: cannot remove entity.");
    mParts.remove(p);
    UniverseConnector::disconnectEntity(p);
    setMergeChanged();
    notifyCollection(&VxEntityConstraintCollection<VxAssemblyBase>::Subscriber::notifyEntity, this, kEntityRemoved, e);
    return true;
}

// Mac 0x42acd8, Win 0x728820
void VxAssemblyBase::addUpdateSubscriber(UpdateSubscriber* s, bool front)
{
    if (!front) {
        for (VxSmartPtr<UpdateSubscriber>* i = mUpdateSubscribers.begin(); i != mUpdateSubscribers.end(); ++i)
            if (i->get() == s)
                return;
        mUpdateSubscribers.push_back(VxSmartPtr<UpdateSubscriber>(s));
    } else if (!hasUpdateSubscriber(s)) {
        mUpdateSubscribers.insert(mUpdateSubscribers.begin(), VxSmartPtr<UpdateSubscriber>(s));
    }
}

// Mac 0x42b0e0 (Win: inlined). NOTE: a constraint already in an assembly answers true.
bool VxAssemblyBase::_addConstraint(VxConstraint* c)
{
    if (c == 0)
        return false;
    if (c->m34 != 0)
        return true;
    mConstraints.push_back(c);
    UniverseConnector::connectConstraint(this, c);
    notifyCollection(&VxEntityConstraintCollection<VxAssemblyBase>::Subscriber::notifyConstraint, this, kConstraintAdded, c);
    return this == (VxAssemblyBase*)c->m34;
}

} // namespace Vx
