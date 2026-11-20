// Vortex 5.0.0 build 800 "vortexldd41" - VxCollisionSensor (restored).
// Mac TU 470 0x439a4c-0x43b519 (file guess Vx/VxCollisionSensor.cpp); Win 0x718700-0x7198b0.
// The TU also holds the out-of-line copies of VxEntity's inline slots 9, 10, 16, 24 (Mac
// 0x43b25e-0x43b28c, Vx/VxEntity.h) and VxArray<VxCollisionGeometry*> template code (std::find
// 0x43b2d4, insert 0x43b390). Not restored (Win only, not reached): the summed mass properties
// 0x718de0, 0x718ad0.
#include "Vx/VxEntity.h"
#include "Vx/VxCollisionGeometry.h"
#include "Vx/VxUniverse.h"
#include "VxCollisionInternal.h"

#include <math.h>
#include <vector>
#include "Vinc/VincSpace.h"
#include <algorithm>
#include <unordered_map>

namespace Vx {

namespace {
// the sensors in pair group mode (kept beside them: the class keeps the original's layout)
struct PairGroup {
    int group;                                        // CxSmallSort group of the universe's space
    bool open;                                        // geometries still join the group
    bool allEnabled;                                  // no geometry was ever disabled
    unsigned next;                                    // the sensor's order of the members
    std::unordered_map<McdModelID, unsigned> order;
};
std::unordered_map<const VxCollisionSensor*, PairGroup>& pairGroups()
{
    static std::unordered_map<const VxCollisionSensor*, PairGroup> groups;
    return groups;
}
PairGroup* findPairGroup(const VxCollisionSensor* s)
{
    std::unordered_map<const VxCollisionSensor*, PairGroup>& g = pairGroups();
    if (g.empty())
        return 0;
    std::unordered_map<const VxCollisionSensor*, PairGroup>::iterator it = g.find(s);
    return it != g.end() ? &it->second : 0;
}
bool orderLess(const PairGroup* pg, McdModelID a, McdModelID b)
{
    std::unordered_map<McdModelID, unsigned>::const_iterator ia = pg->order.find(a), ib = pg->order.find(b);
    unsigned oa = ia != pg->order.end() ? ia->second : 0u, ob = ib != pg->order.end() ? ib->second : 0u;
    return oa < ob;
}
} // namespace

void VxCollisionSensor::setPairGroup(bool on)
{
    if (!on) {
        pairGroups().erase(this);
        return;
    }
    if (mUniverse == 0 || mCollisionGeometries.size() != 0 || findPairGroup(this) != 0)
        return;
    int group = VincSpaceGroupNew((VincSpaceID)mUniverse->mSpace);
    if (group == 0)
        return;
    PairGroup& pg = pairGroups()[this];
    pg.group = group;
    pg.open = true;
    pg.allEnabled = true;
    pg.next = 1;
}

bool VxCollisionSensor::pairGroupAllEnabled() const
{
    const PairGroup* pg = findPairGroup(this);
    return pg != 0 && pg->allEnabled;
}

// Mac 0x439a4c (0x107), types Mac 0x43ae90 / Win 0x719680, isOfType Mac 0x43b218 / Win 0x719700,
// isOfClassType Mac 0x439a56 / Win 0x718af0
VX_IMPLEMENT_CLASS_TYPES(VxCollisionSensor, VxEntity, 0x107)

// operator delete Mac 0x439ec5 / Win 0x718850
VX_IMPLEMENT_ALLOCATOR(VxCollisionSensor)

namespace {

// std::find in the original (Mac 0x43b2d4): the index of cg, or the count
int findIndex(const VxArray<VxCollisionGeometry*>& a, VxCollisionGeometry* cg)
{
    int i = 0;
    while (i < a.size() && a.begin()[i] != cg)
        ++i;
    return i;
}

// Mac 0x439d7b, Win 0x718870 (static, cg in EAX): cg is a child of a composite: removed from it;
// an emptied composite is disabled (name inferred)
void removeFromComposite(VxCollisionGeometry* cg)
{
    VxCompositeCollisionGeometry* parent = (VxCompositeCollisionGeometry*)cg->mModel->parent->userData;
    parent->removeCollisionGeometry(cg);
    if (parent->getCollisionGeometryCount() == 0) {
        VxWarning(0, "VxCollisionSensor::removeCollisionGeometry: A VxCompositeCollisionGeometry has 0 children and has been disabled.\n");
        parent->setEnabled(false);
    }
}

} // namespace

// Mac 0x43ab54, Win 0x719340
VxCollisionSensor::VxCollisionSensor()
    : mResponseModel(kDefaultResponseModel)
    , mSweepEnabled(false)
{
    mControl = kControlAnimated;
    setInternalControl(kControlAnimated);
}

// D2 Mac 0x43aa50 (D1 0x43ab40, D0 0x43a95a), Win 0x7193f0 (scalar deleting 0x7198b0)
VxCollisionSensor::~VxCollisionSensor()
{
    if (!pairGroups().empty())
        pairGroups().erase(this);
    VxCollisionGeometry** end = mCollisionGeometries.end();
    for (VxCollisionGeometry** i = mCollisionGeometries.begin(); i != end; ++i) {
        if (mUniverse != 0)
            mUniverse->removeCollisionModel((*i)->mModel);
        (*i)->setEntity(0);
        delete *i;
    }
}

// Mac 0x43b292, Win 0x719860
const char* VxCollisionSensor::getClassName() const
{
    return "VxCollisionSensor";
}

// Mac 0x43a102, Win 0x718b10
void VxCollisionSensor::setName(const char* name)
{
    VxEntity::setName(name);
    VxCollisionGeometry** end = mCollisionGeometries.end();
    for (VxCollisionGeometry** i = mCollisionGeometries.begin(); i != end; ++i)
        (*i)->mModel->ma8 = getName();
}

// Mac 0x43b2a4, Win 0x719870
bool VxCollisionSensor::isPart() const
{
    return false;
}

// Mac 0x43a3e2, Win 0x718700: sensors accept static (0) and animated (1) only
void VxCollisionSensor::setControl(int control)
{
    if (mControl == control || (unsigned int)control >= 2)
        return;
    if (mInternalControl == kControlFrozen) {
        freeze(false);
        mControl = control;
        setInternalControl(control);
        freeze(true);
    } else {
        mControl = control;
        setInternalControl(control);
    }
}

// Mac 0x439bf8, Win 0x719270
void VxCollisionSensor::freeze(bool b)
{
    if ((mInternalControl == kControlFrozen) == b)
        return;
    if (b)
        setInternalControl(kControlFrozen);
    else
        setInternalControl(mControl);
    if (mUniverse != 0)
        updateCollisionGeometryStates();
}

// Mac 0x439b28, Win 0x718bb0
void VxCollisionSensor::updateCollisionGeometryStates()
{
    bool frozen = mInternalControl == kControlFrozen;
    for (VxCollisionGeometry** i = mCollisionGeometries.begin(); i != mCollisionGeometries.end(); ++i) {
        if (!(*i)->isEnabled())
            continue;
        McdModelID m = (*i)->mModel;
        if (frozen) {
            (*i)->updateCollisionModel();
            (*i)->update();
            VincSpaceUpdateModel(m);
            VincSpaceFreezeModel(m);
            (*i)->setSweepEnabled(false);
        } else {
            VincSpaceUnfreezeModel(m);
            (*i)->setSweepEnabled(mSweepEnabled);
        }
    }
}

// Mac 0x43b2bc, Win 0x719890
int VxCollisionSensor::getCollisionGeometryCount() const
{
    return mCollisionGeometries.size();
}

// Mac 0x439ace, Win 0x719250
VxCollisionGeometry* VxCollisionSensor::getCollisionGeometry(int i) const
{
    return mCollisionGeometries.begin()[i];
}

// Mac 0x43a46e, Win 0x718f80
bool VxCollisionSensor::getBoundingBox(VxVector3& min, VxVector3& max) const
{
    int n = 0;
    VxCollisionGeometry* const* end = mCollisionGeometries.end();
    for (VxCollisionGeometry* const* i = mCollisionGeometries.begin(); i != end; ++i) {
        if ((*i)->getCollisionMode() != 0)
            continue;
        if (n == 0) {
            (*i)->getBoundingBox(min, max);
        } else {
            VxVector3 lmin, lmax;
            (*i)->getBoundingBox(lmin, lmax);
            for (int k = 0; k < 3; ++k) {
                min[k] = lmin[k] <= min[k] ? lmin[k] : min[k];
                max[k] = lmax[k] <= max[k] ? max[k] : lmax[k];
            }
        }
        ++n;
    }
    if (n == 0) {
        const VxReal* p = getPositionPtr();
        min.set(p[0], p[1], p[2]);
        p = getPositionPtr();
        max.set(p[0], p[1], p[2]);
    }
    return n > 0;
}

// Mac 0x43a644, Win 0x7190b0
bool VxCollisionSensor::getBoundingSphere(VxVector3& center, VxReal& radius) const
{
    if (mCollisionGeometries.size() == 0) {
        const VxReal* p = getPositionPtr();
        center.set(p[0], p[1], p[2]);
        radius = 0;
        return false;
    }
    bool found = false;
    VxCollisionGeometry* const* end = mCollisionGeometries.end();
    for (VxCollisionGeometry* const* i = mCollisionGeometries.begin(); i != end; ++i) {
        if ((*i)->getCollisionMode() != 0)
            continue;
        if (!found) {
            (*i)->getBoundingSphere(center, radius);
            found = true;
            continue;
        }
        VxVector3 c;
        VxReal r;
        (*i)->getBoundingSphere(c, r);
        VxReal dx = c[0] - center[0];
        VxReal dy = c[1] - center[1];
        VxReal dz = c[2] - center[2];
        VxReal dist = sqrt(dz * dz + (dy * dy + (dx * dx + 0.0)));
        VxReal R = radius;
        VxReal big = r <= R ? R : r;
        VxReal small = r <= R ? r : R;
        if (big - (small + dist) <= 0.0) {
            if (dist >= 1e-14) {
                VxReal newR = (R + (dist + r)) * 0.5;
                VxReal k = (newR - R) / dist;
                center[0] = dx * k + center[0];
                center[1] = dy * k + center[1];
                center[2] = dz * k + center[2];
                radius = newR;
            } else {
                radius = r <= R ? R : r;
            }
        } else if (R < r) {
            center = c;
            radius = r;
        }
    }
    return found;
}

// Mac 0x43a36a, Win 0x7192c0
void VxCollisionSensor::setTransform(const VxTransform& tm)
{
    if (mInternalControl == kControlFrozen) {
        VxCollisionSensor::freeze(false);
        VxEntity::setTransform(tm);
        VxCollisionSensor::freeze(true);
    } else {
        VxEntity::setTransform(tm);
        updateCollisionGeometryTransforms();
    }
}

// Mac 0x439fe0, Win 0x718c40
void VxCollisionSensor::updateCollisionGeometryTransforms()
{
    VxCollisionGeometry** end = mCollisionGeometries.end();
    for (VxCollisionGeometry** i = mCollisionGeometries.begin(); i != end; ++i)
        (*i)->updateCollisionModel();
}

// Mac 0x43b378 (-> 0x439b04), Win 0x7198a0. NOTE: no universe check, as in the original.
void VxCollisionSensor::registerResponseModel()
{
    mUniverse->registerResponseModel(mResponseModel);
}

// Mac 0x43b2ae, Win 0x719880
int VxCollisionSensor::getResponseModel() const
{
    return mResponseModel;
}

// Mac 0x43a156, Win 0x718f10
bool VxCollisionSensor::setResponseModel(int model)
{
    // NOTE: the original asks the universe without checking that there is one (a crash for a
    // sensor outside a universe); guarded here: no change.
    if (mUniverse == 0 || !mUniverse->canChangeResponseModel(mResponseModel, model))
        return false;
    mResponseModel = model;
    for (VxCollisionGeometry** i = mCollisionGeometries.begin(); i != mCollisionGeometries.end(); ++i)
        VincModelSetM80((*i)->mModel, (void*)(size_t)mResponseModel);
    return true;
}

// Mac 0x43a2e6, Win 0x718ca0
void VxCollisionSensor::removeFromUniverse()
{
    if (mUniverse == 0)
        return;
    VxCollisionGeometry** end = mCollisionGeometries.end();
    for (VxCollisionGeometry** i = mCollisionGeometries.begin(); i != end; ++i) {
        if ((*i)->isEnabled())
            mUniverse->removeCollisionModel((*i)->mModel);
        if ((*i)->hasFluidInteractionForceFn())
            mUniverse->registerFluidInteraction(*i, false);
    }
}

// Mac 0x43a1ca, Win 0x718d00. NOTE: the models go to this->mUniverse, the type table check to u.
void VxCollisionSensor::addToUniverse(VxUniverse* u)
{
    for (VxCollisionGeometry** i = mCollisionGeometries.begin(); i != mCollisionGeometries.end(); ++i) {
        VxCollisionGeometry* cg = *i;
        cg->prepare();
        if (cg->isEnabled()) {
            McdModelID m = cg->mModel;
            mUniverse->insertCollisionModel(m);
            // the same pairs in the same order, in one call (CxSmallSort::disablePairs)
            std::vector<McdModelID> others;
            for (VxCollisionGeometry** j = mCollisionGeometries.begin(); j != i; ++j)
                if (((*j)->mFlags & kEnabledFlagMask) != 0)
                    others.push_back((*j)->mModel);
            if (!others.empty())
                VincSpaceDisablePairs(m, &others[0], (int)others.size());
            if (u != 0) {
                if (u->getMaxCollisionGeometryType() <= cg->getGeometryTypeID())
                    u->setCollisionGeometryTypeCount(cg->getGeometryTypeID() * 2);
            }
        }
        if ((*i)->hasFluidInteractionForceFn())
            mUniverse->registerFluidInteraction(*i, true);
    }
}

// Mac 0x43ac42, Win 0x719730
void VxCollisionSensor::addCollisionGeometry(VxCollisionGeometry* cg)
{
    VxCollisionSensor* previous = cg->mEntity;
    if (previous != 0) {
        if (previous == this)
            return;
        previous->removeCollisionGeometry(cg, mUniverse != previous->mUniverse);
    }
    if (mUniverse != 0)
        cg->prepare();
    cg->setEntity(this);
    McdModelID m = cg->mModel;
    if (!isPart()) {
        VincModelSetTransformPtr(m, &mTransformPtr->m[0][0]);
        m->setVelocityPtrsFn(m, mLinearVelocity.data(), mAngularVelocity.data());
    }
    VincModelSetM80(m, (void*)(size_t)mResponseModel);
    mCollisionGeometries.push_back(cg);
    if (!cg->isEnabled())
        if (PairGroup* pg = findPairGroup(this))
            pg->allEnabled = false;
    if (mUniverse != 0 && cg->isEnabled())
        collisionGeometryEnableChanged(cg);
    if (!getNameString().empty())
        m->ma8 = getName();
    if (mInternalControl == kControlFrozen) {
        if (mUniverse != 0) {
            VincSpaceUpdateModel(m);
            VincSpaceFreezeModel(m);
        }
        cg->setSweepEnabled(false);
    } else {
        cg->setSweepEnabled(mSweepEnabled);
    }
}

// Mac 0x439de0, Win 0x718760
VxCollisionGeometry* VxCollisionSensor::addGeometry(VxGeometry* geometry, VxMaterial* material,
                                                    const VxTransform* relative, VxReal density)
{
    VxCollisionGeometry* cg = new VxCollisionGeometry(geometry);
    cg->setMaterial(material);
    if (relative != 0)
        cg->setTransformRelative(relative);
    cg->setDensity(density);
    addCollisionGeometry(cg);
    return cg;
}

// Mac 0x439c66, Win 0x719520
bool VxCollisionSensor::removeCollisionGeometry(int index, bool removeFromSpace)
{
    VxCollisionGeometry* cg = mCollisionGeometries.begin()[index];
    for (int i = index; i + 1 < mCollisionGeometries.size(); ++i)
        mCollisionGeometries.begin()[i] = mCollisionGeometries.begin()[i + 1];
    mCollisionGeometries.pop_back();
    if (mUniverse != 0 && cg->isEnabled()) {
        if (!removeFromSpace) {
            if (PairGroup* pg = findPairGroup(this)) {
                VincSpaceGroupLeave(cg->mModel);   // its pairs as they are, then enabled below
                pg->order.erase(cg->mModel);
                pg->open = false;
            }
            if (mInternalControl == kControlFrozen)
                VincSpaceUnfreezeModel(cg->mModel);
            VxCollisionGeometry** end = mCollisionGeometries.end();
            for (VxCollisionGeometry** i = mCollisionGeometries.begin(); i != end; ++i)
                if ((*i)->isEnabled())
                    VincSpaceEnablePair(cg->mModel, (*i)->mModel);
        } else {
            mUniverse->removeCollisionModel(cg->mModel);
        }
    }
    cg->setEntity(0);
    return true;
}

// Mac 0x43a8e0, Win 0x719620. NOTE: like the original, no check that cg is in the list.
bool VxCollisionSensor::removeCollisionGeometry(VxCollisionGeometry* cg, bool removeFromSpace)
{
    if (cg->mModel->parent == 0 || cg->mEntity != this)
        return removeCollisionGeometry(findIndex(mCollisionGeometries, cg), removeFromSpace);
    removeFromComposite(cg);
    return true;
}

// Mac 0x439d56, Win 0x719600
bool VxCollisionSensor::removeCollisionGeometry(int index)
{
    return removeCollisionGeometry(index, true);
}

// Mac 0x43a5c8, Win 0x7194b0
bool VxCollisionSensor::removeCollisionGeometry(VxCollisionGeometry* cg)
{
    if (cg->mModel->parent == 0 || cg->mEntity != this) {
        int index = findIndex(mCollisionGeometries, cg);
        if (index == mCollisionGeometries.size())
            return false;
        return removeCollisionGeometry(index);
    }
    removeFromComposite(cg);
    return true;
}

// Mac 0x43a8ae, Win 0x7189b0
void VxCollisionSensor::setLinearVelocity(const VxVector3& v)
{
    mLinearVelocity = v;
}

// Mac 0x439a9c, Win 0x7189e0
void VxCollisionSensor::setAngularVelocity(const VxVector3& w)
{
    mAngularVelocity = w;
}

// Mac 0x439a6c, Win 0x718950
void VxCollisionSensor::setLinearVelocity(VxReal x, VxReal y, VxReal z)
{
    mLinearVelocity.set(x, y, z);
}

// Mac 0x43a598, Win 0x718980
void VxCollisionSensor::setAngularVelocity(VxReal x, VxReal y, VxReal z)
{
    mAngularVelocity.set(x, y, z);
}

// Mac 0x439f12, Win 0x718a70
void VxCollisionSensor::getLinearVelocity(VxReal& x, VxReal& y, VxReal& z) const
{
    x = mLinearVelocity[0];
    y = mLinearVelocity[1];
    z = mLinearVelocity[2];
}

// Mac 0x439edc, Win 0x718aa0
void VxCollisionSensor::getAngularVelocity(VxReal& x, VxReal& y, VxReal& z) const
{
    x = mAngularVelocity[0];
    y = mAngularVelocity[1];
    z = mAngularVelocity[2];
}

// Mac 0x439f7a, Win 0x718a10
void VxCollisionSensor::getLinearVelocity(VxVector3& v) const
{
    v = mLinearVelocity;
}

// Mac 0x439f48, Win 0x718a40
void VxCollisionSensor::getAngularVelocity(VxVector3& w) const
{
    w = mAngularVelocity;
}

// Mac 0x439fac, Win 0x718c70
void VxCollisionSensor::updateCollisionGeometries()
{
    VxCollisionGeometry** end = mCollisionGeometries.end();
    for (VxCollisionGeometry** i = mCollisionGeometries.begin(); i != end; ++i)
        (*i)->update();
}

// Mac 0x43a074, Win 0x7188c0. NOTE: assumes cg is the last entry (count - 1), as the original.
void VxCollisionSensor::collisionGeometryEnableChanged(VxCollisionGeometry* cg)
{
    McdModelID m = cg->mModel;
    PairGroup* pg = findPairGroup(this);
    if (pg != 0 && !cg->isEnabled()) {
        pg->allEnabled = false;
        pg->order.erase(m);
    }
    if (!cg->isEnabled()) {
        mUniverse->removeCollisionModel(m);   // (a member leaves the group with its pairs reset)
        return;
    }
    mUniverse->insertCollisionModel(m);
    // pair group mode: cg was appended (the last geometry): the others below are the members of
    // the group (the sensor's enabled geometries in the space). Their pairs with a record are
    // disabled one by one in the sensor's order as disablePairs does, the rest by joining.
    if (pg != 0 && pg->open && m->space == (void*)mUniverse->mSpace &&
        mCollisionGeometries.size() != 0 && mCollisionGeometries.end()[-1] == cg) {
        VincModelID partners[64];
        int n = VincSpaceGroupRecordPartners(m, pg->group, partners, 64);
        std::vector<VincModelID> records;
        if (n <= 64) {
            records.assign(partners, partners + n);
        } else {
            records.resize(n);
            VincSpaceGroupRecordPartners(m, pg->group, &records[0], n);
        }
        std::sort(records.begin(), records.end(),
                  [pg](McdModelID a, McdModelID b) { return orderLess(pg, a, b); });
        for (size_t i = 0; i < records.size(); ++i)
            VincSpaceDisablePair(m, records[i]);
        VincSpaceGroupJoin(m, pg->group);
        pg->order[m] = pg->next++;
        return;
    }
    if (pg != 0)
        pg->open = false;   // from here the original's steps (the members keep the group)
    // the same pairs in the same order, in one call (CxSmallSort::disablePairs)
    std::vector<McdModelID> others;
    for (int i = 0; i < getCollisionGeometryCount() - 1; ++i) {
        if (getCollisionGeometry(i)->isEnabled())
            others.push_back(getCollisionGeometry(i)->mModel);
    }
    if (!others.empty())
        VincSpaceDisablePairs(m, &others[0], (int)others.size());
}

// Mac 0x43a014, Win 0x718b60
void VxCollisionSensor::setSweepEnabled(bool b)
{
    mSweepEnabled = b;
    if (mInternalControl == kControlFrozen)
        return;
    VxCollisionGeometry** end = mCollisionGeometries.end();
    for (VxCollisionGeometry** i = mCollisionGeometries.begin(); i != end; ++i)
        (*i)->setSweepEnabled(mSweepEnabled);
}

} // namespace Vx
