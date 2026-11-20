// Vortex 5.0.0 build 800 "vortexldd41" - VxIntersectSubscriberList, VxSubscriberListResponse,
// VxSubscriberListResponseHandler (restored).
// Mac TU 500 0x45c540-0x45d3d3 (file guess Vx/VxSubscriberListResponseHandler.cpp; the
// std::find / VxArray insert instances 0x45cdd9-0x45d144 are inlined template code here);
// Win 0x7428c0-0x742f90, 0x725740 / 0x7257b0, 0x7c2310.
#include "Vx/VxSubscriberListResponseHandler.h"

#include <algorithm>

namespace Vx {

// --------------------------------------------------------------------------------------
// VxIntersectSubscriberList

// Mac D2 0x45c926 (D1 0x45c992, D0 0x45c9a6), Win 0x742d10 (sdd 0x742f40): the subscribers
// are not owned
VxIntersectSubscriberList::~VxIntersectSubscriberList()
{
}

// Mac 0x45c56e, Win 0x742980
void VxIntersectSubscriberList::notifyDisjoint(int event, VxIntersectResult* result)
{
    for (VxUniverse::IntersectSubscriber** s = mSubscribers.begin(); s != mSubscribers.end(); ++s)
        (*s)->notifyDisjoint(event, result);
}

// Mac 0x45c5aa, Win 0x7429b0
void VxIntersectSubscriberList::notifyIntersect(int event, VxIntersectResult* result, VxDynamicsResponseInput* response)
{
    for (VxUniverse::IntersectSubscriber** s = mSubscribers.begin(); s != mSubscribers.end(); ++s)
        (*s)->notifyIntersect(event, result, response);
}

// Mac 0x45c824 (std::find 0x45ce7c), Win 0x742bb0
bool VxIntersectSubscriberList::contains(VxUniverse::IntersectSubscriber* s) const
{
    VxUniverse::IntersectSubscriber* const* e = mSubscribers.end();
    return std::find(mSubscribers.begin(), e, s) != e;
}

// Mac 0x45c748 (std::find 0x45cdd9), Win 0x742b30 (erase 0x7c2310)
bool VxIntersectSubscriberList::remove(VxUniverse::IntersectSubscriber* s)
{
    VxUniverse::IntersectSubscriber** e = mSubscribers.end();
    VxUniverse::IntersectSubscriber** i = std::find(mSubscribers.begin(), e, s);
    if (i == e)
        return false;
    for (VxUniverse::IntersectSubscriber** j = i + 1; j != e; ++j)
        *(j - 1) = *j;
    --mSubscribers.mEnd;
    return true;
}

// Win 0x742b70
int VxIntersectSubscriberList::indexOf(VxUniverse::IntersectSubscriber* s) const
{
    VxUniverse::IntersectSubscriber* const* b = mSubscribers.begin();
    VxUniverse::IntersectSubscriber* const* e = mSubscribers.end();
    VxUniverse::IntersectSubscriber* const* i = std::find(b, e, s);
    return i != e ? (int)(i - b) : -1;
}

// Mac 0x45cb5a (VxArray insert 0x45d144), Win 0x742e70
void VxIntersectSubscriberList::insert(VxUniverse::IntersectSubscriber* s, unsigned int index)
{
    VxUniverse::IntersectSubscriber** pos = mSubscribers.begin() + index;
    if ((unsigned int)mSubscribers.size() < index)
        pos = mSubscribers.end();
    mSubscribers.insert(pos, s);
}

// --------------------------------------------------------------------------------------
// VxSubscriberListResponse

// Mac D1 0x45d2c4 (D0 0x45d268), Win 0x725740 (sdd 0x7257b0): deletes the filter
VxSubscriberListResponse::~VxSubscriberListResponse()
{
    if (mFilter != 0)
        delete mFilter;
}

// Mac 0x45c5ee, Win 0x7429f0
void VxSubscriberListResponse::notifyDisjoint(int event, VxIntersectResult* result)
{
    if (mFilter != 0 && !mFilter->accept(result))
        return;
    VxIntersectSubscriberList::notifyDisjoint(event, result);
}

// Mac 0x45c632, Win 0x742a30
void VxSubscriberListResponse::notifyIntersect(int event, VxIntersectResult* result, VxDynamicsResponseInput* response)
{
    if (mFilter != 0 && !mFilter->accept(result))
        return;
    VxIntersectSubscriberList::notifyIntersect(event, result, response);
}

// --------------------------------------------------------------------------------------
// VxSubscriberListResponseHandler

// Mac D2 0x45cab4 (D1 0x45cb46, D0 0x45ca1a), Win 0x742df0 (sdd 0x742f70): deletes the lists
// (the inlined body of slot 4, not a virtual call)
VxSubscriberListResponseHandler::~VxSubscriberListResponseHandler()
{
    VxSubscriberListResponseHandler::removeAllSubscribers();
}

// Mac 0x45cc86, Win 0x742eb0
void VxSubscriberListResponseHandler::addSubscriber(int type1, int type2, int event, VxUniverse::IntersectSubscriber* s,
                                                    unsigned int index)
{
    VxSubscriberListResponse* l = findSubscriberList(type1, type2, event);
    if (l == 0) {
        l = createSubscriberList(type1, type2, event);
        mLists.push_back(l);
    }
    if (!l->contains(s))
        l->insert(s, index);
}

// Mac 0x45c7ac, Win 0x742c50
bool VxSubscriberListResponseHandler::removeSubscriber(int type1, int type2, int event, VxUniverse::IntersectSubscriber* s)
{
    VxSubscriberListResponse* l = findSubscriberList(type1, type2, event);
    if (l == 0)
        return false;
    bool removed = l->remove(s);
    if (l->empty()) {
        removeSubscriberList(l);
        delete l;
    }
    return removed;
}

// Mac 0x45c88e -> 0x45c85a (+ the D0 loop 0x45c718), Win 0x742e60 -> 0x742da0
void VxSubscriberListResponseHandler::removeAllSubscribers()
{
    for (VxSubscriberListResponse** l = mLists.begin(); l != mLists.end(); ++l)
        if (*l != 0)
            delete *l;
    mLists.mEnd = mLists.mBegin;
}

// Mac 0x45c8c2 (std::find 0x45cf24), Win 0x742cd0 (erase 0x7c2310)
bool VxSubscriberListResponseHandler::removeSubscriberList(VxSubscriberListResponse* list)
{
    VxSubscriberListResponse** e = mLists.end();
    VxSubscriberListResponse** i = std::find(mLists.begin(), e, list);
    if (i == e)
        return false;
    for (VxSubscriberListResponse** j = i + 1; j != e; ++j)
        *(j - 1) = *j;
    --mLists.mEnd;
    return true;
}

// Mac 0x45d318, Win 0x7428c0
VxSubscriberListResponse* VxSubscriberListResponseHandler::createSubscriberList(int type1, int type2, int event)
{
    return new VxSubscriberListResponse(type1, type2, event);
}

// Mac 0x45c6ac, Win 0x742aa0
VxSubscriberListResponse* VxSubscriberListResponseHandler::findSubscriberList(int type1, int type2, int event) const
{
    if (mLists.empty())
        return 0;
    int n = mLists.size();
    for (int i = 0; i != n; ++i) {
        VxSubscriberListResponse* l = mLists.mBegin[i];
        if (event == l->mEvent) {
            if (l->mType1 == type1 && l->mType2 == type2)
                return l;
            if (l->mType2 == type1 && l->mType1 == type2)
                return l;
        }
    }
    return 0;
}

// Win 0x742c00
int VxSubscriberListResponseHandler::getSubscriberIndex(int type1, int type2, int event,
                                                        VxUniverse::IntersectSubscriber* s) const
{
    VxSubscriberListResponse* l = findSubscriberList(type1, type2, event);
    return l != 0 ? l->indexOf(s) : -1;
}

} // namespace Vx
