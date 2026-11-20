// Vortex 5.0.0 build 800 "vortexldd41" - VxInternalIDIntersectSubscriber (restored).
// Mac TU 495 tail 0x45b59c-0x45b67c + TU 496 0x45b67c-0x45bfa8 (file guess
// Vx/VxInternalIDIntersectSubscriber.cpp; the static initialiser 0x45bf43 is iostream only),
// Win 0x743010-0x7437a0.
#include "Vx/VxInternalIDIntersectSubscriber.h"
#include "Vx/VxCollisionGeometry.h"

namespace Vx {

namespace {

// the internal ID of a collision geometry (Mac +0x40, Win +0x58) (name inferred)
inline const int& internalID(VxCollisionGeometry* cg) { return *(const int*)&cg->m40; }

inline void order(const int& a, const int& b, int& lo, int& hi)
{
    if (a < b) {
        lo = a;
        hi = b;
    } else {
        lo = b;
        hi = a;
    }
}

} // namespace

// Mac D2 0x45ba1c (D1 0x45bc34, D0 0x45bc48), Win 0x743400 (sdd 0x743760)
VxInternalIDIntersectSubscriber::~VxInternalIDIntersectSubscriber()
{
    for (VxIntersectSubscriberList** l = mOwnedLists.begin(); l != mOwnedLists.end(); ++l)
        if (*l != 0)
            delete *l;
    for (ListArray** r = mOwnedRows.begin(); r != mOwnedRows.end(); ++r)
        if (*r != 0) {
            // the rows' storage only (their elements are lists owned above)
            if ((*r)->mBegin != 0)
                vxFree((*r)->mBegin);
            (*r)->mBegin = (*r)->mEnd = (*r)->mCapacity = 0;
            delete *r;
        }
}

// Mac 0x45b64e (inlined on Win)
VxIntersectSubscriberList* VxInternalIDIntersectSubscriber::getList(const int& id, int event) const
{
    const ListArray& a = mLists[event];
    return id < a.size() ? a.mBegin[id] : 0;
}

// Mac 0x45b72e, Win 0x743010
VxIntersectSubscriberList* VxInternalIDIntersectSubscriber::getPairList(const int& id1, const int& id2, int event) const
{
    int lo, hi;
    order(id1, id2, lo, hi);
    const VxArray<ListArray*>& rows = mPairLists[event];
    if (lo < rows.size()) {
        ListArray* row = rows.mBegin[lo];
        if (row != 0 && hi < row->size())
            return row->mBegin[hi];
    }
    return 0;
}

// Mac 0x45b6d8, Win 0x7431b0
void VxInternalIDIntersectSubscriber::notifyDisjoint(const int& id, int event, VxIntersectResult* result)
{
    VxIntersectSubscriberList* l = getList(id, event);
    if (l != 0)
        for (VxUniverse::IntersectSubscriber** s = l->mSubscribers.begin(); s != l->mSubscribers.end(); ++s)
            (*s)->notifyDisjoint(event, result);
}

// Mac 0x45b67c, Win 0x743260
void VxInternalIDIntersectSubscriber::notifyIntersect(const int& id, int event, VxIntersectResult* result,
                                                      VxDynamicsResponseInput* response)
{
    VxIntersectSubscriberList* l = getList(id, event);
    if (l != 0)
        for (VxUniverse::IntersectSubscriber** s = l->mSubscribers.begin(); s != l->mSubscribers.end(); ++s)
            (*s)->notifyIntersect(event, result, response);
}

// Mac 0x45b832, Win 0x743210
void VxInternalIDIntersectSubscriber::notifyPairDisjoint(const int& id1, const int& id2, int event,
                                                         VxIntersectResult* result)
{
    VxIntersectSubscriberList* l = getPairList(id1, id2, event);
    if (l != 0)
        for (VxUniverse::IntersectSubscriber** s = l->mSubscribers.begin(); s != l->mSubscribers.end(); ++s)
            (*s)->notifyDisjoint(event, result);
}

// Mac 0x45b7ce, Win 0x7432c0
void VxInternalIDIntersectSubscriber::notifyPairIntersect(const int& id1, const int& id2, int event,
                                                          VxIntersectResult* result, VxDynamicsResponseInput* response)
{
    VxIntersectSubscriberList* l = getPairList(id1, id2, event);
    if (l != 0)
        for (VxUniverse::IntersectSubscriber** s = l->mSubscribers.begin(); s != l->mSubscribers.end(); ++s)
            (*s)->notifyIntersect(event, result, response);
}

// Mac 0x45b8b0, Win 0x743310
void VxInternalIDIntersectSubscriber::notifyDisjoint(int event, VxIntersectResult* result)
{
    VxCollisionGeometry* cg1;
    VxCollisionGeometry* cg2;
    result->getCollisionGeometries(cg1, cg2);
    const int& id1 = internalID(cg1);
    const int& id2 = internalID(cg2);
    notifyDisjoint(id1, event, result);
    if (id1 != id2)
        notifyDisjoint(id2, event, result);
    notifyPairDisjoint(id1, id2, event, result);
}

// Mac 0x45b95c, Win 0x743380
void VxInternalIDIntersectSubscriber::notifyIntersect(int event, VxIntersectResult* result, VxDynamicsResponseInput* response)
{
    VxCollisionGeometry* cg1;
    VxCollisionGeometry* cg2;
    result->getCollisionGeometries(cg1, cg2);
    const int& id1 = internalID(cg1);
    const int& id2 = internalID(cg2);
    notifyIntersect(id1, event, result, response);
    if (id1 != id2)
        notifyIntersect(id2, event, result, response);
    notifyPairIntersect(id1, id2, event, result, response);
}

// Win 0x743500
void VxInternalIDIntersectSubscriber::addSubscriber(const int& id, int event, VxUniverse::IntersectSubscriber* s)
{
    ListArray& a = mLists[event];
    if (a.size() <= id)
        a.resize(id + 1, 0);
    VxIntersectSubscriberList*& l = a.mBegin[id];
    if (l == 0) {
        l = new VxIntersectSubscriberList;
        mOwnedLists.push_back(l);
    }
    if (!l->contains(s))
        l->insert(s, 0);
}

// Win 0x7435d0
void VxInternalIDIntersectSubscriber::addPairSubscriber(const int& id1, const int& id2, int event,
                                                        VxUniverse::IntersectSubscriber* s)
{
    int lo, hi;
    order(id1, id2, lo, hi);
    VxArray<ListArray*>& rows = mPairLists[event];
    if (rows.size() <= lo)
        rows.resize(lo + 1, 0);
    ListArray*& row = rows.mBegin[lo];
    if (row == 0) {
        row = new ListArray;
        mOwnedRows.push_back(row);
    }
    if (row->size() <= hi)
        row->resize(hi + 1, 0);
    VxIntersectSubscriberList*& l = row->mBegin[hi];
    if (l == 0) {
        l = new VxIntersectSubscriberList;
        mOwnedLists.push_back(l);
    }
    if (!l->contains(s))
        l->insert(s, 0);
}

// Win 0x743140
void VxInternalIDIntersectSubscriber::removeSubscriber(const int& id, int event, VxUniverse::IntersectSubscriber* s)
{
    VxIntersectSubscriberList* l = getList(id, event);
    if (l != 0)
        l->remove(s);
}

// Win 0x743180
void VxInternalIDIntersectSubscriber::removePairSubscriber(const int& id1, const int& id2, int event,
                                                           VxUniverse::IntersectSubscriber* s)
{
    VxIntersectSubscriberList* l = getPairList(id1, id2, event);
    if (l != 0)
        l->remove(s);
}

} // namespace Vx
