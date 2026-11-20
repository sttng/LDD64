// Vortex 5.0.0 build 800 "vortexldd41" - VxInternalIDIntersectSubscriber (restored header):
// the tail of Mac TU 495 (0x45b59c-0x45b67c) + TU 496 (0x45b67c-0x45bfa8);
// Win 0x743010-0x7437a0. Member and method names inferred.
//
// The universe's per-collision-geometry intersect subscriptions; a geometry's "internal ID" is
// VxCollisionGeometry::m40 (Mac +0x40, Win +0x58). VxUniverse creates it lazily (Mac 0x48105a:
// new(0xa4)) for events 2, 0, 3 of the response-model pairs (0, 0) and (0, 1). Per event:
// mLists[event][id] (one geometry) and mPairLists[event][lo][hi] (lo < hi); an event goes to both
// geometries' lists (once when equal) and the pair's. Add / remove are out of line only on Win
// (inlined into VxUniverse on Mac). Mac vtable 0x75dfa8 (D1 0x45bc34 (D2 0x45ba1c), D0 0x45bc48,
// 2 0x45b8b0, 3 0x45b95c); Win 0x743760 sdd, 0x743310, 0x743380. 0xa4 bytes on both.
#ifndef VX_VXINTERNALIDINTERSECTSUBSCRIBER_H
#define VX_VXINTERNALIDINTERSECTSUBSCRIBER_H

#include "Vx/VxSubscriberListResponseHandler.h"

namespace Vx {

class VxInternalIDIntersectSubscriber : public VxUniverse::IntersectSubscriber {
public:
    enum { kEventCount = 4 };
    typedef VxArray<VxIntersectSubscriberList*> ListArray;

    VxInternalIDIntersectSubscriber() {}                // C2 Mac 0x45b59c (C1 0x45b63a), Win 0x7430a0
    virtual ~VxInternalIDIntersectSubscriber();         // Mac D2 0x45ba1c, Win 0x743400 (sdd 0x743760)
    virtual void notifyDisjoint(int event, VxIntersectResult* result); // slot 2 Mac 0x45b8b0, Win 0x743310
    virtual void notifyIntersect(int event, VxIntersectResult* result, VxDynamicsResponseInput* response); // slot 3 Mac 0x45b95c, Win 0x743380

    VxIntersectSubscriberList* getList(const int& id, int event) const;                 // Mac 0x45b64e
    VxIntersectSubscriberList* getPairList(const int& id1, const int& id2, int event) const; // Mac 0x45b72e, Win 0x743010
    void notifyDisjoint(const int& id, int event, VxIntersectResult* result);           // Mac 0x45b6d8, Win 0x7431b0
    void notifyIntersect(const int& id, int event, VxIntersectResult* result, VxDynamicsResponseInput* response); // Mac 0x45b67c, Win 0x743260
    void notifyPairDisjoint(const int& id1, const int& id2, int event, VxIntersectResult* result); // Mac 0x45b832, Win 0x743210
    void notifyPairIntersect(const int& id1, const int& id2, int event, VxIntersectResult* result,
                             VxDynamicsResponseInput* response);                        // Mac 0x45b7ce, Win 0x7432c0

    void addSubscriber(const int& id, int event, VxUniverse::IntersectSubscriber* s);  // Win 0x743500 (first in the list)
    void addPairSubscriber(const int& id1, const int& id2, int event, VxUniverse::IntersectSubscriber* s); // Win 0x7435d0
    void removeSubscriber(const int& id, int event, VxUniverse::IntersectSubscriber* s); // Win 0x743140
    void removePairSubscriber(const int& id1, const int& id2, int event, VxUniverse::IntersectSubscriber* s); // Win 0x743180

    ListArray mLists[kEventCount];                      // +0x04
    VxArray<ListArray*> mPairLists[kEventCount];        // +0x44
    ListArray mOwnedLists;                              // +0x84 every list created (deleted by the dtor)
    VxArray<ListArray*> mOwnedRows;                     // +0x94 every pair row created
};

} // namespace Vx

#endif
