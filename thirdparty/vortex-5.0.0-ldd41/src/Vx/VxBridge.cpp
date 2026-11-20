// Vortex 5.0.0 build 800 "vortexldd41" - VxBridge (restored).
// Mac TU 462 0x432e8e-0x437019 (file guess Vx/VxBridge.cpp; its static initialiser 0x436fa9
// only stores MdtGetMaxConstraintSize() into a header static, Mac 0x77f80c); Win
// 0x740750-0x742960 (the VxSymmetricTable / VxCollisionPairRequest template instances
// 0x741fc0-0x742790 are outlined there; the restored code uses the header templates).
#include "Vx/VxBridge.h"
#include "Vx/VxCollisionGeometry.h"
#include "Vx/VxEntity.h"
#include "Vx/VxPart.h"
#include "Vx/VxDynamicsContactInput.h"
#include "Vx/VxMaterial.h"
#include "Vinc/VincIntersect.h"
#include "Vinc/VincModel.h"
#include "Vinc/VincSpace.h"
#include "Mdt/MdtBody.h"
#include "Mdt/MdtConstraint.h"
#include "Me/VinPool.h"

#include <new>
#include <string.h>

namespace Vx {

namespace {

// subscriber events (VxUniverse::IntersectSubscriber) (names inferred)
enum { kEventActive = 0, kEventStaying = 1, kEventFirst = 2, kEventDisjoint = 3 };

inline int responseModel(VincModelID m) { return (int)(size_t)m->m80; }
inline int geometryTypeID(VincModelID m) { return (int)(size_t)m->m7c; }

// Mac 0x4332ee (cg in EAX), Win inlined: the geometry belongs to a part that is in a universe
bool isPartInUniverse(VxCollisionGeometry* cg)
{
    VxCollisionSensor* e = cg->mEntity;
    if (e == 0 || e->getUniverse() == 0)
        return false;
    return e->isOfClassType(VxPart::getStaticClassType());
}

// Mac 0x433331 (model in EAX), Win inlined
bool isPartModelInUniverse(VincModelID m)
{
    return isPartInUniverse((VxCollisionGeometry*)m->userData);
}

// the MdtBody of a model, as the bridge sees it (the body's self pointer: model m6c is the
// part's MdtBody, VxCollisionGeometry::setEntity) (name inferred)
inline MdtBody* modelBody(VincModelID m)
{
    MdtBody* b = (MdtBody*)(size_t)m->m6c;
    return b ? b->self : 0;
}

} // namespace

// The intersect filter of a collision geometry (model ma4): the typed form of
// VxCollisionGeometry::unk_437f26 (Mac 0x437f26, Win 0x7104b0; TU) (names inferred)
void VxCollisionGeometry::setIntersectFilter(VxIntersectFilter* f)
{
    mModel->ma4 = (ptrdiff_t)f;
}
VxIntersectFilter* VxCollisionGeometry::getIntersectFilter() const
{
    return (VxIntersectFilter*)mModel->ma4;
}

// --------------------------------------------------------------------------------------
// Mac C2 0x4361a8 (C1 0x436f74), Win 0x7416b0
VxBridge::VxBridge(VxMaterialTable* materials, CxSpace* space, MdtWorldID world, int contactBufferSize)
{
    mHasStayingSubscribers = false;
    mHasSensorSensorSubscribers = false;
    mSleepingPairsIntersect = false;
    mStayingSubscriberCount = 0;
    mSensorSensorSubscriberCount = 0;
    mMaterialTable = materials;
    {
        VxResponseEntry empty;
        mResponseTable.init(6, empty);
    }
    {
        VxCollisionPairRequest defaults;
        mPairRequests.init(7, defaults);
    }
    mContactPool = 0;
    mPairContainerSize = 0;
    mPairs = 0;
    mSpace = space;
    mWorld = world;
    mSafeTime = 0;
    mNotifyEnabled = true;

    mContactPool = (VinPool*)::operator new(sizeof(VinPool));
    memset(mContactPool, 0, sizeof(VinPool));
    VinPoolSetName(mContactPool, "bridge contact pool");
    setContactBufferSize(contactBufferSize, 2);
    setPairContainerSize(1000);

    // pair-request defaults per geometry type ID (0 = convex shapes, 1 = height fields / meshes,
    // 2-4 / 5 = types the item's code does not name)
    mPairRequests.getEntry(0, 0).setGroupMaxCount(1);
    mPairRequests.getEntry(0, 1).setGroupMaxCount(4);
    mPairRequests.getEntry(0, 5).setGroupMaxCount(4);
    mPairRequests.getEntry(1, 1).setGroupMaxCount(4);
    for (unsigned int i = 0; i != 6; ++i) {
        mPairRequests.getEntry(3, i).setContactMaxCount(2);
        mPairRequests.getEntry(3, i).setGroupMaxCount(3);
    }
    for (unsigned int i = 0; i != 6; ++i) {
        mPairRequests.getEntry(4, i).setContactMaxCount(2);
        mPairRequests.getEntry(4, i).setGroupMaxCount(2);
    }
    mPairRequests.getEntry(4, 4).setGroupMaxCount(1);
    mPairRequests.getEntry(4, 4).setContactMaxCount(1);
    mPairRequests.getEntry(3, 3).setGroupMaxCount(1);
    mPairRequests.getEntry(3, 0).setGroupMaxCount(1);
    mPairRequests.getEntry(4, 0).setGroupMaxCount(1);
    for (unsigned int i = 0; i != 6; ++i)
        mPairRequests.getEntry(2, i).setContactMaxCount(0);
}

// Mac D2 0x435ec6 (D1 0x436194, D0 0x435bec), Win 0x741e20 (sdd 0x742950)
VxBridge::~VxBridge()
{
    destroyUserContacts();
    if (mPairs != 0)
        VincModelPairContainerDestroy(mPairs);
    VinPoolDestroy(mContactPool);
    ::operator delete(mContactPool);
    // (mUserContacts' storage: VxArray dtor)
    mPairRequests.destroy();
    mResponseTable.destroy();
}

// --------------------------------------------------------------------------------------
// Mac 0x432f8c, Win 0x740cf0
void VxBridge::addSubscriber(int type1, int type2, int event, VxUniverse::IntersectSubscriber* s, unsigned int index)
{
    VxSubscriberListResponseHandler::addSubscriber(type1, type2, event, s, index);
    if (event == kEventStaying) {
        mHasStayingSubscribers = true;
        ++mStayingSubscriberCount;
    }
    if (type1 == kVxResponseModelSensor && type1 == type2) {
        mHasSensorSensorSubscribers = true;
        ++mSensorSensorSubscriberCount;
    }
}

// Mac 0x432f08, Win 0x740d40
bool VxBridge::removeSubscriber(int type1, int type2, int event, VxUniverse::IntersectSubscriber* s)
{
    bool removed = VxSubscriberListResponseHandler::removeSubscriber(type1, type2, event, s);
    if (removed) {
        if (event == kEventStaying && --mStayingSubscriberCount == 0)
            mHasStayingSubscribers = false;
        if (type1 == kVxResponseModelSensor && type1 == type2 && --mSensorSensorSubscriberCount == 0)
            mHasSensorSensorSubscribers = false;
    }
    return removed;
}

// Mac 0x432eda, Win 0x740da0
void VxBridge::removeAllSubscribers()
{
    VxSubscriberListResponseHandler::removeAllSubscribers();
    mHasStayingSubscribers = false;
    mHasSensorSensorSubscribers = false;
    mStayingSubscriberCount = 0;
    mSensorSensorSubscriberCount = 0;
}

// --------------------------------------------------------------------------------------
// Mac 0x432e8e, Win 0x740840
int VxBridge::getContactBufferSize() const
{
    return (unsigned int)mContactPool->structSize / sizeof(VincContact);
}

// Mac 0x432eaa, Win 0x740850
int VxBridge::getContactBufferCount() const
{
    return mContactPool->blockCount;
}

// Mac 0x4333da, Win 0x740860
void VxBridge::setContactBufferSize(int contactCount, int bufferCount)
{
    VxAssert(mContactPool->used == 0, "VxBridge::setContactBufferSize: some element are still in used\n");
    VinPoolDestroy(mContactPool);
    VinPoolInit(mContactPool, bufferCount, contactCount * (int)sizeof(VincContact), 2, 0.0f, 0);
}

// Mac 0x43339e, Win 0x7408b0
void VxBridge::setPairContainerSize(unsigned int size)
{
    if (mPairContainerSize < size) {
        if (mPairs != 0)
            VincModelPairContainerDestroy(mPairs);
        mPairContainerSize = size;
        mPairs = VincModelPairContainerCreate(size);
    }
}

// --------------------------------------------------------------------------------------
// Mac 0x43305c / 0x4330fe, Win 0x7423a0
VxResponseEntry& VxBridge::getResponseEntry(unsigned int model1, unsigned int model2)
{
    return mResponseTable.getEntry(model1, model2);
}

// Mac 0x434626, Win 0x742420
VxCollisionPairRequest& VxBridge::getPairRequest(unsigned int type1, unsigned int type2)
{
    return mPairRequests.getEntry(type1, type2);
}

// Mac 0x435148, Win 0x742640
void VxBridge::setResponseEntry(unsigned int model1, unsigned int model2, const VxResponseEntry& e)
{
    mResponseTable.setEntry(model1, model2, e);
}

// Mac 0x4355e4, Win 0x741a60
unsigned int VxBridge::registerResponseModel(unsigned int copyFrom)
{
    unsigned int n = mResponseTable.mSize;
    VxResponseEntry e(mResponseTable.getEntry(copyFrom, copyFrom));
    mResponseTable.setEntry(n, n, e);
    return n;
}

// Win 0x741bd0
bool VxBridge::setResponseModelCount(unsigned int n)
{
    VxResponseEntry empty;
    if (mResponseTable.mSize == 0 || mResponseTable.mSize <= n) {
        mResponseTable.setEntry(n, n, empty);
        return true;
    }
    return false;
}

// Mac 0x433890, Win 0x740e30
bool VxBridge::canChangeResponseModel(unsigned int oldModel, unsigned int newModel)
{
    VxResponseEntry& a = mResponseTable.getEntry(oldModel, oldModel);
    VxResponseEntry& b = mResponseTable.getEntry(oldModel, newModel);
    return a.mActive.mPtr == b.mActive.mPtr;
}

// Mac 0x434886, Win 0x741c60
bool VxBridge::setCollisionGeometryTypeCount(unsigned int n)
{
    unsigned int old = mPairRequests.mSize;
    VxCollisionPairRequest defaults(mPairRequests.getEntry(1, 1));
    if (mPairRequests.mSize != 0 && n < mPairRequests.mSize)
        return false;
    mPairRequests.setEntry(n, n, defaults);
    // NOTE: the new columns are [old - 1, n): entry (r, n) of the new last column keeps the
    // (1, 1) copy - the original's bounds.
    for (unsigned int i = old - 1; i < n; ++i) {
        mPairRequests.getEntry(3, i).setContactMaxCount(2);
        mPairRequests.getEntry(3, i).setGroupMaxCount(3);
    }
    for (unsigned int i = old - 1; i < n; ++i) {
        mPairRequests.getEntry(4, i).setContactMaxCount(2);
        mPairRequests.getEntry(4, i).setGroupMaxCount(2);
    }
    for (unsigned int i = old - 1; i < n; ++i) {
        mPairRequests.getEntry(0, i).setContactMaxCount(4);
        mPairRequests.getEntry(0, i).setGroupMaxCount(4);
    }
    for (unsigned int i = old - 1; i < n; ++i)
        mPairRequests.getEntry(2, i).setContactMaxCount(0);
    return true;
}

// Mac 0x4346c8, Win 0x741030
void VxBridge::updateCollisionGeometryType(VxCollisionGeometry* cg)
{
    if (!isPartInUniverse(cg))
        return;
    VincSpacePairIterator it;
    VincSpacePairIteratorBegin(mSpace, &it);
    VincModelPairContainerReset(mPairs);
    VincModelID model = cg->mModel;
    int more;
    do {
        if (VincSpaceModelIsFrozen(model) == 0)
            more = VincSpaceGetActivePairs(mSpace, &it, mPairs);
        else
            more = VincSpaceGetPairs(mSpace, &it, mPairs);
        for (int i = mPairs->middle; i < mPairs->helloEnd; ++i) {
            VincModelPair* p = mPairs->array[i];
            if ((p->model1 == model && isPartModelInUniverse(p->model2)) ||
                (p->model2 == model && isPartModelInUniverse(p->model1)))
                p->request = getPairRequest(geometryTypeID(p->model1), geometryTypeID(p->model2)).mRecord;
        }
        if (VincSpaceModelIsFrozen(model) != 0) {
            for (int i = mPairs->stayingFirst; i < mPairs->stayingEnd; ++i) {
                VincModelPair* p = mPairs->array[i];
                if ((p->model1 == model && isPartModelInUniverse(p->model2)) ||
                    (p->model2 == model && isPartModelInUniverse(p->model1)))
                    p->request = getPairRequest(geometryTypeID(p->model1), geometryTypeID(p->model2)).mRecord;
            }
        }
    } while (more != 0);
}

// Mac 0x433344, Win 0x7408f0
VxContactMaterial* VxBridge::getContactMaterial(VxMaterial* m1, VxMaterial* m2)
{
    return mMaterialTable->getContactMaterial(m1, m2);
}

// --------------------------------------------------------------------------------------
// Mac 0x4331a0 (m1 EAX, m2 EDX, sensorSensor CL, sleeping on the stack), Win 0x740750
bool VxBridge::canCollide(McdModel* m1, McdModel* m2, bool sensorSensor, bool sleeping)
{
    VxIntersectFilter* f1 = (VxIntersectFilter*)(size_t)m1->ma4;
    if (f1 != 0 && !f1->accept((VxCollisionGeometry*)m1->userData, (VxCollisionGeometry*)m2->userData))
        return false;
    VxIntersectFilter* f2 = (VxIntersectFilter*)(size_t)m2->ma4;
    if (f2 != 0 && f2 != f1 && !f2->accept((VxCollisionGeometry*)m1->userData, (VxCollisionGeometry*)m2->userData))
        return false;
    // geometry type 12 on exactly one side always collides, on both never
    char special = (char)((m2->geometry->type == 12) + (m1->geometry->type == 12));
    if (special == 1)
        return true;
    if (special == 2)
        return false;
    MdtBody* b1 = modelBody(m1);
    MdtBody* b2 = modelBody(m2);
    if (b1 == b2)
        return b1 == 0 && sensorSensor;
    if ((b1 != 0 && MdtBodyIsEnabled(b1)) || (b2 != 0 && MdtBodyIsEnabled(b2)))
        return true;
    if (sleeping) {
        if (b1 != 0 && !MdtBodyIsFrozen(b1))
            return true;
        if (b2 != 0)
            return !MdtBodyIsFrozen(b2);
    }
    return false;
}

// --------------------------------------------------------------------------------------
// Mac 0x43454e, Win 0x741980
void VxBridge::update(VxReal dt)
{
    VincSpaceEndChanges(mSpace);
    VincSpacePairIterator it;
    VincSpacePairIteratorBegin(mSpace, &it);
    int more;
    if (!mHasStayingSubscribers) {
        do {
            VincModelPairContainerReset(mPairs);
            more = VincSpaceGetActivePairs(mSpace, &it, mPairs);
            handleGoodbyesAndHellos();
            handleCollisions(dt);
        } while (more != 0);
    } else {
        do {
            VincModelPairContainerReset(mPairs);
            more = VincSpaceGetPairs(mSpace, &it, mPairs);
            handleGoodbyesAndHellos();
            handleCollisions(dt);
        } while (more != 0);
    }
    VincSpaceBeginChanges(mSpace);
}

// Mac 0x433efc, Win 0x741b20
void VxBridge::updatePairs()
{
    VincSpaceEndChanges(mSpace);
    VincSpacePairIterator it;
    VincSpacePairIteratorBegin(mSpace, &it);
    int more;
    do {
        VincModelPairContainerReset(mPairs);
        more = VincSpaceGetActivePairs(mSpace, &it, mPairs);
        handleGoodbyesAndHellos();
    } while (more != 0);
    VincSpaceBeginChanges(mSpace);
}

// Mac 0x433acc, Win 0x740e70
void VxBridge::handleGoodbyesAndHellos()
{
    for (int i = 0; i < mPairs->goodbyeEnd; ++i) {
        VincModelPair* pair = mPairs->array[i];
        VxResponseEntry& e = mResponseTable.getEntry(responseModel(pair->model1), responseModel(pair->model2));
        if (VincModelPairGetFlag0(pair) != 0) {
            VxSubscriberListResponse* l;
            if (hasSubscribers() && mNotifyEnabled &&
                (l = findSubscriberList(responseModel(pair->model1), responseModel(pair->model2), kEventDisjoint)) != 0) {
                // NOTE: the original leaves the fields not listed here uninitialised
                VincIntersectResult r;
                memset(&r, 0, sizeof(r));
                r.pair = pair;
                r.touch = (int)VincModelPairGetFlag0(pair);
                VxIntersectResult result(&r);
                l->notifyDisjoint(kEventDisjoint, &result);
            }
            VincModelPairSetFlag0(pair, 0);
        }
        if (e.mGoodbye.mPtr != 0)
            e.mGoodbye.mPtr->respond(this, pair, mSpace, mWorld, &e);
    }
    VincGoodbyeEach(mPairs);
    for (int i = mPairs->helloFirst; i < mPairs->middle; ++i) {
        VincModelPair* pair = mPairs->array[i];
        VxResponseEntry& e = mResponseTable.getEntry(responseModel(pair->model1), responseModel(pair->model2));
        if (e.mHello.mPtr == 0)
            VincHello(pair);
        else
            e.mHello.mPtr->respond(this, pair, mSpace, mWorld, &e);
    }
}

// Mac 0x4338e2, Win 0x741240
void VxBridge::dispatch(McdModelPair* pair, McdIntersectResult* r, int event)
{
    int t1 = responseModel(pair->model1), t2 = responseModel(pair->model2);
    VxResponseEntry& e = mResponseTable.getEntry(t1, t2);
    if (e.mActive.mPtr != 0)
        e.mActive.mPtr->respond(this, pair, r, mSpace, mWorld, &e);
    if (hasSubscribers() && mNotifyEnabled) {
        VxSubscriberListResponse* l = findSubscriberList(t1, t2, event);
        if (l != 0) {
            VxIntersectResult result(r);
            if (event == kEventDisjoint) {
                l->notifyDisjoint(kEventDisjoint, &result);
            } else {
                VxDynamicsContactInput input((MdtContactID)(size_t)pair->m24);
                l->notifyIntersect(event, &result, &input);
            }
        }
    }
}

// Mac 0x433f6a, Win 0x741360 ("VxBridge::handleCollisions")
void VxBridge::handleCollisions(VxReal dt)
{
    mSafeTime = dt;
    for (int i = mPairs->helloFirst; i < mPairs->helloEnd; ++i) {
        VincModelPair* pair = mPairs->array[i];
        if (pair->model1->state == 0 && pair->model2->state == 0) {
            if ((dt == 0.0 || VincSpaceModelIsFrozen(pair->model1) == 0 || VincSpaceModelIsFrozen(pair->model2) == 0) &&
                canCollide(pair->model1, pair->model2, mHasSensorSensorSubscribers, mSleepingPairsIntersect)) {
                VincIntersectResult r;
                memset(&r, 0, sizeof(r));
                r.contacts = (VincContact*)VinPoolGetStruct(mContactPool);
                r.contactMaxCount = getContactBufferSize();
                r.contactPool = mContactPool;
                VincIntersectWithTime(pair, &r, dt);
                if (getContactBufferSize() - 1 <= r.contactCount || mContactPool->m30 != 0) {
                    int n = getContactBufferSize();
                    VxInfo(0, "VxBridge::handleCollisions: ContactBuffer size has been increased to %d, the initial buffer size can be set using VxUniverse::setContactBufferSize.\n",
                           n * 2);
                    VinPoolPutStruct(mContactPool, r.contacts);
                    setContactBufferSize(getContactBufferSize() * 2, getContactBufferCount());
                    r.contacts = (VincContact*)VinPoolGetStruct(mContactPool);
                    r.contactMaxCount = getContactBufferSize();
                    r.contactPool = mContactPool;
                    VincIntersectWithTime(pair, &r, dt);
                }
                if (r.touch == 0) {
                    if (VincModelPairGetFlag0(pair) != 0) {
                        VincModelPairSetFlag0(pair, 0);
                        dispatch(pair, &r, kEventDisjoint);
                    }
                } else {
                    if (0.0 < r.time && r.time < mSafeTime)
                        mSafeTime = r.time;
                    int event;
                    if (VincModelPairGetFlag0(pair) == 0) {
                        VincModelPairSetFlag0(pair, 1);
                        event = kEventFirst;
                    } else {
                        event = kEventActive;
                    }
                    dispatch(pair, &r, event);
                }
                VinPoolPutStruct(mContactPool, r.contacts);
            }
        } else if (VincModelPairGetFlag0(pair) != 0) {
            // a model got disabled: the pair says goodbye
            VincModelPairSetFlag0(pair, 0);
            VxResponseEntry& e = mResponseTable.getEntry(responseModel(pair->model1), responseModel(pair->model2));
            if (e.mGoodbye.mPtr != 0)
                e.mGoodbye.mPtr->respond(this, pair, mSpace, mWorld, &e);
        }
    }
    if (hasSubscribers() && mHasStayingSubscribers) {
        for (int i = mPairs->stayingFirst; i < mPairs->stayingEnd; ++i) {
            VincModelPair* pair = mPairs->array[i];
            VxSubscriberListResponse* l = findSubscriberList(responseModel(pair->model1), responseModel(pair->model2), kEventStaying);
            if (l != 0) {
                VincIntersectResult r;
                memset(&r, 0, sizeof(r));
                r.pair = pair;
                VxIntersectResult result(&r);
                l->notifyIntersect(kEventStaying, &result, 0);
            }
        }
    }
}

// Mac 0x43344e, Win 0x740960
int VxBridge::intersect(VxCollisionGeometry* cg, VxReal dt, VxIntersect::IntersectSubscriber* subscriber)
{
    int found = 0;
    if (cg->mModel->state != 0)
        return found;
    VxCollisionGeometry* top = cg;
    while (top->getParent() != 0)
        top = top->getParent();

    VincIntersectResult r;
    memset(&r, 0, sizeof(r));
    VxIntersectResult result(&r);
    r.contacts = (VincContact*)VinPoolGetStruct(mContactPool);
    r.contactPool = mContactPool;
    if (subscriber != 0) {
        subscriber->mResult = &result;
        r.contactMaxCount = getContactBufferSize();
    } else {
        r.contactMaxCount = 0;
    }
    VincModelPair localPair;
    for (int i = mPairs->helloFirst; i < mPairs->helloEnd; ++i) {
        VincModelPair* pair = mPairs->array[i];
        VincModelID other;
        if (pair->model1 == top->mModel)
            other = pair->model2;
        else if (top->mModel == pair->model2)
            other = pair->model1;
        else
            continue;
        if (other->state != 0)
            continue;
        if (subscriber != 0) {
            int f = subscriber->filter((VxCollisionGeometry*)VincModelGetUserData(other));
            if (f == 1)
                continue;
            if (f == 2) {
                // NOTE: the original returns here without giving the contact buffer back
                if (found == 2)
                    found = 1;
                return found;
            }
        }
        if (!canCollide(pair->model1, pair->model2, mHasSensorSensorSubscribers, mSleepingPairsIntersect))
            continue;
        // NOTE: for a geometry inside a composite the pair is rebuilt with the top-level model
        // (the original's choice)
        if (top == cg) {
            VincIntersectWithTime(pair, &r, dt);
        } else {
            VincModelPairReset(&localPair, other, top->mModel);
            VincHelloWithRequest(&localPair, pair->request);
            VincIntersectWithTime(&localPair, &r, dt);
        }
        if (getContactBufferSize() - 1 <= r.contactCount || mContactPool->m30 != 0) {
            // NOTE: this copy prints the old size
            VxInfo(0, "VxBridge::handleCollisions: ContactBuffer size has been increased to %d, the initial buffer size can be set using VxUniverse::setContactBufferSize.\n",
                   getContactBufferSize());
            VinPoolPutStruct(mContactPool, r.contacts);
            setContactBufferSize(getContactBufferSize() * 2, getContactBufferCount());
            r.contacts = (VincContact*)VinPoolGetStruct(mContactPool);
            r.contactMaxCount = getContactBufferSize();
            r.contactPool = mContactPool;
            if (top == cg) {
                VincIntersectWithTime(pair, &r, dt);
            } else {
                VincModelPairReset(&localPair, other, top->mModel);
                VincHelloWithRequest(&localPair, pair->request);
                VincIntersectWithTime(&localPair, &r, dt);
            }
        }
        if (r.touch != 0) {
            if (subscriber != 0 &&
                subscriber->notifyIntersect((VxCollisionGeometry*)VincModelGetUserData(other)) != 2) {
                found = 2;
                continue;
            }
            found = 1;
            break;
        }
    }
    VinPoolPutStruct(mContactPool, r.contacts);
    return found;
}

// --------------------------------------------------------------------------------------
// Mac 0x433368, Win 0x740dc0
void VxBridge::disableUserContacts()
{
    for (MdtContactID* c = mUserContacts.begin(); c != mUserContacts.end(); ++c)
        ((VxDynamicsContact*)*c)->setEnabled(false);
}

// Mac 0x433002, Win 0x741dc0
void VxBridge::destroyUserContacts()
{
    for (MdtContactID* p = mUserContacts.begin(); p != mUserContacts.end(); ++p) {
        MdtContactID c = *p;
        MdtConstraintWakeBodies(c);
        MdtConstraintDisable(c);
        MdtContactDestroy(c);
    }
    mUserContacts.mEnd = mUserContacts.mBegin;
}

// Win 0x741ed0
MdtContactID VxBridge::createUserContact(VxPart* part1, VxPart* part2, VxContactMaterial* material,
                                         const VxVector3& position, const VxVector3& normal, VxReal penetration)
{
    MdtContactID c = MdtContactCreate(mWorld);
    if (c == 0)
        return 0;
    MdtConstraintSetBodies(c, part1 ? part1->mBody : 0, part2 ? part2->mBody : 0);
    MdtConstraintWakeBodies(c);
    MdtConstraintEnable(c);
    MdtContactSetNormal(c, normal[0], normal[1], normal[2]);
    MdtContactSetPosition(c, position[0], position[1], position[2]);
    MdtContactSetPenetration(c, penetration);
    MdtContactSetM_b58(c, 0.0);
    MdtContactSetWakeBodies(c, 0);
    MdtContactSetM_d04(c, 0);
    MdtContactSetContactMaterial(c, material);
    mUserContacts.push_back(c);
    return c;
}

// Win 0x741b80
void VxBridge::destroyUserContact(MdtContactID c)
{
    for (MdtContactID* p = mUserContacts.begin(); p != mUserContacts.end(); ++p) {
        if (*p == c) {
            for (MdtContactID* q = p + 1; q != mUserContacts.end(); ++q)
                *(q - 1) = *q;
            --mUserContacts.mEnd;
            MdtConstraintWakeBodies(c);
            MdtConstraintDisable(c);
            MdtContactDestroy(c);
            return;
        }
    }
}

} // namespace Vx
